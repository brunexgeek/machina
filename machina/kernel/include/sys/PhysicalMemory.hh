#ifndef MACHINA_PHYSICALMEMORY_H
#define MACHINA_PHYSICALMEMORY_H


#include <sys/types.h>


namespace machina {


enum MemoryTag
{
	PFT_INVALID = 0x00,
	PFT_FREE,       /// Available for allocation
	PFT_HTAB,
	PFT_RESERVED,  /// Reserved by system (according BIOS)
	PFT_MEM,
	PFT_BAD,
	PFT_PTAB,
	PFT_DMA,       /// DMA buffer
	PFT_SYS,
	PFT_TCB,
	PFT_BOOT,
	PFT_FMAP,
	PFT_STACK,     /// Thread stack
	PFT_KMEM,      /// Kernel allocated (heap) memory
	PFT_KMOD,
	PFT_UMOD,
	PFT_VM,        /// Virtual memory
	PFT_HEAP,
	PFT_TIB,
	PFT_PEB,
	PFT_CACHE,
};


class PhysicalMemory
{
	public:
		PhysicalMemory(
			size_t size );

		~PhysicalMemory();

		uint32_t allocate(
			size_t count,
			uint8_t tag );

		void free(
			size_t index,
			size_t count = 1 );

	private:
		/**
		* @brief Number of free frames.
		*/
		size_t freeCount;

		/**
		* @brief Number of free frames.
		*/
		size_t useableCount;

		/**
		* @brief Number of frames in memory.
		*/
		size_t frameCount;

		/**
		* @brief Pointer to the table containing information
		* about all physical frames.
		*/
		size_t *frameTable;
};


}


#endif // MACHINA_PHYSICALMEMORY_H