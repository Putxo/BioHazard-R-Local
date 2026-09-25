OUTPUT_FORMAT(elf32-i386)
ENTRY(hook_begin)
lc_active = 0x057D9188;
lc_sub0 = 0x057D9184;
api_self = 0x01C16468;
api_find = 0x01C07341;
api_local_serial = 0x01C85962;
api_allowed = 0x01BDD159;
api_kind_of = 0x01C75C7E;
api_door_dti = 0x05581EA8;
api_set_group = 0x01C7F26A;
api_static_invoke = 0x01B993F0;
api_static_manage = 0x01C3D063;
api_charge_zero = 0x01BCB4D6;
api_quint_zero = 0x01C92CAC;
api_ac_charge_zero = 0x01C7D7D5;
api_foot_zero = 0x01C2543B;
SECTIONS {
 . = 0x057E2000;
 .lcfix : { *(.text*) *(.rodata*) }
 . = ALIGN(4096);
 .lcdata : { *(.bss*) *(COMMON) *(.data*) }
 /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) }
}
