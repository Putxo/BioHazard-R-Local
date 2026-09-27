# Source-only jump/call targets for audited January callsites.
# No installation or game-image generation is performed here.
.intel_syntax noprefix
.code32
.text
.extern rev_menu_pause_open_word
.extern rev_menu_pause_word_1a0
.extern rev_menu_pause_word_1ac
.extern rev_menu_submenu_open_word
.extern rev_menu_submenu_word_198
.extern rev_menu_submenu_word_1a0
.extern rev_menu_submenu_actor
.extern rev_menu_stock_self
.extern rev_menu_observe_state
.extern rev_menu_state_commit_continue

# Original ABI at sPad wrapper calls: ECX=sPad, one dummy DWORD argument,
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
pad_bridge pause_1a0, rev_menu_pause_word_1a0
pad_bridge pause_1ac, rev_menu_pause_word_1ac
pad_bridge submenu_open, rev_menu_submenu_open_word
pad_bridge submenu_198, rev_menu_submenu_word_198
pad_bridge submenu_1a0, rev_menu_submenu_word_1a0

.global rev_menu_gate_submenu_actor
.type rev_menu_gate_submenu_actor,@function
rev_menu_gate_submenu_actor:
    call rev_menu_stock_self
    push eax
    call rev_menu_submenu_actor
    add esp,4
    ret
.size rev_menu_gate_submenu_actor,.-rev_menu_gate_submenu_actor

# Inline JMP at 02CDD415 replaces nine bytes. Replay loads/store, then
# preserve GPRs/flags across notification. Continuation is before pop edi.
.global rev_menu_gate_state_commit
.type rev_menu_gate_state_commit,@function
rev_menu_gate_state_commit:
    mov eax,[ebp-8]
    mov ecx,[ebp+8]
    mov [eax+0x24],ecx
    pushfd
    pushad
    push ecx
    push eax
    call rev_menu_observe_state
    add esp,8
    popad
    popfd
    jmp rev_menu_state_commit_continue
.size rev_menu_gate_state_commit,.-rev_menu_gate_state_commit
.section .note.GNU-stack,"",@progbits
