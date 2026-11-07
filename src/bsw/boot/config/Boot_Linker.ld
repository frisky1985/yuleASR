/*
 * Boot_Linker.ld — Primary Bootloader (PBL) Linker Script
 * Platform: NXP S32K312 (Cortex-M33)
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

    /* App<->Bootloader 握手块 (NOINIT, 跨复位保持)
     * 地址必须与 s32k312.ld 的 ram_handshake 一致 — 两侧链接脚本共同约定
     * 同一物理地址, C 代码经 __boot_handshake_start 符号访问, 无硬编码。
     * 选位约束: 避开栈区 (0x20060000-0x20080000) / App BSS 清零范围 /
     * PBL ram_pbl (0x20000000-0x20000FFF); 0x2000FFE0 位于 ram_safe 顶部。 */
    ram_handshake (rw): ORIGIN = 0x2000FFE0, LENGTH = 0x20     /* 32B Boot HandShake */
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

    /* App<->Bootloader 握手块 — NOLOAD, PBL 启动不清零;
     * 导出 __boot_handshake_start 供 Boot_Handshake.h 引用 */
    .boot_handshake (NOLOAD) :
    {
        . = ALIGN(4);
        __boot_handshake_start = ABSOLUTE(.);
        KEEP(*(.boot_handshake))
        KEEP(*(.boot_handshake.*))
        . = . + 0x20;   /* 强制保留 32 字节, 即使无输入段也占位 */
        __boot_handshake_end = ABSOLUTE(.);
    } > ram_handshake

    ASSERT((__boot_handshake_end - __boot_handshake_start) == 0x20,
           "boot_handshake region must be 32 bytes")
}

/* Export symbols for startup code */
__StackTop     = __boot_stack_top;
__StackLimit   = __boot_stack_top - 0x400;
__bss_start__  = __boot_bss_start;
__bss_end__    = __boot_bss_end;
