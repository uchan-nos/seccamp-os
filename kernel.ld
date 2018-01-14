OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(MyMain)

/*SEARCH_DIR(/home/uchan/x86_64-none-elf/x86_64-none-elf/lib)*/
SEARCH_DIR(/home/uchan/x86_64-pc-linux-gnu/lib)

SECTIONS
{
    . = SEGMENT_START("text-segment", 0x00100000) + SIZEOF_HEADERS;
    /*. = SEGMENT_START("text-segment", 0) + SIZEOF_HEADERS;*/
    .text : { *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }
    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) }
    .ctors : { *(.ctors) *(.ctors.*) }
}
