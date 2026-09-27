.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    lea eax,[weapon]
    stc
    call rev_genesis_weapon_born_gate
    jnc fail
    cmp eax,offset weapon
    jne fail
    mov dword ptr [weapon],0x87654321
    lea eax,[weapon]
    stc
    call rev_genesis_weapon_dying_gate
    jnc fail
    cmp dword ptr [weapon],0x04D38EE4
    jne fail
    mov [before],esp
    mov dword ptr [scanner+0x2FC],0x55555555
    push 0xED1000
    push 1
    call native_entry
    cmp esp,[before]
    jne fail
    cmp dword ptr [scanner+0x2FC],0x55555555
    jne fail
    cmp dword ptr [resumes],0
    jne fail
    mov dword ptr [allow],1
    push 0xED1000
    push 1
    call native_entry
    cmp esp,[before]
    jne fail
    cmp dword ptr [scanner+0x2FC],0xED1000
    jne fail
    cmp dword ptr [resumes],1
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
    cmp dword ptr [observations],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_weapon_event
rev_genesis_weapon_event:
    cmp dword ptr [esp+8],offset weapon
    jne fail
    mov eax,[esp+4]
    cmp eax,0x0248731E
    jne dying
    cmp dword ptr [weapon],0x04D38EE4
    jne fail
    jmp event_done
dying:
    cmp eax,0x02487426
    jne fail
    cmp dword ptr [weapon],0x87654321
    jne fail
event_done:
    inc dword ptr [observations]
    xor eax,eax
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_weapon_born_resume
rev_genesis_weapon_born_resume: ret
.global rev_genesis_weapon_dying_resume
rev_genesis_weapon_dying_resume: ret
.global rev_genesis_weapon_retain
rev_genesis_weapon_retain:
    cmp dword ptr [esp+4],offset scanner
    jne fail
    cmp dword ptr [esp+8],0xED1000
    jne fail
    xor ecx,ecx
    xor edx,edx
    mov eax,[allow]
    ret
native_entry:
    push ebp
    mov ebp,esp
    sub esp,0x164
    push ebx
    push esi
    push edi
    lea eax,[scanner]
    mov ecx,[ebp+12]
    stc
    jmp rev_genesis_weapon_retain_gate
.global rev_genesis_weapon_retain_resume
rev_genesis_weapon_retain_resume:
    jnc fail
    inc dword ptr [resumes]
    stc
.global rev_genesis_weapon_activate_end
rev_genesis_weapon_activate_end:
    jnc fail
    pop edi
    pop esi
    pop ebx
    add esp,0x164
    mov esp,ebp
    pop ebp
    ret 8
.bss
.align 4
weapon:.skip 4
scanner:.skip 0x400
before:.skip 4
allow:.skip 4
resumes:.skip 4
observations:.skip 4
.section .note.GNU-stack,"",@progbits
