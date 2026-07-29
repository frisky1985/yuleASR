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
 *                                      FEE CONFIGURATION
 *==================================================================================================
 * FILENAME: Fee_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header for Flash EEPROM Emulation module
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
#define FEE_DEV_ERROR_DETECT            (STD_ON)
#define FEE_VERSION_INFO_API            (STD_ON)
#define FEE_SET_MODE_SUPPORTED          (STD_ON)
#define FEE_POLL_MODE                   (STD_ON)
#define FEE_USE_ERASE_SUSPEND           (STD_OFF)
#define FEE_IMMEDIATE_DATA_SUPPORTED    (STD_ON)
#define FEE_BLOCK_CRC_ENABLED           (STD_ON)
#define FEE_BLOCK_CRC_TYPE              (0u)  /* 0=CRC16, 1=CRC32 */

/*==================================================================================================
 *                                    NUMBER OF BLOCKS
 *==================================================================================================*/
#define FEE_NUM_BLOCKS                  (16u)
#define FEE_MAX_BLOCK_SIZE              (1024u)
#define FEE_NUM_SECTORS                 (4u)

/*==================================================================================================
 *                                    BLOCK IDs
 *==================================================================================================*/
#define FEE_BLOCK_ID_CONFIG             ((Fee_BlockIdType)1u)
#define FEE_BLOCK_ID_CALIBRATION        ((Fee_BlockIdType)2u)
#define FEE_BLOCK_ID_FAULT_MEMORY       ((Fee_BlockIdType)3u)
#define FEE_BLOCK_ID_VIN                ((Fee_BlockIdType)4u)
#define FEE_BLOCK_ID_ODOMETER           ((Fee_BlockIdType)5u)
#define FEE_BLOCK_ID_USER_DATA_1        ((Fee_BlockIdType)6u)
#define FEE_BLOCK_ID_USER_DATA_2        ((Fee_BlockIdType)7u)
#define FEE_BLOCK_ID_USER_DATA_3        ((Fee_BlockIdType)8u)
#define FEE_BLOCK_ID_DIAG_DATA          ((Fee_BlockIdType)9u)
#define FEE_BLOCK_ID_TEST_DATA          ((Fee_BlockIdType)10u)
#define FEE_BLOCK_ID_RESERVED_1         ((Fee_BlockIdType)11u)
#define FEE_BLOCK_ID_RESERVED_2         ((Fee_BlockIdType)12u)
#define FEE_BLOCK_ID_RESERVED_3         ((Fee_BlockIdType)13u)
#define FEE_BLOCK_ID_RESERVED_4         ((Fee_BlockIdType)14u)
#define FEE_BLOCK_ID_RESERVED_5         ((Fee_BlockIdType)15u)
#define FEE_BLOCK_ID_RESERVED           ((Fee_BlockIdType)0u)

/*==================================================================================================
 *                                    BLOCK SIZES
 *==================================================================================================*/
#define FEE_BLOCK_SIZE_CONFIG           (64u)
#define FEE_BLOCK_SIZE_CALIBRATION      (256u)
#define FEE_BLOCK_SIZE_FAULT_MEMORY     (512u)
#define FEE_BLOCK_SIZE_VIN              (17u)
#define FEE_BLOCK_SIZE_ODOMETER         (8u)
#define FEE_BLOCK_SIZE_USER_DATA        (128u)
#define FEE_BLOCK_SIZE_DIAG_DATA        (256u)
#define FEE_BLOCK_SIZE_TEST_DATA        (64u)

/*==================================================================================================
 *                                    FLASH CONFIGURATION
 *==================================================================================================*/
#define FEE_SECTOR_SIZE                 (0x10000u)   /* 64KB sectors */
#define FEE_FLASH_BASE_ADDRESS          (0x10000000u) /* Flash base address */
#define FEE_SECTOR_0_START              (0x10000000u)
#define FEE_SECTOR_1_START              (0x10010000u)
#define FEE_SECTOR_2_START              (0x10020000u)
#define FEE_SECTOR_3_START              (0x10030000u)
#define FEE_VIRTUAL_PAGE_SIZE           (8u)         /* 8 bytes alignment */

/*==================================================================================================
 *                                    GARBAGE COLLECTION
 *==================================================================================================*/
#define FEE_MAX_GC_CYCLES               (10000u)
#define FEE_MAX_GC_ERASES               (100000u)
#define FEE_MAX_WRITE_CYCLES            (100000u)
#define FEE_GC_THRESHOLD_PERCENT        (20u)        /* GC when free space < 20% */

/*==================================================================================================
 *                                    TIMING
 *==================================================================================================*/
#define FEE_MAXIMUM_BLOCKING_TIME_MS    (10u)
#define FEE_MAIN_FUNCTION_PERIOD_MS     (10u)
#define FEE_TIMEOUT_VALUE               (10000u)

/*==================================================================================================
 *                                    NOTIFICATIONS
 *==================================================================================================*/
#define FEE_NVM_JOB_END_NOTIFICATION    (STD_ON)
#define FEE_NVM_JOB_ERROR_NOTIFICATION  (STD_ON)

/*==================================================================================================
 *                                    CALLBACK FUNCTIONS
 *==================================================================================================*/
#define Fee_JobEndNotification_NvM      Fee_NvmJobEndNotification
#define Fee_JobErrorNotification_NvM    Fee_NvmJobErrorNotification

/*==================================================================================================
 *                                    MEMIF INTEGRATION
 *==================================================================================================*/
#define MEMIF_UNDERLYING_FEE            (1u)
#define MEMIF_UNDERLYING_EA             (2u)
#define MEMIF_UNDERLYING_EEP            (3u)

#ifdef __cplusplus
}
#endif

#endif /* FEE_CFG_H */
