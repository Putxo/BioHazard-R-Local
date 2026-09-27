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
    mov ecx,0x100000
    push 0xDEAD0001
    call rev_genesis_effect_toggle_gate
    cmp esp,[before]
    jne fail
    cmp dword ptr [stock],1
    jne fail
    mov dword ptr [handled],1
    mov ecx,0x100000
    push 0xDEAD0000
    call rev_genesis_effect_toggle_gate
    cmp esp,[before]
    jne fail
    cmp dword ptr [stock],1
    jne fail
    cmp dword ptr [calls],2
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_effect_toggle
rev_genesis_effect_toggle:
    cmp dword ptr [esp+4],0x100000
    jne fail
    mov eax,[handled]
    xor eax,1
    cmp [esp+8],eax
    jne fail
    inc dword ptr [calls]
    xor ecx,ecx
    xor edx,edx
    mov eax,[handled]
    ret
.global rev_genesis_effect_toggle_resume
rev_genesis_effect_toggle_resume:
    cmp ecx,0x100000
    jne fail
    lea eax,[ebp-0xCC]
    cmp esp,eax
    jne fail
    cmp dword ptr [ebp+8],0xDEAD0001
    jne fail
    inc dword ptr [stock]
    mov esp,ebp
    pop ebp
    ret 4
.bss
.align 4
before:.skip 4
handled:.skip 4
calls:.skip 4
stock:.skip 4
.section .note.GNU-stack,"",@progbits
