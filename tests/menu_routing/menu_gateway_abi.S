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
    mov [stack_before],esp
    push dword ptr [ebp+12]
    mov ecx,[ebp+8]
    call rev_menu_gate_submenu_actor
    mov [stack_after],esp
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
    mov [seen_finder_this],ecx
    mov eax,[esp+4]
    mov [seen_predicate],eax
    mov eax,0x33330000
    ret 4

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
cdecl1 rev_menu_pause_word_198,0xA198
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

.global call_options_tests
call_options_tests:
    pushad
    mov [stack_before],esp
    mov ecx,0x11112222
    call rev_menu_gate_options_current
    cmp dword ptr [option_config],0x11112222
    jne option_fail
    cmp dword ptr [option_reference],0
    jne option_fail
    cmp esp,[stack_before]
    jne option_fail
    mov ecx,0x33334444
    call rev_menu_gate_options_reference
    cmp dword ptr [option_config],0x33334444
    jne option_fail
    cmp dword ptr [option_reference],1
    jne option_fail
    cmp esp,[stack_before]
    jne option_fail
    push 0x89
    push 0x67
    mov ecx,0x55556666
    call rev_menu_gate_options_apply
    cmp eax,0xABCD0001
    jne option_fail
    cmp dword ptr [option_config],0x55556666
    jne option_fail
    cmp dword ptr [option_save],0x67
    jne option_fail
    cmp dword ptr [option_flag],0x89
    jne option_fail
    cmp esp,[stack_before]
    jne option_fail
    mov dword ptr [synthetic_pad+0x970],0
    call test_options_ecx
    call test_options_edx
    mov dword ptr [synthetic_pad+0x970],1
    call test_options_ecx
    call test_options_edx
    popad
    xor eax,eax
    ret
option_fail:
    cld
    mov esp,[stack_before]
    popad
    mov eax,40
    ret

.macro test_index name,base,destination,other
test_options_\name:
    mov eax,0x12345678
    mov ecx,0x23456789
    mov edx,0x3456789A
    lea \base,[synthetic_pad]
    mov \other,0xAAAAAAAA
    mov ebx,0x456789AB
    mov esi,0x56789ABC
    mov edi,0x6789ABCD
    mov ebp,0x789ABCDE
    fninit
    fld1
    pcmpeqd xmm0,xmm0
    stc
    std
    pushfd
    pop dword ptr [option_flags]
    call rev_menu_gate_options_\name
    pushfd
    pop dword ptr [option_flags_after]
    cld
    cmp \destination,1
    jne option_fail
    cmp \other,0xAAAAAAAA
    jne option_fail
    cmp ebx,0x456789AB
    jne option_fail
    cmp esi,0x56789ABC
    jne option_fail
    cmp edi,0x6789ABCD
    jne option_fail
    cmp ebp,0x789ABCDE
    jne option_fail
    lea \destination,[synthetic_pad]
    cmp \base,\destination
    jne option_fail
    mov eax,[option_flags]
    cmp eax,[option_flags_after]
    jne option_fail
    mov eax,[option_stock]
    cmp eax,[synthetic_pad+0x970]
    jne option_fail
    lea eax,[synthetic_pad]
    cmp eax,[option_pad]
    jne option_fail
    movd eax,xmm0
    cmp eax,-1
    jne option_fail
    fstp dword ptr [option_float]
    cmp dword ptr [option_float],0x3F800000
    jne option_fail
    mov eax,esp
    add eax,4
    cmp eax,[stack_before]
    jne option_fail
    ret
.endm
test_index ecx,eax,ecx,edx
test_index edx,ecx,edx,eax

.global rev_menu_options_read
rev_menu_options_read:
    mov eax,[esp+4]
    mov [option_config],eax
    mov eax,[esp+8]
    mov [option_reference],eax
    ret
.global rev_menu_options_apply
rev_menu_options_apply:
    mov eax,[esp+4]
    mov [option_config],eax
    mov eax,[esp+8]
    mov [option_save],eax
    mov eax,[esp+12]
    mov [option_flag],eax
    mov eax,0xABCD0001
    ret
.global rev_menu_options_index
rev_menu_options_index:
    pushfd
    pop eax
    test eax,0x400
    jnz option_fail
    lea eax,[esp+4]
    test eax,15
    jnz option_fail
    mov eax,[esp+4]
    mov [option_pad],eax
    mov eax,[esp+8]
    mov [option_stock],eax
    mov ecx,-1
    mov edx,-1
    pxor xmm0,xmm0
    fninit
    mov eax,1
    ret

.bss
.align 16
synthetic_pad:.skip 0x974
option_config:.skip 4
option_reference:.skip 4
option_save:.skip 4
option_flag:.skip 4
option_pad:.skip 4
option_stock:.skip 4
option_flags:.skip 4
option_flags_after:.skip 4
option_float:.skip 4
.global seen_finder_this,seen_predicate
seen_finder_this:.skip 4
seen_predicate:.skip 4
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
