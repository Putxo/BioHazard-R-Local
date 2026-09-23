.intel_syntax noprefix
.code32
.section .text
.global _start

.equ CHECK_CAMERA_READY, 0x01B7BA8A
.equ INIT_CAMERA_MODE,   0x01B8AAEE
.equ ACTIVATE_MANAGER,   0x01BF353F
.equ SET_VIEW_CAMERA,    0x01C34D5A
.equ REFRESH_CAMERA,     0x01BFD4B8
.equ STACK_CHECK,        0x01C82CBC

_start:
    pushad
    mov esi, dword ptr [ebp-8]
    test esi, esi
    jz done_ops

    mov ebx, dword ptr [esi+0xCE0]
    mov edi, dword ptr [esi+0xCE4]
    test ebx, ebx
    jz done_ops
    test edi, edi
    jz done_ops

    mov ecx, ebx
    call CHECK_CAMERA_READY
    test al, al
    jnz self_ready
    push 0x13
    mov ecx, ebx
    call INIT_CAMERA_MODE
self_ready:
    mov ecx, ebx
    call ACTIVATE_MANAGER

    mov ecx, edi
    call CHECK_CAMERA_READY
    test al, al
    jnz partner_ready
    push 0x13
    mov ecx, edi
    call INIT_CAMERA_MODE
partner_ready:
    mov ecx, edi
    call ACTIVATE_MANAGER

    push ebx
    push 0
    mov ecx, esi
    call SET_VIEW_CAMERA

    push edi
    push 1
    mov ecx, esi
    call SET_VIEW_CAMERA

    mov byte ptr [esi+0x40], 1
    mov byte ptr [esi+0x43], 2
    mov byte ptr [esi+0x44], 0

    mov byte ptr [esi+0x1D0], 1
    mov byte ptr [esi+0x1D3], 3
    mov byte ptr [esi+0x1D4], 0

    mov ecx, esi
    call REFRESH_CAMERA

done_ops:
    popad
    pop edi
    pop esi
    pop ebx
    add esp, 0x10C
    cmp ebp, esp
    call STACK_CHECK
    mov esp, ebp
    pop ebp
    ret
