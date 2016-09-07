#ifndef MACHINA_PHYSICALMEMORY_H
#define MACHINA_PHYSICALMEMORY_H


#include <sys/types.h>


#define FRAME_TO_ADDRESS(frame) \
	(void*) ( (frame) * SYS_PAGE_SIZE )
//	(void*) ( ( (frame) + (SYS_PAGE_SIZE - 1) ) & ~(SYS_PAGE_SIZE - 1) )

#define ADDRESS_TOFRAME(address) \
	( (size_t) (address) / SYS_PAGE_SIZE )

namespace machina {


enum PhysicalFrameTag
{
	PFT_FREE = 0x00, // Available for allocation
	PFT_DIRTY,       // Available for allocation (but dirty)
	PFT_KERNEL,
	PFT_RESERVED,    // Reserved by system (according BIOS)
	PFT_KSTACK,      // kernel stack
	PFT_ASTACK,      // Abort stack
	PFT_ISTACK,      // IRQ stack
	PFT_PHYS,        // Physical memory table
	PFT_ALLOCATED,   // Allocated frame
	PFT_KHEAP,       // Allocated frame
	PFT_LAST
};


class TextScreen;


class PhysicalMemory
{
	public:
		PhysicalMemory();

		~PhysicalMemory();

		static PhysicalMemory &getInstance();

		void *allocate(
			size_t count,
			uint8_t tag = PFT_ALLOCATED );

		void free(
			void *address,
			size_t count,
			bool cleanup = false );

		void print(
			TextScreen &screen );

		int printMap(
			TextScreen &screen );

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


}


#endif // MACHINA_PHYSICALMEMORY_H