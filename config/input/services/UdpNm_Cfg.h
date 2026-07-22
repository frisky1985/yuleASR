/**
 * @file UdpNm_Cfg.h
 * @brief UDP Network Management Configuration
 * @version 1.0.0
 * @date 2026-05-06
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: UDP Network Management Configuration
 * Module ID: 0x33
 * Layer: Service Layer
 */

#ifndef UDPNM_CFG_H
#define UDPNM_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define UDPNM_CFG_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define UDPNM_CFG_AR_RELEASE_MINOR_VERSION      (0x04U)
#define UDPNM_CFG_AR_RELEASE_REVISION_VERSION   (0x00U)

#define UDPNM_CFG_SW_MAJOR_VERSION              (0x01U)
#define UDPNM_CFG_SW_MINOR_VERSION              (0x00U)
#define UDPNM_CFG_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    GENERAL CONFIGURATION
==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 * STD_ON: Enable development error detection
 * STD_OFF: Disable development error detection
 */
#define UDPNM_DEV_ERROR_DETECT                  STD_ON

/**
 * @brief Version info API enable/disable
 * STD_ON: Enable version info API
 * STD_OFF: Disable version info API
 */
#define UDPNM_VERSION_INFO_API                  STD_ON

/**
 * @brief Passive mode enabled
 * If enabled, the node does not transmit NM messages
 */
#define UDPNM_PASSIVE_MODE_ENABLED              STD_OFF

/**
 * @brief Number of channels
 * Maximum number of NM channels supported
 */
#define UDPNM_NUMBER_OF_CHANNELS                (0x04U)

/**
 * @brief Main function period in milliseconds
 * Cycle time for calling UdpNm_MainFunction
 */
#define UDPNM_MAIN_FUNCTION_PERIOD              (0x0AU)  /* 10ms */

/*==================================================================================================
*                                    PDU CONFIGURATION
==================================================================================================*/

/**
 * @brief PDU length in bytes
 * Standard NM PDU length is 8 bytes
 */
#define UDPNM_PDU_LENGTH                        (0x08U)

/**
 * @brief User data enabled
 * STD_ON: Enable user data in NM PDU
 * STD_OFF: Disable user data in NM PDU
 */
#define UDPNM_USER_DATA_ENABLED                 STD_ON

/**
 * @brief User data offset in PDU
 * Starting byte position of user data
 */
#define UDPNM_USER_DATA_OFFSET                  (0x02U)

/**
 * @brief User data length in bytes
 * Length of user data in NM PDU
 */
#define UDPNM_USER_DATA_LENGTH                  (0x06U)

/**
 * @brief Node ID enabled
 * STD_ON: Include source node ID in NM PDU
 * STD_OFF: Exclude source node ID from NM PDU
 */
#define UDPNM_NODE_ID_ENABLED                   STD_ON

/**
 * @brief Control bit vector enabled
 * STD_ON: Include control bit vector in NM PDU
 * STD_OFF: Exclude control bit vector from NM PDU
 */
#define UDPNM_CONTROL_BIT_VECTOR_ENABLED        STD_ON

/**
 * @brief Position of node ID in PDU
 */
#define UDPNM_NODE_ID_POSITION                  (0x00U)  /* Byte 0 */

/**
 * @brief Position of control bit vector in PDU
 */
#define UDPNM_CONTROL_BIT_VECTOR_POSITION       (0x01U)  /* Byte 1 */

/*==================================================================================================
*                                    NODE DETECTION CONFIGURATION
==================================================================================================*/

/**
 * @brief Node detection enabled
 * STD_ON: Enable node detection
 * STD_OFF: Disable node detection
 */
#define UDPNM_NODE_DETECTION_ENABLED            STD_ON

/**
 * @brief Repeat message indication enabled
 * STD_ON: Enable repeat message indication
 * STD_OFF: Disable repeat message indication
 */
#define UDPNM_REPEAT_MESSAGE_IND_ENABLED        STD_ON

/**
 * @brief Bus synchronization enabled
 * STD_ON: Enable bus synchronization
 * STD_OFF: Disable bus synchronization
 */
#define UDPNM_BUS_SYNCHRONIZATION_ENABLED       STD_ON

/**
 * @brief Remote sleep indication enabled
 * STD_ON: Enable remote sleep indication
 * STD_OFF: Disable remote sleep indication
 */
#define UDPNM_REMOTE_SLEEP_IND_ENABLED          STD_ON

/*==================================================================================================
*                                    TIMING CONFIGURATION
==================================================================================================*/

/**
 * @brief NM message cycle time in milliseconds (TTyp)
 * Time between two NM messages in Normal Operation state
 */
#define UDPNM_MSG_CYCLE_TIME                    (0x0100U)  /* 256ms */

/**
 * @brief NM message timeout time in milliseconds (TMax)
 * Timeout for receiving NM messages
 */
#define UDPNM_MSG_TIMEOUT_TIME                  (0x0200U)  /* 512ms */

/**
 * @brief Repeat message time in milliseconds (TTyp)
 * Duration of the Repeat Message state
 */
#define UDPNM_REPEAT_MESSAGE_TIME               (0x0080U)  /* 128ms */

/**
 * @brief Wait bus sleep time in milliseconds (TWbs)
 * Time to wait before entering Bus Sleep mode
 */
#define UDPNM_WAIT_BUS_SLEEP_TIME               (0x0780U)  /* 1920ms */

/**
 * @brief NM timeout time in milliseconds (TError)
 * Timeout for error detection
 */
#define UDPNM_TIMEOUT_TIME                      (0x0400U)  /* 1024ms */

/**
 * @brief Immediate NM cycle time in milliseconds
 * Cycle time for immediate NM transmissions
 */
#define UDPNM_IMMEDIATE_NM_CYCLE_TIME           (0x0014U)  /* 20ms */

/**
 * @brief Number of immediate NM transmissions
 * Number of fast NM messages to transmit on network request
 */
#define UDPNM_IMMEDIATE_NM_TRANSMISIONS         (0x05U)    /* 5 transmissions */

/*==================================================================================================
*                                    BUS LOAD REDUCTION CONFIGURATION
==================================================================================================*/

/**
 * @brief Bus load reduction enabled
 * STD_ON: Enable bus load reduction
 * STD_OFF: Disable bus load reduction
 */
#define UDPNM_BUS_LOAD_REDUCTION_ENABLED        STD_OFF

/**
 * @brief Bus load reduction interval
 * Interval for bus load reduction in seconds
 */
#define UDPNM_BUS_LOAD_REDUCTION_INTERVAL       (0x64U)    /* 100 seconds */

/*==================================================================================================
*                                    PARTIAL NETWORKING CONFIGURATION
==================================================================================================*/

/**
 * @brief Partial networking enabled
 * STD_ON: Enable partial networking
 * STD_OFF: Disable partial networking
 */
#define UDPNM_PARTIAL_NETWORKING_ENABLED        STD_OFF

/**
 * @brief PN info length in bytes
 * Length of partial networking info in PDU
 */
#define UDPNM_PN_INFO_LENGTH                    (0x06U)

/**
 * @brief PN info offset in PDU
 * Starting byte position of PN info
 */
#define UDPNM_PN_INFO_OFFSET                    (0x02U)

/**
 * @brief EIRA calculation enabled
 * STD_ON: Enable EIRA (External Internal Request Array) calculation
 * STD_OFF: Disable EIRA calculation
 */
#define UDPNM_PN_EIRA_CALC_ENABLED              STD_OFF

/**
 * @brief ERA calculation enabled
 * STD_ON: Enable ERA (External Request Array) calculation
 * STD_OFF: Disable ERA calculation
 */
#define UDPNM_PN_ERA_CALC_ENABLED               STD_OFF

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/**
 * @brief State change notification callback
 * This macro is called when the NM state changes
 */
#define UDPNM_STATE_CHANGE_NOTIFICATION(channel, prev_state, curr_state) \
    Appl_UdpNm_StateChangeNotification((channel), (prev_state), (curr_state))

/**
 * @brief Remote sleep indication callback
 * This macro is called when remote sleep is indicated
 */
#define UDPNM_REMOTE_SLEEP_INDICATION(channel) \
    Appl_UdpNm_RemoteSleepIndication(channel)

/**
 * @brief Remote sleep cancellation callback
 * This macro is called when remote sleep is cancelled
 */
#define UDPNM_REMOTE_SLEEP_CANCELLATION(channel) \
    Appl_UdpNm_RemoteSleepCancellation(channel)

/**
 * @brief Network start indication callback
 * This macro is called when network start is indicated
 */
#define UDPNM_NETWORK_START_INDICATION(channel) \
    Appl_UdpNm_NetworkStartIndication(channel)

/**
 * @brief Network mode entry callback
 * This macro is called when network mode is entered
 */
#define UDPNM_NETWORK_MODE_ENTRY(channel) \
    Appl_UdpNm_NetworkModeEntry(channel)

/**
 * @brief Bus sleep mode entry callback
 * This macro is called when bus sleep mode is entered
 */
#define UDPNM_BUS_SLEEP_MODE_ENTRY(channel) \
    Appl_UdpNm_BusSleepModeEntry(channel)

/**
 * @brief Prepare bus sleep mode entry callback
 * This macro is called when prepare bus sleep mode is entered
 */
#define UDPNM_PREPARE_BUS_SLEEP_MODE_ENTRY(channel) \
    Appl_UdpNm_PrepareBusSleepModeEntry(channel)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/* Channel IDs */
#define UDPNM_CHANNEL_0                         (0x00U)
#define UDPNM_CHANNEL_1                         (0x01U)
#define UDPNM_CHANNEL_2                         (0x02U)
#define UDPNM_CHANNEL_3                         (0x03U)

/* Channel 0 Configuration */
#define UDPNM_CHANNEL_0_NODE_ID                 (0x01U)
#define UDPNM_CHANNEL_0_CLUSTER_ID              (0x0001U)
#define UDPNM_CHANNEL_0_TX_PDU_ID               (0x0100U)
#define UDPNM_CHANNEL_0_RX_PDU_ID               (0x0200U)

/* Channel 1 Configuration */
#define UDPNM_CHANNEL_1_NODE_ID                 (0x02U)
#define UDPNM_CHANNEL_1_CLUSTER_ID              (0x0001U)
#define UDPNM_CHANNEL_1_TX_PDU_ID               (0x0101U)
#define UDPNM_CHANNEL_1_RX_PDU_ID               (0x0201U)

/* Channel 2 Configuration */
#define UDPNM_CHANNEL_2_NODE_ID                 (0x03U)
#define UDPNM_CHANNEL_2_CLUSTER_ID              (0x0002U)
#define UDPNM_CHANNEL_2_TX_PDU_ID               (0x0102U)
#define UDPNM_CHANNEL_2_RX_PDU_ID               (0x0202U)

/* Channel 3 Configuration */
#define UDPNM_CHANNEL_3_NODE_ID                 (0x04U)
#define UDPNM_CHANNEL_3_CLUSTER_ID              (0x0002U)
#define UDPNM_CHANNEL_3_TX_PDU_ID               (0x0103U)
#define UDPNM_CHANNEL_3_RX_PDU_ID               (0x0203U)

/*==================================================================================================
*                                    COMPILE-TIME CHECKS
==================================================================================================*/
/* Ensure configuration is valid */
#if defined(UDPNM_PDU_LENGTH) && defined(UDPNM_USER_DATA_LENGTH) && defined(UDPNM_USER_DATA_OFFSET) && (UDPNM_USER_DATA_OFFSET + UDPNM_USER_DATA_LENGTH > UDPNM_PDU_LENGTH)
    #error "User data exceeds PDU length"
#endif

#if defined(UDPNM_NUMBER_OF_CHANNELS) && (UDPNM_NUMBER_OF_CHANNELS == 0)
    #error "At least one channel must be configured"
#endif

#if defined(UDPNM_MAIN_FUNCTION_PERIOD) && (UDPNM_MAIN_FUNCTION_PERIOD == 0)
    #error "Main function period must be greater than 0"
#endif

#endif /* UDPNM_CFG_H */
