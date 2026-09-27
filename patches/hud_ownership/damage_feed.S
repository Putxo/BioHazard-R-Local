# Owned damage widget feed; cdecl(unit, actor). Native targets supplied by linker.
# No global Self swap. State wrapper is one word; health return is x87 ST(0).
.intel_syntax noprefix
.code32
.text
.global rev_january_damage
rev_january_damage:
    push ebp
    mov ebp,esp
    sub esp,24
    lea eax,[ebp-4]
    push eax
    mov ecx,[ebp+12]
    call rev_damage_actor_state
    mov ecx,eax
    call rev_damage_state_value
    cmp eax,4
    je .Lhidden
    mov ecx,[ebp+12]
    call rev_damage_health_ratio
    # Inspect a rounded copy without consuming or rounding the native result.
    fst dword ptr [ebp-8]
    mov eax,[ebp-8]
    test eax,0x80000000
    jnz .Lbad_ratio
    cmp eax,0x3F800000
    ja .Lbad_ratio
    fld1
    fsubrp st(1),st(0)
    fmul dword ptr [.Lhundred]
    fnstcw word ptr [ebp-12]
    movzx eax,word ptr [ebp-12]
    or eax,0xC00
    mov [ebp-16],eax
    fldcw word ptr [ebp-16]
    fistp qword ptr [ebp-24]
    fldcw word ptr [ebp-12]
    push dword ptr [ebp-24]
    mov ecx,[ebp+8]
    call rev_damage_set_percent
    push 1
    jmp .Lvisible
.Lbad_ratio:
    fstp st(0)
    xor eax,eax
    jmp .Lreturn
.Lhidden:
    push 0
.Lvisible:
    mov ecx,[ebp+8]
    call rev_damage_set_visible
    mov eax,1
.Lreturn:
    mov esp,ebp
    pop ebp
    ret
.section .rodata
.align 4
.Lhundred: .long 0x42C80000
.section .note.GNU-stack,"",@progbits
