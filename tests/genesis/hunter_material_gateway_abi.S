.intel_syntax noprefix
.code32
.text
.global _start
_start:
    and esp,-16
    call run_tests
    mov ebx,eax
    mov eax,1
    int 0x80
.global call_bind
call_bind:
    push ebp
    mov ebp,esp
    sub esp,4
    mov eax,[ebp+8]
    mov [ebp-4],eax
    push dword ptr [ebp+16]
    mov ecx,[ebp+12]
    call rev_genesis_hunter_material_bind_gate
    mov esp,ebp
    pop ebp
    ret
.global call_unbind
call_unbind:
    push ebp
    mov ebp,esp
    mov ecx,[ebp+8]
    call rev_genesis_hunter_material_unbind_gate
    mov esp,ebp
    pop ebp
    ret
.global rev_runtime_hunter_material_begin
rev_runtime_hunter_material_begin:
    mov eax,[esp+4]
    mov [seen_begin_hunter],eax
    mov eax,[esp+8]
    mov [seen_begin_context],eax
    ret
.global rev_runtime_hunter_material_end
rev_runtime_hunter_material_end:
    inc dword ptr [seen_end]
    ret
.global rev_genesis_hunter_material_bind_stock
rev_genesis_hunter_material_bind_stock:
    mov [seen_bind_context],ecx
    mov eax,[esp+4]
    mov [seen_bind_material],eax
    mov eax,0xB16DB16D
    ret 4
.global rev_genesis_hunter_material_unbind_stock
rev_genesis_hunter_material_unbind_stock:
    mov [seen_unbind_context],ecx
    mov eax,0xABCD1234
    ret
.bss
.global seen_begin_hunter,seen_begin_context,seen_bind_context,seen_bind_material,seen_unbind_context,seen_end
seen_begin_hunter:.skip 4
seen_begin_context:.skip 4
seen_bind_context:.skip 4
seen_bind_material:.skip 4
seen_unbind_context:.skip 4
seen_end:.skip 4
.section .note.GNU-stack,"",@progbits
