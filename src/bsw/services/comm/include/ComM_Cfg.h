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
 * @file ComM_Cfg.h
 * @brief AUTOSAR Communication Manager Configuration Header
 * @version 1.0.0
 * 
 * Configuration parameters for the Communication Manager module.
 * Defines channels, users, network modes, and Partial Network Cluster (PNC) settings.
 */

#ifndef COMM_CFG_H
#define COMM_CFG_H

#include "Std_Types.h"

/*=============================================================================
 * Module Configuration Switches
 *===========================================================================*/
#define COMM_DEV_ERROR_DETECT               STD_ON
#define COMM_VERSION_INFO_API               STD_ON
#define COMM_ENABLE_WAKEUP                  STD_ON
#define COMM_WAKEUP_INHIBITION_ENABLED      STD_ON
#define COMM_LIMIT_TO_NO_COM_ENABLED        STD_ON
#define COMM_PNC_SUPPORT                    STD_ON
#define COMM_DCM_SUPPORT                    STD_ON
#define COMM_ECUM_SUPPORT                   STD_ON
#define COMM_NVM_BLOCK_DESCRIPTOR           STD_ON
#define COMM_NVM_STORAGE_ENABLED            STD_ON

/*=============================================================================
 * Channel Configuration
 *===========================================================================*/
#define COMM_NUM_CHANNELS                   4U
#define COMM_NUM_USERS                      8U
#define COMM_NUM_PNCS                       2U

/* Channel Handle Definitions */
#define COMM_CHANNEL_CAN0                   0U
#define COMM_CHANNEL_CAN1                   1U
#define COMM_CHANNEL_ETH0                   2U
#define COMM_CHANNEL_LIN0                   3U

/* User Handle Definitions */
#define COMM_USER_DCM                       0U
#define COMM_USER_DEM                       1U
#define COMM_USER_NVM                       2U
#define COMM_USER_ECUM                      3U
#define COMM_USER_SWC0                      4U
#define COMM_USER_SWC1                      5U
#define COMM_USER_DIAG                      6U
#define COMM_USER_APPL                      7U

/* PNC Handle Definitions */
#define COMM_PNC_0                          0U
#define COMM_PNC_1                          1U

/*=============================================================================
 * Timing Configuration (in ms)
 *===========================================================================*/
#define COMM_MAIN_FUNCTION_PERIOD           10U
#define COMM_MINIMUM_INHIBIT_TIME           5000U
#define COMM_NVM_STORE_RETRY_TIME           1000U
#define COMM_T_MAX_NVM_STORE                500U
#define COMM_T_WAKEUP                         100U

/* Bus Wake-up Configuration */
#define COMM_BUS_WAKEUP_DELAY               50U
#define COMM_WAKEUP_RETRY_LIMIT             3U

/* Channel Timing */
#define COMM_CHANNEL_NOCOM_TIMEOUT          1000U
#define COMM_CHANNEL_SILENT_TIMEOUT         500U
#define COMM_CHANNEL_FULLCOM_TIMEOUT        200U

/* PNC Timing */
#define COMM_PNC_PREPARE_SLEEP_TIMEOUT      300U
#define COMM_PNC_REQUEST_TIMEOUT            100U

/*=============================================================================
 * Feature Configuration
 *===========================================================================*/
#define COMM_CHANNEL_WAKEUP_SUPPORT         STD_ON
#define COMM_CHANNEL_DCM_SUPPORT            STD_ON
#define COMM_CHANNEL_PASSIVE_WAKEUP         STD_ON

/*=============================================================================
 * Data Types (using uint8 as base type; actual ComM typedefs in ComM.h)
 *===========================================================================*/

/* Channel Configuration Structure */
typedef struct {
    uint8 ChannelId;
    uint8 BusType;
    boolean WakeUpSupport;
    boolean DcmSupport;
    boolean PassiveWakeUp;
    uint16 NoComTimeout;
    uint16 SilentTimeout;
    uint16 FullComTimeout;
    uint16 WakeUpDelay;
} ComM_ChannelConfigType;

/* User Configuration Structure */
typedef struct {
    uint8 UserId;
    const uint8* ChannelMap;
    uint8 NumChannels;
    const uint8* PncMap;
    uint8 NumPncs;
} ComM_UserConfigType;

/* PNC Channel Mapping */
typedef struct {
    uint8 ChannelId;
    boolean IsRequester;
} ComM_PncChannelMappingType;

/* PNC Configuration Structure */
typedef struct {
    uint8 PncId;
    const ComM_PncChannelMappingType* ChannelMap;
    uint8 NumChannels;
    uint16 PrepareSleepTimeout;
    uint16 RequestTimeout;
    boolean WakeUpSupport;
} ComM_PncConfigType;

/* Request Manager Entry */
typedef struct {
    uint8 RequestedMode;
    boolean Active;
} ComM_UserRequestType;

/* Channel State Structure */
typedef struct {
    uint8 State;
    uint8 CurrentMode;
    uint8 RequestedMode;
    boolean CommunicationAllowed;
    boolean WakeUpInhibition;
    boolean LimitToNoCom;
    boolean DcmActive;
    boolean PassiveDiagnostic;
    uint32 TimeoutCounter;
    uint32 WakeUpRetryCounter;
    uint8 UserRequestCount;
} ComM_ChannelStateStrType;

/* PNC State Structure */
typedef struct {
    uint8 Mode;
    boolean RequestActive;
    uint32 TimeoutCounter;
    uint8 ActiveRequestCount;
} ComM_PncStateType;

/*=============================================================================
 * External Configuration Declarations
 *===========================================================================*/
extern const ComM_ChannelConfigType ComM_ChannelConfig[COMM_NUM_CHANNELS];
extern const ComM_ChannelStateStrType ComM_ChannelStateConfig[COMM_NUM_CHANNELS];
extern const ComM_UserConfigType ComM_UserConfig[COMM_NUM_USERS];
extern const ComM_PncConfigType ComM_PncConfig[COMM_NUM_PNCS];

/* Channel Mappings for Users */
extern const uint8 ComM_User0_Channels[];
extern const uint8 ComM_User1_Channels[];
extern const uint8 ComM_User2_Channels[];
extern const uint8 ComM_User3_Channels[];
extern const uint8 ComM_User4_Channels[];
extern const uint8 ComM_User5_Channels[];
extern const uint8 ComM_User6_Channels[];
extern const uint8 ComM_User7_Channels[];

/* PNC Mappings for Users */
extern const uint8 ComM_User0_Pncs[];
extern const uint8 ComM_User1_Pncs[];
extern const uint8 ComM_User2_Pncs[];
extern const uint8 ComM_User3_Pncs[];
extern const uint8 ComM_User4_Pncs[];
extern const uint8 ComM_User5_Pncs[];
extern const uint8 ComM_User6_Pncs[];
extern const uint8 ComM_User7_Pncs[];

/* PNC Channel Mappings */
extern const ComM_PncChannelMappingType ComM_Pnc0_Channels[];
extern const ComM_PncChannelMappingType ComM_Pnc1_Channels[];

/*=============================================================================
 * Default Configuration Values
 *===========================================================================*/
#define COMM_DEFAULT_MODE                   COMM_NO_COMMUNICATION
#define COMM_DEFAULT_CHANNEL_STATE          COMM_CHANNEL_STATE_NOCOM
#define COMM_DEFAULT_PNC_MODE               COMM_PNC_NO_COMMUNICATION

#endif /* COMM_CFG_H */
