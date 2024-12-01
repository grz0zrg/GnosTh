@ a set of standalone debug code
@ mostly used during development
@ on some cases (caching issues)
@ use with caution due to direct
@ PC manipulation (no labels)

@ uboot API putc
.macro putc c
    push {r0}
    mov r0, #\c
    putrc r0
    pop {r0}
.endm

@ uboot API putc with register
.macro putrc r
    push {r0-r12, lr}
    mov r3, \r
    mov r1, #0                  @ retval
    str r3, [pc, #8]
    ldr r5, [pc]
    add pc, #4
        .word 0x1bbab1c8        @ api address (change to yours)
        .word 0                 @ value (is replaced)
    sub r2, pc, #12             @ value address
    mov r0, #2                  @ PUTC
    mov lr, pc
    ldr pc, [r5, #16]
    pop {r0-r12, lr}
.endm

@ print memory area per 32b word
.macro print_mem4 start end
    push {r0-r12, lr}
    ldr r2, [pc, #8]
    ldr r1, [pc]
    add pc, #4
        .word \start
        .word \end
    ldr r0, [r1], #4
    print_hex r0
    putc ' '
    cmp r1, r2
    suble pc, pc, #196
    pop {r0-r12, lr}
.endm

@ print reg. hexadecimal value
.macro print_hex r
    push {r1, r2, r3}           @ save registers
    mov r0, \r
    mov r1, pc                  @ load the address of hex digits string
    add pc, #12
        .ascii "0123456789ABCDEF"
    mov r2, #28                 @ 32 bits, 8 hex digits, so we start at 28 (7th nibble)
        mov r3, r0
        lsr r3, r2              @ shift the desired nibble to the least significant position
        and r3, r3, #0xf        @ mask to get only 4 bits (1 nibble)

        ldrb r3, [r1, r3]       @ load the corresponding hex character from the table
        putrc r3

        subs r2, #4             @ move to the next nibble (4 bits less)
        subpl pc, pc, #80       @ loop until all nibbles are printed
    pop {r1, r2, r3}            @ restore registers
.endm
