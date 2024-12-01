ENTRY(_start)

MEMORY
{
    ram : ORIGIN = 0x80000, LENGTH = 0x1000000
}
SECTIONS
{
    .text : { *(.text) } > ram
    .data : { *(.data) } > ram
    .bss : { *(.bss) } > ram
}
