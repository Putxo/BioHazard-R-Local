.intel_syntax noprefix
.code32
.text
.global _start,script_call_checked
_start:
    and esp,-16
    call run_script_tests
    push eax
    test eax,eax
    jnz .fail_message
    mov ecx,offset pass_message
    mov edx,pass_end-pass_message
    jmp .print
.fail_message:
    mov ecx,offset fail_message
    mov edx,fail_end-fail_message
.print:
    mov ebx,1
    mov eax,4
    int 0x80
    pop ebx
    mov eax,1
    int 0x80
# cdecl(owner,event); calls the actual helper using its Win32 thiscall ABI.
# Top bit signals register/stack contract failure; normal result is zero or one.
script_call_checked:
    push ebp
    mov ebp,esp
    push ebx
    push esi
    push edi
    mov ecx,dword ptr [ebp+8]
    push dword ptr [ebp+12]
    mov ebx,0x12345678
    mov esi,0x76543210
    mov edi,0x0badcafe
    call script_sub0_member
    cmp ebx,0x12345678
    jne .contract_failed
    cmp esi,0x76543210
    jne .contract_failed
    cmp edi,0x0badcafe
    jne .contract_failed
    cmp ecx,dword ptr [ebp+8]
    jne .contract_failed
    lea edx,[ebp-12]
    cmp esp,edx
    jne .contract_failed
    jmp .restore
.contract_failed:
    or eax,0x80000000
.restore:
    lea esp,[ebp-12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
.section .rodata
pass_message:.ascii "PASS: native i386 script selector, 2592 guard combinations and ABI checks; no game execution\n"
pass_end:
fail_message:.ascii "FAIL: native i386 script selector or ABI check\n"
fail_end:
.section .fixtures,"aw",@progbits
.skip 0x184
.global fixture_sub,fixture_active
fixture_sub:.long 0
fixture_active:.long 0
.section .note.GNU-stack,"",@progbits
