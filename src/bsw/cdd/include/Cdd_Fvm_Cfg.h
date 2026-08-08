/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Dependencies         : AUTOSAR 4.7
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Cdd_Fvm_Cfg.h
 * @brief   Complex Driver — Flash Virtual Memory (FVM) Pre-Compile Configuration
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Compile-time configuration of the FVM (Flash Virtual Memory) complex
 *   driver.  The FVM virtualizes the physical flash into multiple logical
 *   banks (primary + backup(s)) and provides:
 *     - bank registration / selection
 *     - bank-to-bank data migration (CopyBank)
 *     - bank state query (valid / corrupt / erased / protected)
 *     - erase and write protection
 *     - automatic failover when the active bank is corrupt
 *
 *   Default layout follows the S32K312 P-Flash memory map:
 *     P-Flash base 0x00400000, two 256 KB banks by default
 *     (banks 2/3 descriptors are provided for extension to 4 banks).
 *   Bank 0 = primary application image, bank 1 = backup image.
 *
 * @ASIL-D Safety Level
 */

#ifndef CDD_FVM_CFG_H
#define CDD_FVM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         MODULE SWITCHES
 *==================================================================================================*/

/** @brief Development error detection (Det_ReportError on invalid API usage) */
#define CDD_FVM_DEV_ERROR_DETECT            (STD_ON)

/** @brief Version info API (Cdd_Fvm_GetVersionInfo) */
#define CDD_FVM_VERSION_INFO_API            (STD_ON)

/** @brief Automatic integrity re-scan in Cdd_Fvm_MainFunction */
#define CDD_FVM_PERIODIC_CHECK_ENABLED      (STD_ON)

/** @brief MainFunction call period in milliseconds */
#define CDD_FVM_MAIN_FUNCTION_PERIOD        (10u)

/*==================================================================================================
 *                                         BACKEND SELECTION
 *==================================================================================================*/

/** @brief RAM-mirror backend (native build / SIL / unit tests, no hardware) */
#define CDD_FVM_BACKEND_RAM                 (0u)

/** @brief MCAL Fls-driver backend (production S32K312 target) */
#define CDD_FVM_BACKEND_FLS                 (1u)

/**
 * @brief Active backend
 * @note  Production S32K312 builds must set CDD_FVM_BACKEND_FLS.
 *        RAM backend keeps every FVM feature fully testable on host.
 */
#ifndef CDD_FVM_BACKEND
#define CDD_FVM_BACKEND                     (CDD_FVM_BACKEND_RAM)
#endif

/*==================================================================================================
 *                                         BANK GEOMETRY
 *==================================================================================================*/

/** @brief Maximum number of concurrently registered banks (runtime bound) */
#define CDD_FVM_MAX_BANKS                   (4u)

/** @brief Number of banks pre-registered by Cdd_Fvm_Init from the default table */
#define CDD_FVM_NUM_CONFIGURED_BANKS        (2u)

/** @brief S32K312 P-Flash base address (bank 0) */
#define CDD_FVM_BANK_0_START_ADDR           (0x00400000u)
#define CDD_FVM_BANK_0_SIZE                 (0x00040000u)   /* 256 KB */

/** @brief Bank 1 (backup image) */
#define CDD_FVM_BANK_1_START_ADDR           (0x00440000u)
#define CDD_FVM_BANK_1_SIZE                 (0x00040000u)   /* 256 KB */

/** @brief Bank 2 (optional extension, e.g. configuration storage) */
#define CDD_FVM_BANK_2_START_ADDR           (0x00480000u)
#define CDD_FVM_BANK_2_SIZE                 (0x00040000u)   /* 256 KB */

/** @brief Bank 3 (optional extension) */
#define CDD_FVM_BANK_3_START_ADDR           (0x004C0000u)
#define CDD_FVM_BANK_3_SIZE                 (0x00040000u)   /* 256 KB */

/** @brief Erase granularity in bytes (S32K312 P-Flash sector size) */
#define CDD_FVM_ERASE_GRANULARITY           (4096u)

/*==================================================================================================
 *                                         INTEGRITY METADATA
 *==================================================================================================*/

/** @brief Magic value stored at bank offset 0 by a finalized (valid) bank */
#define CDD_FVM_BANK_MAGIC                  (0xA5F1C0DEu)

/** @brief Size of the integrity signature (CRC32) stored at the bank tail */
#define CDD_FVM_SIGNATURE_SIZE              (4u)

/*==================================================================================================
 *                                         RAM BACKEND (TEST / NATIVE)
 *==================================================================================================*/

/** @brief Base address of the RAM mirror (kept equal to the flash base) */
#define CDD_FVM_RAM_BASE_ADDR               (0x00400000u)

/** @brief Size of the RAM mirror pool (covers the default bank table) */
#define CDD_FVM_RAM_POOL_SIZE               (0x00080000u)   /* 512 KB */

/*==================================================================================================
 *                                         DET ERROR CODES
 *==================================================================================================*/
#if (CDD_FVM_DEV_ERROR_DETECT == STD_ON)

#define CDD_FVM_E_PARAM_POINTER             (0x01u)   /* NULL pointer argument */
#define CDD_FVM_E_PARAM_BANK                (0x02u)   /* invalid / unregistered bank id */
#define CDD_FVM_E_PARAM_RANGE               (0x03u)   /* offset/length outside bank */
#define CDD_FVM_E_UNINIT                    (0x04u)   /* module not initialized */
#define CDD_FVM_E_BUSY                      (0x05u)   /* operation in progress */
#define CDD_FVM_E_WRITE_PROTECTED           (0x06u)   /* bank is write-protected */
#define CDD_FVM_E_CORRUPT                   (0x07u)   /* bank integrity check failed */
#define CDD_FVM_E_NO_VALID_BANK             (0x08u)   /* no valid bank available */
#define CDD_FVM_E_COPY_VERIFY               (0x09u)   /* post-copy verification failed */
#define CDD_FVM_E_HW                        (0x0Au)   /* hardware backend error */

#endif /* CDD_FVM_DEV_ERROR_DETECT */

#ifdef __cplusplus
}
#endif

#endif /* CDD_FVM_CFG_H */
