.intel_syntax noprefix
.code32
.text
.global rev_hud_heal_gate
rev_hud_heal_gate:
    push ecx
    push dword ptr [ebp-8]
    call rev_hud_heal_request
    add esp,4
    pop ecx
    test eax,eax
    jnz .Lstock
    ret
.Lstock:
    jmp rev_hud_stock_heal
.section .note.GNU-stack,"",@progbits
