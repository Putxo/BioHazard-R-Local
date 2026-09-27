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
    mov dword ptr [operation],0
    mov dword ptr [argument],0
    mov ecx,0xabcdef00
    call rev_genesis_read
    call check
    mov dword ptr [operation],1
    mov dword ptr [argument],0xfedc1234
    push dword ptr [argument]
    mov ecx,0xabcdef00
    call rev_genesis_set
    call check
    mov dword ptr [operation],2
    mov dword ptr [argument],0xfedc5678
    push dword ptr [argument]
    mov ecx,0xabcdef00
    call rev_genesis_add
    call check
    inc dword ptr [mode]
    cmp dword ptr [mode],3
    jne loop
    cmp dword ptr [stock_calls],3
    jne fail
    cmp dword ptr [router_calls],9
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
check:
    cmp eax,73
    jne fail
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
.global rev_genesis_progress
rev_genesis_progress:
    inc dword ptr [router_calls]
    cmp dword ptr [esp+4],0x12345000
    jne fail
    mov eax,[operation]
    cmp [esp+8],eax
    jne fail
    mov eax,[argument]
    cmp [esp+12],eax
    jne fail
    mov eax,[esp+16]
    mov dword ptr [eax],73
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_stock_read,rev_genesis_stock_set,rev_genesis_stock_add
rev_genesis_stock_read:
    cmp ecx,0xabcdef00
    jne fail
    cmp dword ptr [operation],0
    jne fail
    inc dword ptr [stock_calls]
    mov eax,73
    ret
rev_genesis_stock_set:
    cmp dword ptr [operation],1
    jne fail
    jmp stock_write
rev_genesis_stock_add:
    cmp dword ptr [operation],2
    jne fail
stock_write:
    cmp ecx,0xabcdef00
    jne fail
    mov eax,[argument]
    cmp [esp+4],eax
    jne fail
    inc dword ptr [stock_calls]
    mov eax,73
    ret 4
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before: .skip 4
mode: .skip 4
operation: .skip 4
argument: .skip 4
router_calls: .skip 4
stock_calls: .skip 4
.section .note.GNU-stack,"",@progbits
