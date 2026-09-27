.intel_syntax noprefix
.code32
.text
.global _start
_start:
    sub esp,0x100
    lea ebp,[esp+0x80]
    mov [before],esp
    mov dword ptr [ebp+8],0xABCD1000
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    call rev_runtime_draw_schedule_gate
    cmp eax,1
    jne fail
    cmp dword ptr [stock_calls],1
    jne fail
    cmp esp,[before]
    jne fail
    lea eax,[esp+0x80]
    cmp ebp,eax
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
# Synthetic original callee and dispatcher; neither invokes the game.
.global rev_runtime_stock_immediate
rev_runtime_stock_immediate:
    inc dword ptr [stock_calls]
    mov eax,0xABCD0000
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_runtime_draw_schedule
rev_runtime_draw_schedule:
    cmp dword ptr [esp+4],0xABCD1000
    jne fail
    cmp dword ptr [esp+8],0xABCD0000
    jne fail
    mov eax,1
    ret
.bss
.align 4
before: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
