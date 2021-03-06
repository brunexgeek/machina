/*
 *    Copyright 2016-2021 Bruno Ribeiro
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

#include <sys/uart.h>
#include <sys/heap.h>
#include <sys/pmm.hh>
#include <sys/procfs.h>
#include <sys/system.h>
#include <sys/types.h>
#include <mc/stdio.h>
#include <mc/string.h>


#define BLOCK_SIGNATURE      (0x5353U)
#define BLOCK_HEADER_SIZE    ( sizeof(struct block_info_t) - sizeof(void*) )
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


/*
 * @brief Structure used to hold information about every
 * allocated block.
 *
 * The field @c next is only used when the block is in the
 * free list, reducing the overhead from 8 bytes to 4 bytes
 * for allocated blocks.
 */
struct block_info_t
{
	uint16_t signature;
	uint16_t bucket;
	void *next;
};

#define BLOCK_INFO_SIZE  ((size_t)&(((struct block_info_t *)0)->next))


struct bucket_info_t
{
	size_t size;
	size_t count;
	size_t peak;
	struct block_info_t *entries;
};


/*
 * @brief Buckets containing statistics and freed entries.
 *
 * For each allocation, we adjust the requested size to one of
 * the following buckets (the smaller one in which the size fits).
 * Dynamic allocation will mostly be used when creating C++ objects
 * (large chunks will probably be allocated via physical frames),
 * so we have more granularity in the smaller buckets. Granularity
 * means less waste of space (to some extent).
 */
static struct bucket_info_t heap_buckets[] =
{
	{ 32            , 0, 0, NULL }, //  32 bytes
	{ 64            , 0, 0, NULL }, //  64 bytes
	{ 96            , 0, 0, NULL }, //  96 bytes
	{ 128           , 0, 0, NULL }, // 128 bytes
	{ 256           , 0, 0, NULL }, // 256 bytes
	{ 512           , 0, 0, NULL }, // 512 bytes
	{ HEAP_KB(1)    , 0, 0, NULL }, //   1 KiB
	{ HEAP_KB(4)    , 0, 0, NULL }, //   4 KiB
	{ HEAP_KB(16)   , 0, 0, NULL }, //  16 KiB
	{ HEAP_KB(64)   , 0, 0, NULL }, //  64 KiB
	{ HEAP_KB(128)  , 0, 0, NULL }, // 128 KiB
	{ HEAP_KB(256)  , 0, 0, NULL }, // 256 KiB
	{ HEAP_KB(512)  , 0, 0, NULL }, // 512 KiB
	{ HEAP_MB(1)    , 0, 0, NULL }, //   1 MiB
	{ HEAP_MB(2)    , 0, 0, NULL }, //   2 MiB
	{ HEAP_MB(4)    , 0, 0, NULL }, //   4 MiB
	{ HEAP_MB(16)   , 0, 0, NULL }, //  16 MiB
	{ HEAP_MB(32)   , 0, 0, NULL }, //  32 MiB
	{ HEAP_MB(64)   , 0, 0, NULL }, //  64 MiB
	{ INVALID_BUCKET, 0, 0, NULL }
};

#define MAX_BUCKETS   (sizeof(heap_buckets) / sizeof(struct bucket_info_t))

/**
 * @brief Start address of the dynamic Heap region.
 */
static size_t heap_start;

/**
 * @brief Current offset of the dynamic Heap region.
 */
static size_t heap_offset;

/**
 * @brief End address of the dynamic Heap region.
 */
static size_t heap_end;


void kernel_panic( const char *path, int line );

void heap_initialize()
{
	heap_start = heap_offset = (size_t) pmm_allocate(HEAP_SIZE / SYS_PAGE_SIZE, PFT_FREE);
	if (heap_start == 0) kernel_panic(__FILE__, __LINE__);
	heap_end = heap_offset + HEAP_SIZE;
	uart_print("Initializing memmory allocator with heap of %d MB\n", HEAP_SIZE / 1024 / 1024);
}

void *heap_allocate( size_t size )
{
	if (heap_offset == 0) heap_initialize();

	// we have to take into account the extra bytes for a block header
	size += BLOCK_INFO_SIZE;

	// find out in which bucket the allocation goes
	struct bucket_info_t *bucket = heap_buckets;
	for (; size > bucket->size; ++bucket);
	if (bucket->size == INVALID_BUCKET) return NULL;

	size = bucket->size;

	// look for some free block in the bucket
	struct block_info_t *block = bucket->entries;
	if (block != NULL)
	{
		// we can reuse a free entry
		bucket->entries = (struct block_info_t*) block->next;
		block->next = 0;
	}
	else
	{
		// check whether we have available memory
		if ( heap_offset + bucket->size > heap_end ) return NULL;

		// fill the block information
		block = (struct block_info_t *) heap_offset;
		block->signature = BLOCK_SIGNATURE;
		block->bucket = (uint8_t) (bucket - heap_buckets); // bucket index
		heap_offset += sizeof(struct block_info_t) + bucket->size;
	}

	++bucket->count;
	if (bucket->count > bucket->peak)
		bucket->peak = bucket->count;

	return &block->next;
}

void heap_free( void *address )
{
	// we need to be sure that the given address is from a valid allocation
	if (address < (void*) heap_start || address >= (void*) heap_end) return;
	// check the block information (more validations :)
	struct block_info_t *block = (struct block_info_t*) ( (size_t) address - BLOCK_HEADER_SIZE ) ;
	if (block->signature != BLOCK_SIGNATURE ||
		block->bucket >= MAX_BUCKETS)
		return;

	// put the block in the free list of the corresponding bucket
	struct bucket_info_t *bucket = &heap_buckets[block->bucket];
	block->next = bucket->entries;
	bucket->entries = block;
}

static int proc_heap( uint8_t *buffer, int size, void * /* data */ )
{
	static const char *UNITS[] = { "B ", "KB", "MB" };

	char *p = (char*) buffer;
	size_t ps = (size_t) size / sizeof(char);
	// 'sncatprintf' requires a null-terminator
	p[0] = 0;

	sncatprintf(p, ps, "Size     Count   Peak\n");
	sncatprintf(p, ps, "-------  ------  --------\n");
	struct bucket_info_t *bucket = heap_buckets;
	for (; bucket->size != INVALID_BUCKET; ++bucket)
	{
		int unit = 0;
		size_t size = bucket->size;

		for (; size >= 1024; ++unit, size /= 1024);

		if (bucket->count == 0 && bucket->peak == 0) continue;
		sncatprintf(p, ps, "%4d %s  %-6d  %-8d\n",
			size,
			UNITS[unit],
			bucket->count,
			bucket->peak );
	}

	return (int) (strlen(p) * sizeof(char));
}

void heap_register()
{
	procfs_register("/heap", proc_heap, nullptr);
}