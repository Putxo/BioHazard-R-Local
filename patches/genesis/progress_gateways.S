.intel_syntax noprefix
.code32
.text
// Verified scanner methods keep their widget this at [caller EBP-8].
// The stock manager in ECX and original stack argument are preserved on Stock.
.macro PROGRESS_GATE name, operation, stock, argument_bytes
.global \name
.type \name,@function
\name:
    push ecx
    sub esp,4
    lea edx,[esp]
.if \argument_bytes
    mov eax,[esp+12]
.else
    xor eax,eax
.endif
    push edx
    push eax
    push \operation
    push dword ptr [ebp-8]
    call rev_genesis_progress
    add esp,16
    pop edx
    pop ecx
    test eax,eax
    jz 1f
    mov eax,edx
    ret \argument_bytes
1:
    jmp \stock
.size \name,.-\name
.endm
PROGRESS_GATE rev_genesis_read,0,rev_genesis_stock_read,0
PROGRESS_GATE rev_genesis_set,1,rev_genesis_stock_set,4
PROGRESS_GATE rev_genesis_add,2,rev_genesis_stock_add,4
.section .note.GNU-stack,"",@progbits
