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

#include <sys/heap.hh>
#include <sys/pmm.hh>
#include <machina/Kernel.hh>
#include <sys/system.h>
#include <sys/uart.hh>
#include <sys/types.h>
#ifndef __arm__
#include <cstdlib>
#endif


#define BLOCK_SIGNATURE      (0x5353U)
#define BLOCK_HEADER_SIZE    ( sizeof(BlockInformation) - sizeof(void*) )
#define HEAP_KB(x)           ( (x) * 1024 )
#define HEAP_MB(x)           ( (x) * 1024 * 1024 )

/*
 * @brief Maximum amount of Heap available for dynamic allocation.
 */
#define HEAP_SIZE          (SYS_KERNEL_HEAP_SIZE)

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
 * The field @c next is only used when the block is in the
 * free list, reducing the overhead from 8 bytes to 4 bytes
 * for allocated blocks.
 */
struct BlockInformation
{
	uint16_t signature;
	uint16_t bucket;
	void *next;
};

#define BLOCK_INFO_SIZE  ((size_t)&(((struct BlockInformation *)0)->next))


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
	{ HEAP_KB(1)    , 0, 0, nullptr }, //   1 KiB
	{ HEAP_KB(4)    , 0, 0, nullptr }, //   4 KiB
	{ HEAP_KB(16)   , 0, 0, nullptr }, //  16 KiB
	{ HEAP_KB(64)   , 0, 0, nullptr }, //  64 KiB
	{ HEAP_KB(128)  , 0, 0, nullptr }, // 128 KiB
	{ HEAP_KB(256)  , 0, 0, nullptr }, // 256 KiB
	{ HEAP_KB(512)  , 0, 0, nullptr }, // 512 KiB
	{ HEAP_MB(1)    , 0, 0, nullptr }, //   1 MiB
	{ HEAP_MB(2)    , 0, 0, nullptr }, //   2 MiB
	{ HEAP_MB(4)    , 0, 0, nullptr }, //   4 MiB
	{ HEAP_MB(16)   , 0, 0, nullptr }, //  16 MiB
	{ HEAP_MB(32)   , 0, 0, nullptr }, //  32 MiB
	{ HEAP_MB(64)   , 0, 0, nullptr }, //  64 MiB
	{ INVALID_BUCKET, 0, 0, nullptr }
};

#define MAX_BUCKETS   (sizeof(HeapBuckets) / sizeof(struct BucketInformation))

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


void heap_initialize()
{
	heapStart = heapOffset = (size_t) pmm_allocate(HEAP_SIZE / SYS_PAGE_SIZE, PFT_KHEAP);
	if (heapStart == 0) KernelPanic();

	heapEnd = heapOffset + HEAP_SIZE;
}


void *heap_allocate(
	size_t size )
{
	if (heapOffset == 0) heap_initialize();

	// we have to take into account the extra bytes for a block header
	size += BLOCK_INFO_SIZE;

	// find out in which bucket the allocation goes
	BucketInformation *bucket = HeapBuckets;
	for (; size > bucket->size; ++bucket);
	if (bucket->size == INVALID_BUCKET) return nullptr;

	size = bucket->size;

	// look for some free block in the bucket
	BlockInformation *block = bucket->entries;
	if (block != nullptr)
	{
		// we can reuse a free entry
		bucket->entries = (BlockInformation*) block->next;
		block->next = 0;
	}
	else
	{
		// check whether we have available memory
		if ( heapOffset + bucket->size > heapEnd ) return nullptr;

		// fill the block information
		block = (BlockInformation *) heapOffset;
		block->signature = BLOCK_SIGNATURE;
		block->bucket = (uint8_t) (bucket - HeapBuckets); // bucket index
		heapOffset += sizeof(BlockInformation) + bucket->size;
	}

	++bucket->count;
	if (bucket->count > bucket->peak)
		bucket->peak = bucket->count;

	return &block->next;
}


void heap_free(
	void *address )
{
	// we need to be sure that the given address is
	// from a valid allocation
	if (address < (void*) heapStart || address >= (void*) heapEnd) return;
	// check the block information (more validations :)
	BlockInformation *block = (BlockInformation*) ( (size_t) address - BLOCK_HEADER_SIZE ) ;
	if (block->signature != BLOCK_SIGNATURE ||
		block->bucket >= MAX_BUCKETS)
		return;

	// put the block in the free list of the corresponding bucket
	BucketInformation *bucket = &HeapBuckets[block->bucket];
	block->next = bucket->entries;
	bucket->entries = block;
}


void heap_dump()
{
	static const char16_t *UNITS[] = { u"B ", u"KB", u"MB" };

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