.intel_syntax noprefix
.code32
.text
.global rev_runtime_entry
.type rev_runtime_entry,@function
rev_runtime_entry:
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    call rev_runtime_initialize
    test eax,eax
    jz .Lfailed
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
    jmp rev_original_entry
.Lfailed:
    # No installed gateway may run with a partially constructed graph.
    ud2
.size rev_runtime_entry,.-rev_runtime_entry
.section .note.GNU-stack,"",@progbits
