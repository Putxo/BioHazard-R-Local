.intel_syntax noprefix
.code32
.text
.global script_sub0_member
# January-only constants. This is an original leaf helper, not game code.
.equ LOCAL_ACTIVE, 0x057D9188
.equ TRACKED_SUB0, 0x057D9184
.equ FSM_SUB_VTABLE, 0x04DCF41C
# thiscall: ECX is the command owner; one event argument, ignored as in stock.
# Read extended owner fields ONLY after verifying the exact concrete subtype.
# Never dereference the context field itself: it must equal the live tracker.
script_sub0_member:
    xor eax,eax
    test ecx,ecx
    jz .done
    cmp dword ptr [LOCAL_ACTIVE],0
    je .done
    cmp dword ptr [ecx],FSM_SUB_VTABLE
    jne .done
    mov edx,dword ptr [TRACKED_SUB0]
    test edx,edx
    jz .done
    cmp dword ptr [edx+0xE40],1
    jne .done
    cmp dword ptr [ecx+0x1078],edx
    jne .done
    mov edx,dword ptr [edx+0xE3C]
    test edx,edx
    js .done
    cmp dword ptr [ecx+0x1074],edx
    sete al
.done:
    ret 4
.section .note.GNU-stack,"",@progbits
