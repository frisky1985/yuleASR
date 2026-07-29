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
 * @file BswM.h
 * @brief Basic Software Mode Manager
 * @version 1.0.0
 */

#ifndef BSWM_H
#define BSWM_H

#include "Std_Types.h"

#define BSWM_MODULE_ID          42U
#define BSWM_VENDOR_ID          0x0001U

/* Error Codes */
#define BSWM_E_NO_ERROR         0x00U
#define BSWM_E_PARAM_POINTER    0x01U
#define BSWM_E_UNINIT           0x02U
#define BSWM_E_PARAM_INVALID    0x03U

/* Service IDs */
#define BSWM_SID_INIT                   0x00U
#define BSWM_SID_DEINIT                 0x01U
#define BSWM_SID_GET_VERSION_INFO       0x02U
#define BSWM_SID_REQUEST_MODE           0x03U
#define BSWM_SID_MAIN_FUNCTION          0x04U

/* Mode Request Source Types */
typedef enum {
    BSWM_GENERIC_REQUEST = 0,
    BSWM_ECUM_REQUEST,
    BSWM_COMM_REQUEST,
    BSWM_DCM_REQUEST,
    BSWM_NVM_REQUEST,
    BSWM_SWC_REQUEST
} BswM_ModeRequestSourceType;

/* Mode Type */
typedef uint8 BswM_ModeType;

/* Mode Request Port */
typedef struct {
    uint16 PortId;
    BswM_ModeRequestSourceType SourceType;
    BswM_ModeType CurrentMode;
    boolean IsValid;
} BswM_ModeRequestPortType;

/* Action Type */
typedef enum {
    BSWM_ACTION_SCHEDULE = 0,
    BSWM_ACTION_SWITCH_MODE,
    BSWM_ACTION_EXECUTE_ACTION_LIST,
    BSWM_ACTION_USER_CALL
} BswM_ActionItemType;

/* Action Structure */
typedef struct {
    uint16 ActionId;
    BswM_ActionItemType ActionType;
    uint16 TargetId;
} BswM_ActionType;

/* Rule Structure */
typedef struct {
    uint16 RuleId;
    uint16 ModeRequestPortId;
    BswM_ModeType ExpectedMode;
    uint16 ActionListId;
    boolean IsActive;
} BswM_RuleType;

/* Configuration */
typedef struct {
    uint16 NumModeRequestPorts;
    uint16 NumRules;
    uint16 NumActionLists;
    const BswM_ModeRequestPortType* ModeRequestPorts;
    const BswM_RuleType* Rules;
} BswM_ConfigType;

/* Functions */
void BswM_Init(const BswM_ConfigType* ConfigPtr);
void BswM_DeInit(void);
#if (BSWM_VERSION_INFO_API == STD_ON)
void BswM_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif
void BswM_RequestMode(uint16 PortId, BswM_ModeType Mode);
void BswM_MainFunction(void);

/* Callbacks */
void BswM_EcuM_CurrentState(uint8 State);
void BswM_EcuM_CurrentWakeup(uint32 Sources, uint8 Status);
void BswM_ComM_CurrentMode(uint8 Network, uint8 Mode);
void BswM_Dcm_RequestCommunicationMode(uint8 Mode);

#endif
