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

#define CPU_IO_BASE          (0x3F000000U)
#define CPU_IO_END           (CPU_IO_BASE + 0x01000000)

#define GPFSEL0              (CPU_IO_BASE + 0x00200000)
#define GPFSEL1              (CPU_IO_BASE + 0x00200004)
#define GPFSEL2              (CPU_IO_BASE + 0x00200008)
#define GPFSEL3              (CPU_IO_BASE + 0x0020000C)
#define GPFSEL4              (CPU_IO_BASE + 0x00200010)
#define GPFSEL5              (CPU_IO_BASE + 0x00200014)
#define GPSET0               (CPU_IO_BASE + 0x0020001C)
#define GPSET1               (CPU_IO_BASE + 0x00200020)
#define GPCLR0               (CPU_IO_BASE + 0x00200028)
#define GPLEV0               (CPU_IO_BASE + 0x00200034)
#define GPLEV1               (CPU_IO_BASE + 0x00200038)
#define GPEDS0               (CPU_IO_BASE + 0x00200040)
#define GPEDS1               (CPU_IO_BASE + 0x00200044)
#define GPHEN0               (CPU_IO_BASE + 0x00200064)
#define GPHEN1               (CPU_IO_BASE + 0x00200068)
#define GPPUD                (CPU_IO_BASE + 0x00200094)
#define GPPUDCLK0            (CPU_IO_BASE + 0x00200098)
#define GPPUDCLK1            (CPU_IO_BASE + 0x0020009C)

#define UART0_BASE   (CPU_IO_BASE + 0x201000)
#define UART0_DR     (UART0_BASE + 0x00)
#define UART0_RSRECR (UART0_BASE + 0x04)
#define UART0_FR     (UART0_BASE + 0x18)
#define UART0_ILPR   (UART0_BASE + 0x20)
#define UART0_IBRD   (UART0_BASE + 0x24)
#define UART0_FBRD   (UART0_BASE + 0x28)
#define UART0_LCRH   (UART0_BASE + 0x2C)
#define UART0_CR     (UART0_BASE + 0x30)
#define UART0_IFLS   (UART0_BASE + 0x34)
#define UART0_IMSC   (UART0_BASE + 0x38)
#define UART0_RIS    (UART0_BASE + 0x3C)
#define UART0_MIS    (UART0_BASE + 0x40)
#define UART0_ICR    (UART0_BASE + 0x44)
#define UART0_DMACR  (UART0_BASE + 0x48)
#define UART0_ITCR   (UART0_BASE + 0x80)
#define UART0_ITIP   (UART0_BASE + 0x84)
#define UART0_ITOP   (UART0_BASE + 0x88)
#define UART0_TDR    (UART0_BASE + 0x8C)

/*
 * The GPU physical address 0x7E00000 is mapped to the ARM physical address
 * given by CPU_IO_BASE.
 *
 * For more information, consult the "BCM2835 ARM Peripherals" manual.
 */

#define CPU_GPIO_BASE            (CPU_IO_BASE + 0x200000U) // GPU = 0x7E200000
//#define CPU_TIMER_BASE           (/*CPU_IO_BASE +*/ 0x20003000U) // GPU = 0x7E003000

#define GPU_MEMORY_BASE	0xC0000000

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

#define SYS_MEMORY_TOTAL         (1U << 30) // bytes
#define SYS_FRAME_SIZE           (4096) // bytes
#define SYS_FRAME_TOTAL          (SYS_MEMORY_TOTAL / SYS_PAGE_SIZE) // frames
#define SYS_BITMAP_START         (0x100) // bytes (16 bytes aligned)
#define SYS_BITMAP_SIZE          (SYS_FRAME_TOTAL * 4 / 8) // bytes (using 4 bits per frame)
#define SYS_BITMAP_END           (SYS_BITMAP_START + SYS_BITMAP_SIZE)

#endif // MACHINA_SOC_H