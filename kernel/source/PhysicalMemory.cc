#include <sys/PhysicalMemory.hh>
#include <sys/Mailbox.hh>
#include <sys/Screen.hh>
#include <sys/soc.h>
#include <sys/system.h>
#include <sys/errors.h>

#ifndef __arm__
#include <cstdlib>
#include <cstring>
#else
#include <mc/string.h>
#endif

/*#define PFRAME_GET_TAG(index) \
	( this->frameTable[index] & 0xFF )

#define PFRAME_SET_TAG(index,value) \
	{ this->frameTable[index] = (this->frameTable[index] & (~0xff)) | (value & 0xFF); }*/

#define PFRAME_GET_TAG(index) \
	( this->pageTable[index] )

#define PFRAME_SET_TAG(index,value) \
	{ this->pageTable[index] = value; }


namespace machina {



static struct
{
	const char *symbol;
    const char *name;
} PFT_NAMES[PFT_LAST] =
{
	{ ".", "Free" },
	{ "K", "Kernel image" },
	{ "-", "Reserved" },
	{ "1", "Kernel stack" },
	{ "2", "Abort stack" },
	{ "3", "IRQ stack" },
	{ "T", "Frame table" },
	{ "A", "Alocated frame" }
};


static PhysicalMemory instance;



PhysicalMemory::PhysicalMemory()
{
#ifdef __arm__
	// probe the ARM memory map
	MemoryTag split;
	Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010005, &split, sizeof(split));
#else
	MemoryTag split;
	split.base = 0;
	split.size = 1024 * 1024 * 1024;
#endif

	// if (split.base != 0 || split.size < 256) panic();
	freeCount = frameCount = split.size / SYS_PAGE_SIZE;
	startIndex = 0;

	size_t temp = (frameCount + SYS_PAGE_SIZE - 1) & ~(SYS_PAGE_SIZE - 1);
#ifdef __arm__
	pageTable = (uint8_t*) SYS_HEAP_START - temp;
#else
	pageTable = (uint8_t*) calloc(1, temp);
#endif

	memset(pageTable, PFT_FREE, frameCount);

	const void *tableStart = (uint8_t*) SYS_HEAP_START - temp;
	// reserve the pages used by physical memory table
	for (size_t i = (size_t) tableStart; i < SYS_HEAP_START; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_PHYS );

	// reserve everything before the kernel
	for (size_t i = 0; i < SYS_KERNEL_START; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_RESERVED );
	// reserve the kernel memory
	for (size_t i = SYS_KERNEL_START; i < SYS_KERNEL_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_KERNEL );
	// reserve the kernel stack
	for (size_t i = SYS_KERNEL_STACK_START; i < SYS_KERNEL_STACK_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_KSTACK );
	// reserve the Abort stack
	for (size_t i = SYS_ABORT_STACK_START; i < SYS_ABORT_STACK_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_ASTACK );
	// reserve the IRQ stack
	for (size_t i = SYS_IRQ_STACK_START; i < SYS_IRQ_STACK_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_ISTACK );
}


PhysicalMemory &PhysicalMemory::getInstance()
{
	return instance;
}


void PhysicalMemory::print(
	TextScreen &screen )
{
	size_t type = pageTable[0];
	size_t start = 0;

 	screen.print("Start       End         Frames       Description\n");
	screen.print("----------  ----------  ----------  ---------------------------------------\n");

	for (size_t i = 0; i < frameCount; ++i)
	{
		if (pageTable[i] != type || i + 1 == frameCount)
		{
			screen.printHex( (uint32_t) (start * SYS_PAGE_SIZE) );
			screen.print("  ");
			screen.printHex( (uint32_t) (i * SYS_PAGE_SIZE - 1) );
			screen.print("  ");
			screen.printHex( (uint32_t)(i - start) );
			screen.print("  ");
			screen.print( PFT_NAMES[type].name );
			screen.print("\n");
			type = pageTable[i];
			start = i;
		}
	}
}


int PhysicalMemory::printMap(
	TextScreen &screen )
{
	static const size_t LINE_SIZE = 64;

	for (size_t n = 0, tag = 0; n < PFT_LAST; ++n)
	{
		if (PFT_NAMES[n].name == nullptr) continue;
		if (tag != 0 && (tag % 6) == 0) screen.print("\n");
		screen.print(PFT_NAMES[n].symbol);
		screen.print(" = ");
		screen.print(PFT_NAMES[n].name);
		screen.print("  ");
		++tag;
	}

	screen.print("\n\n");

	size_t current = 0;
	bool skip = false;

	for (; current < frameCount; current += LINE_SIZE)
	{

		size_t max = current + LINE_SIZE;
		if (max >= frameCount)
		{
			max = frameCount;
			skip = false;
		}

		bool freeLine = true;
		for (size_t i = current; i < max; ++i)
		{
			uint8_t tag = (uint8_t) PFRAME_GET_TAG(i);
			freeLine &= (tag == PFT_FREE);
		}
		if (freeLine && skip) continue;

		screen.printHex( current * 4096 );
		screen.print(": ");

		for (size_t i = current; i < max; ++i)
		{
			uint8_t tag = (uint8_t) PFRAME_GET_TAG(i);

			if (tag > PFT_LAST || PFT_NAMES[tag].symbol == nullptr)
				screen.print("?");
			else
				screen.print(PFT_NAMES[tag].symbol);
		}

		if (freeLine && current + LINE_SIZE < frameCount)
			screen.print("\n ...\n");
		else
			screen.print('\n');
		skip = freeLine;

		current += LINE_SIZE;
	}

	screen.print("\n");
	return 0;
}


PhysicalMemory::~PhysicalMemory()
{
	// nothing to do
}


size_t PhysicalMemory::allocate(
	size_t count,
	uint8_t tag )
{
	if (count == 0) return (~0x00);
	if (tag == PFT_FREE) return (~0x00);//panic("Can not allocate with tag PFT_FREE");

	// check if we have enough free memory
	if (freeCount < count) return (~0x00);
	// find some region with available frames
	for (size_t i = startIndex; i < frameCount; ++i)
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
	size_t count,
	bool cleaup )
{
	if (index >= frameCount || count == 0) return;

	for (size_t i = index, t = index + count; i < t; ++i)
	{
		PFRAME_SET_TAG(i, PFT_FREE);
		freeCount++;
	}
}


} // machina