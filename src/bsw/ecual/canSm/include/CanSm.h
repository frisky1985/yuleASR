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

/*******************************************************************************
* File: CanSm.h
* Description: Public API header for CAN State Manager (CanSm)
*              AUTOSAR SWS CANStateManager 4.4.0 compliant
*******************************************************************************/

#ifndef CANSM_H
#define CANSM_H

/*******************************************************************************
* Includes
*******************************************************************************/
#include "Std_Types.h"
#include "ComStack_Types.h"
#include "CanSm_Cfg.h"
#include "ComM.h"
#include "CanIf.h"

#if (CANSM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*******************************************************************************
* Version Information
*******************************************************************************/
#define CANSM_SW_MAJOR_VERSION      4U
#define CANSM_SW_MINOR_VERSION      4U
#define CANSM_SW_PATCH_VERSION      0U

/*******************************************************************************
* Type Definitions
*******************************************************************************/

/* Network handle type - identifies a CAN network */
typedef uint8 CanSm_NetworkHandleType;

/* Controller handle type - identifies a CAN controller */
typedef uint8 CanSm_ControllerHandleType;

/* Transceiver handle type - identifies a CAN transceiver */
typedef uint8 CanSm_TransceiverHandleType;

/* CANSM internal states for a network */
typedef enum
{
    CANSM_NO_COM = 0,           /* No communication - bus is OFF */
    CANSM_SILENT_COM,           /* Silent communication - only receiving */
    CANSM_FULL_COM,             /* Full communication - TX and RX active */
    CANSM_SILENT_COM_BUS_OFF,   /* Silent with bus-off recovery ongoing */
    CANSM_FULL_COM_BUS_OFF      /* Full communication with bus-off recovery */
} CanSm_NetworkStateType;

/* Bus-off recovery substates */
typedef enum
{
    CANSM_BOR_IDLE = 0,         /* No bus-off detected */
    CANSM_BOR_WAIT_RESTART,     /* Waiting T_RESTART before restarting */
    CANSM_BOR_WAIT_RECOVERY,    /* Waiting T_RECOVERY before retry */
    CANSM_BOR_FAILED            /* Bus-off recovery failed (max retries exceeded) */
} CanSm_BusOffRecoveryStateType;

/* Internal mode request states */
typedef enum
{
    CANSM_REQ_NONE = 0,         /* No mode change requested */
    CANSM_REQ_NO_COM,           /* Request to go to NO_COM */
    CANSM_REQ_SILENT_COM,       /* Request to go to SILENT_COM */
    CANSM_REQ_FULL_COM          /* Request to go to FULL_COM */
} CanSm_InternalReqType;

/* Configuration structure for a CAN controller */
typedef struct
{
    CanSm_ControllerHandleType  ControllerId;
    CanIf_ControllerModeType    InitialMode;
} CanSm_ControllerConfigType;

/* Configuration structure for a CAN transceiver */
typedef struct
{
    CanSm_TransceiverHandleType TransceiverId;
    CanIf_TransceiverModeType   InitialMode;
    boolean                     WakeupSupport;
} CanSm_TransceiverConfigType;

/* Configuration structure for a CAN network */
typedef struct
{
    CanSm_NetworkHandleType     NetworkId;
    uint8                       ControllerCount;
    const CanSm_ControllerConfigType* ControllerRefs;
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    uint8                       TransceiverCount;
    const CanSm_TransceiverConfigType* TransceiverRefs;
#endif
    float32                     TRestart;       /* Bus-off restart time in seconds */
    float32                     TRecovery;      /* Bus-off recovery time in seconds */
    uint8                       BusOffMaxRetries;
} CanSm_NetworkConfigType;

/* CanSm configuration type */
typedef struct
{
    uint8                           NetworkCount;
    const CanSm_NetworkConfigType*  Networks;
} CanSm_ConfigType;

/* Network runtime data structure (internal use) */
typedef struct
{
    CanSm_NetworkStateType          CurrentState;
    CanSm_NetworkStateType          PreviousState;
    CanSm_InternalReqType           RequestedMode;
    ComM_ModeType                   ComMode;
    boolean                         BusOffDetected;
    CanSm_BusOffRecoveryStateType   BusOffState;
    uint8                           BusOffRetryCount;
    float32                         BusOffTimer;
    boolean                         WakeupPending;
    boolean                         ModeChangePending;
    CanIf_ControllerModeType        RequestedControllerMode;
    CanIf_ControllerModeType        IndicatedControllerMode;
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    CanIf_TransceiverModeType       RequestedTransceiverMode;
    CanIf_TransceiverModeType       IndicatedTransceiverMode;
#endif
} CanSm_NetworkRuntimeType;

/*******************************************************************************
* External Variables
*******************************************************************************/
#define CANSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Runtime data for all networks - defined in CanSm.c */
extern CanSm_NetworkRuntimeType CanSm_NetworkRuntime[CANSM_NETWORK_COUNT];

#define CANSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define CANSM_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* Link-time configuration - defined in CanSm_Lcfg.c */
extern const CanSm_ConfigType CanSm_Config;

#define CANSM_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

#define CANSM_START_SEC_CODE
#include "MemMap.h"

/*******************************************************************************
* Name: CanSm_Init
* Description: Initializes the CanSm module
* Parameters: ConfigPtr - Pointer to configuration structure
* Returns: None
*******************************************************************************/
extern void CanSm_Init(const CanSm_ConfigType* ConfigPtr);

/*******************************************************************************
* Name: CanSm_DeInit
* Description: De-initializes the CanSm module
* Parameters: None
* Returns: None
*******************************************************************************/
extern void CanSm_DeInit(void);

/*******************************************************************************
* Name: CanSm_StartWakeUpSource
* Description: Starts the wake-up source (ECUM interface)
* Parameters: network - Network handle
* Returns: E_OK: success, E_NOT_OK: failed
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
extern Std_ReturnType CanSm_StartWakeUpSource(CanSm_NetworkHandleType network);
#endif

/*******************************************************************************
* Name: CanSm_StopWakeUpSource
* Description: Stops the wake-up source (ECUM interface)
* Parameters: network - Network handle
* Returns: E_OK: success, E_NOT_OK: failed
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
extern Std_ReturnType CanSm_StopWakeUpSource(CanSm_NetworkHandleType network);
#endif

/*******************************************************************************
* Name: CanSm_RequestComMode
* Description: Requests a communication mode change from ComM
* Parameters: network - Network handle
*             ComM_Mode - Requested communication mode
* Returns: E_OK: success, E_NOT_OK: failed
*******************************************************************************/
extern Std_ReturnType CanSm_RequestComMode(
    CanSm_NetworkHandleType network,
    ComM_ModeType ComM_Mode
);

/*******************************************************************************
* Name: CanSm_GetCurrentComMode
* Description: Gets the current communication mode for ComM
* Parameters: network - Network handle
*             ComM_ModePtr - Pointer to store current mode
* Returns: E_OK: success, E_NOT_OK: failed
*******************************************************************************/
extern Std_ReturnType CanSm_GetCurrentComMode(
    CanSm_NetworkHandleType network,
    ComM_ModeType* ComM_ModePtr
);

/*******************************************************************************
* Name: CanSm_TxTimeoutException
* Description: Handles Tx timeout exception from CanIf
* Parameters: network - Network handle
* Returns: None
*******************************************************************************/
#if (CANSM_TX_TIMEOUT_EXCEPTION == STD_ON)
extern void CanSm_TxTimeoutException(CanSm_NetworkHandleType network);
#endif

/*******************************************************************************
* Name: CanSm_MainFunction
* Description: Main function called periodically
* Parameters: None
* Returns: None
*******************************************************************************/
extern void CanSm_MainFunction(void);

/*******************************************************************************
* Name: CanSm_GetVersionInfo
* Description: Returns version information of CanSm
* Parameters: VersionInfo - Pointer to version info structure
* Returns: None
*******************************************************************************/
#if (CANSM_VERSION_INFO_API == STD_ON)
extern void CanSm_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/*******************************************************************************
* Name: CanSm_ControllerBusOff
* Description: Callback from CanIf indicating bus-off condition
* Parameters: ControllerId - CAN controller that experienced bus-off
* Returns: None
* Note: This is called by CanIf, not by upper layers
*******************************************************************************/
extern void CanSm_ControllerBusOff(uint8 ControllerId);

/*******************************************************************************
* Name: CanSm_ControllerModeIndication
* Description: Callback from CanIf indicating mode change complete
* Parameters: ControllerId - CAN controller
*             ControllerMode - New controller mode
* Returns: None
* Note: This is called by CanIf
*******************************************************************************/
extern void CanSm_ControllerModeIndication(
    uint8 ControllerId,
    CanIf_ControllerModeType ControllerMode
);

/*******************************************************************************
* Name: CanSm_TransceiverModeIndication
* Description: Callback from CanIf indicating transceiver mode change
* Parameters: TransceiverId - CAN transceiver
*             TransceiverMode - New transceiver mode
* Returns: None
* Note: This is called by CanIf
*******************************************************************************/
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
extern void CanSm_TransceiverModeIndication(
    uint8 TransceiverId,
    CanIf_TransceiverModeType TransceiverMode
);
#endif

/*******************************************************************************
* Name: CanSm_CheckWakeup
* Description: Check for wake-up event (ECUM interface)
* Parameters: TransceiverId - CAN transceiver to check
* Returns: None
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
extern void CanSm_CheckWakeup(uint8 TransceiverId);
#endif

#define CANSM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANSM_H */
