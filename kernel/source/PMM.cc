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

#include <machina/PMM.hh>
#include <sys/Mailbox.hh>
#include <sys/Screen.hh>
#include <sys/soc.h>
#include <sys/system.h>
#include <sys/errors.h>

#ifndef __arm__
#include <cstdlib>
#include <cstring>
#include <iostream>
#else
#include <mc/string.h>
#include <mc/memory.h>
#endif


#define PFRAME_GET_TAG(index) \
	( this->frameTable[index] )

#define PFRAME_SET_TAG(index,value) \
	{ this->frameTable[index] = value; }


namespace machina {


static struct
{
	const char16_t *symbol;
    const char16_t *name;
} PFT_NAMES[] =
{
	{ u".", u"Free" },
	{ u".", u"Free (dirty)" },
	{ u"K", u"Kernel image" },
	{ u"-", u"Reserved" },
	{ u"1", u"Kernel stack" },
	{ u"2", u"Abort stack" },
	{ u"3", u"IRQ stack" },
	{ u"T", u"Frame table" },
	{ u"A", u"Allocated (physical frame)" },
	{ u"H", u"Allocated (kernel heap)" },
	{ u"V", u"Video memory" },
	{ u"P", u"Page table" },
	{ u"x", u"Invalid" },
	{ u"I", u"Memory-mapped I/O" },
};


static PMM instance;


PMM::PMM()
{
}


void PMM::initialize()
{
#ifdef __arm__
	// probe the ARM memory map
	MemoryTag armSplit;
	Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010005, &armSplit, sizeof(armSplit));
	// probe the GPU memory map
	MemoryTag gpuSplit;
	Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010006, &gpuSplit, sizeof(gpuSplit));
#else
	MemoryTag armSplit;
	armSplit.base = 0;
	armSplit.size = 1024 * 1024 * 1024;

	MemoryTag gpuSplit;
	gpuSplit.base = armSplit.size;
	gpuSplit.size = 16 * 1024 * 1024;
#endif

	// if (split.base != 0 || split.size < 256) panic();
	freeCount = frameCount = (armSplit.size - SYS_HEAP_START) / SYS_PAGE_SIZE;
	startIndex = SYS_HEAP_START / SYS_PAGE_SIZE;

	//size_t temp = (frameCount + SYS_PAGE_SIZE - 1) & ~(SYS_PAGE_SIZE - 1);
#ifdef __arm__
	frameTable = (uint8_t*) SYS_BITMAP_START;
#else
	size_t bla = SYS_BITMAP_SIZE;
	frameTable = (uint8_t*) calloc(1, SYS_BITMAP_SIZE);
#endif
	FillMemory(frameTable, PFT_INVALID, SYS_BITMAP_SIZE);

	// reserve everything before the heap
	for (size_t i = 0; i < SYS_HEAP_START; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_RESERVED );
	// reserve the frames used by physical memory table
	for (size_t i = SYS_BITMAP_START; i < SYS_BITMAP_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_PHYS );
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

	// sets the free memory region
	for (size_t i = SYS_HEAP_START; i < gpuSplit.base; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_FREE );

	// reserve the video memory
	for (size_t i = gpuSplit.base; i < gpuSplit.base + gpuSplit.size; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_VIDEO );
	// reserve the IO memory
	for (size_t i = CPU_IO_BASE; i < CPU_IO_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_MMIO );
}


PMM &PMM::getInstance()
{
	return instance;
}


void PMM::print(
	TextScreen &screen )
{
	size_t type = frameTable[0];
	size_t start = 0;

 	screen.print(u"Start       End         Frames      Description\n");
	screen.print(u"----------  ----------  ----------  ---------------------------------------\n");

	for (size_t i = 0; i <= SYS_BITMAP_SIZE; ++i)
	{
		if (i == SYS_BITMAP_SIZE || frameTable[i] != type)
		{
			screen.print(u"0x%08x  0x%08x  %-10d  %s\n",
				(uint32_t) (start * SYS_PAGE_SIZE),
				(uint32_t) ( (i - 1) * SYS_PAGE_SIZE + (SYS_PAGE_SIZE - 1) ), // to avoid overflow
				(uint32_t) (i - start),
				PFT_NAMES[ GET_PFT_INDEX(type) ].name );
			type = frameTable[i];
			start = i;
		}
	}
}


PMM::~PMM()
{
	// nothing to do
}


size_t PMM::allocate(
	size_t count,
	PhysicalFrameTag tag )
{
	if (count == 0) return 0;
	if (tag & 0x01) return 0;//panic("Can not allocate with tag PFT_FREE");

	bool updateIndex = true;

	// check if we have enough free memory
	if (freeCount < count) return 0;
	// find some region with available frames
	for (size_t i = startIndex; i < frameCount; ++i)
	{
		if (PFRAME_GET_TAG(i) != PFT_FREE) continue;

		// check if current region has enough frames
		size_t j = 0;
		for (; j < count && PFRAME_GET_TAG(i+j) == PFT_FREE; ++j);
		if (j == count)
		{
			// reserve frames with given tag
			for (j = 0; j < count; ++j)
				PFRAME_SET_TAG(i+j, tag);
			// decrease the free frames counter
			freeCount -= count;

			if (updateIndex) startIndex = i + count;
#ifdef __arm__
			return FRAME_TO_ADDRESS(i);
#else
			return (size_t) calloc(1, count * SYS_PAGE_SIZE);
#endif
		}
		else
		{
			i += j;
			updateIndex = false;
		}
	}

	return 0;
}


size_t PMM::allocate(
	size_t count,
	size_t alignment,
	PhysicalFrameTag tag )
{
	if (count == 0) return 0;
	if (tag & 0x01) return 0;//panic("Can not allocate with tag PFT_FREE");

	// check if we have enough free memory
	if (freeCount < count) return 0;
	// find some region with available frames
	size_t i = startIndex + (alignment - (startIndex % (alignment + 1)));
	for (; i < frameCount; i += alignment)
	{
		if (PFRAME_GET_TAG(i) != PFT_FREE) continue;

		// check if current region has enough frames
		size_t j = 0;
		for (; j < count && PFRAME_GET_TAG(i+j) == PFT_FREE; ++j);
		if (j == count)
		{
			// reserve frames with given tag
			for (j = 0; j < count; ++j)
				PFRAME_SET_TAG(i+j, tag);
			// decrease the free frames counter
			freeCount -= count;

#ifdef __arm__
			return FRAME_TO_ADDRESS(i);
#else
			return (size_t) calloc(1, count * SYS_PAGE_SIZE);
#endif
		}
	}

	return 0;
}


void PMM::free(
	size_t address,
	size_t count,
	bool /* cleaup */ )
{
	size_t index = ADDRESS_TOFRAME(address);
	if (index >= frameCount || count == 0) return;

	for (size_t i = index, t = index + count; i < t; ++i)
	{
		PFRAME_SET_TAG(i, PFT_FREE);
		freeCount++;
	}

	if (index < startIndex) startIndex = index;
}


size_t PMM::getMemorySize() const
{
	return frameCount;
}


size_t PMM::getFreeMemory() const
{
	return freeCount;
}


} // machina