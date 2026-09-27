.intel_syntax noprefix
.code32
.text
.global rev_genesis_effect_toggle_gate
rev_genesis_effect_toggle_gate:
    push ecx
    movzx eax,byte ptr [esp+8]
    push eax
    push ecx
    call rev_genesis_effect_toggle
    add esp,8
    pop ecx
    test eax,eax
    jz rev_genesis_effect_toggle_stock
    ret 4
.global rev_genesis_effect_toggle_stock
rev_genesis_effect_toggle_stock:
    push ebp
    mov ebp,esp
    sub esp,0xCC
    jmp rev_genesis_effect_toggle_resume
.section .note.GNU-stack,"",@progbits
