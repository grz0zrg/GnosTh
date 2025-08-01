@ ========== U-Boot Context Utils
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

@ ========= U-Boot standalone API
@ a simpler API access
@ using exported function call
@ stored in global_data jt member
@
@ UBOOT_API_INIT must be called
@ at program start to use it
@
@ WARNING: jt offset may change
@ with different U-Boot config. /
@ device target (will likely be
@ different in your use case !)
.equ UBOOT_GD_JT_OFF, 120       @ jump table offset computed from global_data struct (jt member); see structure in include/asm-generic/global_data.h

@ jt offset: include/_exports.h
.equ UBOOT_JT_GETC, 4
.equ UBOOT_JT_TSTC, 8
.equ UBOOT_JT_PUTC, 12
.equ UBOOT_JT_PUTS, 16
.equ UBOOT_JT_GET_TIMER, 48

@ restore U-Boot gd
.macro UBOOT_RESTORE_GD
    ldr r9, [pc]
    mov pc, pc
    .word uboot_gd_addr
    ldr r9, [r9]                @ load gd address (note: must be loaded in r9 because U-Boot will use it in the calls !)
.endm

@ load jump table address in r8
@ from saved global data addr
.macro UBOOT_LOAD_JT_ADDR_IN_R8
    UBOOT_RESTORE_GD
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

@ get a char. from input stream
@                   r5: character
.macro UBOOT_GETC
    push { r5 }
    push { r0-r4, r6-r12, lr }
    UBOOT_RESTORE_SP
    UBOOT_LOAD_JT_ADDR_IN_R8
    adr lr, 0f
    ldr pc, [r8, #UBOOT_JT_GETC]@ call getc() : result in r0
    0:
    mov r5, r0                  @ move result to TOS
    UBOOT_RESTORE_USER_SP
    pop { r0-r4, r6-r12, lr }
.endm

@ test input stream for input
.macro UBOOT_TSTC
    push { r5 }
    push { r0-r4, r6-r12, lr }
    UBOOT_RESTORE_SP
    UBOOT_LOAD_JT_ADDR_IN_R8
    adr lr, 0f
    ldr pc, [r8, #UBOOT_JT_TSTC]@ call tstc() : result in r0 (-1 if char. available 0 if not)
    0:
    mvn r5, r0
    add r5, r5, #1              @ move result to TOS
    UBOOT_RESTORE_USER_SP
    pop { r0-r4, r6-r12, lr }
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

@ output a NULL terminated string
@ to output stream
@                r5: string addr.
.macro UBOOT_PUTS
    push { r0-r4, r6-r12, lr }
    UBOOT_RESTORE_SP
    UBOOT_LOAD_JT_ADDR_IN_R8
    adr lr, 0f
    mov r0, r5
    ldr pc, [r8, #UBOOT_JT_PUTS]@ call puts(r0)
    0:
    UBOOT_RESTORE_USER_SP
    pop { r0-r4, r6-r12, lr }
    pop { r5 }
.endm

@ get timer
@                        r5: base
@                return: ms in r5
.macro UBOOT_GET_TIMER
    push { r0-r4, r6-r12, lr }
    UBOOT_RESTORE_SP
    UBOOT_LOAD_JT_ADDR_IN_R8
    adr lr, 0f
    mov r0, r5
    ldr pc, [r8, #UBOOT_JT_GET_TIMER]         @ call get_timer(r0) : result in r0
    0:
    mov r5, r0                  @ move result to TOS
    UBOOT_RESTORE_USER_SP
    pop { r0-r4, r6-r12, lr }
.endm

@ ==================== U-Boot API
@ api_address env. variable
@ value which is a pointer to 
@ U-Boot allocated api_signature
@ struct in api/api.c
.equ UBOOT_API_ADDR, 0x1bbac1f8 @ WARNING : might change with U-Boot config change (even simple bootcmd change might move it away !)

@ U-Boot API func. offset
.equ UBOOT_API_GETC, 1          @ I: none O: char.
.equ UBOOT_API_PUTC, 2          @ I: char. O: none
.equ UBOOT_API_TSTC, 3          @ I: none O: -1 if char. available 0 if not
.equ UBOOT_API_PUTS, 4          @ I: string addr.
.equ UBOOT_API_RESET, 5         @ I: none O: none
.equ UBOOT_API_GET_SYS_INFO, 6  @ unsupported
.equ UBOOT_API_UDELAY, 7        @ I: microseconds O: none
.equ UBOOT_API_GET_TIMER, 8     @ I: base O: ms
.equ UBOOT_API_DEV_ENUM, 9      @ I: device addr. O: none
.equ UBOOT_API_DEV_OPEN, 10     @ I: device addr. O: err. type (or 0)
.equ UBOOT_API_DEV_CLOSE, 11    @ I: device addr. O: err. type (or 0)
.equ UBOOT_API_DEV_READ, 12     @ I: device addr, dst buffer addr, # blocks to read, start block O: err. type (or 0), blocks read
.equ UBOOT_API_DEV_WRITE, 13    @ I: device addr, src buffer addr, # blocks to write, start block O: err. type (or 0)
.equ UBOOT_API_ENV_ENUM, 14     @ unsupported
.equ UBOOT_API_ENV_GET, 15      @ I: env. name O: env. value
.equ UBOOT_API_ENV_SET, 16      @ I: env. name followed by value (TOS) O: none
@ U-Boot API custom extensions : see my U-Boot write-up
.equ UBOOT_API_CACHE_CARE, 20   @ I: cache maintenance type to perform on entire cache, 1 dcache flush, 2 dcache invalidate, any other values is dcache + icache clean O: none

@ U-Boot API syscall
@                        r5: arg.
@ put return value on stack
@ and errval is also pushed (TOS)
@ for a few syscall (DEV ones)
@ NOTE: struct filling syscall
@ require a pointer to a RAM area
@ as argument, area should have
@ enough space to hold the struct
.macro UBOOT_API_SYSCALL, number, no_stack=0
    .if \number == UBOOT_API_GETC || \number == UBOOT_API_TSTC
        push { r5 }
    .endif
    .if \number == UBOOT_API_DEV_READ || \number == UBOOT_API_DEV_WRITE
        ldr r8, [r5]            @ get device type; see device_info include/api_public.h
        tst r8, #0x002          @ DEV_TYP_STOR ?
        pop { r6 }
        pop { r7 }
    .endif
    .if \number == UBOOT_API_DEV_WRITE
        popne { r8 }            @ DEV_TYP_STOR has 4 params
    .elseif \number == UBOOT_API_DEV_READ
        pop { r8 }
        popne { r9 }            @ DEV_TYP_STOR has 5 params
    .elseif \number == UBOOT_API_ENV_SET
        pop { r6 }
    .endif
    str r6, [pc, #12]
    str r7, [pc, #12]
    str r8, [pc, #12]
    str r9, [pc, #12]
    b 41f
        40:                     @ params
        .word 0
        .word 0
        .word 0
        .word 0
    41:
    push { r0-r4, r6-r12, lr }
    UBOOT_RESTORE_SP
    UBOOT_RESTORE_GD
    adr r1, 42f
    mov pc, pc
    42: .word 0                 @ return value is stored here
    .if \number == UBOOT_API_PUTS || \number == UBOOT_API_DEV_ENUM || \number == UBOOT_API_DEV_OPEN || \number == UBOOT_API_DEV_CLOSE || \number == UBOOT_API_DEV_WRITE || \number == UBOOT_API_DEV_READ
        mov r2, r5              @ pointer to string addr. or device struct
        .if \number == UBOOT_API_DEV_WRITE || \number == UBOOT_API_DEV_READ
            adr r0, 40b
            ldr r3, [r0]        @ ptr to buffer to get data to write / where to read data
            .if \number == UBOOT_API_DEV_READ
                add r12, r0, #12
                push { r12 }    @ stack alignment
                push { r12 }    @ fifth param value (where to put the len actually read; storage type only)
            .endif
            add r12, r0, #8
            push { r12 }        @ fourth param value (start block or where to put the len actually read if not storage type)
            add r12, r0, #4
            push { r12 }        @ third param value (# of blocks to write / read or packet length to write / read if not a storage type)
        .endif
    .else
        .if \number == UBOOT_API_ENV_SET
            mov r3, r5
            adr r0, 40b
            ldr r2, [r0]
        .elseif \number == UBOOT_API_ENV_GET
            mov r2, r5
            adr r3, 43f
            mov pc, pc
            43: .word 0         @ first arg. / result
        .else
            adr r2, 43f
            str r5, [r2]        @ store arg.
            mov pc, pc
            43: .word 0         @ first arg. / result
        .endif
    .endif
    .if \number == UBOOT_API_GET_TIMER
        adr r3, 44f
        str r5, [r3]            @ store arg.
        mov pc, pc
        44: .word 0             @ second arg.
    .endif
    mov r0, #\number            @ syscall; see api/api.c
    ldr r12, [pc]
    mov pc, pc
        .word UBOOT_API_ADDR
    adr lr, 0f                  @ set return addr.
    ldr pc, [r12, #16]          @ call syscall(), offset is syscall member (pointer) of api_signature struct in include/api_public.h
    0:
    .if \number == UBOOT_API_DEV_READ
        add sp, sp, #16         @ clean stack
    .elseif \number == UBOOT_API_DEV_WRITE
        add sp, sp, #8          @ clean stack
    .endif
    UBOOT_RESTORE_USER_SP
    pop { r0-r4, r6-r12, lr }
    .if \number == UBOOT_API_DEV_OPEN || \number == UBOOT_API_DEV_CLOSE || \number == UBOOT_API_DEV_WRITE || \number == UBOOT_API_DEV_READ
        .if \number == UBOOT_API_DEV_READ
            adr r5, 40b
            ldr r5, [r5, #12]
            push { r5 }         @ push read length
        .endif
        adr r5, 42b
        ldr r5, [r5]            @ retval on TOS
    .endif
    .if \number == UBOOT_API_GETC || \number == UBOOT_API_TSTC || \number == UBOOT_API_GET_TIMER || \number == UBOOT_API_ENV_GET
        adr r5, 43b
        ldr r5, [r5]            @ value on TOS
        .if \number == UBOOT_API_TSTC
            mvn r5, r5
            add r5, r5, #1
        .endif
    .endif
    .if \no_stack == 0
        .if \number == UBOOT_API_PUTC || \number == UBOOT_API_PUTS || \number == UBOOT_API_UDELAY || \number == UBOOT_API_ENV_SET || \number == UBOOT_API_DEV_ENUM || \number == UBOOT_API_CACHE_CARE
            pop { r5 }
        .endif
    .endif
.endm

@ ===== U-Boot API initialization
UBOOT_API_SETUP:                @ MUST BE CALLED AT PROGRAM START TO USE THE UBOOT API !
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
