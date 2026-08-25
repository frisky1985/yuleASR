/**
 * @file UdpNm_Lcfg.c
 * @brief UDP Network Management Link-Time Configuration
 * @version 1.0.0
 * @date 2026-05-06
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: UDP Network Management Configuration
 * Module ID: 0x33
 * Layer: Service Layer
 */
/* @req SWS_UdpNm_00001 @req SWS_UdpNm_00002 @req SWS_UdpNm_00003 */


/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "UdpNm.h"
#include "UdpNm_Cfg.h"
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                    CHANNEL CONFIGURATIONS
==================================================================================================*/
#define UDPNM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Channel 0 Configuration
 */
static const UdpNm_ChannelConfigType UdpNm_Channel0Config = {
    .ChannelId = UDPNM_CHANNEL_0,
    .NodeId = UDPNM_CHANNEL_0_NODE_ID,
    .ClusterId = UDPNM_CHANNEL_0_CLUSTER_ID,
    .PassiveModeEnabled = FALSE,
    .RepeatMessageIndEnabled = UDPNM_REPEAT_MESSAGE_IND_ENABLED,
    .NodeDetectionEnabled = UDPNM_NODE_DETECTION_ENABLED,
    .NodeIdEnabled = UDPNM_NODE_ID_ENABLED,
    .BusSynchronizationEnabled = UDPNM_BUS_SYNCHRONIZATION_ENABLED,
    .RemoteSleepIndEnabled = UDPNM_REMOTE_SLEEP_IND_ENABLED,
    .UserDataEnabled = UDPNM_USER_DATA_ENABLED,
    .UserDataOffset = UDPNM_USER_DATA_OFFSET,
    .UserDataLength = UDPNM_USER_DATA_LENGTH,
    .NodeIdPosition = UDPNM_NODE_ID_POSITION,
    .ControlBitVectorPosition = UDPNM_CONTROL_BIT_VECTOR_POSITION,
    .MsgCycleTime = UDPNM_MSG_CYCLE_TIME,
    .MsgTimeoutTime = UDPNM_MSG_TIMEOUT_TIME,
    .RepeatMessageTime = UDPNM_REPEAT_MESSAGE_TIME,
    .WaitBusSleepTime = UDPNM_WAIT_BUS_SLEEP_TIME,
    .TimeoutTime = UDPNM_TIMEOUT_TIME,
    .ImmediateNmCycleTime = UDPNM_IMMEDIATE_NM_CYCLE_TIME,
    .ImmediateNmTransmissions = UDPNM_IMMEDIATE_NM_TRANSMISIONS,
    .TxPduId = UDPNM_CHANNEL_0_TX_PDU_ID,
    .RxPduId = UDPNM_CHANNEL_0_RX_PDU_ID
};

/**
 * @brief Channel 1 Configuration
 */
static const UdpNm_ChannelConfigType UdpNm_Channel1Config = {
    .ChannelId = UDPNM_CHANNEL_1,
    .NodeId = UDPNM_CHANNEL_1_NODE_ID,
    .ClusterId = UDPNM_CHANNEL_1_CLUSTER_ID,
    .PassiveModeEnabled = FALSE,
    .RepeatMessageIndEnabled = UDPNM_REPEAT_MESSAGE_IND_ENABLED,
    .NodeDetectionEnabled = UDPNM_NODE_DETECTION_ENABLED,
    .NodeIdEnabled = UDPNM_NODE_ID_ENABLED,
    .BusSynchronizationEnabled = UDPNM_BUS_SYNCHRONIZATION_ENABLED,
    .RemoteSleepIndEnabled = UDPNM_REMOTE_SLEEP_IND_ENABLED,
    .UserDataEnabled = UDPNM_USER_DATA_ENABLED,
    .UserDataOffset = UDPNM_USER_DATA_OFFSET,
    .UserDataLength = UDPNM_USER_DATA_LENGTH,
    .NodeIdPosition = UDPNM_NODE_ID_POSITION,
    .ControlBitVectorPosition = UDPNM_CONTROL_BIT_VECTOR_POSITION,
    .MsgCycleTime = UDPNM_MSG_CYCLE_TIME,
    .MsgTimeoutTime = UDPNM_MSG_TIMEOUT_TIME,
    .RepeatMessageTime = UDPNM_REPEAT_MESSAGE_TIME,
    .WaitBusSleepTime = UDPNM_WAIT_BUS_SLEEP_TIME,
    .TimeoutTime = UDPNM_TIMEOUT_TIME,
    .ImmediateNmCycleTime = UDPNM_IMMEDIATE_NM_CYCLE_TIME,
    .ImmediateNmTransmissions = UDPNM_IMMEDIATE_NM_TRANSMISIONS,
    .TxPduId = UDPNM_CHANNEL_1_TX_PDU_ID,
    .RxPduId = UDPNM_CHANNEL_1_RX_PDU_ID
};

/**
 * @brief Channel 2 Configuration
 */
static const UdpNm_ChannelConfigType UdpNm_Channel2Config = {
    .ChannelId = UDPNM_CHANNEL_2,
    .NodeId = UDPNM_CHANNEL_2_NODE_ID,
    .ClusterId = UDPNM_CHANNEL_2_CLUSTER_ID,
    .PassiveModeEnabled = FALSE,
    .RepeatMessageIndEnabled = UDPNM_REPEAT_MESSAGE_IND_ENABLED,
    .NodeDetectionEnabled = UDPNM_NODE_DETECTION_ENABLED,
    .NodeIdEnabled = UDPNM_NODE_ID_ENABLED,
    .BusSynchronizationEnabled = UDPNM_BUS_SYNCHRONIZATION_ENABLED,
    .RemoteSleepIndEnabled = UDPNM_REMOTE_SLEEP_IND_ENABLED,
    .UserDataEnabled = UDPNM_USER_DATA_ENABLED,
    .UserDataOffset = UDPNM_USER_DATA_OFFSET,
    .UserDataLength = UDPNM_USER_DATA_LENGTH,
    .NodeIdPosition = UDPNM_NODE_ID_POSITION,
    .ControlBitVectorPosition = UDPNM_CONTROL_BIT_VECTOR_POSITION,
    .MsgCycleTime = UDPNM_MSG_CYCLE_TIME,
    .MsgTimeoutTime = UDPNM_MSG_TIMEOUT_TIME,
    .RepeatMessageTime = UDPNM_REPEAT_MESSAGE_TIME,
    .WaitBusSleepTime = UDPNM_WAIT_BUS_SLEEP_TIME,
    .TimeoutTime = UDPNM_TIMEOUT_TIME,
    .ImmediateNmCycleTime = UDPNM_IMMEDIATE_NM_CYCLE_TIME,
    .ImmediateNmTransmissions = UDPNM_IMMEDIATE_NM_TRANSMISIONS,
    .TxPduId = UDPNM_CHANNEL_2_TX_PDU_ID,
    .RxPduId = UDPNM_CHANNEL_2_RX_PDU_ID
};

/**
 * @brief Channel 3 Configuration
 */
static const UdpNm_ChannelConfigType UdpNm_Channel3Config = {
    .ChannelId = UDPNM_CHANNEL_3,
    .NodeId = UDPNM_CHANNEL_3_NODE_ID,
    .ClusterId = UDPNM_CHANNEL_3_CLUSTER_ID,
    .PassiveModeEnabled = FALSE,
    .RepeatMessageIndEnabled = UDPNM_REPEAT_MESSAGE_IND_ENABLED,
    .NodeDetectionEnabled = UDPNM_NODE_DETECTION_ENABLED,
    .NodeIdEnabled = UDPNM_NODE_ID_ENABLED,
    .BusSynchronizationEnabled = UDPNM_BUS_SYNCHRONIZATION_ENABLED,
    .RemoteSleepIndEnabled = UDPNM_REMOTE_SLEEP_IND_ENABLED,
    .UserDataEnabled = UDPNM_USER_DATA_ENABLED,
    .UserDataOffset = UDPNM_USER_DATA_OFFSET,
    .UserDataLength = UDPNM_USER_DATA_LENGTH,
    .NodeIdPosition = UDPNM_NODE_ID_POSITION,
    .ControlBitVectorPosition = UDPNM_CONTROL_BIT_VECTOR_POSITION,
    .MsgCycleTime = UDPNM_MSG_CYCLE_TIME,
    .MsgTimeoutTime = UDPNM_MSG_TIMEOUT_TIME,
    .RepeatMessageTime = UDPNM_REPEAT_MESSAGE_TIME,
    .WaitBusSleepTime = UDPNM_WAIT_BUS_SLEEP_TIME,
    .TimeoutTime = UDPNM_TIMEOUT_TIME,
    .ImmediateNmCycleTime = UDPNM_IMMEDIATE_NM_CYCLE_TIME,
    .ImmediateNmTransmissions = UDPNM_IMMEDIATE_NM_TRANSMISIONS,
    .TxPduId = UDPNM_CHANNEL_3_TX_PDU_ID,
    .RxPduId = UDPNM_CHANNEL_3_RX_PDU_ID
};

/**
 * @brief Channel Configuration Array
 */
static const UdpNm_ChannelConfigType* const UdpNm_ChannelConfigArray[UDPNM_NUMBER_OF_CHANNELS] = {
    &UdpNm_Channel0Config,
    &UdpNm_Channel1Config,
    &UdpNm_Channel2Config,
    &UdpNm_Channel3Config
};

/*==================================================================================================
*                                    GLOBAL CONFIGURATION
==================================================================================================*/

/**
 * @brief UDP NM Configuration
 */
const UdpNm_ConfigType UdpNm_Config = {
    .ChannelConfig = UdpNm_ChannelConfigArray[0],
    .NumberOfChannels = UDPNM_NUMBER_OF_CHANNELS,
    .DevErrorDetect = UDPNM_DEV_ERROR_DETECT,
    .VersionInfoApi = UDPNM_VERSION_INFO_API,
    .BusLoadReductionEnabled = UDPNM_BUS_LOAD_REDUCTION_ENABLED,
    .ComControlEnabled = TRUE,
    .PnEnabled = UDPNM_PARTIAL_NETWORKING_ENABLED,
    .MainFunctionPeriod = UDPNM_MAIN_FUNCTION_PERIOD
};

#define UDPNM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    END OF FILE
==================================================================================================*/
