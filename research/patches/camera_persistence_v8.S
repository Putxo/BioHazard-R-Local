.intel_syntax noprefix
.code32
.section .text
.global local_enable
.global pad_rebind_check
.global conditional_deactivate
.global partner_view_index
.global conditional_bit0
.global conditional_bit1
.global ensure_split

.equ G_SUB0_NPC,         0x057D9184
.equ G_LOCAL_ACTIVE,     0x057D9188
.equ LOOP_CONTINUE,      0x02DF5022

.equ GET_GAME_CAMERA,    0x01C4A745
.equ SET_TARGET,         0x01C275EC
.equ CHECK_READY,        0x01B7BA8A
.equ INIT_CAMERA_MODE,   0x01B8AAEE
.equ ACTIVATE_MANAGER,   0x01BF353F
.equ DEACTIVATE_MANAGER, 0x01C684D4
.equ SET_VIEW_CAMERA,    0x01C34D5A
.equ REFRESH_CAMERA,     0x01BFD4B8
.equ STOCK_VIEW_INDEX,   0x01C083B3
.equ SET_STATE_BIT0,     0x01C5EFA1
.equ SET_STATE_BIT1,     0x01B8D636

local_enable:
    mov dword ptr [G_LOCAL_ACTIVE], 1
    call ensure_split
    jmp LOOP_CONTINUE

pad_rebind_check:
    cmp dword ptr [eax+0xE40], 1
    jne LOOP_CONTINUE
    cmp dword ptr [G_LOCAL_ACTIVE], 1
    jne LOOP_CONTINUE
    call ensure_split
    jmp LOOP_CONTINUE

conditional_deactivate:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je 1f
    call ensure_split
    ret
1:
    jmp DEACTIVATE_MANAGER

partner_view_index:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je 1f
    mov eax, 1
    ret
1:
    jmp STOCK_VIEW_INDEX

conditional_bit0:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je 1f
    mov dword ptr [esp+4], 1
1:
    jmp SET_STATE_BIT0

conditional_bit1:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je 1f
    mov dword ptr [esp+4], 1
1:
    jmp SET_STATE_BIT1

ensure_split:
    pushad
    mov esi, dword ptr [G_SUB0_NPC]
    test esi, esi
    jz done

    call GET_GAME_CAMERA
    test eax, eax
    jz done
    mov edi, eax

    mov ebp, dword ptr [edi+0xCE0]
    mov ebx, dword ptr [edi+0xCE4]
    test ebp, ebp
    jz done
    test ebx, ebx
    jz done

    push esi
    mov ecx, ebx
    call SET_TARGET

    mov ecx, ebp
    call CHECK_READY
    test al, al
    jnz self_ready
    push 0x13
    mov ecx, ebp
    call INIT_CAMERA_MODE
self_ready:
    mov ecx, ebp
    call ACTIVATE_MANAGER

    mov ecx, ebx
    call CHECK_READY
    test al, al
    jnz partner_ready
    push 0x13
    mov ecx, ebx
    call INIT_CAMERA_MODE
partner_ready:
    mov ecx, ebx
    call ACTIVATE_MANAGER

    push ebp
    push 0
    mov ecx, edi
    call SET_VIEW_CAMERA

    push ebx
    push 1
    mov ecx, edi
    call SET_VIEW_CAMERA

    mov byte ptr [edi+0x40], 1
    mov byte ptr [edi+0x43], 2
    mov byte ptr [edi+0x44], 0

    mov byte ptr [edi+0x1D0], 1
    mov byte ptr [edi+0x1D3], 3
    mov byte ptr [edi+0x1D4], 0

    mov ecx, edi
    call REFRESH_CAMERA

done:
    popad
    ret
