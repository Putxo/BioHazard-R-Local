# Source-only i386 bridges for the three verified Self call sites.
# No installer invokes these yet. The final engine linker must resolve
# rev_hud_stock_self to the original thunk 0x01C2C7F4.
# The caller's EBP is deliberately retained: [EBP-8] is its widget this.
# Original ECX and the one original stack argument reach stock unchanged.
.intel_syntax noprefix
.code32
.text
.extern rev_hud_resolve_actor
.extern rev_hud_stock_self
.macro HUD_BRIDGE name, kind
.global \name
.type \name, @function
\name:
    push ecx
    sub esp, 4
    lea edx, [esp]
    push edx
    push \kind
    push dword ptr [ebp-8]
    call rev_hud_resolve_actor
    add esp, 12
    pop edx
    pop ecx
    test eax, eax
    jz 1f
    mov eax, edx
    ret 4
1:
    jmp rev_hud_stock_self
.size \name, .-\name
.endm
HUD_BRIDGE rev_hud_reticle_self, 0
HUD_BRIDGE rev_hud_equipment_self, 1
HUD_BRIDGE rev_hud_herb_self, 2
.section .note.GNU-stack,"",@progbits
