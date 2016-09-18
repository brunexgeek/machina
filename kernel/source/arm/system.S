#include <sys/system.h>
#include <sys/soc.h>
#include <sys/arm.h>


	.macro changeToSVC
#if (RPIGEN > 1)
	// check if the current processor mode is HYP
	mrs r0 , cpsr
	eor r0, r0, #CPU_HYP_MODE
	tst r0, #0x1F
	// ensures the SVC processor mode and mask
	// IRQ and FIQ exceptions [B1-1148, B1-1139]
	and r0 , r0 , #0xFFFFFFE0
	orr r0 , r0 , #(CPU_FIQ_MASK | CPU_IRQ_MASK | CPU_SVC_MODE)
	// if the current processor mode is HYP
	// jump to the code that changes to SVC
	beq 1f
	//...just updates the CPSR with the new masks...
	msr cpsr_c, r0
	b 2f
	//...otherwise, we need to change from HYP to SVC
1:	orr r0, r0, #(CPU_ABORT_MASK)
	adr lr, 2f
	msr spsr_cxsf, r0
	msr ELR_hyp, lr
	eret
2:
#endif

	.endm


	// we create this special section to be used in the linker script
	.section .entryPoint

	.global	system_entryPoint
system_entryPoint:
	changeToSVC
/*
	// tell to MMU the location of our L1 table
	ldr         r0, =kernelL1
	mcr         p15, 0, r0, c2, c0, 0

	// fill the L1 table to create an identity map (ignoring the
	// amount of available memory)
	mov         r1, #4096
	mov         r2, #0x02     //  section entry
1:	str         r2, [r0]
	add         r0, r0, #4
	add         r2, #0x00100000
	subs        r1, #1
	bne         1b

	// disable permissions check
	mov         r1, #0x3
	mcr         p15, 0, r1, c3, c0, 0

	// enable MMU
	mrc         p15, 0, r0, c1, c0, 0
	orr         r0, r0, #0x1
	mcr         p15, 0, r0, c1, c0, 0
*/
	// Note: From this point we have the MMU enabled with an
	//       identity mapping (which means that physical address ==
	//       virtual address). The virtual memory mapping will be
	//       changed later by the MMU class.

	// enters IRQ mode and set its stack base [B1-1140]
	cps         #CPU_IRQ_MODE
	mov         sp, #SYS_IRQ_STACK_END

	// enters Abort mode and set its stack base [B1-1140]
	cps         #CPU_ABORT_MODE
	mov         sp, #SYS_ABORT_STACK_END

	// enters Undefined mode and set its stack base [B1-1140]
	cps         #CPU_UNDEF_MODE
	mov         sp, #SYS_ABORT_STACK_END

	// enters System mode and set its stack base [B1-1139]
	cps         #CPU_SYSTEM_MODE
	mov         sp, #SYS_KERNEL_STACK_END

	b system_initialize


#if (RPIGEN > 1 && defined(ENABLE_MULTI_CORE))

	.global	system_enableCore
system_enableCore:
	changeToSVC

	// retrieves the CPU ID
	mrc p15, 0, r0, c0, c0, 5
	and r0, r0, #SYS_CPU_CORES - 1

	// computes the exception stack offset for the current core
	mov r1, #SYS_EXCEPT_STACK_SIZE
	mul r1, r0, r1

	// change to IRQ mode and set it stack base [B1-1140]
	cps #CPU_IRQ_MODE
	mov r2, #SYS_IRQ_STACK
	add sp, r1, r2

	// change to Abort mode and set it stack base [B1-1140]
	cps #CPU_ABORT_MODE
	mov r2, #SYS_ABORT_STACK
	add sp, r1, r2

	// change to Undefined mode and set it stack base [B1-1140]
	cps #CPU_UNDEF_MODE
	add sp, r1, r2

	// computes the kernel stack offset for the current core
	mov r1, #SYS_EXCEPT_STACK_SIZE
	mul r1, r0, r1

	// change to System mode and set it stack base [B1-1139]
	cps #CPU_SYSTEM_MODE
	mov r2, #SYS_KERNEL_STACK
	add sp, r1, r2

	b system_enableCoreEx

#endif


