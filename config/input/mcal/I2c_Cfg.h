/**
 * @file I2c_Cfg.h
 * @brief I2C Driver configuration header following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-05-01
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: I2C Driver (I2C)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef I2C_CFG_H
#define I2C_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
/** @brief Development error detection enable/disable */
#define I2C_DEV_ERROR_DETECT            (STD_ON)

/** @brief Version info API enable/disable */
#define I2C_VERSION_INFO_API            (STD_ON)

/** @brief Enable/disable DMA support */
#define I2C_DMA_SUPPORTED               (STD_ON)

/** @brief Enable/disable interrupt support */
#define I2C_INTERRUPT_SUPPORTED         (STD_ON)

/** @brief Enable/disable 10-bit addressing support */
#define I2C_10BIT_ADDRESS_SUPPORTED     (STD_ON)

/** @brief Enable/disable 16-bit addressing support (for some devices) */
#define I2C_16BIT_ADDRESS_SUPPORTED     (STD_OFF)

/** @brief Enable/disable SMBus support */
#define I2C_SMBUS_SUPPORTED             (STD_ON)

/** @brief Enable/disable multi-master support */
#define I2C_MULTI_MASTER_SUPPORTED      (STD_ON)

/** @brief Enable/disable clock stretching support */
#define I2C_CLOCK_STRETCHING_SUPPORTED  (STD_ON)

/** @brief Enable/disable slave mode support */
#define I2C_SLAVE_MODE_SUPPORTED        (STD_ON)

/** @brief Enable/disable general call address support */
#define I2C_GENERAL_CALL_SUPPORTED      (STD_ON)

/** @brief Enable/disable software reset API */
#define I2C_SW_RESET_API                (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
/** @brief Number of I2C hardware channels */
#define I2C_NUM_CHANNELS                (8U)

/** @brief Number of I2C hardware modules */
#define I2C_NUM_HW_UNITS                (4U)

/** @brief Number of DMA channels used by I2C */
#define I2C_NUM_DMA_CHANNELS            (8U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define I2C_CHANNEL_0                   ((I2c_ChannelType)0U)
#define I2C_CHANNEL_1                   ((I2c_ChannelType)1U)
#define I2C_CHANNEL_2                   ((I2c_ChannelType)2U)
#define I2C_CHANNEL_3                   ((I2c_ChannelType)3U)
#define I2C_CHANNEL_4                   ((I2c_ChannelType)4U)
#define I2C_CHANNEL_5                   ((I2c_ChannelType)5U)
#define I2C_CHANNEL_6                   ((I2c_ChannelType)6U)
#define I2C_CHANNEL_7                   ((I2c_ChannelType)7U)

/*==================================================================================================
*                                    HW UNIT DEFINITIONS
==================================================================================================*/
#define I2C_HW_UNIT_0                   ((I2c_HWUnitType)0U)
#define I2C_HW_UNIT_1                   ((I2c_HWUnitType)1U)
#define I2C_HW_UNIT_2                   ((I2c_HWUnitType)2U)
#define I2C_HW_UNIT_3                   ((I2c_HWUnitType)3U)

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
/** @brief I2C peripheral clock frequency in Hz (typically 24MHz or 48MHz) */
#define I2C_PERIPHERAL_CLOCK_FREQ       (24000000U)

/** @brief Standard mode clock frequency (100 KHz) */
#define I2C_STANDARD_MODE_FREQ          (100000U)

/** @brief Fast mode clock frequency (400 KHz) */
#define I2C_FAST_MODE_FREQ              (400000U)

/** @brief Fast mode plus clock frequency (1 MHz) */
#define I2C_FAST_MODE_PLUS_FREQ         (1000000U)

/** @brief High speed mode clock frequency (3.4 MHz) */
#define I2C_HIGH_SPEED_MODE_FREQ        (3400000U)

/** @brief Maximum buffer size for I2C transfers */
#define I2C_MAX_BUFFER_SIZE             (256U)

/** @brief Maximum number of slave addresses supported */
#define I2C_MAX_SLAVE_ADDRESSES         (4U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
/** @brief Bus busy timeout in milliseconds */
#define I2C_BUS_BUSY_TIMEOUT_MS         (100U)

/** @brief Transfer timeout in milliseconds */
#define I2C_TRANSFER_TIMEOUT_MS         (1000U)

/** @brief Clock stretching timeout in milliseconds */
#define I2C_CLOCK_STRETCH_TIMEOUT_MS    (50U)

/*==================================================================================================
*                                    INTERRUPT PRIORITY CONFIGURATION
==================================================================================================*/
/** @brief I2C interrupt priority (0-15, lower is higher priority) */
#define I2C_INTERRUPT_PRIORITY          (5U)

/** @brief I2C error interrupt priority */
#define I2C_ERROR_INTERRUPT_PRIORITY    (4U)

/** @brief I2C DMA TX interrupt priority */
#define I2C_DMA_TX_INTERRUPT_PRIORITY   (6U)

/** @brief I2C DMA RX interrupt priority */
#define I2C_DMA_RX_INTERRUPT_PRIORITY   (6U)

/*==================================================================================================
*                                    SMBUS CONFIGURATION
==================================================================================================*/
/** @brief SMBus timeout in milliseconds */
#define I2C_SMBUS_TIMEOUT_MS            (35U)

/** @brief SMBus PEC (Packet Error Checking) supported */
#define I2C_SMBUS_PEC_SUPPORTED         (STD_ON)

/** @brief SMBus Alert pin supported */
#define I2C_SMBUS_ALERT_SUPPORTED       (STD_ON)

/*==================================================================================================
*                                    ADDRESS MODE CONFIGURATION
==================================================================================================*/
/** @brief Default address mode (7-bit or 10-bit) */
#define I2C_DEFAULT_ADDRESS_MODE        (I2C_ADDR_MODE_7BIT)

/*==================================================================================================
*                                    TRANSFER MODE
==================================================================================================*/
/** @brief Default transfer mode (polling, interrupt, or DMA) */
#define I2C_DEFAULT_TRANSFER_MODE       (I2C_MODE_INTERRUPT)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
/** @brief Main function period in milliseconds */
#define I2C_MAIN_FUNCTION_PERIOD_MS     (10U)

#endif /* I2C_CFG_H */
