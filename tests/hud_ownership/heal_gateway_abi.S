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
    mov dword ptr [decision],1
    mov ecx,0xABCDEF00
    call rev_hud_heal_gate
    cmp dword ptr [stock_calls],1
    jne fail
    mov dword ptr [decision],0
    mov ecx,0xABCDEF00
    call rev_hud_heal_gate
    cmp dword ptr [stock_calls],1
    jne fail
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
    xor ebx,ebx
    mov eax,1
    int 0x80
.global rev_hud_heal_request
rev_hud_heal_request:
    cmp dword ptr [esp+4],0x12345000
    jne fail
    mov eax,[decision]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_hud_stock_heal
rev_hud_stock_heal:
    cmp ecx,0xABCDEF00
    jne fail
    inc dword ptr [stock_calls]
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
decision: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
