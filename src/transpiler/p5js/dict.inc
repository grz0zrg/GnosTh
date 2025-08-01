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
    entrypoint_head_code:       @ define device + Forth registers + setup / draw func.
    .ascii "// 32-bit 512MB 1024x600 device\n"
    .ascii "MEM_SIZE = 0x1fffffff\n"
    .ascii "FB_BASE = 0x1e99a000\n"
    .ascii "DATA_BASE = 0x00 // replace by data area base addr.\n"
    .ascii "rA = 0\n"
    .ascii "rB = 0\n"
    .ascii "rC = 0\n"
    .ascii "rD = 0\n"
    .ascii "mem = new DataView(new ArrayBuffer(MEM_SIZE))\n"
    .ascii "data = [\n/* INSERT DATA CONTENT HERE */\n]\n"
    .ascii "tcx = 16\n"
    .ascii "tcy = 16\n"
    .ascii "function setup() {\n"
    .ascii "createCanvas(1024, 600)\n"
    .ascii "background(0)\n"
    .ascii "fill(255)\n"
    .ascii "noStroke()\n"
    .ascii "textFont('Monospace')\n"
    .ascii "textSize(16)\n"
    .ascii "for (let i = 0; i < data.length; i += 1) {\n"
    .ascii "mem.setInt32(DATA_BASE + i * 4, data[i], true)\n"
    .ascii "}\n"
    .ascii "}\n"
    .ascii "function draw() {\n"
    .ascii "loadPixels()\n"
    .ascii "for (let i = 0; i < pixels.length; i += 4) {\n"
    .ascii "pixels[i + 0] = mem.getUint8(FB_BASE + i + 0)\n"
    .ascii "pixels[i + 1] = mem.getUint8(FB_BASE + i + 1)\n"
    .ascii "pixels[i + 2] = mem.getUint8(FB_BASE + i + 2)\n"
    .ascii "}\n"
    .ascii "updatePixels()\n"
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
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = mem.getInt32(v\x14, true)\n"

forth_word "!"
    .ascii "\x0f\x0f"
    .ascii "mem.setInt32(v\x14, v\x15, true)\n"

forth_word "db@"
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = mem.getUint8(v\x14)\n"

forth_word "db!"
    .ascii "\x0f\x0f"
    .ascii "mem.setUint8(v\x14, v\x15)\n"

forth_word "dlen"
    .ascii "\x0f\x0e\x0e"
    .ascii "var v\x15 = v\x14\n"
    .ascii "var v\x16 = mem.getInt32(v\x14 - 4, true)\n"

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
    .ascii "var v\x15 = v\x14\n"

forth_word "drop"
    .ascii "\x0f"

forth_word "swap"
    .ascii "\x0f\x0f\x0e\x0e"
    .ascii "var v\x16 = v\x14\n"
    .ascii "var v\x17 = v\x15\n"

forth_word "over"
    .ascii "\x0f\x0f\x0e\x0e\x0e"
    .ascii "var v\x16 = v\x15\n"
    .ascii "var v\x17 = v\x14\n"
    .ascii "var v\x18 = v\x15\n"

forth_word "rot"
    .ascii "\x0f\x0f\x0f\x0e\x0e\x0e"
    .ascii "var v\x17 = v\x15\n"
    .ascii "var v\x18 = v\x14\n"
    .ascii "var v\x19 = v\x16\n"

@ =========================== I/O
forth_word "tstc"
    .ascii "// tstc() // TODO\n"

forth_word "getc"
    .ascii "// getc() // TODO\n"

forth_word "gett"
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = performance.now() - v\x14\n"

forth_word "dev_enum"
    .ascii "// dev_enum() // TODO\n"

forth_word "dev_open"
    .ascii "// dev_open() // TODO\n"

forth_word "dev_close"
    .ascii "// dev_close() // TODO\n"

forth_word "dev_read"
    .ascii "// dev_read() // TODO\n"

forth_word "dev_write"
    .ascii "// dev_write() // TODO\n"

forth_word "reset"
    .ascii "// reset() // TODO\n"

forth_word "udelay"
    .ascii "// udelay() // TODO\n"

forth_word "envget"
    .ascii "// envget() // TODO\n"

forth_word "envset"
    .ascii "// envset() // TODO\n"

forth_word "putc"
    .ascii "// putc() // TODO\n"

forth_word "puts"
    .ascii "\x0f\x10\x10"
    .ascii "v\x15 = ''\n"
    .ascii "while (v\x16 = mem.getUint8(v\x14)) {\n"
    .ascii "v\x15 += String.fromCharCode(v\x16)\n"
    .ascii "v\x14 += 1\n"
    .ascii "}\n"
    .ascii "text(v\x15, tcx, tcy)\n"
    .ascii "tcx += textWidth(v\x15)\n"
    .ascii "tcy += floor(tcx / width) * 16\n"
    .ascii "tcx %= width\n"

forth_word "cache_care"
    .ascii "// cache_care() // TODO\n"

@ ====================== GRAPHICS
forth_word "pix"
    .ascii "\x0f\x0f\x0f\x0f\x0f"
    .ascii "mem.setInt32(((v\x15 * v\x16 + v\x17) << 2) + v\x14, v\x18, true)\n"

@ ========================= LOGIC
forth_word "0="
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = (v\x14 === 0) ? -1 : 0\n"

forth_word "0<>"
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = (v\x14 != 0) ? -1 : 0\n"

forth_word "="
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 === v\x14) ? -1 : 0\n"

forth_word "<>"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 != v\x14) ? -1 : 0\n"

forth_word "<"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 < v\x14) ? -1 : 0\n"

forth_word ">"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 > v\x14) ? -1 : 0\n"

forth_word "<="
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 <= v\x14) ? -1 : 0\n"

forth_word ">="
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = (v\x15 >= v\x14) ? -1 : 0\n"

forth_word "~"
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = ~v\x14\n"

forth_word "&"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 & v\x14\n"

forth_word "|"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 | v\x14\n"

forth_word "^"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 ^ v\x14\n"

forth_word "<<"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 << v\x14\n"

forth_word ">>"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 >> v\x14\n"

forth_word ">>>"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 >>> v\x14\n"

@ ================== FLOW CONTROL
forth_word "call"
    .ascii "// call() // TODO\n"

forth_word "loop"
    .ascii "do {\n"

forth_word "again"
    .ascii "} while (1)\n"

forth_word "until"
    .ascii "\x0f"
    .ascii "} while (v\x14 === -1)\n"

forth_word "for"
    .ascii "\x0f\x11\x0e"
    .ascii "for (l\x15 = v\x14; l\x15 >= 0; l\x15 -= 1) {\n"
    .ascii "var v\x16 = l\x15\n"

forth_word "do"                 @ same as "for" but takes 3 parameters instead of 1
    .ascii "\x0f\x0f\x0f\x11\x11\x11\x0e"
    .ascii "l\x17 = v\x16 < v\x15 ? 1 : -1\n"
    .ascii "l\x18 = (i) => l\x17 > 0 ? i <= v\x15 : i >= v\x15\n"
    .ascii "for (l\x19 = v\x16; l\x18(l\x19); l\x19 += v\x14 * l\x17) {\n"
    .ascii "var v\x1a = l\x19\n"

forth_word "next"
    .ascii "}\n"

forth_word "if"
    .ascii "\x0f"
    .ascii "if (v\x14 === -1) {\n"
    .ascii "\x06\x01"

forth_word "else"
    .ascii "} else {\n"
    .ascii "\x06\x02"

forth_word "then"
    .ascii "}\n"
    .ascii "\x07"

forth_word "case"
    .ascii "// case // TODO\n"

@ ===================== REGISTERS
forth_word "A@"
    .ascii "\x0e"
    .ascii "var v\x14 = rA\n"

forth_word "A!"
    .ascii "\x0f"
    .ascii "rA = v\x14\n"

forth_word "B@"
    .ascii "\x0e"
    .ascii "var v\x14 = rB\n"

forth_word "B!"
    .ascii "\x0f"
    .ascii "rB = v\x14\n"

forth_word "C@"
    .ascii "\x0e"
    .ascii "var v\x14 = rC\n"

forth_word "C!"
    .ascii "\x0f"
    .ascii "rC = v\x14\n"

forth_word "D@"
    .ascii "\x0e"
    .ascii "var v\x14 = rD\n"

forth_word "D!"
    .ascii "\x0f"
    .ascii "rD = v\x14\n"

@ ==================== ARITHMETIC
forth_word "negate"
    .ascii "\x0f\x0e"
    .ascii "var v\x15 = -v\x14\n"

forth_word "*"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 * v\x14\n"

forth_word "/"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 / v\x14\n"

forth_word "%"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 % v\x14\n"

forth_word "*/"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "v\x17 = v\x16 * v\x15\n"
    .ascii "var v\x18 = v\x17 / v\x14\n"

forth_word ">>-"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "v\x17 = v\x15 >> v\x14\n"
    .ascii "var v\x18 = v\x16 - v\x17\n"

forth_word ">>+"
    .ascii "\x0f\x0f\x0f\x10\x0e"
    .ascii "v\x17 = v\x15 >> v\x14\n"
    .ascii "var v\x18 = v\x16 + v\x17\n"

forth_word "-"
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 - v\x14\n"

forth_word "+" WORD_RUNTIME IS_LAST_WORD
    .ascii "\x0f\x0f\x0e"
    .ascii "var v\x16 = v\x15 + v\x14\n"
    .asciz "\x08" @ NOTE: mandatory for the last word in transpile mode
    .align 2
forth_dict_end_addr:
999:
