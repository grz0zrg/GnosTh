trans_verbatim:
    .asciz "// verbatim (unsupported)\n\x08"
    .align 2

trans_phi: @ this doesn't use the "VM" (see trans_block_end)
    .ascii "var v\x14 = v\x17 ?? v\x15\n"
	.ascii "var v\x15 = undefined\n"
	.asciz "var v\x17 = undefined\n"
    .align 2

trans_literal:
    .ascii "\x0d\x0e"
    .asciz "var v\x15 = 0x\x14\n\x08"
    .align 2

trans_var_load:
    .ascii "\x0d\x0e"
    .asciz "var v\x15 = gv_\x04\n\x08"
    .align 2

trans_var_store:
    .ascii "\x0d\x0f"
    .asciz "var gv_\x04 = v\x15\n\x08"
    .align 2

trans_footer:
    .asciz "}\n"
    .align 2
