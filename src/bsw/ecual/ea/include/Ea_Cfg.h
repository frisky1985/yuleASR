/**
 * @file Ea_Cfg.h
 * @brief EEPROM Abstraction configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef EA_CFG_H
#define EA_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define EA_DEV_ERROR_DETECT             (STD_ON)
#define EA_VERSION_INFO_API             (STD_ON)
#define EA_SET_MODE_SUPPORTED           (STD_ON)
#define EA_POLL_MODE                    (STD_ON)

/*==================================================================================================
*                                    NUMBER OF BLOCKS
==================================================================================================*/
#define EA_NUM_BLOCKS                   (32U)
#define EA_MAX_BLOCK_SIZE               (256U)

/*==================================================================================================
*                                    BLOCK IDs
==================================================================================================*/
#define EA_BLOCK_ID_CONFIG              ((Ea_BlockIdType)1U)
#define EA_BLOCK_ID_CALIBRATION         ((Ea_BlockIdType)2U)
#define EA_BLOCK_ID_FAULT_MEMORY        ((Ea_BlockIdType)3U)
#define EA_BLOCK_ID_VIN                 ((Ea_BlockIdType)4U)
#define EA_BLOCK_ID_ODOMETER            ((Ea_BlockIdType)5U)
#define EA_BLOCK_ID_USER_DATA_1         ((Ea_BlockIdType)6U)
#define EA_BLOCK_ID_USER_DATA_2         ((Ea_BlockIdType)7U)
#define EA_BLOCK_ID_RESERVED            ((Ea_BlockIdType)0U)

/*==================================================================================================
*                                    BLOCK SIZES
==================================================================================================*/
#define EA_BLOCK_SIZE_CONFIG            (64U)
#define EA_BLOCK_SIZE_CALIBRATION       (128U)
#define EA_BLOCK_SIZE_FAULT_MEMORY      (256U)
#define EA_BLOCK_SIZE_VIN               (17U)
#define EA_BLOCK_SIZE_ODOMETER          (8U)
#define EA_BLOCK_SIZE_USER_DATA         (64U)

/*==================================================================================================
*                                    EEPROM CONFIGURATION
==================================================================================================*/
#define EA_SECTOR_SIZE                  (0x1000U)    /* 4KB sectors */
#define EA_NUMBER_OF_SECTORS            (8U)
#define EA_INDEX_SIZE                   (16U)        /* 16 bytes index */

/*==================================================================================================
*                                    DEVICE INDEX
==================================================================================================*/
#define EA_DEVICE_INDEX                 (0U)

/*==================================================================================================
*                                    TIMING
==================================================================================================*/
#define EA_MAIN_FUNCTION_PERIOD_MS      (10U)

/*==================================================================================================
*                                    NOTIFICATIONS
==================================================================================================*/
#define EA_NVM_JOB_END_NOTIFICATION     (STD_ON)
#define EA_NVM_JOB_ERROR_NOTIFICATION   (STD_ON)

/*==================================================================================================
*                                    CRC CONFIGURATION
==================================================================================================*/
#define EA_BLOCK_CRC_ENABLED            (STD_ON)

#endif /* EA_CFG_H */
