/*=============================================================================
 * link.ld -- QEMU Cortex-M7 (MPS2 AN500) Linker Script
 *
 * Target:  mps2-an500 (Cortex-M7)
 * Flash:  0x00000000 - 0x003FFFFF  (4 MB)
 * SRAM:   0x20000000 - 0x203FFFFF  (4 MB)
 *
 * This script replaces the real S32K312 linker script when
 * building for QEMU simulation.  Entry point is Reset_Handler.
 *============================================================================*/

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    SRAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

/* Stack and heap sizes */
__stack_size   = DEFINED(__stack_size)   ? __stack_size   : 0x4000;
__heap_size    = DEFINED(__heap_size)    ? __heap_size    : 0x4000;

SECTIONS
{
    /* Vector table -- must be first in flash */
    .vector_table :
    {
        . = ALIGN(4);
        KEEP(*(.vector_table))
        . = ALIGN(4);
    } > FLASH

    /* Code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Initialization data values (copied to SRAM at startup) */
    .data_init : AT (_etext)
    {
        . = ALIGN(4);
        __data_init_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_init_end = .;
    } > SRAM

    /* Initial data load address in FLASH */
    __data_load_start = LOADADDR(.data_init);

    /* BSS -- zero-initialized */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > SRAM

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + __heap_size;
        . = ALIGN(8);
        __heap_end__ = .;
    } > SRAM

    /* Stack */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . = . + __stack_size;
        . = ALIGN(8);
        __stack_end__ = .;
        _estack = .;
    } > SRAM

    /* Discard unwind tables (not useful on bare-metal) */
    /DISCARD/ :
    {
        *(.ARM.exidx)
        *(.ARM.extab)
        *(.note.gnu.build-id)
        *(.comment)
        *(.debug*)
    }
}

/* Heap symbols for _sbrk */
PROVIDE(__HeapBase = __heap_start__);
PROVIDE(__HeapLimit = __heap_end__);
