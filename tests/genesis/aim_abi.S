.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebp,0x66778899
    mov dword ptr [argument],0x11223301
    call exercise
    mov dword ptr [argument],0x11223300
    call exercise
    mov dword ptr [argument],0x112233FF
    call exercise
    cmp dword ptr [calls],3
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
exercise:
    push 0xDEADBEEF
    push dword ptr [argument]
    call native_frame
    lea eax,[esp+4]
    cmp eax,[before]
    jne fail
    cmp ebp,0x66778899
    jne fail
    ret
native_frame:
    push ebp
    mov ebp,esp
    sub esp,0xE8
    push ebx
    push esi
    push edi
    mov dword ptr [ebp-8],0xABCDEF00
    mov eax,0x12345678
    mov ecx,0x23456789
    mov edx,0x34567890
    mov ebx,0x45678901
    mov esi,0x56789012
    mov edi,0x67890123
    clc
    jmp rev_aim_visibility_gate
.global rev_aim_visibility
rev_aim_visibility:
    cmp dword ptr [esp+4],0xABCDEF00
    jne fail
    movzx eax,byte ptr [argument]
    cmp [esp+8],eax
    jne fail
    cmp dword ptr [esp+12],0xEF
    jne fail
    inc dword ptr [calls]
    xor eax,eax
    xor ecx,ecx
    xor edx,edx
    xor ebx,ebx
    xor esi,esi
    xor edi,edi
    xor ebp,ebp
    stc
    ret
.global rev_aim_visibility_resume
rev_aim_visibility_resume:
    jc fail
    cmp eax,0x12345678
    jne fail
    cmp ecx,0x23456789
    jne fail
    cmp edx,0x34567890
    jne fail
    cmp ebx,0x45678901
    jne fail
    cmp esi,0x56789012
    jne fail
    cmp edi,0x67890123
    jne fail
    cmp dword ptr [ebp-8],0xABCDEF00
    jne fail
    mov eax,[argument]
    cmp [ebp+8],eax
    jne fail
    cmp dword ptr [ebp+12],0xDEADBEEF
    jne fail
    lea eax,[esp+0xF4]
    cmp eax,ebp
    jne fail
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret 8
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.data
before:.long 0
argument:.long 0
calls:.long 0
.section .note.GNU-stack,"",@progbits
