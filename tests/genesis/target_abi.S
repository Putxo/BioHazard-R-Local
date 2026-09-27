.intel_syntax noprefix
.code32
.text
.global _start
_start:
    sub esp,64
    lea ebp,[esp+32]
    lea eax,[target]
    mov [ebp-8],eax
    mov [before],esp
    mov ebx,0x11223344
    mov esi,0x22334455
    mov edi,0x33445566
    mov ecx,0x44556677
    mov edx,0x55667788
    # Constructor boundary has saved EDI and ESI still on its stack.
    push 0xaabbccdd
    push 0x99aabbcc
    stc
    jmp rev_genesis_target_born_gate
.global rev_genesis_target_born_resume
rev_genesis_target_born_resume:
    jnc fail
    cmp eax,offset target
    jne fail
    cmp edi,0x99aabbcc
    jne fail
    cmp esi,0xaabbccdd
    jne fail
    cmp esp,[before]
    jne fail
    call preserved
    mov esi,0x22334455
    mov edi,0x33445566
    stc
    jmp rev_genesis_target_dying_gate
.global rev_genesis_target_dying_resume
rev_genesis_target_dying_resume:
    jnc fail
    cmp eax,offset target
    jne fail
    cmp dword ptr [target],0x04da8570
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    cmp esp,[before]
    jne fail
    call preserved
    cmp dword ptr [events],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
preserved:
    cmp ebx,0x11223344
    jne fail
    cmp ecx,0x44556677
    jne fail
    cmp edx,0x55667788
    jne fail
    lea eax,[esp+36]
    cmp ebp,eax
    jne fail
    ret
.global rev_genesis_target_event
rev_genesis_target_event:
    cmp dword ptr [esp+8],offset target
    jne fail
    cmp dword ptr [events],0
    jne death
    cmp dword ptr [esp+4],0x0281ae26
    jne fail
    jmp clobber
death:
    cmp dword ptr [esp+4],0x02823a53
    jne fail
    cmp dword ptr [target],0
    jne fail
clobber:
    inc dword ptr [events]
    xor eax,eax
    xor ecx,ecx
    xor edx,edx
    clc
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before:.skip 4
target:.skip 4
events:.skip 4
.section .note.GNU-stack,"",@progbits
