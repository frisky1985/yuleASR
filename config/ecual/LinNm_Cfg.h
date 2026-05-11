/**
 * @file LinNm_Cfg.h
 * @brief LIN Network Management module configuration following AutoSAR Classic Platform 4.x
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef LINNM_CFG_H
#define LINNM_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINNM_CFG_VENDOR_ID                     (0x01U)
#define LINNM_CFG_MODULE_ID                     (0x45U)
#define LINNM_CFG_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define LINNM_CFG_AR_RELEASE_MINOR_VERSION      (0x04U)
#define LINNM_CFG_AR_RELEASE_REVISION_VERSION   (0x00U)
#define LINNM_CFG_SW_MAJOR_VERSION              (0x01U)
#define LINNM_CFG_SW_MINOR_VERSION              (0x00U)
#define LINNM_CFG_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    CONFIGURATION PARAMETERS
==================================================================================================*/

/**
 * @brief Development error detection switch
 * STD_ON: Development error detection enabled
 * STD_OFF: Development error detection disabled
 */
#ifndef LINNM_DEV_ERROR_DETECT
#define LINNM_DEV_ERROR_DETECT                  (STD_ON)
#endif

/**
 * @brief Version info API switch
 * STD_ON: GetVersionInfo API enabled
 * STD_OFF: GetVersionInfo API disabled
 */
#ifndef LINNM_VERSION_INFO_API
#define LINNM_VERSION_INFO_API                  (STD_ON)
#endif

/**
 * @brief Communication control enabled switch
 * STD_ON: Communication control API enabled
 * STD_OFF: Communication control API disabled
 */
#ifndef LINNM_COM_CONTROL_ENABLED
#define LINNM_COM_CONTROL_ENABLED               (STD_ON)
#endif

/**
 * @brief Passive mode enabled switch
 * STD_ON: Passive mode (no sending) enabled
 * STD_OFF: Passive mode disabled
 */
#ifndef LINNM_PASSIVE_MODE_ENABLED
#define LINNM_PASSIVE_MODE_ENABLED              (STD_OFF)
#endif

/**
 * @brief State change indication enabled
 * STD_ON: State change callbacks to Nm enabled
 * STD_OFF: State change callbacks disabled
 */
#ifndef LINNM_STATE_CHANGE_IND_ENABLED
#define LINNM_STATE_CHANGE_IND_ENABLED          (STD_ON)
#endif

/**
 * @brief Remote sleep indication enabled
 * STD_ON: Remote sleep indication enabled
 * STD_OFF: Remote sleep indication disabled
 */
#ifndef LINNM_REMOTE_SLEEP_IND_ENABLED
#define LINNM_REMOTE_SLEEP_IND_ENABLED          (STD_ON)
#endif

/**
 * @brief Node detection enabled
 * STD_ON: Node detection feature enabled
 * STD_OFF: Node detection feature disabled
 */
#ifndef LINNM_NODE_DETECTION_ENABLED
#define LINNM_NODE_DETECTION_ENABLED            (STD_OFF)
#endif

/**
 * @brief Node ID enabled
 * STD_ON: Node ID in PDU enabled
 * STD_OFF: Node ID in PDU disabled
 */
#ifndef LINNM_NODE_ID_ENABLED
#define LINNM_NODE_ID_ENABLED                   (STD_ON)
#endif

/**
 * @brief User data enabled
 * STD_ON: User data in NM PDU enabled
 * STD_OFF: User data disabled
 */
#ifndef LINNM_USER_DATA_ENABLED
#define LINNM_USER_DATA_ENABLED                 (STD_ON)
#endif

/**
 * @brief Communication control with synchronized PNC shutdown
 * STD_ON: PNC coordinated shutdown enabled
 * STD_OFF: PNC coordinated shutdown disabled
 */
#ifndef LINNM_COM_CONTROL_SYNCHRONIZED
#define LINNM_COM_CONTROL_SYNCHRONIZED          (STD_OFF)
#endif

/**
 * @brief Coordinator support enabled
 * STD_ON: Coordinator synchronization enabled
 * STD_OFF: Coordinator synchronization disabled
 */
#ifndef LINNM_COORDINATOR_SYNC_SUPPORT
#define LINNM_COORDINATOR_SYNC_SUPPORT          (STD_OFF)
#endif

/**
 * @brief Bus synchronization enabled
 * STD_ON: Bus synchronization API enabled
 * STD_OFF: Bus synchronization API disabled
 */
#ifndef LINNM_BUS_SYNCHRONIZATION_ENABLED
#define LINNM_BUS_SYNCHRONIZATION_ENABLED       (STD_ON)
#endif

/**
 * @brief Bus load reduction enabled
 * STD_ON: Bus load reduction algorithm enabled
 * STD_OFF: Bus load reduction algorithm disabled
 */
#ifndef LINNM_BUSLOAD_REDUCTION_ENABLED
#define LINNM_BUSLOAD_REDUCTION_ENABLED         (STD_OFF)
#endif

/**
 * @brief Number of LIN NM channels
 */
#ifndef LINNM_NUMBER_OF_CHANNELS
#define LINNM_NUMBER_OF_CHANNELS                (2U)
#endif

/**
 * @brief Maximum number of nodes per channel (for diagnostic/tracking)
 */
#ifndef LINNM_MAX_NODES_PER_CHANNEL
#define LINNM_MAX_NODES_PER_CHANNEL             (16U)
#endif

/*==================================================================================================
*                                    TIMEOUT PARAMETERS
==================================================================================================*/

/**
 * @brief NM timeout time (ms)
 * Time to wait in Ready Sleep state before entering Prepare Bus Sleep
 * Range: 10ms - 65535ms
 */
#ifndef LINNM_TIMEOUT_TIME
#define LINNM_TIMEOUT_TIME                      (100U)
#endif

/**
 * @brief Wait bus sleep time (ms)
 * Time to wait in Prepare Bus Sleep state before entering Bus Sleep
 * Range: 10ms - 65535ms
 */
#ifndef LINNM_WAIT_BUSSLEEP_TIME
#define LINNM_WAIT_BUSSLEEP_TIME                (50U)
#endif

/**
 * @brief Remote sleep indication time (ms)
 * Time to wait before indicating remote sleep
 * Range: 10ms - 65535ms
 */
#ifndef LINNM_REMOTE_SLEEP_IND_TIME
#define LINNM_REMOTE_SLEEP_IND_TIME             (500U)
#endif

/**
 * @brief Message cycle time (ms)
 * Standard NM message cycle time
 * Range: 10ms - 255ms
 */
#ifndef LINNM_MSG_CYCLE_TIME
#define LINNM_MSG_CYCLE_TIME                    (20U)
#endif

/**
 * @brief Reduced message cycle time (ms)
 * Message cycle time during bus load reduction
 * Range: 10ms - 255ms
 */
#ifndef LINNM_MSG_REDUCED_TIME
#define LINNM_MSG_REDUCED_TIME                  (50U)
#endif

/**
 * @brief Message cycle offset (ms)
 * Offset for message transmission to avoid bus load peaks
 * Range: 0ms - 255ms
 */
#ifndef LINNM_MSG_CYCLE_OFFSET
#define LINNM_MSG_CYCLE_OFFSET                  (5U)
#endif

/**
 * @brief Repeat message time (ms)
 * Duration of Repeat Message state
 * Range: 10ms - 65535ms
 */
#ifndef LINNM_REPEAT_MESSAGE_TIME
#define LINNM_REPEAT_MESSAGE_TIME               (150U)
#endif

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/**
 * @brief LIN NM Channel 0 - Master Channel
 */
#define LINNM_CHANNEL_0                         (0U)
#define LINNM_CH0_NETWORK_HANDLE                (0U)
#define LINNM_CH0_LINIF_CHANNEL                 (0U)
#define LINNM_CH0_NODE_ID                       (0x01U)
#define LINNM_CH0_NODE_TYPE                     (LINNM_NODE_TYPE_MASTER)
#define LINNM_CH0_PASSIVE_MODE                  (STD_OFF)
#define LINNM_CH0_STATE_REPORT                  (STD_ON)
#define LINNM_CH0_TIMEOUT_TIME                  (100U)
#define LINNM_CH0_WAIT_BUSSLEEP_TIME            (50U)
#define LINNM_CH0_REMOTE_SLEEP_IND_TIME         (500U)
#define LINNM_CH0_MSG_CYCLE_TIME                (20U)
#define LINNM_CH0_MSG_REDUCED_TIME              (50U)
#define LINNM_CH0_MSG_CYCLE_OFFSET              (5U)
#define LINNM_CH0_USER_DATA_LENGTH              (6U)
#define LINNM_CH0_BUS_SYNC_ENABLED              (STD_ON)
#define LINNM_CH0_REMOTE_SLEEP_ENABLED          (STD_ON)
#define LINNM_CH0_COM_CONTROL_ENABLED           (STD_ON)
#define LINNM_CH0_COORDINATOR_SYNC              (STD_OFF)

/**
 * @brief LIN NM Channel 1 - Slave Channel
 */
#define LINNM_CHANNEL_1                         (1U)
#define LINNM_CH1_NETWORK_HANDLE                (1U)
#define LINNM_CH1_LINIF_CHANNEL                 (1U)
#define LINNM_CH1_NODE_ID                       (0x02U)
#define LINNM_CH1_NODE_TYPE                     (LINNM_NODE_TYPE_SLAVE)
#define LINNM_CH1_PASSIVE_MODE                  (STD_OFF)
#define LINNM_CH1_STATE_REPORT                  (STD_ON)
#define LINNM_CH1_TIMEOUT_TIME                  (100U)
#define LINNM_CH1_WAIT_BUSSLEEP_TIME            (50U)
#define LINNM_CH1_REMOTE_SLEEP_IND_TIME         (500U)
#define LINNM_CH1_MSG_CYCLE_TIME                (20U)
#define LINNM_CH1_MSG_REDUCED_TIME              (50U)
#define LINNM_CH1_MSG_CYCLE_OFFSET              (10U)
#define LINNM_CH1_USER_DATA_LENGTH              (6U)
#define LINNM_CH1_BUS_SYNC_ENABLED              (STD_ON)
#define LINNM_CH1_REMOTE_SLEEP_ENABLED          (STD_ON)
#define LINNM_CH1_COM_CONTROL_ENABLED           (STD_ON)
#define LINNM_CH1_COORDINATOR_SYNC              (STD_OFF)

/*==================================================================================================
*                                    PDU CONFIGURATION
==================================================================================================*/

/**
 * @brief NM PDU ID for channel 0
 */
#define LINNM_CH0_TX_PDU_ID                     (0U)
#define LINNM_CH0_RX_PDU_ID                     (0U)

/**
 * @brief NM PDU ID for channel 1
 */
#define LINNM_CH1_TX_PDU_ID                     (1U)
#define LINNM_CH1_RX_PDU_ID                     (1U)

/**
 * @brief NM PDU Size
 */
#define LINNM_PDU_SIZE                          (8U)
#define LINNM_PDU_USER_DATA_SIZE                (6U)

/**
 * @brief NM PDU Byte Position Definitions
 */
#define LINNM_PDU_BYTE_0                        (0U)
#define LINNM_PDU_BYTE_1                        (1U)
#define LINNM_PDU_BYTE_2                        (2U)
#define LINNM_PDU_BYTE_3                        (3U)
#define LINNM_PDU_BYTE_4                        (4U)
#define LINNM_PDU_BYTE_5                        (5U)
#define LINNM_PDU_BYTE_6                        (6U)
#define LINNM_PDU_BYTE_7                        (7U)

/**
 * @brief Control Bit Vector (CBV) Position and Masks
 */
#define LINNM_PDU_CBV_POS                       (0U)
#define LINNM_CBV_ACTIVEWAKEUP_MASK             (0x01U)  /**< Active wake-up bit */
#define LINNM_CBV_RESERVED_MASK                 (0xFEU)  /**< Reserved bits */

/**
 * @brief Node Identifier Byte Position
 */
#define LINNM_PDU_NODEID_POS                    (1U)

/**
 * @brief User Data Start Position
 */
#define LINNM_PDU_USERDATA_START                (2U)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/**
 * @brief State change notification callback
 * STD_ON: Call Nm_StateChangeNotification
 * STD_OFF: Don't call
 */
#ifndef LINNM_STATE_CHANGE_NOTIFICATION
#define LINNM_STATE_CHANGE_NOTIFICATION         (STD_ON)
#endif

/**
 * @brief Remote sleep indication callback
 * STD_ON: Call Nm_RemoteSleepIndication/Cancellation
 * STD_OFF: Don't call
 */
#ifndef LINNM_REMOTE_SLEEP_CALLBACK
#define LINNM_REMOTE_SLEEP_CALLBACK             (STD_ON)
#endif

/**
 * @brief Synchronization point callback
 * STD_ON: Call Nm_SynchronizationPoint
 * STD_OFF: Don't call
 */
#ifndef LINNM_SYNC_POINT_CALLBACK
#define LINNM_SYNC_POINT_CALLBACK               (STD_ON)
#endif

/*==================================================================================================
*                                    CALLBACK FUNCTION DEFINITIONS
==================================================================================================*/

/**
 * @brief Define callbacks based on configuration
 */
#if (LINNM_STATE_CHANGE_NOTIFICATION == STD_ON)
#define LINNM_CALL_STATE_CHANGE_NOTIFICATION(networkHandle, state) \
    Nm_StateChangeNotification((networkHandle), (state))
#else
#define LINNM_CALL_STATE_CHANGE_NOTIFICATION(networkHandle, state)
#endif

#if (LINNM_REMOTE_SLEEP_CALLBACK == STD_ON)
#define LINNM_CALL_REMOTE_SLEEP_INDICATION(networkHandle) \
    Nm_RemoteSleepIndication((networkHandle))
#define LINNM_CALL_REMOTE_SLEEP_CANCELLATION(networkHandle) \
    Nm_RemoteSleepCancellation((networkHandle))
#else
#define LINNM_CALL_REMOTE_SLEEP_INDICATION(networkHandle)
#define LINNM_CALL_REMOTE_SLEEP_CANCELLATION(networkHandle)
#endif

#if (LINNM_SYNC_POINT_CALLBACK == STD_ON)
#define LINNM_CALL_SYNC_POINT(networkHandle) \
    Nm_SynchronizationPoint((networkHandle))
#else
#define LINNM_CALL_SYNC_POINT(networkHandle)
#endif

#endif /* LINNM_CFG_H */
