.intel_syntax noprefix
.code32
.text
# Called only at the two audited update callsites, thiscall void with no args.
.macro update name,kind
.global rev_input_update_\name
rev_input_update_\name:
    push \kind
    push ecx
    call rev_local_input_update
    add esp,8
    ret
.endm
update device,0
update keyboard,1

# Internal MOV replacement. The stock read happens first; only its destination
# may change. Pinning is checked against the update's gamepad and owning thread.
.macro index name,destination,base,dest_saved,base_saved
.global rev_input_index_\name
rev_input_index_\name:
    mov \destination,[\base+0x970]
    pushfd
    pushad
    mov esi,esp
    and esp,-16
    sub esp,512
    fxsave [esp]
    cld
    sub esp,8
    push dword ptr [esi+\dest_saved]
    push dword ptr [esi+\base_saved]
    call rev_local_input_index
    add esp,16
    mov [esi+\dest_saved],eax
    fxrstor [esp]
    mov esp,esi
    popad
    popfd
    ret
.endm
index ecx_eax,ecx,eax,24,28
index edx_ecx,edx,ecx,20,24
index esi_eax,esi,eax,4,28
index ecx_edx,ecx,edx,24,20
index eax_edx,eax,edx,28,20
.section .note.GNU-stack,"",@progbits
