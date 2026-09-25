# Freestanding Linux i386 harness. Executes our bridges and registry only.
# rev_hud_stock_self is a mock. No original engine bytes are linked.
.intel_syntax noprefix
.code32
.text
.global _start
_start:
    call test_main
    mov ebx,eax
    mov eax,1
    int 0x80

# cdecl invoke_bridge(widget, bridge, result[10]); emulates caller EBP-8.
.global invoke_bridge
invoke_bridge:
    push ebp
    mov ebp,esp
    sub esp,20
    push ebx
    push esi
    push edi
    mov eax,[ebp+16]
    mov [ebp-4],eax
    mov eax,[ebp+8]
    mov [ebp-8],eax
    mov [ebp-12],esp
    mov [ebp-16],ebp
    mov dword ptr [ebp-20],0x1234abcd
    mov eax,[ebp+12]
    mov ebx,0x11223344
    mov esi,0x55667788
    mov edi,0x7abc1234
    mov ecx,0x41414141
    push 0x12344321
    call eax
    mov edx,[ebp-4]
    mov [edx],eax
    mov [edx+4],ecx
    mov [edx+8],ebx
    mov [edx+12],esi
    mov [edx+16],edi
    mov [edx+20],ebp
    mov eax,[ebp-16]
    mov [edx+24],eax
    mov [edx+28],esp
    mov eax,[ebp-12]
    mov [edx+32],eax
    mov eax,[ebp-20]
    mov [edx+36],eax
    mov esp,[ebp-12]
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret

.global rev_hud_stock_self
rev_hud_stock_self:
    inc dword ptr [stock_calls]
    cmp ecx,0x41414141
    jne .bad
    cmp dword ptr [esp+4],0x12344321
    jne .bad
    mov eax,0x11110000
    ret 4
.bad:
    inc dword ptr [mock_errors]
    xor eax,eax
    ret 4
.section .note.GNU-stack,"",@progbits
