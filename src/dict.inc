@ ======================= COMPILE
@forth_word "immediate" WORD_IMMEDIATE
@    mov r5, #WORD_IMMEDIATE
@    strb r5, [r2, #8]

forth_word "entrypoint" WORD_IMMEDIATE
    mov r5, #WORD_ENTRY_POINT
    strb r5, [r2, #8]           @ mark word as entry point
    str r14, [r3]               @ remember code output target before the entrypoint switch
    add r14, r3, #4             @ now point to program code

@ ========================= STACK
forth_word "dup"
    push { r5 }

forth_word "drop"
    pop { r5 }

forth_word "swap"
    mov r6, r5
    pop { r5 }
    push { r6 }

forth_word "over"
    push { r5 }
    ldr r5, [sp, #4]

forth_word "rot"
    pop { r6, r7 }
    push { r6 }
    push { r5 }
    mov r5, r7

@ ======================== MEMORY
forth_word "allot" WORD_IMMEDIATE
    pop { r5 }
    add r10, r4, #4             @ assign data addr. (+4 to skip length)
    adr r7, number_code_generic @ reuse generic number load code
    ldmia r7, {r7-r9}           @ load generated code
    stmia r14!, {r7-r10}        @ copy generated code
    str r5, [r4], #4            @ store length
    add r4, r4, r5              @ offset data
    add r4, #4
    bic r4, #3                  @ align

forth_word "@"
    ldr r5, [r5]

forth_word "!"
    pop { r6 }
    str r6, [r5]
    pop { r5 }

forth_word "db@"
    ldrb r5, [r5]

forth_word "db!"
    pop { r6 }
    strb r6, [r5]
    pop { r5 }

forth_word "dlen"
    push { r5 }
    ldr r5, [r5, #-4]           @ get data length stored prior data ptr

forth_word "#!" WORD_IMMEDIATE
    pop { r5 }
    str r5, [r14], #4           @ store TOS value to current code addr. useful to generate opcodes etc.

@ =========================== I/O
forth_word "tstc"
    UBOOT_API_SYSCALL UBOOT_API_TSTC

forth_word "getc"
    UBOOT_API_SYSCALL UBOOT_API_GETC

forth_word "gett"
    UBOOT_API_SYSCALL UBOOT_API_GET_TIMER

forth_word "dev_enum"
    UBOOT_API_SYSCALL UBOOT_API_DEV_ENUM

forth_word "dev_open"
    UBOOT_API_SYSCALL UBOOT_API_DEV_OPEN

forth_word "dev_close"
    UBOOT_API_SYSCALL UBOOT_API_DEV_CLOSE

forth_word "dev_read"
    UBOOT_API_SYSCALL UBOOT_API_DEV_READ

forth_word "dev_write"
    UBOOT_API_SYSCALL UBOOT_API_DEV_WRITE

forth_word "reset"
    UBOOT_API_SYSCALL UBOOT_API_RESET

forth_word "udelay"             @ test
    UBOOT_API_SYSCALL UBOOT_API_UDELAY

forth_word "envget"             @ test
    UBOOT_API_SYSCALL UBOOT_API_ENV_GET

forth_word "envset"             @ test
    UBOOT_API_SYSCALL UBOOT_API_ENV_SET

forth_word "putc"               @ debug only
    UBOOT_API_SYSCALL UBOOT_API_PUTC

forth_word "puts"               @ debug only
    UBOOT_API_SYSCALL UBOOT_API_PUTS

@ ====================== GRAPHICS
forth_word "pix"
    pop { r6-r9 }
    mla r6, r6, r7, r8
    str r9, [r5, r6, lsl #2]
    pop { r5 }

@ ========================= LOGIC
forth_word "0="
    cmp r5, #0
    moveq r5, #-1
    movne r5, #0

forth_word "0<>"
    cmp r5, #0
    movne r5, #-1
    moveq r5, #0

forth_word "="
    pop { r6 }
    cmp r5, r6
    moveq r5, #-1
    movne r5, #0

forth_word "<>"
    pop { r6 }
    cmp r5, r6
    movne r5, #-1
    moveq r5, #0

forth_word "<"
    pop { r6 }
    cmp r6, r5
    movlt r5, #-1
    movge r5, #0

forth_word ">"
    pop { r6 }
    cmp r6, r5
    movgt r5, #-1
    movle r5, #0

forth_word "<="
    pop { r6 }
    cmp r6, r5
    movle r5, #-1
    movgt r5, #0

forth_word ">="
    pop { r6 }
    cmp r6, r5
    movge r5, #-1
    movlt r5, #0

forth_word "~"
    mvn r5, r5

forth_word "&"
    pop { r6 }
    and r5, r6, r5

forth_word "|"
    pop { r6 }
    orr r5, r6, r5

forth_word "^"
    pop { r6 }
    eor r5, r6, r5

forth_word "<<"
    pop { r6 }
    lsl r5, r6, r5

forth_word ">>"
    pop { r6 }
    asr r5, r6, r5

forth_word ">>>"
    pop { r6 }
    lsr r5, r6, r5

@ ================== FLOW CONTROL
forth_word "call"
    pop { r6 }
    mov r7, r5
    mov r5, r6
    rpush pc
    mov pc, r7

forth_word "loop"
    mov r6, pc
    rpush r6

forth_word "again"
    rgetf pc 0

forth_word "until"
    cmp r5, #0
    pop { r5 }
    rgetf pc 0 eq
    rpop r6

forth_word "for"
    mov r6, r5                  @ start value
    mov r5, #1                  @ step
    mov r7, #0                  @ end value
    rpush r5-r7                 @ push loop parameters on return stack
    rpush pc                    @ push start address of the loop body
    mov r5, r7                  @ "push" iteration value on data stack (stack top is r5)

forth_word "do"                 @ same as for but takes 3 parameters instead of 1
    pop { r6-r7 }
    rpush r5-r7
    rpush pc
    mov r5, r7

forth_word "next"
    rpop r6-r9
    cmp r8, r9                  @ determine iteration direction
    addgt r9, r9, r7            @ positive iteration
    cmpgt r8, r9                @ check stop condition
    sublt r9, r9, r7            @ negative iteration
    cmplt r9, r8                @ check stop condition
    addle pc, pc, #12           @ jump out of the loop when stop condition is reached
    rpush r6-r9                 @ otherwise push parameters back
    push { r5 }
    mov r5, r9                  @ "push" iteration value on data stack
    mov pc, r6                  @ jump to start address of the loop body

forth_word "if" WORD_IMMEDIATE
    adr r5, forth_if_code
    ldmia r5, {r5-r10}          @ load generated code
    stmia r14!, {r5-r10}        @ copy generated code
    rpop r5
    rpush r14                   @ push body length addr.
    mov pc, r5
    forth_if_code:
        cmp r5, #0
        pop { r5 }
        addne pc, #8            @ jump to if body
        forth_skip_over_code:
        ldr r6, [pc]            @ get body length
        add pc, pc, r6          @ jump to either "else" body or "then"
        .word 0                 @ if body length

forth_word "else" WORD_IMMEDIATE
    adr r5, forth_skip_over_code
    ldmia r5, {r5-r7}           @ load generated code
    stmia r14!, {r5-r7}         @ copy generated code
    rpop r5-r6
    sub r8, r14, r6             @ compute body length
    str r8, [r6, #-4]           @ update if body length
    rpush r14                   @ push body length addr.
    mov pc, r5

forth_word "then" WORD_IMMEDIATE
    rpop r5-r6
    sub r8, r14, r6             @ compute body length
    str r8, [r6, #-4]           @ update "if" or "else" body length
    mov pc, r5

forth_word "case"
    pop { r6-r7 }
    cmp r6, r7
    moveq r9, r5                @ get case blocks addr. (when cond. is true)
    popeq { r5 }                @ restore stack
    moveq pc, r9                @ call case blocks
    mov r5, r7                  @ continue with the value to compare to otherwise

@ ===================== REGISTERS
forth_word "A@"
    push { r5 }
    mov r5, r1

forth_word "A!"
    mov r1, r5
    pop { r5 }

forth_word "B@"
    push { r5 }
    mov r5, r10

forth_word "B!"
    mov r10, r5
    pop { r5 }

forth_word "C@"
    push { r5 }
    mov r5, r11

forth_word "C!"
    mov r11, r5
    pop { r5 }

forth_word "D@"
    push { r5 }
    mov r5, r12

forth_word "D!"
    mov r12, r5
    pop { r5 }

@ ==================== ARITHMETIC
forth_word "negate"
    neg r5, r5

forth_word "*"
    pop { r6 }
    mul r5, r6, r5

forth_word "/"
    pop { r6 }
.macro forth_div_code
    mov r7, r5
    mov r5, #0
    0:
        cmp r6, r7
        subge r6, r6, r7
        addge r5, r5, #1
        bge 0b
.endm
    forth_div_code

forth_word "%"
    mov r7, r5
    pop { r5 }
    mov r6, #0
    0:
        cmp r5, r7
        subge r5, r5, r7
        addge r6, r6, #1
        bge 0b

forth_word "*/"
    pop { r6, r7 }
    mul r6, r7, r6
    forth_div_code

forth_word ">>-"
    pop { r6, r7 }
    sub r5, r7, r6, asr r5

forth_word ">>+"
    pop { r6, r7 }
    add r5, r7, r6, asr r5

forth_word "-"
    pop { r6 }
    sub r5, r6, r5

forth_word "+" WORD_RUNTIME IS_LAST_WORD
    pop { r6 }
    add r5, r5, r6
forth_dict_end_addr:
999:
