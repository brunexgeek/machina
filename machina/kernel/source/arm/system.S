#include <sys/System.hh>
#include <sys/soc.h>

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

	// enters IRQ mode and set it stack base [B1-1140]
	cps #CPU_IRQ_MODE
	mov sp, #SYS_IRQ_STACK

	// enters Abort mode and set it stack base [B1-1140]
	cps #CPU_ABORT_MODE
	mov sp, #SYS_ABORT_STACK

	// enters Undefined mode and set it stack base [B1-1140]
	cps #CPU_UNDEF_MODE
	mov sp, #SYS_ABORT_STACK

	// enters System mode and set it stack base [B1-1139]
	cps #CPU_SYSTEM_MODE
	mov sp, #SYS_KERNEL_STACK

	b system_initialize


#if (RPIGEN > 1 && defined(ENABLE_MULTI_CORE))

	.global	system_enableCore
system_enableCore:
	changeToSVC

	mrc p15, 0, r0, c0, c0, 5		/* read MPIDR */
	and r0, r0, #SYS_CPU_CORES-1		/* get CPU ID */

	mov r1, #SYS_EXCEPT_STACK_SIZE	/* calculate exception stack offset for core */
	mul r1, r0, r1

	// enters IRQ mode and set it stack base [B1-1140]
	cps #CPU_IRQ_MODE
	mov r2, #SYS_IRQ_STACK
	add sp, r1, r2

	// enters Abort mode and set it stack base [B1-1140]
	cps #CPU_ABORT_MODE
	mov r2, #SYS_ABORT_STACK
	add sp, r1, r2

	// enters Undefined mode and set it stack base [B1-1140]
	cps #CPU_UNDEF_MODE
	add sp, r1, r2

	mov r1, #SYS_EXCEPT_STACK_SIZE		/* calculate kernel stack offset for core */
	mul r1, r0, r1

	// enters System mode and set it stack base [B1-1139]
	cps #CPU_SYSTEM_MODE
	mov r2, #SYS_KERNEL_STACK
	add sp, r1, r2

	b system_enableCoreEx

#endif


