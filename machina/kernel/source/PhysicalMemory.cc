#include <sys/PhysicalMemory.hh>
#include <sys/errors.h>


/*#define PFRAME_GET_TAG(index) \
	( this->frameTable[index] & 0xFF )

#define PFRAME_SET_TAG(index,value) \
	{ this->frameTable[index] = (this->frameTable[index] & (~0xff)) | (value & 0xFF); }*/

#define PFRAME_GET_TAG(index) \
	( this->frameTable[index] )

#define PFRAME_SET_TAG(index,value) \
	{ this->frameTable[index] = value; }


namespace machina {


PhysicalMemory::PhysicalMemory(
	size_t size ) : freeCount(size), useableCount(size), frameCount(size)
{
	PFRAME_SET_TAG(0, PFT_RESERVED);

	// TODO: receive the memory map

	for (size_t i = 1; i < size; ++i)
		PFRAME_SET_TAG(i, PFT_FREE);
}


PhysicalMemory::~PhysicalMemory()
{
	for (size_t i = 0; i < frameCount; ++i)
		PFRAME_SET_TAG(i, PFT_INVALID);
}


uint32_t PhysicalMemory::allocate(
	uint32_t count,
	uint8_t tag )
{
	if (count == 0) return EINVALID;
	if (tag == PFT_FREE) return EINVALID;//panic("Can not allocate with tag PFT_FREE");

	// check if we have enough free memory
	if (freeCount < count) return EEXHAUSTED;
	// find some region with available frames
	for (size_t i = 0; i < frameCount; ++i)
	{
		if (PFRAME_GET_TAG(i) != PFT_FREE) continue;
		// check if current region has enough frames
		size_t j;
		for (j = 0; j < count && PFRAME_GET_TAG(i+j) == PFT_FREE; ++j);
		if (j == count)
		{
			// reserve frames with given tag
			for (j = 0; j < count; ++j)
				PFRAME_SET_TAG(i+j, tag);
			// decrease the used frames counter
			freeCount -= count;
			return i;
		}
	}

	return (~0x00);
}


void PhysicalMemory::free(
	size_t index,
	size_t count )
{
	if (index >= frameCount || count == 0) return;

	for (size_t i = index, t = index + count; i < t; ++i)
	{
		PFRAME_SET_TAG(i, PFT_FREE);
		freeCount++;
	}
}


} // machina