.intel_syntax noprefix
.code32
.section .text
.global door_gimmick_pad_query

.equ G_LOCAL_ACTIVE, 0x057D9188
.equ ACTOR_SELECTOR, 0x01C6C746
.equ GET_SGAMEPAD,   0x01C8B7F4
.equ PAD_QUERY,      0x01B94F53

# Called from MoveState::slot8 instead of:
#   push 0
#   get sGamePad
#   sGamePad::query(selector)
#
# Reuse the caller's EBP frame so [ebp-0x20] is the actor already resolved by
# MoveState. Preserve stock/online behavior by forcing selector 0 unless the
# canonical local-coop flag is active.
door_gimmick_pad_query:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je .stock_zero

    mov ecx, dword ptr [ebp-0x20]
    call ACTOR_SELECTOR
    jmp .have_selector

.stock_zero:
    xor eax, eax

.have_selector:
    push eax
    call GET_SGAMEPAD
    mov ecx, eax
    call PAD_QUERY
    ret
