.intel_syntax noprefix
.code32
.section .text
.global pickup_is_player_or_local_sub0

.equ STOCK_IS_PL,       0x01C8C9A1
.equ GET_ACTOR_ID,      0x01C53859
.equ G_SUB0_NPC,        0x057D9184
.equ G_LOCAL_ACTIVE,    0x057D9188

pickup_is_player_or_local_sub0:
    mov eax, dword ptr [esp+4]
    push eax
    call STOCK_IS_PL
    add esp, 4
    test al, al
    jnz true_result

    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je false_result

    mov ecx, dword ptr [G_SUB0_NPC]
    test ecx, ecx
    jz false_result

    call GET_ACTOR_ID
    cmp eax, dword ptr [esp+4]
    sete al
    ret

true_result:
    mov al, 1
    ret

false_result:
    xor eax, eax
    ret
