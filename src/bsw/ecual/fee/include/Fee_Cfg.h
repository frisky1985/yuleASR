/**
 * @file Fee_Cfg.h
 * @brief Flash EEPROM Emulation configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FEE_CFG_H
#define FEE_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define FEE_DEV_ERROR_DETECT            (STD_ON)
#define FEE_VERSION_INFO_API            (STD_ON)
#define FEE_SET_MODE_SUPPORTED          (STD_ON)
#define FEE_POLL_MODE                   (STD_ON)
#define FEE_USE_ERASE_SUSPEND           (STD_OFF)

/*==================================================================================================
*                                    NUMBER OF BLOCKS
==================================================================================================*/
#define FEE_NUM_BLOCKS                  (32U)
#define FEE_MAX_BLOCK_SIZE              (4096U)

/*==================================================================================================
*                                    BLOCK IDs
==================================================================================================*/
#define FEE_BLOCK_ID_CONFIG             ((Fee_BlockIdType)1U)
#define FEE_BLOCK_ID_CALIBRATION        ((Fee_BlockIdType)2U)
#define FEE_BLOCK_ID_FAULT_MEMORY       ((Fee_BlockIdType)3U)
#define FEE_BLOCK_ID_VIN                ((Fee_BlockIdType)4U)
#define FEE_BLOCK_ID_ODOMETER           ((Fee_BlockIdType)5U)
#define FEE_BLOCK_ID_USER_DATA_1        ((Fee_BlockIdType)6U)
#define FEE_BLOCK_ID_USER_DATA_2        ((Fee_BlockIdType)7U)
#define FEE_BLOCK_ID_RESERVED           ((Fee_BlockIdType)0U)

/*==================================================================================================
*                                    BLOCK SIZES
==================================================================================================*/
#define FEE_BLOCK_SIZE_CONFIG           (64U)
#define FEE_BLOCK_SIZE_CALIBRATION      (256U)
#define FEE_BLOCK_SIZE_FAULT_MEMORY     (512U)
#define FEE_BLOCK_SIZE_VIN              (17U)
#define FEE_BLOCK_SIZE_ODOMETER         (8U)
#define FEE_BLOCK_SIZE_USER_DATA        (128U)

/*==================================================================================================
*                                    FLASH CONFIGURATION
==================================================================================================*/
#define FEE_SECTOR_SIZE                 (0x10000U)   /* 64KB sectors */
#define FEE_NUMBER_OF_SECTORS           (4U)
#define FEE_VIRTUAL_PAGE_SIZE           (8U)         /* 8 bytes alignment */

/*==================================================================================================
*                                    GARBAGE COLLECTION
==================================================================================================*/
#define FEE_MAX_GC_CYCLES               (10000U)
#define FEE_MAX_GC_ERASES               (100000U)
#define FEE_MAX_WRITE_CYCLES            (100000U)

/*==================================================================================================
*                                    TIMING
==================================================================================================*/
#define FEE_MAXIMUM_BLOCKING_TIME_MS    (10U)
#define FEE_MAIN_FUNCTION_PERIOD_MS     (10U)

/*==================================================================================================
*                                    NOTIFICATIONS
==================================================================================================*/
#define FEE_NVM_JOB_END_NOTIFICATION    (STD_ON)
#define FEE_NVM_JOB_ERROR_NOTIFICATION  (STD_ON)

/*==================================================================================================
*                                    CRC CONFIGURATION
==================================================================================================*/
#define FEE_BLOCK_CRC_ENABLED           (STD_ON)
#define FEE_BLOCK_CRC_TYPE              (0U)  /* 0=CRC16, 1=CRC32 */

#endif /* FEE_CFG_H */
