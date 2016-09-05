#ifndef MACHINA_PHYSICALMEMORY_H
#define MACHINA_PHYSICALMEMORY_H


#include <sys/types.h>


namespace machina {


enum PageTag
{
	PFT_FREE,      // Available for allocation
	PFT_KERNEL,
	PFT_RESERVED,  // Reserved by system (according BIOS)
	PFT_KSTACK,    // kernel stack
	PFT_ASTACK,    // Abort stack
	PFT_ISTACK,    // IRQ stack
	PFT_PHYS,      // Physical memory table
	PFT_ALLOCATED, // Allocated frame
	PFT_LAST
};


class Display;


class PhysicalMemory
{
	public:
		PhysicalMemory();

		~PhysicalMemory();

		size_t allocate(
			size_t count,
			uint8_t tag = PFT_ALLOCATED );

		void free(
			size_t index,
			size_t count,
			bool cleanup = false );

		void print(
			Display &display );

		int printMap(
			Display &display );

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
		uint8_t *pageTable;
};


}


#endif // MACHINA_PHYSICALMEMORY_H