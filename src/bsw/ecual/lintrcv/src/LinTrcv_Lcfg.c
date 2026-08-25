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
/* @req SHALL_LINTRCV */


/*******************************************************************************
 * File Name          : LinTrcv_Lcfg.c
 * Description        : AUTOSAR LIN Transceiver Driver Link-Time Configuration
 *                      Channel configurations, pin mappings, and wake-up sources
 ******************************************************************************/

/*=============================================================================
 * Includes
 ============================================================================*/
#include "LinTrcv.h"
#include "LinTrcv_Cfg.h"

/*=============================================================================
 * Version Check
 ============================================================================*/
#if (LINTRCV_AR_RELEASE_MAJOR_VERSION != 4)
#error "LinTrcv_Lcfg.c: AR major version mismatch"
#endif

#if (LINTRCV_AR_RELEASE_MINOR_VERSION != 4)
#error "LinTrcv_Lcfg.c: AR minor version mismatch"
#endif

/*=============================================================================
 * Link-Time Configuration Data
 ============================================================================*/
#define LINTRCV_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
 * Channel Configuration Table
 * 
 * This table defines the configuration for each LIN transceiver channel.
 * Supports TJA1021 single and TJA1022 dual transceivers.
 * 
 * TJA1021 Pin Mapping:
 * - Pin 2 (EN):    Enable control - DIO output
 * - Pin 4 (TXD):   LIN TX data - Connected to LIN controller
 * - Pin 5 (RXD):   LIN RX data - Connected to LIN controller
 * - Pin 6 (NWake): Local wake-up input - DIO input (active low)
 * - Pin 9 (NERR):  Error output - DIO input (active low, open drain)
 * - Pin 12 (SLP_N): Sleep control (alternative mode control)
 ******************************************************************************/

/* Channel 0 Configuration - TJA1021 Single Transceiver */
static const LinTrcv_ChannelConfigType LinTrcv_Channel0Config =
{
    /* Channel Identification */
    0U,                                 /* ChannelId: Channel 0 */
    LINTRCV_TJA1021,                    /* HwType: TJA1021 transceiver */
    LINTRCV_CTRL_DIO,                   /* CtrlIf: DIO control interface */
    
    /* DIO Control Pin Configuration */
    DIO_CHANNEL_TJA1021_0_EN,           /* EnPinDio: Enable pin - TJA1021 Pin 2 */
    DIO_CHANNEL_TJA1021_0_TXD,          /* TxDPinDio: TXD pin - TJA1021 Pin 4 */
    DIO_CHANNEL_TJA1021_0_NWAKE,        /* NwadrsPinDio: NWake pin - TJA1021 Pin 6 */
    DIO_CHANNEL_TJA1021_0_NERR,         /* NerrPinDio: NERR pin - TJA1021 Pin 9 */
    
    /* Wake-up Configuration */
    TRUE,                               /* WakeupByBusEnabled: Enable bus wake-up */
    TRUE,                               /* WakeupByPinEnabled: Enable pin wake-up */
    LINTRCV_WAKEUP_SOURCE_0,            /* WakeupSourceRef: EcuM wake-up source for Ch0 */
    
    /* SPI Configuration (not used for DIO control) */
    0U,                                 /* SpiChannel: Not used */
    0U,                                 /* SpiDevice: Not used */
    
    /* Mode Transition Delays (microseconds) - Per TJA1021 datasheet */
    1000U,                              /* SleepToNormalDelay: 1ms max */
    100U,                               /* StandbyToNormalDelay: 100us typical */
    50U,                                /* NormalToStandbyDelay: 50us typical */
    50U,                                /* NormalToSleepDelay: 50us typical */
    
    /* Initial Operation Mode */
    LINTRCV_OPMODE_NORMAL               /* InitialMode: Start in Normal mode */
};

/* Channel 1 Configuration - TJA1021 Single Transceiver or TJA1022 Channel 2 */
#if (LINTRCV_NUM_CHANNELS > 1)
static const LinTrcv_ChannelConfigType LinTrcv_Channel1Config =
{
    /* Channel Identification */
    1U,                                 /* ChannelId: Channel 1 */
#if (LINTRCV_TJA1022_SUPPORT == STD_ON)
    LINTRCV_TJA1022,                    /* HwType: TJA1022 transceiver (dual) */
#else
    LINTRCV_TJA1021,                    /* HwType: TJA1021 transceiver */
#endif
    LINTRCV_CTRL_DIO,                   /* CtrlIf: DIO control interface */
    
    /* DIO Control Pin Configuration */
    DIO_CHANNEL_TJA1021_1_EN,           /* EnPinDio: Enable pin - TJA1021 Pin 2 */
    0xFFFFU,                            /* TxDPinDio: TXD - Not managed by LinTrcv */
    DIO_CHANNEL_TJA1021_1_NWAKE,        /* NwadrsPinDio: NWake pin - TJA1021 Pin 6 */
    DIO_CHANNEL_TJA1021_1_NERR,         /* NerrPinDio: NERR pin - TJA1021 Pin 9 */
    
    /* Wake-up Configuration */
    TRUE,                               /* WakeupByBusEnabled: Enable bus wake-up */
    TRUE,                               /* WakeupByPinEnabled: Enable pin wake-up */
    LINTRCV_WAKEUP_SOURCE_1,            /* WakeupSourceRef: EcuM wake-up source for Ch1 */
    
    /* SPI Configuration (not used for DIO control) */
    0U,                                 /* SpiChannel: Not used */
    0U,                                 /* SpiDevice: Not used */
    
    /* Mode Transition Delays (microseconds) */
    1000U,                              /* SleepToNormalDelay: 1ms max */
    100U,                               /* StandbyToNormalDelay: 100us typical */
    50U,                                /* NormalToStandbyDelay: 50us typical */
    50U,                                /* NormalToSleepDelay: 50us typical */
    
    /* Initial Operation Mode */
    LINTRCV_OPMODE_NORMAL               /* InitialMode: Start in Normal mode */
};
#endif

/* Channel 2 Configuration - Optional */
#if (LINTRCV_NUM_CHANNELS > 2)
static const LinTrcv_ChannelConfigType LinTrcv_Channel2Config =
{
    /* Channel Identification */
    2U,                                 /* ChannelId: Channel 2 */
    LINTRCV_TJA1021,                    /* HwType: TJA1021 transceiver */
    LINTRCV_CTRL_DIO,                   /* CtrlIf: DIO control interface */
    
    /* DIO Control Pin Configuration */
    0xFFFFU,                            /* EnPinDio: To be configured */
    0xFFFFU,                            /* TxDPinDio: Not managed */
    0xFFFFU,                            /* NwadrsPinDio: To be configured */
    0xFFFFU,                            /* NerrPinDio: To be configured */
    
    /* Wake-up Configuration */
    FALSE,                              /* WakeupByBusEnabled: Disabled */
    FALSE,                              /* WakeupByPinEnabled: Disabled */
    LINTRCV_WAKEUP_SOURCE_2,            /* WakeupSourceRef: EcuM wake-up source for Ch2 */
    
    /* SPI Configuration */
    0U,                                 /* SpiChannel: Not used */
    0U,                                 /* SpiDevice: Not used */
    
    /* Mode Transition Delays */
    1000U,                              /* SleepToNormalDelay */
    100U,                               /* StandbyToNormalDelay */
    50U,                                /* NormalToStandbyDelay */
    50U,                                /* NormalToSleepDelay */
    
    /* Initial Operation Mode */
    LINTRCV_OPMODE_STANDBY              /* InitialMode: Start in Standby mode */
};
#endif

/* Channel 3 Configuration - Optional */
#if (LINTRCV_NUM_CHANNELS > 3)
static const LinTrcv_ChannelConfigType LinTrcv_Channel3Config =
{
    /* Channel Identification */
    3U,                                 /* ChannelId: Channel 3 */
    LINTRCV_TJA1021,                    /* HwType: TJA1021 transceiver */
    LINTRCV_CTRL_DIO,                   /* CtrlIf: DIO control interface */
    
    /* DIO Control Pin Configuration */
    0xFFFFU,                            /* EnPinDio: To be configured */
    0xFFFFU,                            /* TxDPinDio: Not managed */
    0xFFFFU,                            /* NwadrsPinDio: To be configured */
    0xFFFFU,                            /* NerrPinDio: To be configured */
    
    /* Wake-up Configuration */
    FALSE,                              /* WakeupByBusEnabled: Disabled */
    FALSE,                              /* WakeupByPinEnabled: Disabled */
    LINTRCV_WAKEUP_SOURCE_3,            /* WakeupSourceRef: EcuM wake-up source for Ch3 */
    
    /* SPI Configuration */
    0U,                                 /* SpiChannel: Not used */
    0U,                                 /* SpiDevice: Not used */
    
    /* Mode Transition Delays */
    1000U,                              /* SleepToNormalDelay */
    100U,                               /* StandbyToNormalDelay */
    50U,                                /* NormalToStandbyDelay */
    50U,                                /* NormalToSleepDelay */
    
    /* Initial Operation Mode */
    LINTRCV_OPMODE_STANDBY              /* InitialMode: Start in Standby mode */
};
#endif

/*******************************************************************************
 * Channel Configuration Array
 ******************************************************************************/
static const LinTrcv_ChannelConfigType LinTrcv_ChannelConfig[LINTRCV_NUM_CHANNELS] =
{
    LinTrcv_Channel0Config,
#if (LINTRCV_NUM_CHANNELS > 1)
    LinTrcv_Channel1Config,
#endif
#if (LINTRCV_NUM_CHANNELS > 2)
    LinTrcv_Channel2Config,
#endif
#if (LINTRCV_NUM_CHANNELS > 3)
    LinTrcv_Channel3Config
#endif
};

/*******************************************************************************
 * Global LIN Transceiver Configuration
 ******************************************************************************/
static const LinTrcv_ConfigType LinTrcv_Config =
{
    LINTRCV_NUM_CHANNELS,               /* NumChannels: Number of configured channels */
    &LinTrcv_ChannelConfig[0],          /* ChannelCfg: Channel configuration array */
#if (LINTRCV_VERSION_INFO_API == STD_ON)
    TRUE,                               /* VersionInfoApi: Version info API enabled */
#else
    FALSE,
#endif
#if (LINTRCV_WAKEUP_BY_BUS_USED == STD_ON)
    TRUE,                               /* WakeupByBusUsed: Wake-up by bus enabled */
#else
    FALSE,
#endif
#if (LINTRCV_WAKEUP_BY_PIN_USED == STD_ON)
    TRUE                                /* WakeupByPinUsed: Wake-up by pin enabled */
#else
    FALSE
#endif
};

#define LINTRCV_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*=============================================================================
 * EcuM Wake-up Integration
 ============================================================================*/

/* EcuM_SetWakeupEvent is provided by the EcuM module */
/* This function is called by LinTrcv when wake-up events are detected */

/* Example EcuM_WakeupSource configuration in EcuM:
 * 
 * #define ECUM_WKSOURCE_LIN_CHANNEL_0  0x00000010UL
 * #define ECUM_WKSOURCE_LIN_CHANNEL_1  0x00000020UL
 * #define ECUM_WKSOURCE_LIN_CHANNEL_2  0x00000040UL
 * #define ECUM_WKSOURCE_LIN_CHANNEL_3  0x00000080UL
 */

/*=============================================================================
 * DIO Notification Configuration
 * 
 * These functions are called by the Dio driver when NWake pin changes state
 * (wake-up detection via external interrupt)
 ============================================================================*/

#if (LINTRCV_WAKEUP_SUPPORTED == STD_ON) && (LINTRCV_DIO_NOTIFICATION_API == STD_ON)

#define LINTRCV_START_SEC_CODE
#include "MemMap.h"

/*******************************************************************************
 * Function Name : Dio_LinTrcv_WakeUpNotification_Channel0
 * Description   : DIO notification for Channel 0 NWake pin (wake-up detection)
 ******************************************************************************/
static void Dio_LinTrcv_WakeUpNotification_Channel0(void)
{
    /* Notify LinTrcv about wake-up by pin */
    LinTrcv_Cbk_WakeupByBus(0U);
}

#if (LINTRCV_NUM_CHANNELS > 1)
/*******************************************************************************
 * Function Name : Dio_LinTrcv_WakeUpNotification_Channel1
 * Description   : DIO notification for Channel 1 NWake pin (wake-up detection)
 ******************************************************************************/
static void Dio_LinTrcv_WakeUpNotification_Channel1(void)
{
    LinTrcv_Cbk_WakeupByBus(1U);
}
#endif

#if (LINTRCV_NUM_CHANNELS > 2)
/*******************************************************************************
 * Function Name : Dio_LinTrcv_WakeUpNotification_Channel2
 * Description   : DIO notification for Channel 2 NWake pin (wake-up detection)
 ******************************************************************************/
void Dio_LinTrcv_WakeUpNotification_Channel2(void)
{
    LinTrcv_Cbk_WakeupByBus(2U);
}
#endif

#if (LINTRCV_NUM_CHANNELS > 3)
/*******************************************************************************
 * Function Name : Dio_LinTrcv_WakeUpNotification_Channel3
 * Description   : DIO notification for Channel 3 NWake pin (wake-up detection)
 ******************************************************************************/
void Dio_LinTrcv_WakeUpNotification_Channel3(void)
{
    LinTrcv_Cbk_WakeupByBus(3U);
}
#endif

#define LINTRCV_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINTRCV_WAKEUP_SUPPORTED && LINTRCV_DIO_NOTIFICATION_API */

/*=============================================================================
 * SPI Control Interface Configuration (Optional)
 * 
 * For TJA1021 variants with SPI control interface (TJA1021T/20)
 ============================================================================*/

#if (LINTRCV_SPI_SUPPORT == STD_ON)

#define LINTRCV_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* SPI Command Definitions for TJA1021 SPI variants */
#define TJA1021_SPI_CMD_NORMAL_MODE     (0x00U)  /* Enter Normal mode */
#define TJA1021_SPI_CMD_STANDBY_MODE    (0x01U)  /* Enter Standby mode */
#define TJA1021_SPI_CMD_SLEEP_MODE      (0x02U)  /* Enter Sleep mode */
#define TJA1021_SPI_CMD_READ_STATUS     (0x10U)  /* Read status register */
#define TJA1021_SPI_CMD_CLR_WU_FLAG     (0x20U)  /* Clear wake-up flag */

#define LINTRCV_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"
static void Dio_LinTrcv_WakeUpNotification_Channel1(void);
static void Dio_LinTrcv_WakeUpNotification_Channel0(void);
extern const LinTrcv_ConfigType LinTrcv_Config;

#endif /* LINTRCV_SPI_SUPPORT */

/*=============================================================================
 * Configuration Validation Checks
 ============================================================================*/

#if (LINTRCV_NUM_CHANNELS == 0U )
#error "At least one LIN transceiver channel must be configured"
#endif

#if (LINTRCV_NUM_CHANNELS > LINTRCV_MAX_CHANNELS)
#error "Number of channels exceeds maximum supported"
#endif

#if (LINTRCV_WAKEUP_SUPPORTED == STD_ON)
#if (LINTRCV_WAKEUP_BY_BUS_USED == STD_OFF) && (LINTRCV_WAKEUP_BY_PIN_USED == STD_OFF)
#warning "Wake-up supported but no wake-up source enabled"
#endif
#endif

/*=============================================================================
 * End of File
 ============================================================================*/