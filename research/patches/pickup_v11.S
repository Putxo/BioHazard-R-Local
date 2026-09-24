.intel_syntax noprefix
.code32
.section .text
.global pickup_find_candidate
.global pickup_accept_f48
.global pickup_accept_arg

.equ G_SUB0_NPC,       0x057D9184
.equ G_LOCAL_ACTIVE,   0x057D9188
.equ STOCK_FIND,       0x01C07341
.equ IS_PLAYER,        0x01C8C9A1
.equ GET_ACTOR_POS,    0x01C684B6

# Call-site-specific wrapper for uItem update @ 0x02462159.
# Entry matches stock finder: ECX=manager, [esp+4]=local ID.
# Returns actor in EAX and RET 4, matching stock ABI.
pickup_find_candidate:
    push ebx
    push esi
    push edi

    mov edi, ecx
    mov edx, dword ptr [esp+16]
    push edx
    mov ecx, edi
    call STOCK_FIND
    mov ebx, eax

    cmp dword ptr [G_LOCAL_ACTIVE], 1
    jne .return_main
    mov esi, dword ptr [G_SUB0_NPC]
    test esi, esi
    jz .return_main
    test ebx, ebx
    jz .return_sub

    # Obtain item position exactly through the stock virtual +0x60 path.
    mov ecx, dword ptr [ebp-8]
    test ecx, ecx
    jz .return_main
    mov eax, dword ptr [ecx]
    push -1
    call dword ptr [eax+0x60]
    test eax, eax
    jz .return_main
    add eax, 0x30
    mov edi, eax

    # Main squared 3D distance.
    mov ecx, ebx
    call GET_ACTOR_POS
    movss xmm3, dword ptr [eax]
    subss xmm3, dword ptr [edi]
    mulss xmm3, xmm3
    movss xmm0, dword ptr [eax+4]
    subss xmm0, dword ptr [edi+4]
    mulss xmm0, xmm0
    addss xmm3, xmm0
    movss xmm0, dword ptr [eax+8]
    subss xmm0, dword ptr [edi+8]
    mulss xmm0, xmm0
    addss xmm3, xmm0

    # Exact Sub0 squared 3D distance.
    mov ecx, esi
    call GET_ACTOR_POS
    movss xmm1, dword ptr [eax]
    subss xmm1, dword ptr [edi]
    mulss xmm1, xmm1
    movss xmm0, dword ptr [eax+4]
    subss xmm0, dword ptr [edi+4]
    mulss xmm0, xmm0
    addss xmm1, xmm0
    movss xmm0, dword ptr [eax+8]
    subss xmm0, dword ptr [edi+8]
    mulss xmm0, xmm0
    addss xmm1, xmm0

    comiss xmm1, xmm3
    jb .return_sub

.return_main:
    mov eax, ebx
    pop edi
    pop esi
    pop ebx
    ret 4

.return_sub:
    mov eax, esi
    pop edi
    pop esi
    pop ebx
    ret 4

# Replacement for isPlayer at uItem processing @ 0x02460664.
# Original ID stays on caller stack; caller still does add esp,4.
pickup_accept_f48:
    push dword ptr [esp+4]
    call IS_PLAYER
    add esp, 4
    test al, al
    jnz .f48_done

    cmp dword ptr [G_LOCAL_ACTIVE], 1
    jne .f48_false
    mov eax, dword ptr [ebp-8]
    test eax, eax
    jz .f48_false
    mov eax, dword ptr [eax+0xF48]
    cmp eax, dword ptr [G_SUB0_NPC]
    sete al
    ret

.f48_false:
    xor eax, eax
.f48_done:
    ret

# Replacement for isPlayer inside helper 0x024646F0 @ 0x02464726.
# The helper's actor argument is caller [ebp+8].
pickup_accept_arg:
    push dword ptr [esp+4]
    call IS_PLAYER
    add esp, 4
    test al, al
    jnz .arg_done

    cmp dword ptr [G_LOCAL_ACTIVE], 1
    jne .arg_false
    mov eax, dword ptr [ebp+8]
    cmp eax, dword ptr [G_SUB0_NPC]
    sete al
    ret

.arg_false:
    xor eax, eax
.arg_done:
    ret
