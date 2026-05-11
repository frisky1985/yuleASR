/**
 * @file NvM_Cfg.h
 * @brief NVRAM Manager configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef NVM_CFG_H
#define NVM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define NVM_DEV_ERROR_DETECT            (STD_ON)
#define NVM_VERSION_INFO_API            (STD_ON)
#define NVM_SET_RAM_BLOCK_STATUS_API    (STD_ON)
#define NVM_GET_ERROR_STATUS_API        (STD_ON)
#define NVM_SET_BLOCK_PROTECTION_API    (STD_ON)
#define NVM_GET_BLOCK_PROTECTION_API    (STD_OFF)
#define NVM_SET_DATA_INDEX_API          (STD_ON)
#define NVM_GET_DATA_INDEX_API          (STD_OFF)
#define NVM_CANCEL_JOB_API              (STD_ON)
#define NVM_KILL_WRITE_ALL_API          (STD_OFF)
#define NVM_KILL_READ_ALL_API           (STD_OFF)
#define NVM_REPAIR_DAMAGED_BLOCKS_API   (STD_OFF)

/*==================================================================================================
*                                    BLOCK CONFIGURATION
==================================================================================================*/
#define NVM_NUM_OF_NVRAM_BLOCKS         (32U)
#define NVM_NUM_OF_DATASETS             (8U)
#define NVM_NUM_OF_ROM_BLOCKS           (16U)

/*==================================================================================================
*                                    BLOCK IDs
==================================================================================================*/
#define NVM_BLOCK_ID_FIRST              ((NvM_BlockIdType)1U)
#define NVM_BLOCK_ID_CONFIG             ((NvM_BlockIdType)1U)
#define NVM_BLOCK_ID_CALIBRATION        ((NvM_BlockIdType)2U)
#define NVM_BLOCK_ID_FAULT_MEMORY       ((NvM_BlockIdType)3U)
#define NVM_BLOCK_ID_VIN                ((NvM_BlockIdType)4U)
#define NVM_BLOCK_ID_ODOMETER           ((NvM_BlockIdType)5U)
#define NVM_BLOCK_ID_USER_DATA_1        ((NvM_BlockIdType)6U)
#define NVM_BLOCK_ID_USER_DATA_2        ((NvM_BlockIdType)7U)
#define NVM_BLOCK_ID_LAST               ((NvM_BlockIdType)31U)
#define NVM_BLOCK_ID_RESERVED           ((NvM_BlockIdType)0U)

/*==================================================================================================
*                                    BLOCK SIZES
==================================================================================================*/
#define NVM_BLOCK_SIZE_CONFIG           (64U)
#define NVM_BLOCK_SIZE_CALIBRATION      (256U)
#define NVM_BLOCK_SIZE_FAULT_MEMORY     (512U)
#define NVM_BLOCK_SIZE_VIN              (17U)
#define NVM_BLOCK_SIZE_ODOMETER         (8U)
#define NVM_BLOCK_SIZE_USER_DATA        (128U)

/*==================================================================================================
*                                    RETRY CONFIGURATION
==================================================================================================*/
#define NVM_MAX_NUMBER_OF_WRITE_RETRIES (3U)
#define NVM_MAX_NUMBER_OF_READ_RETRIES  (3U)

/*==================================================================================================
*                                    CRC CONFIGURATION
==================================================================================================*/
#define NVM_CALC_RAM_BLOCK_CRC          (STD_ON)
#define NVM_USE_CRC_COMP_MECHANISM      (STD_ON)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define NVM_MAIN_FUNCTION_PERIOD_MS     (10U)

/*==================================================================================================
*                                    QUEUE SIZES
==================================================================================================*/
#define NVM_SIZE_STANDARD_JOB_QUEUE     (16U)
#define NVM_SIZE_IMMEDIATE_JOB_QUEUE    (4U)

/*==================================================================================================
*                                    MULTI BLOCK REQUESTS
==================================================================================================*/
#define NVM_MULTI_BLOCK_CALLBACK        (STD_OFF)

#endif /* NVM_CFG_H */
