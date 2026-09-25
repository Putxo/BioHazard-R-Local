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
SECTIONS {
 . = 0x057E2000;
 .lcfix : { *(.text*) *(.rodata*) }
 . = ALIGN(4096);
 .lcdata : { *(.bss*) *(COMMON) *(.data*) }
 /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) }
}
