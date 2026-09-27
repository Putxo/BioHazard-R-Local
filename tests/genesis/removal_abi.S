.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebx,11
    mov esi,22
    mov edi,33
    mov ebp,44
    mov ecx,0x12345678
    call rev_genesis_remove_target_gate
    cmp eax,0xABCDEFFF
    jne fail
    cmp esp,[before]
    jne fail
    cmp ebx,11
    jne fail
    cmp esi,22
    jne fail
    cmp edi,33
    jne fail
    cmp ebp,44
    jne fail
    cmp dword ptr [calls],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_genesis_remove_target
rev_genesis_remove_target:
    cmp dword ptr [esp+4],0x12345678
    jne fail
    cmp dword ptr [calls],0
    jne fail
    inc dword ptr [calls]
    xor ecx,ecx
    xor edx,edx
    xor eax,eax
    ret
.global rev_genesis_target_cleanup_stock
rev_genesis_target_cleanup_stock:
    cmp ecx,0x12345678
    jne fail
    cmp dword ptr [calls],1
    jne fail
    inc dword ptr [calls]
    mov eax,0xABCDEFFF
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before:.skip 4
calls:.skip 4
.section .note.GNU-stack,"",@progbits
