ENTRY(_start)

SECTIONS
{
  . = 0x80000;
  .text : { *(.text) }
  /*
  . = 0x100000;
  .data : { *(.data) }
  .bss : { *(.bss) }
  */
}
