@    ___                     _   _      
@   / _ \  ____   ___   ___ | |_| |__   
@  / /_\/ |  _ \ / _ \ / __│| __| '_ \  
@ / ╱ ╲ \ | | | | (_) |\__ \| |_| | | | 
@ \ ╲_/ / |_| |_|\___/ |___/ \__|_| |_| 
@  \___/ ARM based Forth dialect by grz 
@ ------------------------------------->

.include "src/utils.inc"

@ =================== PARSE STATE
.equ VOID_PST, 0
.equ DEFINITION_PST, 1
.equ ARRAY_PST, 2

@ ================= SYNTAX TOKENS
.equ START_DEFINITION_TKN, ':'
.equ LINE_COMMENT_TKN, ';'
.equ STORE_TKN, '>'
.equ START_GROUP_TKN, '('
.equ END_GROUP_TKN, ')'
.equ STRING_TKN, '"'
.equ CHAR_TKN, '\''
.equ START_ARRAY_TKN, '['
.equ END_ARRAY_TKN, ']'
.equ START_QUOTATION_TKN, '{'
.equ END_QUOTATION_TKN, '}'
.equ IMM_LITERAL_TKN, '#'
.equ VER_LITERAL_TKN, '$'

@ =================== FIND A WORD
@        last dict. word addr: r2
@                 word length: r5
@             word start addr: r9
@ ===================== ON RETURN
@        found word dict addr:r12
@    found word name end addr:r10
@ ===================== CLOBBERED
@       r6, r7, r8, r10, r11, r12
@ ========================== NOTE
@ return from return stack stored
@ address when word is not found
@ ===============================
find_word:
    mov r12, r2
    0:
        @ note : word can be skipped based on word type for faster compile speed
        ldrb r7, [r12, #9]      @ get dict. word len.
        cmp r5, r7              @ word length match ?
        bne 2f                  @ skip word if not
        add r10, r12, #10       @ get dict word addr.
        mov r11, r9             @ get word addr.
        1:
            ldrb r6, [r11], #1  @ word char.
            ldrb r8, [r10], #1  @ dict. word char.
            cmp r6, r8
            bne 2f              @ skip word on != char.
            subs r7, #1
            bne 1b
            rpop r6             @ flush return stack
            b eval_word         @ found
        2:
        ldr r12, [r12]          @ get previous dict. word addr.
        cmp r12, #0
        rpopf pc eq
        b 0b

@ ================== PARSE NUMBER
@                   data addr: r4
@                 word length: r5
@             word start addr: r9
@                          r12: 0
@ ===================== ON RETURN
@ ===================== CLOBBERED
@        r4, r5, r7, r9, r10, r12
@ ===============================
parse_number:
    ldrb r11, [r9]              @ get first char.
    cmp r11, #IMM_LITERAL_TKN   @ is an immediate ?
    cmpne r11, #VER_LITERAL_TKN @ is a verbatim ?
    bne 7f                      @ yes ? adjust
        add r9, r9, #1
        sub r5, r5, #1
    7:
    ldrb r7, [r9], #1           @ get char.
    subs r10, r7, #87           @ get char. numeric value (a-f)
    sublts r10, r7, #'0'        @ get char. numeric value (0-9)
    addge r12,r10,r12,LSL #4    @ n * 16 + v
    subs r5, #1
    bgt 7b
    compile_number:
    cmp r6, #ARRAY_PST          @ in array ?
    bne 7f
        str r12, [r4], #4       @ put values verbatim in data
        b 1f
    7:                          @ compile
    cmp r11, #IMM_LITERAL_TKN   @ immediate ?
    bne 7f
        push { r12 }            @ push on data stack
        b parser
    7:
    cmp r11, #VER_LITERAL_TKN   @ verbatim value ?
    bne 7f
        str r12, [r14], #4      @ put values verbatim in code
        b parser
    7:
    cmp r12, #255               @ is 8 bits ?
    bhi generic_number
    optimized_number:           @ optimized 8 bits number
        adr r10, number_code_8bits
        ldmia r10, {r9-r10}
        stmia r14!, {r9-r10}    @ store optimized code
        strb r12, [r14, #-4]    @ update 8 bits value
        b parser
    number_code_8bits:          @ 8 bits optimized version (constant load)
        push { r5 }
        mov r5, #0
    generic_number:
        adr r10, number_code_generic
        ldmia r10, {r9-r11}     @ load generated code
        stmia r14!, {r9-r12}    @ store generated code at current definition code addr.
        b parser
    number_code_generic:        @ generic compiled number code
        push { r5 }
        ldr r5, [pc, #0]
        mov pc, pc              @ value is stored after this, stored by "compile" from r12

@ =============== EVALUATE A WORD
@        last dict. word addr: r2
@             word start addr: r9
@    found word name end addr:r10 ; unaligned ok, will be aligned later
@        found word dict addr:r12
@ ===================== ON RETURN
@ ===================== CLOBBERED
@        r5, r7, r8, r9, r10, r12
@ ===============================
eval_word:
    ldrb r8, [r12, #8]          @ get word flag
    add r10, #4                 @ adjust word name end addr for alignment
    bic r10, r10, #3            @ align (point to code addr.)
    cmp r8, #WORD_VARIABLE
    bne 7f
        ldrb r8, [r9, #-1]     @@ check parsed word has a
        cmp r8, #STORE_TKN     @@ store token (to choose store / load code)
        adreq r7, var_assign_code
        adrne r7, var_load_code
        b compile_var           @ generate var. code
    7:
    ands r9, r8, #0xff          @ is an immediate word ?
    bne compile_word
        adr r5, parser          @
        rpush r5                @ return addr.
        mov pc, r10             @ jump to word code
    compile_word:
        ldrb r8, [r2, #8]       @ get last defined word flag
        cmp r8, #WORD_VARIABLE  @ when the last
        mov r8, r2              @ defined word
        bne 7f                  @ is a variable
        find_root_word:         @ we have to find
            ldr r8, [r8]        @ its root definition
            ldrb r9, [r8, #8]
            cmp r9, #WORD_VARIABLE
            beq find_root_word
        7:
        cmp r8, r12             @ detect self call
        bne inline_word
        call_word:              @ a regular word
            adr r11, word_code  @ call (STC) mainly
            ldmia r11, {r7-r9}  @ used to address a
            stmia r14!, {r7-r10}@ word that call itself (recurse case)
            b parser
        inline_word:            @ word inlining
        ldr r9, [r12, #4]       @ get end addr.
        sub r9, r9, #4          @ without RET
        copy_code:
            ldr r8, [r10], #4
            cmp r10, r9         @ word code end reached ?
            bgt parser
            str r8, [r14], #4   @ copy word body to program addr.
            b copy_code
    word_code:                  @ generated call code:
        add r6, pc, #8          @ word code addr. to jump to
        rpush r6                @ will be stored after
        ldr pc, [pc, #-4]       @ this instruction, stored by "compile" from r12

@ ================== FORTH PARSER
@             data stack addr: sp
@           return stack addr: r0
@           input buffer addr: r1
@         dict last word addr: r2
@  program compiled code addr: r3
@                   data addr: r4
@             dict. end addr :r14
@ ===================== ON RETURN
@ ===============================
forth:
    rpush r2
    parser:
    ldrb r11, [r1]             @@
    cmp r11, #0                @@ handle input edge case
    beq go_to_entry_point      @@ directly ending with NUL

    0:
    mov r6, #VOID_PST
    1:                          @ PARSER INITIAL STATE
        adr r7, 2f
        adr r8, 3f
        mov r9, #0
        mov r10, #0
    2:                          @ PARSE LOOP
        ldrb r11, [r1], #1
        cmp r11, #0             @ NUL case (parse ended)
        beq go_to_entry_point
        cmp r11, #START_GROUP_TKN
        beq 2b
        cmp r10, #0             @ skip non printable ?
        cmpeq r11, #' '         @
        movle pc, r7            @ non printable case
        cmp r11, r9
        moveq pc, r7            @ custom character case
        mov pc, r8
    3:                          @ START DEFINITION ?
        cmp r11, #START_DEFINITION_TKN
        bne 5f
            rpushl r12 7f close_definition
            7:
            ldrb r6, [r2, #8]   @ check word type
            cmp r6, #WORD_ENTRY_POINT
            bne 7f              @ if prev. def. was an entry
                ldr r14, [r3]   @ point, restore dict. end
            7:
            mov r6, #DEFINITION_PST
            adr pc, 1b
    5:
        cmp r11, #END_ARRAY_TKN @ ARRAY TERMINATOR ?
        bne 5f
            rpop r7
            sub r8, r4, r7      @ compute length
            sub r8, r8, #4
            lsr r8, r8, #2
            str r8, [r7]        @ store length
            b parser            @ reset parse state
    5:                          @ ARRAY ?
        cmp r11, #START_ARRAY_TKN
        bne 5f
            rpush r4            @ remember data ptr for array length
            mov r6, #ARRAY_PST
            adr r7, 1b
            b compile_static_data
    5:
        cmp r11, #END_QUOTATION_TKN           @ QUOTATION TERMINATOR ?
        bne 5f
            ldr r10, ret_instruction
            str r10, [r14], #4
            adr r7, parser
            b finish_quotation
    5:                          @ QUOTATION ?
        cmp r11, #START_QUOTATION_TKN
        bne 5f
            adr r7, 1b
            b compile_quotation
    5:                          @ CHAR ?
        cmp r11, #CHAR_TKN
        bne 5f
            mov r11, #0
            ldrb r12, [r1], #2
            b compile_number
    5:                          @ STRING ?
        cmp r11, #STRING_TKN
        bne 5f
            adr r7, 7f
            b compile_static_data
            7:
            mov r7, r4          @ remember string start ptr to compute string length
            7:                  @ copy string at data ptr.
                ldrb r11, [r1], #1
                cmp r11, #STRING_TKN
                strneb r11, [r4], #1
                bne 7b
            sub r8, r4, r7      @ compute length
            str r8, [r7, #-4]   @ store length
            mov r8, #0
            strb r8, [r4], #1   @ add NUL character
            add r4, #4
            bic r4, r4, #3      @ align code addr.
            b parser
    5:                          @ COMMENT ?
        cmp r11, #LINE_COMMENT_TKN
        bne 5f
            adr r7, 1b
            adr r8, 2b
            mov r9, #'\n'
            mov r10, #1
            b 2b
    5:                          @ DEFINITION STATE ?
        cmp r6, #DEFINITION_PST
        bne 5f
            rpop r7             @ get previous dict. entry addr.
            str r14, [r7, #4]   @ link previous to current (dict. only)
            rpush r14           @ push new dict. entry addr.
            rpushl r12 7f start_definition
            7:
            adr r7, 7f
            mov r9, #0
            mov r10, #0
            adr r8, copy_definition_character
            b copy_definition_character
            7:
            mov r8, #WORD_RUNTIME
            rpushl r7 parser finish_definition_name
    5:                          @ WORD STATE (default)
        sub r12, r1, #1         @ save word start addr.
        adr r7, finish_word_parse
        adr r8, 2b
        mov r9, #END_GROUP_TKN
        mov r10, #0
        b 2b

@ ============= GO TO ENTRY POINT ; code is
@ program compiled code addr. :r3 ; compiled at
@ ===================== ON RETURN ; this point
@ ===================== CLOBBERED ; so we jump
@ =============================== ; to it !
go_to_entry_point:
    rpop r7                     @ clean return stack
    add pc, r3, #4

@ ============= FINISH WORD PARSE
@          current input addr: r1
@                   data addr: r4
@             word start addr:r12
@      current def. code addr:r14
@ ===================== ON RETURN
@ ===============================
finish_word_parse:
    sub r5, r1, r12             @ compute word length
    sub r5, r5, #1              @ due to r1 auto. increment
    mov r9, r12                 @ assign word start addr.
    cmp r6, #ARRAY_PST         @@ in an array case we just
    moveq r12, #0              @@ parse numbers but make
    beq parse_number           @@ sure that r12 is 0 first
    cmp r5, #1                  @ no special case for single char. 
    beq 7f
    ldrb r7, [r12]
    cmp r7, #STORE_TKN          @ check variable store tkn
    bne 7f
    ldrb r7, [r12, #1]
    cmp r7, #'@'                @ variable name must starts with specific characters (makes words such as >= or >> etc. works)
    bgt 8f
    7:                          @ find regular word
        rpushl r7 parse_number find_word
    8:                          @ find variable word (will define it if not found)
        sub r5, r5, #1          @ adjust length (don't include token)
        add r9, r9, #1          @ adjust word start. (don't include token)
        rpushl r7 variable_definition find_word

@ ===============================
variable_definition:
    sub r1, r1, r5              @ rewind for copy
    sub r1, r1, #1              @ adjust back
    mov r6, r14                 @ save code addr.
    mov r14, r4                 @ point to data addr. for def.
    rpushl r12 7f start_definition
    7:
        adr r7, 7f
        mov r9, #0
        mov r10, #0
        adr r8, copy_definition_character
        b 2b
    7:
        mov r8, #WORD_VARIABLE
        rpushl r7 7f finish_definition_name
    7:
        add r4, r14, #4         @ point to next data def. (skip stored value)
        mov r10, r14            @ var. code addr.
        mov r14, r6             @ restore code addr.
        adr r7, var_assign_code
        b compile_var

@ ============== START DEFINITION
@       last dict. entry addr: r2
@      current def. code addr:r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                         r2, r14
@ ===============================
start_definition:
    str r2, [r14], #10          @ link current to previous and point to name field
    sub r2, r14, #10            @ upd. last dict. word addr.
    ret_instruction:
    ret

@ ======== FINISH DEFINITION NAME
@      last dict. entry addr.: r2
@                   word type: r8
@      current def. code addr:r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                     r5, r7, r14
@ ===============================
finish_definition_name:
    strb r8, [r2, #8]           @ assign word type
    mov r7, #0
    strb r7, [r14]              @ append def. name \0
    add r7, r2, #10
    sub r5, r14, r7             @ compute length
    strb r5, [r2, #9]           @ store length
    add r14, #4
    bic r14, #3                 @ align end of dict. addr.
    ret

@ ============== CLOSE DEFINITION
@      last dict. entry addr.: r2
@ program compiled code addr. :r3
@     current def. code addr: r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                         r7, r14
@ ===============================
close_definition:
    ldrb r5, [r2, #8]           @ check definition type
    cmp r5, #WORD_ENTRY_POINT   @ was an entry point ?
    ldreq r14, [r3]             @ yes ? restore default code target (dict.)

    ldr r7, ret_instruction     @ get RET opcode
    str r7, [r14], #4           @ store RET opcode

    mov r7, #WORD_RUNTIME
    strb r7, [r14, #8]          @ initialize next word type
    ret

@ = COMPILE DATA (STRING / VALUE)
@                   data addr: r4
@           current code addr:r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@           r8, r9, r10, r11, r14
@ ===============================
compile_static_data:
    add r4, r4, #4              @ +4 because length will be stored prior data
    mov r11, r4                 @ get static data addr.
    adr r10, number_code_generic
    ldmia r10, {r8-r10}         @ compile data code
    stmia r14!, {r8-r11}
    mov pc, r7

@ ============= COMPILE QUOTATION
@           current code addr:r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@           r8, r9, r10, r11, r14
@ ===============================
compile_quotation:
    add r10, r14, #20           @ compute length addr
    rpush r10                   @ push it, will be updated later
    adr r10, quotation_code
    ldmia r10, {r8-r12}         @ compile code
    stmia r14!, {r8-r12}
    mov pc, r7
    quotation_code:
        push { r5 }
        add r5, pc, #8          @ put data start addr on stack
        ldr r6, [pc]            @ get quotation length
        add pc, pc, r6          @ jump over
        quotation_length:
        .word 0

@ ============== FINISH QUOTATION
@           current code addr:r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                             r10
@ ===============================
finish_quotation:
    rpop r10                    @ get back length addr
    sub r8, r14, r10            @ compute body length
    str r8, [r10, #-4]          @ update it
    mov pc, r7

@ ============ COMPILE A VARIABLE
@    assign or load code addr: r7
@         variable value addr:r10
@ ===================== ON RETURN
@ ===================== CLOBBERED
@  r7, r8, r9, r10, r11, r12, r14
@ ===============================
compile_var:
    str r10, var_value_addr     @ store value address
    adr r10, var_code
    ldmia r10, {r8-r12}         @ load code
    stmia r14!, {r8-r12}        @ store it
    ldmia r7, {r8-r9}           @ load assign / load part
    sub r7, r14, #16            @ adjust
    stmia r7, {r8-r9}           @ replace
    b 0b

@ ===============================
var_assign_code:
    str r5, [r6]
    pop { r5 }

@ ===============================
var_load_code:
    push { r5 }
    ldr r5, [r6] 

@ ===============================
var_code:
    ldr r6, [pc, #8]
    .word 0                    @@ will be
    .word 0                    @@ replaced
    mov pc, pc
    var_value_addr:
    .word 0

@ == COPY CHARACTER TO CODE ADDR.
@     current def. code addr: r14
@ ===================== ON RETURN
@ ===================== CLOBBERED
@                             r14
@ ===============================
copy_definition_character:
    strb r11, [r14], #1
    b 2b
