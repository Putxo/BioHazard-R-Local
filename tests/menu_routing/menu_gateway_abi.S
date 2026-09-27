.intel_syntax noprefix
.code32
.text
.global _start
_start:
    and esp,-16
    call run_tests
    mov ebx,eax
    mov eax,1
    int 0x80

.global call_pad_gate
call_pad_gate:
    push ebp
    mov ebp,esp
    mov [stack_before],esp
    push dword ptr [ebp+12]
    mov ecx,[ebp+8]
    call dword ptr [ebp+16]
    mov [stack_after],esp
    mov esp,ebp
    pop ebp
    ret

.global call_actor_gate
call_actor_gate:
    push ebp
    mov ebp,esp
    mov ecx,[ebp+8]
    call rev_menu_gate_submenu_actor
    mov esp,ebp
    pop ebp
    ret

.global call_state_gate
call_state_gate:
    push ebp
    mov ebp,esp
    mov [stack_before],esp
    push dword ptr [ebp+16]
    push dword ptr [ebp+12]
    mov ecx,[ebp+8]
    mov eax,0x11223344
    mov edx,0x55667788
    call rev_menu_gate_state_transition
    mov [stack_after],esp
    mov esp,ebp
    pop ebp
    ret

.global rev_menu_stock_self
rev_menu_stock_self:
    mov eax,0x33330000
    ret

.global rev_menu_stock_state_transition
rev_menu_stock_state_transition:
    mov [seen_incoming_eax],eax
    mov [seen_incoming_edx],edx
    mov [seen_state_this],ecx
    mov eax,[esp+4]
    mov [seen_state],eax
    mov eax,[esp+8]
    mov [seen_flag],eax
    mov eax,0xC0DEC0DE
    ret 8

.macro cdecl1 name,value
.global \name
\name:
    mov eax,[esp+4]
    mov [seen_pad],eax
    mov eax,\value
    ret
.endm
cdecl1 rev_menu_pause_open_word,0x8008
cdecl1 rev_menu_pause_word_1a0,0xA1A0
cdecl1 rev_menu_pause_word_1ac,0xA1AC
cdecl1 rev_menu_submenu_open_word,0x9001
cdecl1 rev_menu_submenu_word_198,0xB198
cdecl1 rev_menu_submenu_word_1a0,0xB1A0

.global rev_menu_submenu_actor
rev_menu_submenu_actor:
    mov eax,[esp+4]
    mov [seen_actor],eax
    add eax,0x10
    ret

.global rev_menu_state_transition
rev_menu_state_transition:
    mov eax,[esp+4]
    mov [seen_observed_state],eax
    mov eax,0xDEADBEEF
    mov edx,0xBAADF00D
    ret

.bss
.global seen_pad,seen_actor,seen_state_this,seen_state,seen_flag,seen_observed_state
seen_pad:.skip 4
seen_actor:.skip 4
seen_state_this:.skip 4
seen_state:.skip 4
seen_flag:.skip 4
seen_observed_state:.skip 4
.global stack_before,stack_after,seen_incoming_eax,seen_incoming_edx
stack_before:.skip 4
stack_after:.skip 4
seen_incoming_eax:.skip 4
seen_incoming_edx:.skip 4
.section .note.GNU-stack,"",@progbits
