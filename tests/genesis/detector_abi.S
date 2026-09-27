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
again:
    mov ecx,0xabcdef00
    call rev_genesis_detect_gate
    call check
    mov ecx,0xabcdef00
    call rev_genesis_detector_camera_gate
    call check_getter
    push 0x12345678
    mov ecx,0xabcdef00
    call rev_genesis_detector_actor_gate
    call check_getter
    mov ecx,0xabcdef00
    call rev_genesis_detector_widget_gate
    call check_getter
    push 0x12345678
    mov ecx,0xabcdef00
    call rev_genesis_set_focus_gate
    cmp eax,0xabcdef00
    jne fail
    call check
    push 0x12345678
    mov ecx,0xabcdef00
    call rev_genesis_set_position_gate
    cmp eax,0xabcdef10
    jne fail
    call check
    push 0x12345678
    mov ecx,0xabcdef00
    call rev_genesis_candidate_gate
    call check
    inc dword ptr [mode]
    cmp dword ptr [mode],3
    jne again
    cmp dword ptr [stock_calls],8
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
check_getter:
    cmp dword ptr [mode],2
    je hidden
    cmp eax,0x43210000
    jne fail
    jmp check
hidden:
    test eax,eax
    jnz fail
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
.global rev_genesis_detect
rev_genesis_detect:
    cmp dword ptr [esp+4],0xabcdef00
    jne fail
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_detector_camera
rev_genesis_detector_camera:
    cmp dword ptr [esp+4],0xabcdef00
    jne fail
    mov edx,[esp+8]
    jmp output
.global rev_genesis_detector_actor
.global rev_genesis_detector_widget
rev_genesis_detector_actor:
rev_genesis_detector_widget:
    mov edx,[esp+4]
output:
    mov eax,[mode]
    test eax,eax
    jz output_done
    mov dword ptr [edx],0
    cmp eax,2
    je output_done
    mov dword ptr [edx],0x43210000
output_done:
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_set_focus
.global rev_genesis_set_position
rev_genesis_set_focus:
rev_genesis_set_position:
    cmp dword ptr [esp+4],0xabcdef00
    jne fail
    cmp dword ptr [esp+8],0x12345678
    jne fail
    mov eax,[mode]
    xor ecx,ecx
    xor edx,edx
    ret
.global rev_genesis_candidate
rev_genesis_candidate:
    cmp dword ptr [esp+4],0x12345678
    jne fail
    xor eax,eax
    cmp dword ptr [mode],2
    setne al
    xor ecx,ecx
    xor edx,edx
    ret
.macro stock name, cleanup
.global rev_genesis_\name\()_stock
rev_genesis_\name\()_stock:
    cmp ecx,0xabcdef00
    jne fail
.if \cleanup
    cmp dword ptr [esp+4],0x12345678
    jne fail
.endif
    inc dword ptr [stock_calls]
    mov eax,0x43210000
    ret \cleanup
.endm
stock detect,0
stock detector_camera,0
stock detector_actor,4
stock detector_widget,0
stock candidate,4
.macro resume name, result
.global rev_genesis_set_\name\()_resume
rev_genesis_set_\name\()_resume:
    cmp ecx,0xabcdef00
    jne fail
    cmp dword ptr [ebp],0x66778899
    jne fail
    cmp dword ptr [ebp+8],0x12345678
    jne fail
    lea eax,[esp+0xCC]
    cmp eax,ebp
    jne fail
    inc dword ptr [stock_calls]
    lea eax,[ecx+\result]
    mov esp,ebp
    pop ebp
    ret 4
.endm
resume focus,0
resume position,0x10
fail:
    mov ebx,1
    mov eax,1
    int 0x80
.bss
.align 4
before:.skip 4
mode:.skip 4
stock_calls:.skip 4
.section .note.GNU-stack,"",@progbits
