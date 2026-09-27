.intel_syntax noprefix
.code32
.text
.global _start
_start:
    sub esp,64
    lea ebp,[esp+32]
    mov [before],esp
    mov dword ptr [ebp-8],0x12345000
    mov ebx,0x11223344
    mov esi,0x22334455
    mov edi,0x33445566
    mov dword ptr [mode],0
loop:
    mov ecx,0xabcdef00
    call rev_genesis_camera_gate
    cmp dword ptr [mode],2
    je hidden
    cmp eax,0x55667788
    jne fail
    jmp preserved
hidden:
    test eax,eax
    jnz fail
preserved:
    cmp esp,[before]
    jne fail
    lea eax,[esp+32]
    cmp ebp,eax
    jne fail
    cmp ebx,0x11223344
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    inc dword ptr [mode]
    cmp dword ptr [mode],3
    jne loop
    cmp dword ptr [stock_calls],1
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_genesis_camera
rev_genesis_camera:
    cmp dword ptr [esp+4],0x12345000
    jne fail
    cmp dword ptr [esp+8],0xabcdef00
    jne fail
    mov edx,[esp+12]
    mov dword ptr [edx],0x55667788
    cmp dword ptr [mode],2
    jne value_ready
    mov dword ptr [edx],0
value_ready:
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_stock_camera
rev_genesis_stock_camera:
    cmp ecx,0xabcdef00
    jne fail
    inc dword ptr [stock_calls]
    mov eax,0x55667788
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
mode: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
