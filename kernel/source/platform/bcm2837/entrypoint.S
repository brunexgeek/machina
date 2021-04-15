/*
 *    Copyright 2021 Bruno Ribeiro
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// Based on the implementation by Leon de Boer
// <https://github.com/LdB-ECM/Raspberry-Pi-Multicore/blob/master/xRTOS_MMU/SmartStart64.S>

.section ".text.entry"

.global _entry_point

_entry_point:
//
// probe system information
//

    // store the boot address
    adr x12, _entry_point
    ldr x1, =kvar_boot_addr
    str w12, [x1]

    // initialize the available core count
    mov x0, #1
    ldr x1, =kvar_cores_available
    str w0, [x1]

    mov x0, #0x24                 // 4 cores total, 4 cores enabled
.if (__ARM_FP == 14)
    orr x0, x0, #0x40
.endif
    ldr x1, =kvar_soc_info
    str w0, [x1]

//
// Initialize CPU cores
//
aux_entry_point:

    // set up stacks for core 0
    ldr x2, = __EL2_stack_core0
    ldr x3, = __EL1_stack_core0
    ldr x4, = __EL0_stack_core0
    // set the stacks if the current core is 0
    mrs x6, mpidr_el1
    ands x6, x6, #0x3
    beq set_stacks

    // set up stacks for core 1
    ldr x2, = __EL2_stack_core1
    ldr x3, = __EL1_stack_core1
    ldr x4, = __EL0_stack_core1
    // set the stacks if the current core is 1
    cmp x6, #1
    beq set_stacks

    // set up stacks for core 2
    ldr x2, = __EL2_stack_core2
    ldr x3, = __EL1_stack_core2
    ldr x4, = __EL0_stack_core2
    // set the stacks if the current core is 2
    cmp x6, #2
    beq set_stacks

    // set up stacks for core 3
    ldr x2, = __EL2_stack_core3
    ldr x3, = __EL1_stack_core3
    ldr x4, = __EL0_stack_core3
set_stacks:
    // apply the stack addresses
    mov sp, x2      // EL2
    msr sp_el1, x3  // EL1
    msr sp_el0, x4  // EL0

//
// initilize MPID/MPIDR registers for each core
//
    mrs x0, midr_el1
    mrs x1, mpidr_el1
    msr vpidr_el2, x0
    msr   vmpidr_el2, x1

//
// initialize Generic Timers for Core0
//
    mrs x0, cnthctl_el2
    orr x0, x0, #0x3      // enable EL1 to access timers
    msr cnthctl_el2, x0
    msr cntvoff_el2, xzr

//
// Disable coprocessor traps for each core if hard float is present
//
.if (__ARM_FP == 14)
    mov x0, #0x33ff
    msr cptr_el2, x0      // disable coprocessor traps to EL2
    msr hstr_el2, xzr     // disable coprocessor traps to EL2
    mov x0, #3 << 20
    msr cpacr_el1, x0     // enable FP/SIMD at EL1
.endif

//
//  initialize HCR_EL2 so EL1 is 64 bits for each core
//
    mov x0, #(1 << 31)
    msr hcr_el2, x0

//
// initialize SCTLR_EL1 for each core
//
    // RES1 bits (29,28,23,22,20,11) to 1
    // RES0 bits (31,30,27,21,17,13,10,6) and
    // UCI, EE, EOE, WXN, nTWE, nTWI, UCT, DZE, I, UMA,
    // SED, ITD, CP15BEN, SA0, SA, C, A, M to 0
    mov  x0, #0x0800
    movk x0, #0x30d0, lsl #16
    orr  x0, x0, #(0x1 << 2)     // C bit on (data cache).
    orr  x0, x0, #(0x1 << 12)    // I bit on (instruction cache)
    msr  sctlr_el1, x0

//
// return to the EL1_SP1 mode from EL2 in each core
//
    mov    x0, #0x3c5       // EL1_SP1 | D | A | I | F
    msr    spsr_el2, x0     // set spsr_el2 with settings
    adr    x0, exit_el1     // address to exit EL2
    msr    elr_el2, x0      // set elevated return register
    eret                    // call elevated return
exit_el1:

//
// set vector table for EL1 in each core (all cores use same vectors)
//
    ldr x0, =kvar_vector_table
    msr vbar_el1,x0

    // skip to parking code if the current core is 0
    mrs x6, mpidr_el1
    and x6, x6, #0x3
    cbz x6, skip_parking

    // update the core available counter
    ldr x1, =kvar_cores_available
    ldr w0, [x1]
    add w0, w0, #1
    str w0, [x1]
    // jump to parking code
    b  park_core

skip_parking:

    // clear BSS area
    ldr x3, =__begin_bss  // from kernel.ld
    ldr x0, =__end_bss    // from kernel.ld
    cmp    x0, x3
    bcs    .bss_cleared
.bss_zero_loop:
    str    wzr, [x0], 4
    cmp    x3, x0
    bhi    .bss_zero_loop
.bss_cleared:

//
// Sequentially wakes up remaining cores (1-3)
//

    // load the 'available cores' counter
 	ldr x3, =kvar_cores_available
.equ cpu1_addr, 0xe0
    // set the core 1 address to 'aux_entry_point'
	mov x1, #cpu1_addr
	ldr x2, =aux_entry_point
	str	x2, [x1]
	sev
.wait_for_core1:
    // is core 1 ready (kvar_cores_available == 2)?
	ldr	w1, [x3]
	cmp	w1, #2
	bne	.wait_for_core1
.equ cpu2_addr, 0xe8
    // set the core 2 address to 'aux_entry_point'
	mov x1, #cpu2_addr
	ldr x2, =aux_entry_point
	str	x2, [x1]
	sev
.wait_for_core2:
    // is core 2 ready (kvar_cores_available == 3)?
	ldr	w1, [x3]
	cmp	w1, #3
	bne	.wait_for_core2
.equ cpu3_addr, 0xf0
    // set the core 2 address to 'aux_entry_point'
	mov x1, #cpu3_addr
	ldr x2, =aux_entry_point
	str	x2, [x1]
	sev
.wait_for_core3:
    // is core 3 ready (kvar_cores_available == 4)?
	ldr	w1, [x3]
	cmp	w1, #4
	bne	.wait_for_core3

    // call kernel main function
    b kernel_main
hang:
    b hang


.balign    4
park_core:
    wfi
    b park_core
.balign    4
.ltorg


.weak swi_handler_stub
swi_handler_stub:
    b .


.weak irq_handler_stub
irq_handler_stub:
    b .


.weak fiq_handler_stub
fiq_handler_stub:
    b .
.balign    4
.ltorg

.balign	4
.globl kern_wake_core;
.type kern_wake_core, %function
kern_wake_core:
	ubfiz x0, x0, 4, 4						// multiply core number x16
	mov	x2, 140
	movk x2, 0x4000, lsl 16					// Load address of mailbox0 write addr = 0x4000008C
	str	w1, [x2, x0]						// Save caller func address
	sev										// Wake any sleeping core to read mailbox
 	mov	x0, #1								// Return success
	ret
kern_wake_core_fail:
	mov x0, #0								// Return failure
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	kern_wake_core, .-kern_wake_core

//
// Vector Table
//

// align handlers every 0x80 bytes
.macro    vector handler
.balign 0x80
b    \handler
.endm

.balign 0x800
.globl    kvar_vector_table
kvar_vector_table:
    // from current EL with sp_el0
    vector    hang             // Synchronous
    vector    hang             // IRQ
    vector    hang             // FIQ
    vector    hang             // SErrorStub

    // from current EL with sp_elx, x != 0
    vector    swi_handler_stub // Synchronous
    vector    irq_handler_stub // IRQ
    vector    fiq_handler_stub // FIQ
    vector    hang             // SErrorStub

    // from lower EL, target EL minus 1 is AArch64
    vector    hang             // Synchronous
    vector    hang             // IRQ
    vector    hang             // FIQ
    vector    hang             // SErrorStub

    // from lower EL, target EL minus 1 is AArch32
    vector    hang            // Synchronous
    vector    hang            // IRQ
    vector    hang            // FIQ
    vector    hang            // SErrorStub


.section ".data.internal", "aw"
.balign 8

.globl kvar_boot_addr;
kvar_boot_addr : .4byte 0;                // CPU boot address

.globl kvar_cores_available;
kvar_cores_available : .4byte 0;          // CPU cores available

.globl kvar_soc_info;
kvar_soc_info : .4byte 0;                // SoC information

.balign 8
.globl kvar_ccb_vector;
kvar_ccb_vector:
.8byte 0x0;                              // core 0 CCB (Core Control Block) pointer
.8byte 0x0;                              // core 1 CCB (Core Control Block) pointer
.8byte 0x0;                              // core 2 CCB (Core Control Block) pointer
.8byte 0x0;                              // core 3 CCB (Core Control Block) pointer

kvar_fiq_function : .8byte 0;            // fiq function