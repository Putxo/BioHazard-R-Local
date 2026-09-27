.intel_syntax noprefix
.code32
.text
.macro WEAPON name, site, dying
.global rev_genesis_weapon_\name\()_gate
rev_genesis_weapon_\name\()_gate:
    .if !\dying
    mov dword ptr [eax],0x04D38EE4
    .endif
    pushfd
    pushad
    push eax
    push \site
    call rev_genesis_weapon_event
    add esp,8
    popad
    popfd
    .if \dying
    mov dword ptr [eax],0x04D38EE4
    .endif
    jmp rev_genesis_weapon_\name\()_resume
.endm
WEAPON born,0x0248731E,0
WEAPON dying,0x02487426,1

# At the first store in activate: EAX=Scanner, ECX=weapon. Refusal leaves the
# native object untouched and unwinds through its existing ret8 epilogue.
.global rev_genesis_weapon_retain_gate
rev_genesis_weapon_retain_gate:
    pushfd
    pushad
    push ecx
    push eax
    call rev_genesis_weapon_retain
    add esp,8
    test eax,eax
    jz 1f
    popad
    popfd
    mov dword ptr [eax+0x2FC],ecx
    jmp rev_genesis_weapon_retain_resume
1:
    popad
    popfd
    jmp rev_genesis_weapon_activate_end
.section .note.GNU-stack,"",@progbits
