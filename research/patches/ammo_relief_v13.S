.intel_syntax noprefix
.code32
.section .text
.global coop_or_local
.global select_relief_other
.global valid_relief_other
.global ammo_multiplier_local

.equ G_SUB0_NPC,          0x057D9184
.equ G_LOCAL_ACTIVE,      0x057D9188
.equ COOP_AMMO_MULT,      0x05479970

.equ STOCK_IS_COOP,       0x01C076CF
.equ STOCK_VALID_OTHER,   0x01C853D1
.equ STOCK_AMMO_MULT,     0x01B8D7F8
.equ GET_ACTOR_ID,        0x01C53859
.equ IS_PL,               0x01C8C9A1
.equ GET_NETWORK_MANAGER, 0x01B95665
.equ GET_SELF_ID,         0x01C85962
.equ GET_ACTOR_BY_ID,     0x01C07341

coop_or_local:
    call STOCK_IS_COOP
    test al, al
    jnz .coop_done
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    setne al
.coop_done:
    ret

# EAX = stock-selected other actor.
# EBP is the frame of 0x027ABFE0.
select_relief_other:
    push ebx
    push esi
    push edi
    mov edi, eax

    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je .select_finish
    mov esi, dword ptr [G_SUB0_NPC]
    test esi, esi
    jz .select_finish

    mov ebx, dword ptr [ebp-0x08]
    cmp ebx, esi
    je .current_is_sub

    # Non-Sub callers are overridden only when they are the actual pl actor.
    mov ecx, ebx
    call GET_ACTOR_ID
    push eax
    call IS_PL
    add esp, 4
    test al, al
    jz .select_finish
    mov edi, esi
    jmp .select_finish

.current_is_sub:
    # Same stock P1 lookup used by uItem pickup candidate selection.
    call GET_NETWORK_MANAGER
    test eax, eax
    jz .select_finish
    mov ecx, eax
    call GET_SELF_ID
    push eax
    call GET_NETWORK_MANAGER
    test eax, eax
    jz .drop_self_id
    mov ecx, eax
    call GET_ACTOR_BY_ID
    mov edi, eax
    jmp .select_finish
.drop_self_id:
    add esp, 4

.select_finish:
    mov eax, edi
    pop edi
    pop esi
    pop ebx
    mov dword ptr [ebp-0x14], eax
    cmp dword ptr [ebp-0x14], 0
    ret

# Stock accepts the NPC/Cpu partner. For local co-op only, also accept a pl
# actor so relief is symmetric when Sub0 is the actor that picked the ammo.
valid_relief_other:
    push ecx
    call STOCK_VALID_OTHER
    pop ecx
    test al, al
    jnz .valid_done
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je .valid_done

    push ecx
    call GET_ACTOR_ID
    push eax
    call IS_PL
    add esp, 4
    pop ecx
.valid_done:
    ret

# Use the exact configured Coop ammo acquisition multiplier without changing
# cSystemData<Game>::mGameMode globally.
ammo_multiplier_local:
    cmp dword ptr [G_LOCAL_ACTIVE], 0
    je .stock_multiplier
    fld dword ptr [COOP_AMMO_MULT]
    ret
.stock_multiplier:
    jmp STOCK_AMMO_MULT
