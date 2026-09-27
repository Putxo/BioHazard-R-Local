.intel_syntax noprefix
.code32
.text
.global _start
.macro CHECK name, site, vt, dying
    mov dword ptr [expected_site],\site
    mov dword ptr [target],0x12345678
    .if \dying
    mov dword ptr [expected_during],0x12345678
    .else
    mov dword ptr [expected_during],\vt
    .endif
    lea eax,[target]
    mov ebx,0x11223344
    mov ecx,0x22334455
    mov edx,0x33445566
    mov esi,0x44556677
    mov edi,0x55667788
    mov ebp,0x66778899
    mov [before],esp
    stc
    jmp rev_genesis_effect_\name\()_gate
.global rev_genesis_effect_\name\()_resume
rev_genesis_effect_\name\()_resume:
    jnc fail
    cmp esp,[before]
    jne fail
    cmp eax,offset target
    jne fail
    cmp dword ptr [target],\vt
    jne fail
    cmp ebx,0x11223344
    jne fail
    cmp ecx,0x22334455
    jne fail
    cmp edx,0x33445566
    jne fail
    cmp esi,0x44556677
    jne fail
    cmp edi,0x55667788
    jne fail
    cmp ebp,0x66778899
    jne fail
.endm
_start:
    CHECK filter_born, 0x0293344E, 0x04DB80CC, 0
    CHECK filter_dying, 0x02933576, 0x04DB80CC, 1
    CHECK noise_born, 0x0293559E, 0x04DB833C, 0
    CHECK noise_dying, 0x02935816, 0x04DB833C, 1
    CHECK outline_born, 0x037DD309, 0x04F9A7F4, 0
    CHECK outline_dying, 0x037DD4A2, 0x04F9A7F4, 1
    cmp dword ptr [events],6
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_genesis_effect_event
rev_genesis_effect_event:
    cmp dword ptr [esp+8],offset target
    jne fail
    mov eax,[expected_site]
    cmp eax,[esp+4]
    jne fail
    mov eax,[expected_during]
    cmp eax,[target]
    jne fail
    inc dword ptr [events]
    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx
    xor esi,esi
    xor edi,edi
    xor ebp,ebp
    clc
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
target:.skip 4
before:.skip 4
events:.skip 4
expected_site:.skip 4
expected_during:.skip 4
.section .note.GNU-stack,"",@progbits
