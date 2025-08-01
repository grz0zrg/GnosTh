@.include "debug/debug.inc"

@ CODE GENERATOR OUTPUT TYPES
.equ ARM_OUTPUT, 0
.equ LLVM_IR_OUTPUT, 1
.equ P5JS_OUTPUT, 2

@ CODE GENERATOR OUTPUT SELECTION
.equ FORTH_OUTPUT_CODE, ARM_OUTPUT

.if (FORTH_OUTPUT_CODE == LLVM_IR_OUTPUT) | (FORTH_OUTPUT_CODE == P5JS_OUTPUT)
.equ FORTH_TRANSPILE, 1
.else
.equ FORTH_TRANSPILE, 0
.endif

@ transpile option to gen. global vars instead of load / store from data section (legacy)
.equ TRANSPILE_GENERATE_GLOBAL_VARS, 0

.if FORTH_OUTPUT_CODE == P5JS_OUTPUT
    .equ TRANSPILE_GENERATE_GLOBAL_VARS, 1
.endif

@ MEMORY LAYOUT CONSTANTS (always multiple of 4)
.equ FORTH_DATA_STACK_SIZE, 4 * 4096
.equ FORTH_RETN_STACK_SIZE, 4 * 4096
.equ FORTH_DICT_SIZE, 1024 * 1000 * 16
.equ PROGRAM_OFFSET, 1024 * 1000 * 16
.if FORTH_TRANSPILE == 1
.equ FORTH_TRANS_STACK_SIZE, 8 * 4096
.equ FORTH_TRANS_STACK_BLOCKS_SIZE, FORTH_TRANS_STACK_SIZE * 4
.equ FORTH_TRANS_REG_STACK_ADDR, (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE + FORTH_RETN_STACK_SIZE + FORTH_TRANS_STACK_SIZE + FORTH_TRANS_STACK_BLOCKS_SIZE)
.endif

.global _start

_start:
    @ == UBOOT API INITIALIZATION
    bl UBOOT_API_SETUP
    b 0f                        @ skip U-Boot code
        .include "uboot/api.inc"@ first label of this file
    0:                          @ is called by U-Boot exceptions handler to get U-Boot debug report on ARM exception ! (custom U-Boot patch)

    @ =============== FORTH SETUP
    @ data stack
    ldr sp, forth_data_stack_addr
    @ return stack
    ldr r0, forth_retn_stack_addr
    @ dict last word
    ldr r2, last_dict_word_addr
    @ compiled program addr.
    ldr r3, compiled_program_addr
    @ data addr.
    ldr r4, forth_data_addr
    @ dict. end
    ldr r14, dict_end_addr

    @ ====== FORTH CALL (compile)
    adr r1, program
    b forth

    @ =========== DICT. END ADDR.
    dict_end_addr:
    .word forth_dict_end_addr
    @ ===== LAST DICT. WORD ADDR.
    last_dict_word_addr:
    .word forth_last_word_addr
    @ ==== FORTH DATA ADDR.
    forth_data_addr:
    .word (_end + FORTH_DICT_SIZE)
    @ ==== FORTH DATA STACK ADDR.
    forth_data_stack_addr:
    .word (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE)
    @ ==== FORTH RETN STACK ADDR.
    forth_retn_stack_addr:
    .word (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE + FORTH_RETN_STACK_SIZE)
    @ ==== COMPILED PROGRAM ADDR.
    compiled_program_addr:
.if FORTH_TRANSPILE == 1
    .word (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE + FORTH_RETN_STACK_SIZE + FORTH_TRANS_STACK_SIZE + FORTH_TRANS_STACK_BLOCKS_SIZE + PROGRAM_OFFSET)
.else
    .word (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE + FORTH_RETN_STACK_SIZE + PROGRAM_OFFSET)
.endif

    @ =================== PROGRAM
    program:                    @ embed sample program code
        .include "program.th.inc"
        .byte 0
        .align 2

    @ ======= CODE GEN. SPECIFICS
.if FORTH_TRANSPILE == 1
    .include "src/transpiler/transpiler.inc"
.endif

    @ ============= COMPILER CODE
    .include "src/gnosth.s"

    @ ================ DICTIONARY
.if FORTH_OUTPUT_CODE == ARM_OUTPUT
    .include "src/dict.inc"
.elseif FORTH_OUTPUT_CODE == LLVM_IR_OUTPUT
    .include "src/transpiler/llvm/dict.inc"
.elseif FORTH_OUTPUT_CODE == P5JS_OUTPUT
    .include "src/transpiler/p5js/dict.inc"
.endif
_end:
