
.text

.balign	4
.globl memzero32;
.type memzero32, %function
memzero32:
    ldr x3, =zeroes
    mov x4, x0
    ldr q0, [x3]
    ldr q1, [x3]
loop:
    sub  x1, x1, 32
    stp  q0, q1, [x4], 32
    cbnz x1, loop
    ret
zeroes:
    .8byte 0x0
    .8byte 0x0
.balign	4
.ltorg
.size	memzero32, .-memzero32
