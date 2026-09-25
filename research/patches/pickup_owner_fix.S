.intel_syntax noprefix
.code32
.section .text
.global pickup_member_selector
# ECX is the uItem owner. Return member 1 only for its exact live local Sub0.
# RET 4 preserves the delegate ABI. EAX/EDX are volatile x86 registers.
pickup_member_selector:
    xor eax, eax
    test ecx, ecx
    jz .done
    cmp dword ptr [0x057D9188], 0
    je .done
    mov edx, dword ptr [0x057D9184]
    test edx, edx
    jz .done
    cmp dword ptr [ecx+0xF48], edx
    sete al
.done:
    ret 4
