.intel_syntax noprefix
.code32
.text
.global rev_aim_visibility_gate
rev_aim_visibility_gate:
    # Existing mismatch branch at 0279CFBD. P1 never passes through this gate.
    # Preserve the original function frame and its caller-visible return state.
    pushfd
    pushad
    movzx eax,byte ptr [ebp+12]
    push eax
    movzx eax,byte ptr [ebp+8]
    push eax
    push dword ptr [ebp-8]
    call rev_aim_visibility
    add esp,12
    popad
    popfd
    jmp rev_aim_visibility_resume
.section .note.GNU-stack,"",@progbits
