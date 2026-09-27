.intel_syntax noprefix
.code32
.text
.global rev_genesis_filter_load_gate
// Replaces mov eax,[edx+30];call eax at either resource-load branch.
// Original thiscall has three stack arguments and ret 12. EBP is the Scanner
// setup frame; its -8 slot owns the widget. Caller still checks native ESP.
rev_genesis_filter_load_gate:
    push dword ptr [esp+12]
    push dword ptr [esp+12]
    push dword ptr [esp+12]
    mov eax,dword ptr [edx+0x30]
    call eax
    push eax
    push dword ptr [ebp-8]
    call rev_genesis_filter_loaded
    add esp,8
    ret 12
.section .note.GNU-stack,"",@progbits
