/*
 *    Copyright 2016 Bruno Ribeiro
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <machina/Heap.hh>
#include <machina/PMM.hh>
#include <machina/Kernel.hh>
#include <sys/system.h>
#include <sys/uart.hh>
#ifndef __arm__
#include <cstdlib>
#endif


#define BLOCK_SIGNATURE      (0x5353U)

#define BLOCK_HEADER_SIZE    ( sizeof(BlockInformation) - sizeof(void*) )

#define Heap_KB(x)         ( (x) * 1024 )
#define Heap_MB(x)         ( (x) * 1024 * 1024 )

/*
 * @brief Maximum amount of Heap available for dynamic allocation.
 */
#define Heap_SIZE          (SYS_KERNEL_HEAP_SIZE)

/**
 * @brief Size of the invalid bucket.
 *
 * All allocations which does not fit in any other bucket
 * will fit in the invalid bucket (and will result in failure);
 */
#define INVALID_BUCKET      ( (size_t) ~0x00U )


namespace machina {


/*
 * @brief Structure used to hold information about every
 * allocated block.
 *
 * The field @c content is only used when the block is in the
 * free list, reducing the overhead from 8 bytes to 4 bytes
 * for allocated blocks.
 */
struct BlockInformation
{
	uint16_t signature;
	uint16_t bucket;
	void *content;
};


struct BucketInformation
{
	size_t size;
	size_t count;
	size_t peak;
	BlockInformation *entries;
};


/*
 * @brief Buckets containing statistics and freed entries.
 *
 * For each allocation, we adjust the requested size to one of
 * the following buckets (the smaller one in which the size fits).
 * Dynamic allocation will mostly be used when creating C++ objects
 * (large chunks will probably be allocated via physical frames),
 * so we have more granularity in the smallest buckets. Granularity
 * means less waste of space (to some extent).
 */
static BucketInformation HeapBuckets[] =
{
	{ 32            , 0, 0, nullptr }, //  32 bytes
	{ 64            , 0, 0, nullptr }, //  64 bytes
	{ 96            , 0, 0, nullptr }, //  96 bytes
	{ 128           , 0, 0, nullptr }, // 128 bytes
	{ 256           , 0, 0, nullptr }, // 256 bytes
	{ 512           , 0, 0, nullptr }, // 512 bytes
	{ Heap_KB(1)  , 0, 0, nullptr }, //   1 KiB
	{ Heap_KB(4)  , 0, 0, nullptr }, //   4 KiB
	{ Heap_KB(16) , 0, 0, nullptr }, //  16 KiB
	{ Heap_KB(64) , 0, 0, nullptr }, //  64 KiB
	{ Heap_KB(128), 0, 0, nullptr }, // 128 KiB
	{ Heap_KB(256), 0, 0, nullptr }, // 256 KiB
	{ Heap_KB(512), 0, 0, nullptr }, // 512 KiB
	{ Heap_MB(1)  , 0, 0, nullptr }, //   1 MiB
	{ Heap_MB(2)  , 0, 0, nullptr }, //   2 MiB
	{ Heap_MB(4)  , 0, 0, nullptr }, //   4 MiB
	{ Heap_MB(16) , 0, 0, nullptr }, //  16 MiB
	{ Heap_MB(32) , 0, 0, nullptr }, //  32 MiB
	{ Heap_MB(64) , 0, 0, nullptr }, //  64 MiB
	{ INVALID_BUCKET, 0, 0, nullptr }
};


/**
 * @brief Start address of the dynamic Heap region.
 */
static size_t heapStart;

/**
 * @brief Current offset of the dynamic Heap region.
 */
static size_t heapOffset;

/**
 * @brief End address of the dynamic Heap region.
 */
static size_t heapEnd;


Heap Heap::instance;


Heap::Heap()
{
	// nothing to do
}


Heap::~Heap()
{
	// nothing to do
}


Heap &Heap::getInstance()
{
	return instance;
}


void Heap::initialize()
{
	PMM &phys = PMM::getInstance();
	heapStart = heapOffset = (size_t) phys.allocate(Heap_SIZE / SYS_PAGE_SIZE, PFT_KHEAP);
	if (heapStart == 0) KernelPanic();

	heapEnd = heapOffset + Heap_SIZE;
}


void *Heap::allocate(
	size_t size )
{
	if (heapOffset == 0) initialize();

	// find out in which bucket the allocation goes
	BucketInformation *bucket = HeapBuckets;
	for (; size > bucket->size; ++bucket);
	if (bucket->size == INVALID_BUCKET)
		return nullptr;

	size = bucket->size;

	// look for some free block in the bucket
	BlockInformation *block = bucket->entries;
	if (block != nullptr)
	{
		// we can reuse a free entry
		bucket->entries = (BlockInformation*) block->content;
		block->content = 0;
	}
	else
	{
		if ( heapOffset + bucket->size > heapEnd )
			return nullptr;

		// we have to take some bytes from the buffer to 'create'
		// a new block in the bucket
		block = (BlockInformation *) heapOffset;
		block->signature = BLOCK_SIGNATURE;
		block->bucket = (uint8_t) (bucket - HeapBuckets); // bucket index
		heapOffset += sizeof(BlockInformation) + bucket->size;
	}

	++bucket->count;
	if (bucket->count > bucket->peak)
		bucket->peak = bucket->count;

	return &block->content;
}


void Heap::free(
	void *address )
{
	// we need to be sure that the given address is
	// from a valid allocation
	if (address < (void*) heapStart || address >= (void*) heapEnd)
		return;
	// check the block information (more validations :)
	BlockInformation *block = (BlockInformation*) ( (size_t) address - BLOCK_HEADER_SIZE ) ;
	if (block->signature != BLOCK_SIGNATURE ||
		block->bucket >= sizeof(HeapBuckets) - 1)
		return;

	// put the block in the free list of the corresponding bucket
	BucketInformation *bucket = HeapBuckets + block->bucket;
	block->content = bucket->entries;
	bucket->entries = block;
}


void Heap::print()
{
	static const char16_t *UNITS[] = { u"B ", u"kB", u"MB" };

	uart_print(u"Size     Count   Peak\n");
	uart_print(u"-------  ------  --------\n");
	BucketInformation *bucket = HeapBuckets;
	for (; bucket->size != INVALID_BUCKET; ++bucket)
	{
		int unit = 0;
		size_t size = bucket->size;

		for (; size >= 1024; ++unit, size /= 1024);

		if (bucket->count == 0 && bucket->peak == 0) continue;
		uart_print(u"%4d %s  %-6d  %-8d\n",
			size,
			UNITS[unit],
			bucket->count,
			bucket->peak );
	}
}


} // machina