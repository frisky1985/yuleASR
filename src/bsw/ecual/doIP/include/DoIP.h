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

/**********************************************************************************
**                                                                               **
**  DoIP.h - AUTOSAR Diagnostic over IP Module Header                            **
**                                                                               **
**  Implements ISO 13400-2 diagnostic over IP protocol                           **
**                                                                               **
**********************************************************************************/

#ifndef DOIP_H
#define DOIP_H

#ifdef __cplusplus
extern "C" {
#endif

/*================================================================================
**  INCLUDE FILES
================================================================================*/
#include "Std_Types.h"
#include "DoIP_Cfg.h"
#include "ComStack_Types.h"
#include "SoAd.h"
#include "PduR_DoIP.h"

/*================================================================================
**  VERSION INFORMATION
================================================================================*/
#define DOIP_VENDOR_ID                    (0x1234U)
#define DOIP_MODULE_ID                    (0x25U)

#define DOIP_SW_MAJOR_VERSION             (1U)
#define DOIP_SW_MINOR_VERSION             (0U)
#define DOIP_SW_PATCH_VERSION             (0U)

/*================================================================================
**  DET ERROR CODES
================================================================================*/
#define DOIP_E_UNINIT                     (0x01U)
#define DOIP_E_INVALID_POINTER            (0x02U)
#define DOIP_E_INVALID_PARAM              (0x03U)
#define DOIP_E_INVALID_PDU_SDU_ID         (0x04U)
#define DOIP_E_INVALID_REQUEST_LENGTH     (0x05U)

/*================================================================================
**  SERVICE IDs
================================================================================*/
#define DOIP_SID_INIT                     (0x01U)
#define DOIP_SID_DEINIT                   (0x02U)
#define DOIP_SID_GET_VERSION_INFO         (0x03U)
#define DOIP_SID_ACTIVATION_LINE_ACTIVE   (0x04U)
#define DOIP_SID_ACTIVATION_LINE_INACTIVE (0x05U)
#define DOIP_SID_IF_RX_INDICATION         (0x06U)
#define DOIP_SID_TP_RX_INDICATION         (0x07U)
#define DOIP_SID_TP_TX_CONFIRMATION       (0x08U)
#define DOIP_SID_TRIGGER_TRANSMIT         (0x09U)
#define DOIP_SID_MAIN_FUNCTION            (0x0AU)
#define DOIP_SID_SOAD_IF_RX_INDICATION    (0x0BU)
#define DOIP_SID_SOAD_TP_RX_INDICATION    (0x0CU)
#define DOIP_SID_SOAD_TP_TX_CONFIRMATION  (0x0DU)
#define DOIP_SID_SOAD_IF_TX_CONFIRMATION  (0x0EU)

/*================================================================================
**  TYPE DEFINITIONS
================================================================================*/

/* DoIP module state */
typedef enum
{
    DOIP_STATE_UNINIT = 0,
    DOIP_STATE_INIT   = 1
} DoIP_StateType;

/* Routing activation type (values from DoIP_Cfg.h) */
typedef uint8 DoIP_RoutingActivationType;

/* Routing activation response code */
typedef enum
{
    DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS                  = 0x00,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNKNOWN_SA        = 0x01,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_SA_ACTIVE         = 0x02,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_AUTHENTIC_MISSING = 0x03,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_CONFIRM_MISSING   = 0x04,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNSUPPORTED_RA    = 0x05,
    DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_TLS_REQUIRED      = 0x06
} DoIP_RoutingActivationResType;

/* Connection state */
typedef enum
{
    DOIP_SOCKET_STATE_DISCONNECTED = 0,
    DOIP_SOCKET_STATE_RESERVED     = 1,
    DOIP_SOCKET_STATE_REGISTERED   = 2,
    DOIP_SOCKET_STATE_ACTIVATED    = 3
} DoIP_SocketStateType;

/* Tester connection info */
typedef struct
{
    uint16                   testerLogicalAddress;
    uint8                    testerPhysicalAddress[6];
    uint16                   targetLogicalAddress;
    DoIP_SocketStateType     socketState;
    uint8                    routingActivationType;
    boolean                  aliveCheckPending;
    uint16                   aliveCheckTimeout;
} DoIP_TesterConnectionType;

/* Vehicle announcement info */
typedef struct
{
    uint8                    vin[17];
    uint8                    eid[6];
    uint8                    gid[6];
    uint8                    furtherActionReq;
    uint16                   logicalAddress;
    uint8                    syncStatus;
    uint16                   announcementCount;
    uint16                   announcementInterval;
} DoIP_VehicleAnnouncementType;

/* Version info structure */
typedef struct
{
    uint16                   vendorID;
    uint16                   moduleID;
    uint8                    swMajorVersion;
    uint8                    swMinorVersion;
    uint8                    swPatchVersion;
} DoIP_VersionInfoType;

/*================================================================================
**  FUNCTION PROTOTYPES
================================================================================*/

/* Module lifecycle */
extern void DoIP_Init(const DoIP_ConfigType* ConfigPtr);
extern void DoIP_DeInit(void);

/* Activation line management */
extern void DoIP_ActivationLineSwitchActive(void);
extern void DoIP_ActivationLineSwitchInactive(void);

/* Version info */
#if (DOIP_VERSION_INFO_API == STD_ON)
extern void DoIP_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/* SoAd callbacks */
extern void DoIP_SoAdIfRxIndication(
    PduIdType RxPduId,
    const PduInfoType* PduInfoPtr
);

extern void DoIP_SoAdIfTxConfirmation(
    PduIdType TxPduId
);

extern void DoIP_SoAdTpRxIndication(
    PduIdType RxPduId,
    Std_ReturnType result
);

extern void DoIP_SoAdTpTxConfirmation(
    PduIdType TxPduId,
    Std_ReturnType result
);

extern Std_ReturnType DoIP_SoAdTriggerTransmit(
    PduIdType TxPduId,
    PduInfoType* PduInfoPtr
);

/* PduR callbacks */
extern Std_ReturnType DoIP_Transmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);

extern void DoIP_RxIndication(
    PduIdType RxPduId,
    const PduInfoType* PduInfoPtr
);

extern void DoIP_TxConfirmation(
    PduIdType TxPduId,
    Std_ReturnType result
);

/* Main function */
extern void DoIP_MainFunction(void);

/* Internal functions exposed for testing */
extern Std_ReturnType DoIP_SendVehicleAnnouncement(void);
extern Std_ReturnType DoIP_ProcessRoutingActivation(
    uint16 socketId,
    const uint8* requestPtr,
    uint16 requestLength
);
extern Std_ReturnType DoIP_SendAliveCheckRequest(uint16 socketId);
extern void DoIP_ProcessAliveCheckResponse(
    uint16 socketId,
    const uint8* responsePtr
);

#ifdef __cplusplus
}
#endif

#endif /* DOIP_H */
