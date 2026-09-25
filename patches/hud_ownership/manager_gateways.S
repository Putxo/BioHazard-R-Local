# Jump-in gateways for the six hash-verified January normal-loop entries.
# SOURCE ONLY. No code installation or executable generation here.
# Preserve ALL GPRs/EFLAGS and x87/MMX/XMM/MXCSR, then replay the exact two
# displaced instructions. 32-bit SSE/FXSR is a target precondition.
.intel_syntax noprefix
.code32
.text
.extern rev_hud_manager_event
.macro manager_gateway name, site, family, draw
.global rev_hud_gate_\name
.type rev_hud_gate_\name,@function
rev_hud_gate_\name:
    pushfd
    pushad
    mov esi, esp
    and esp, -16
    sub esp, 512
    fxsave [esp]
    cld
    sub esp, 4
.if \draw
    push dword ptr [ebp+8]
.else
    push 0
.endif
    push dword ptr [ebp-8]
    push \site
    call rev_hud_manager_event
    add esp, 16
    fxrstor [esp]
    mov esp, esi
    popad
    popfd
    mov eax, dword ptr [ebp-8]
.if \family
    add eax, 0x30
.else
    mov ecx, dword ptr [eax+0x3C]
.endif
    jmp rev_hud_continue_\name
.size rev_hud_gate_\name,.-rev_hud_gate_\name
.endm
manager_gateway cockpit8,  0x02B497CA, 0, 0
manager_gateway cockpit9,  0x02B49C5B, 0, 0
manager_gateway cockpit11, 0x02B49DE3, 0, 1
manager_gateway minimap8,  0x02B6847B, 1, 0
manager_gateway minimap9,  0x02B685B3, 1, 0
manager_gateway minimap11, 0x02B68724, 1, 1
.section .note.GNU-stack,"",@progbits
