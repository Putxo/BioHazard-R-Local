.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebp,0x11223344
    call phase
    inc dword ptr [mode]
    call phase
    inc dword ptr [mode]
    call phase
    mov dword ptr [mode],1
    mov dword ptr [actor],0
    call phase
    cmp esp,[before]
    jne fail
    cmp ebp,0x11223344
    jne fail
    cmp dword ptr [stock_calls],1
    jne fail
    cmp dword ptr [continued],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
phase:
    push ebp
    mov ebp,esp
    sub esp,0x120
    push ebx
    push esi
    push edi
    mov dword ptr [ebp-8],0xABCDEF00
    mov byte ptr [ebp-0x119],0
    mov ebx,0x22334455
    mov esi,0x33445566
    mov edi,0x44556677
    lea eax,[ebp-0x119]
    push eax
    mov ecx,0x55667788
    call rev_scope_actor_gate
    inc dword ptr [continued]
    cmp dword ptr [mode],0
    jne 1f
    cmp eax,0xA00000
    jne fail
    jmp rev_scope_phase8_end
1:
    cmp eax,0xB00000
    jne fail
.global rev_scope_phase8_end
rev_scope_phase8_end:
    cmp dword ptr [ebp-8],0xABCDEF00
    jne fail
    lea eax,[esp+0x12C]
    cmp eax,ebp
    jne fail
    cmp ebx,0x22334455
    jne fail
    cmp esi,0x33445566
    jne fail
    cmp edi,0x44556677
    jne fail
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret
.global rev_scope_actor
rev_scope_actor:
    cmp dword ptr [esp+4],0xABCDEF00
    jne fail
    mov ecx,[esp+8]
    mov edx,[actor]
    mov [ecx],edx
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_scope_actor_stock
rev_scope_actor_stock:
    cmp ecx,0x55667788
    jne fail
    lea eax,[ebp-0x119]
    cmp [esp+4],eax
    jne fail
    inc dword ptr [stock_calls]
    mov eax,0xA00000
    ret 4
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.data
before:.long 0
mode:.long 0
actor:.long 0xB00000
stock_calls:.long 0
continued:.long 0
.section .note.GNU-stack,"",@progbits
