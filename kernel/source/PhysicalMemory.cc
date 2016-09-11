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
	( this->frameTable[index] )

#define PFRAME_SET_TAG(index,value) \
	{ this->frameTable[index] = value; }


namespace machina {



static struct
{
	const char16_t *symbol;
    const char16_t *name;
} PFT_NAMES[PFT_LAST] =
{
	{ u".", u"Free" },
	{ u".", u"Free (dirty)" },
	{ u"K", u"Kernel image" },
	{ u"-", u"Reserved" },
	{ u"1", u"Kernel stack" },
	{ u"2", u"Abort stack" },
	{ u"3", u"IRQ stack" },
	{ u"T", u"Frame table" },
	{ u"A", u"Alocated (physical frame)" },
	{ u"H", u"Alocated (kernel heap)" }
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
	frameTable = (uint8_t*) SYS_HEAP_START - temp;
	mc_memset(frameTable, PFT_FREE, frameCount);
#else
	frameTable = (uint8_t*) calloc(1, temp);
#endif

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
	size_t type = frameTable[0];
	size_t start = 0;

 	screen.print(u"Start       End         Frames      Description\n");
	screen.print(u"----------  ----------  ----------  ---------------------------------------\n");

	for (size_t i = 0; i < frameCount; ++i)
	{
		if (frameTable[i] != type || i + 1 == frameCount)
		{
			screen.print(u"0x%08p  0x%08p  %-10d  %s\n",
				(uint32_t) (start * SYS_PAGE_SIZE),
				(uint32_t) (i * SYS_PAGE_SIZE - 1),
				(uint32_t)(i - start),
				PFT_NAMES[type].name );
			type = frameTable[i];
			start = i;
		}
	}
}


PhysicalMemory::~PhysicalMemory()
{
	// nothing to do
}


void *PhysicalMemory::allocate(
	size_t count,
	uint8_t tag )
{
	if (count == 0) return nullptr;
	if (tag == PFT_FREE) return nullptr;//panic("Can not allocate with tag PFT_FREE");

	// check if we have enough free memory
	if (freeCount < count) return nullptr;
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
#ifdef __arm__
			return FRAME_TO_ADDRESS(i);
#else
			return calloc(1, count * SYS_PAGE_SIZE);
#endif
		}
		else
			i += j;
	}

	return nullptr;
}


void PhysicalMemory::free(
	void *address,
	size_t count,
	bool cleaup )
{
	size_t index = ADDRESS_TOFRAME(address);
	if (index >= frameCount || count == 0) return;

#ifndef __arm__
	::free(address);
#endif

	for (size_t i = index, t = index + count; i < t; ++i)
	{
		PFRAME_SET_TAG(i, PFT_FREE);
		freeCount++;
	}
}


} // machina