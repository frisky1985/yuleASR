/************************************************************************************
 * File:       Flash_Cfg.h
 * Description: AUTOSAR Flash Driver (Fls) Configuration Header
 * Author:      YuleTech AutoSAR Team
 * Version:     1.0.0
 * Date:        2025
 *
 * AUTOSAR Version: 4.4.0
 * Target:        ARM Cortex-M4/M7 (STM32H743)
 ************************************************************************************/

#ifndef FLASH_CFG_H
#define FLASH_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************
 *                              Version Information
 ************************************************************************************/
#define FLS_CFG_VENDOR_ID               (uint16)0x0001U
#define FLS_CFG_MODULE_ID               (uint16)0x005CU

#define FLS_CFG_SW_MAJOR_VERSION        (uint8)1U
#define FLS_CFG_SW_MINOR_VERSION        (uint8)0U
#define FLS_CFG_SW_PATCH_VERSION        (uint8)0U

#define FLS_CFG_AR_MAJOR_VERSION        (uint8)4U
#define FLS_CFG_AR_MINOR_VERSION        (uint8)4U
#define FLS_CFG_AR_PATCH_VERSION        (uint8)0U

/************************************************************************************
 *                          Pre-compile Configurations
 ************************************************************************************/

/* Development Error Detection */
#define FLS_DEV_ERROR_DETECT            STD_ON

/* Version Info API */
#define FLS_VERSION_INFO_API            STD_ON

/* Cancel Operation API */
#define FLS_CANCEL_API                  STD_ON

/* Set Mode API */
#define FLS_SET_MODE_API                STD_ON

/* Compare API */
#define FLS_COMPARE_API                 STD_ON

/* Blank Check API */
#define FLS_BLANK_CHECK_API             STD_ON

/* Timeout Supervision */
#define FLS_TIMEOUT_SUPERVISION_ENABLED STD_ON

/* Use Interrupts for Flash Operations */
#define FLS_USE_INTERRUPTS              STD_OFF

/* Runtime Configuration */
#define FLS_RUNTIME_CONFIG_API          STD_OFF

/* Number of Configured Sectors */
#define FLS_NUM_OF_CONFIGURED_SECTORS   (8U)

/* Flash Bank Configuration */
#define FLS_NUM_OF_FLASH_BANKS          (2U)
#define FLS_BANK1_BASE_ADDRESS          (0x08000000UL)
#define FLS_BANK2_BASE_ADDRESS          (0x08100000UL)

/* Flash Size Configuration */
#define FLS_TOTAL_SIZE                  (0x00200000UL)  /* 2MB Total */
#define FLS_BANK_SIZE                   (0x00100000UL)  /* 1MB per bank */

/************************************************************************************
 *                          Sector Size Definitions
 ************************************************************************************/
#define FLS_SECTOR_SIZE_4KB_VALUE       (0x00001000UL)  /* 4KB */
#define FLS_SECTOR_SIZE_32KB_VALUE      (0x00008000UL)  /* 32KB */
#define FLS_SECTOR_SIZE_64KB_VALUE      (0x00010000UL)  /* 64KB */
#define FLS_SECTOR_SIZE_128KB_VALUE     (0x00020000UL)  /* 128KB */

/************************************************************************************
 *                          Timing Configuration
 ************************************************************************************/

/* Erase Timing (in microseconds) */
#define FLS_ERASE_4KB_TIME_TYPICAL      (125000U)   /* 125ms typical */
#define FLS_ERASE_4KB_TIME_MAX          (400000U)   /* 400ms maximum */
#define FLS_ERASE_32KB_TIME_TYPICAL     (500000U)   /* 500ms typical */
#define FLS_ERASE_32KB_TIME_MAX         (1600000U)  /* 1.6s maximum */
#define FLS_ERASE_64KB_TIME_TYPICAL     (1000000U)  /* 1s typical */
#define FLS_ERASE_64KB_TIME_MAX         (3200000U)  /* 3.2s maximum */
#define FLS_ERASE_128KB_TIME_TYPICAL    (2000000U)  /* 2s typical */
#define FLS_ERASE_128KB_TIME_MAX        (6400000U)  /* 6.4s maximum */

/* Programming Timing */
#define FLS_PROGRAM_BYTE_TIME_TYPICAL   (16U)       /* 16us typical */
#define FLS_PROGRAM_BYTE_TIME_MAX       (100U)      /* 100us maximum */
#define FLS_PROGRAM_WORD_TIME_TYPICAL   (32U)       /* 32us typical */
#define FLS_PROGRAM_WORD_TIME_MAX       (200U)      /* 200us maximum */

/* Maximum Wait Timeouts */
#define FLS_MAX_ERASE_TIMEOUT_US        (10000000U) /* 10s maximum */
#define FLS_MAX_PROGRAM_TIMEOUT_US      (100000U)   /* 100ms maximum */
#define FLS_MAX_WAIT_STATES             (15U)

/************************************************************************************
 *                          Flash Controller Registers
 ************************************************************************************/

/* Flash Access Control Register (FLASH_ACR) */
#define FLS_ACR_LATENCY_POS             (0U)
#define FLS_ACR_LATENCY_MASK            (0x0000000FUL)
#define FLS_ACR_PRFTEN                  (0x00000100UL)
#define FLS_ACR_ICEN                    (0x00000200UL)
#define FLS_ACR_DCEN                    (0x00000400UL)
#define FLS_ACR_RUN_PD                  (0x00000800UL)
#define FLS_ACR_SLEEP_PD                (0x00001000UL)

/* Flash Key Register (FLASH_KEYR) */
#define FLS_KEY1                        (0x45670123UL)
#define FLS_KEY2                        (0xCDEF89ABUL)
#define FLS_OPT_KEY1                    (0x08192A3BUL)
#define FLS_OPT_KEY2                    (0x4C5D6E7FUL)

/* Flash Status Register (FLASH_SR) */
#define FLS_SR_EOP                      (0x00000001UL)
#define FLS_SR_OPERR                    (0x00000002UL)
#define FLS_SR_PROGERR                  (0x00000008UL)
#define FLS_SR_WRPERR                   (0x00000010UL)
#define FLS_SR_PGAERR                   (0x00000020UL)
#define FLS_SR_SIZERR                   (0x00000040UL)
#define FLS_SR_PGSERR                   (0x00000080UL)
#define FLS_SR_MISERR                   (0x00000100UL)
#define FLS_SR_FASTERR                  (0x00000200UL)
#define FLS_SR_RDERR                    (0x00004000UL)
#define FLS_SR_OPTVERR                  (0x00008000UL)
#define FLS_SR_BSY                      (0x00010000UL)

/* Flash Control Register (FLASH_CR) */
#define FLS_CR_PG                       (0x00000001UL)
#define FLS_CR_SER                      (0x00000002UL)
#define FLS_CR_MER                      (0x00000004UL)
#define FLS_CR_SNB_POS                  (3U)
#define FLS_CR_SNB_MASK                 (0x00000078UL)
#define FLS_CR_PSIZE_POS                (8U)
#define FLS_CR_PSIZE_MASK               (0x00000300UL)
#define FLS_CR_MER1                     (0x00008000UL)
#define FLS_CR_STRT                     (0x00010000UL)
#define FLS_CR_OPTSTRT                  (0x00020000UL)
#define FLS_CR_FSTPG                    (0x00040000UL)
#define FLS_CR_EOPIE                    (0x01000000UL)
#define FLS_CR_ERRIE                    (0x02000000UL)
#define FLS_CR_RDERRIE                  (0x04000000UL)
#define FLS_CR_OBL_LAUNCH               (0x08000000UL)
#define FLS_CR_OPTLOCK                  (0x40000000UL)
#define FLS_CR_LOCK                     (0x80000000UL)

/* Programming Size */
#define FLS_PSIZE_BYTE                  (0x00000000UL)  /* 8-bit */
#define FLS_PSIZE_HALFWORD              (0x00000100UL)  /* 16-bit */
#define FLS_PSIZE_WORD                  (0x00000200UL)  /* 32-bit */
#define FLS_PSIZE_DOUBLEWORD            (0x00000300UL)  /* 64-bit */

/************************************************************************************
 *                          Protection Configuration
 ************************************************************************************/

/* Read Protection Level */
#define FLS_RDP_LEVEL_NONE              (0xAA)
#define FLS_RDP_LEVEL_1                 (0x00)
#define FLS_RDP_LEVEL_2                 (0xCC)

/* Write Protection */
#define FLS_WRP_SECTOR_0                (0x00000001UL)
#define FLS_WRP_SECTOR_1                (0x00000002UL)
#define FLS_WRP_SECTOR_2                (0x00000004UL)
#define FLS_WRP_SECTOR_3                (0x00000008UL)
#define FLS_WRP_SECTOR_4                (0x00000010UL)
#define FLS_WRP_SECTOR_5                (0x00000020UL)
#define FLS_WRP_SECTOR_6                (0x00000040UL)
#define FLS_WRP_SECTOR_7                (0x00000080UL)
#define FLS_WRP_ALL_SECTORS             (0x000000FFUL)

/* PCROP (Proprietary Code Readout Protection) */
#define FLS_PCROP_ENABLED               STD_OFF
#define FLS_PCROP_RDP_ERASE             STD_OFF

/************************************************************************************
 *                          Type Definitions
 ************************************************************************************/

/* Address Type */
typedef uint32 Fls_AddressType;

/* Length Type */
typedef uint32 Fls_LengthType;

/* Sector Index Type */
typedef uint8 Fls_SectorIndexType;

/* Error Flags Type */
typedef uint32 Fls_ErrorFlagsType;

/************************************************************************************
 *                          Configuration Structures
 ************************************************************************************/

/* Flash Sector Configuration */
typedef struct
{
    Fls_AddressType       StartAddress;
    Fls_LengthType        Size;
    Fls_SectorSizeType    SizeType;
    boolean               WriteProtected;
    boolean               ReadProtected;
    uint8                 Bank;
    uint8                 SectorNumber;
} Fls_SectorConfigType;

/* Flash Timing Configuration */
typedef struct
{
    uint32 EraseTime4KB;
    uint32 EraseTime32KB;
    uint32 EraseTime64KB;
    uint32 EraseTime128KB;
    uint32 ProgramByteTime;
    uint32 ProgramWordTime;
    uint32 MaxEraseTimeout;
    uint32 MaxProgramTimeout;
    uint8  WaitStates;
} Fls_TimingConfigType;

/* Flash Protection Configuration */
typedef struct
{
    uint8 ReadProtectionLevel;
    uint32 WriteProtectionMask;
    boolean PcropEnabled;
    uint32 PcropStartAddress;
    uint32 PcropEndAddress;
} Fls_ProtectionConfigType;

/* General Configuration */
typedef struct
{
    boolean UseInterrupts;
    boolean PrefetchEnable;
    boolean InstructionCacheEnable;
    boolean DataCacheEnable;
    uint8   FlashLatency;
} Fls_GeneralConfigType;

/************************************************************************************
 *                          Extern Configuration
 ************************************************************************************/
extern const Fls_SectorConfigType Fls_SectorConfig[FLS_NUM_OF_CONFIGURED_SECTORS];
extern const Fls_TimingConfigType Fls_TimingConfig;
extern const Fls_ProtectionConfigType Fls_ProtectionConfig;
extern const Fls_GeneralConfigType Fls_GeneralConfig;

#ifdef __cplusplus
}
#endif

#endif /* FLASH_CFG_H */
