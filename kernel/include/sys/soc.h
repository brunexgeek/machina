#ifndef MACHINA_SOC_H
#define MACHINA_SOC_H


#define CPU_FIQ_MODE         (0x0011)
#define CPU_HYP_MODE         (0x001A)
#define CPU_SVC_MODE         (0x0013)
#define CPU_IRQ_MODE         (0x0012)
#define CPU_ABORT_MODE       (0x0017)
#define CPU_UNDEF_MODE       (0x001B)
#define CPU_SYSTEM_MODE      (0x001F)

#define CPU_FIQ_MASK         (0x0040)
#define CPU_IRQ_MASK         (0x0080)
#define CPU_ABORT_MASK       (0x0100)

#define BCMXXXX_EXCEPTTBL    (0x00000004)


#if RPIGEN == 1
    #define CPU_IO_BASE          (0x20000000U)
#else
    #define CPU_IO_BASE          (0x3F000000U)
#endif

#define CPU_IO_END               (CPU_IO_BASE + 0x01000000)

/*
 * The GPU physical address 0x7E00000 is mapped to the ARM physical address
 * given by CPU_IO_BASE.
 *
 * For more information, consult the "BCM2835 ARM Peripherals" manual.
 */

#define CPU_GPIO_BASE            (CPU_IO_BASE + 0x200000U) // GPU = 0x7E200000
#define CPU_TIMER_BASE           (/*CPU_IO_BASE +*/ 0x20003000U) // GPU = 0x7E003000


#define GPU_CACHED_BASE		0x40000000
#define GPU_UNCACHED_BASE	0xC0000000

#if (RPIGEN == 1)
	#ifdef ENABLE_GPU_L2
		#define GPU_MEMORY_BASE	GPU_CACHED_BASE
	#else
		#define GPU_MEMORY_BASE	GPU_UNCACHED_BASE
	#endif
#else
	#define GPU_MEMORY_BASE	GPU_UNCACHED_BASE
#endif


#define SOC_PM_BASE              (CPU_IO_BASE + 0x00100000U)
#define SOC_PM_RSTC              (SOC_PM_BASE + 0x0000001CU)
#define SOC_PM_WDOG              (SOC_PM_BASE + 0x00000024U)
#define SOC_PM_PASSWORD          (0x5A000000U)
#define SOC_PM_RSTC_WRCFG_FULL_RESET (0x20)


#define SOC_MAILBOX_BASE         (CPU_IO_BASE + 0x0000B880U) // GPU = 0x7E00B880
#define SOC_MAILBOX_READ         (SOC_MAILBOX_BASE)
#define SOC_MAILBOX_POLL         (SOC_MAILBOX_BASE + 0x00000018U) // GPU = 0x7E00B898
#define SOC_MAILBOX_WRITE        (SOC_MAILBOX_BASE + 0x00000020U) // GPU = 0x7E00B8A0

#define SOC_MAILBOX_STATUS_EMPTY (0x40000000)
#define SOC_MAILBOX_STATUS_FULL  (0x80000000)

#define MAILBOX_CHANNEL_POWER    (0x00)
#define MAILBOX_CHANNEL_DISPLAY  (0x01)
#define MAILBOX_CHANNEL_ARM      (0x08)  // ARM to VC

#endif // MACHINA_SOC_H