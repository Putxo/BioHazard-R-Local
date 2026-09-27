.intel_syntax noprefix
.code32
.text
# Only the call at 034F1EEC. EBP-4 is the active model and EBX+8 its context.
# The stock resource predicate is evaluated first; no resource flags change.
.global rev_genesis_hunter_cache_gate
rev_genesis_hunter_cache_gate:
    call rev_genesis_hunter_cache_stock
    pushfd
    pushad
    movzx eax,al
    push eax
    push dword ptr [ebx+8]
    push dword ptr [ebp-4]
    call rev_runtime_hunter_uncached
    add esp,12
    mov dword ptr [esp+28],eax
    popad
    popfd
    ret
.section .note.GNU-stack,"",@progbits
