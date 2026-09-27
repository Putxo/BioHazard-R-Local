.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebp,esp
    sub esp,0x340
    mov [before],esp
    mov dword ptr [ebp-0x300],0xEC0000
    mov dword ptr [ebp-0x1B8],0xEF0000
    mov ebx,11
    mov esi,22
    mov edi,33
again:
    mov ecx,0xED0000
    call rev_genesis_notify_gate
    cmp esp,[before]
    jne fail
    lea eax,[esp+0x340]
    cmp eax,ebp
    jne fail
    cmp ebx,11
    jne fail
    cmp esi,22
    jne fail
    cmp edi,33
    jne fail
    inc dword ptr [mode]
    cmp dword ptr [mode],2
    jne again
    cmp dword ptr [stock_calls],1
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_genesis_notify
rev_genesis_notify:
    cmp dword ptr [esp+4],0xEF0000
    jne fail
    cmp dword ptr [esp+8],0xEC0000
    jne fail
    cmp dword ptr [esp+12],0xED0000
    jne fail
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_notify_stock
rev_genesis_notify_stock:
    cmp ecx,0xED0000
    jne fail
    inc dword ptr [stock_calls]
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before:.skip 4
mode:.skip 4
stock_calls:.skip 4
.section .note.GNU-stack,"",@progbits
