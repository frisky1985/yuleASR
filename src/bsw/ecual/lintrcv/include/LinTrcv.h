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

/*******************************************************************************
 * File Name          : LinTrcv.h
 * Description        : AUTOSAR LIN Transceiver Driver header file
 *                      Provides interface for LIN transceiver hardware control
 ******************************************************************************/

#ifndef LINTRCV_H
#define LINTRCV_H

/*=============================================================================
 * Includes
 ============================================================================*/
#include "Std_Types.h"
#include "LinTrcv_Cfg.h"

/*=============================================================================
 * Version Information
 ============================================================================*/
#define LINTRCV_VENDOR_ID                    (0x01U)
#define LINTRCV_MODULE_ID                    (122U)

#define LINTRCV_SW_MAJOR_VERSION             (1U)
#define LINTRCV_SW_MINOR_VERSION             (0U)
#define LINTRCV_SW_PATCH_VERSION             (0U)

#define LINTRCV_AR_RELEASE_MAJOR_VERSION     (4U)
#define LINTRCV_AR_RELEASE_MINOR_VERSION     (4U)
#define LINTRCV_AR_RELEASE_REVISION_VERSION  (0U)

/*=============================================================================
 * Development Error Codes
 ============================================================================*/
#define LINTRCV_E_UNINIT                     (0x01U)  /* Module not initialized */
#define LINTRCV_E_INVALID_CHANNEL            (0x02U)  /* Invalid channel parameter */
#define LINTRCV_E_INVALID_OPMODE             (0x03U)  /* Invalid operation mode */
#define LINTRCV_E_PARAM_POINTER              (0x04U)  /* NULL pointer error */
#define LINTRCV_E_INIT_FAILED                (0x05U)  /* Initialization failed */
#define LINTRCV_E_BUS_WU_NOT_SUPPORTED       (0x06U)  /* Bus wake-up not supported */
#define LINTRCV_E_PARAM_CONFIG               (0x07U)  /* Invalid configuration */
#define LINTRCV_E_HW_FAILURE                 (0x08U)  /* Hardware failure detected */

/*=============================================================================
 * Service IDs for Error Tracing
 ============================================================================*/
#define LINTRCV_SID_INIT                     (0x00U)
#define LINTRCV_SID_DEINIT                   (0x01U)
#define LINTRCV_SID_SETOPMODE                (0x02U)
#define LINTRCV_SID_GETOPMODE                (0x03U)
#define LINTRCV_SID_GETBUSWUREASON           (0x04U)
#define LINTRCV_SID_GETVERSIONINFO           (0x05U)
#define LINTRCV_SID_WAKEUP                   (0x06U)
#define LINTRCV_SID_CHECKWAKEUP              (0x07U)
#define LINTRCV_SID_CBK_WAKEUPBYBUS          (0x08U)

/*=============================================================================
 * Type Definitions
 ============================================================================*/

/* LIN Transceiver Operation Modes */
typedef enum
{
    LINTRCV_OPMODE_NORMAL = 0,    /* Normal operation mode - LIN communication active */
    LINTRCV_OPMODE_STANDBY,       /* Standby mode - low power, wake-up enabled */
    LINTRCV_OPMODE_SLEEP          /* Sleep mode - lowest power, wake-up enabled */
} LinTrcv_OpmodeType;

/* LIN Transceiver Wake-up Reasons */
typedef enum
{
    LINTRCV_WU_BY_BUS = 0,        /* Wake-up by bus activity */
    LINTRCV_WU_BY_PIN,            /* Wake-up by local wake-up pin (TJA1021 NWake) */
    LINTRCV_WU_BY_SYSERR,         /* Wake-up by system error */
    LINTRCV_WU_RESET,             /* Wake-up by power-on reset */
    LINTRCV_WU_INTERNAL,          /* Internal wake-up */
    LINTRCV_WU_NOT_SUPPORTED,     /* Wake-up not supported */
    LINTRCV_WU_ERROR,             /* Error during wake-up detection */
    LINTRCV_WU_POWER_ON,          /* Power-on wake-up */
    LINTRCV_WU_BY_BUS_CS          /* Wake-up by bus change state (TJA1021 specific) */
} LinTrcv_WakeupReasonType;

/* LIN Transceiver Wake-up Mode */
typedef enum
{
    LINTRCV_WUMODE_ENABLE = 0,    /* Wake-up enabled */
    LINTRCV_WUMODE_DISABLE,       /* Wake-up disabled */
    LINTRCV_WUMODE_CLEAR          /* Clear wake-up event */
} LinTrcv_WakeupModeType;

/* LIN Transceiver Channel State */
typedef enum
{
    LINTRCV_CHANNEL_UNINIT = 0,   /* Channel not initialized */
    LINTRCV_CHANNEL_INIT          /* Channel initialized */
} LinTrcv_ChannelStateType;

/* TJA1021/TJA1022 Specific Control Pin States */
typedef enum
{
    LINTRCV_PIN_LOW = 0,          /* Control pin LOW */
    LINTRCV_PIN_HIGH              /* Control pin HIGH */
} LinTrcv_PinStateType;

/* LIN Transceiver Hardware Type */
typedef enum
{
    LINTRCV_TJA1021 = 0,          /* NXP TJA1021 LIN transceiver */
    LINTRCV_TJA1022,              /* NXP TJA1022 dual LIN transceiver */
    LINTRCV_TJA1028,              /* NXP TJA1028 LIN transceiver */
    LINTRCV_GENERIC               /* Generic LIN transceiver */
} LinTrcv_HardwareType;

/* LIN Transceiver Control Interface Type */
typedef enum
{
    LINTRCV_CTRL_DIO = 0,         /* Digital IO control pins */
    LINTRCV_CTRL_SPI,             /* SPI control interface */
    LINTRCV_CTRL_I2C              /* I2C control interface */
} LinTrcv_ControlInterfaceType;

/* LIN Transceiver Channel Configuration Type */
typedef struct
{
    uint8 ChannelId;                       /* Unique channel identifier */
    LinTrcv_HardwareType HwType;           /* Hardware type (TJA1021, TJA1022, etc.) */
    LinTrcv_ControlInterfaceType CtrlIf;   /* Control interface type */
    
    /* DIO Control Pin Configurations (for DIO interface) */
    uint16 EnPinDio;                       /* Enable pin (EN) - TJA1021 Pin 2 */
    uint16 TxDPinDio;                      /* Transmit data pin (TXD) */
    uint16 NwadrsPinDio;                   /* NWake/Address pin - TJA1021 Pin 6 */
    uint16 NerrPinDio;                     /* Error pin (NERR) - TJA1021 Pin 9 */
    
    /* Wake-up Configuration */
    boolean WakeupByBusEnabled;            /* Enable bus wake-up detection */
    boolean WakeupByPinEnabled;            /* Enable pin wake-up detection (NWake) */
    uint32 WakeupSourceRef;                /* EcuM wakeup source reference */
    
    /* SPI Configuration (for SPI interface) */
    uint8 SpiChannel;                      /* SPI channel for control */
    uint8 SpiDevice;                       /* SPI device ID */
    
    /* Mode-specific delays (in microseconds) */
    uint32 SleepToNormalDelay;             /* Delay from Sleep to Normal mode */
    uint32 StandbyToNormalDelay;           /* Delay from Standby to Normal mode */
    uint32 NormalToStandbyDelay;           /* Delay from Normal to Standby mode */
    uint32 NormalToSleepDelay;             /* Delay from Normal to Sleep mode */
    
    /* Initial operation mode after Init */
    LinTrcv_OpmodeType InitialMode;
    
} LinTrcv_ChannelConfigType;

/* LIN Transceiver Configuration Type */
typedef struct
{
    uint8 NumChannels;                              /* Number of configured channels */
    const LinTrcv_ChannelConfigType *ChannelCfg;    /* Array of channel configurations */
    boolean VersionInfoApi;                          /* Version info API enabled */
    boolean WakeupByBusUsed;                         /* Wake-up by bus feature used */
    boolean WakeupByPinUsed;                         /* Wake-up by pin feature used */
} LinTrcv_ConfigType;

/*=============================================================================
 * External Data Declarations
 ============================================================================*/
#define LINTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Global configuration pointer */
extern const LinTrcv_ConfigType *LinTrcv_ConfigPtr;

#define LINTRCV_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*=============================================================================
 * Function Prototypes
 ============================================================================*/
#define LINTRCV_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_LinTrcv_00001 */
/*******************************************************************************
 * Function Name : LinTrcv_Init
 * Description   : Initializes the LIN transceiver driver
 *                 Configures all channels with their initial operation modes
 * Parameters    : ConfigPtr - Pointer to configuration structure
 * Return        : None
 ******************************************************************************/
extern void LinTrcv_Init(const LinTrcv_ConfigType *ConfigPtr);

/** @req SWS_LinTrcv_00002 */
/*******************************************************************************
 * Function Name : LinTrcv_DeInit
 * Description   : De-initializes the LIN transceiver driver
 *                 Sets all channels to default state
 * Parameters    : None
 * Return        : None
 ******************************************************************************/
extern void LinTrcv_DeInit(void);

/** @req SWS_LinTrcv_00005 */
/*******************************************************************************
 * Function Name : LinTrcv_SetOpMode
 * Description   : Sets the operation mode of a LIN transceiver channel
 *                 Controls mode transitions: Normal <-> Standby <-> Sleep
 * Parameters    : Channel - LIN transceiver channel ID
 *               : OpMode  - Target operation mode
 * Return        : E_OK     - Mode transition successful
 *               : E_NOT_OK - Mode transition failed
 ******************************************************************************/
extern Std_ReturnType LinTrcv_SetOpMode(uint8 Channel, LinTrcv_OpmodeType OpMode);

/** @req SWS_LinTrcv_00006 */
/*******************************************************************************
 * Function Name : LinTrcv_GetOpMode
 * Description   : Gets the current operation mode of a LIN transceiver channel
 * Parameters    : Channel - LIN transceiver channel ID
 *               : OpMode  - Pointer to store current operation mode
 * Return        : E_OK     - Successful
 *               : E_NOT_OK - Failed (invalid channel or NULL pointer)
 ******************************************************************************/
extern Std_ReturnType LinTrcv_GetOpMode(uint8 Channel, LinTrcv_OpmodeType *OpMode);

/** @req SWS_LinTrcv_00007 */
/*******************************************************************************
 * Function Name : LinTrcv_GetBusWuReason
 * Description   : Gets the wake-up reason for the specified channel
 *                 Reports why the transceiver woke up (bus, pin, etc.)
 * Parameters    : Channel    - LIN transceiver channel ID
 *               : WuReason   - Pointer to store wake-up reason
 * Return        : E_OK     - Successful
 *               : E_NOT_OK - Failed (invalid channel or NULL pointer)
 ******************************************************************************/
extern Std_ReturnType LinTrcv_GetBusWuReason(uint8 Channel, LinTrcv_WakeupReasonType *WuReason);

/*******************************************************************************
 * Function Name : LinTrcv_GetVersionInfo
 * Description   : Returns version information of the LIN transceiver driver
 * Parameters    : VersionInfo - Pointer to version info structure
 * Return        : None
 ******************************************************************************/
#if (LINTRCV_VERSION_INFO_API == STD_ON)
/** @req SWS_LinTrcv_00003 */
extern void LinTrcv_GetVersionInfo(Std_VersionInfoType *VersionInfo);
#endif

/** @req SWS_LinTrcv_00008 */
/*******************************************************************************
 * Function Name : LinTrcv_Wakeup
 * Description   : Initiates wake-up on the specified channel
 *                 Triggers wake-up signal on LIN bus
 * Parameters    : Channel - LIN transceiver channel ID
 * Return        : E_OK     - Wake-up initiated successfully
 *               : E_NOT_OK - Wake-up failed
 ******************************************************************************/
extern Std_ReturnType LinTrcv_Wakeup(uint8 Channel);

/** @req SWS_LinTrcv_00009 */
/*******************************************************************************
 * Function Name : LinTrcv_CheckWakeup
 * Description   : Checks if wake-up event occurred on specified channel
 *                 Called by EcuM during wake-up validation
 * Parameters    : Channel - LIN transceiver channel ID
 * Return        : E_OK     - Wake-up detected
 *               : E_NOT_OK - No wake-up detected
 ******************************************************************************/
extern Std_ReturnType LinTrcv_CheckWakeup(uint8 Channel);

/** @req SWS_LinTrcv_00010 */
/*******************************************************************************
 * Function Name : LinTrcv_Cbk_WakeupByBus
 * Description   : Callback for wake-up by bus notification
 *                 Called by Dio or SPI driver when wake-up event detected
 * Parameters    : Channel - LIN transceiver channel ID
 * Return        : None
 ******************************************************************************/
extern void LinTrcv_Cbk_WakeupByBus(uint8 Channel);

/** @req SWS_LinTrcv_00004 */
/*******************************************************************************
 * Function Name : LinTrcv_MainFunction
 * Description   : Main function for periodic wake-up detection and mode monitoring
 *                 Called cyclically by the scheduler
 * Parameters    : None
 * Return        : None
 ******************************************************************************/
extern void LinTrcv_MainFunction(void);

#define LINTRCV_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINTRCV_H */