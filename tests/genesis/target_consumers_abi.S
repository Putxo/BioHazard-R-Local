.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebp,0x66778899
    mov ebx,0x11223344
    mov esi,0x22334455
    mov edi,0x33445566
    mov dword ptr [mode],0
loop:
    mov ecx,0xabcdef00
    call rev_genesis_target_focus_gate
    cmp eax,[focus_expected]
    jne fail
    call check
    lea eax,[output]
    push eax
    mov ecx,0xabcdef00
    call rev_genesis_target_position_gate
    cmp eax,offset output
    jne fail
    mov eax,[position_expected]
    cmp [output],eax
    jne fail
    call check
    inc dword ptr [mode]
    cmp dword ptr [mode],3
    jne loop
    cmp dword ptr [stock_calls],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
check:
    lea eax,[esp+4]
    cmp eax,[before]
    jne fail
    cmp ebp,0x66778899
    jne fail
    cmp ebx,0x11223344
    jne fail
    cmp esi,0x22334455
    jne fail
    cmp edi,0x33445566
    jne fail
    ret
.global rev_genesis_target_focus
rev_genesis_target_focus:
    cmp dword ptr [esp+4],0xabcdef00
    jne fail
    mov eax,[mode]
    test eax,eax
    jz focus_done
    mov ecx,22
    cmp eax,1
    je focus_value
    xor ecx,ecx
focus_value:
    mov [focus_expected],ecx
    mov edx,[esp+8]
    mov [edx],ecx
focus_done:
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_target_position
rev_genesis_target_position:
    cmp dword ptr [esp+4],0xabcdef00
    jne fail
    cmp dword ptr [esp+8],offset output
    jne fail
    mov eax,[mode]
    test eax,eax
    jz position_done
    mov ecx,222
    cmp eax,1
    je position_value
    xor ecx,ecx
position_value:
    mov [position_expected],ecx
    mov [output],ecx
position_done:
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_target_focus_resume
rev_genesis_target_focus_resume:
    cmp ecx,0xabcdef00
    jne fail
    cmp dword ptr [ebp],0x66778899
    jne fail
    lea eax,[esp+0xCC]
    cmp eax,ebp
    jne fail
    inc dword ptr [stock_calls]
    mov dword ptr [focus_expected],11
    mov eax,11
    mov esp,ebp
    pop ebp
    ret
.global rev_genesis_target_position_resume
rev_genesis_target_position_resume:
    cmp ecx,0xabcdef00
    jne fail
    cmp dword ptr [ebp],0x66778899
    jne fail
    cmp dword ptr [ebp+8],offset output
    jne fail
    lea eax,[esp+0xCC]
    cmp eax,ebp
    jne fail
    inc dword ptr [stock_calls]
    mov dword ptr [position_expected],111
    mov dword ptr [output],111
    mov eax,[ebp+8]
    mov esp,ebp
    pop ebp
    ret 4
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before:.skip 4
mode:.skip 4
focus_expected:.skip 4
position_expected:.skip 4
stock_calls:.skip 4
output:.skip 16
.section .note.GNU-stack,"",@progbits
