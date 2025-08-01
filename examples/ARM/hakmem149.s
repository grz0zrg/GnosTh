@ handcrafted 52 bytes ARM port of hakmem149.th
@ used to compare binary size
@ compile with :
@  arm-linux-gnueabihf-as --warn --fatal-warnings hakmem149.s -o hakmem149.o
@  arm-linux-gnueabihf-ld -T rpi0.ld -nostdlib hakmem149.o -o hakmem149.elf
@  arm-linux-gnueabihf-objcopy hakmem149.elf -O binary hakmem149.bin
@ run with U-Boot
.global _start

.section .text
_start:
    mvn r0, #0
    mov r1, #0
    mov r2, r0
    ldr r3, [pc, #28]
loop:
    add r0, r0, r1, lsr #2
    sub r1, r1, r0, asr #1
    add r0, r0, r1, asr #2

    mov r4, r0, lsr #23
    mov r5, r1, lsr #23
    add r4, r4, r5, lsl #10
    str r2, [r3, r4, lsl #2]
    b loop
    .word 0x1E9C6400 @ U-Boot RPI 0 1.3 framebuffer + drawing offset
.section .note.GNU-stack,"",%progbits
