@ Transpiler, set of routines to
@ interpret templated strings and
@ output a result (e.g. LLVM IR)
@ to program address, some ASCII
@ char. act as control codes for
@ a small virtual machine which
@ produce SSA friendly output,
@ it keep track of registers /
@ indexes to craft unique symbols
@ There is two internal stacks
@ which mirror Forth stacks with
@ independent counters to
@ highlight registers and labels
@ difference in the output code.
@ Templated string have \x08 chr
@ at the end to prepare the ctx.
@ stack for the next word, they
@ are terminated by NULL. Stack
@ operations are in the first
@ line usually for clarity,
@ literal push instruction must
@ be placed before other instr.
@ in the templated string due
@ to some overwritten register.
@ Registers are prefixed by 'r',
@ labels are prefixed by 'l',
@ flow registers are prefixed by
@ 'lr', generated symbols are
@ 32 bits hexadecimal value so
@ there is a limit to the amount
@ of registers / labels although
@ this can be changed easily if
@ needed.
@ Stack frame is splitted by two
@ to contain reg. / labels stack
@ with multiple levels to be able
@ to generate phi nodes.
@ ------------------------------>
@ NOTE: code is only resolved in
@ the entrypoint due to inlining,
@ it is copied verbatim in words
@ unless a literal, those have
@ partial resolved code. (value)
@ Instructions which use literal
@ value should be used first due
@ to some instr. that overwrite
@ the register. (see literals
@ templates)
@ Also note that register stacks
@ hold register index with
@ associated block as a 32b
@ integer pair.
@ ------------------------------>
@ 00: end of templated code
@ 04: ctx. TOS-4  str. out.
@ 05: set current block index 
@     from context stack TOS
@ 06: push frame, next byte is
@     a block marker for e.g.
@     'if' block or 'else' block
@ 07: pop frame (add phi nodes)
@ 08: reset context stack
@  -> placed before end code
@ 0B: get data stack TOS value
@ 0D: push r12 on context stack
@  -> used to expose literals
@ 0E: get new register index
@     push on reg. stack
@     push associated block
@     push on context stack
@  -> emulate Forth push
@ 0F: pop associated block
@     pop from reg. stack
@     push val. on context stack
@  -> emulate Forth pop
@ 10: get new register index
@     push on context stack
@  -> get a temporary register
@     used across the word
@     definition
@ 11: get new flow index
@     push on flow stack
@     push on context stack
@  -> emulate return stack push
@ 12: pop from flow stack
@     push val. on context stack
@  -> emulate return stack pop
@ 13: get new flow index
@     push on context stack
@  -> get a temporary flow
@     register
@ 14: ctx. TOS-0  hex. lit out.
@ 15: ctx. TOS-4  hex. lit out.
@ 16: ctx. TOS-8  hex. lit out.
@ 17: ctx. TOS-12 hex. lit out.
@ 18: ctx. TOS-16 hex. lit out.
@ 19: ctx. TOS-20 hex. lit out.
@ 1A: ctx. TOS-24 hex. lit out.
@ 1B: ctx. TOS-28 hex. lit out.
@ 1C: ctx. TOS-32 hex. lit out.
@ 1D: ctx. TOS-36 hex. lit out.
@ 1E: ctx. TOS-40 hex. lit out.
@ 1F: ctx.  TOS-N hex. lit out.
@     where N is the next byte
@ ------------------------------>

@ ============ CONVENIENCE MACROS
.macro get_new_index counter r
    adreq r5, \counter
    ldreq \r, [r5]
    addeq \r, \r, #1            @ get new register / label index
    streq \r, [r5]
.endm

.macro push_index stack r
    adreq r5, \stack
    ldreq r7, [r5]
    stmeqdb r7!, { \r }         @ push index
.if \stack == trans_reg_stack_addr
    adreq r11, trans_current_block
    ldreq r11, [r11]
    stmeqdb r7!, { r11 }        @ push associated block
.endif
    streq r7, [r5]              @ update target stack
.endm

.macro pop_index stack r
    adreq r5, \stack
    ldreq r7, [r5]
.if \stack == trans_reg_stack_addr
    ldmeqia r7!, { \r }         @ pop associated block
.endif
    ldmeqia r7!, { \r }         @ pop index
    streq r7, [r5]              @ update target stack
.endm

.macro sp_hex_out char offset label
    cmp r11, #\char
    ldreq r12, [r8, #-\offset]
    adreq r7, \label
    pusheq { r7 }
    beq trans_hex_out
.endm

.macro check_if_in_entrypoint
    ldrb r5, [r2, #8]           @ get last defined word type
    cmp r5, #WORD_VARIABLE      @ if a variable
    mov r11, r2
    bne 420f
    421:                        @ find root word
        ldr r11, [r11]          @ get root definition addr.
        ldrb r5, [r11, #8]
        cmp r5, #WORD_VARIABLE
        beq 421b
    420:
    cmp r5, #WORD_ENTRY_POINT   @ check if in entry point word
.endm

trans_literal_addr:
    .word trans_literal

trans_var_load_addr:
    .word trans_var_load

trans_var_store_addr:
    .word trans_var_store

trans_verbatim_addr:
    .word trans_verbatim

@ ==== OUTPUT CODE FROM TEMPLATES
@        last dict. word addr: r2
@                return addr.: r9
@                output code: r10
@                    literal: r12
@ ===================== ON RETURN
@ ===================== CLOBBERED
@  r5, r7, r8, r10, r11, r12, r14
@ ===============================
transpile:
    check_if_in_entrypoint
    mov r8, r13                 @ save SP (used as a local context valid during the template string processing)
    beq 424f
    adr r7, trans_literal_addr
    ldr r7, [r7]
    cmp r10, r7
    adrne r7, trans_verbatim_addr
    ldrne r7, [r7]
    cmpne r10, r7
    adrne r7, trans_var_load_addr
    ldrne r7, [r7]
    cmpne r10, r7
    adrne r7, trans_var_store_addr
    ldrne r7, [r7]
    cmpne r10, r7
    beq 423f
    422:                        @ in a word definition
        ldrb r11, [r10], #1
        cmp r11, #0
        strneb r11, [r14], #1   @ copy templated code + "VM" instr. verbatim
        bne 422b
        b 425f
    423:                        @ in a word definition + is a literal
        ldrb r11, [r10], #1
        cmp r11, #4             @ 0x04
        ldreq r12, [r8, #-4]
        adreq r7, 423b
        pusheq { r7 }
        beq trans_str_out
        cmp r11, #13            @ 0x0d
        pusheq { r12 }
        streqb r11, [r14], #1   @ output it back (inlining support)
        beq 423b
        sp_hex_out 20 4 423b
        cmp r11, #0
        strneb r11, [r14], #1
        bne 423b
        b 425f
    424:                        @ in an entry point
        ldrb r11, [r10], #1
        cmp r11, #4             @ 0x04 ? output string from address on ctx stack TOS-4
        ldreq r12, [r8, #-4]
        adreq r7, 424b
        pusheq { r7 }
        beq trans_str_out
        @ phi nodes
        cmp r11, #5             @ 0x05 ? update block index
        ldreq r7, [r13]         @ get current block from TOS
        streq r7, trans_current_block
        beq 424b
        cmp r11, #6             @ 0x06 ?
        ldreqb r12, [r10], #1   @ get phi block index
        subeq r12, r12, #1      @ adjust (avoid instr. parameters interpreted as NULL char.)
        beq trans_block_start   @ start of a phi block
        cmp r11, #7             @ 0x07 ?
        beq trans_block_end     @ resolve phi nodes
        @
        cmp r11, #8             @ 0x08 ?
        moveq r13, r8           @ reset local stack
        beq 424b
        cmp r11, #11            @ 0x0b ? get data stack top value
        adreq r5, trans_reg_stack_addr
        ldreq r7, [r5]
        ldreq r12, [r7, #4]
        pusheq { r12 }
        beq 424b
        cmp r11, #13            @ 0x0d ?
        pusheq { r12 }          @ push literal on ctx stack
        beq 424b
        cmp r11, #14            @ 0x0e ? emulate Forth push (data stack)
        get_new_index trans_reg_counter r12
        push_index trans_reg_stack_addr r12
        pusheq { r12 }
        beq 424b
        cmp r11, #15            @ 0x0f ? emulate Forth pop (data stack)
        pop_index trans_reg_stack_addr r12
        pusheq { r12 }
        beq 424b
        cmp r11, #16            @ 0x10 ? get temp. symbol (register)
        get_new_index trans_reg_counter r12
        pusheq { r12 }
        beq 424b
        cmp r11, #17            @ 0x11 ? emulate Forth push (return stack)
        get_new_index trans_lbl_counter r12
        push_index trans_lbl_stack_addr r12
        pusheq { r12 }
        beq 424b
        cmp r11, #18            @ 0x12 ? emulate Forth pop (return stack)
        pop_index trans_lbl_stack_addr r12
        pusheq { r12 }
        beq 424b
        cmp r11, #19            @ 0x13 ? get temp. symbol (label)
        get_new_index trans_lbl_counter r12
        pusheq { r12 }
        beq 424b
        sp_hex_out 20 4 424b
        sp_hex_out 21 8 424b
        sp_hex_out 22 12 424b
        sp_hex_out 23 16 424b
        sp_hex_out 24 20 424b
        sp_hex_out 25 24 424b
        sp_hex_out 26 28 424b
        sp_hex_out 27 32 424b
        sp_hex_out 28 36 424b
        sp_hex_out 29 40 424b
        sp_hex_out 30 44 424b
        cmp r11, #31            @ as above but retrieve any values from the ctx stack 
        ldreqb r11, [r10], #1
        subeq r11, r11, #1      @ adjust (avoid instr. parameters interpreted as NULL char.)
        ldreq r12, [r13, r11]
        adreq r7, 424b
        pusheq { r7 }
        beq trans_hex_out
        cmp r11, #0
        strneb r11, [r14], #1
        bne 424b
    425:
    mov r13, r8
    mov r15, r9

@ =========== OUTPUT HEX. LITERAL
@               return addr.: TOS
@                    literal: r12
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                r5, r7, r12, r14
@ ===============================
trans_hex_out:
    mov r7, #8
    242:
        mov r5, r12, LSR #28
        and r5, r5, #15
        cmp r5, #10
        addge r5, r5, #0x37
        addlt r5, r5, #0x30
        strb r5, [r14], #1
        lsl r12, r12, #4
        subs r7, r7, #1
        bne 242b
    pop { pc }

@ ================= OUTPUT STRING
@               return addr.: TOS
@               string addr.: r12
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                    r7, r12, r14
@ ===============================
trans_str_out:
    243:
        ldrb r7, [r12], #1
        cmp r7, #0
        strneb r7, [r14], #1
        bne 243b
    pop { pc }

@ ============ OUTPUT FOOTER CODE
@                return addr.: r9
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                    r5, r12, r14
@ ===============================
trans_copy_footer:
    adr r5, trans_footer
    trans_copy_footer_code:
        ldrb r12, [r5], #1
        cmp r12, #0
        strneb r12, [r14], #1
        bne trans_copy_footer_code
    mov r15, r9

@ = ENTER BLOCK (add stack frame)
@ compute new stack frame addr.
@ and compute old one (depends on
@ whether we start from 'if' or
@ 'else') then copy old into new
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                r5, r7, r11, r12
@ ===============================
trans_block_start:
    ldr r5, trans_reg_stack_base_addr
    str r12, [r5, #4]           @ get block type ('if' or 'else')
    cmp r12, #1
    ldr r11, trans_stack_size
    add r7, r5, r11             @ new stack frame addr. (base)
    str r7, trans_reg_stack_base_addr
    ldr r12, trans_reg_stack_addr
    str r12, [r5, #8]           @ store current stack frame addr.
    add r7, r12, r11            @ get new stack frame addr. (head)
    subeq r12, r5, r11          @ in 'else' block the src stack frame
    ldreq r12, [r12, #8]        @ is the block before the conditional
    addeq r7, r12, r11, LSL #1  @ new stack frame is 2 blocks after
    str r7, trans_reg_stack_addr
    ldr r5, trans_reg_stack_base_addr
    subs r11, r5, r7
    beq 424b
    trans_clone_stack:
        ldr r5, [r12], #4
        str r5, [r7], #4
        subs r11, r11, #4
        bne trans_clone_stack
    b 424b

@ ===== END BLOCK (add phi nodes)
@ go back to previous stack frame
@ level and add needed phi nodes
@ ===================== ON RETURN
@ ===================== CLOBBERED
@           r5, r7, r11, r12, r14
@ ===============================
trans_block_end:
    push { r0-r6 }
    ldr r11, trans_stack_size
    ldr r5, trans_reg_stack_base_addr
    mov r2, r5
    sub r5, r5, r11             @ go back to previous stack frame addr. (base)
    ldrb r12, [r5, #4]          @ get block type ('if' or 'else')
    cmp r12, #1
    subeq r5, r5, r11           @ go back further if 'else'
    str r5, trans_reg_stack_base_addr
    ldr r6, trans_reg_stack_addr
    sub r0, r6, r11             @ same for head
    mov r12, r0
    subeq r0, r0, r11
    str r0, trans_reg_stack_addr
    subs r11, r2, r6            @ get stack length
    lsr r11, r11, #1            @ due to values being paired
    beq trans_skip_add_phi_nodes
    trans_add_phi_nodes:
        ldr r2, [r12], #4       @ get block
        ldr r4, [r6], #4        @ get block
        ldr r1, [r12], #4       @ get reg.
        ldr r3, [r6], #4        @ get reg.
        cmp r1, r3              @ compare reg. index value
        beq skip_phi_node_gen
            cmp r1, r1          @ get the macro below working
            get_new_index trans_reg_counter r0
            @ update stack frame with the merged reg. index
            adr r5, trans_reg_stack_addr
            ldr r5, [r5]
            adr r5, trans_reg_stack_base_addr
            ldr r5, [r5]
            sub r5, r5, r11, LSL #1
            str r0, [r5, #4]
            ldr r7, trans_current_block
            str r7, [r5]
            @ output phi node code
            push { r12 }
            push { r0-r4 }
            adr r2, trans_phi
            copy_phi_code:
                ldrb r1, [r2], #1
                cmp r1, #20
                blt skip_hex_out
                cmp r1, #24
                bgt skip_hex_out
                    sub r12, r1, #20
                    ldr r12, [r13, r12, LSL #2]
                    adr r3, copy_phi_code
                    push { r3 }
                    b trans_hex_out
                skip_hex_out:
                cmp r1, #0
                strneb r1, [r14], #1
                bne copy_phi_code
            pop { r0-r4 }
            pop { r12 }
        skip_phi_node_gen:
        subs r11, r11, #4
        bne trans_add_phi_nodes
    trans_skip_add_phi_nodes:
    pop { r0-r6 }
    b 424b

trans_reg_counter:
    .word -1

trans_reg_stack_addr:
    .word FORTH_TRANS_REG_STACK_ADDR

trans_reg_stack_base_addr:
    .word FORTH_TRANS_REG_STACK_ADDR

trans_lbl_counter:
    .word 0                     @ start a 0 due to entry point

trans_lbl_stack_addr:
    .word (FORTH_TRANS_REG_STACK_ADDR - FORTH_TRANS_STACK_SIZE / 2)

trans_stack_size:
    .word FORTH_TRANS_STACK_SIZE

trans_current_block:
    .word 0

@ ===== LITERALS / MISC TEMPLATE
.if FORTH_OUTPUT_CODE == LLVM_IR_OUTPUT
    .include "src/transpiler/llvm/misc.inc"
.endif

.if FORTH_OUTPUT_CODE == P5JS_OUTPUT
    .include "src/transpiler/p5js/misc.inc"
.endif
