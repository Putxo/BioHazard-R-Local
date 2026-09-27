.intel_syntax noprefix
.code32
.text
.global rev_genesis_selected_actor
rev_genesis_selected_actor:
    push ecx
    sub esp, 4
    lea edx, [esp]
    push edx
    push 6
    push dword ptr [ebp-8]
    call rev_hud_resolve_actor
    add esp, 12
    pop edx
    pop ecx
    test eax, eax
    jz 1f
    mov eax, edx
    ret 4
1:
    jmp rev_genesis_stock_selected_actor
.section .note.GNU-stack,"",@progbits
