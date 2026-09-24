.intel_syntax noprefix
.code32
.section .text
.global choose_pickup_actor
.global pickup_gate1_false
.global pickup_gate2_false

.equ G_SUB0_NPC,        0x057D9184
.equ G_LOCAL_ACTIVE,    0x057D9188
.equ GATE1_CONTINUE,    0x02460677
.equ GATE1_EXIT,        0x02460B64
.equ GATE2_CONTINUE,    0x02464739
.equ GATE2_EXIT,        0x024649AE

choose_pickup_actor:
    push ebx
    push esi
    push edi
    mov edi, eax
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je .finish
    mov esi, dword ptr [G_SUB0_NPC]
    test esi, esi
    jz .finish
    test edi, edi
    jz .choose_sub

    mov ecx, dword ptr [ebp-0x08]
    test ecx, ecx
    jz .finish
    mov edx, dword ptr [ecx]
    push -1
    call dword ptr [edx+0x60]
    test eax, eax
    jz .finish
    mov ebx, eax

    movss xmm0, dword ptr [edi+0x40]
    subss xmm0, dword ptr [ebx+0x30]
    mulss xmm0, xmm0
    movss xmm1, dword ptr [edi+0x44]
    subss xmm1, dword ptr [ebx+0x34]
    mulss xmm1, xmm1
    addss xmm0, xmm1
    movss xmm1, dword ptr [edi+0x48]
    subss xmm1, dword ptr [ebx+0x38]
    mulss xmm1, xmm1
    addss xmm0, xmm1

    movss xmm2, dword ptr [esi+0x40]
    subss xmm2, dword ptr [ebx+0x30]
    mulss xmm2, xmm2
    movss xmm3, dword ptr [esi+0x44]
    subss xmm3, dword ptr [ebx+0x34]
    mulss xmm3, xmm3
    addss xmm2, xmm3
    movss xmm3, dword ptr [esi+0x48]
    subss xmm3, dword ptr [ebx+0x38]
    mulss xmm3, xmm3
    addss xmm2, xmm3

    comiss xmm2, xmm0
    jae .finish
.choose_sub:
    mov edi, esi
.finish:
    mov eax, edi
    pop edi
    pop esi
    pop ebx
    mov dword ptr [ebp-0x2C], eax
    cmp dword ptr [ebp-0x2C], 0
    ret

pickup_gate1_false:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je GATE1_EXIT
    mov eax, dword ptr [ebp-0x08]
    mov eax, dword ptr [eax+0xF48]
    cmp eax, dword ptr [G_SUB0_NPC]
    je GATE1_CONTINUE
    jmp GATE1_EXIT

pickup_gate2_false:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je GATE2_EXIT
    mov eax, dword ptr [ebp+0x08]
    cmp eax, dword ptr [G_SUB0_NPC]
    je GATE2_CONTINUE
    jmp GATE2_EXIT
