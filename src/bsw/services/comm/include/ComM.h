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
 * @file ComM.h
 * @brief AUTOSAR Communication Manager API Header
 * @version 1.0.0
 * 
 * AUTOSAR Communication Manager (ComM) manages communication modes and ECU states.
 * Provides API for requesting communication modes, managing channel states,
 * Partial Network Cluster (PNC) handling, and bus wake-up support.
 */

#ifndef COMM_H
#define COMM_H

#include "Std_Types.h"
#include "ComM_Cfg.h"

/* AUTOSAR Version */
#define COMM_AR_RELEASE_MAJOR_VERSION       4
#define COMM_AR_RELEASE_MINOR_VERSION       0
#define COMM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define COMM_SW_MAJOR_VERSION               1
#define COMM_SW_MINOR_VERSION               0
#define COMM_SW_PATCH_VERSION               0

/* Module ID */
#define COMM_MODULE_ID                      0x12

/* Service IDs */
#define COMM_INIT_SID                       0x01
#define COMM_DEINIT_SID                     0x02
#define COMM_GETVERSIONINFO_SID             0x03
#define COMM_REQUESTCOMMODE_SID             0x04
#define COMM_GETMAXCOMMODE_SID              0x05
#define COMM_GETREQUESTEDCOMMODE_SID        0x06
#define COMM_GETCURRENTCOMMODE_SID          0x07
#define COMM_COMMUNICATIONALLOWED_SID       0x08
#define COMM_MAINFUNCTION_SID               0x60
#define COMM_MAINFUNCTIONPNC_SID            0x61
#define COMM_BUSSM_MODEINDICATION_SID       0x62
#define COMM_ECUM_WAKEUPINDICATION_SID      0x63
#define COMM_DCM_ACTIVATEDIAGNOSTIC_SID     0x64
#define COMM_DCM_INACTIVATEDIAGNOSTIC_SID   0x65
#define COMM_NVM_STARTUPERROR_SID           0x66
#define COMM_ECNM_BUSSLEEPMODE_SID          0x67
#define COMM_ECNM_NETWORKMODE_SID           0x68
#define COMM_ECNM_PREPAREBUSSLEEPMODE_SID   0x69

/* Error Codes */
#define COMM_E_NOT_INIT                     0x01
#define COMM_E_WRONG_PARAMETERS             0x02
#define COMM_E_ERROR_IN_PROV_SERVICE        0x03
#define COMM_E_UNINIT                       0x04
#define COMM_E_PARAM_POINTER                0x05
#define COMM_E_PARAM_CHANNEL                0x06
#define COMM_E_PARAM_USER                   0x07
#define COMM_E_PARAM_PNC                    0x08

/* Communication Modes */
typedef uint8 ComM_ModeType;
#define COMM_NO_COMMUNICATION               0x00
#define COMM_SILENT_COMMUNICATION           0x01
#define COMM_FULL_COMMUNICATION             0x02

/* Inhibition Status */
typedef uint8 ComM_InhibitionStatusType;
#define COMM_INHIBITION_STATUS_NONE         0x00
#define COMM_INHIBITION_STATUS_WAKEUP       0x01
#define COMM_INHIBITION_STATUS_LIMIT_TO_NO_COM  0x02

/* PNC Mode Type */
typedef uint8 ComM_PncModeType;
#define COMM_PNC_REQUESTED                  0x00
#define COMM_PNC_READY_SLEEP                0x01
#define COMM_PNC_PREPARE_SLEEP              0x02
#define COMM_PNC_NO_COMMUNICATION           0x03

/* State Types */
typedef uint8 ComM_StateType;
#define COMM_STATE_UNINIT                   0x00
#define COMM_STATE_INIT                     0x01
#define COMM_STATE_READY                    0x02

/* Channel State Types */
typedef uint8 ComM_ChannelStateType;
#define COMM_CHANNEL_STATE_NOCOM            0x00
#define COMM_CHANNEL_STATE_SILENTCOM        0x01
#define COMM_CHANNEL_STATE_FULLCOM          0x02
#define COMM_CHANNEL_STATE_PENDING          0x03

/* Bus Types */
typedef uint8 ComM_BusTypeType;
#ifndef COMM_BUS_TYPE_CAN
#define COMM_BUS_TYPE_CAN                   0x00
#define COMM_BUS_TYPE_ETH                   0x01
#define COMM_BUS_TYPE_LIN                   0x02
#define COMM_BUS_TYPE_FR                    0x03
#define COMM_BUS_TYPE_INTERNAL              0x04
#endif

/* Handle Types */
typedef uint8 ComM_UserHandleType;
typedef uint8 ComM_ChannelHandleType;
typedef uint8 ComM_PncHandleType;

/* Wake-up Reason */
typedef uint8 ComM_EcuM_WakeUpType;
#define COMM_WAKEUP_COMM                    0x00
#define COMM_WAKEUP_DIAG                    0x01
#define COMM_WAKEUP_ECU                     0x02

/* Diagnostic Request State */
typedef uint8 ComM_DiagRequestStateType;
#define COMM_DIAG_REQ_NONE                  0x00
#define COMM_DIAG_REQ_ACTIVE                0x01
#define COMM_DIAG_REQ_PASSIVE               0x02

/* Version Information Type */
#ifndef STD_VERSIONINFO_TYPE_DEFINED
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;
#endif

/* Configuration Type */
typedef struct {
    const ComM_ChannelConfigType* ChannelConfigs;
    const ComM_UserConfigType* UserConfigs;
    const ComM_PncConfigType* PncConfigs;
    uint8 NumChannels;
    uint8 NumUsers;
    uint8 NumPncs;
    uint8 BusWakeUpDelay;
    boolean PncSupportEnabled;
    boolean DcmSupportEnabled;
    boolean EcuMSupportEnabled;
} ComM_ConfigType;

/* Function Prototypes - Core API */
extern void ComM_Init(const ComM_ConfigType* ConfigPtr);
extern void ComM_DeInit(void);
extern void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/* Function Prototypes - Communication Mode Management */
extern Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode);
extern Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);

/* Function Prototypes - Channel Management */
extern void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed);
extern void ComM_MainFunction(void);

/* Function Prototypes - PNC Management */
extern void ComM_MainFunctionPnc(void);
extern Std_ReturnType ComM_RequestPncMode(ComM_PncHandleType Pnc, ComM_PncModeType PncMode);
extern Std_ReturnType ComM_GetPncMode(ComM_PncHandleType Pnc, ComM_PncModeType* PncModePtr);

/* Function Prototypes - ECU State Manager Integration */
extern void ComM_EcuM_WakeUpIndication(ComM_EcuM_WakeUpType WakeupType);
extern void ComM_EcuM_BusWakeUpIndication(ComM_ChannelHandleType Channel);
extern void ComM_EcuM_RunRequestIndication(boolean Requested);

/* Function Prototypes - Bus State Manager Interface */
extern void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode);
extern void ComM_BusSM_BusSleepMode(ComM_ChannelHandleType Channel);
extern void ComM_BusSM_NetworkMode(ComM_ChannelHandleType Channel);
extern void ComM_BusSM_PrepareBusSleepMode(ComM_ChannelHandleType Channel);

/* Function Prototypes - DCM Integration */
extern Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel);
extern Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel);
extern Std_ReturnType ComM_DCM_PassiveDiagnostic(ComM_ChannelHandleType Channel, boolean Active);

/* Function Prototypes - ECNM Integration */
extern void ComM_ECNM_NetworkMode(ComM_ChannelHandleType Channel);
extern void ComM_ECNM_PrepareBusSleepMode(ComM_ChannelHandleType Channel);
extern void ComM_ECNM_BusSleepMode(ComM_ChannelHandleType Channel);

/* Function Prototypes - NVM Integration */
extern void ComM_Nvm_StartUpError(void);
extern void ComM_Nvm_StoreInhibitionStatus(void);

/* Function Prototypes - Diagnostic Support */
extern Std_ReturnType ComM_GetInhibitionStatus(ComM_ChannelHandleType Channel, ComM_InhibitionStatusType* StatusPtr);
extern void ComM_LimitChannelToNoComMode(ComM_ChannelHandleType Channel, boolean Status);
extern void ComM_LimitECUToNoComMode(boolean Status);
extern void ComM_PreventWakeUp(ComM_ChannelHandleType Channel, boolean Status);

#endif /* COMM_H */
