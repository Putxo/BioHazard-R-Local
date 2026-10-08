.intel_syntax noprefix
.code32
.text
.global rev_pad_clear_gate
.type rev_pad_clear_gate,@function
rev_pad_clear_gate:
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    mov eax,[esi+8] # native EBP: sGamePad this is at EBP-8
    push dword ptr [esi+48] # size
    push dword ptr [esi+44] # value
    push dword ptr [esi+40] # destination
    push dword ptr [eax-8]
    call rev_preserve_other_pad
    add esp,16
    test eax,eax
    jz stock
    # memset returns its destination; original caller owns the three arguments.
    mov eax,[esi+40]
    mov [esi+28],eax
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
    ret
stock:
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
    jmp rev_pad_stock_memset
.size rev_pad_clear_gate,.-rev_pad_clear_gate
.section .note.GNU-stack,"",@progbits
