/**
 * @file Ocu_Cfg.h
 * @brief OCU (Output Compare Unit) Driver configuration header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver Configuration
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 */

#ifndef OCU_CFG_H
#define OCU_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define OCU_CFG_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define OCU_CFG_MODULE_ID                   (0x7AU) /* OCU Module ID */
#define OCU_CFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define OCU_CFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define OCU_CFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define OCU_CFG_SW_MAJOR_VERSION            (0x01U)
#define OCU_CFG_SW_MINOR_VERSION            (0x00U)
#define OCU_CFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    CONFIGURATION VARIANT
==================================================================================================*/
/**
 * @brief Configuration variant selection
 * @options:
 *   OCU_VARIANT_PRE_COMPILE  - Pre-compile time configuration
 *   OCU_VARIANT_LINK_TIME    - Link time configuration
 *   OCU_VARIANT_POST_BUILD   - Post build configuration
 */
#define OCU_CONFIGURATION_VARIANT           (OCU_VARIANT_PRE_COMPILE)

/*==================================================================================================
*                                    GENERAL CONFIGURATION
==================================================================================================*/
/**
 * @brief Development error detection
 * @values STD_ON / STD_OFF
 */
#define OCU_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable
 * @values STD_ON / STD_OFF
 */
#define OCU_VERSION_INFO_API                (STD_ON)

/**
 * @brief De-initialization API enable
 * @values STD_ON / STD_OFF
 */
#define OCU_DE_INIT_API                     (STD_ON)

/**
 * @brief Set pin state API enable
 * @values STD_ON / STD_OFF
 */
#define OCU_SET_PIN_STATE_API               (STD_ON)

/**
 * @brief Set pin action API enable
 * @values STD_ON / STD_OFF
 */
#define OCU_SET_PIN_ACTION_API              (STD_ON)

/**
 * @brief Set threshold API enable
 * @values STD_ON / STD_OFF
 */
#define OCU_SET_THRESHOLD_API               (STD_ON)

/**
 * @brief Notification support
 * @values STD_ON / STD_OFF
 */
#define OCU_NOTIFICATION_SUPPORTED          (STD_ON)

/**
 * @brief Number of OCU channels
 */
#define OCU_NUM_CHANNELS                    (0x04U)

/**
 * @brief Maximum counter value
 */
#define OCU_MAX_COUNTER_VALUE               (0xFFFFFFFFU)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
/**
 * @brief Channel IDs
 */
#define OCU_CHANNEL_0                       (0x00U)
#define OCU_CHANNEL_1                       (0x01U)
#define OCU_CHANNEL_2                       (0x02U)
#define OCU_CHANNEL_3                       (0x03U)

/**
 * @brief Channel 0 configuration
 */
#define OCU_CHANNEL_0_ENABLE                (STD_ON)
#define OCU_CHANNEL_0_DEFAULT_PIN_STATE     (OCU_LOW)
#define OCU_CHANNEL_0_DEFAULT_THRESHOLD     (0x00010000U)
#define OCU_CHANNEL_0_NOTIFICATION          (NULL_PTR)
#define OCU_CHANNEL_0_BACKGROUND_MODE       (STD_OFF)
#define OCU_CHANNEL_0_BASE_ADDRESS          (0x40000000U)

/**
 * @brief Channel 1 configuration
 */
#define OCU_CHANNEL_1_ENABLE                (STD_ON)
#define OCU_CHANNEL_1_DEFAULT_PIN_STATE     (OCU_LOW)
#define OCU_CHANNEL_1_DEFAULT_THRESHOLD     (0x00010000U)
#define OCU_CHANNEL_1_NOTIFICATION          (NULL_PTR)
#define OCU_CHANNEL_1_BACKGROUND_MODE       (STD_OFF)
#define OCU_CHANNEL_1_BASE_ADDRESS          (0x40000010U)

/**
 * @brief Channel 2 configuration
 */
#define OCU_CHANNEL_2_ENABLE                (STD_ON)
#define OCU_CHANNEL_2_DEFAULT_PIN_STATE     (OCU_LOW)
#define OCU_CHANNEL_2_DEFAULT_THRESHOLD     (0x00010000U)
#define OCU_CHANNEL_2_NOTIFICATION          (NULL_PTR)
#define OCU_CHANNEL_2_BACKGROUND_MODE       (STD_OFF)
#define OCU_CHANNEL_2_BASE_ADDRESS          (0x40000020U)

/**
 * @brief Channel 3 configuration
 */
#define OCU_CHANNEL_3_ENABLE                (STD_ON)
#define OCU_CHANNEL_3_DEFAULT_PIN_STATE     (OCU_LOW)
#define OCU_CHANNEL_3_DEFAULT_THRESHOLD     (0x00010000U)
#define OCU_CHANNEL_3_NOTIFICATION          (NULL_PTR)
#define OCU_CHANNEL_3_BACKGROUND_MODE       (STD_OFF)
#define OCU_CHANNEL_3_BASE_ADDRESS          (0x40000030U)

/*==================================================================================================
*                                    HARDWARE CONFIGURATION
==================================================================================================*/
/**
 * @brief Hardware register offsets
 */
#define OCU_REG_CTRL_OFFSET                 (0x00U)
#define OCU_REG_STATUS_OFFSET               (0x04U)
#define OCU_REG_COUNTER_OFFSET              (0x08U)
#define OCU_REG_COMPARE_OFFSET              (0x0CU)
#define OCU_REG_ACTION_OFFSET               (0x10U)
#define OCU_REG_PIN_CTRL_OFFSET             (0x14U)

/**
 * @brief Control register bits
 */
#define OCU_CTRL_ENABLE_BIT                 (0x00000001U)
#define OCU_CTRL_INTERRUPT_BIT              (0x00000002U)
#define OCU_CTRL_PRESCALER_MASK             (0x0000FF00U)
#define OCU_CTRL_PRESCALER_SHIFT            (8U)

/**
 * @brief Status register bits
 */
#define OCU_STATUS_MATCH_BIT                (0x00000001U)
#define OCU_STATUS_OVERFLOW_BIT             (0x00000002U)

/**
 * @brief Default prescaler value
 */
#define OCU_DEFAULT_PRESCALER               (0x00U)

#endif /* OCU_CFG_H */
