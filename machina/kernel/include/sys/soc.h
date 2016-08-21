#ifndef MACHINA_SOC_H
#define MACHINA_SOC_H


#define CPU_HYP_MODE         (0x001A)
#define CPU_SVC_MODE         (0x0013)
#define CPU_IRQ_MODE         (0x0012)
#define CPU_ABORT_MODE       (0x0017)
#define CPU_UNDEF_MODE       (0x001B)
#define CPU_SYSTEM_MODE      (0x001F)

#define CPU_FIQ_MASK         (0x0040)
#define CPU_IRQ_MASK         (0x0080)
#define CPU_ABORT_MASK       (0x0100)


#if RPIGEN == 1
    #define CPU_IO_BASE          (0x20000000U)
#else
    #define CPU_IO_BASE          (0x3F000000U)
#endif

/*
 * The GPU physical address 0x7E00000 is mapped to the ARM physical address
 * given by CPU_IO_BASE.
 *
 * For more information, consult the "BCM2835 ARM Peripherals" manual.
 */

#define CPU_GPIO_BASE        (CPU_IO_BASE + 0x00200000U) // GPU = 0x7E200000
#define CPU_TIMER_BASE       (CPU_IO_BASE + 0x00003000U) // GPU = 0x7E003000

#define SOC_PM_BASE          (CPU_IO_BASE + 0x00100000U)
#define SOC_PM_RSTC          (SOC_PM_BASE + 0x0000001CU)
#define SOC_PM_WDOG          (SOC_PM_BASE + 0x00000024U)
#define SOC_PM_PASSWORD      (0x5A000000U)
#define SOC_PM_RSTC_WRCFG_FULL_RESET (0x20)


#endif // MACHINA_SOC_H