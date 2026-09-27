.intel_syntax noprefix
.code32
.text
.extern rev_genesis_effect_draw
.macro gate name, kind
.global rev_genesis_\name\()_draw_gate
.type rev_genesis_\name\()_draw_gate,@function
rev_genesis_\name\()_draw_gate:
    push ecx
    push dword ptr [esp+8]
    push dword ptr [esp+4]
    push \kind
    call rev_genesis_effect_draw
    add esp,12
    pop ecx
    test eax,eax
    jz rev_genesis_\name\()_stock_draw
    ret 4
.size rev_genesis_\name\()_draw_gate,.-rev_genesis_\name\()_draw_gate
.endm
gate filter,0
gate noise,1
gate outline,2

.macro standard_stock name, bytes
.global rev_genesis_\name\()_stock_draw
.type rev_genesis_\name\()_stock_draw,@function
rev_genesis_\name\()_stock_draw:
    push ebp
    mov ebp,esp
    sub esp,\bytes
    jmp rev_genesis_\name\()_draw_resume
.size rev_genesis_\name\()_stock_draw,.-rev_genesis_\name\()_stock_draw
.endm
standard_stock filter,0xE4
standard_stock noise,0xCC

.global rev_genesis_outline_stock_draw
.type rev_genesis_outline_stock_draw,@function
rev_genesis_outline_stock_draw:
    push ebx
    mov ebx,esp
    sub esp,8
    jmp rev_genesis_outline_draw_resume
.size rev_genesis_outline_stock_draw,.-rev_genesis_outline_stock_draw
.section .note.GNU-stack,"",@progbits
