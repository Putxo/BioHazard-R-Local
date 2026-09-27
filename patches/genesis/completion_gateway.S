.intel_syntax noprefix
.code32
.text
.global rev_genesis_complete_gate
rev_genesis_complete_gate:
    # Replaces mov eax,[edx+14]; call eax in the scanner's completion frame.
    pushfd
    pushad
    push ecx
    push dword ptr [ebp-8]
    call rev_genesis_complete
    add esp,8
    test eax,eax
    jz .stock
    cmp eax,1
    jne .abandon
    popad
    popfd
    ret
.stock:
    popad
    popfd
    mov eax,[edx+0x14]
    jmp eax
.abandon:
    popad
    popfd
    add esp,4
    jmp rev_genesis_scan_end
.global rev_genesis_scan_call_gate
rev_genesis_scan_call_gate:
    call rev_genesis_scan_stock
    pushfd
    pushad
    push dword ptr [ebp-8]
    call rev_genesis_scan_aborted
    add esp,4
    test eax,eax
    jnz .scan_abandon
    popad
    popfd
    ret
.scan_abandon:
    popad
    popfd
    add esp,4
    jmp rev_genesis_state_end
.global rev_genesis_state_call_gate
rev_genesis_state_call_gate:
    push dword ptr [esp+4]
    call rev_genesis_state_stock
    pushfd
    pushad
    push dword ptr [ebp-8]
    call rev_genesis_scan_aborted
    add esp,4
    test eax,eax
    jnz .state_abandon
    popad
    popfd
    ret 4
.state_abandon:
    popad
    popfd
    add esp,8
    jmp rev_genesis_phase9_end
.section .note.GNU-stack,"",@progbits
