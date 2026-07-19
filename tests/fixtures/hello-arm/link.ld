/* Linker script for ARM Cortex-M3 (QEMU lm3s6965evb) SIL test */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    /* Interrupt vector table at start of flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code */
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Initialized data (copied from flash to RAM at startup) */
    _sidata = .;
    .data : AT(_sidata)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* Uninitialized data (zero-filled at startup) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Stack */
    ._stack :
    {
        . = ALIGN(8);
        . = . + 8K;
        _estack = .;
    } > RAM

    /* Discard unused sections */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.attributes*)
        *(.comment*)
        *(.note*)
    }
}

PROVIDE(__stack = _estack);
