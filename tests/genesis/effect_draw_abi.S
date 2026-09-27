.intel_syntax noprefix
.code32
.text
.macro exercise name, kind
    mov dword ptr [expected_kind],\kind
    mov [before],esp
    push 0xABCD1000
    mov ecx,0xABCD2000
    call rev_genesis_\name\()_draw_gate
    cmp esp,[before]
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
.endm
.global _start
_start:
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    exercise filter,0
    exercise noise,1
    exercise outline,2
    cmp dword ptr [stock_calls],3
    jne fail
    mov dword ptr [handled],1
    exercise filter,0
    exercise noise,1
    exercise outline,2
    cmp dword ptr [stock_calls],3
    jne fail
    cmp dword ptr [gate_calls],6
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_effect_draw
rev_genesis_effect_draw:
    mov eax,[esp+4]
    cmp eax,[expected_kind]
    jne fail
    cmp dword ptr [esp+8],0xABCD2000
    jne fail
    cmp dword ptr [esp+12],0xABCD1000
    jne fail
    inc dword ptr [gate_calls]
    xor ecx,ecx
    xor edx,edx
    mov eax,[handled]
    ret
.macro standard_resume name, bytes
.global rev_genesis_\name\()_draw_resume
rev_genesis_\name\()_draw_resume:
    cmp ecx,0xABCD2000
    jne fail
    cmp dword ptr [ebp+8],0xABCD1000
    jne fail
    mov eax,ebp
    sub eax,esp
    cmp eax,\bytes
    jne fail
    inc dword ptr [stock_calls]
    mov esp,ebp
    pop ebp
    ret 4
.endm
standard_resume filter,0xE4
standard_resume noise,0xCC
.global rev_genesis_outline_draw_resume
rev_genesis_outline_draw_resume:
    cmp ecx,0xABCD2000
    jne fail
    cmp dword ptr [ebx+8],0xABCD1000
    jne fail
    mov eax,ebx
    sub eax,esp
    cmp eax,8
    jne fail
    inc dword ptr [stock_calls]
    # Same remaining aligned-frame setup and unwind as the native body.
    and esp,0xFFFFFFF0
    add esp,4
    push ebp
    mov ebp,[ebx+4]
    mov [esp+4],ebp
    mov ebp,esp
    mov esp,ebp
    pop ebp
    mov esp,ebx
    pop ebx
    ret 4
.bss
.align 4
before:.skip 4
handled:.skip 4
expected_kind:.skip 4
gate_calls:.skip 4
stock_calls:.skip 4
.section .note.GNU-stack,"",@progbits
