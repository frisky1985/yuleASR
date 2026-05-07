/**
 * @file Dlt_Cfg.h
 * @brief Diagnostic Log and Trace module configuration
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: DLT Configuration
 */

#ifndef DLT_CFG_H
#define DLT_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DLT_CFG_VENDOR_ID                   (0x01U)
#define DLT_CFG_MODULE_ID                   (0x4CU)
#define DLT_CFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DLT_CFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DLT_CFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define DLT_CFG_SW_MAJOR_VERSION            (0x01U)
#define DLT_CFG_SW_MINOR_VERSION            (0x00U)
#define DLT_CFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    GENERAL CONFIGURATION
==================================================================================================*/

/** @brief Development error detection enabled */
#define DLT_DEV_ERROR_DETECT                (STD_ON)

/** @brief Version info API enabled */
#define DLT_VERSION_INFO_API                (STD_ON)

/** @brief Timestamp usage enabled */
#define DLT_USE_TIMESTAMP                   (STD_ON)

/** @brief ECU ID usage enabled */
#define DLT_USE_ECU_ID                      (STD_ON)

/** @brief Session ID usage enabled */
#define DLT_USE_SESSION_ID                  (STD_ON)

/** @brief Extended header enabled */
#define DLT_USE_EXTENDED_HEADER             (STD_ON)

/** @brief Verbose mode (more detailed logs) */
#define DLT_VERBOSE_MODE                    (STD_OFF)

/** @brief Enable serial output */
#define DLT_SERIAL_OUTPUT_ENABLED           (STD_ON)

/** @brief Enable network output */
#define DLT_NETWORK_OUTPUT_ENABLED          (STD_ON)

/** @brief Enable buffer output */
#define DLT_BUFFER_OUTPUT_ENABLED           (STD_ON)

/** @brief Default log level (DLT_LOG_DEBUG) */
#define DLT_DEFAULT_LOG_LEVEL               (DLT_LOG_DEBUG)

/** @brief Default output mode */
#define DLT_DEFAULT_OUTPUT_MODE             (DLT_OUTPUT_MODE_BOTH)

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/

/** @brief Ring buffer size (number of entries) */
#define DLT_RING_BUFFER_SIZE                (128U)

/** @brief Single buffer entry size in bytes */
#define DLT_BUFFER_ENTRY_SIZE               (256U)

/** @brief Maximum message length */
#define DLT_MAX_MESSAGE_LENGTH              (240U)

/** @brief Maximum number of arguments in formatted message */
#define DLT_MAX_ARGUMENTS                   (16U)

/*==================================================================================================
*                                    CONTEXT CONFIGURATION
==================================================================================================*/

/** @brief Maximum number of registered contexts */
#define DLT_MAX_CONTEXTS                    (32U)

/** @brief Maximum application ID length (4 chars + null) */
#define DLT_MAX_APPID_LENGTH                (5U)

/** @brief Maximum context ID length (4 chars + null) */
#define DLT_MAX_CONTEXTID_LENGTH            (5U)

/** @brief Maximum description length */
#define DLT_MAX_DESCRIPTION_LENGTH          (32U)

/*==================================================================================================
*                                    NETWORK CONFIGURATION
==================================================================================================*/

/** @brief Default network port */
#define DLT_DEFAULT_NETWORK_PORT            (3490U)

/** @brief Maximum network packet size */
#define DLT_MAX_NETWORK_PACKET_SIZE         (1400U)

/** @brief Network connection timeout (ms) */
#define DLT_NETWORK_TIMEOUT_MS              (5000U)

/*==================================================================================================
*                                    SERIAL CONFIGURATION
==================================================================================================*/

/** @brief Default UART baud rate */
#define DLT_SERIAL_BAUD_RATE                (115200U)

/** @brief Serial TX timeout (ms) */
#define DLT_SERIAL_TIMEOUT_MS               (100U)

/*==================================================================================================
*                                    TIMING CONFIGURATION
==================================================================================================*/

/** @brief Main function period (ms) */
#define DLT_MAIN_FUNCTION_PERIOD_MS         (10U)

/** @brief Buffer flush period (ms) */
#define DLT_FLUSH_PERIOD_MS                 (100U)

/** @brief Message timestamp resolution (microseconds) */
#define DLT_TIMESTAMP_RESOLUTION_US         (1000U)

/*==================================================================================================
*                                    ECU CONFIGURATION
==================================================================================================*/

/** @brief ECU ID (4 characters) */
#define DLT_ECU_ID                          {'Y', 'U', 'L', 'E', '\0'}

/** @brief Session ID */
#define DLT_SESSION_ID                      (0x00000001U)

/*==================================================================================================
*                                    PUBLISHED INFORMATION
==================================================================================================*/

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/** @brief Enable user-defined serial output callback */
#define DLT_USER_SERIAL_CALLBACK            (STD_OFF)

/** @brief Enable user-defined network output callback */
#define DLT_USER_NETWORK_CALLBACK           (STD_OFF)

/** @brief Enable user-defined timestamp callback */
#define DLT_USER_TIMESTAMP_CALLBACK         (STD_OFF)

/*==================================================================================================
*                                    FEATURE CONFIGURATION
==================================================================================================*/

/** @brief Enable control messages processing */
#define DLT_CONTROL_MESSAGES_ENABLED        (STD_ON)

/** @brief Enable non-blocking writes */
#define DLT_NON_BLOCKING_WRITE              (STD_ON)

/** @brief Enable overflow handling */
#define DLT_OVERFLOW_HANDLING               (STD_ON)

/** @brief Drop messages on overflow (if STD_OFF, block until space available) */
#define DLT_DROP_ON_OVERFLOW                (STD_ON)

/*==================================================================================================
*                                    FILTER CONFIGURATION
==================================================================================================*/

/** @brief Enable log level filtering */
#define DLT_LOG_LEVEL_FILTER_ENABLED        (STD_ON)

/** @brief Enable application ID filtering */
#define DLT_APPID_FILTER_ENABLED            (STD_OFF)

/** @brief Enable context ID filtering */
#define DLT_CONTEXTID_FILTER_ENABLED        (STD_OFF)

#endif /* DLT_CFG_H */
