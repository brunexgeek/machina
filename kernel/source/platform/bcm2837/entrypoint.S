.section ".text.entry"

.global _entry_point

_entry_point:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfi
    b       1b
2:  // cpu id == 0

    // initialize the stack
    ldr     x5, =_entry_point
    mov     sp, x5

    // clear bss
    ldr     x1, =__begin_bss
    ldr     w2, =__size_bss
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

    // jump to C++ code
4:  bl      kernel_main
    // for failsafe, halt this core too
    b       1b