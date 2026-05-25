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

/**
 * @file CanTrcv.h
 * @brief CAN Transceiver Driver - AUTOSAR 4.4.0
 *
 * This file contains the public API and type definitions for the CAN Transceiver
 * Driver module according to AUTOSAR SWS CANTransceiverDriver specification.
 *
 * @copyright Copyright (c) 2025
 * @author yuleASR
 * @version 1.0.0
 */

#ifndef CANTRCV_H
#define CANTRCV_H

/*==================================================================================================
 * INCLUDE FILES
 ==================================================================================================*/

#include "Std_Types.h"
#include "CanTrcv_Cfg.h"
#include "ComStack_Types.h"

#if (CANTRCV_WAKEUP_BY_BUS_USED == STD_ON)
#include "EcuM.h"
#endif

/*==================================================================================================
 * VERSION INFORMATION
 ==================================================================================================*/

/** @brief Vendor ID: Nous Research */
#define CANTRCV_VENDOR_ID                   (200u)

/** @brief Module ID: CAN Transceiver Driver */
#define CANTRCV_MODULE_ID                   (70u)

/** @brief Driver Instance ID */
#define CANTRCV_DRIVER_INSTANCE_ID          (0u)

/* Software Version */
#define CANTRCV_SW_MAJOR_VERSION            (1u)
#define CANTRCV_SW_MINOR_VERSION            (0u)
#define CANTRCV_SW_PATCH_VERSION            (0u)

/* AUTOSAR Version */
#define CANTRCV_AR_RELEASE_MAJOR_VERSION    (4u)
#define CANTRCV_AR_RELEASE_MINOR_VERSION    (4u)
#define CANTRCV_AR_RELEASE_REVISION_VERSION (0u)

/*==================================================================================================
 * DET ERROR CODES
 ==================================================================================================*/

#if (CANTRCV_DEV_ERROR_DETECT == STD_ON)
/** @brief API called with wrong parameter for Transceiver */
#define CANTRCV_E_INVALID_TRANSCEIVER       (0x01u)

/** @brief API called with invalid pointer parameter */
#define CANTRCV_E_PARAM_POINTER             (0x02u)

/** @brief API called with invalid transceiver mode parameter */
#define CANTRCV_E_INVALID_TRCVMODE          (0x03u)

/** @brief API called with invalid transceiver wakeup mode parameter */
#define CANTRCV_E_INVALID_TRCV_WAKEUP_MODE  (0x04u)

/** @brief Module initialization has failed */
#define CANTRCV_E_INIT_FAILED               (0x05u)

/** @brief API called before initialization or after De-Init */
#define CANTRCV_E_UNINIT                    (0x11u)

/** @brief API called with invalid transceiver wakeup reason parameter */
#define CANTRCV_E_PARAM_WAKEUPREASON        (0x06u)

/** @brief API called with invalid network configuration */
#define CANTRCV_E_INVALID_CONFIGURATION     (0x07u)
#endif /* CANTRCV_DEV_ERROR_DETECT == STD_ON */

/*==================================================================================================
 * API SERVICE IDs
 ==================================================================================================*/

/** @brief Service ID for CanTrcv_Init */
#define CANTRCV_SID_INIT                    (0x01u)

/** @brief Service ID for CanTrcv_DeInit */
#define CANTRCV_SID_DEINIT                  (0x02u)

/** @brief Service ID for CanTrcv_SetOpMode */
#define CANTRCV_SID_SETOPMODE               (0x03u)

/** @brief Service ID for CanTrcv_GetOpMode */
#define CANTRCV_SID_GETOPMODE               (0x04u)

/** @brief Service ID for CanTrcv_GetBusWuReason */
#define CANTRCV_SID_GETBUSWUREASON          (0x05u)

/** @brief Service ID for CanTrcv_SetWakeupMode */
#define CANTRCV_SID_SETWAKEUPMODE           (0x06u)

/** @brief Service ID for CanTrcv_GetVersionInfo */
#define CANTRCV_SID_GETVERSIONINFO          (0x07u)

/** @brief Service ID for CanTrcv_MainFunction */
#define CANTRCV_SID_MAINFUNCTION            (0x08u)

/** @brief Service ID for CanTrcv_CheckWakeup */
#define CANTRCV_SID_CHECKWAKEUP             (0x09u)

/** @brief Service ID for CanTrcv_CheckWakeupByTransceiver */
#define CANTRCV_SID_CHECKWAKEUPBYTRCV       (0x0Au)

/*==================================================================================================
 * TYPE DEFINITIONS
 ==================================================================================================*/

/**
 * @brief CAN Transceiver operation modes
 *
 * These modes represent the different operational states of the CAN transceiver.
 * Mode transitions must follow hardware-specific requirements.
 */
typedef enum
{
    /** @brief Transceiver in Normal mode - full communication capability */
    CANTRCV_TRCVMODE_NORMAL = 0u,
    
    /** @brief Transceiver in Standby mode - wake-up capable, reduced power */
    CANTRCV_TRCVMODE_STANDBY = 1u,
    
    /** @brief Transceiver in Sleep mode - lowest power, wake-up capable */
    CANTRCV_TRCVMODE_SLEEP = 2u
} CanTrcv_TrcvModeType;

/**
 * @brief CAN Transceiver wake-up reason
 *
 * Indicates the source of a wake-up event detected by the transceiver.
 */
typedef enum
{
    /** @brief No wake-up reason available or cleared */
    CANTRCV_WU_ERROR = 0u,
    
    /** @brief Wake-up caused by bus activity */
    CANTRCV_WU_BY_BUS = 1u,
    
    /** @brief Wake-up caused by external pin transition (e.g., local wake-up) */
    CANTRCV_WU_BY_PIN = 2u,
    
    /** @brief Wake-up caused internally (e.g., power-on or software) */
    CANTRCV_WU_INTERNALLY = 3u,
    
    /** @brief Wake-up reason not supported by hardware */
    CANTRCV_WU_NOT_SUPPORTED = 4u,
    
    /** @brief Wake-up caused by bus activity in power-on state */
    CANTRCV_WU_POWER_ON = 5u,
    
    /** @brief Wake-up caused by reset (e.g., after overtemperature) */
    CANTRCV_WU_BY_SYSERR = 6u,
    
    /** @brief Reserved value for future extensions */
    CANTRCV_WU_RESERVED = 7u
} CanTrcv_TrcvWakeupReasonType;

/**
 * @brief CAN Transceiver wake-up mode
 *
 * Controls whether the transceiver is enabled for wake-up detection.
 */
typedef enum
{
    /** @brief Transceiver disabled for wake-up detection */
    CANTRCV_WUMODE_DISABLE = 0u,
    
    /** @brief Transceiver enabled for wake-up detection */
    CANTRCV_WUMODE_ENABLE = 1u,
    
    /** @brief Transceiver enabled for wake-up detection and clears existing wake-up events */
    CANTRCV_WUMODE_CLEAR = 2u
} CanTrcv_TrcvWakeupModeType;

/**
 * @brief CAN Transceiver types supported by the driver
 *
 * Different transceiver hardware requires different control sequences.
 */
typedef enum
{
    /** @brief Generic/unknown transceiver type */
    CANTRCV_TJA1043 = 0u,
    
    /** @brief NXP TJA1042 high-speed CAN transceiver */
    CANTRCV_TJA1042 = 1u,
    
    /** @brief NXP TJA1043 high-speed CAN transceiver with local wake-up */
    CANTRCV_GENERIC = 2u,
    
    /** @brief Infineon TLE6250 high-speed CAN transceiver */
    CANTRCV_TLE6250 = 3u,
    
    /** @brief NXP UJA1168 System Basis Chip with CAN transceiver */
    CANTRCV_UJA1168 = 4u
} CanTrcv_HwType;

/**
 * @brief CAN Transceiver control pin types
 *
 * Defines the available control pins for transceiver mode control.
 */
typedef enum
{
    /** @brief Standby control pin (STB) - active low */
    CANTRCV_PIN_STB = 0u,
    
    /** @brief Enable control pin (EN) - active high */
    CANTRCV_PIN_EN = 1u,
    
    /** @brief Error/Interrupt pin (NERR/INTN) - active low */
    CANTRCV_PIN_NERR = 2u,
    
    /** @brief Wake-up pin (WAK) */
    CANTRCV_PIN_WAK = 3u,
    
    /** @brief Inhibit pin (INH) */
    CANTRCV_PIN_INH = 4u
} CanTrcv_PinType;

/**
 * @brief CAN Transceiver configuration structure
 *
 * This structure contains all configuration parameters for a single
 * CAN transceiver channel.
 */
typedef struct
{
    /** @brief Transceiver hardware type */
    CanTrcv_HwType hwType;
    
    /** @brief DIO channel ID for STB pin (if used) */
    Dio_ChannelType stbPin;
    
    /** @brief DIO channel ID for EN pin (if used) */
    Dio_ChannelType enPin;
    
    /** @brief DIO channel ID for NERR pin (if used) */
    Dio_ChannelType nerrPin;
    
    /** @brief DIO channel ID for WAK pin (if used) */
    Dio_ChannelType wakPin;
    
    /** @brief SPI sequence ID (for SPI-based transceivers) */
    Spi_SequenceType spiSequence;
    
    /** @brief SPI channel ID (for SPI-based transceivers) */
    Spi_ChannelType spiChannel;
    
    /** @brief Initial operation mode after initialization */
    CanTrcv_TrcvModeType initMode;
    
    /** @brief Wake-up enabled at startup (TRUE/FALSE) */
    boolean wakeupByBusUsed;
    
    /** @brief Wake-up by pin enabled */
    boolean wakeupByPinUsed;
    
    /** @brief Transceiver-specific timeout value in microseconds */
    uint32 modeTransitionTimeout;
    
    /** @brief ECUM wake-up source reference (for EcuM integration) */
    EcuM_WakeupSourceType wakeupSource;
    
    /** @brief Unique transceiver identifier */
    uint8 trcvId;
    
    /** @brief CAN controller associated with this transceiver */
    uint8 controllerId;
    
    /** @brief Flag indicating if SPI interface is used */
    boolean spiUsed;
    
    /** @brief Flag indicating if DIO interface is used */
    boolean dioUsed;
} CanTrcv_TrcvConfigType;

/**
 * @brief CAN Transceiver driver configuration structure
 *
 * This structure contains the complete configuration for the CAN Transceiver
 * Driver module.
 */
typedef struct
{
    /** @brief Number of configured transceivers */
    uint8 numTransceivers;
    
    /** @brief Pointer to array of transceiver configurations */
    const CanTrcv_TrcvConfigType* transceiverConfig;
    
    /** @brief Pointer to channel mapping table */
    const uint8* channelMapping;
    
    /** @brief Maximum number of transceivers supported */
    uint8 maxTransceivers;
} CanTrcv_ConfigType;

/**
 * @brief CAN Transceiver state structure (internal use)
 *
 * Tracks the runtime state of each transceiver channel.
 */
typedef struct
{
    /** @brief Current operation mode */
    CanTrcv_TrcvModeType currentMode;
    
    /** @brief Last detected wake-up reason */
    CanTrcv_TrcvWakeupReasonType wakeupReason;
    
    /** @brief Current wake-up mode setting */
    CanTrcv_TrcvWakeupModeType wakeupMode;
    
    /** @brief Mode transition in progress flag */
    boolean modeTransitionPending;
    
    /** @brief Timer for mode transition timeout handling */
    uint32 modeTransitionTimer;
    
    /** @brief Wake-up event detected flag */
    boolean wakeupDetected;
    
    /** @brief Transceiver initialized flag */
    boolean isInitialized;
    
    /** @brief Error flags (hardware-specific) */
    uint8 errorFlags;
} CanTrcv_TrcvStateType;

/*==================================================================================================
 * FUNCTION PROTOTYPES
 ==================================================================================================*/

/*================================================================================================*/
/**
 * @brief Initializes the CAN Transceiver Driver module
 *
 * This service initializes the CAN Transceiver Driver module and all
 * configured transceiver channels.
 *
 * @param[in] ConfigPtr Pointer to configuration structure
 *
 * @pre None
 * @post Module is initialized, transceivers are in configured initial mode
 *
 * @requirements SWS_CanTrcv_00001
 */
extern void CanTrcv_Init(const CanTrcv_ConfigType* ConfigPtr);

/*================================================================================================*/
/**
 * @brief De-initializes the CAN Transceiver Driver module
 *
 * This service de-initializes the CAN Transceiver Driver module and
 * puts all transceivers into a safe state (typically Sleep or Standby).
 *
 * @param None
 *
 * @pre Driver must be initialized
 * @post Module is de-initialized, transceivers in safe state
 *
 * @requirements SWS_CanTrcv_00002
 */
extern void CanTrcv_DeInit(void);

/*================================================================================================*/
/**
 * @brief Sets the operation mode of a CAN transceiver
 *
 * This service sets the operation mode of the specified CAN transceiver
 * to Normal, Standby, or Sleep mode.
 *
 * @param[in] Transceiver CAN transceiver to which API call applies (0..CANTRCV_MAX_TRANSCEIVERS-1)
 * @param[in] OpMode Requested operation mode
 *
 * @return Std_ReturnType
 *         - E_OK: Mode change successful
 *         - E_NOT_OK: Mode change failed
 *
 * @pre Driver must be initialized
 * @post Transceiver operation mode is set according to request
 *
 * @requirements SWS_CanTrcv_00003
 */
extern Std_ReturnType CanTrcv_SetOpMode(uint8 Transceiver, CanTrcv_TrcvModeType OpMode);

/*================================================================================================*/
/**
 * @brief Gets the current operation mode of a CAN transceiver
 *
 * This service reads the current operation mode of the specified CAN transceiver.
 *
 * @param[in] Transceiver CAN transceiver to which API call applies (0..CANTRCV_MAX_TRANSCEIVERS-1)
 * @param[out] OpMode Pointer to operation mode variable
 *
 * @return Std_ReturnType
 *         - E_OK: Read successful
 *         - E_NOT_OK: Read failed
 *
 * @pre Driver must be initialized
 * @post Current operation mode is returned via OpMode parameter
 *
 * @requirements SWS_CanTrcv_00004
 */
extern Std_ReturnType CanTrcv_GetOpMode(uint8 Transceiver, CanTrcv_TrcvModeType* OpMode);

/*================================================================================================*/
/**
 * @brief Gets the wake-up reason of a CAN transceiver
 *
 * This service returns the wake-up reason of the specified CAN transceiver.
 * The wake-up reason is cleared on read or by calling SetWakeupMode(CLEAR).
 *
 * @param[in] Transceiver CAN transceiver to which API call applies (0..CANTRCV_MAX_TRANSCEIVERS-1)
 * @param[out] Reason Pointer to wake-up reason variable
 *
 * @return Std_ReturnType
 *         - E_OK: Read successful
 *         - E_NOT_OK: Read failed
 *
 * @pre Driver must be initialized
 * @post Wake-up reason is returned via Reason parameter
 *
 * @requirements SWS_CanTrcv_00005
 */
extern Std_ReturnType CanTrcv_GetBusWuReason(uint8 Transceiver, CanTrcv_TrcvWakeupReasonType* Reason);

/*================================================================================================*/
/**
 * @brief Sets the wake-up mode of a CAN transceiver
 *
 * This service enables, disables, or clears wake-up detection for the
 * specified CAN transceiver.
 *
 * @param[in] Transceiver CAN transceiver to which API call applies (0..CANTRCV_MAX_TRANSCEIVERS-1)
 * @param[in] TrcvWakeupMode Requested wake-up mode
 *
 * @return Std_ReturnType
 *         - E_OK: Mode setting successful
 *         - E_NOT_OK: Mode setting failed
 *
 * @pre Driver must be initialized
 * @post Wake-up mode is set according to request
 *
 * @requirements SWS_CanTrcv_00006
 */
extern Std_ReturnType CanTrcv_SetWakeupMode(uint8 Transceiver, CanTrcv_TrcvWakeupModeType TrcvWakeupMode);

/*================================================================================================*/
/**
 * @brief Gets version information of the CAN Transceiver Driver
 *
 * This service returns version information for this module.
 *
 * @param[out] versioninfo Pointer to version information structure
 *
 * @pre None (if DET is off), Driver must be initialized (if DET is on)
 * @post Version information is returned via versioninfo parameter
 *
 * @requirements SWS_CanTrcv_00007
 */
#if (CANTRCV_VERSION_INFO_API == STD_ON)
extern void CanTrcv_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif /* CANTRCV_VERSION_INFO_API == STD_ON */

/*================================================================================================*/
/**
 * @brief Main function for cyclic processing
 *
 * This service performs cyclic processing tasks such as:
 * - Wake-up detection polling
 * - Mode transition monitoring
 * - Error detection and handling
 *
 * @param None
 *
 * @pre None (cyclic function)
 * @post Periodic tasks are executed
 *
 * @requirements SWS_CanTrcv_00008
 */
extern void CanTrcv_MainFunction(void);

/*================================================================================================*/
/**
 * @brief Checks for wake-up events
 *
 * This service is called by the EcuM to check for wake-up events.
 * It is typically called during startup or wake-up sequence.
 *
 * @param[in] WakeupSource Identifier of the wake-up source
 *
 * @return None
 *
 * @pre None
 * @post Wake-up events are checked and reported to EcuM
 *
 * @requirements SWS_CanTrcv_00108
 */
#if (CANTRCV_WAKEUP_BY_BUS_USED == STD_ON)
extern void CanTrcv_CheckWakeup(EcuM_WakeupSourceType WakeupSource);
#endif /* CANTRCV_WAKEUP_BY_BUS_USED == STD_ON */

/*================================================================================================*/
/**
 * @brief Checks for wake-up events by specific transceiver
 *
 * This service checks for wake-up events on a specific transceiver.
 *
 * @param[in] Transceiver CAN transceiver to check (0..CANTRCV_MAX_TRANSCEIVERS-1)
 *
 * @return Std_ReturnType
 *         - E_OK: Check performed
 *         - E_NOT_OK: Check failed
 *
 * @pre Driver must be initialized
 * @post Wake-up events for the specified transceiver are checked
 *
 * @requirements SWS_CanTrcv_00180
 */
extern Std_ReturnType CanTrcv_CheckWakeupByTransceiver(uint8 Transceiver);

/*==================================================================================================
 * EXTERNAL VARIABLES
 ==================================================================================================*/

/** @brief External reference to the link-time configuration structure */
extern const CanTrcv_ConfigType CanTrcv_Lcfg;

/** @brief External reference to the module initialization state */
extern boolean CanTrcv_IsInitialized;

#endif /* CANTRCV_H */
