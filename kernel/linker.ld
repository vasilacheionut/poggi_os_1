OUTPUT_FORMAT("binary")
ENTRY(kernel_main)

SECTIONS
{
    /* Kernelul se încarcă la adresa fizică 0x9000 */
    . = 0x9000;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}