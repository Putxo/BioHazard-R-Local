.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebx,0x11223344
    mov esi,0x22334455
    mov edi,0x33445566
    mov ebp,0x44556677
    mov dword ptr [decision],1
again:
    mov ecx,0x12345000
    push 0x98765432
    call rev_script_input_gate
    cmp eax,[decision]
    jne fail
    cmp esp,[before]
    jne fail
    cmp ebx,0x11223344
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    cmp ebp,0x44556677
    jne fail
    dec dword ptr [decision]
    jns again
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_script_input_member
rev_script_input_member:
    cmp dword ptr [esp+4],0x12345000
    jne fail
    cmp dword ptr [esp+12],0x98765432
    jne fail
    mov eax,[decision]
    xor ecx,ecx
    xor edx,edx
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
decision: .skip 4
.section .note.GNU-stack,"",@progbits
