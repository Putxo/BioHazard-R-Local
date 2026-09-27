.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    xor ebp,ebp
    call exercise
    mov dword ptr [mode],1
    call exercise
    mov dword ptr [mode],2
    call exercise
    cmp dword ptr [loads],3
    jne fail
    cmp dword ptr [copies],3
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
exercise:
    push ebp
    mov ebp,esp
    sub esp,16
    mov dword ptr [ebp-8],0xABCDEF00
    mov ebx,0x11223344
    mov esi,0x22334455
    mov edi,0x33445566
    push 1
    push 0x04DE4490
    push 0x0558CE40
    mov ecx,0x55667788
    lea edx,[vtable]
    call rev_genesis_filter_load_gate
    lea ecx,[esp+16]
    cmp ecx,ebp
    jne fail
    cmp dword ptr [mode],0
    jne changed
    cmp eax,0x77889900
    jne fail
    jmp registers
changed:
    cmp dword ptr [mode],1
    jne rejected
    cmp eax,0x8899AA00
    jne fail
    jmp registers
rejected:
    test eax,eax
    jne fail
registers:
    cmp ebx,0x11223344
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    cmp dword ptr [ebp-8],0xABCDEF00
    jne fail
    mov esp,ebp
    pop ebp
    lea ecx,[esp+4]
    cmp ecx,[before]
    jne fail
    ret
load:
    cmp ecx,0x55667788
    jne fail
    cmp dword ptr [esp+4],0x0558CE40
    jne fail
    cmp dword ptr [esp+8],0x04DE4490
    jne fail
    cmp dword ptr [esp+12],1
    jne fail
    inc dword ptr [loads]
    mov eax,0x77889900
    xor ecx,ecx
    xor edx,edx
    ret 12
.global rev_genesis_filter_loaded
rev_genesis_filter_loaded:
    cmp dword ptr [esp+4],0xABCDEF00
    jne fail
    cmp dword ptr [esp+8],0x77889900
    jne fail
    inc dword ptr [copies]
    mov eax,0x77889900
    cmp dword ptr [mode],0
    je done
    mov eax,0x8899AA00
    cmp dword ptr [mode],1
    je done
    xor eax,eax
done:
    xor ecx,ecx
    xor edx,edx
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.data
before:.long 0
mode:.long 0
loads:.long 0
copies:.long 0
vtable:.zero 48
.long load
.section .note.GNU-stack,"",@progbits
