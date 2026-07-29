/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*******************************************************************************
 * Linker_Cfg.h
 *
 * Linker configuration and memory map definitions for S32K312
 *
 * AUTOSAR Partition: Memory Mapping Configuration
 * Platform: NXP S32K312 (Cortex-M7)
 * Safety Features: ECC, Lockstep, MPU
 ******************************************************************************/

#ifndef LINKER_CFG_H
#define LINKER_CFG_H

/*******************************************************************************
 * Memory Region Definitions
 ******************************************************************************/

/* Flash Memory Layout */
#define FLASH_BASE_ADDR         0x00000000UL
#define FLASH_SIZE              (2UL * 1024UL * 1024UL)    /* 2 MB */
#define FLASH_VECTOR_TABLE_ADDR 0x00000400UL
#define FLASH_VECTOR_TABLE_SIZE 0x00000100UL               /* 256 bytes */
#define FLASH_CODE_START        0x00000500UL
#define FLASH_CODE_SIZE         (FLASH_SIZE - FLASH_CODE_START)

/* RAM Memory Layout */
#define RAM_BASE_ADDR           0x20000000UL
#define RAM_SIZE                (512UL * 1024UL)           /* 512 KB */

/* Safe RAM (ECC Protected, Lockstep Monitored) */
#define RAM_SAFE_START          0x20000000UL
#define RAM_SAFE_SIZE           (64UL * 1024UL)            /* 64 KB */
#define RAM_SAFE_END            (RAM_SAFE_START + RAM_SAFE_SIZE)

/* General Purpose RAM */
#define RAM_GENERAL_START       0x20010000UL
#define RAM_GENERAL_SIZE        (384UL * 1024UL)           /* 384 KB */
#define RAM_GENERAL_END         (RAM_GENERAL_START + RAM_GENERAL_SIZE)

/* Shared RAM */
#define RAM_SHARED_START        0x20070000UL
#define RAM_SHARED_SIZE         (64UL * 1024UL)            /* 64 KB */
#define RAM_SHARED_END          (RAM_SHARED_START + RAM_SHARED_SIZE)

#define RAM_END_ADDR            0x20080000UL

/*******************************************************************************
 * Stack and Heap Configuration
 ******************************************************************************/

#define STACK_TOP               RAM_END_ADDR
#define STACK_SIZE              (128UL * 1024UL)           /* 128 KB */
#define STACK_BOTTOM            (STACK_TOP - STACK_SIZE)

#define HEAP_SIZE               (256UL * 1024UL)           /* 256 KB */

/*******************************************************************************
 * AUTOSAR Memory Partition IDs
 ******************************************************************************/

typedef enum
{
    MEM_PARTITION_OS        = 0,    /* Operating System */
    MEM_PARTITION_SAFE      = 1,    /* Safety-Critical Applications */
    MEM_PARTITION_QM        = 2,    /* QM (Quality Managed) Applications */
    MEM_PARTITION_SHARED    = 3,    /* Shared Resources */
    MEM_PARTITION_NUM       = 4
} MemPartitionType;

/*******************************************************************************
 * MPU Region Configuration
 ******************************************************************************/

/* MPU Region Count */
#define MPU_REGION_COUNT        8

/* MPU Region Definitions */
typedef enum
{
    MPU_REGION_FLASH_CODE   = 0,    /* Flash - Execute Only (Privileged) */
    MPU_REGION_FLASH_RODATA = 1,    /* Flash - Read Only */
    MPU_REGION_RAM_SAFE     = 2,    /* Safe RAM - Full Access (ECC Protected) */
    MPU_REGION_RAM_GENERAL  = 3,    /* General RAM - Full Access */
    MPU_REGION_RAM_SHARED   = 4,    /* Shared RAM - Full Access */
    MPU_REGION_STACK        = 5,    /* Stack - Read/Write Only */
    MPU_REGION_PERIPHERALS  = 6,    /* Peripherals - Device Type */
    MPU_REGION_OS           = 7     /* OS Reserved */
} MpuRegionType;

/* MPU Access Permissions */
#define MPU_AP_NO_ACCESS        0x0
#define MPU_AP_PRIV_RW          0x1
#define MPU_AP_PRIV_RW_USER_RO  0x2
#define MPU_AP_FULL_ACCESS      0x3
#define MPU_AP_PRIV_RO          0x5
#define MPU_AP_RO               0x6

/* MPU Region Sizes (log2(size) - 1) */
#define MPU_SIZE_32B            0x04
#define MPU_SIZE_64B            0x05
#define MPU_SIZE_128B           0x06
#define MPU_SIZE_256B           0x07
#define MPU_SIZE_512B           0x08
#define MPU_SIZE_1KB            0x09
#define MPU_SIZE_2KB            0x0A
#define MPU_SIZE_4KB            0x0B
#define MPU_SIZE_8KB            0x0C
#define MPU_SIZE_16KB           0x0D
#define MPU_SIZE_32KB           0x0E
#define MPU_SIZE_64KB           0x0F
#define MPU_SIZE_128KB          0x10
#define MPU_SIZE_256KB          0x11
#define MPU_SIZE_512KB          0x12
#define MPU_SIZE_1MB            0x13
#define MPU_SIZE_2MB            0x14
#define MPU_SIZE_4MB            0x15

/*******************************************************************************
 * Safety-Related Memory Attributes
 ******************************************************************************/

/* ECC Protection Attributes */
#define MEM_ATTR_ECC_ENABLED    0x01
#define MEM_ATTR_LOCKSTEP       0x02
#define MEM_ATTR_MPU_PROTECTED  0x04

/* Memory Section Attributes */
#define SEC_ATTR_SAFE           (MEM_ATTR_ECC_ENABLED | MEM_ATTR_LOCKSTEP)
#define SEC_ATTR_QM             0x00
#define SEC_ATTR_SHARED         MEM_ATTR_MPU_PROTECTED

/*******************************************************************************
 * Linker Symbol Extern Declarations
 * These symbols are defined in the linker script
 ******************************************************************************/

/* Code Section */
extern uint32_t __text_start;
extern uint32_t __text_end;

/* Read-Only Data */
extern uint32_t __rodata_start;
extern uint32_t __rodata_end;

/* Initialized Data */
extern uint32_t __data_start;
extern uint32_t __data_end;
extern uint32_t __data_load;

/* BSS */
extern uint32_t __bss_start;
extern uint32_t __bss_end;

/* Safe RAM (ECC Protected) */
extern uint32_t __safe_data_start;
extern uint32_t __safe_data_end;
extern uint32_t __safe_data_load;
extern uint32_t __safe_bss_start;
extern uint32_t __safe_bss_end;

/* OS BSS */
extern uint32_t __os_bss_start;
extern uint32_t __os_bss_end;

/* Heap */
extern uint32_t __heap_start;
extern uint32_t __heap_end;

/* Stack */
extern uint32_t __stack_top;
extern uint32_t __stack_bottom;

/* Vector Table */
extern uint32_t __vector_table;

/* Copy and Zero Tables */
extern uint32_t __copy_table_start;
extern uint32_t __copy_table_end;
extern uint32_t __zero_table_start;
extern uint32_t __zero_table_end;

/* MPU Region Boundaries */
extern uint32_t __mpu_region_text_start;
extern uint32_t __mpu_region_text_size;
extern uint32_t __mpu_region_ram_start;
extern uint32_t __mpu_region_ram_size;
extern uint32_t __mpu_region_safe_start;
extern uint32_t __mpu_region_safe_size;

/*******************************************************************************
 * Memory Macros
 ******************************************************************************/

/* Get section start address */
#define SECTION_START(sym)      ((uint32_t)&(sym))

/* Get section end address */
#define SECTION_END(sym)        ((uint32_t)&(sym))

/* Get section size */
#define SECTION_SIZE(start, end) ((uint32_t)&(end) - (uint32_t)&(start))

/* Get section load address */
#define LOAD_ADDR(sym)          ((uint32_t)&(sym))

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/* Get total RAM usage */
static inline uint32_t Linker_GetRamUsage(void)
{
    return (uint32_t)&__bss_end - RAM_BASE_ADDR;
}

/* Get total Flash usage */
static inline uint32_t Linker_GetFlashUsage(void)
{
    return (uint32_t)&__rodata_end - FLASH_BASE_ADDR;
}

/* Check if address is in safe RAM */
static inline uint8_t Linker_IsSafeRamAddr(uint32_t addr)
{
    return (addr >= RAM_SAFE_START) && (addr < RAM_SAFE_END);
}

/* Check if address is in general RAM */
static inline uint8_t Linker_IsGeneralRamAddr(uint32_t addr)
{
    return (addr >= RAM_GENERAL_START) && (addr < RAM_GENERAL_END);
}

/* Check if address is in shared RAM */
static inline uint8_t Linker_IsSharedRamAddr(uint32_t addr)
{
    return (addr >= RAM_SHARED_START) && (addr < RAM_SHARED_END);
}

#endif /* LINKER_CFG_H */
