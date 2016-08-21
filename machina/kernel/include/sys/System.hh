#ifndef MACHINA_SYSTEM_H
#define MACHINA_SYSTEM_H


/**
 * @brief Size for virtual memory pages.
 */
#define SYS_PAGE_SIZE             (4096)

/**
 * @brief Maximum size for kernel binary.
 */
#define SYS_KERNEL_MAX_SIZE       (2 * 1024 * 1024)

/**
 * @brief Memory offset of the kernel start.
 */
#define SYS_KERNEL_START          (0x8000)

/**
 * @brief Memory offset of the kernel end.
 *
 * This offset assumes the kernel size is @ref SYS_KERNEL_MAX_SIZE .
 */
#define SYS_KERNEL_END            (SYS_KERNEL_START + SYS_KERNEL_MAX_SIZE)

/**
 * @brief Size of the kernel stack.
 */
#define SYS_KERNEL_STACK_SIZE     (0x40000)

/**
 * @brief Memory offset of the kernel stack.
 *
 * The stack grows downward.
 */
#define SYS_KERNEL_STACK          (SYS_KERNEL_END + SYS_KERNEL_STACK_SIZE)

/**
 * @brief Memory offset of the exception handler stack.
 */
#define SYS_EXCEPT_STACK_SIZE     (0x8000)


#if (RPIGEN == 1)

/**
 * @brief Amount of CPU cores.
 */
#define SYS_CPU_CORES            1

/**
 * @brief Memory offset of the exception handler stack.
 *
 * The stack grows downward.
 */
#define SYS_ABORT_STACK \
	(SYS_KERNEL_STACK + SYS_EXCEPT_STACK_SIZE)

/**
 * @brief Memory offset of the IRQ handler stack.
 *
 * The stack grows downward.
 */
#define SYS_IRQ_STACK \
	(SYS_ABORT_STACK + SYS_EXCEPT_STACK_SIZE)

#else

/**
 * @brief Amount of CPU cores.
 */
#define SYS_CPU_CORES            4

/**
 * @brief Memory offset of the exception handler stack.
 *
 * The stack grows downward.
 */
#define SYS_ABORT_STACK \
	(SYS_KERNEL_STACK + SYS_KERNEL_STACK_SIZE * (SYS_CPU_CORES-1) + SYS_EXCEPT_STACK_SIZE)

/**
 * @brief Memory offset of the IRQ handler stack.
 *
 * The stack grows downward.
 */
#define SYS_IRQ_STACK \
	(SYS_ABORT_STACK + SYS_EXCEPT_STACK_SIZE * (SYS_CPU_CORES-1) + SYS_EXCEPT_STACK_SIZE)

#endif


#endif // MACHINA_SYSTEM_H