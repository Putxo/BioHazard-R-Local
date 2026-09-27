.intel_syntax noprefix
.code32
.text
.global rev_genesis_detect_gate
rev_genesis_detect_gate:
    push ecx
    push ecx
    call rev_genesis_detect
    add esp,4
    pop ecx
    test eax,eax
    jz rev_genesis_detect_stock
    ret

.global rev_genesis_detector_camera_gate
rev_genesis_detector_camera_gate:
    push ecx
    sub esp,4
    lea edx,[esp]
    push edx
    push ecx
    call rev_genesis_detector_camera
    add esp,8
    pop edx
    pop ecx
    test eax,eax
    jz rev_genesis_detector_camera_stock
    mov eax,edx
    ret

.macro detector_getter name, cleanup
.global rev_genesis_detector_\name\()_gate
rev_genesis_detector_\name\()_gate:
    push ecx
    sub esp,4
    lea edx,[esp]
    push edx
    call rev_genesis_detector_\name
    add esp,4
    pop edx
    pop ecx
    test eax,eax
    jz rev_genesis_detector_\name\()_stock
    mov eax,edx
    ret \cleanup
.endm
detector_getter actor,4
detector_getter widget,0

.macro target_setter name, result_offset
.global rev_genesis_set_\name\()_gate
rev_genesis_set_\name\()_gate:
    push ecx
    push dword ptr [esp+8]
    push ecx
    call rev_genesis_set_\name
    add esp,8
    pop ecx
    test eax,eax
    jz rev_genesis_set_\name\()_stock
    lea eax,[ecx+\result_offset]
    ret 4
.global rev_genesis_set_\name\()_stock
rev_genesis_set_\name\()_stock:
    push ebp
    mov ebp,esp
    sub esp,0xCC
    jmp rev_genesis_set_\name\()_resume
.endm
target_setter focus,0
target_setter position,0x10

.global rev_genesis_candidate_gate
rev_genesis_candidate_gate:
    push ecx
    push dword ptr [esp+8]
    call rev_genesis_candidate
    add esp,4
    pop ecx
    test eax,eax
    jnz rev_genesis_candidate_stock
    xor eax,eax
    ret 4
.section .note.GNU-stack,"",@progbits
