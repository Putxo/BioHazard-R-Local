.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebx,offset context_slot
    mov ebp,offset model_slot+4
    mov esi,0x23456789
    mov edi,0x3456789A
    mov [before],esp
    call invoke
    test eax,eax
    jnz fail
    mov dword ptr [allow],1
    call invoke
    cmp eax,1
    jne fail
    mov dword ptr [allow],0
    mov dword ptr [raw],0xDEAD0001
    call invoke
    cmp eax,1
    jne fail
    cmp dword ptr [calls],3
    jne fail
    cmp dword ptr [stock_calls],3
    jne fail
    cmp esp,[before]
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
invoke:
    mov ecx,0xF10000
    call rev_genesis_hunter_cache_gate
    pushfd
    pop dword ptr [saved_flags]
    test dword ptr [saved_flags],1
    jz fail
    cmp ebx,offset context_slot
    jne fail
    cmp ebp,offset model_slot+4
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ecx,0xABCDEF01
    jne fail
    cmp edx,0xABCDEF02
    jne fail
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_hunter_cache_stock
rev_genesis_hunter_cache_stock:
    cmp ecx,0xF10000
    jne fail
    inc dword ptr [stock_calls]
    mov ecx,0xABCDEF01
    mov edx,0xABCDEF02
    mov eax,[raw]
    stc
    ret
.global rev_runtime_hunter_uncached
rev_runtime_hunter_uncached:
    cmp dword ptr [esp+4],0xF20000
    jne fail
    cmp dword ptr [esp+8],0xF30000
    jne fail
    mov eax,[raw]
    and eax,255
    cmp [esp+12],eax
    jne fail
    inc dword ptr [calls]
    xor ecx,ecx
    xor edx,edx
    or eax,[allow]
    clc
    ret
.data
raw:.long 0xDEAD0000
model_slot:.long 0xF20000
context_slot:.long 0,0,0xF30000
.bss
.align 4
before:.skip 4
allow:.skip 4
calls:.skip 4
stock_calls:.skip 4
saved_flags:.skip 4
.section .note.GNU-stack,"",@progbits

