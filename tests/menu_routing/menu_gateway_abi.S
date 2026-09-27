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

.global call_state_gate
call_state_gate:
    push ebp
    mov ebp,esp
    sub esp,8
    push ebx
    push esi
    push edi
    mov eax,[ebp+8]
    mov [ebp-8],eax
    mov eax,[ebp+12]
    mov [ebp+8],eax
    mov ebx,0x11223344
    mov esi,0x55667788
    mov edi,0x12345678
    mov edx,0x76543210
    mov [stack_before],esp
    cmp eax,eax
    pushfd
    pop dword ptr [flags_before]
    call rev_menu_gate_state_commit
    jmp state_after
.global rev_menu_state_commit_continue
rev_menu_state_commit_continue:
    # Test-only continuation returns through the synthetic CALL above.
    ret
state_after:
    mov [stack_after],esp
    pushfd
    pop dword ptr [flags_after]
    mov [state_regs],eax
    mov [state_regs+4],ecx
    mov [state_regs+8],edx
    mov [state_regs+12],ebx
    mov [state_regs+16],esi
    mov [state_regs+20],edi
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret
.global rev_menu_observe_state
rev_menu_observe_state:
    mov eax,[esp+4]
    mov [seen_cockpit],eax
    mov edx,[eax+0x24]
    mov [seen_stored_state],edx
    mov eax,[esp+8]
    mov [seen_state],eax
    mov eax,0xBAD
    mov ecx,eax
    mov edx,eax
    mov ebx,eax
    mov esi,eax
    mov edi,eax
    cmp eax,0
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

.global rev_menu_stock_self
rev_menu_stock_self:
    mov eax,0x33330000
    ret

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

.bss
.global seen_pad,seen_actor
seen_pad:.skip 4
seen_actor:.skip 4
.global seen_cockpit,seen_state,seen_stored_state,state_regs,flags_before,flags_after,stack_before,stack_after
seen_cockpit:.skip 4
seen_state:.skip 4
seen_stored_state:.skip 4
state_regs:.skip 24
flags_before:.skip 4
flags_after:.skip 4
stack_before:.skip 4
stack_after:.skip 4
.section .note.GNU-stack,"",@progbits
