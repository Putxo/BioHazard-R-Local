# Jump-in gateways for the six hash-verified January normal-loop entries.
# SOURCE ONLY. No code installation or executable generation here.
# Preserve ALL GPRs/EFLAGS and x87/MMX/XMM/MXCSR, then replay the exact two
# displaced instructions. 32-bit SSE/FXSR is a target precondition.
.intel_syntax noprefix
.code32
.text
.extern rev_hud_manager_event
.extern rev_hud_manager_stop
.extern rev_hud_p1_mask_begin
.extern rev_hud_p1_mask_end
.extern rev_hud_cockpit_tail_call
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
.if \draw
    test eax, eax
    jz .Lmask_done_\@
    sub esp, 4
    push dword ptr [ebp+8]
    push dword ptr [ebp-8]
    push \site
    call rev_hud_p1_mask_begin
    add esp, 16
    test eax, eax
    jnz .Lmask_done_\@
    call rev_hud_manager_stop
.Lmask_done_\@:
.endif
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


# Restore the temporary P1-only mDrawView scope after each stock draw loop.
# MiniMap's common epilogue can also be reached without its normal draw loop;
# rev_hud_p1_mask_end is deliberately a no-op when no scope is active.
.macro p1_mask_exit name, site, family
.global rev_hud_mask_exit_\name
.type rev_hud_mask_exit_\name,@function
rev_hud_mask_exit_\name:
    pushfd
    pushad
    mov esi, esp
    and esp, -16
    sub esp, 512
    fxsave [esp]
    cld
    sub esp, 8
    push dword ptr [ebp-8]
    push \site
    call rev_hud_p1_mask_end
    add esp, 16
    test eax, eax
    jnz .Lrestore_ok_\@
    call rev_hud_manager_stop
.Lrestore_ok_\@:
    fxrstor [esp]
    mov esp, esi
    popad
    popfd
.if \family
    pop edi
    pop esi
    pop ebx
    add esp, 0x10C
.else
    mov ecx, dword ptr [ebp-8]
    call rev_hud_cockpit_tail_call
.endif
    jmp rev_hud_continue_mask_exit_\name
.size rev_hud_mask_exit_\name,.-rev_hud_mask_exit_\name
.endm
p1_mask_exit cockpit11, 0x02B49E5C, 0
p1_mask_exit minimap11, 0x02B687F0, 1
