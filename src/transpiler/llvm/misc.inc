trans_verbatim:
    .ascii "\x0d"
    .asciz "call void asm sideeffect \".long 0x\x14\",\"\"()\n\x08"
    .align 2

trans_phi: @ this doesn't use the "VM" (see trans_block_end)
    .asciz "%r\x14 = phi i32 [%r\x15, %l\x16], [%r\x17, %l\x18]\n"
    .align 2

trans_literal:
    .ascii "\x0d\x0e"
    .asciz "%r\x15 = add i32 0,u0x\x14\n\x08"
    .align 2

.if TRANSPILE_GENERATE_GLOBAL_VARS == 0
trans_var_load:
    .ascii "\x0d\x10\x0e"
    .ascii "%r\x15 = inttoptr i64 u0x\x14 to i32*\n"
    .asciz "%r\x16 = load i32,i32* %r\x15\n\x08"
    .align 2

trans_var_store:
    .ascii "\x0d\x0f\x10"
    .ascii "%r\x16 = inttoptr i64 u0x\x14 to i32*\n"
    .asciz "store i32 %r\x15,i32* %r\x16\n\x08"
    .align 2
.else
trans_var_load:
    .ascii "\x0d\x0e"
    .asciz "%r\x15 = load i32,i32* @\x04\n\x08"
    .align 2

trans_var_store:
    .ascii "\x0d\x0f"
    .asciz "store i32 %r\x15,i32* @\x04\n\x08"
    .align 2
.endif

trans_footer:
    .ascii "ret void\n"
    .asciz "}\n"
    .align 2
