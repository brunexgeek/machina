#include <sys/Memory.hh>
#include <sys/PhysicalMemory.hh>
#include <sys/system.h>
#ifndef __arm__
#include <cstdlib>
#endif


#define BLOCK_SIGNATURE      (0x5353U)

#define BLOCK_HEADER_SIZE    ( sizeof(BlockInformation) - sizeof(void*) )

#define MEMORY_KB(x)         ( (x) * 1024 )
#define MEMORY_MB(x)         ( (x) * 1024 * 1024 )

/*
 * @brief Maximum amount of memory available for dynamic allocation.
 */
#define MEMORY_SIZE          ( MEMORY_MB(64) ) // 64 MiB

/**
 * @brief Size of the invalid bucket.
 *
 * All allocations which does not fit in any other bucket
 * will fit in the invalid bucket (and will result in failure);
 */
#define INVALID_BUCKET      ( (size_t) ~0x00U )


namespace machina {


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
static BucketInformation memoryBuckets[] =
{
	{ 32            , 0, nullptr }, //  32 bytes
	{ 64            , 0, nullptr }, //  64 bytes
	{ 96            , 0, nullptr }, //  96 bytes
	{ 128           , 0, nullptr }, // 128 bytes
	{ 256           , 0, nullptr }, // 256 bytes
	{ 512           , 0, nullptr }, // 512 bytes
	{ MEMORY_KB(1)  , 0, nullptr }, //   1 KiB
	{ MEMORY_KB(4)  , 0, nullptr }, //   4 KiB
	{ MEMORY_KB(16) , 0, nullptr }, //  16 KiB
	{ MEMORY_KB(64) , 0, nullptr }, //  64 KiB
	{ MEMORY_KB(128), 0, nullptr }, // 128 KiB
	{ MEMORY_KB(256), 0, nullptr }, // 256 KiB
	{ MEMORY_MB(1)  , 0, nullptr }, //   1 MiB
	{ MEMORY_MB(4)  , 0, nullptr }, //   4 MiB
	{ MEMORY_MB(16) , 0, nullptr }, //  16 MiB
	{ MEMORY_MB(64) , 0, nullptr }, //  64 MiB
	{ INVALID_BUCKET, 0, nullptr }
};


/**
 * @brief Start address of the dynamic memory region.
 */
static size_t heapStart;

/**
 * @brief Current offset of the dynamic memory region.
 */
static size_t heapOffset;

/**
 * @brief End address of the dynamic memory region.
 */
static size_t heapEnd;


Memory Memory::instance;


Memory::Memory()
{
	// nothing to do
}


Memory::~Memory()
{
	// nothing to do
}


Memory &Memory::getInstance()
{
	return instance;
}


void Memory::initialize()
{
#ifdef __arm__
	PhysicalMemory &phys = PhysicalMemory::getInstance();

	heapStart = heapOffset = (size_t) phys.allocate(MEMORY_SIZE / SYS_PAGE_SIZE, PFT_KHEAP);
#else
	heapStart = heapOffset = (size_t) calloc(1, MEMORY_SIZE);
#endif
	if (heapStart == 0) return; // TODO: should panic

	heapEnd = heapOffset + MEMORY_SIZE;
}


void *Memory::allocate(
	size_t size )
{
	if (heapOffset == 0) initialize();

	// find out in which bucket the allocation goes
	BucketInformation *bucket = memoryBuckets;
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
		block->bucket = (uint8_t) (bucket - memoryBuckets); // bucket index
		heapOffset += sizeof(BlockInformation) + bucket->size;
	}

	++bucket->count;
	return &block->content;
}


void Memory::free(
	void *address )
{
	// we need to be sure that the given address is
	// from a valid allocation
	if (address < (void*) heapStart || address >= (void*) heapEnd)
		return;
	// check the block information (more validations :)
	BlockInformation *block = (BlockInformation*) ( (size_t) address - BLOCK_HEADER_SIZE ) ;
	if (block->signature != BLOCK_SIGNATURE ||
		block->bucket >= sizeof(memoryBuckets) - 1)
		return;

	// put the block in the free list of the corresponding bucket
	BucketInformation *bucket = memoryBuckets + block->bucket;
	block->content = bucket->entries;
	bucket->entries = block;
}


} // machina