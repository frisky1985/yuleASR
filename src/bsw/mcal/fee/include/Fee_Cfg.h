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

/*==================================================================================================
 *                                      FLASH EEPROM EMULATION DRIVER
 *                                      (MCAL LAYER)
 *==================================================================================================
 * FILENAME: Fee_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Fee driver configuration header
 *==================================================================================================
 */

#ifndef FEE_CFG_H
#define FEE_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#define FEE_DEV_ERROR_DETECT            (STD_ON)

/**
 * @brief Version info API enable/disable
 */
#define FEE_VERSION_INFO_API            (STD_ON)

/**
 * @brief Erase suspend/resume support
 */
#define FEE_ERASE_SUSPEND_SUPPORT       (STD_ON)

/**
 * @brief Write verification support
 */
#define FEE_WRITE_VERIFY_SUPPORT        (STD_ON)

/**
 * @brief Compare operation support
 */
#define FEE_COMPARE_SUPPORT             (STD_ON)

/**
 * @brief Blank check operation support
 */
#define FEE_BLANK_CHECK_SUPPORT         (STD_ON)

/**
 * @brief Cancel operation support
 */
#define FEE_CANCEL_SUPPORT              (STD_ON)

/**
 * @brief Enable/disable ECC check
 */
#define FEE_ECC_CHECK_ENABLED           (STD_ON)

/**
 * @brief Enable/disable hardware error recovery
 */
#define FEE_HW_ERROR_RECOVERY           (STD_ON)

/*==================================================================================================
 *                                    FLASH HARDWARE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Flash base address
 */
#define FEE_FLASH_BASE_ADDR             (0x10000000U)

/**
 * @brief Flash total size in bytes
 */
#define FEE_FLASH_TOTAL_SIZE            (0x00800000U)   /* 8 MB */

/**
 * @brief Flash sector size
 */
#define FEE_FLASH_SECTOR_SIZE           (0x00010000U)   /* 64 KB */

/**
 * @brief Flash page size (minimum write unit)
 */
#define FEE_FLASH_PAGE_SIZE             (256U)          /* 256 bytes */

/**
 * @brief Flash word size (minimum programmable unit)
 */
#define FEE_FLASH_WORD_SIZE             (4U)            /* 4 bytes */

/**
 * @brief Number of configured sectors
 */
#define FEE_NUM_SECTORS                 (4U)

/**
 * @brief Number of configured blocks
 */
#define FEE_NUM_BLOCKS                  (16U)

/*==================================================================================================
 *                                    EEPROM EMULATION CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Virtual page size for EEPROM emulation
 */
#define FEE_VIRTUAL_PAGE_SIZE           (8U)            /* 8 bytes */

/**
 * @brief Maximum write cycles per block
 */
#define FEE_MAX_WRITE_CYCLES            (100000U)       /* 100k cycles */

/**
 * @brief Maximum erase cycles per sector
 */
#define FEE_MAX_ERASE_CYCLES            (100000U)       /* 100k cycles */

/**
 * @brief Garbage collection threshold (percentage)
 */
#define FEE_GC_THRESHOLD_PERCENT        (80U)           /* 80% full triggers GC */

/*==================================================================================================
 *                                    OPERATION LIMITS
 *==================================================================================================*/

/**
 * @brief Maximum read bytes in normal mode per cycle
 */
#define FEE_MAX_READ_NORMAL_MODE        (256U)

/**
 * @brief Maximum read bytes in fast mode per cycle
 */
#define FEE_MAX_READ_FAST_MODE          (1024U)

/**
 * @brief Maximum write bytes in normal mode per cycle
 */
#define FEE_MAX_WRITE_NORMAL_MODE       (256U)

/**
 * @brief Maximum write bytes in fast mode per cycle
 */
#define FEE_MAX_WRITE_FAST_MODE         (512U)

/*==================================================================================================
 *                                    TIMEOUT CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Erase operation timeout in microseconds
 */
#define FEE_ERASE_TIMEOUT_US            (5000000U)      /* 5 seconds */

/**
 * @brief Write operation timeout in microseconds
 */
#define FEE_WRITE_TIMEOUT_US            (100000U)       /* 100 ms */

/**
 * @brief Read operation timeout in microseconds
 */
#define FEE_READ_TIMEOUT_US             (10000U)        /* 10 ms */

/**
 * @brief Blank check timeout in microseconds
 */
#define FEE_BLANK_CHECK_TIMEOUT_US      (10000U)        /* 10 ms */

/*==================================================================================================
 *                                    SECTOR CONFIGURATION
 *==================================================================================================*/

/* Sector 0 - Primary EEPROM emulation sector */
#define FEE_SECTOR0_START_ADDR          (FEE_FLASH_BASE_ADDR + 0x00000000U)
#define FEE_SECTOR0_SIZE                (FEE_FLASH_SECTOR_SIZE)
#define FEE_SECTOR0_ERASE_CYCLES        (FEE_MAX_ERASE_CYCLES)
#define FEE_SECTOR0_WRITABLE            (TRUE)
#define FEE_SECTOR0_ERASABLE            (TRUE)

/* Sector 1 - Secondary EEPROM emulation sector */
#define FEE_SECTOR1_START_ADDR          (FEE_FLASH_BASE_ADDR + 0x00010000U)
#define FEE_SECTOR1_SIZE                (FEE_FLASH_SECTOR_SIZE)
#define FEE_SECTOR1_ERASE_CYCLES        (FEE_MAX_ERASE_CYCLES)
#define FEE_SECTOR1_WRITABLE            (TRUE)
#define FEE_SECTOR1_ERASABLE            (TRUE)

/* Sector 2 - Reserved for immediate data */
#define FEE_SECTOR2_START_ADDR          (FEE_FLASH_BASE_ADDR + 0x00020000U)
#define FEE_SECTOR2_SIZE                (FEE_FLASH_SECTOR_SIZE)
#define FEE_SECTOR2_ERASE_CYCLES        (FEE_MAX_ERASE_CYCLES)
#define FEE_SECTOR2_WRITABLE            (TRUE)
#define FEE_SECTOR2_ERASABLE            (TRUE)

/* Sector 3 - Reserved for garbage collection */
#define FEE_SECTOR3_START_ADDR          (FEE_FLASH_BASE_ADDR + 0x00030000U)
#define FEE_SECTOR3_SIZE                (FEE_FLASH_SECTOR_SIZE)
#define FEE_SECTOR3_ERASE_CYCLES        (FEE_MAX_ERASE_CYCLES)
#define FEE_SECTOR3_WRITABLE            (TRUE)
#define FEE_SECTOR3_ERASABLE            (TRUE)

/*==================================================================================================
 *                                    BLOCK CONFIGURATION
 *==================================================================================================*/

/* Block size definitions */
#define FEE_BLOCK_SIZE_8                (8U)
#define FEE_BLOCK_SIZE_16               (16U)
#define FEE_BLOCK_SIZE_32               (32U)
#define FEE_BLOCK_SIZE_64               (64U)
#define FEE_BLOCK_SIZE_128              (128U)
#define FEE_BLOCK_SIZE_256              (256U)
#define FEE_BLOCK_SIZE_512              (512U)
#define FEE_BLOCK_SIZE_1024             (1024U)

/* Default write cycles */
#define FEE_DEFAULT_WRITE_CYCLES        (100000U)

/* Data alignment */
#define FEE_DATA_ALIGNMENT              (FEE_VIRTUAL_PAGE_SIZE)

/* Number of blocks */
#define FEE_NUMBER_OF_BLOCKS            (10U)

/* Number of pages (dual-page scheme) */
#define FEE_NUMBER_OF_PAGES             (2U)

/* Page size */
#define FEE_PAGE_SIZE                   (FEE_FLASH_SECTOR_SIZE)

/* Page addresses */
#define FEE_PAGE_0_START_ADDRESS        (FEE_SECTOR0_START_ADDR)
#define FEE_PAGE_1_START_ADDRESS        (FEE_SECTOR1_START_ADDR)

/* GC repetitions */
#define FEE_GC_REPETITIONS              (3U)

/* Use erase suspend */
#define FEE_USE_ERASE_SUSPEND           (STD_ON)

/* Block IDs */
#define FEE_BLOCK_NVM_CONFIG_ID         (0x0001U)
#define FEE_BLOCK_NVM_ADMIN_ID          (0x0002U)
#define FEE_BLOCK_ID_1                  (0x0010U)
#define FEE_BLOCK_ID_2                  (0x0011U)
#define FEE_BLOCK_ID_3                  (0x0012U)
#define FEE_BLOCK_ID_4                  (0x0013U)
#define FEE_BLOCK_ID_5                  (0x0014U)
#define FEE_BLOCK_ID_6                  (0x0015U)
#define FEE_BLOCK_ID_7                  (0x0016U)
#define FEE_BLOCK_ID_8                  (0x0017U)

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Enable/disable job end notification
 */
#define FEE_JOB_END_NOTIFICATION_ENABLED    (STD_ON)

/**
 * @brief Enable/disable job error notification
 */
#define FEE_JOB_ERROR_NOTIFICATION_ENABLED  (STD_ON)

/*==================================================================================================
 *                                    INTERNAL DEFINES
 *==================================================================================================*/

/**
 * @brief Magic number for sector header validation
 */
#define FEE_SECTOR_MAGIC_NUMBER         (0x46454555U)   /* "FEEU" */

/**
 * @brief Invalid/empty flash value
 */
#define FEE_FLASH_EMPTY_VALUE           (0xFFFFFFFFU)

/**
 * @brief Block header size in bytes
 */
#define FEE_BLOCK_HEADER_SIZE           (8U)

/**
 * @brief Sector header size in bytes
 */
#define FEE_SECTOR_HEADER_SIZE          (16U)

#ifdef __cplusplus
}
#endif

#endif /* FEE_CFG_H */
