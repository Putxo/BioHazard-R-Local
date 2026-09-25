.intel_syntax noprefix
.code32
.text
.global hook_begin,hook_wait_actor,hook_wait_serial,hook_candidate,hook_door_init
.global hook_availability,hook_wait_allowed
hook_begin:
 push dword ptr [ebp-0x14]
 call lc_begin
 add esp,4
 ret
hook_wait_actor:
 push dword ptr [ebp-0x14]
 call lc_wait_actor
 add esp,4
 ret
hook_wait_serial:
 push dword ptr [ebp-0x20]
 push ecx
 call lc_wait_serial
 add esp,8
 ret
hook_candidate:
 push dword ptr [ebp-0x14]
 call lc_is_managed
 add esp,4
 test al,al
 jz .stock_candidate
 push dword ptr [ebp-0x50]
 push dword ptr [ebp-0x14]
 call lc_consider
 add esp,8
 test al,al
 jz .candidate_done
 mov byte ptr [ebp-0x35],1
.candidate_done:
 jmp 0x02590429
.stock_candidate:
 mov eax,dword ptr [ebp-0x50]
 mov edx,dword ptr [eax]
 jmp 0x025903F3
hook_availability:
 push dword ptr [esp+4]
 push ecx
 push dword ptr [ebp-0x14]
 call lc_availability
 add esp,12
 ret 4
hook_wait_allowed:
 push dword ptr [esp+4]
 push ecx
 push dword ptr [ebp-0x14]
 call lc_wait_allowed
 add esp,12
 ret 4
hook_door_init:
 push ebp
 mov ebp,esp
 push ebx
 mov ebx,ecx
 push dword ptr [ebp+16]
 push dword ptr [ebp+12]
 push dword ptr [ebp+8]
 call 0x01C3C5F0
 push eax
 push ebx
 call lc_reset_owner
 add esp,4
 pop eax
 pop ebx
 pop ebp
 ret 12
# The original binder frame is extended by eight bytes by the patcher.
# -0x118 holds the previous local flag; -0x11C the previous tracked actor.
.global hook_bind_clear,hook_bind_actor
hook_bind_clear:
 mov dword ptr [eax+0x44],0
 cmp dword ptr [eax],0x04E1649C
 jne .clear_done
 mov edx,dword ptr [lc_active]
 mov dword ptr [ebp-0x118],edx
 mov edx,dword ptr [lc_sub0]
 mov dword ptr [ebp-0x11C],edx
 mov dword ptr [lc_active],0
 mov dword ptr [lc_sub0],0
.clear_done:
 jmp 0x02DF4FAE
hook_bind_actor:
 mov ecx,eax
 call 0x01BEE332
 mov ecx,dword ptr [ebp-8]
 mov dword ptr [ecx+0x44],eax
 cmp dword ptr [ecx],0x04E1649C
 jne .bind_done
 mov dword ptr [lc_sub0],eax
 test eax,eax
 jz .bind_done
 cmp dword ptr [eax+0xE40],2
 je .convert_cpu
 cmp dword ptr [eax+0xE40],1
 jne .bind_done
 cmp dword ptr [ebp-0x118],0
 je .bind_done
 cmp eax,dword ptr [ebp-0x11C]
 jne .bind_done
 cmp eax,dword ptr [ebp-0x20]
 jne .bind_done
 jmp .activate
.convert_cpu:
 push eax
 call lc_forget_all
 pop eax
 push 1
 mov ecx,eax
 call 0x01BB8B60
.activate:
 mov dword ptr [lc_active],1
 call 0x01C9504C
.bind_done:
 jmp 0x02DF5022
.section .note.GNU-stack,"",@progbits
