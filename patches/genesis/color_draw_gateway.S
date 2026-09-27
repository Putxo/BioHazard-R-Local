.intel_syntax noprefix
.code32
.text
.global rev_genesis_color_draw_enabled_gate
rev_genesis_color_draw_enabled_gate:
    push ecx
    call rev_genesis_color_draw_enabled_stock
    movzx eax,al
    push eax
    push dword ptr [esp+4]
    call rev_runtime_unit_draw_enabled
    add esp,8
    pop ecx
    ret
.section .note.GNU-stack,"",@progbits
