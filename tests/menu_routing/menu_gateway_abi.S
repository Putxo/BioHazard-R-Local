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
    push dword ptr [ebp+12]
    mov ecx,[ebp+8]
    call dword ptr [ebp+16]
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

# Synthetic stock Self finder preserves only the observable return contract.
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
cdecl1 rev_menu_submenu_open_word,0x9001
cdecl1 rev_menu_submenu_word_198,0xA198
cdecl1 rev_menu_submenu_word_1a0,0xA1A0
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
.section .note.GNU-stack,"",@progbits
