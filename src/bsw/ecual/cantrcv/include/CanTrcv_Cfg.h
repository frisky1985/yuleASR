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
 * @file CanTrcv_Cfg.h
 * @brief CAN Transceiver Driver Configuration - AUTOSAR 4.4.0
 *
 * This file contains all pre-compile and link-time configuration parameters
 * for the CAN Transceiver Driver module.
 *
 * @copyright Copyright (c) 2025
 * @author yuleASR
 * @version 1.0.0
 */

#ifndef CANTRCV_CFG_H
#define CANTRCV_CFG_H

/*==================================================================================================
 * PRE-COMPILE CONFIGURATION PARAMETERS
 ==================================================================================================*/

/**
 * @brief Enables/Disables the API to read out the modules version information.
 *
 * STD_ON: Version info API is available
 * STD_OFF: Version info API is not available
 *
 * @requirements SWS_CanTrcv_00101
 */
#define CANTRCV_VERSION_INFO_API            STD_ON

/**
 * @brief Switches the development error detection and notification on or off.
 *
 * STD_ON: Development error detection enabled
 * STD_OFF: Development error detection disabled
 *
 * @requirements SWS_CanTrcv_00102
 */
#define CANTRCV_DEV_ERROR_DETECT            STD_ON

/**
 * @brief Specifies if wake-up by bus is supported and if the according
 *        hardware shall be configured and enabled.
 *
 * STD_ON: Wake-up by bus is supported and enabled
 * STD_OFF: Wake-up by bus is not supported
 *
 * @requirements SWS_CanTrcv_00103
 */
#define CANTRCV_WAKEUP_BY_BUS_USED          STD_ON

/**
 * @brief Specifies if wake-up by pin (local wake-up) is supported.
 *
 * STD_ON: Wake-up by pin is supported and enabled
 * STD_OFF: Wake-up by pin is not supported
 */
#define CANTRCV_WAKEUP_BY_PIN_USED          STD_ON

/**
 * @brief Specifies if the transceiver control is done via SPI interface.
 *
 * STD_ON: SPI interface is used for transceiver control
 * STD_OFF: SPI interface is not used
 */
#define CANTRCV_SPI_USED                    STD_OFF

/**
 * @brief Specifies if the transceiver control is done via DIO interface.
 *
 * STD_ON: DIO interface is used for transceiver control
 * STD_OFF: DIO interface is not used
 */
#define CANTRCV_DIO_USED                    STD_ON

/**
 * @brief Enables/Disables the CanTrcv_CheckWakeup API.
 *
 * STD_ON: CheckWakeup API is available
 * STD_OFF: CheckWakeup API is not available
 */
#define CANTRCV_CHECK_WAKEUP_API            STD_ON

/**
 * @brief Enables/Disables the CanTrcv_CheckWakeupByTransceiver API.
 *
 * STD_ON: CheckWakeupByTransceiver API is available
 * STD_OFF: CheckWakeupByTransceiver API is not available
 */
#define CANTRCV_CHECK_WAKEUP_BY_TRCV_API    STD_ON

/**
 * @brief Enables/Disables the CanTrcv_DeInit API.
 *
 * STD_ON: DeInit API is available
 * STD_OFF: DeInit API is not available
 */
#define CANTRCV_DEINIT_API                  STD_ON

/**
 * @brief Enable/disable the transceiver timing optimization (faster mode transitions).
 *
 * STD_ON: Timing optimization enabled
 * STD_OFF: Timing optimization disabled (use safe default timings)
 */
#define CANTRCV_TIMING_OPTIMIZATION         STD_OFF

/**
 * @brief Specifies if multiple transceiver channels are supported.
 *
 * STD_ON: Multiple transceivers supported
 * STD_OFF: Single transceiver only
 */
#define CANTRCV_MULTIPLE_TRCV_SUPPORT       STD_ON

/*==================================================================================================
 * HARDWARE CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Maximum number of transceivers supported by this driver configuration.
 *
 * This value defines the size of internal data structures and must match
 * the number of transceiver configurations in CanTrcv_Lcfg.c
 */
#define CANTRCV_MAX_TRANSCEIVERS            (2u)

/**
 * @brief Number of CAN channels/transceivers configured.
 */
#define CANTRCV_NUM_CHANNELS                (2u)

/**
 * @brief Transceiver hardware type selection.
 *
 * Supported types:
 * - CANTRCV_TJA1043: NXP TJA1043 high-speed CAN transceiver with local wake-up
 * - CANTRCV_TJA1042: NXP TJA1042 high-speed CAN transceiver
 * - CANTRCV_GENERIC: Generic transceiver with basic control pins
 * - CANTRCV_TLE6250: Infineon TLE6250 high-speed CAN transceiver
 * - CANTRCV_UJA1168: NXP UJA1168 System Basis Chip with CAN
 */
#define CANTRCV_HARDWARE_TYPE               CANTRCV_TJA1043

/**
 * @brief Default transceiver type for channels without specific configuration.
 */
#define CANTRCV_DEFAULT_HARDWARE_TYPE       CANTRCV_TJA1043

/*==================================================================================================
 * TIMING PARAMETERS
 ==================================================================================================*/

/**
 * @brief Mode transition timeout in milliseconds.
 *
 * This timeout is used when waiting for hardware mode transitions to complete.
 * The value must be large enough for the slowest supported transceiver.
 */
#define CANTRCV_MODE_TRANSITION_TIMEOUT_MS  (100u)

/**
 * @brief Delay between STB and EN signal toggling for TJA1043 (microseconds).
 *
 * TJA1043 requires specific timing between STB and EN transitions.
 */
#define CANTRCV_TJA1043_STB_EN_DELAY_US     (10u)

/**
 * @brief Delay for transceiver to enter Sleep mode (milliseconds).
 *
 * Time required for the transceiver to complete sleep mode entry.
 */
#define CANTRCV_SLEEP_MODE_DELAY_MS         (5u)

/**
 * @brief Delay for transceiver to enter Standby mode (milliseconds).
 */
#define CANTRCV_STANDBY_MODE_DELAY_MS       (2u)

/**
 * @brief Delay for transceiver to enter Normal mode (milliseconds).
 */
#define CANTRCV_NORMAL_MODE_DELAY_MS        (2u)

/**
 * @brief Main function call period in milliseconds.
 *
 * This value should match the scheduling period of CanTrcv_MainFunction.
 */
#define CANTRCV_MAIN_FUNCTION_PERIOD_MS     (10u)

/*==================================================================================================
 * DIO CHANNEL CONFIGURATION
 ==================================================================================================*/

/* Channel 0 (CAN0) - TJA1043 */
#define CANTRCV_CH0_STB_PIN                 (DIO_CHANNEL_10)    /* Standby pin - active low */
#define CANTRCV_CH0_EN_PIN                  (DIO_CHANNEL_11)    /* Enable pin - active high */
#define CANTRCV_CH0_NERR_PIN                (DIO_CHANNEL_12)    /* Error pin - active low */
#define CANTRCV_CH0_WAK_PIN                 (DIO_CHANNEL_13)    /* Wake-up pin */

/* Channel 1 (CAN1) - TJA1042 */
#define CANTRCV_CH1_STB_PIN                 (DIO_CHANNEL_20)    /* Standby pin - active low */
#define CANTRCV_CH1_EN_PIN                  (DIO_CHANNEL_21)    /* Enable pin - active high */
#define CANTRCV_CH1_NERR_PIN                (DIO_CHANNEL_22)    /* Error pin - active low */
#define CANTRCV_CH1_WAK_PIN                 (DIO_CHANNEL_23)    /* Wake-up pin (not used for TJA1042) */

/*==================================================================================================
 * SPI CONFIGURATION (for SPI-based transceivers)
 ==================================================================================================*/

/**
 * @brief SPI sequence used for transceiver 0 communication.
 */
#define CANTRCV_CH0_SPI_SEQUENCE            (SPI_SEQUENCE_0)

/**
 * @brief SPI channel used for transceiver 0 communication.
 */
#define CANTRCV_CH0_SPI_CHANNEL             (SPI_CHANNEL_0)

/**
 * @brief SPI sequence used for transceiver 1 communication.
 */
#define CANTRCV_CH1_SPI_SEQUENCE            (SPI_SEQUENCE_1)

/**
 * @brief SPI channel used for transceiver 1 communication.
 */
#define CANTRCV_CH1_SPI_CHANNEL             (SPI_CHANNEL_1)

/*==================================================================================================
 * WAKE-UP CONFIGURATION
 ==================================================================================================*/

/**
 * @brief EcuM wake-up source identifier for transceiver 0.
 *
 * This value is reported to the EcuM when a wake-up event is detected
 * on transceiver 0.
 */
#define CANTRCV_CH0_WAKEUP_SOURCE           (ECUM_WKSOURCE_CAN)

/**
 * @brief EcuM wake-up source identifier for transceiver 1.
 */
#define CANTRCV_CH1_WAKEUP_SOURCE           (ECUM_WKSOURCE_CAN1)

/**
 * @brief Default wake-up mode at initialization.
 *
 * CANTRCV_WUMODE_ENABLE: Wake-up detection enabled at startup
 * CANTRCV_WUMODE_DISABLE: Wake-up detection disabled at startup
 */
#define CANTRCV_DEFAULT_WAKEUP_MODE         CANTRCV_WUMODE_ENABLE

/**
 * @brief Enable wake-up filtering to reduce false wake-up events.
 *
 * STD_ON: Wake-up filtering enabled
 * STD_OFF: Wake-up filtering disabled
 */
#define CANTRCV_WAKEUP_FILTERING_ENABLED    STD_ON

/*==================================================================================================
 * ERROR HANDLING CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Enable error reporting to the Development Error Tracer (Det).
 *
 * STD_ON: Error reporting enabled
 * STD_OFF: Error reporting disabled
 */
#define CANTRCV_REPORT_TO_DET               STD_ON

/**
 * @brief Enable runtime error reporting.
 *
 * STD_ON: Runtime error reporting enabled
 * STD_OFF: Runtime error reporting disabled
 */
#define CANTRCV_RUNTIME_ERROR_REPORTING     STD_OFF

/**
 * @brief Enable production error reporting (via DEM).
 *
 * STD_ON: Production error reporting enabled
 * STD_OFF: Production error reporting disabled
 */
#define CANTRCV_PROD_ERROR_REPORTING        STD_OFF

/*==================================================================================================
 * CALLBACK CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Enable notification callback when wake-up is detected.
 *
 * STD_ON: Wake-up notification callback enabled
 * STD_OFF: Wake-up notification callback disabled
 */
#define CANTRCV_WAKEUP_NOTIFICATION_ENABLED STD_ON

/**
 * @brief Enable notification callback when transceiver error is detected.
 *
 * STD_ON: Error notification callback enabled
 * STD_OFF: Error notification callback disabled
 */
#define CANTRCV_ERROR_NOTIFICATION_ENABLED  STD_ON

/*==================================================================================================
 * INCLUDES
 ==================================================================================================*/

/* Include standard types */
#include "Std_Types.h"

/* Include DIO header if DIO interface is used */
#if (CANTRCV_DIO_USED == STD_ON)
#include "Dio.h"
#endif

/* Include SPI header if SPI interface is used */
#if (CANTRCV_SPI_USED == STD_ON)
#include "Spi.h"
#endif

/* Include EcuM header if wake-up is used */
#if (CANTRCV_WAKEUP_BY_BUS_USED == STD_ON)
#include "EcuM.h"
#endif

/*==================================================================================================
 * CALLBACK FUNCTION PROTOTYPES
 ==================================================================================================*/

#if (CANTRCV_WAKEUP_NOTIFICATION_ENABLED == STD_ON)
/**
 * @brief Callback function for wake-up notification.
 *
 * This function is called by the CanTrcv module when a wake-up event
 * is detected on any configured transceiver.
 *
 * @param[in] Transceiver Transceiver on which wake-up was detected
 */
extern void CanTrcv_WakeupNotification(uint8 Transceiver);
#endif

#if (CANTRCV_ERROR_NOTIFICATION_ENABLED == STD_ON)
/**
 * @brief Callback function for error notification.
 *
 * This function is called by the CanTrcv module when a transceiver
 * error is detected.
 *
 * @param[in] Transceiver Transceiver on which error was detected
 * @param[in] ErrorCode Error code indicating the type of error
 */
extern void CanTrcv_ErrorNotification(uint8 Transceiver, uint8 ErrorCode);
#endif

#endif /* CANTRCV_CFG_H */
