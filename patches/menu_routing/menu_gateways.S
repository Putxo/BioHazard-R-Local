# Source-only jump/call targets for the six audited January callsites.
# No installation or game-image generation is performed here.
.intel_syntax noprefix
.code32
.text
.extern rev_menu_pause_open_word
.extern rev_menu_submenu_open_word
.extern rev_menu_submenu_word_198
.extern rev_menu_submenu_word_1a0
.extern rev_menu_submenu_actor
.extern rev_menu_stock_self

# Original ABI at the four sPad calls: ECX=sPad, one dummy DWORD argument,
# callee cleans 4 bytes. The bridge converts to cdecl and keeps ret 4.
.macro pad_bridge name, target
.global rev_menu_gate_\name
.type rev_menu_gate_\name,@function
rev_menu_gate_\name:
    push ecx
    call \target
    add esp,4
    ret 4
.size rev_menu_gate_\name,.-rev_menu_gate_\name
.endm
pad_bridge pause_open, rev_menu_pause_open_word
pad_bridge submenu_open, rev_menu_submenu_open_word
pad_bridge submenu_198, rev_menu_submenu_word_198
pad_bridge submenu_1a0, rev_menu_submenu_word_1a0

# Original Self finder is still executed first with its untouched ECX/context.
# Only its returned actor may be replaced by the router.
.global rev_menu_gate_submenu_actor
.type rev_menu_gate_submenu_actor,@function
rev_menu_gate_submenu_actor:
    call rev_menu_stock_self
    push eax
    call rev_menu_submenu_actor
    add esp,4
    ret
.size rev_menu_gate_submenu_actor,.-rev_menu_gate_submenu_actor
.section .note.GNU-stack,"",@progbits
