@.include "debug/debug.inc"

@ MEMORY LAYOUT CONSTANTS (always multiple of 4)
.equ FORTH_DATA_STACK_SIZE, 1024 * 1000 * 16
.equ FORTH_RETN_STACK_SIZE, 1024 * 1000 * 16
.equ FORTH_DICT_SIZE, 1024 * 1000 * 16
.equ PROGRAM_OFFSET, 1024 * 1000 * 16

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
    .word (_end + FORTH_DICT_SIZE + FORTH_DATA_STACK_SIZE + FORTH_RETN_STACK_SIZE + PROGRAM_OFFSET)

    @ =================== PROGRAM
    program:                    @ embed sample program code
        .include "program.th.inc"
        .byte 0
        .align 2

    @ ============= COMPILER CODE
    .include "src/gnosth.s"

    @ ================ DICTIONARY
    .include "src/dict.inc"
_end:
