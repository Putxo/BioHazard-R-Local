.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov [before],esp
    mov ebp,0x11223344
    mov ebx,0x22334455
    mov esi,0x33445566
    mov edi,0x44556677
.again:
    call phase9_frame
    cmp esp,[before]
    jne fail
    cmp ebp,0x11223344
    jne fail
    cmp ebx,0x22334455
    jne fail
    cmp esi,0x33445566
    jne fail
    cmp edi,0x44556677
    jne fail
    inc dword ptr [mode]
    cmp dword ptr [mode],4
    jb .again
    cmp dword ptr [stock_calls],1
    jne fail
    cmp dword ptr [helper_calls],4
    jne fail
    cmp dword ptr [continued],2
    jne fail
    cmp dword ptr [exits],4
    jne fail
    cmp dword ptr [state_continued],2
    jne fail
    cmp dword ptr [phase_continued],2
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
phase9_frame:
    push ebx
    mov ebx,esp
    sub esp,8
    and esp,0xFFFFFFF0
    add esp,4
    push ebp
    mov ebp,[ebx+4]
    mov [esp+4],ebp
    mov ebp,esp
    sub esp,0x248
    push esi
    push edi
    mov dword ptr [ebp-8],0xAABBCC00
    mov ecx,0xAABBCC00
    push 1
    call rev_genesis_state_call_gate
    inc dword ptr [phase_continued]
.global rev_genesis_phase9_end
rev_genesis_phase9_end:
    lea eax,[esp+0x250]
    cmp eax,ebp
    jne fail
    pop edi
    pop esi
    mov esp,ebp
    pop ebp
    mov esp,ebx
    pop ebx
    ret
.global rev_genesis_state_stock
rev_genesis_state_stock:
    cmp ecx,0xAABBCC00
    jne fail
    cmp dword ptr [esp+4],1
    jne fail
    push ebp
    mov ebp,esp
    sub esp,0x108
    push ebx
    push esi
    push edi
    mov [ebp-8],ecx
    call rev_genesis_scan_call_gate
    inc dword ptr [state_continued]
.global rev_genesis_state_end
rev_genesis_state_end:
    lea eax,[esp+0x114]
    cmp eax,ebp
    jne fail
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret 4
.global rev_genesis_scan_stock
rev_genesis_scan_stock:
    cmp ecx,0xAABBCC00
    jne fail
    # Match the scanner's dynamically aligned stack, including EBX restore.
    push ebx
    mov ebx,esp
    sub esp,8
    and esp,0xFFFFFFF0
    add esp,4
    push ebp
    mov ebp,[ebx+4]
    mov [esp+4],ebp
    mov ebp,esp
    sub esp,0x3E8
    push esi
    push edi
    mov dword ptr [ebp-8],0xAABBCC00
    mov ecx,0xBBCCDD00
    lea edx,[target_vtable]
    mov esi,esp
    call rev_genesis_complete_gate
    cmp esi,esp
    jne fail
    inc dword ptr [continued]
.global rev_genesis_scan_end
rev_genesis_scan_end:
    # The original RTC helper pushes and removes two words before this tail.
    inc dword ptr [exits]
    lea eax,[esp+0x3F0]
    cmp eax,ebp
    jne fail
    pop edi
    pop esi
    mov esp,ebp
    pop ebp
    mov esp,ebx
    pop ebx
    ret
.global rev_genesis_complete
rev_genesis_complete:
    cmp dword ptr [esp+4],0xAABBCC00
    jne fail
    cmp dword ptr [esp+8],0xBBCCDD00
    jne fail
    inc dword ptr [helper_calls]
    xor ebp,ebp
    xor ebx,ebx
    xor esi,esi
    xor edi,edi
    xor ecx,ecx
    xor edx,edx
    mov eax,[mode]
    ret
.global rev_genesis_scan_aborted
rev_genesis_scan_aborted:
    cmp dword ptr [esp+4],0xAABBCC00
    jne fail
    xor ebp,ebp
    xor ebx,ebx
    xor esi,esi
    xor edi,edi
    xor ecx,ecx
    xor edx,edx
    xor eax,eax
    cmp dword ptr [mode],2
    setae al
    ret
stock:
    cmp ecx,0xBBCCDD00
    jne fail
    cmp edx,offset target_vtable
    jne fail
    inc dword ptr [stock_calls]
    ret
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.data
before:.long 0
mode:.long 0
stock_calls:.long 0
helper_calls:.long 0
continued:.long 0
exits:.long 0
state_continued:.long 0
phase_continued:.long 0
target_vtable:.long 0,0,0,0,0,stock
.section .note.GNU-stack,"",@progbits
