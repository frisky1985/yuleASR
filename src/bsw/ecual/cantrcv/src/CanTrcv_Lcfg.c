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
/* @req SHALL_CANTRCV */


/**
 * @file CanTrcv_Lcfg.c
 * @brief AUTOSAR CAN Transceiver Driver Link-Time Configuration
 * @version 4.4.0
 * @date 2026-05-05
 */

#include "CanTrcv.h"
#include "CanTrcv_Cfg.h"

/*==================================================================================================
 *                                       LOCAL MACROS
 *=================================================================================================*/

#define CANTRCV_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                              CHANNEL CONFIGURATION TABLES
 *=================================================================================================*/

/**
 * @brief Channel 0 (Primary CAN) Configuration - TJA1043
 */
static const CanTrcv_ChannelConfigType CanTrcv_ChannelConfig_0 =
{
    /* Channel ID */
    0U,
    
    /* Transceiver Type */
    CANTRCV_TJA1043,
    
    /* Pin Configuration */
    {
        /* STB Pin - Standby control (inverted logic on TJA1043) */
        DIO_CHANNEL_CAN0_STB,
        /* EN Pin - Enable control */
        DIO_CHANNEL_CAN0_EN,
        /* ERR/NERR Pin - Error/Wake indication */
        DIO_CHANNEL_CAN0_ERR,
        /* STB Pin uses inverted logic */
        TRUE
    },
    
    /* Spi Configuration - Not used for TJA1043 */
    FALSE,  /* UsesSpi */
    0U,     /* SpiSequence */
    0U,     /* SpiChannel */
    
    /* Wakeup Configuration */
    TRUE,   /* Wake-up by bus enabled */
    FALSE,  /* Wake-up by pin enabled */
    ECUM_WKSOURCE_CAN0,
    
    /* Timing */
    10U,  /* Mode transition delay in ms */
    5U    /* Debounce count for wake-up */
};

/**
 * @brief Channel 1 (Secondary CAN) Configuration - TJA1043
 */
static const CanTrcv_ChannelConfigType CanTrcv_ChannelConfig_1 =
{
    /* Channel ID */
    1U,
    
    /* Transceiver Type */
    CANTRCV_TJA1043,
    
    /* Pin Configuration */
    {
        /* STB Pin */
        DIO_CHANNEL_CAN1_STB,
        /* EN Pin */
        DIO_CHANNEL_CAN1_EN,
        /* ERR Pin */
        DIO_CHANNEL_CAN1_ERR,
        /* Inverted logic */
        TRUE
    },
    
    /* Spi Configuration */
    FALSE,
    0U,
    0U,
    
    /* Wakeup Configuration */
    TRUE,
    FALSE,
    ECUM_WKSOURCE_CAN1,
    
    /* Timing */
    10U,
    5U
};

/**
 * @brief Channel 2 (Internal CAN) Configuration - TJA1042 (Simpler, no EN pin)
 */
static const CanTrcv_ChannelConfigType CanTrcv_ChannelConfig_2 =
{
    /* Channel ID */
    2U,
    
    /* Transceiver Type */
    CANTRCV_TJA1042,
    
    /* Pin Configuration */
    {
        /* STB Pin only */
        DIO_CHANNEL_CAN2_STB,
        /* No EN pin - use invalid channel */
        DIO_INVALID_CHANNEL,
        /* ERR Pin */
        DIO_CHANNEL_CAN2_ERR,
        /* Inverted logic */
        TRUE
    },
    
    /* Spi Configuration */
    FALSE,
    0U,
    0U,
    
    /* Wakeup Configuration */
    TRUE,
    FALSE,
    ECUM_WKSOURCE_CAN2,
    
    /* Timing */
    5U,
    3U
};

/**
 * @brief Array of all channel configurations
 */
static const CanTrcv_ChannelConfigType CanTrcv_ChannelConfig[CANTRCV_MAX_CHANNELS] =
{
    CanTrcv_ChannelConfig_0,
    CanTrcv_ChannelConfig_1,
    CanTrcv_ChannelConfig_2
};

/*==================================================================================================
 *                              GENERAL CONFIGURATION
 *=================================================================================================*/

/**
 * @brief General CanTrcv configuration
 */
static const CanTrcv_GeneralConfigType CanTrcv_GeneralConfig =
{
    /* Maximum number of transceiver channels */
    CANTRCV_MAX_CHANNELS,
    
    /* Development error detection enabled */
    CANTRCV_DEV_ERROR_DETECT,
    
    /* Version info API enabled */
    CANTRCV_VERSION_INFO_API,
    
    /* Wakeup check by polling (TRUE) or interrupt (FALSE) */
    TRUE,
    
    /* Main function period in ms */
    10U
};

/*==================================================================================================
 *                              MAIN CONFIGURATION STRUCTURE
 *=================================================================================================*/

/**
 * @brief Link-time configuration structure for CanTrcv
 */
const CanTrcv_ConfigType CanTrcv_Config =
{
    /* General configuration pointer */
    &CanTrcv_GeneralConfig,
    
    /* Channel configuration array */
    CanTrcv_ChannelConfig,
    
    /* Number of configured channels */
    CANTRCV_MAX_CHANNELS
};

/**
 * @brief Pointer to active configuration (initialized to link-time config)
 */
const CanTrcv_ConfigType* CanTrcv_ConfigPtr = &CanTrcv_Config;

/*==================================================================================================
 *                              POST-BUILD VARIANT CONFIGURATIONS
 *=================================================================================================*/

#ifdef CANTRCV_POSTBUILD_VARIANT_SUPPORT

/**
 * @brief Variant 1: All channels enabled (default)
 */
const CanTrcv_ConfigType CanTrcv_Config_Variant1 =
{
    &CanTrcv_GeneralConfig,
    CanTrcv_ChannelConfigs,
    CANTRCV_MAX_CHANNELS
};

/**
 * @brief Variant 2: Only first two channels enabled
 */
static const CanTrcv_ChannelConfigType* const CanTrcv_ChCfg_Variant2[2] =
{
    &CanTrcv_ChannelConfig_0,
    &CanTrcv_ChannelConfig_1
};

const CanTrcv_ConfigType CanTrcv_Config_Variant2 =
{
    &CanTrcv_GeneralConfig,
    CanTrcv_ChCfg_Variant2,
    2U
};

/**
 * @brief Variant 3: Only primary channel enabled
 */
static const CanTrcv_ChannelConfigType* const CanTrcv_ChCfg_Variant3[1] =
{
    &CanTrcv_ChannelConfig_0
};

const CanTrcv_ConfigType CanTrcv_Config_Variant3 =
{
    &CanTrcv_GeneralConfig,
    CanTrcv_ChCfg_Variant3,
    1U
};

#endif /* CANTRCV_POSTBUILD_VARIANT_SUPPORT */

#define CANTRCV_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                       END OF FILE
 *=================================================================================================*/
