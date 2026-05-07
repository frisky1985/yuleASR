/**
 * @file ComM.h
 * @brief Communication Manager
 * @version 1.0.0
 * 
 * Manages communication modes and bus states for AUTOSAR BSW.
 */

#ifndef COMM_H
#define COMM_H

#include "Std_Types.h"

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
#define COMM_REQUESTCOMODE_SID              0x04
#define COMM_GETMAXCOMODE_SID               0x05
#define COMM_GETREQUESTEDCOMODE_SID         0x06
#define COMM_GETCURRENTCOMODE_SID           0x07
#define COMM_COMMUNICATIONALLOWED_SID       0x08
#define COMM_MAINFUNCTION_SID               0x60

/* Error Codes */
#define COMM_E_NOT_INIT                     0x01
#define COMM_E_WRONG_PARAMETERS             0x02
#define COMM_E_ERROR_IN_PROV_SERVICE        0x03

/* Communication Modes */
typedef uint8 ComM_ModeType;
#define COMM_NO_COMMUNICATION               0x00
#define COMM_SILENT_COMMUNICATION           0x01
#define COMM_FULL_COMMUNICATION             0x02

/* State Types */
typedef uint8 ComM_StateType;
#define COMM_STATE_UNINIT                   0x00
#define COMM_STATE_INIT                     0x01

/* Bus Types */
typedef uint8 ComM_BusTypeType;
#define COMM_BUS_TYPE_CAN                   0x00
#define COMM_BUS_TYPE_ETH                   0x01
#define COMM_BUS_TYPE_LIN                   0x02
#define COMM_BUS_TYPE_FR                    0x03
#define COMM_BUS_TYPE_INTERNAL              0x04

/* Handle Types */
typedef uint8 ComM_UserHandleType;
typedef uint8 ComM_ChannelHandleType;

/* Configuration Types */
typedef struct {
    uint8 dummy;
} ComM_ConfigType;

/* Function Prototypes */
extern void ComM_Init(const ComM_ConfigType* ConfigPtr);
extern void ComM_DeInit(void);
extern void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo);

extern Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode);
extern Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);
extern Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr);

extern void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed);
extern void ComM_MainFunction(void);

/* ECU Classification */
extern Std_ReturnType ComM_EcuM_WakeUpIndication(void);
extern Std_ReturnType ComM_EcuM_RunRequestIndication(void);

/* BusSM Interfaces */
extern void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode);

/* DCM Interfaces */
extern Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel);
extern Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel);

#endif /* COMM_H */
