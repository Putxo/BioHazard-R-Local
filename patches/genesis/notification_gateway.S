.intel_syntax noprefix
.code32
.text
.global rev_genesis_notify_gate
rev_genesis_notify_gate:
    push ecx
    push ecx
    push dword ptr [ebp-0x300]
    push dword ptr [ebp-0x1B8]
    call rev_genesis_notify
    add esp,12
    pop ecx
    test eax,eax
    jz rev_genesis_notify_stock
    ret
.section .note.GNU-stack,"",@progbits
