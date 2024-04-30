OUTPUT_ARCH(riscv)
ENTRY(main)

TEXT_ADDRESS = 0x80000000;
DATA_ADDRESS = 0x80400000;

SECTIONS
{
    . = TEXT_ADDRESS;
    .text : {
        *(.text.entry)
        *(.text .text.*)
    }
    . = DATA_ADDRESS;
    .rodata : {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    }
    .data : {
        __global_pointer$ = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
    }
    .bss : {
        start_bss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        end_bss = .;
    }
    /DISCARD/ : {
        *(.eh_frame)
        *(.debug*)
    }
}
