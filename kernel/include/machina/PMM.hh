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


#define FRAME_TO_ADDRESS(frame) \
	( (frame) * SYS_PAGE_SIZE )


#define ADDRESS_TOFRAME(address) \
	( (size_t) (address) / SYS_PAGE_SIZE )


namespace machina {


#define SET_FREE_PFT(index)    ( index << 1 | 1 )
#define SET_USED_PFT(index)    ( index << 1 )
#define GET_PFT_INDEX(code)    ( code >> 1 )


/**
 * @brief Codes for physical frame types.
 *
 * The LSB indicates whether the frame is free to be allocated
 * or not.
 */
enum PhysicalFrameTag
{
	PFT_FREE       = SET_FREE_PFT(0x00), // Available for allocation
	PFT_DIRTY      = SET_FREE_PFT(0x01), // Available for allocation (but dirty)
	PFT_KERNEL     = SET_USED_PFT(0x02), // Kernel image
	PFT_RESERVED   = SET_USED_PFT(0x03), // Reserved
	PFT_KSTACK     = SET_USED_PFT(0x04), // kernel stack
	PFT_ASTACK     = SET_USED_PFT(0x05), // Abort stack
	PFT_ISTACK     = SET_USED_PFT(0x06), // IRQ stack
	PFT_PHYS       = SET_USED_PFT(0x07), // Physical memory table
	PFT_ALLOCATED  = SET_USED_PFT(0x08), // Allocated frame
	PFT_KHEAP      = SET_USED_PFT(0x09), // Allocated frame
	PFT_VIDEO      = SET_USED_PFT(0x0a), // Video memory
	PFT_PTABLE     = SET_USED_PFT(0x0b), // Page table
	PFT_INVALID    = SET_USED_PFT(0x0c), // Invalid frame
	PFT_MMIO       = SET_USED_PFT(0x0d), // Memory-mapped I/O
	//PFT_LAST
};


class TextScreen;


class PMM
{
	public:
		PMM();

		~PMM();

		static PMM &getInstance();

		void initialize();

		size_t allocate(
			size_t count,
			PhysicalFrameTag tag = PFT_ALLOCATED );

		size_t allocate(
			size_t count,
			size_t alignment,
			PhysicalFrameTag tag = PFT_ALLOCATED );

		void free(
			size_t address,
			size_t count,
			bool cleanup = false );

		void print();

		int printMap();

		size_t getMemorySize() const INLINE_ALWAYS;

		size_t getFreeMemory() const INLINE_ALWAYS;

	private:
		/**
		* @brief Number of free frames.
		*/
		size_t freeCount;

		/**
		* @brief Number of frames in memory.
		*/
		size_t frameCount;

		/**
		 * @brief Index of the page in which the allocate funcion
		 * will start to look for free pages.
		 *
		 * This should be equals to @ref SYS_HEAP_START.
		 */
		size_t startIndex;

		/**
		* @brief Pointer to the table containing information
		* about all physical frames.
		*/
		uint8_t *frameTable;
};



inline size_t PMM::getMemorySize() const
{
	return frameCount;
}


inline size_t PMM::getFreeMemory() const
{
	return freeCount;
}


}


#endif // MACHINA_PMM_H