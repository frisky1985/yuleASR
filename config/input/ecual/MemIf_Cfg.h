/**
 * @file MemIf_Cfg.h
 * @brief Memory Interface configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef MEMIF_CFG_H
#define MEMIF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define MEMIF_DEV_ERROR_DETECT          (STD_ON)
#define MEMIF_VERSION_INFO_API          (STD_ON)

/*==================================================================================================
*                                    NUMBER OF DEVICES
==================================================================================================*/
#define MEMIF_NUM_DEVICES               (2U)

/*==================================================================================================
*                                    DEVICE IDs
==================================================================================================*/
#define MEMIF_DEVICE_0                  ((MemIf_DeviceIdType)0U)
#define MEMIF_DEVICE_1                  ((MemIf_DeviceIdType)1U)

/*==================================================================================================
*                                    UNDERLYING DRIVERS
==================================================================================================*/
#define MEMIF_UNDERLYING_EEP            (0U)
#define MEMIF_UNDERLYING_FEE            (1U)
#define MEMIF_UNDERLYING_EA             (2U)

/*==================================================================================================
*                                    DEVICE CONFIGURATION
==================================================================================================*/
#define MEMIF_DEVICE_0_DRIVER           (MEMIF_UNDERLYING_FEE)
#define MEMIF_DEVICE_0_UNDERLYING_ID    (0U)
#define MEMIF_DEVICE_0_TOTAL_SIZE       (0x100000U)  /* 1MB Flash */
#define MEMIF_DEVICE_0_BLOCK_SIZE       (0x100U)     /* 256 bytes */

#define MEMIF_DEVICE_1_DRIVER           (MEMIF_UNDERLYING_EEP)
#define MEMIF_DEVICE_1_UNDERLYING_ID    (0U)
#define MEMIF_DEVICE_1_TOTAL_SIZE       (0x8000U)    /* 32KB EEPROM */
#define MEMIF_DEVICE_1_BLOCK_SIZE       (0x40U)      /* 64 bytes */

/*==================================================================================================
*                                    DEFAULT MODE
==================================================================================================*/
#define MEMIF_DEFAULT_MODE              (MEMIF_MODE_FAST)

/*==================================================================================================
*                                    BLOCK CONFIGURATION
==================================================================================================*/
#define MEMIF_MAX_BLOCK_NUMBER          (256U)
#define MEMIF_MAX_BLOCK_SIZE            (4096U)

#endif /* MEMIF_CFG_H */
