.intel_syntax noprefix
.code32
.text
.global rev_genesis_target_focus_gate
rev_genesis_target_focus_gate:
    push ecx
    sub esp,4
    lea edx,[esp]
    push edx
    push ecx
    call rev_genesis_target_focus
    add esp,8
    pop edx
    pop ecx
    test eax,eax
    jz rev_genesis_target_focus_stock
    mov eax,edx
    ret
.global rev_genesis_target_focus_stock
rev_genesis_target_focus_stock:
    push ebp
    mov ebp,esp
    sub esp,0xCC
    jmp rev_genesis_target_focus_resume
.global rev_genesis_target_position_gate
rev_genesis_target_position_gate:
    push ecx
    push dword ptr [esp+8]
    push ecx
    call rev_genesis_target_position
    add esp,8
    pop ecx
    test eax,eax
    jz rev_genesis_target_position_stock
    mov eax,[esp+4]
    ret 4
.global rev_genesis_target_position_stock
rev_genesis_target_position_stock:
    push ebp
    mov ebp,esp
    sub esp,0xCC
    jmp rev_genesis_target_position_resume
.section .note.GNU-stack,"",@progbits
