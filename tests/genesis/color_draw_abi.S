.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    mov [before],esp
    mov ecx,0xF806A0
    call rev_genesis_color_draw_enabled_gate
    test eax,eax
    jnz fail
    mov dword ptr [allow],1
    mov ecx,0xF806A0
    call rev_genesis_color_draw_enabled_gate
    cmp eax,1
    jne fail
    mov dword ptr [raw],0xDEAD0001
    mov ecx,0xF806A0
    call rev_genesis_color_draw_enabled_gate
    cmp eax,1
    jne fail
    cmp ecx,0xF806A0
    jne fail
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
    cmp dword ptr [calls],3
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_color_draw_enabled_stock
rev_genesis_color_draw_enabled_stock:
    cmp ecx,0xF806A0
    jne fail
    xor ecx,ecx
    xor edx,edx
    mov eax,[raw]
    ret
.global rev_runtime_unit_draw_enabled
rev_runtime_unit_draw_enabled:
    cmp dword ptr [esp+4],0xF806A0
    jne fail
    mov eax,[raw]
    and eax,255
    cmp [esp+8],eax
    jne fail
    inc dword ptr [calls]
    xor ecx,ecx
    xor edx,edx
    or eax,[allow]
    ret
.data
raw:.long 0xDEAD0000
.bss
.align 4
before:.skip 4
allow:.skip 4
calls:.skip 4
.section .note.GNU-stack,"",@progbits
