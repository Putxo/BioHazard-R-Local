OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
PHDRS {
  fixtures PT_LOAD FLAGS(6);
  code PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}
SECTIONS {
  .fixtures 0x057D9000 : { *(.fixtures) } :fixtures
  .text 0x08048000 : { *(.text*) *(.rodata*) } :code
  . = ALIGN(4096);
  .data : { *(.data*) *(.bss*) *(COMMON) } :data
  /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
ASSERT(fixture_sub == 0x057D9184, "tracker address mismatch")
ASSERT(fixture_active == 0x057D9188, "local flag address mismatch")
