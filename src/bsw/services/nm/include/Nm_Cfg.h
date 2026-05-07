/**
 * @file Nm_Cfg.h
 * @brief Network Management Configuration
 */

#ifndef NM_CFG_H
#define NM_CFG_H

#include "Nm.h"

/* Development Error Detection */
#define NM_DEV_ERROR_DETECT               STD_ON
#define NM_VERSION_INFO_API               STD_ON

/* Number of Channels */
#define NM_MAX_CHANNELS                   8

/* Node Configuration */
#define NM_NODE_ID                        0x01
#define NM_NODE_COUNT                     4

/* Timeouts (in ms) */
#define NM_TIMEOUT_TIME                   1000
#define NM_REPEAT_MESSAGE_TIME            100
#define NM_WAIT_BUS_SLEEP_TIME            500
#define NM_REMOTE_SLEEP_IND_TIME          2000

/* Main Function Period */
#define NM_MAIN_FUNCTION_PERIOD           10

/* Coordinator Support */
#define NM_COORDINATOR_SUPPORT_ENABLED    STD_ON
#define NM_COORDINATOR_SYNC_SUPPORT       STD_ON
#define NM_CLUSTER_COUNT                  2

/* Passive Mode */
#define NM_PASSIVE_MODE_ENABLED           STD_OFF

/* Car Wakeup Support */
#define NM_CAR_WAKEUP_RX_ENABLED          STD_OFF
#define NM_CAR_WAKEUP_TX_ENABLED          STD_OFF

/* PN (Partial Networking) Support */
#define NM_PARTIAL_NETWORKING_ENABLED     STD_OFF
#define NM_PN_EIRA_CALCULATION_ENABLED    STD_OFF
#define NM_PN_ERA_CALCULATION_ENABLED     STD_OFF

/* Callback Configuration */
#define NM_STATE_CHANGE_IND_ENABLED       STD_ON
#define NM_REMOTE_SLEEP_IND_ENABLED       STD_ON
#define NM_BUS_SYNCHRONIZATION_ENABLED    STD_OFF

/* Channel IDs */
#define NM_CHANNEL_CAN0                   0x00
#define NM_CHANNEL_CAN1                   0x01
#define NM_CHANNEL_LIN0                   0x02
#define NM_CHANNEL_ETH0                   0x03

/* ComM Integration */
#define NM_COMM_ENABLED                   STD_ON

/* Callback Function Declarations */
#define NM_STATE_CHANGE_NOTIFICATION(ch, prev, curr) \
    Appl_Nm_StateChangeNotification(ch, prev, curr)

#define NM_REMOTE_SLEEP_INDICATION(ch) \
    Appl_Nm_RemoteSleepIndication(ch)

#define NM_REMOTE_SLEEP_CANCELLATION(ch) \
    Appl_Nm_RemoteSleepCancellation(ch)

#endif /* NM_CFG_H */
