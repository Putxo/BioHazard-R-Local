.intel_syntax noprefix
.code32
.text
.global _start
_start:
    finit
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    mov [before],esp
    mov dword ptr [expected_ok],1
    mov dword ptr [expected_visible],1
    mov dword ptr [expected_writes],1
    mov dword ptr [ratio],0x3F800000
    mov dword ptr [percent],0
    call test
    mov dword ptr [ratio],0x3F400000
    mov dword ptr [percent],25
    call test
    mov dword ptr [ratio],0x3F000000
    mov dword ptr [percent],50
    call test
    mov dword ptr [ratio],0
    mov dword ptr [percent],100
    call test
    mov dword ptr [ratio],0x3DCCCCCD
    mov dword ptr [percent],89
    call test
    mov dword ptr [state],4
    mov dword ptr [expected_writes],0
    mov dword ptr [expected_visible],0
    call test
    mov dword ptr [state],0
    mov dword ptr [expected_ok],0
    mov dword ptr [ratio],0x7FC00000
    call test
    mov dword ptr [ratio],0x7F800000
    call test
    mov dword ptr [ratio],0xBF000000
    call test
    mov dword ptr [ratio],0x3F800001
    call test
    cmp esp,[before]
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
# Test a callback invocation without repairing ESP before checking it.
test:
    mov [test_sp],esp
    fnstcw [cw_before]
    mov dword ptr [writes],0
    mov dword ptr [visible_calls],0
    push 0xA10000
    push 0xB20000
    call rev_january_damage
    add esp,8
    cmp esp,[test_sp]
    jne fail
    cmp eax,[expected_ok]
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
    mov eax,[writes]
    cmp eax,[expected_writes]
    jne fail
    mov eax,[visible_calls]
    cmp eax,[expected_ok]
    jne fail
    fnstcw [cw_after]
    mov ax,[cw_before]
    cmp ax,[cw_after]
    jne fail
    fnstsw ax
    test ax,0x3800
    jnz fail
    ret
.global rev_damage_actor_state
rev_damage_actor_state:
    cmp ecx,0xA10000
    jne fail
    mov eax,[esp+4]
    mov edx,[state]
    mov [eax],edx
    ret 4
.global rev_damage_state_value
rev_damage_state_value:
    mov eax,[ecx]
    xor ecx,ecx
    ret
.global rev_damage_health_ratio
rev_damage_health_ratio:
    cmp ecx,0xA10000
    jne fail
    cmp dword ptr [state],4
    je fail
    fld dword ptr [ratio]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_damage_set_percent
rev_damage_set_percent:
    cmp ecx,0xB20000
    jne fail
    mov eax,[esp+4]
    cmp eax,[percent]
    jne fail
    inc dword ptr [writes]
    xor ecx,ecx
    xor edx,edx
    ret 4
.global rev_damage_set_visible
rev_damage_set_visible:
    cmp ecx,0xB20000
    jne fail
    mov eax,[esp+4]
    cmp eax,[expected_visible]
    jne fail
    inc dword ptr [visible_calls]
    xor ecx,ecx
    ret 4
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
test_sp: .skip 4
state: .skip 4
ratio: .skip 4
percent: .skip 4
expected_ok: .skip 4
expected_visible: .skip 4
expected_writes: .skip 4
writes: .skip 4
visible_calls: .skip 4
cw_before: .skip 2
cw_after: .skip 2
.section .note.GNU-stack,"",@progbits
