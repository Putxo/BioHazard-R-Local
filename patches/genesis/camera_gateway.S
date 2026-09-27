.intel_syntax noprefix
.code32
.text
.global rev_genesis_camera_gate
rev_genesis_camera_gate:
    push ecx
    sub esp,4
    lea edx,[esp]
    push edx
    push ecx
    push dword ptr [ebp-8]
    call rev_genesis_camera
    add esp,12
    pop edx
    pop ecx
    test eax,eax
    jz 1f
    mov eax,edx
    ret
1:
    jmp rev_genesis_stock_camera
.section .note.GNU-stack,"",@progbits
