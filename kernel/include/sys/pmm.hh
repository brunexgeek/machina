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

#ifndef MACHINA_PMM_H
#define MACHINA_PMM_H


#include <sys/types.h>
#include <sys/compiler.h>

/**
 * @brief Codes for physical frame types.
 *
 * The LSB indicates whether the frame is free to be allocated
 * or not.
 */
typedef enum
{
	PFT_FREE       = 0x00, // Available for allocation
	PFT_RESERVED   = 0x01, // Reserved
	PFT_DIRTY      = 0x02, // Available for allocation (but dirty)
	PFT_ALLOCATED  = 0x03, // Allocated frame
} frame_type_t;

struct memory_entry_t
{
	uintptr_t begin;
	uintptr_t end;
};

struct memory_map_t
{
	memory_entry_t bitmap; // fixed
	memory_entry_t kernel;
	memory_entry_t heap;
	memory_entry_t io; // fixed
	struct
	{
		memory_entry_t el0_core0; // fixed
		memory_entry_t el1_core0; // fixed
		memory_entry_t el2_core0; // fixed
		memory_entry_t el0_core1; // fixed
		memory_entry_t el1_core1; // fixed
		memory_entry_t el2_core1; // fixed
		memory_entry_t el0_core2; // fixed
		memory_entry_t el1_core2; // fixed
		memory_entry_t el2_core2; // fixed
	} stack;
};

/**
 * Public table with the memory map.
 */
extern memory_map_t kern_memory_map;

void pmm_initialize();

uintptr_t pmm_allocate( size_t count, frame_type_t tag );

uintptr_t pmm_allocate_aligned( size_t count, size_t alignment, frame_type_t tag );

void pmm_free( uintptr_t address, size_t count );

size_t pmm_size();

size_t pmm_available();

void pmm_register();

void pmm_print();

#endif // MACHINA_PMM_H