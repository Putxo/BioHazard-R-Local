.intel_syntax noprefix
.code32
.text
.global rev_genesis_remove_target_gate
rev_genesis_remove_target_gate:
    push ecx
    push ecx
    call rev_genesis_remove_target
    add esp,4
    pop ecx
    # The caller reaches this unconditionally after clearing target+8 and
    # trying the original scanner. Base cleanup still runs exactly once.
    jmp rev_genesis_target_cleanup_stock
.section .note.GNU-stack,"",@progbits
