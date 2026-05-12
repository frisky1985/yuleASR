/**
 * @file Ocu_Lcfg.h
 * @brief OCU (Output Compare Unit) Driver link-time configuration header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver Link Configuration
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 */

#ifndef OCU_LCFG_H
#define OCU_LCFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Ocu.h"
#include "Ocu_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define OCU_LCFG_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define OCU_LCFG_MODULE_ID                   (0x7AU) /* OCU Module ID */
#define OCU_LCFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define OCU_LCFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define OCU_LCFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define OCU_LCFG_SW_MAJOR_VERSION            (0x01U)
#define OCU_LCFG_SW_MINOR_VERSION            (0x00U)
#define OCU_LCFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    VERSION CHECK
==================================================================================================*/
#if (OCU_AR_RELEASE_MAJOR_VERSION != OCU_LCFG_AR_RELEASE_MAJOR_VERSION)
    #error "Ocu_Lcfg.h: AR major version mismatch with Ocu.h"
#endif

#if (OCU_AR_RELEASE_MINOR_VERSION != OCU_LCFG_AR_RELEASE_MINOR_VERSION)
    #error "Ocu_Lcfg.h: AR minor version mismatch with Ocu.h"
#endif

#if (OCU_CFG_AR_RELEASE_MAJOR_VERSION != OCU_LCFG_AR_RELEASE_MAJOR_VERSION)
    #error "Ocu_Lcfg.h: AR major version mismatch with Ocu_Cfg.h"
#endif

#if (OCU_CFG_AR_RELEASE_MINOR_VERSION != OCU_LCFG_AR_RELEASE_MINOR_VERSION)
    #error "Ocu_Lcfg.h: AR minor version mismatch with Ocu_Cfg.h"
#endif

/*==================================================================================================
*                                    LINK-TIME CONFIGURATION MACROS
==================================================================================================*/
/**
 * @brief Number of configured channels (link-time constant)
 */
#define OCU_CFG_NUM_CHANNELS                 (OCU_NUM_CHANNELS)

/**
 * @brief Maximum counter value (link-time constant)
 */
#define OCU_CFG_MAX_COUNTER_VALUE            (OCU_MAX_COUNTER_VALUE)

/**
 * @brief Channel configuration indices
 */
#define OCU_CFG_CHANNEL_0_INDEX              (0x00U)
#define OCU_CFG_CHANNEL_1_INDEX              (0x01U)
#define OCU_CFG_CHANNEL_2_INDEX              (0x02U)
#define OCU_CFG_CHANNEL_3_INDEX              (0x03U)

/*==================================================================================================
*                                    EXTERNAL REFERENCES
==================================================================================================*/
/**
 * @brief External reference to channel configuration array
 */
#define OCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Ocu_ChannelConfigType Ocu_ChannelConfig[OCU_NUM_CHANNELS];

#define OCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CALLBACK DECLARATIONS
==================================================================================================*/
/**
 * @brief Notification callback declarations (to be implemented by application)
 */
#if (OCU_NOTIFICATION_SUPPORTED == STD_ON)

#if (OCU_CHANNEL_0_ENABLE == STD_ON)
void Ocu_Channel0_Notification(void);
#endif

#if (OCU_CHANNEL_1_ENABLE == STD_ON)
void Ocu_Channel1_Notification(void);
#endif

#if (OCU_CHANNEL_2_ENABLE == STD_ON)
void Ocu_Channel2_Notification(void);
#endif

#if (OCU_CHANNEL_3_ENABLE == STD_ON)
void Ocu_Channel3_Notification(void);
#endif

#endif /* OCU_NOTIFICATION_SUPPORTED */

#endif /* OCU_LCFG_H */
