/**
 * @file Nm.h
 * @brief Network Management Interface
 * @version 1.0.0
 * 
 * Generic network management interface for AUTOSAR BSW.
 * Supports CAN, LIN, FlexRay, and Ethernet networks.
 */

#ifndef NM_H
#define NM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define NM_AR_RELEASE_MAJOR_VERSION       4
#define NM_AR_RELEASE_MINOR_VERSION       0
#define NM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define NM_SW_MAJOR_VERSION               1
#define NM_SW_MINOR_VERSION               0
#define NM_SW_PATCH_VERSION               0

/* Module ID */
#define NM_MODULE_ID                      0x1D

/* Service IDs */
#define NM_INIT_SID                       0x01
#define NM_DEINIT_SID                     0x02
#define NM_GETVERSIONINFO_SID             0x03
#define NM_PASSIVESSTARTUP_SID            0x04
#define NM_NETWORKREQUEST_SID             0x05
#define NM_NETWORKRELEASE_SID             0x06
#define NM_DISABLECOMMUNICATION_SID       0x07
#define NM_ENABLECOMMUNICATION_SID        0x08
#define NM_GETSTATE_SID                   0x09
#define NM_GETLOCALNODEIDENTIFIER_SID     0x0A
#define NM_GETPDUDATA_SID                 0x0B
#define NM_GETUSERDATA_SID                0x0C
#define NM_REPEATMESSAGEREQUEST_SID       0x0D
#define NM_GETNODECLUSTER_ID              0x0E
#define NM_MAINFUNCTION_SID               0x60

/* Error Codes */
#define NM_E_UNINIT                       0x01
#define NM_E_INVALID_CHANNEL              0x02
#define NM_E_INVALID_POINTER              0x03
#define NM_E_NOT_OK                       0x04

/* NM States */
typedef uint8 Nm_StateType;
#define NM_STATE_UNINIT                   0x00
#define NM_STATE_BUS_SLEEP                0x01
#define NM_STATE_PREPARE_BUS_SLEEP        0x02
#define NM_STATE_READY_SLEEP              0x03
#define NM_STATE_NORMAL_OPERATION         0x04
#define NM_STATE_REPEAT_MESSAGE           0x05
#define NM_STATE_SYNCHRONIZE              0x06

/* NM Modes */
typedef uint8 Nm_ModeType;
#define NM_MODE_BUS_SLEEP                 0x00
#define NM_MODE_PREPARE_BUS_SLEEP         0x01
#define NM_MODE_SYNCHRONIZE               0x02
#define NM_MODE_NETWORK                   0x03

/* Bus Types */
typedef uint8 Nm_BusNmTypeType;
#define NM_BUSNM_CANNM                    0x00
#define NM_BUSNM_FRNM                     0x01
#define NM_BUSNM_UDPNM                    0x02
#define NM_BUSNM_LINNM                    0x03

/* Channel Handle */
typedef uint8 Nm_ChannelHandleType;

/* Node Identifier */
typedef uint8 Nm_NodeIdType;

/* Configuration Type */
typedef struct {
    uint8 dummy;
} Nm_ConfigType;

/* State Change Callback Type */
typedef void (*Nm_StateChangeNotificationCallbackType)(
    Nm_ChannelHandleType nmNetworkHandle,
    Nm_StateType nmPreviousState,
    Nm_StateType nmCurrentState
);

/* Function Prototypes */
extern void Nm_Init(const Nm_ConfigType* ConfigPtr);
extern void Nm_DeInit(void);
extern void Nm_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/* Network Management Functions */
extern Std_ReturnType Nm_PassiveStartUp(Nm_ChannelHandleType nmChannelHandle);
extern Std_ReturnType Nm_NetworkRequest(Nm_ChannelHandleType nmChannelHandle);
extern Std_ReturnType Nm_NetworkRelease(Nm_ChannelHandleType nmChannelHandle);

/* Communication Control */
extern Std_ReturnType Nm_DisableCommunication(Nm_ChannelHandleType nmChannelHandle);
extern Std_ReturnType Nm_EnableCommunication(Nm_ChannelHandleType nmChannelHandle);

/* State Query Functions */
extern Std_ReturnType Nm_GetState(Nm_ChannelHandleType nmChannelHandle, Nm_StateType* nmStatePtr);
extern Std_ReturnType Nm_GetMode(Nm_ChannelHandleType nmChannelHandle, Nm_ModeType* nmModePtr);
extern Std_ReturnType Nm_GetLocalNodeIdentifier(Nm_ChannelHandleType nmChannelHandle, Nm_NodeIdType* nmNodeIdPtr);

/* PDU Data Functions */
extern Std_ReturnType Nm_GetPduData(Nm_ChannelHandleType nmChannelHandle, uint8* nmPduDataPtr);
extern Std_ReturnType Nm_GetUserData(Nm_ChannelHandleType nmChannelHandle, uint8* nmUserDataPtr);
extern Std_ReturnType Nm_SetUserData(Nm_ChannelHandleType nmChannelHandle, const uint8* nmUserDataPtr);
extern Std_ReturnType Nm_RepeatMessageRequest(Nm_ChannelHandleType nmChannelHandle);

/* Remote Sleep */
extern Std_ReturnType Nm_CheckRemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle, boolean* nmRemoteSleepIndPtr);
extern Std_ReturnType Nm_GetCoordinatorSleepReady(Nm_ChannelHandleType nmCoordClusterHandle, boolean* nmSleepReadyPtr);

/* Main Function */
extern void Nm_MainFunction(void);

/* Callback Functions (called by BusNm) */
extern void Nm_BusSleepModeEntry(Nm_ChannelHandleType nmNetworkHandle);
extern void Nm_PrepareBusSleepModeEntry(Nm_ChannelHandleType nmNetworkHandle);
extern void Nm_NetworkModeEntry(Nm_ChannelHandleType nmNetworkHandle);
extern void Nm_NetworkStartIndication(Nm_ChannelHandleType nmNetworkHandle);
extern void Nm_RxIndication(Nm_ChannelHandleType nmNetworkHandle, const uint8* nmPduDataPtr);
extern void Nm_StateChangeNotification(Nm_ChannelHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState);
extern void Nm_RemoteSleepIndication(Nm_ChannelHandleType nmNetworkHandle);
extern void Nm_RemoteSleepCancellation(Nm_ChannelHandleType nmNetworkHandle);

#endif /* NM_H */
