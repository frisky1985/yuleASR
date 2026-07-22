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
 * @file CanNm_Lcfg.c
 * @brief CAN Network Management link-time configuration file
 * @version 4.4.0
 *
 * AUTOSAR CAN Network Management Module Link-Time Configuration
 * Following AUTOSAR_SWS_CANNetworkManagement specification version 4.4.0
 */

/*==================================================================================================
 *                                           INCLUDES
 ==================================================================================================*/
#include "CanNm.h"
#include "CanNm_Cfg.h"

/*==================================================================================================
 *                                      LOCAL DEFINES
 ==================================================================================================*/

/*==================================================================================================
 *                               LOCAL CONSTANTS
 ==================================================================================================*/

extern const Std_VersionInfoType CanNm_VersionInfo;
/**
 * @brief Channel 0 configuration
 * This is the default/powertrain CAN channel
 */
static const CanNm_ChannelConfigType CanNm_Channel0_Config =
{
    /* ChannelId */                 CANNM_CHANNEL_0,
    /* NmTimeoutTime */             CANNM_NM_TIMEOUT_TIME_DEFAULT,      /* 1000ms */
    /* RepeatMessageTime */         CANNM_REPEAT_MESSAGE_TIME_DEFAULT,  /* 400ms */
    /* WaitBusSleepTime */          CANNM_WAIT_BUS_SLEEP_TIME_DEFAULT,  /* 1000ms */
    /* MessageCycleTime */          CANNM_MESSAGE_CYCLE_TIME_DEFAULT,   /* 100ms */
    /* MessageCycleOffset */        CANNM_MESSAGE_CYCLE_OFFSET_DEFAULT, /* 10ms */
    /* ImmediateNmCycleTime */      CANNM_IMMEDIATE_NM_CYCLE_TIME,      /* 20ms */
    /* ImmediateNmTransmissions */  CANNM_IMMEDIATE_NM_TRANSMSSIONS,    /* 3 transmissions */
    /* NidPosition */               CANNM_PDU_NID_POSITION,             /* Byte 0 */
    /* CbvPosition */               CANNM_PDU_CBV_POSITION,             /* Byte 1 */
    /* NodeDetectionEnabled */      (boolean)CANNM_NODE_DETECTION_ENABLED,
    /* NodeIdEnabled */             (boolean)CANNM_NODE_ID_ENABLED,
    /* PassiveModeEnabled */        (boolean)CANNM_PASSIVE_MODE_ENABLED,
    /* RemoteSleepIndEnabled */     (boolean)CANNM_REMOTE_SLEEP_IND_ENABLED,
    /* ActiveWakeupBitEnabled */    (boolean)CANNM_ACTIVE_WAKEUP_BIT_ENABLED,
    /* ComControlEnabled */         (boolean)CANNM_COM_CONTROL_ENABLED,
    /* CoordinatorSyncSupport */    (boolean)CANNM_COORDINATOR_SUPPORT_ENABLED,
    /* PduLength */                 CANNM_PDU_LENGTH,                   /* 8 bytes */
    /* NodeId */                    CANNM_CHANNEL_0_NODE_ID,            /* Node ID 0x01 */
    /* TxPduId */                   CANNM_CHANNEL_0_TX_PDUID,           /* Tx PDU handle 0 */
    /* RxPduId */                   CANNM_CHANNEL_0_RX_PDUID            /* Rx PDU handle 0 */
};

/**
 * @brief Channel 1 configuration
 * This is the body/chassis CAN channel
 */
static const CanNm_ChannelConfigType CanNm_Channel1_Config =
{
    /* ChannelId */                 CANNM_CHANNEL_1,
    /* NmTimeoutTime */             CANNM_NM_TIMEOUT_TIME_DEFAULT,      /* 1000ms */
    /* RepeatMessageTime */         CANNM_REPEAT_MESSAGE_TIME_DEFAULT,  /* 400ms */
    /* WaitBusSleepTime */          CANNM_WAIT_BUS_SLEEP_TIME_DEFAULT,  /* 1000ms */
    /* MessageCycleTime */          (uint16)150U,                       /* 150ms - different from ch0 */
    /* MessageCycleOffset */        CANNM_MESSAGE_CYCLE_OFFSET_DEFAULT, /* 10ms */
    /* ImmediateNmCycleTime */      CANNM_IMMEDIATE_NM_CYCLE_TIME,      /* 20ms */
    /* ImmediateNmTransmissions */  CANNM_IMMEDIATE_NM_TRANSMSSIONS,    /* 3 transmissions */
    /* NidPosition */               CANNM_PDU_NID_POSITION,             /* Byte 0 */
    /* CbvPosition */               CANNM_PDU_CBV_POSITION,             /* Byte 1 */
    /* NodeDetectionEnabled */      (boolean)CANNM_NODE_DETECTION_ENABLED,
    /* NodeIdEnabled */             (boolean)CANNM_NODE_ID_ENABLED,
    /* PassiveModeEnabled */        (boolean)FALSE,                     /* Active mode */
    /* RemoteSleepIndEnabled */     (boolean)CANNM_REMOTE_SLEEP_IND_ENABLED,
    /* ActiveWakeupBitEnabled */    (boolean)CANNM_ACTIVE_WAKEUP_BIT_ENABLED,
    /* ComControlEnabled */         (boolean)CANNM_COM_CONTROL_ENABLED,
    /* CoordinatorSyncSupport */    (boolean)CANNM_COORDINATOR_SUPPORT_ENABLED,
    /* PduLength */                 CANNM_PDU_LENGTH,                   /* 8 bytes */
    /* NodeId */                    CANNM_CHANNEL_1_NODE_ID,            /* Node ID 0x02 */
    /* TxPduId */                   CANNM_CHANNEL_1_TX_PDUID,           /* Tx PDU handle 1 */
    /* RxPduId */                   CANNM_CHANNEL_1_RX_PDUID            /* Rx PDU handle 1 */
};

/*==================================================================================================
 *                               GLOBAL CONSTANTS
 ==================================================================================================*/

/**
 * @brief Channel configuration array
 * This array contains all configured CAN NM channels
 */
static const CanNm_ChannelConfigType CanNm_ChannelConfig[CANNM_NUMBER_OF_CHANNELS] =
{
    CanNm_Channel0_Config,
    CanNm_Channel1_Config
};

/**
 * @brief Global CAN NM configuration structure
 * This is the main configuration structure used by the CanNm module
 */
const CanNm_ConfigType CanNm_Config =
{
    /* ChannelConfig */             CanNm_ChannelConfig,                /* Pointer to channel configs */
    /* ChannelCount */              CANNM_NUMBER_OF_CHANNELS,           /* 2 channels */
    /* VersionInfoApi */            (boolean)CANNM_VERSION_INFO_API,    /* Enabled */
    /* PassiveModeEnabled */        (boolean)CANNM_PASSIVE_MODE_ENABLED,/* Disabled globally */
    /* PnEnabled */                 (boolean)CANNM_PN_ENABLED,          /* Enabled */
    /* StateReportEnabled */        (boolean)CANNM_STATE_CHANGE_IND_ENABLED /* Enabled */
};

/*==================================================================================================
 *                                    TX PDU CONFIGURATION
 ==================================================================================================*/

/**
 * @brief TX PDU configuration table for CanIf integration
 * Maps CanNm Tx PDUs to CanIf PDU handles
 */
static const PduIdType CanNmTxPdu[CANNM_NUMBER_OF_CHANNELS] =
{
    CANNM_CHANNEL_0_TX_PDUID,
    CANNM_CHANNEL_1_TX_PDUID
};

/*==================================================================================================
 *                                    RX PDU CONFIGURATION
 ==================================================================================================*/

/**
 * @brief RX PDU configuration table
 * Maps CanNm Rx PDUs to incoming PDU handles
 */
static const PduIdType CanNmRxPdu[CANNM_NUMBER_OF_CHANNELS] =
{
    CANNM_CHANNEL_0_RX_PDUID,
    CANNM_CHANNEL_1_RX_PDUID
};

/*==================================================================================================
 *                               CHANNEL HANDLE MAPPING
 ==================================================================================================*/

/**
 * @brief ComM channel handle to CanNm channel index mapping
 * Used to convert ComM channel handles to CanNm internal indices
 */
static const NetworkHandleType CanNm_ComMChannelMapping[CANNM_NUMBER_OF_CHANNELS] =
{
    CANNM_CHANNEL_0,
    CANNM_CHANNEL_1
};

/*==================================================================================================
 *                                  VERSION INFORMATION
 ==================================================================================================*/

/**
 * @brief Module version information (link-time constant)
 */
static const Std_VersionInfoType CanNm_VersionInfo =
{
    /* vendorID */                  CANNM_VENDOR_ID,
    /* moduleID */                  CANNM_MODULE_ID,
    /* sw_major_version */          CANNM_SW_MAJOR_VERSION,
    /* sw_minor_version */          CANNM_SW_MINOR_VERSION,
    /* sw_patch_version */          CANNM_SW_PATCH_VERSION
};

/*==================================================================================================
 *                                    END OF FILE
 ==================================================================================================*/
