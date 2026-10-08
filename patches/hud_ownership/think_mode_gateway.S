.intel_syntax noprefix
.code32
.text
.global rev_local_think_mode_gate
.type rev_local_think_mode_gate,@function
rev_local_think_mode_gate:
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    sub esp,8
    # Original thiscall argument = saved ESP + 36 + 4.
    push dword ptr [esi+40]
    push dword ptr [esi+24]
    call rev_local_think_mode
    add esp,16
    mov dword ptr [esi+40],eax
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
    push ebp
    mov ebp,esp
    sub esp,0xCC
    jmp rev_local_think_mode_continue
.size rev_local_think_mode_gate,.-rev_local_think_mode_gate
.section .note.GNU-stack,"",@progbits
