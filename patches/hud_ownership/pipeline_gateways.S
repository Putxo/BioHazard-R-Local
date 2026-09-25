# Source-only CPU-cycle boundaries. No game-image builder or hook installer.
.intel_syntax noprefix
.code32
.text
.extern rev_hud_pipeline_event
.macro pipeline_gate name, site, ending
.global rev_pipeline_gate_\name
.type rev_pipeline_gate_\name,@function
rev_pipeline_gate_\name:
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    sub esp,4
    push ebp
.if \ending
    push dword ptr [ebp-8]
.else
    # [EBP-8] has not been initialized yet; use the saved original ECX.
    push dword ptr [esi+24]
.endif
    push \site
    call rev_hud_pipeline_event
    add esp,16
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
.if \ending
    pop edi
    pop esi
    pop ebx
    add esp,0xD0
.else
    mov dword ptr [ebp-8],ecx
    mov ecx,dword ptr [ebp-8]
.endif
    jmp rev_pipeline_continue_\name
.size rev_pipeline_gate_\name,.-rev_pipeline_gate_\name
.endm
pipeline_gate begin, 0x02F506A0, 0
pipeline_gate end,   0x02F50F4B, 1
.section .note.GNU-stack,"",@progbits
