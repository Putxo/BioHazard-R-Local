# Isolated i386 harness. Real gateways, synthetic receiver/continuations.
.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov dword ptr [case_no],0
.next:
    call test_one
    test eax,eax
    jne .failed
    inc dword ptr [case_no]
    cmp dword ptr [case_no],3
    jb .next
    mov eax,4
    mov ebx,1
    mov ecx,offset pass_msg
    mov edx,offset pass_len
    int 0x80
    xor ebx,ebx
    jmp .exit
.failed:
    mov ebx,[case_no]
    inc ebx
.exit:
    mov eax,1
    int 0x80

test_one:
    push ebp
    push ebx
    push esi
    push edi
    mov [saved_esp],esp
    mov dword ptr [callback_error],0
    mov dword ptr [callback_count],0
    fninit
    fld1
    movdqu xmm0,[xmm_seed]
    ldmxcsr [mxcsr_seed]
    fxsave [fx_before]
    mov ebp,offset fake_frame+128
    mov dword ptr [ebp-8],0xCCCCCCCC
    mov esp,offset test_stack+8192
    mov [initial_esp],esp
    mov dword ptr [esp],0x77777777
    mov dword ptr [esp+4],0x66666666
    mov dword ptr [esp+8],0x33333333
    mov eax,0x11221122
    mov ecx,0x00550000
    mov edx,0x33443344
    mov ebx,0x55555555
    mov esi,0x88888888
    mov edi,0x99999999
    cmp dword ptr [case_no],1
    jne .begin
    # END must use its initialized local, not the current ECX.
    mov dword ptr [ebp-8],0x00550000
    mov ecx,0x12345678
    stc
    std
    pushfd
    pop dword ptr [initial_flags]
    jmp rev_pipeline_gate_end
.begin:
    # BEGIN's local is poisoned until the replayed MOV stores ECX.
    stc
    std
    pushfd
    pop dword ptr [initial_flags]
    jmp rev_pipeline_gate_begin

.global rev_pipeline_continue_begin
rev_pipeline_continue_begin:
    jmp .verify
.global rev_pipeline_continue_end
rev_pipeline_continue_end:
.verify:
    mov [actual],eax
    mov [actual+4],ecx
    mov [actual+8],edx
    mov [actual+12],ebx
    mov [actual+16],esi
    mov [actual+20],edi
    mov [actual+24],ebp
    mov [actual+28],esp
    pushfd
    pop dword ptr [actual+32]
    fxsave [fx_after]
    cld
    mov eax,1
    cmp dword ptr [callback_error],0
    jne .done
    cmp dword ptr [callback_count],1
    jne .done
    mov edx,[fx_before]
    cmp edx,[fx_after]
    jne .done
    mov dl,[fx_before+4]
    cmp dl,[fx_after+4]
    jne .done
    mov edx,[fx_before+24]
    cmp edx,[fx_after+24]
    jne .done
    mov esi,offset fx_before+32
    mov edi,offset fx_after+32
    mov ecx,10
    repe cmpsb
    jne .done
    mov esi,offset fx_before+160
    mov edi,offset fx_after+160
    mov ecx,16
    repe cmpsb
    jne .done
    cmp dword ptr [actual],0x11221122
    jne .done
    cmp dword ptr [actual+8],0x33443344
    jne .done
    cmp dword ptr [actual+24],offset fake_frame+128
    jne .done
    cmp dword ptr [fake_frame+128-8],0x00550000
    jne .done
    cmp dword ptr [case_no],1
    je .endcheck
    cmp dword ptr [actual+4],0x00550000
    jne .done
    cmp dword ptr [actual+12],0x55555555
    jne .done
    cmp dword ptr [actual+16],0x88888888
    jne .done
    cmp dword ptr [actual+20],0x99999999
    jne .done
    mov edx,[initial_esp]
    cmp edx,[actual+28]
    jne .done
    mov edx,[actual+32]
    xor edx,[initial_flags]
    test edx,0xCD5
    jnz .done
    jmp .success
.endcheck:
    cmp dword ptr [actual+4],0x12345678
    jne .done
    cmp dword ptr [actual+12],0x33333333
    jne .done
    cmp dword ptr [actual+16],0x66666666
    jne .done
    cmp dword ptr [actual+20],0x77777777
    jne .done
    mov edx,[initial_esp]
    add edx,12
    add edx,0xD0
    pushfd
    pop esi
    cmp edx,[actual+28]
    jne .done
    xor esi,[actual+32]
    test esi,0x8D5
    jnz .done
    test dword ptr [actual+32],0x400
    jz .done
.success:
    xor eax,eax
.done:
    mov esp,[saved_esp]
    cld
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

.global rev_hud_pipeline_event
rev_hud_pipeline_event:
    pushfd
    pop eax
    test eax,0x400
    jnz .bad
    mov eax,esp
    and eax,15
    cmp eax,12
    jne .bad
    mov eax,0x02F506A0
    cmp dword ptr [case_no],1
    jne .site
    mov eax,0x02F50F4B
.site:
    cmp eax,[esp+4]
    jne .bad
    cmp dword ptr [esp+8],0x00550000
    jne .bad
    cmp dword ptr [esp+12],offset fake_frame+128
    jne .bad
    jmp .callback
.bad:
    mov dword ptr [callback_error],1
.callback:
    inc dword ptr [callback_count]
    fninit
    pxor xmm0,xmm0
    ldmxcsr [mxcsr_other]
    mov ecx,0xBBBBBBBB
    mov edx,0xCCCCCCCC
    # Third invocation returns false; original instruction replay must survive.
    xor eax,eax
    cmp dword ptr [case_no],2
    sete al
    xor eax,1
    ret
.section .rodata
.align 16
xmm_seed: .long 0x12345678,0x98ABCDEF,0x76543210,0xFFEEDDCC
mxcsr_seed: .long 0x1F80
mxcsr_other: .long 0x3F80
pass_msg: .ascii "{\"status\":\"PASS\",\"gateways\":2,\"invocations\":3,\"gpr_flags_stack_fx_verified\":true,\"native_i386\":true,\"receiver_mocked\":true,\"gameplay_executed\":false}\n"
.set pass_len,.-pass_msg
.bss
.align 16
fx_before: .skip 512
fx_after: .skip 512
actual: .skip 36
case_no: .skip 4
initial_flags: .skip 4
initial_esp: .skip 4
saved_esp: .skip 4
callback_error: .skip 4
callback_count: .skip 4
fake_frame: .skip 256
.align 16
test_stack: .skip 16384
.section .note.GNU-stack,"",@progbits
