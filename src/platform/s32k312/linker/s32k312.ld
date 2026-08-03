/*
 * S32K312 Linker Script — yuleDKCS BSW Phase 1 (fixed LMA overlap)
 * 
 * Copyright (c) 2024 YuleTech
 * Platform: NXP S32K312 (Cortex-M33)
 * Features:
 *   - AUTOSAR Memory Partition Support
 *   - MPU Region Configuration
 *   - Safety-Critical Features (Lockstep)
 *   - ECC Protection Support
 * 
 * Memory Map:
 *   Flash: 2MB (0x0000_0000 - 0x001F_FFFF)
 *   SRAM:  512KB (0x2000_0000 - 0x2007_FFFF)
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory Definitions */
MEMORY
{
    /* Flash Memory - 2MB */
    flash_rsvd1   (rx): ORIGIN = 0x00000000, LENGTH = 0x400       /* 1KB - Reserved for bootloader/flash config */
    flash_vector  (rx): ORIGIN = 0x00000400, LENGTH = 0x100       /* 256B - Interrupt vector table */
    flash_text    (rx): ORIGIN = 0x00000500, LENGTH = 0x1F7B00    /* ~2MB - Code and constants */
    flash_rsvd2   (rx): ORIGIN = 0x001F8000, LENGTH = 0x8000      /* 32KB - Reserved for NVM/EEPROM emulation */

    /* RAM - 512KB */
    /* First 64KB: Safe RAM for critical data (ECC protected) */
    ram_safe      (rwx): ORIGIN = 0x20000000, LENGTH = 0x10000    /* 64KB - Safe RAM (Lockstep, ECC) */
    
    /* Next 384KB: General purpose RAM */
    ram_general   (rwx): ORIGIN = 0x20010000, LENGTH = 0x60000    /* 384KB - General RAM */
    
    /* Last 64KB: Shared/uncached RAM */
    ram_shared    (rwx): ORIGIN = 0x20070000, LENGTH = 0x10000    /* 64KB - Shared RAM */

    /* Peripheral space - not used for allocation */
    peripherals   (rw): ORIGIN = 0x40000000, LENGTH = 0x10000000
}

/* Stack and Heap Configuration */
STACK_TOP    = 0x20080000;    /* Top of RAM */
STACK_SIZE   = 0x00020000;    /* 128KB Stack */
HEAP_SIZE    = 0x00040000;    /* 256KB Heap */

/* Region symbols for MPU configuration */
__FLASH_START    = ORIGIN(flash_text);
__FLASH_SIZE     = LENGTH(flash_text);
__RAM_SAFE_START = ORIGIN(ram_safe);
__RAM_SAFE_SIZE  = LENGTH(ram_safe);
__RAM_GP_START   = ORIGIN(ram_general);
__RAM_GP_SIZE    = LENGTH(ram_general);
__RAM_SHARED_START = ORIGIN(ram_shared);
__RAM_SHARED_SIZE = LENGTH(ram_shared);

/* AUTOSAR Memory Partition Symbols */
__OS_STACK_TOP   = STACK_TOP;
__OS_STACK_SIZE  = STACK_SIZE;

/* Section Definitions */
SECTIONS
{
    /* Interrupt Vector Table */
    .vectors :
    {
        __vector_table = ABSOLUTE(.);
        KEEP(*(.isr_vector))
        KEEP(*(.isr_vector.*))
        . = ALIGN(256);  /* Vector table must be 256-byte aligned */
    } > flash_vector

    /* Code Sections */
    .text :
    {
        . = ALIGN(4);
        __text_start = ABSOLUTE(.);
        
        /* AUTOSAR OS Code */
        KEEP(*(.os_startup))
        KEEP(*(.os_text))
        KEEP(*(.os_text.*))
        
        /* MCAL Code */
        KEEP(*(.mcal_text))
        KEEP(*(.mcal_text.*))
        
        /* Safety-Critical Code */
        KEEP(*(.safety_text))
        KEEP(*(.safety_text.*))
        
        /* General Code */
        *(.text)
        *(.text.*)
        *(.glue_7)         /* ARM to Thumb glue */
        *(.glue_7t)        /* Thumb to ARM glue */
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        /* C++ Constructors/Destructors */
        *(.ctors)
        *(.dtors)
        
        . = ALIGN(4);
        __text_end = ABSOLUTE(.);
    } > flash_text

    /* Read-Only Data */
    .rodata :
    {
        . = ALIGN(4);
        __rodata_start = ABSOLUTE(.);
        
        /* AUTOSAR Calibration Data */
        KEEP(*(.calibration))
        KEEP(*(.calibration.*))
        
        /* Safety-Critical Constants */
        KEEP(*(.safety_rodata))
        KEEP(*(.safety_rodata.*))
        
        /* General Constants */
        *(.rodata)
        *(.rodata.*)
        
        . = ALIGN(4);
        __rodata_end = ABSOLUTE(.);
    } > flash_text

    /* ARM Exception Tables */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash_text
    
    __exidx_start = ABSOLUTE(.);
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash_text
    __exidx_end = ABSOLUTE(.);

    /* Safe RAM Section (ECC Protected) — LMA in flash after all flash-only sections */
    .ram_safe_data : AT(ALIGN(__exidx_end, 4))
    {
        . = ALIGN(8);  /* 8-byte alignment for ECC */
        __safe_data_start = ABSOLUTE(.);
        
        /* Safety-Critical Variables */
        KEEP(*(.safety_data))
        KEEP(*(.safety_data.*))
        
        /* ECC Protected Data */
        KEEP(*(.ecc_data))
        KEEP(*(.ecc_data.*))
        
        . = ALIGN(8);
        __safe_data_end = ABSOLUTE(.);
    } > ram_safe
    __safe_data_load = LOADADDR(.ram_safe_data);

    .ram_safe_bss (NOLOAD) :
    {
        . = ALIGN(8);
        __safe_bss_start = ABSOLUTE(.);
        
        KEEP(*(.safety_bss))
        KEEP(*(.safety_bss.*))
        KEEP(*(.ecc_bss))
        KEEP(*(.ecc_bss.*))
        
        . = ALIGN(8);
        __safe_bss_end = ABSOLUTE(.);
    } > ram_safe

    /* Initialized Data Section — LMA contiguous after .ram_safe_data LMA */
    .data : AT(__safe_data_load + SIZEOF(.ram_safe_data))
    {
        . = ALIGN(4);
        __data_start = ABSOLUTE(.);
        
        /* AUTOSAR BSW Data */
        KEEP(*(.bsw_data))
        KEEP(*(.bsw_data.*))
        
        /* RTE Data */
        KEEP(*(.rte_data))
        KEEP(*(.rte_data.*))
        
        /* Application Data */
        *(.data)
        *(.data.*)
        
        . = ALIGN(4);
        __data_end = ABSOLUTE(.);
    } > ram_general
    __data_load = LOADADDR(.data);

    /* BSS Section */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = ABSOLUTE(.);
        
        /* AUTOSAR BSW BSS */
        KEEP(*(.bsw_bss))
        KEEP(*(.bsw_bss.*))
        
        /* RTE BSS */
        KEEP(*(.rte_bss))
        KEEP(*(.rte_bss.*))
        
        /* Application BSS */
        *(.bss)
        *(.bss.*)
        *(COMMON)
        
        . = ALIGN(4);
        __bss_end = ABSOLUTE(.);
    } > ram_general

    /* OS-specific BSS */
    .os_bss (NOLOAD) :
    {
        . = ALIGN(8);
        __os_bss_start = ABSOLUTE(.);
        
        /* OS Task Stacks and Control Blocks */
        KEEP(*(.os_bss))
        KEEP(*(.os_bss.*))
        KEEP(*(.task_stacks))
        KEEP(*(.isr_stacks))
        
        . = ALIGN(8);
        __os_bss_end = ABSOLUTE(.);
    } > ram_general

    /* Heap Section */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start = ABSOLUTE(.);
        . = . + HEAP_SIZE;
        . = ALIGN(8);
        __heap_end = ABSOLUTE(.);
    } > ram_general

    /* Stack Section */
    __stack_bottom = STACK_TOP - STACK_SIZE;
    
    /* Shared RAM Section */
    .shared_ram (NOLOAD) :
    {
        . = ALIGN(4);
        __shared_ram_start = ABSOLUTE(.);
        
        /* Inter-core communication buffers */
        KEEP(*(.shared_data))
        KEEP(*(.shared_data.*))
        
        /* DMA buffers */
        KEEP(*(.dma_buffer))
        KEEP(*(.dma_buffer.*))
        
        . = ALIGN(4);
        __shared_ram_end = ABSOLUTE(.);
    } > ram_shared

    /* Lockstep Test Region */
    .lockstep_test (NOLOAD) :
    {
        . = ALIGN(4);
        __lockstep_test_start = ABSOLUTE(.);
        KEEP(*(.lockstep_test))
        KEEP(*(.lockstep_test.*))
        . = ALIGN(4);
        __lockstep_test_end = ABSOLUTE(.);
    } > ram_safe

    /* Copy Table — LMA at end of .data LMA (avoids LMA overlap) */
    __copy_table_start = ABSOLUTE(.);
    .copy.table : AT(__data_load + SIZEOF(.data))
    {
        . = ALIGN(4);
        LONG(LOADADDR(.ram_safe_data))
        LONG(ADDR(.ram_safe_data))
        LONG(SIZEOF(.ram_safe_data))
        LONG(LOADADDR(.data))
        LONG(ADDR(.data))
        LONG(SIZEOF(.data))
    } > flash_text
    __copy_table_end = ABSOLUTE(.);

    /* Zero Table — LMA after .copy.table */
    __zero_table_start = ABSOLUTE(.);
    .zero.table : AT(LOADADDR(.copy.table) + SIZEOF(.copy.table))
    {
        . = ALIGN(4);
        LONG(ADDR(.ram_safe_bss))
        LONG(SIZEOF(.ram_safe_bss))
        LONG(ADDR(.bss))
        LONG(SIZEOF(.bss))
        LONG(ADDR(.os_bss))
        LONG(SIZEOF(.os_bss))
    } > flash_text
    __zero_table_end = ABSOLUTE(.);

    /* MPU Region Alignment Sections */
    __mpu_text_start = ALIGN(__text_start, 32);
    __mpu_text_end = ALIGN(__text_end, 32);
    __mpu_ram_start = ALIGN(__data_start, 32);
    __mpu_ram_end = ALIGN(__bss_end, 32);

    /* Debug Information */
    .debug_info 0 : { *(.debug_info) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_aranges 0 : { *(.debug_aranges) }
    .debug_line 0 : { *(.debug_line) }
    .debug_str 0 : { *(.debug_str) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_ranges 0 : { *(.debug_ranges) }

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.gnu.build-id)
    }
}

/* Symbol Definitions for Startup and Application */
__end = __bss_end;
end = __end;

/* Stack initialization value */
__stack_top = STACK_TOP;

/* AUTOSAR Memory Partition Extern Declarations */
PROVIDE(__os_stack_bottom = __stack_bottom);
PROVIDE(__os_heap_start = __heap_start);
PROVIDE(__os_heap_end = __heap_end);

/* Safety-related symbols */
PROVIDE(__ecc_protected_start = __safe_data_start);
PROVIDE(__ecc_protected_end = __safe_bss_end);

/* MPU Region Symbols */
PROVIDE(__mpu_region_text_start = __mpu_text_start);
PROVIDE(__mpu_region_text_size = __mpu_text_end - __mpu_text_start);
PROVIDE(__mpu_region_ram_start = __mpu_ram_start);
PROVIDE(__mpu_region_ram_size = __mpu_ram_end - __mpu_ram_start);
PROVIDE(__mpu_region_safe_start = __RAM_SAFE_START);
PROVIDE(__mpu_region_safe_size = __RAM_SAFE_SIZE);
