/*
 * Boot_Linker.ld — Primary Bootloader (PBL) Linker Script
 * Platform: NXP S32K312 (Cortex-M7)
 * Memory:   PBL fits in 4KB flash_rsvd1 + flash_vector
 *
 * Included by the main linker script via INCLUDE when building PBL.
 */

MEMORY
{
    flash_pbl     (rx): ORIGIN = 0x00000000, LENGTH = 0x400    /* 1KB */
    flash_vector  (rx): ORIGIN = 0x00000400, LENGTH = 0x100    /* 256B vector table */
    flash_pbl_ro  (rx): ORIGIN = 0x00000500, LENGTH = 0xB00    /* 2.75KB code + rodata */
    ram_pbl       (rw): ORIGIN = 0x20000000, LENGTH = 0x1000   /* 4KB safe RAM */
}

SECTIONS
{
    .vectors :
    {
        KEEP(*(.boot_vectors))
        . = ALIGN(4);
    } > flash_vector

    .boot_text :
    {
        . = ALIGN(4);
        __boot_text_start = .;
        KEEP(*(.boot_text))
        KEEP(*(.boot_text.*))
        *(.text)
        *(.text.*)
        . = ALIGN(4);
        __boot_text_end = .;
    } > flash_pbl_ro

    .boot_rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
        /* Public keys for signature verification */
        KEEP(*(.boot_pubkey))
        . = ALIGN(4);
    } > flash_pbl_ro

    .boot_bss (NOLOAD) :
    {
        . = ALIGN(4);
        __boot_bss_start = .;
        *(.boot_bss)
        *(.boot_bss.*)
        *(.bss)
        *(COMMON)
        . = ALIGN(4);
        __boot_bss_end = .;
    } > ram_pbl

    .boot_stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + 0x400;  /* 1KB stack */
        __boot_stack_top = .;
    } > ram_pbl
}

/* Export symbols for startup code */
__StackTop     = __boot_stack_top;
__StackLimit   = __boot_stack_top - 0x400;
__bss_start__  = __boot_bss_start;
__bss_end__    = __boot_bss_end;
