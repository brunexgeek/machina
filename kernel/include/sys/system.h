#ifndef MACHINA_SYSTEM_H
#define MACHINA_SYSTEM_H


/*
 * Raspberry Pi 2/3 multi-core
 *
 *
 * Start       End         Pages   Description
 * ----------  ----------  ------  ---------------------------------------------
 * 0x00000000  0x00007FFF  8       Unused, video memory, SOC stuff
 * 0x00008000  0x00107fff  256     Kernel
 * 0x00108000  0x00127fff  128     SVC stack (kernel stack)
 * 0x00128000  0x00187FFF  8       Abort stack
 * 0x00140000  0x00157fff  8       IRQ stack
 * 0x00198000  0x001BFFFF  40      Unused
 * 0x001C8000  0x001FFFFF  64      Physical memory table
 * 0x00200000  0xXXXXXXXX  ?       Heap
 */


/**
 * @brief Size for virtual memory pages.
 */
#define SYS_PAGE_SIZE             (0x1000)  // 4096 bytes

/**
 * @brief Memory offset of the kernel start.
 */
#define SYS_KERNEL_START          (0x8000)

/**
 * @brief Maximum size for kernel binary.
 */
#define SYS_KERNEL_MAX_SIZE       (256 * SYS_PAGE_SIZE) // 1MiB

/**
 * @brief Memory offset of the kernel end.
 *
 * This offset assumes the kernel size is at most @ref SYS_KERNEL_MAX_SIZE .
 */
#define SYS_KERNEL_END            (SYS_KERNEL_START + SYS_KERNEL_MAX_SIZE)

/**
 * @brief Size of the kernel stack.
 */
#define SYS_KERNEL_STACK_SIZE     (32 * SYS_PAGE_SIZE) // 128 KiB

/*
 * @brief Maximum amount of memory available for dynamic allocation.
 *
 * This memory is subtracted from the heap region (staring at @ref SYS_HEAP_START).
 */
#define SYS_KERNEL_HEAP_SIZE       (64 * 1024 * 1024) // 64 MiB

/**
 * @brief Memory offset of the kernel stack.
 *
 * The stack grows downward.
 */
#define SYS_KERNEL_STACK_START \
	(SYS_KERNEL_END)

#define SYS_KERNEL_STACK_END \
	(SYS_KERNEL_STACK_START + SYS_KERNEL_STACK_SIZE)

/**
 * @brief Memory offset of the exception handler stack.
 */
#define SYS_EXCEPT_STACK_SIZE     (8 * SYS_PAGE_SIZE) // 32 KiB


#if (RPIGEN == 1)

/**
 * @brief Amount of CPU cores.
 */
#define SYS_CPU_CORES            1

#else

/**
 * @brief Amount of CPU cores.
 */
#define SYS_CPU_CORES            4

#endif // RPIGEN


/**
 * @brief Memory offset of the exception handler stack.
 *
 * The stack grows downward.
 */
#define SYS_ABORT_STACK_START \
	(SYS_KERNEL_STACK_END)

#define SYS_ABORT_STACK_END \
	(SYS_ABORT_STACK_START + SYS_EXCEPT_STACK_SIZE * (SYS_CPU_CORES))

/**
 * @brief Memory offset of the IRQ handler stack.
 *
 * The stack grows downward.
 */
#define SYS_IRQ_STACK_START \
	(SYS_ABORT_STACK_END)

#define SYS_IRQ_STACK_END \
	(SYS_IRQ_STACK_START + SYS_EXCEPT_STACK_SIZE * (SYS_CPU_CORES))


#define SYS_HEAP_START            (0x00200000)



#endif // MACHINA_SYSTEM_H