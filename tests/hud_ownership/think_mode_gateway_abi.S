.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov dword ptr [mode],0
next:
    mov [saved_sp],esp
    fninit
    fld1
    movdqu xmm0,[seed]
    fxsave [before]
    push dword ptr [mode]
    mov eax,0x12345678
    mov ecx,0x56780000
    mov edx,0x98765432
    mov ebx,0x11223344
    mov esi,0x55667788
    mov edi,0x99aabbcc
    mov ebp,0x10203040
    std
    call rev_local_think_mode_gate
    cld
    cmp esp,[saved_sp]
    jne failed
    cmp ebp,0x10203040
    jne failed
    cmp eax,0xcafebabe
    jne failed
    cmp dword ptr [callback_count],1
    jne failed
    mov dword ptr [callback_count],0
    inc dword ptr [mode]
    cmp dword ptr [mode],5
    jb next
    mov eax,4
    mov ebx,1
    mov ecx,offset message
    mov edx,offset message_size
    int 0x80
    xor ebx,ebx
    jmp exit
failed:
    mov ebx,1
exit:
    mov eax,1
    int 0x80

.global rev_local_think_mode
rev_local_think_mode:
    cmp dword ptr [esp+4],0x56780000
    jne failed
    mov eax,[mode]
    cmp [esp+8],eax
    jne failed
    mov ecx,esp
    and ecx,15
    cmp ecx,12
    jne failed
    pushfd
    pop ecx
    test ecx,0x400
    jnz failed
    inc dword ptr [callback_count]
    cmp eax,3
    jne selected
    mov eax,1
selected:
    fninit
    pxor xmm0,xmm0
    mov ecx,0xdead0001
    mov edx,0xdead0002
    clc
    ret

.global rev_local_think_mode_continue
rev_local_think_mode_continue:
    # Synthetic replacement for the native body, after its displaced prologue.
    cmp eax,0x12345678
    jne failed
    cmp ecx,0x56780000
    jne failed
    cmp edx,0x98765432
    jne failed
    cmp ebx,0x11223344
    jne failed
    cmp esi,0x55667788
    jne failed
    cmp edi,0x99aabbcc
    jne failed
    lea eax,[ebp-0xcc]
    cmp eax,esp
    jne failed
    pushfd
    pop eax
    test eax,0x400
    jz failed
    cld
    mov eax,[mode]
    cmp eax,3
    jne check_mode
    mov eax,1
check_mode:
    cmp [ebp+8],eax
    jne failed
    fxsave [after]
    mov eax,[before]
    cmp eax,[after]
    jne failed
    mov al,[before+4]
    cmp al,[after+4]
    jne failed
    mov eax,[before+24]
    cmp eax,[after+24]
    jne failed
    mov esi,offset before+32
    mov edi,offset after+32
    mov ecx,10
    repe cmpsb
    jne failed
    mov esi,offset before+160
    mov edi,offset after+160
    mov ecx,16
    repe cmpsb
    jne failed
    mov eax,0xcafebabe
    mov esp,ebp
    pop ebp
    ret 4
.section .rodata
.balign 16
seed: .long 0x12345678,0xabcdef01,0x76543210,0x87654321
message: .ascii "PASS: ThinkMode gateway, 5 synthetic requests, stack/register/FP preservation\n"
.set message_size,.-message
.bss
.balign 16
before: .skip 512
after: .skip 512
mode: .skip 4
saved_sp: .skip 4
callback_count: .skip 4
.section .note.GNU-stack,"",@progbits
