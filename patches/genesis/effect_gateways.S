.intel_syntax noprefix
.code32
.text
// All six replaced instructions are mov [eax],vtable. EAX names the unit.
// Record birth after the store, revoke death before it. No native call or
// constructor/destructor body is skipped. These sites have no live FP result.
.macro EFFECT name, site, vtable, dying
.global rev_genesis_effect_\name\()_gate
rev_genesis_effect_\name\()_gate:
    .if !\dying
    mov dword ptr [eax],\vtable
    .endif
    pushfd
    pushad
    push eax
    push \site
    call rev_genesis_effect_event
    add esp,8
    popad
    popfd
    .if \dying
    mov dword ptr [eax],\vtable
    .endif
    jmp rev_genesis_effect_\name\()_resume
.endm
EFFECT filter_born, 0x0293344E, 0x04DB80CC, 0
EFFECT filter_dying, 0x02933576, 0x04DB80CC, 1
EFFECT noise_born, 0x0293559E, 0x04DB833C, 0
EFFECT noise_dying, 0x02935816, 0x04DB833C, 1
EFFECT outline_born, 0x037DD309, 0x04F9A7F4, 0
EFFECT outline_dying, 0x037DD4A2, 0x04F9A7F4, 1
.section .note.GNU-stack,"",@progbits
