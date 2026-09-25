.intel_syntax noprefix
.code32
.text
.global pcs_script_member
# ECX: cFsmAction or cFsmActionPcs owner; one original event argument on stack.
# Only the exact cFsmActionPcsSub vtable permits reading its extended actor field.
# Compare pointers before dereferencing the tracked actor. No engine calls/writes.
pcs_script_member:
    xor eax, eax
    test ecx, ecx
    jz .done
    cmp dword ptr [0x057D9188], 0
    je .done
    cmp dword ptr [ecx], 0x04DCF41C
    jne .done
    mov edx, dword ptr [0x057D9184]
    test edx, edx
    jz .done
    cmp dword ptr [ecx+0x1078], edx
    jne .done
    cmp dword ptr [edx+0xE40], 1
    sete al
.done:
    ret 4
.section .note.GNU-stack,"",@progbits
