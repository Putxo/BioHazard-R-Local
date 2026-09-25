.intel_syntax noprefix
.code32
.bss
.global abi_kind,abi_self,abi_arg,abi_extra,abi_bad_alignment
abi_kind: .skip 4
abi_self: .skip 4
abi_arg: .skip 4
abi_extra: .skip 4
abi_bad_alignment: .skip 4
.text
.macro alignment
 lea eax,[esp+4]
 and eax,15
 or dword ptr [abi_bad_alignment],eax
.endm
.global abi_global
abi_global:
 alignment
 mov dword ptr [abi_kind],0
 mov eax,0x13579BDF
 mov ecx,0xABABABAB
 mov edx,0xCDCDCDCD
 ret
.global abi_allocate
abi_allocate:
 alignment
 mov dword ptr [abi_kind],2
 mov eax,[esp+4]
 mov [abi_arg],eax
 mov ecx,[esp+8]
 mov [abi_extra],ecx
 xor eax,ecx
 mov ecx,0xABABABAB
 mov edx,0xCDCDCDCD
 ret
.global abi_member0
abi_member0:
 alignment
 mov dword ptr [abi_kind],3
 mov [abi_self],ecx
 mov eax,ecx
 mov ecx,0xABABABAB
 mov edx,0xCDCDCDCD
 ret
.global abi_member1
abi_member1:
 alignment
 mov dword ptr [abi_kind],4
 mov [abi_self],ecx
 mov eax,[esp+4]
 mov [abi_arg],eax
 xor eax,ecx
 mov ecx,0xABABABAB
 mov edx,0xCDCDCDCD
 ret 4
.global _start
_start:
 and esp,-16
 mov ebx,0x11223344
 mov esi,0x55667788
 mov edi,0x1234FEDC
 mov ebp,0x76543210
 mov dword ptr [start_sp],esp
 call january_abi_cases
 test eax,eax
 jne failed
 cmp ebx,0x11223344
 jne failed
 cmp esi,0x55667788
 jne failed
 cmp edi,0x1234FEDC
 jne failed
 cmp ebp,0x76543210
 jne failed
 cmp esp,dword ptr [start_sp]
 jne failed
 mov eax,4
 mov ebx,1
 lea ecx,[report]
 mov edx,report_end-report
 int 0x80
 xor ebx,ebx
 jmp done
failed:
 mov ebx,1
done:
 mov eax,1
 int 0x80
.bss
start_sp: .skip 4
.section .rodata
report: .ascii "{\"status\":\"PASS\",\"abi_invocations\":30,\"native_i386\":true,\"exact_production_invoker_table\":true,\"synthetic_callees\":true,\"gameplay_executed\":false}\n"
report_end:
.section .note.GNU-stack,"",@progbits
