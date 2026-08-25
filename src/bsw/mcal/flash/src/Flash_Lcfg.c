/************************************************************************************
 * File:       Flash_Lcfg.c
 * Description: AUTOSAR Flash Driver (Fls) Link-Time Configuration
 * Author:      YuleTech AutoSAR Team
 * Version:     1.0.0
 * Date:        2025
 *
 * AUTOSAR Version: 4.4.0
 * Target:        ARM Cortex-M4/M7 (STM32H743)
 ************************************************************************************/

/************************************************************************************
 *                                   Includes
 ************************************************************************************/
#include "Flash.h"
#include "Flash_Cfg.h"

/************************************************************************************
 *                              Sector Configuration
 ************************************************************************************/

/*
 * STM32H743 Flash Memory Map:
 * 
 * Bank 1 (0x0800_0000 - 0x080F_FFFF) - 1MB
static uint32 Fls_GetSectorSize(uint32 SectorIndex);
static sint32 Fls_GetSectorIndexByAddress(uint32 Address);
static const Fls_SectorConfigType* Fls_GetSectorConfig(uint32 SectorIndex);
 *   Sector 0:  0x0800_0000 - 0x0800_3FFF  (16KB)   - Boot sector
 *   Sector 1:  0x0800_4000 - 0x0800_7FFF  (16KB)
 *   Sector 2:  0x0800_8000 - 0x0800_BFFF  (16KB)
 *   Sector 3:  0x0800_C000 - 0x0800_FFFF  (16KB)
 *   Sector 4:  0x0801_0000 - 0x0801_FFFF  (64KB)
 *   Sector 5:  0x0802_0000 - 0x0803_FFFF  (128KB)
 *   Sector 6:  0x0804_0000 - 0x0805_FFFF  (128KB)
 *   Sector 7:  0x0806_0000 - 0x0807_FFFF  (128KB)
 *   ... (continues with 128KB sectors)
 * 
 * Bank 2 (0x0810_0000 - 0x081F_FFFF) - 1MB
 *   Similar structure as Bank 1
 */

/* Sector Configuration Table */
const Fls_SectorConfigType Fls_SectorConfig[FLS_NUM_OF_CONFIGURED_SECTORS] =
{
    /* Bank 1 - Sectors */
    {
        /* Sector 0: 16KB - Boot Sector */
        .StartAddress      = 0x08000000UL,
        .Size              = 0x00004000UL,      /* 16KB */
        .SizeType          = FLS_SECTOR_SIZE_4KB,
        .WriteProtected    = TRUE,               /* Protected boot sector */
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 0U
    },
    {
        /* Sector 1: 16KB */
        .StartAddress      = 0x08004000UL,
        .Size              = 0x00004000UL,      /* 16KB */
        .SizeType          = FLS_SECTOR_SIZE_4KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 1U
    },
    {
        /* Sector 2: 16KB */
        .StartAddress      = 0x08008000UL,
        .Size              = 0x00004000UL,      /* 16KB */
        .SizeType          = FLS_SECTOR_SIZE_4KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 2U
    },
    {
        /* Sector 3: 16KB */
        .StartAddress      = 0x0800C000UL,
        .Size              = 0x00004000UL,      /* 16KB */
        .SizeType          = FLS_SECTOR_SIZE_4KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 3U
    },
    {
        /* Sector 4: 64KB */
        .StartAddress      = 0x08010000UL,
        .Size              = 0x00010000UL,      /* 64KB */
        .SizeType          = FLS_SECTOR_SIZE_64KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 4U
    },
    {
        /* Sector 5: 128KB */
        .StartAddress      = 0x08020000UL,
        .Size              = 0x00020000UL,      /* 128KB */
        .SizeType          = FLS_SECTOR_SIZE_128KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 5U
    },
    {
        /* Sector 6: 128KB */
        .StartAddress      = 0x08040000UL,
        .Size              = 0x00020000UL,      /* 128KB */
        .SizeType          = FLS_SECTOR_SIZE_128KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 6U
    },
    {
        /* Sector 7: 128KB */
        .StartAddress      = 0x08060000UL,
        .Size              = 0x00020000UL,      /* 128KB */
        .SizeType          = FLS_SECTOR_SIZE_128KB,
        .WriteProtected    = FALSE,
        .ReadProtected     = FALSE,
        .Bank              = 0U,
        .SectorNumber      = 7U
    }
};

/************************************************************************************
 *                              Timing Configuration
 ************************************************************************************/

const Fls_TimingConfigType Fls_TimingConfig =
{
    /* Erase Timing (in microseconds) */
    .EraseTime4KB       = FLS_ERASE_4KB_TIME_TYPICAL,
    .EraseTime32KB      = FLS_ERASE_32KB_TIME_TYPICAL,
    .EraseTime64KB      = FLS_ERASE_64KB_TIME_TYPICAL,
    .EraseTime128KB     = FLS_ERASE_128KB_TIME_TYPICAL,
    
    /* Programming Timing (in microseconds) */
    .ProgramByteTime    = FLS_PROGRAM_BYTE_TIME_TYPICAL,
    .ProgramWordTime    = FLS_PROGRAM_WORD_TIME_TYPICAL,
    
    /* Timeout Configuration */
    .MaxEraseTimeout    = FLS_MAX_ERASE_TIMEOUT_US,
    .MaxProgramTimeout  = FLS_MAX_PROGRAM_TIMEOUT_US,
    
    /* Flash Wait States - Based on CPU frequency */
    /* For 480MHz CPU with VOS1: 4 wait states recommended */
    .WaitStates         = 4U
};

/************************************************************************************
 *                            Protection Configuration
 ************************************************************************************/

const Fls_ProtectionConfigType Fls_ProtectionConfig =
{
    /* Read Protection Level */
    /* FLS_RDP_LEVEL_NONE = 0xAA (No protection) */
    /* FLS_RDP_LEVEL_1    = 0x00 (Read protection) */
    /* FLS_RDP_LEVEL_2    = 0xCC (Full protection) */
    .ReadProtectionLevel = FLS_RDP_LEVEL_1,
    
    /* Write Protection Mask */
    /* Bit 0 = Sector 0, Bit 1 = Sector 1, etc. */
    /* 0x01 = Protect Sector 0 only (boot sector) */
    .WriteProtectionMask = FLS_WRP_SECTOR_0,
    
    /* PCROP Configuration */
    .PcropEnabled        = FALSE,
    .PcropStartAddress   = 0x00000000UL,
    .PcropEndAddress     = 0x00000000UL
};

/************************************************************************************
 *                             General Configuration
 ************************************************************************************/

const Fls_GeneralConfigType Fls_GeneralConfig =
{
    /* Interrupt Configuration */
    .UseInterrupts          = FLS_USE_INTERRUPTS,
    
    /* Performance Features */
    .PrefetchEnable         = TRUE,
    .InstructionCacheEnable = TRUE,
    .DataCacheEnable        = TRUE,
    
    /* Flash Latency (Wait States) */
    .FlashLatency           = 4U
};

/************************************************************************************
 *                           Sector Info for Flash API
 ************************************************************************************/

static const Fls_SectorInfoType Fls_SectorInfoTable[FLS_NUM_OF_CONFIGURED_SECTORS] =
{
    { 0x08000000UL, 0x00004000UL, FLS_SECTOR_SIZE_4KB,   TRUE,  0U },
    { 0x08004000UL, 0x00004000UL, FLS_SECTOR_SIZE_4KB,   FALSE, 0U },
    { 0x08008000UL, 0x00004000UL, FLS_SECTOR_SIZE_4KB,   FALSE, 0U },
    { 0x0800C000UL, 0x00004000UL, FLS_SECTOR_SIZE_4KB,   FALSE, 0U },
    { 0x08010000UL, 0x00010000UL, FLS_SECTOR_SIZE_64KB,  FALSE, 0U },
    { 0x08020000UL, 0x00020000UL, FLS_SECTOR_SIZE_128KB, FALSE, 0U },
    { 0x08040000UL, 0x00020000UL, FLS_SECTOR_SIZE_128KB, FALSE, 0U },
    { 0x08060000UL, 0x00020000UL, FLS_SECTOR_SIZE_128KB, FALSE, 0U }
};

/************************************************************************************
 *                           Notification Callbacks
 ************************************************************************************/

/* Job End Notification Callback */
/** @req SWS_Fls_00031 */
static void Fls_JobEndNotification(void)
{
    /* User-defined notification function */
    /* Called when a flash operation completes successfully */
    /* Can be used to trigger further operations or signal completion */
}

/* Job Error Notification Callback */
/** @req SWS_Fls_00032 */
static void Fls_JobErrorNotification(void)
{
    /* User-defined error notification function */
    /* Called when a flash operation fails */
    /* Can be used for error logging or recovery actions */
}

/************************************************************************************
 *                           Main Configuration Structure
 ************************************************************************************/

const Fls_ConfigType Fls_Config =
{
    /* Flash Memory Base Address */
    #ifdef S32K312
#include "S32K312.h"
    .BaseAddress            = S32K312_FLASH_BASE_ALIAS,
#else
    .BaseAddress            = 0x08000000UL,
#endif
    
    /* Total Flash Size */
    .TotalSize              = FLS_TOTAL_SIZE,
    
    /* Sector Configuration Table */
    .SectorInfo             = Fls_SectorInfoTable,
    .SectorCount            = FLS_NUM_OF_CONFIGURED_SECTORS,
    
    /* Page and Programming Unit Size */
    .PageSize               = 0x00000020UL,     /* 32 bytes - flash programming page */
    .ProgrammingUnit        = 0x00000004UL,     /* 4 bytes - word programming */
    
    /* Read Performance Limits (bytes/second) */
    .MaxReadFastMode        = 96000000UL,       /* ~96 MB/s at 480MHz with caches */
    .MaxReadNormalMode      = 48000000UL,       /* ~48 MB/s without optimization */
    
    /* Write Performance Limits (bytes/second) */
    .MaxWriteFastMode       = 125000UL,         /* Based on flash programming spec */
    .MaxWriteNormalMode     = 100000UL,
    
    /* Default Access Mode */
    .DefaultMode            = MEMIF_MODE_FAST,
    
    /* Call Cycle (Flash Latency) */
    .CallCycle              = 4U,
    
    /* Interrupt Configuration */
    .UseInterrupts          = FLS_USE_INTERRUPTS,
    
    /* Notification Callbacks */
    .JobEndNotification     = Fls_JobEndNotification,
    .JobErrorNotification   = Fls_JobErrorNotification
};

/************************************************************************************
 *                         Extended Sector Map for Reference
 ************************************************************************************/

/*
 * Full Bank 1 Sector Map (Reference Only):
 * 
 * Sector 0:   0x0800_0000 - 0x0800_3FFF   (16KB)
 * Sector 1:   0x0800_4000 - 0x0800_7FFF   (16KB)
 * Sector 2:   0x0800_8000 - 0x0800_BFFF   (16KB)
 * Sector 3:   0x0800_C000 - 0x0800_FFFF   (16KB)
 * Sector 4:   0x0801_0000 - 0x0801_FFFF   (64KB)
 * Sector 5:   0x0802_0000 - 0x0803_FFFF   (128KB)
 * Sector 6:   0x0804_0000 - 0x0805_FFFF   (128KB)
 * Sector 7:   0x0806_0000 - 0x0807_FFFF   (128KB)
 * Sector 8:   0x0808_0000 - 0x0809_FFFF   (128KB)
 * Sector 9:   0x080A_0000 - 0x080B_FFFF   (128KB)
 * Sector 10:  0x080C_0000 - 0x080D_FFFF   (128KB)
 * Sector 11:  0x080E_0000 - 0x080F_FFFF   (128KB)
 *
 * Full Bank 2 Sector Map:
 * 
 * Sector 12:  0x0810_0000 - 0x0810_3FFF   (16KB)
 * Sector 13:  0x0810_4000 - 0x0810_7FFF   (16KB)
 * Sector 14:  0x0810_8000 - 0x0810_BFFF   (16KB)
 * Sector 15:  0x0810_C000 - 0x0810_FFFF   (16KB)
 * Sector 16:  0x0811_0000 - 0x0811_FFFF   (64KB)
 * Sector 17:  0x0812_0000 - 0x0813_FFFF   (128KB)
 * ... (continues with 128KB sectors)
 */

/************************************************************************************
 *                          Helper Functions (Optional)
 ************************************************************************************/

/* Get Sector Configuration by Index */
/** @req SWS_Fls_00033 */
static const Fls_SectorConfigType* Fls_GetSectorConfig(uint32 SectorIndex)
{
    const Fls_SectorConfigType* SectorConfigPtr = NULL_PTR;
    
    if (SectorIndex < FLS_NUM_OF_CONFIGURED_SECTORS)
    {
        SectorConfigPtr = &Fls_SectorConfig[SectorIndex];
    }
    
    return SectorConfigPtr;
}

/* Get Sector Index by Address */
/** @req SWS_Fls_00034 */
static sint32 Fls_GetSectorIndexByAddress(uint32 Address)
{
    sint32 SectorIndex = -1;
    uint32 i;
    
    for (i = 0U; i < FLS_NUM_OF_CONFIGURED_SECTORS; i++)
    {
        if ((Address >= Fls_SectorConfig[i].StartAddress) &&
            (Address < (Fls_SectorConfig[i].StartAddress + Fls_SectorConfig[i].Size)))
        {
            SectorIndex = (sint32)i;
            break;
        }
    }
    
    return SectorIndex;
}

/* Get Sector Size by Index */
/** @req SWS_Fls_00035 */
static uint32 Fls_GetSectorSize(uint32 SectorIndex)
{
    uint32 SectorSize = 0U;
    
    if (SectorIndex < FLS_NUM_OF_CONFIGURED_SECTORS)
    {
        SectorSize = Fls_SectorConfig[SectorIndex].Size;
    }
    
    return SectorSize;
}
