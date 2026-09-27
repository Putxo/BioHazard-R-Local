.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov ebp,0x456789AB
    mov dword ptr [scanner+0x2FC],0xED1000
    mov [before],esp
    call command_entry
    cmp esp,[before]
    jne fail
    mov dword ptr [allow],1
    call command_entry
    cmp esp,[before]
    jne fail
    mov dword ptr [allow],0
    mov dword ptr [required],0
    push 0xAC
    call wrapper_entry
    cmp esp,[before]
    jne fail
    mov dword ptr [allow],1
    push 0xAC
    call wrapper_entry
    cmp esp,[before]
    jne fail
    cmp dword ptr [uses],4
    jne fail
    cmp dword ptr [commands],1
    jne fail
    cmp dword ptr [getters],1
    jne fail
    cmp dword ptr [activations],1
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    cmp ebp,0x456789AB
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.global rev_genesis_weapon_use
rev_genesis_weapon_use:
    cmp dword ptr [esp+4],offset scanner
    jne fail
    mov eax,[required]
    cmp [esp+8],eax
    jne fail
    inc dword ptr [uses]
    xor ecx,ecx
    xor edx,edx
    mov eax,[allow]
    ret
command_entry:
    push 7
    push 0
    lea eax,[scanner]
    mov ecx,0xABABABAB
    mov edx,0xCDCDCDCD
    stc
    call rev_genesis_weapon_command_gate
    nop
    call model_getter
    mov ecx,eax
    call model_command
    jnc fail
    # Both paths arrive here with the pending arguments removed.
    ret
model_getter:
    jnc fail
    cmp ecx,0xED1000
    jne fail
    cmp eax,offset scanner
    jne fail
    cmp edx,0xCDCDCDCD
    jne fail
    inc dword ptr [getters]
    mov eax,0xED8000
    ret
model_command:
    cmp ecx,0xED8000
    jne fail
    cmp dword ptr [esp+4],0
    jne fail
    cmp dword ptr [esp+8],7
    jne fail
    inc dword ptr [commands]
    stc
    ret 8
wrapper_entry:
    push ebp
    mov ebp,esp
    sub esp,8
    lea eax,[scanner]
    mov [ebp-8],eax
    stc
    call rev_genesis_weapon_reactivate_gate
    nop
    push ecx
    mov edx,[ebp+8]
    push edx
    mov ecx,[ebp-8]
    call activate
    jnc fail
    mov esp,ebp
    pop ebp
    ret 4
activate:
    jnc fail
    cmp ecx,offset scanner
    jne fail
    cmp dword ptr [esp+4],0xAC
    jne fail
    cmp dword ptr [esp+8],0xED1000
    jne fail
    inc dword ptr [activations]
    stc
    ret 8
.data
required:.long 1
.bss
.align 4
scanner:.skip 0x400
before:.skip 4
allow:.skip 4
uses:.skip 4
commands:.skip 4
getters:.skip 4
activations:.skip 4
.section .note.GNU-stack,"",@progbits
