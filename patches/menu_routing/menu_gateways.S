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
.extern rev_menu_state_transition
.extern rev_menu_stock_self
.extern rev_menu_stock_state_transition

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
    # Stock is thiscall finder(predicate), ret 4. Forward the predicate rather
    # than letting stock mistake our return address for its stack argument.
    push dword ptr [esp+4]
    call rev_menu_stock_self
    push eax
    call rev_menu_submenu_actor
    add esp,4
    ret 4
.size rev_menu_gate_submenu_actor,.-rev_menu_gate_submenu_actor

# Central sIDCockpit transition ABI:
# ECX=this; stack = return,new_state,flag; original callee ret 8.
# Observe new_state, restore all incoming registers, then tail-jump to stock body.
.global rev_menu_gate_state_transition
.type rev_menu_gate_state_transition,@function
rev_menu_gate_state_transition:
    push eax
    push edx
    push ecx
    push dword ptr [esp+16]
    call rev_menu_state_transition
    add esp,4
    pop ecx
    pop edx
    pop eax
    jmp rev_menu_stock_state_transition
.size rev_menu_gate_state_transition,.-rev_menu_gate_state_transition
.section .note.GNU-stack,"",@progbits
