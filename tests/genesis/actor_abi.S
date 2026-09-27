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
    mov dword ptr [finder],0
    push 0x76543210
    mov ecx,0xabcdef00
    call rev_hud_scanner_self
    call check
    mov dword ptr [finder],1
    push 0x76543210
    mov ecx,0xabcdef00
    call rev_genesis_selected_actor
    call check
    inc dword ptr [mode]
    cmp dword ptr [mode],3
    jne loop
    cmp dword ptr [stock_calls],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
check:
    cmp dword ptr [mode],2
    je hidden
    cmp eax,0x55667788
    jne fail
    jmp preserved
hidden:
    test eax,eax
    jnz fail
preserved:
    lea eax,[esp+4]
    cmp eax,[before]
    jne fail
    lea eax,[esp+36]
    cmp ebp,eax
    jne fail
    cmp ebx,0x11223344
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    ret
.global rev_hud_resolve_actor
rev_hud_resolve_actor:
    cmp dword ptr [esp+4],0x12345000
    jne fail
    cmp dword ptr [esp+8],6
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
.global rev_hud_stock_self,rev_genesis_stock_selected_actor
rev_hud_stock_self:
    cmp dword ptr [finder],0
    jne fail
    jmp stock
rev_genesis_stock_selected_actor:
    cmp dword ptr [finder],1
    jne fail
stock:
    cmp ecx,0xabcdef00
    jne fail
    cmp dword ptr [esp+4],0x76543210
    jne fail
    inc dword ptr [stock_calls]
    mov eax,0x55667788
    ret 4
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
mode: .skip 4
finder: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
