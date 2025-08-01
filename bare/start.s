@ stub to copy program data and jump to transpiled code
.global _start
.extern _binary_program_bin_start
.extern _binary_program_bin_end

.section .text
_start:
    @ == UBOOT API INITIALIZATION
    bl UBOOT_API_SETUP
    b 0f                        @ skip U-Boot code
        @ first label of this file is called by U-Boot exceptions handler to get U-Boot debug report on ARM exception ! (custom U-Boot patch)
        .include "../uboot/api.inc"

        @ API JUMP TABLE
        UBOOT_API_SYSCALL UBOOT_API_GET_TIMER 1
        mov r15, r14            @ jump back
        UBOOT_API_SYSCALL UBOOT_API_PUTS 1
        mov r15, r14            @ jump back
    0:

    @ =============== DATA LOADER
    ldr r0, =_binary_program_data_bin_start
    ldr r1, =_binary_program_data_bin_end
    ldr r2, =0x00fa2974         @ from r4 see program.s; should be updated with data start address
    ldr sp, =0x1e84800          @ should not overlap data, program or U-Boot stuff
gnosth_data_loader:
    cmp r0, r1
    beq gnosth_data_loader_done
    ldrb r3, [r0], #1
    strb r3, [r2], #1
    b gnosth_data_loader

gnosth_data_loader_done:
    bl main
    b .

@ avoid link time warning
.section .note.GNU-stack,"",%progbits
