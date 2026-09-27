.intel_syntax noprefix
.code32
.text
.global rev_genesis_target_born_gate
rev_genesis_target_born_gate:
    pushfd
    pushad
    push dword ptr [ebp-8]
    push 0x0281AE26
    call rev_genesis_target_event
    add esp,8
    popad
    popfd
    # Reproduce the five overwritten constructor epilogue bytes.
    mov eax,[ebp-8]
    pop edi
    pop esi
    jmp rev_genesis_target_born_resume
.global rev_genesis_target_dying_gate
rev_genesis_target_dying_gate:
    pushfd
    pushad
    push dword ptr [ebp-8]
    push 0x02823A53
    call rev_genesis_target_event
    add esp,8
    popad
    popfd
    # Revoke before the destructor clears its owner or calls any other code.
    mov eax,[ebp-8]
    mov dword ptr [eax],0x04DA8570
    jmp rev_genesis_target_dying_resume
.section .note.GNU-stack,"",@progbits
