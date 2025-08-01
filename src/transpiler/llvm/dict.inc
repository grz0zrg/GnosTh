@ ======================= COMPILE
forth_word "entrypoint" WORD_IMMEDIATE
    mov r5, #WORD_ENTRY_POINT
    strb r5, [r2, #8]           @ mark word as entry point
    str r14, [r3]               @ remember code output target before the entrypoint switch
    add r14, r3, #4             @ now point to program code
    adr r9, entrypoint_head_code
    adr r10, entrypoint_ret
    copy_entrypoint_head_code:
    ldrb r12, [r9], #1
    strb r12, [r14], #1
    cmp r9, r10
    bne copy_entrypoint_head_code
    b entrypoint_ret
    entrypoint_head_code:       @ define arch + Forth registers + main func.
    .ascii "target triple = \"armv6-unknown-linux-gnueabihf\"\n"
    .ascii "target datalayout = \"e-m:e-p:32:32-Fi8-i64:32-v128:64:128-a:0:32-n32-S32\"\n"
    .ascii "@rA = internal global i32 0\n"
    .ascii "@rB = internal global i32 0\n"
    .ascii "@rC = internal global i32 0\n"
    .ascii "@rD = internal global i32 0\n"
    .ascii "define void @main() nounwind {\n"
    .ascii "l00000000:\n"
    .balign 4, 0x20
    entrypoint_ret:

@ ======================== MEMORY
forth_word "allot" WORD_IMMEDIATE
    add r10, r4, #4             @ assign data addr. (+4 to skip length)
    mov r12, r10
    adr r9, allot
    ldr r10, [pc]               @ literal
    b transpile
    .word trans_literal
    allot:
    pop { r5 }
    str r5, [r4], #4            @ store length
    add r4, r4, r5              @ offset data
    add r4, #4
    bic r4, #3                  @ align

forth_word "align" WORD_IMMEDIATE
    pop { r5 }
    add r4, r4, r5              @ offset to get room for offseted length (so data is aligned)
    sub r5, r5, #1
    mov r10, r5
    mvn r5, r5
    add r4, r4, r10
    and r4, r4, r5              @ align
    sub r4, r4, #4              @ offset length; only data is aligned

forth_word "@"
    .ascii "\x0f\x10\x0e"
    .ascii "%r\x15 = inttoptr i32 %r\x14 to i32*\n"
    .ascii "%r\x16 = load i32, i32* %r\x15\n"

forth_word "!"
    .ascii "\x0f\x0f\x10"
    .ascii "%r\x16 = inttoptr i32 %r\x14 to i32*\n"
    .ascii "store i32 %r\x15, i32* %r\x16\n"

forth_word "db@"
    .ascii "\x0f\x10\x10\x0e"
    .ascii "%r\x15 = inttoptr i32 %r\x14 to i8*\n"
    .ascii "%r\x16 = load i8, i8* %r\x15\n"
    .ascii "%r\x17 = zext i8 %r\x16 to i32\n"

forth_word "db!"
    .ascii "\x0f\x0f\x10\x10"
    .ascii "%r\x16 = inttoptr i32 %r\x14 to i8*\n"
    .ascii "%r\x17 = trunc i32 %r\x15 to i8\n"
    .ascii "store i8 %r\x17, i8* %r\x16\n"

forth_word "dlen"
    .ascii "\x0f\x10\x10\x0e\x0e"
    .ascii "%r\x15 = sub i32 %r\x14, 4\n"
    .ascii "%r\x16 = inttoptr i32 %r\x15 to i32*\n"
    .ascii "%r\x17 = add i32 0, %r\x14\n"
    .ascii "%r\x18 = load i32, i32* %r\x16\n"

forth_word "#!" WORD_IMMEDIATE
    pop { r5 }
    mov r10, r14
    add r14, r14, #4
    bic r14, r14, #3            @ align is needed due to ASCII code output
    cmp r10, r14
    beq imm_store
    mov r12, #32                @ but fill leftover with space char. so it may remains readable
    imm_store_space_pad:
        strb r12, [r10], #1
        cmp r10, r14
        bne imm_store_space_pad
    imm_store:
    str r5, [r14], #4           @ store TOS value to current code addr. useful to generate custom IR etc.

@ ========================= STACK
forth_word "dup"
    .ascii "\x0b\x0e"
    .ascii "%r\x15 = add i32 0, %r\x14\n"

forth_word "drop"
    .ascii "\x0f"

forth_word "swap"
    .ascii "\x0f\x0f\x0e\x0e"
    .ascii "%r\x16 = add i32 0, %r\x14\n"
    .ascii "%r\x17 = add i32 0, %r\x15\n"

forth_word "over"
    .ascii "\x0f\x0f\x0e\x0e\x0e"
    .ascii "%r\x16 = add i32 0, %r\x15\n"
    .ascii "%r\x17 = add i32 0, %r\x14\n"
    .ascii "%r\x18 = add i32 0, %r\x15\n"

forth_word "rot"
    .ascii "\x0f\x0f\x0f\x0e\x0e\x0e"
    .ascii "%r\x17 = add i32 0, %r\x15\n"
    .ascii "%r\x18 = add i32 0, %r\x14\n"
    .ascii "%r\x19 = add i32 0, %r\x16\n"

@ =========================== I/O
forth_word "tstc"
    .ascii "; tstc() ; TODO\n"

forth_word "getc"
    .ascii "; getc() ; TODO\n"

forth_word "gett"
    .ascii "\x0f\x0e"
    .ascii "%r\x15 = call i32 asm sideeffect \"add r14, pc, #8\\0Aldr r12, [pc]\\0Amov pc, r12\\0A.word 0x80040\", \"={r5},{r5},~{r12},~{lr}\"(i32 %r\x14)\n"

forth_word "dev_enum"
    .ascii "; dev_enum() ; TODO\n"

forth_word "dev_open"
    .ascii "; dev_open() ; TODO\n"

forth_word "dev_close"
    .ascii "; dev_close() ; TODO\n"

forth_word "dev_read"
    .ascii "; dev_read() ; TODO\n"

forth_word "dev_write"
    .ascii "; dev_write() ; TODO\n"

forth_word "reset"
    .ascii "; reset() ; TODO\n"

forth_word "udelay"
    .ascii "; udelay() ; TODO\n"

forth_word "envget"
    .ascii "; envget() ; TODO\n"

forth_word "envset"
    .ascii "; envset() ; TODO\n"

forth_word "putc"
    .ascii "; putc() ; TODO\n"

forth_word "puts"
    .ascii "\x0f"
    .ascii "call void asm sideeffect \"add r14, pc, #8\\0Aldr r12, [pc]\\0Amov pc, r12\\0A.word 0x800FC\", \"{r5},~{r5},~{r12},~{lr}\"(i32 %r\x14)\n"

forth_word "cache_care"
    .ascii "; cache_care() ; TODO\n"

@ ====================== GRAPHICS
forth_word "pix"
    .ascii "\x0f\x0f\x0f\x0f\x0f\x10\x10\x10\x10\x10"
    .ascii "%r\x19 = mul i32 %r\x15, %r\x16\n"
    .ascii "%r\x1a = add i32 %r\x19, %r\x17\n"
    .ascii "%r\x1b = shl i32 %r\x1a, 2\n"
    .ascii "%r\x1c = add i32 %r\x14, %r\x1b\n"
    .ascii "%r\x1d = inttoptr i32 %r\x1c to i32*\n"
    .ascii "store i32 %r\x18, i32* %r\x1d\n"

@ ========================= LOGIC
forth_word "0="
    .ascii "\x0f\x10\x10\x0e"
    .ascii "%r\x15 = icmp eq i32 %r\x14, 0\n"
    .ascii "%r\x16 = zext i1 %r\x15 to i32\n"
    .ascii "%r\x17 = sub i32 0, %r\x16\n"

forth_word "0<>"
    .ascii "\x0f\x10\x10\x0e"
    .ascii "%r\x15 = icmp ne i32 %r\x14, 0\n"
    .ascii "%r\x16 = zext i1 %r\x15 to i32\n"
    .ascii "%r\x17 = sub i32 0, %r\x16\n"

forth_word "="
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp eq i32 %r\x14, %r\x15\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word "<>"
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp ne i32 %r\x14, %r\x15\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word "<"
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp slt i32 %r\x15, %r\x14\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word ">"
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp sgt i32 %r\x15, %r\x14\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word "<="
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp sle i32 %r\x15, %r\x14\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word ">="
    .ascii "\x0f\x0f\x10\x10\x0e"
    .ascii "%r\x16 = icmp sge i32 %r\x15, %r\x14\n"
    .ascii "%r\x17 = zext i1 %r\x16 to i32\n"
    .ascii "%r\x18 = sub i32 0, %r\x17\n"

forth_word "~"
    .ascii "\x0f\x0e"
    .ascii "%r\x15 = xor i32 %r\x14, -1\n"

forth_word "&"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = and i32 %r\x15, %r\x14\n"

forth_word "|"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = or i32 %r\x15, %r\x14\n"

forth_word "^"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = xor i32 %r\x15, %r\x14\n"

forth_word "<<"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = shl i32 %r\x15, %r\x14\n"

forth_word ">>"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = ashr i32 %r\x15, %r\x14\n"

forth_word ">>>"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = lshr i32 %r\x15, %r\x14\n"

@ ================== FLOW CONTROL
forth_word "call"
    .ascii "; call() ; TODO\n"

forth_word "loop"
    .ascii "\x11\x05"
    .ascii "br label %l\x14\n"
    .ascii "l\x14:\n"

forth_word "again"
    .ascii "\x12"
    .ascii "br label %l\x14\n"

forth_word "until"
    .ascii "\x0f\x12\x13\x13\x05"
    .ascii "%lr\x16 = icmp ne i32 %r\x14, 0\n"
    .ascii "br i1 %lr\x16, label %l\x17, label %l\x15\n"
    .ascii "l\x17:\n"

forth_word "for"
    .ascii "\x0f\x11\x11\x11\x11\x05\x11\x11\x11\x13\x0e"
    .ascii "br label %l\x1c\n"
    .ascii "l\x1c:\n"
    .ascii "%lr\x15 = add i1 0, 0\n"
    .ascii "%lr\x16 = add i32 0, -1\n"
    .ascii "%lr\x17 = add i32 0, 0\n"
    .ascii "br label %l\x18\n"
    .ascii "l\x18:\n"
    .ascii "%lr\x19 = phi i32 [ %r\x14, %l\x1c ], [ %lr\x1a, %l\x1b ]\n"
    .ascii "%r\x1d = add i32 0, %lr\x19\n"

forth_word "do"                 @ same as "for" but takes 3 parameters instead of 1
    .ascii "\x0f\x0f\x0f\x11\x11\x11\x11\x05\x11\x11\x11\x13\x0e\x13"
    .ascii "br label %l\x1e\n"
    .ascii "l\x1e:\n"
    .ascii "%lr\x17 = icmp sgt i32 %r\x15, %r\x16\n"
    .ascii "%lr\x1f\x01 = sub i32 0, %r\x14\n"
    .ascii "%lr\x18 = select i1 %lr\x17, i32 %r\x14, i32 %lr\x1f\x01\n"
    .ascii "%lr\x19 = add i32 0, %r\x15\n"
    .ascii "br label %l\x1a\n"
    .ascii "l\x1a:\n"
    .ascii "%lr\x1b = phi i32 [ %r\x16, %l\x1e ], [ %lr\x1c, %l\x1d ]\n"
    .ascii "%r\x1f\x05 = add i32 0, %lr\x1b\n"

forth_word "next"
    .ascii "\x12\x12\x12\x12\x12\x12\x12\x13\x13\x13\x13\x05"
    .ascii "%lr\x15 = add i32 %lr\x16, %lr\x19\n"
    .ascii "br label %l\x14\n"
    .ascii "l\x14:\n"
    .ascii "%lr\x1b = icmp slt i32 %lr\x15, %lr\x18\n"
    .ascii "%lr\x1c = icmp sgt i32 %lr\x15, %lr\x18\n"
    .ascii "%lr\x1d = select i1 %lr\x1a, i1 %lr\x1b, i1 %lr\x1c\n"
    .ascii "br i1 %lr\x1d, label %l\x17, label %l\x1e\n"
    .ascii "l\x1e:\n"

forth_word "if"
    .ascii "\x0f\x13\x13\x05\x11"
    .ascii "%lr\x15 = icmp ne i32 %r\x14, 0\n"
    .ascii "br i1 %lr\x15, label %l\x16, label %l\x17\n"
    .ascii "l\x16:\n"
    .ascii "\x06\x01"

forth_word "else"
    .ascii "\x12\x05\x11"
    .ascii "br label %l\x15\n"
    .ascii "l\x14:\n"
    .ascii "\x06\x02"

forth_word "then"
    .ascii "\x12\x05"
    .ascii "br label %l\x14\n"
    .ascii "l\x14:\n"
    .ascii "\x07"

forth_word "case"
    .ascii "; case ; TODO\n"

@ ===================== REGISTERS
forth_word "A@"
    .ascii "\x0e"
    .ascii "%r\x14 = load i32, i32* @rA\n"

forth_word "A!"
    .ascii "\x0f"
    .ascii "store i32 %r\x14, i32* @rA\n"

forth_word "B@"
    .ascii "\x0e"
    .ascii "%r\x14 = load i32, i32* @rB\n"

forth_word "B!"
    .ascii "\x0f"
    .ascii "store i32 %r\x14, i32* @rB\n"

forth_word "C@"
    .ascii "\x0e"
    .ascii "%r\x14 = load i32, i32* @rC\n"

forth_word "C!"
    .ascii "\x0f"
    .ascii "store i32 %r\x14, i32* @rC\n"

forth_word "D@"
    .ascii "\x0e"
    .ascii "%r\x14 = load i32, i32* @rD\n"

forth_word "D!"
    .ascii "\x0f"
    .ascii "store i32 %r\x14, i32* @rD\n"

@ ==================== ARITHMETIC
forth_word "negate"
    .ascii "\x0f\x0e"
    .ascii "%r\x15 = sub i32 0, %r\x14\n"

forth_word "*"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = mul i32 %r\x15, %r\x14\n"

forth_word "/"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = udiv i32 %r\x15, %r\x14\n"

forth_word "%"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = urem i32 %r\x15, %r\x14\n"

forth_word "*/"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "%r\x17 = mul i32 %r\x16, %r\x15\n"
    .ascii "%r\x18 = udiv i32 %r\x17, %r\x14\n"

forth_word ">>-"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "%r\x17 = ashr i32 %r\x15, %r\x14\n"
    .ascii "%r\x18 = sub i32 %r\x16, %r\x17\n"

forth_word ">>+"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "%r\x17 = ashr i32 %r\x15, %r\x14\n"
    .ascii "%r\x18 = add i32 %r\x16, %r\x17\n"

forth_word "-"
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = sub i32 %r\x15, %r\x14\n"

forth_word "+" WORD_RUNTIME IS_LAST_WORD
    .ascii "\x0f\x0f\x0e"
    .ascii "%r\x16 = add i32 %r\x15, %r\x14\n"
    .asciz "\x08" @ NOTE: mandatory for the last word in transpile mode
    .align 2
forth_dict_end_addr:
999:
