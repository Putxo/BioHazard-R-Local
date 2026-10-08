.intel_syntax noprefix
.code32
.text
.global _start
_start:
    push ebp
    mov ebp,esp
    sub esp,16
    mov dword ptr [ebp-8],0x12345000
    mov dword ptr [decision],0
next:
    fninit
    fld1
    movdqu xmm0,[seed]
    fxsave [before]
    mov [saved_sp],esp
    mov dword ptr [calls],0
    mov dword ptr [stock_calls],0
    push 0x2c
    push 0
    lea eax,[buffer]
    push eax
    mov eax,0x11112222
    mov ecx,0x33334444
    mov edx,0x55556666
    mov ebx,0x77778888
    mov esi,0x9999aaaa
    mov edi,0xbbbbcccc
    std
    stc
    call rev_pad_clear_gate
    pushfd
    pop edx
    cld
    and edx,0x401
    cmp edx,0x401
    jne failed
    cmp eax,offset buffer
    jne failed
    cmp ecx,0x33334444
    jne failed
    cmp ebx,0x77778888
    jne failed
    cmp esi,0x9999aaaa
    jne failed
    cmp edi,0xbbbbcccc
    jne failed
    lea eax,[esp+12]
    cmp eax,[saved_sp]
    jne failed
    add esp,12
    cmp dword ptr [calls],1
    jne failed
    mov eax,1
    sub eax,[decision]
    cmp [stock_calls],eax
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
    inc dword ptr [decision]
    cmp dword ptr [decision],2
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

.global rev_preserve_other_pad
rev_preserve_other_pad:
    cmp dword ptr [esp+4],0x12345000
    jne failed
    cmp dword ptr [esp+8],offset buffer
    jne failed
    cmp dword ptr [esp+12],0
    jne failed
    cmp dword ptr [esp+16],0x2c
    jne failed
    mov eax,esp
    and eax,15
    cmp eax,12
    jne failed
    pushfd
    pop eax
    test eax,0x400
    jnz failed
    inc dword ptr [calls]
    fninit
    pxor xmm0,xmm0
    mov ecx,0xdeadbeef
    mov edx,0xdeadbeef
    mov eax,[decision]
    ret

.global rev_pad_stock_memset
rev_pad_stock_memset:
    pushfd
    push eax
    push edx
    mov eax,[esp+8]
    and eax,0x401
    cmp eax,0x401
    jne failed
    cmp dword ptr [esp+4],0x11112222
    jne failed
    cmp ecx,0x33334444
    jne failed
    cmp dword ptr [esp],0x55556666
    jne failed
    cmp dword ptr [esp+16],offset buffer
    jne failed
    cmp dword ptr [esp+20],0
    jne failed
    cmp dword ptr [esp+24],0x2c
    jne failed
    inc dword ptr [stock_calls]
    pop edx
    pop eax
    popfd
    mov eax,[esp+4]
    ret
.section .rodata
.balign 16
seed: .long 1,2,3,4
message: .ascii "PASS: raw pad clear gateway, stock/preserve, cdecl stack and FP/registers\n"
.set message_size,.-message
.bss
.balign 16
before: .skip 512
after: .skip 512
buffer: .skip 44
saved_sp: .skip 4
decision: .skip 4
calls: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
