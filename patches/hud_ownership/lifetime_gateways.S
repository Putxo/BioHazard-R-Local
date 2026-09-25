# Source-only January lifetime entries. No patching or executable builder.
# All sites must be installed before construction starts. Requires x86 FXSR/SSE.
.intel_syntax noprefix
.code32
.text
.extern rev_hud_lifetime_event
.macro lifetime_gateway name, site, shape, frame_size=0
.global rev_life_gate_\name
.type rev_life_gate_\name,@function
rev_life_gate_\name:
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    sub esp,8
.if \shape == 1
    # Destructor entry: saved original ECX, before a new EBP frame exists.
    push dword ptr [esi+24]
.else
    push dword ptr [ebp-8]
.endif
    push \site
    call rev_hud_lifetime_event
    add esp,16
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
.if \shape == 0
    # Final constructor return value and saved nonvolatile register pops.
    mov eax,dword ptr [ebp-8]
    pop edi
    pop esi
    pop ebx
.elseif \shape == 1
    push ebp
    mov ebp,esp
    sub esp,\frame_size
.elseif \shape == 2
    mov dword ptr [ebp-0x2C],eax
    mov ecx,dword ptr [ebp-8]
.elseif \shape == 3
    push edx
    mov ecx,ebp
    push eax
    lea edx,[rev_life_binder_debug_table]
.endif
    jmp rev_life_continue_\name
.size rev_life_gate_\name,.-rev_life_gate_\name
.endm
lifetime_gateway npc_born,      0x0277D4FB,0
lifetime_gateway cockpit_born,  0x02B47907,0
lifetime_gateway minimap_born,  0x02B67F51,0
lifetime_gateway npc_dying,     0x0277DC70,1,0xD8
lifetime_gateway player_dying,  0x027B5D00,1,0xCC
lifetime_gateway cockpit_dying, 0x02B47A30,1,0x2C8
lifetime_gateway minimap_dying, 0x02B68040,1,0xF4
lifetime_gateway main_dying,    0x02DF5970,1,0xCC
lifetime_gateway sub_dying,     0x02DF5F90,1,0xCC
lifetime_gateway bind_begin,    0x02DF4F99,2
lifetime_gateway bind_end,      0x02DF504B,3
.section .note.GNU-stack,"",@progbits
