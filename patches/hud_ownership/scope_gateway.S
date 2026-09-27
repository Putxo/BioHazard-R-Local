.intel_syntax noprefix
.code32
.text
.global rev_scope_actor_gate
rev_scope_actor_gate:
    push ecx
    sub esp,4
    lea edx,[esp]
    push edx
    push dword ptr [ebp-8]
    call rev_scope_actor
    add esp,8
    pop edx
    pop ecx
    test eax,eax
    jz rev_scope_actor_stock
    cmp eax,1
    jne 1f
    test edx,edx
    jz 1f
    mov eax,edx
    ret 4
1:
    # Scope phase8 has no null check after the selected-actor finder. Remove
    # this call's return address and predicate argument, then use its epilogue.
    add esp,8
    jmp rev_scope_phase8_end
.section .note.GNU-stack,"",@progbits
