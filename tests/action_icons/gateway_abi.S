.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    mov [before],esp
    push 0xABCD1000
    mov ecx,0xABCD2000
    call rev_action_draw_gate
    cmp esp,[before]
    jne fail
    mov ecx,0xABCD5000
    call rev_action_mask_gate
    cmp eax,2
    jne fail
    cmp esp,[before]
    jne fail
    cmp dword ptr [seen_icon],0xABCD2000
    jne fail
    cmp dword ptr [seen_context],0xABCD1000
    jne fail
    push 0xABCD3000
    mov ecx,0xABCD4000
    call rev_action_stock_draw
    cmp esp,[before]
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
# Mock C dispatch. Deliberately clobber caller-saved registers.
.global rev_action_draw
rev_action_draw:
    mov eax,[esp+4]
    mov [seen_icon],eax
    mov eax,[esp+8]
    mov [seen_context],eax
    xor ecx,ecx
    xor edx,edx
    ret
# Synthetic continuation only; no original game code is loaded by this test.
.global rev_action_stock_mask
rev_action_stock_mask:
    cmp ecx,0xABCD5000
    jne fail
    xor ecx,ecx
    mov eax,1
    ret
.global rev_runtime_unit_mask
rev_runtime_unit_mask:
    cmp dword ptr [esp+4],0xABCD5000
    jne fail
    cmp dword ptr [esp+8],1
    jne fail
    mov eax,2
    ret
.global rev_action_draw_continue
rev_action_draw_continue:
    cmp ecx,0xABCD4000
    jne fail
    cmp dword ptr [ebp+8],0xABCD3000
    jne fail
    mov eax,ebp
    sub eax,esp
    cmp eax,0xFC
    jne fail
    mov esp,ebp
    pop ebp
    ret 4
.bss
.align 4
before: .skip 4
seen_icon: .skip 4
seen_context: .skip 4
.section .note.GNU-stack,"",@progbits
