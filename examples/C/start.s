.global _start
.global result

.section .text

_start:
    bl UBOOT_API_SETUP
    b 0f
        @ ==================== U-Boot API
        @ restore U-Boot context (R9)
        uboot_restore_gd:
            adr r9, uboot_gd_addr
            ldr r9, [r9]
            mov pc, lr

        @ restore U-Boot stack
        uboot_restore_sp:
            adr r0, uboot_sp_addr
            ldr sp, [r0]
            mov pc, lr
            
        .equ UBOOT_GD_JT_OFF, 120       @ jump table offset computed from global_data struct (jt member); see structure in include/asm-generic/global_data.h

        @ load jump table address in r8
        @ from saved global data addr
        .macro UBOOT_LOAD_JT_ADDR_IN_R8
            ldr r9, [pc]
            mov pc, pc
            .word uboot_gd_addr
            ldr r9, [r9]                @ load gd address (note: must be loaded in r9 because U-Boot will use it in the calls !)
            ldr r8,[r9,#UBOOT_GD_JT_OFF]@ load jump table address from gd address
        .endm

        @ restore U-Boot sp
        .macro UBOOT_RESTORE_SP
            ldr r0, [pc]
            mov pc, pc
            .word uboot_user_sp_addr
            str sp, [r0]
            ldr r0, [pc]
            mov pc, pc
            .word uboot_sp_addr
            ldr sp, [r0]
        .endm

        @ restore user sp
        .macro UBOOT_RESTORE_USER_SP
            ldr r0, [pc]
            mov pc, pc
            .word uboot_user_sp_addr
            ldr sp, [r0]
        .endm

        @ output a char. to output stream
        @                   r5: character
        .macro UBOOT_PUTC
            push { r0-r4, r6-r12, lr }
            UBOOT_RESTORE_SP
            UBOOT_LOAD_JT_ADDR_IN_R8
            adr lr, 0f
            mov r0, r5
            ldr pc, [r8, #UBOOT_JT_PUTC]@ call putc(r0)
            0:
            UBOOT_RESTORE_USER_SP
            pop { r0-r4, r6-r12, lr }
            pop { r5 }
        .endm

        @ get timer
        @                        r5: base
        @                return: ms in r0
        .macro UBOOT_GET_TIMER
            push { r0-r4, r6-r12, lr }
            UBOOT_RESTORE_SP
            UBOOT_LOAD_JT_ADDR_IN_R8
            adr lr, 0f
            mov r0, r5
            ldr pc, [r8, #UBOOT_JT_GET_TIMER]@ call get_timer(r0) : result in r0
            0:
            mov r5, r0                  @ move result to TOS
            UBOOT_RESTORE_USER_SP
            pop { r0-r4, r6-r12, lr }
        .endm
    0:

    ldr sp, =0x80000

    @ start timer   
    mov r5, #0
    UBOOT_GET_TIMER
    adr r6, timer_value
    str r5, [r6]

    bl main

    @ stop timer; get result
    adr r6, timer_value
    ldr r5, [r6]
    UBOOT_GET_TIMER

@ ################# PRINT INTEGER
    push {r0-r4, lr}

    @ Check if the input value is zero
    cmp r5, #0
    bne not_zero
    mov r5, #'0'         @ Move ASCII '0' into r5
    UBOOT_PUTC           @ Print '0'
    b done_printing

not_zero:
    @ Initialize variables
    ldr r6, =digit_buffer @ Point r6 to the start of the digit buffer
    mov r7, #0            @ Digit counter

    @ Store the number in r0 for manipulation
    mov r0, r5            @ r0 now contains the number to print

    @ Begin digit extraction loop
digit_extract_loop:
    @ Get the remainder of r0 divided by 10
    bl get_remainder      @ Result in r1 (digit), r0 updated with quotient

    @ Store the digit in the buffer
    add r1, r1, #'0'      @ Convert digit to ASCII
    strb r1, [r6], #1     @ Store digit in buffer and increment pointer

    @ Increment digit counter
    add r7, r7, #1

    @ Check if r0 (quotient) is zero
    cmp r0, #0
    bne digit_extract_loop

    @ Now, r6 points to the end of the buffer, and r7 has the digit count

    @ Output the digits in reverse order
output_loop:
    subs r6, r6, #1       @ Decrement pointer
    ldrb r5, [r6]         @ Load digit into r5
    UBOOT_PUTC            @ Print the digit
    subs r7, r7, #1       @ Decrement digit counter
    bne output_loop

done_printing:
    b .
    pop {r0-r4, lr}
    bx lr

    @ Simple subroutine to get quotient and remainder of r0 divided by 10
    @ Updates r0 with the quotient, returns remainder in r1
get_remainder:
    @ Use repeated subtraction to get quotient and remainder
    mov r1, #0            @ Initialize remainder
    mov r2, #10           @ Divisor
    mov r3, #0            @ Initialize quotient

divide_loop:
    cmp r0, r2
    blt division_done
    sub r0, r0, r2        @ Subtract divisor from dividend
    add r3, r3, #1        @ Increment quotient
    b divide_loop

division_done:
    mov r1, r0            @ Remainder is what's left in r0
    mov r0, r3            @ Quotient is in r3
    mov pc, lr            @ Return from subroutine

    @ Data section
digit_buffer:
    .space 20             @ Allocate space for up to 20 digits

.align 2

timer_value:
    .word 0

@ ##### U-Boot API initialization
UBOOT_API_SETUP:                @ MUST BE CALLED AT PROGRAM START TO USE API !
    adr r0, uboot_gd_addr
    str r9, [r0]                @ save global_data struct address which is in r9 when program is launched from U-Boot (with "go" command)
    adr r0, uboot_sp_addr
    str sp, [r0]
    mov pc, lr

uboot_gd_addr:                  @ U-Boot global_data struct address (saved from r9 at start on ARM; see UBOOT_API_SETUP)
    .word 0

uboot_sp_addr:                  @ U-Boot stack address (saved from sp at start on ARM; see UBOOT_API_SETUP)
    .word 0

uboot_user_sp_addr:             @ user stack address (saved before switching to uboot stack address and used to restore user stack)
    .word 0

@ jt offset: include/_exports.h
.equ UBOOT_JT_GETC, 4
.equ UBOOT_JT_TSTC, 8
.equ UBOOT_JT_PUTC, 12
.equ UBOOT_JT_PUTS, 16
.equ UBOOT_JT_GET_TIMER, 48
