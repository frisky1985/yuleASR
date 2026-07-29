/**
 * @file CanNm_Cfg.h
 * @brief CAN Network Management Configuration
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Configuration file for CAN Network Management module
 */

#ifndef CANNM_CFG_H
#define CANNM_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/** @brief Development error detection enable/disable */
#define CANNM_DEV_ERROR_DETECT              (STD_ON)

/** @brief Version info API enable/disable */
#define CANNM_VERSION_INFO_API              (STD_ON)

/** @brief Bus load reduction enabled */
#define CANNM_BUS_LOAD_REDUCTION_ENABLED    (STD_OFF)

/** @brief Communication control enabled */
#define CANNM_COM_CONTROL_ENABLED           (STD_ON)

/** @brief Node detection enabled (CBV control) */
#define CANNM_NODE_DETECTION_ENABLED        (STD_ON)

/** @brief Node ID enabled in PDU (Byte 0) */
#define CANNM_NODE_ID_ENABLED               (STD_ON)

/** @brief User data in NM PDU enabled */
#define CANNM_USER_DATA_ENABLED             (STD_ON)

/** @brief Bus synchronization enabled */
#define CANNM_BUS_SYNCHRONIZATION_ENABLED   (STD_ON)

/** @brief Remote sleep indication enabled */
#define CANNM_REMOTE_SLEEP_IND_ENABLED      (STD_ON)

/** @brief Passive mode enabled (no transmission) */
#define CANNM_PASSIVE_MODE_ENABLED          (STD_OFF)

/** @brief Repeat message indication enabled */
#define CANNM_REPEAT_MSG_IND_ENABLED        (STD_ON)

/** @brief Immediate transmission mode enabled */
#define CANNM_IMMEDIATE_TRANSMISSION_ENABLED (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/** @brief Number of configured NM channels */
#define CANNM_NUMBER_OF_CHANNELS            (0x01U)

/** @brief Channel 0 handle */
#define CANNM_CHANNEL_0                     (0x00U)

/** @brief NM PDU length (bytes) - Standard OSEK NM PDU */
#define CANNM_PDU_LENGTH                    (0x08U)

/** @brief Source address position in PDU */
#define CANNM_PDU_SRC_ADDR_POS              (0x00U)

/** @brief Control bit vector position in PDU */
#define CANNM_PDU_CBV_POS                   (0x01U)

/** @brief User data position in PDU */
#define CANNM_PDU_USER_DATA_POS             (0x02U)

/** @brief User data length */
#define CANNM_PDU_USER_DATA_LENGTH          (0x06U)

/*==================================================================================================
*                                    TIMING PARAMETERS (in ms)
==================================================================================================*/

/** @brief NM message cycle time (TTyp) - Normal transmission period */
#define CANNM_MSG_CYCLE_TIME                (0x64U)  /* 100ms */

/** @brief NM message timeout time (TMax) */
#define CANNM_MSG_TIMEOUT_TIME              (0x258U) /* 600ms */

/** @brief Repeat message state time (TTyp) */
#define CANNM_REPEAT_MESSAGE_TIME           (0x5DCU) /* 1500ms */

/** @brief Wait bus sleep time (TWbs) */
#define CANNM_WAIT_BUS_SLEEP_TIME           (0x7D0U) /* 2000ms */

/** @brief NM timeout time (TError) */
#define CANNM_TIMEOUT_TIME                  (0x258U) /* 600ms */

/** @brief Immediate transmission cycle time (TTx) */
#define CANNM_IMMEDIATE_NM_CYCLE_TIME       (0x14U)  /* 20ms */

/** @brief Number of immediate NM transmissions */
#define CANNM_IMMEDIATE_NM_TRANSMISSIONS    (0x05U)

/** @brief Remote sleep indication time */
#define CANNM_REMOTE_SLEEP_IND_TIME         (0x3E8U) /* 1000ms */

/*==================================================================================================
*                                    NODE CONFIGURATION
==================================================================================================*/

/** @brief Local node ID (Source Address) */
#define CANNM_NODE_ID                       (0x01U)

/** @brief Cluster ID */
#define CANNM_CLUSTER_ID                    (0x01U)

/** @brief Function ID for NM messages (Base CAN ID) */
#define CANNM_CAN_ID_BASE                   (0x400U)

/** @brief Tx PDU ID */
#define CANNM_TX_PDU_ID                     (0x00U)

/** @brief Rx PDU ID */
#define CANNM_RX_PDU_ID                     (0x01U)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/** @brief State change notification callback enabled */
#define CANNM_STATE_CHANGE_NOTIFICATION_ENABLED (STD_ON)

/** @brief Remote sleep callback enabled */
#define CANNM_REMOTE_SLEEP_CALLBACK_ENABLED (STD_ON)

/** @brief PDU Rx indication callback enabled */
#define CANNM_PDU_RX_INDICATION_ENABLED     (STD_ON)

/** @brief Bus sleep mode entry callback enabled */
#define CANNM_BUS_SLEEP_MODE_ENTRY_ENABLED  (STD_ON)

/** @brief Prepare bus sleep mode entry callback enabled */
#define CANNM_PREPARE_BUS_SLEEP_MODE_ENTRY_ENABLED (STD_ON)

/** @brief Network mode entry callback enabled */
#define CANNM_NETWORK_MODE_ENTRY_ENABLED    (STD_ON)

/*==================================================================================================
*                                    MISCELLANEOUS
==================================================================================================*/

/** @brief Main function period (ms) */
#define CANNM_MAIN_FUNCTION_PERIOD          (0x0AU)  /* 10ms */

/** @brief Maximum number of nodes in cluster */
#define CANNM_MAX_NODES                     (0x08U)

/** @brief Retry count for transmission failures */
#define CANNM_TX_RETRY_LIMIT                (0x03U)

#endif /* CANNM_CFG_H */
