.intel_syntax noprefix
.code32
.text
.extern rev_runtime_hunter_material_begin
.extern rev_runtime_hunter_material_end
.extern rev_genesis_hunter_material_bind_stock
.extern rev_genesis_hunter_material_unbind_stock

.global rev_genesis_hunter_material_bind_gate
.type rev_genesis_hunter_material_bind_gate,@function
rev_genesis_hunter_material_bind_gate:
    push ebx
    push esi
    push edi
    mov esi,ecx
    mov edi,[esp+16]
    mov ebx,[ebp-4]
    push esi
    push ebx
    call rev_runtime_hunter_material_begin
    add esp,8
    push edi
    mov ecx,esi
    call rev_genesis_hunter_material_bind_stock
    pop edi
    pop esi
    pop ebx
    ret 4
.size rev_genesis_hunter_material_bind_gate,.-rev_genesis_hunter_material_bind_gate

.global rev_genesis_hunter_material_unbind_gate
.type rev_genesis_hunter_material_unbind_gate,@function
rev_genesis_hunter_material_unbind_gate:
    call rev_genesis_hunter_material_unbind_stock
    push eax
    call rev_runtime_hunter_material_end
    pop eax
    ret
.size rev_genesis_hunter_material_unbind_gate,.-rev_genesis_hunter_material_unbind_gate
.section .note.GNU-stack,"",@progbits
