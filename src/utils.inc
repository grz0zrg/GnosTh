
@ assembler macro to ease words
@ definition for the dictionary
@ and ease return stack controls
@ ------------------------------->

.equ FALSE, 0
.equ TRUE, 1

@ FORTH WORD TYPE
.equ WORD_IMMEDIATE,0           @ a word called immediately (compile time)
.equ WORD_RUNTIME, 1            @ a word called at runtime
.equ WORD_VARIABLE, 2           @ a variable
.equ WORD_ENTRY_POINT, 3        @ entry point

.equ IS_LAST_WORD, TRUE

.equ forth_word_counter, 0
.equ forth_word_previous_flag, WORD_IMMEDIATE

@ generate word dictionary data
@ as a double linked list entry.
@ word code should be put after
@ the macro call, they all
@ end with RET which makes
@ direct word calls works
@ to make self-recursion works
.macro forth_word string, flag=WORD_RUNTIME, last_word=FALSE
    .if forth_word_counter != 0
.if ((FORTH_TRANSPILE == 0) & (FORTH_OUTPUT_CODE == ARM_OUTPUT)) | (forth_word_previous_flag == WORD_IMMEDIATE)
    ret
.else
    .asciz "\x08"               @ reset stack + end IR
    .align 2
.endif
    .endif
    .if \last_word == TRUE
        forth_last_word_addr:
    .endif
    999:
    .if forth_word_counter != 0 @ computed prev. word address
        .if forth_word_counter % 2 == 0
            888:
            .word 666b
            .word 999f
        .else
            666:
            .word 888b
            .word 999f
        .endif
    .else
        888:
        .word 0
        .word 999f
    .endif
    .byte \flag                 @ word flag
    .byte (3f - 2f - 1)         @ word length
    2: .asciz "\string"         @ word
    3:
    .align 2
    .set forth_word_counter, forth_word_counter + 1
    @ ... code
.if ((FORTH_OUTPUT_CODE == LLVM_IR_OUTPUT) & (\flag != WORD_IMMEDIATE))
    .ascii "; \string\n"        @ add debug informations to output code
.endif
.if ((FORTH_OUTPUT_CODE == P5JS_OUTPUT) & (\flag != WORD_IMMEDIATE))
    .ascii "// \string\n"       @ add debug informations to output code
.endif

    .equ forth_word_previous_flag, \flag
.endm

@ to manipulate the return stack
.macro rpush r
    stmdb r0!, { \r }
.endm

.macro rpop r
    ldmia r0!, { \r }
.endm

.macro rgetf r i f
    ldr\f \r, [r0, #\i]
.endm

.macro rpushl r label jump_to
    adr \r, \label
    stmdb r0!, { \r }
    b \jump_to
.endm

.macro rpopf r f
    ldr\f \r, [r0], #4
.endm

.macro ret
    ldmia r0!, { pc }
.endm
