/**
 * @file ComM.h
 * @brief Communication Manager
 * @version 1.0.0
 * 
 * Manages communication modes and bus states for AUTOSAR BSW.
 * Implements the full channel state machine per AUTOSAR SWS_ComM.
 */

#ifndef COMM_H
#define COMM_H

#include "Std_Types.h"
#include "ComM_Cfg.h"

/*=============================================================================
 * AUTOSAR Version
 *===========================================================================*/
#define COMM_AR_RELEASE_MAJOR_VERSION       4U
#define COMM_AR_RELEASE_MINOR_VERSION       0U
#define COMM_AR_RELEASE_REVISION_VERSION    3U

/*=============================================================================
 * Module Version
 *===========================================================================*/
#define COMM_SW_MAJOR_VERSION               1U
#define COMM_SW_MINOR_VERSION               0U
#define COMM_SW_PATCH_VERSION               0U

/*=============================================================================
 * Module ID
 *===========================================================================*/
#define COMM_MODULE_ID                      0x12U

/*=============================================================================
 * Service IDs
 *===========================================================================*/
#define COMM_INIT_SID                       0x01U
#define COMM_DEINIT_SID                     0x02U
#define COMM_GETVERSIONINFO_SID             0x03U
#define COMM_REQUESTCOMODE_SID              0x04U
#define COMM_GETMAXCOMODE_SID               0x05U
#define COMM_GETREQUESTEDCOMODE_SID         0x06U
#define COMM_GETCURRENTCOMODE_SID           0x07U
#define COMM_COMMUNICATIONALLOWED_SID       0x08U
#define COMM_MAINFUNCTION_SID               0x60U
#define COMM_EVALUATEWAKEUP_SID             0x61U
#define COMM_BUSSM_MODEINDICATION_SID       0x70U
#define COMM_DCM_ACTIVEDIAGNOSTIC_SID       0x80U
#define COMM_DCM_INACTIVEDIAGNOSTIC_SID     0x81U

/*=============================================================================
 * Error Codes
 *===========================================================================*/
#define COMM_E_NOT_INIT                     0x01U
#define COMM_E_WRONG_PARAMETERS             0x02U
#define COMM_E_ERROR_IN_PROV_SERVICE        0x03U
#define COMM_E_PARAM_POINTER                0x04U

/*=============================================================================
 * Communication Modes
 *===========================================================================*/
typedef uint8 ComM_ModeType;
#define COMM_NO_COMMUNICATION               0x00U
#define COMM_SILENT_COMMUNICATION           0x01U
#define COMM_FULL_COMMUNICATION             0x02U

/*=============================================================================
 * Channel State Machine States (AUTOSAR SWS_ComM)
 * 
 * State machine for each communication channel:
 *   NO_COM_NO_PENDING_REQUEST  - Bus sleep; no active requests
 *   NO_COM_PENDING_REQUEST     - Bus sleep; request pending (wake-up in progress)
 *   FULL_COM                   - Full communication active
 *   FULL_COM_NETWORK_REQUESTED - Full communication with NM network request
 *   FULL_COM_READY_SLEEP       - Full communication, but ready to sleep (no requests)
 *   SILENT_COM                 - Silent communication (NM only, no user data)
 *===========================================================================*/
typedef uint8 ComM_ChannelStateType;
#define COMM_NO_COM_NO_PENDING_REQUEST      0x00U
#define COMM_NO_COM_PENDING_REQUEST         0x01U
#define COMM_FULL_COM                       0x02U
#define COMM_FULL_COM_NETWORK_REQUESTED     0x03U
#define COMM_FULL_COM_READY_SLEEP           0x04U
#define COMM_SILENT_COM                     0x05U

/*=============================================================================
 * Module State Types
 *===========================================================================*/
typedef uint8 ComM_StateType;
#define COMM_STATE_UNINIT                   0x00U
#define COMM_STATE_INIT                     0x01U

/*=============================================================================
 * Bus Types
 *===========================================================================*/
typedef uint8 ComM_BusTypeType;
#define COMM_BUS_TYPE_CAN                   0x00U
#define COMM_BUS_TYPE_ETH                   0x01U
#define COMM_BUS_TYPE_LIN                   0x02U
#define COMM_BUS_TYPE_FR                    0x03U
#define COMM_BUS_TYPE_INTERNAL              0x04U

/*=============================================================================
 * Handle Types
 *===========================================================================*/
typedef uint8 ComM_UserHandleType;
typedef uint8 ComM_ChannelHandleType;

/*=============================================================================
 * Channel Internal State
 *===========================================================================*/
typedef struct {
    ComM_ChannelStateType   State;                  /* Current channel state       */
    ComM_ModeType           CurrentMode;            /* Current communication mode  */
    ComM_ModeType           RequestedMode;          /* Highest requested mode      */
    boolean                 CommunicationAllowed;   /* Communication allowed flag  */
    boolean                 DiagnosticActive;       /* DCM diagnostic flag         */
    uint16                  TimeoutCounter;         /* State timeout counter       */
    uint32                  UserRequestMask;        /* Bitmask of active users     */
} ComM_ChannelRuntimeType;

/*=============================================================================
 * User Request State
 *===========================================================================*/
typedef struct {
    ComM_ModeType           RequestedMode;          /* Requested communication mode */
    boolean                 Active;                 /* User has an active request   */
} ComM_UserRequestType;

/*=============================================================================
 * Configuration Types (simplified for post-build)
 *===========================================================================*/
typedef struct {
    uint16  dummy;
} ComM_ConfigType;

/*=============================================================================
 * Function Prototypes - Core API
 *===========================================================================*/
extern void ComM_Init(const ComM_ConfigType* ConfigPtr);
extern void ComM_DeInit(void);
extern void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/*=============================================================================
 * Function Prototypes - Communication Mode Management
 *===========================================================================*/
extern Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode);
extern Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);

/*=============================================================================
 * Function Prototypes - Channel Management
 *===========================================================================*/
extern void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed);
extern void ComM_MainFunction(void);

/*=============================================================================
 * Function Prototypes - Wake-up Handling
 *===========================================================================*/
extern void ComM_EvaluateWakeup(ComM_ChannelHandleType Channel);

/*=============================================================================
 * Function Prototypes - ECU Manager Interface
 *===========================================================================*/
extern Std_ReturnType ComM_EcuM_WakeUpIndication(void);
extern Std_ReturnType ComM_EcuM_RunRequestIndication(void);

/*=============================================================================
 * Function Prototypes - Bus State Manager Interface
 *===========================================================================*/
extern void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode);

/*=============================================================================
 * Function Prototypes - DCM Interface
 *===========================================================================*/
extern Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel);
extern Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel);

#endif /* COMM_H */
