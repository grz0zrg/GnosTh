ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .text : { *(.text) }
  .data : { *(.data) }
  /DISCARD/ : {
    *(.ARM.exidx*)
    *(.ARM.extab*)
  }
}

