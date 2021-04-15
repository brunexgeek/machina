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

#include <sys/pmm.hh>
#include <sys/uart.h>
#include <sys/mailbox.h>
#include <sys/bcm2837.h>
#include <sys/system.h>
#include <sys/procfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <mc/stdio.h>


#define PFRAME_GET_TAG(index) \
	( table[index] )

#define PFRAME_SET_TAG(index,value) \
	do { table[index] = value; } while(false)

memory_map_t kern_memory_map;

/**
* @brief Number of free frames.
*/
static size_t free_count;

/**
* @brief Number of frames in memory.
*/
static size_t frame_count;

/**
 * @brief Index of the page in which the allocate funcion
 * will start to look for free pages.
 *
 * This should be equals to @ref SYS_HEAP_START.
 */
static size_t start_index;

/**
* @brief Pointer to the table containing information
* about all physical frames.
*/
static uint8_t *table;


static struct
{
	const char *symbol;
    const char *name;
} PFT_NAMES[] =
{
	{ ".", "Free" },
	{ ".", "Free (dirty)" },
	{ "K", "Kernel image" },
	{ "-", "Reserved" },
	{ "1", "Kernel stack" },
	{ "2", "Abort stack" },
	{ "3", "IRQ stack" },
	{ "T", "Frame table" },
	{ "A", "Physical frame" },
	{ "H", "Kernel heap" },
	{ "V", "Video memory" },
	{ "P", "Page table" },
	{ "x", "Invalid" },
	{ "I", "Memory-mapped I/O" },
};

//#include <sys/uart.h>
static int proc_frames( uint8_t *buffer, int size, void *data )
{
	(void) data;

	char *p = (char*) buffer;
	size_t ps = (size_t) size / sizeof(char);
	// 'sncatprintf' requires a null-terminator
	p[0] = 0;

	size_t type = table[0];
	size_t start = 0;

 	sncatprintf(p, ps, "Start       End         Frames      Description\n");
	sncatprintf(p, ps, "----------  ----------  ----------  ---------------------------------------\n");

	size_t bs = kern_memory_map.bitmap.end - kern_memory_map.bitmap.begin;

	for (size_t i = 0; i <= bs; ++i)
	{
		if (i == bs || table[i] != type)
		{
			sncatprintf(p, ps, "0x%08x  0x%08x  %-10d  %s\n",
				(uint32_t) (start * SYS_PAGE_SIZE),
				(uint32_t) ( (i - 1) * SYS_PAGE_SIZE + (SYS_PAGE_SIZE - 1) ), // to avoid overflow
				(uint32_t) (i - start),
				PFT_NAMES[ GET_PFT_INDEX(type) ].name );
			type = table[i];
			start = i;
		}
	}

	return (int) (strlen(p) * sizeof(char));
}

void pmm_print()
{
	uint8_t buffer[2048];
	proc_frames(buffer, sizeof(buffer), nullptr);
	puts((const char *)buffer);
}

void pmm_register()
{
	procfs_register("/frames", proc_frames, NULL);
}

void kernel_panic( const char *path, int line );

// offsets from 'kernel.ld'
extern size_t _kernel_size;
extern uint8_t _kernel_begin;
extern uint8_t _kernel_end;
extern uint8_t __stack_start_core0__;
extern uint8_t __EL0_stack_core0;
extern uint8_t __EL1_stack_core0;
extern uint8_t __EL2_stack_core0;
extern uint8_t __stack_start_core1__;
extern uint8_t __EL0_stack_core1;
extern uint8_t __EL1_stack_core1;
extern uint8_t __EL2_stack_core1;
extern uint8_t __stack_start_core2__;
extern uint8_t __EL0_stack_core2;
extern uint8_t __EL1_stack_core2;
extern uint8_t __EL2_stack_core2;

static void pmm_map_memory( const memory_tag &arm, const memory_tag &vc )
{
	// update the memory map
	kern_memory_map.bitmap.begin = SYS_BITMAP_START;
	kern_memory_map.bitmap.end = SYS_BITMAP_END;

	kern_memory_map.kernel.begin = (uintptr_t) &_kernel_begin;
	kern_memory_map.kernel.end = ((uintptr_t) &_kernel_end + (SYS_FRAME_SIZE-1)) & (~(SYS_FRAME_SIZE-1));

	kern_memory_map.heap.begin = kern_memory_map.kernel.end;
	kern_memory_map.heap.end = arm.size & (~(SYS_FRAME_SIZE-1));

	kern_memory_map.stack.el0_core0.begin = (uintptr_t) &__stack_start_core0__;
	kern_memory_map.stack.el0_core0.end = (uintptr_t) &__EL0_stack_core0;
	kern_memory_map.stack.el1_core0.begin = (uintptr_t) &__EL0_stack_core0;
	kern_memory_map.stack.el1_core0.end = (uintptr_t) &__EL1_stack_core0;
	kern_memory_map.stack.el2_core0.begin = (uintptr_t) &__EL1_stack_core0;
	kern_memory_map.stack.el2_core0.end = (uintptr_t) &__EL2_stack_core0;

	kern_memory_map.stack.el0_core1.begin = (uintptr_t) &__stack_start_core1__;
	kern_memory_map.stack.el0_core1.end = (uintptr_t) &__EL0_stack_core1;
	kern_memory_map.stack.el1_core1.begin = (uintptr_t) &__EL0_stack_core1;
	kern_memory_map.stack.el1_core1.end = (uintptr_t) &__EL1_stack_core1;
	kern_memory_map.stack.el2_core1.begin = (uintptr_t) &__EL1_stack_core1;
	kern_memory_map.stack.el2_core1.end = (uintptr_t) &__EL2_stack_core1;

	kern_memory_map.stack.el0_core2.begin = (uintptr_t) &__stack_start_core2__;
	kern_memory_map.stack.el0_core2.end = (uintptr_t) &__EL0_stack_core2;
	kern_memory_map.stack.el1_core2.begin = (uintptr_t) &__EL0_stack_core2;
	kern_memory_map.stack.el1_core2.end = (uintptr_t) &__EL1_stack_core2;
	kern_memory_map.stack.el2_core2.begin = (uintptr_t) &__EL1_stack_core2;
	kern_memory_map.stack.el2_core2.end = (uintptr_t) &__EL2_stack_core2;
}

void pmm_initialize()
{
	uart_puts("Initializing physical memory manager...\n");
	// probe the ARM memory map
	struct mailbox_message message1;
	memset(&message1, 0, sizeof(message1));
	if (mailbox_tag(MAILBOX_TAG_GET_ARM_MEMORY, &message1) != 0)
		kernel_panic(__FILE__, __LINE__);
	// probe the GPU memory map
	struct mailbox_message message2;
	memset(&message2, 0, sizeof(message2));
	if (mailbox_tag(MAILBOX_TAG_GET_VC_MEMORY, &message2) != 0)
		kernel_panic(__FILE__, __LINE__);

	uart_print("ARM split: %x - %x\n", message1.tag.memory.base, message1.tag.memory.base + message1.tag.memory.size);
	uart_print("GPU split: %x - %x\n", message2.tag.memory.base, message2.tag.memory.base + message2.tag.memory.size);

	pmm_map_memory(message1.tag.memory, message2.tag.memory);

	// if (split.base != 0 || split.size < 256) panic();
	free_count = frame_count = (kern_memory_map.heap.end - kern_memory_map.heap.begin) / SYS_PAGE_SIZE;
	start_index = kern_memory_map.heap.begin / SYS_PAGE_SIZE;

	//size_t temp = (frame_count + SYS_PAGE_SIZE - 1) & ~(SYS_PAGE_SIZE - 1);
	table = (uint8_t*) kern_memory_map.bitmap.begin;
	uint32_t *end = (uint32_t*) (kern_memory_map.bitmap.begin + 4);
	for (uint32_t *p = (uint32_t*) kern_memory_map.bitmap.begin; p < end; p++)
		*p = 0x18181818;

	// reserve everything before the heap
	for (uintptr_t i = 0; i < kern_memory_map.kernel.end; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_RESERVED );
	// reserve the frames used by physical memory table
	for (uintptr_t i = kern_memory_map.bitmap.begin; i < kern_memory_map.bitmap.end; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_PHYS );
	// reserve the kernel memory
	for (uintptr_t i = kern_memory_map.kernel.begin; i < kern_memory_map.kernel.end; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_KERNEL );
	// sets the free memory region
	for (uintptr_t i = kern_memory_map.heap.begin; i < kern_memory_map.heap.end; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_FREE );

	// reserve the video memory
	for (uintptr_t i = message2.tag.memory.base; i < message2.tag.memory.base + message2.tag.memory.size; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_VIDEO );
	// reserve the IO memory
	for (uintptr_t i = CPU_IO_BASE; i < CPU_IO_END; i += SYS_PAGE_SIZE)
		PFRAME_SET_TAG( i >> 12, PFT_MMIO );
}


uintptr_t pmm_allocate( size_t count, frame_type_t tag )
{
	if (count == 0) return 0;
	if (tag & 0x01) return 0;//panic("Can not allocate with tag PFT_FREE");

	bool update = true;
	// check if we have enough free memory
	if (free_count < count) return 0;
	// find some region with available frames
	for (size_t i = start_index; i < frame_count; ++i)
	{
		if (!IS_FREE_PFT(PFRAME_GET_TAG(i))) continue;

		// check if current region has enough frames
		size_t j = 0;
		for (; j < count && IS_FREE_PFT( PFRAME_GET_TAG(i+j) ); ++j);
		if (j == count)
		{
			// reserve frames with given tag
			for (j = 0; j < count; ++j)
				PFRAME_SET_TAG(i+j, tag);
			// decrease the free frames counter
			free_count -= count;

			if (update) start_index = i + count;
			return FRAME_TO_ADDRESS(i);
		}
		else
		{
			i += j;
			update = false;
		}
	}

	return 0;
}


uintptr_t pmm_allocate_aligned( size_t count, size_t alignment, frame_type_t tag )
{
	if (count == 0) return 0;
	if (tag & 0x01) return 0;//panic("Can not allocate with tag PFT_FREE");

	// check if we have enough free memory
	if (free_count < count) return 0;
	// find some region with available frames
	size_t i = start_index + (alignment - (start_index % (alignment + 1)));
	for (; i < frame_count; i += alignment)
	{
		if (!IS_FREE_PFT(PFRAME_GET_TAG(i))) continue;

		// check if current region has enough frames
		size_t j = 0;
		for (; j < count && IS_FREE_PFT(PFRAME_GET_TAG(i+j)); ++j);
		if (j == count)
		{
			// reserve frames with given tag
			for (j = 0; j < count; ++j)
				PFRAME_SET_TAG(i+j, tag);
			// decrease the free frames counter
			free_count -= count;

			return FRAME_TO_ADDRESS(i);
		}
	}

	return 0;
}

void pmm_free( uintptr_t address, size_t count )
{
	size_t index = ADDRESS_TOFRAME(address);
	if (index >= frame_count || count == 0) return;

	for (size_t i = index, t = index + count; i < t; ++i)
	{
		PFRAME_SET_TAG(i, PFT_DIRTY);
		free_count++;
	}

	if (index < start_index) start_index = index;
}

size_t pmm_total()
{
	return frame_count;
}

size_t pmm_available()
{
	return free_count;
}
