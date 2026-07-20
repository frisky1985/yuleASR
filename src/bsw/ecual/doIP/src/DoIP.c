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
**  DoIP.c - AUTOSAR Diagnostic over IP Module Implementation                    **
**                                                                               **
**  Implements ISO 13400-2 diagnostic over IP protocol                           **
**                                                                               **
**********************************************************************************/

#include "DoIP.h"
#include "Det.h"
#include "SchM_DoIP.h"

/*================================================================================
**  INTERNAL DEFINITIONS
================================================================================*/

/* Header length */
#define DOIP_HEADER_LENGTH                (8U)

/* Header fields */
#define DOIP_HDR_IDX_PROTOCOL_VER         (0U)
#define DOIP_HDR_IDX_PROTOCOL_VER_INV     (1U)
#define DOIP_HDR_IDX_PAYLOAD_TYPE_HI      (2U)
#define DOIP_HDR_IDX_PAYLOAD_TYPE_LO      (3U)
#define DOIP_HDR_IDX_PAYLOAD_LENGTH_0     (4U)
#define DOIP_HDR_IDX_PAYLOAD_LENGTH_1     (5U)
#define DOIP_HDR_IDX_PAYLOAD_LENGTH_2     (6U)
#define DOIP_HDR_IDX_PAYLOAD_LENGTH_3     (7U)

/* Minimum routing activation request length */
#define DOIP_ROUTING_ACTIVATION_REQ_MIN_LEN  (7U)

/* Diagnostic message ACK/NACK codes */
#define DOIP_DIAG_ACK_CODE_OK             (0x00U)
#define DOIP_DIAG_NACK_CODE_INVALID_SA    (0x02U)
#define DOIP_DIAG_NACK_CODE_UNKNOWN_TA    (0x03U)
#define DOIP_DIAG_NACK_CODE_UNKNOWN_NET   (0x04U)
#define DOIP_DIAG_NACK_CODE_OUT_OF_MEM    (0x05U)
#define DOIP_DIAG_NACK_CODE_TARGET_BUSY   (0x06U)

/*================================================================================
**  INTERNAL VARIABLES
================================================================================*/

/* Module state */
static DoIP_StateType DoIP_ModuleState = DOIP_STATE_UNINIT;

/* Configuration pointer */
static const DoIP_ConfigType* DoIP_ConfigPtr = NULL_PTR;

/* Tester connections */
static DoIP_TesterConnectionType DoIP_TesterConnections[DOIP_MAX_TESTER_CONNECTIONS];

/* Vehicle announcement state */
static DoIP_VehicleAnnouncementType DoIP_VehicleAnnouncement;

/* Buffer for transmit/receive */
static uint8 DoIP_Buffer[DOIP_BUFFER_SIZE];

/* Announcement state */
static boolean DoIP_AnnouncementActive = FALSE;
static uint16 DoIP_AnnouncementCounter = 0;
static uint16 DoIP_AnnouncementTimer = 0;

/* Activation line state */
static boolean DoIP_ActivationLineState = FALSE;

/* Alive check state */
static uint16 DoIP_AliveCheckTimer = 0;

/*================================================================================
**  INTERNAL FUNCTION PROTOTYPES
================================================================================*/

static uint16 DoIP_GetPayloadType(const uint8* headerPtr);
static uint32 DoIP_GetPayloadLength(const uint8* headerPtr);
static void DoIP_WriteHeader(
    uint8* bufferPtr,
    uint16 payloadType,
    uint32 payloadLength
);
static Std_ReturnType DoIP_SendGenericNack(
    PduIdType TxPduId,
    uint8 nackCode
);
static Std_ReturnType DoIP_ProcessVehicleIdRequest(uint16 socketId);
static void DoIP_SendRoutingActivationResponse(
    uint16 socketId,
    uint16 logicalAddress,
    uint16 testerAddress,
    uint8 responseCode,
    const uint8* oemDataPtr
);
static Std_ReturnType DoIP_ProcessDiagnosticMessage(
    uint16 socketId,
    const uint8* messagePtr,
    uint32 messageLength
);
static void DoIP_SendDiagnosticAck(
    uint16 socketId,
    uint16 sourceAddress,
    uint16 targetAddress,
    uint8 ackCode,
    uint32 previousMsgLength
);
static uint8 DoIP_GetRoutingActivationResponseCode(
    uint16 socketId,
    uint16 testerAddress,
    uint8 activationType,
    const uint8* authData,
    uint16 authLen
);
static void DoIP_CloseSocket(uint16 socketId);
static void DoIP_ResetTesterConnection(uint16 connectionIdx);

/*================================================================================
**  EXTERNAL FUNCTIONS - LIFECYCLE
================================================================================*/

void DoIP_Init(const DoIP_ConfigType* ConfigPtr)
{
    uint8 i;
    const DoIP_VehicleAnnouncementConfigType* vaConfig;

    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_INIT,
            DOIP_E_INVALID_POINTER
        );
        return;
    }
    #endif

    /* Store configuration */
    DoIP_ConfigPtr = ConfigPtr;

    /* Initialize tester connections */
    for (i = 0; i < DOIP_MAX_TESTER_CONNECTIONS; i++)
    {
        DoIP_ResetTesterConnection(i);
    }

    /* Initialize vehicle announcement */
    vaConfig = ConfigPtr->vehicleAnnouncement;
    if (vaConfig != NULL_PTR)
    {
        for (i = 0; i < DOIP_VIN_LENGTH; i++)
        {
            DoIP_VehicleAnnouncement.vin[i] = vaConfig->vin[i];
        }
        for (i = 0; i < DOIP_EID_LENGTH; i++)
        {
            DoIP_VehicleAnnouncement.eid[i] = vaConfig->eid[i];
        }
        for (i = 0; i < DOIP_GID_LENGTH; i++)
        {
            DoIP_VehicleAnnouncement.gid[i] = vaConfig->gid[i];
        }
        DoIP_VehicleAnnouncement.furtherActionReq = vaConfig->furtherActionReq;
        DoIP_VehicleAnnouncement.logicalAddress = vaConfig->logicalAddress;
        DoIP_VehicleAnnouncement.syncStatus = vaConfig->syncStatus;
        DoIP_VehicleAnnouncement.announcementInterval = vaConfig->announcementInterval;
        DoIP_VehicleAnnouncement.announcementCount = vaConfig->announcementCount;
/* [MISRA Advisory] Redundant:         DoIP_VehicleAnnouncement.announcementCount = 0; */
    }

    /* Reset timers and state */
    DoIP_AnnouncementActive = FALSE;
    DoIP_AnnouncementCounter = 0;
    DoIP_AnnouncementTimer = 0;
    DoIP_AliveCheckTimer = 0;
    DoIP_ActivationLineState = FALSE;

    /* Set module state to initialized */
    DoIP_ModuleState = DOIP_STATE_INIT;
}

void DoIP_DeInit(void)
{
    uint8 i;

    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_DEINIT,
            DOIP_E_UNINIT
        );
        return;
    }
    #endif

    /* Close all tester connections */
    for (i = 0; i < DOIP_MAX_TESTER_CONNECTIONS; i++)
    {
        if (DoIP_TesterConnections[i].socketState != DOIP_SOCKET_STATE_DISCONNECTED)
        {
            DoIP_CloseSocket(i);
        }
    }

    /* Reset state */
    DoIP_ConfigPtr = NULL_PTR;
    DoIP_ModuleState = DOIP_STATE_UNINIT;
}

/*================================================================================
**  EXTERNAL FUNCTIONS - ACTIVATION LINE
================================================================================*/

void DoIP_ActivationLineSwitchActive(void)
{
    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_ACTIVATION_LINE_ACTIVE,
            DOIP_E_UNINIT
        );
        return;
    }
    #endif

    SchM_Enter_DoIP();

    DoIP_ActivationLineState = TRUE;

    #if (DOIP_VEHICLE_ANNOUNCEMENT == STD_ON)
    /* Start vehicle announcement */
    DoIP_AnnouncementActive = TRUE;
    DoIP_AnnouncementCounter = 0;
    DoIP_AnnouncementTimer = 0;
    #endif

    SchM_Exit_DoIP();
}

void DoIP_ActivationLineSwitchInactive(void)
{
    uint8 i;

    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_ACTIVATION_LINE_INACTIVE,
            DOIP_E_UNINIT
        );
        return;
    }
    #endif

    SchM_Enter_DoIP();

    DoIP_ActivationLineState = FALSE;

    /* Stop vehicle announcement */
    DoIP_AnnouncementActive = FALSE;

    /* Close all sockets */
    for (i = 0; i < DOIP_MAX_TESTER_CONNECTIONS; i++)
    {
        DoIP_CloseSocket(i);
    }

    SchM_Exit_DoIP();
}

/*================================================================================
**  EXTERNAL FUNCTIONS - VERSION INFO
================================================================================*/

#if (DOIP_VERSION_INFO_API == STD_ON)
void DoIP_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_GET_VERSION_INFO,
            DOIP_E_INVALID_POINTER
        );
        return;
    }
    #endif

    versioninfo->vendorID = DOIP_VENDOR_ID;
    versioninfo->moduleID = DOIP_MODULE_ID;
    versioninfo->sw_major_version = DOIP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DOIP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DOIP_SW_PATCH_VERSION;
}
#endif

/*================================================================================
**  EXTERNAL FUNCTIONS - SOAD CALLBACKS
================================================================================*/

void DoIP_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint16 payloadType;
    uint32 payloadLength;
    uint8* dataPtr;
    uint16 headerLength;
    uint8 nackCode;

    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_SOAD_IF_RX_INDICATION,
            DOIP_E_UNINIT
        );
        return;
    }
    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_SOAD_IF_RX_INDICATION,
            DOIP_E_INVALID_POINTER
        );
        return;
    }
    #endif

    /* Check minimum header length */
    if (PduInfoPtr->SduLength < DOIP_HEADER_LENGTH)
    {
        return;
    }

    dataPtr = PduInfoPtr->SduDataPtr;

    /* Check protocol version */
    if ((dataPtr[DOIP_HDR_IDX_PROTOCOL_VER] != DOIP_PROTOCOL_VERSION) ||
        (dataPtr[DOIP_HDR_IDX_PROTOCOL_VER_INV] != DOIP_PROTOCOL_VERSION_INVERTED))
    {
        DoIP_SendGenericNack(RxPduId, DOIP_NACK_CODE_INCORRECT_PATTERN_FORMAT);
        return;
    }

    /* Get payload type and length */
    payloadType = DoIP_GetPayloadType(dataPtr);
    payloadLength = DoIP_GetPayloadLength(dataPtr);

    /* Validate payload length */
    if ((PduInfoPtr->SduLength - DOIP_HEADER_LENGTH) < payloadLength)
    {
        DoIP_SendGenericNack(RxPduId, DOIP_NACK_CODE_INVALID_PAYLOAD_LENGTH);
        return;
    }

    headerLength = DOIP_HEADER_LENGTH;

    /* Process based on payload type */
    switch (payloadType)
    {
        case DOIP_PAYLOAD_TYPE_VID_REQUEST:
            DoIP_ProcessVehicleIdRequest(RxPduId);
            break;

        case DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_REQUEST:
            if (payloadLength >= DOIP_ROUTING_ACTIVATION_REQ_MIN_LEN)
            {
                DoIP_ProcessRoutingActivation(
                    RxPduId,
                    &dataPtr[headerLength],
                    (uint16)payloadLength
                );
            }
            else
            {
                DoIP_SendGenericNack(RxPduId, DOIP_NACK_CODE_INVALID_PAYLOAD_LENGTH);
            }
            break;

        case DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE:
            DoIP_ProcessDiagnosticMessage(
                RxPduId,
                &dataPtr[headerLength],
                payloadLength
            );
            break;

        case DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE:
            DoIP_ProcessAliveCheckResponse(RxPduId, &dataPtr[headerLength]);
            break;

        case DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST:
        case DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POWER_MODE_REQUEST:
            /* Handle additional requests */
            break;

        default:
            /* Unknown payload type */
            nackCode = DOIP_NACK_CODE_UNKNOWN_PAYLOAD_TYPE;
            DoIP_SendGenericNack(RxPduId, nackCode);
            break;
    }
}

void DoIP_SoAdIfTxConfirmation(PduIdType TxPduId)
{
    /* Transmission confirmation */
    (void)TxPduId;
}

void DoIP_SoAdTpRxIndication(PduIdType RxPduId, Std_ReturnType result)
{
    (void)RxPduId;
    (void)result;
}

void DoIP_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
}

Std_ReturnType DoIP_SoAdTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_NOT_OK;
}

/*================================================================================
**  EXTERNAL FUNCTIONS - PDUR CALLBACKS
================================================================================*/

Std_ReturnType DoIP_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    #if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        Det_ReportError(
            DOIP_MODULE_ID,
            0,
            DOIP_SID_TRIGGER_TRANSMIT,
            DOIP_E_UNINIT
        );
        return E_NOT_OK;
    }
    #endif

    /* Forward to SoAd for transmission */
    return SoAd_IfTransmit(TxPduId, PduInfoPtr);
}

void DoIP_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
}

void DoIP_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
}

/*================================================================================
**  EXTERNAL FUNCTIONS - MAIN FUNCTION
================================================================================*/

void DoIP_MainFunction(void)
{
    uint8 i;

    if (DoIP_ModuleState != DOIP_STATE_INIT)
    {
        return;
    }

    SchM_Enter_DoIP();

    #if (DOIP_VEHICLE_ANNOUNCEMENT == STD_ON)
    /* Handle vehicle announcement */
    if (DoIP_AnnouncementActive)
    {
        if (DoIP_AnnouncementTimer == 0U )
        {
            DoIP_SendVehicleAnnouncement();
            DoIP_AnnouncementCounter++;
            if (DoIP_AnnouncementCounter >= DoIP_ConfigPtr->vehicleAnnouncement->announcementCount)
            {
                DoIP_AnnouncementActive = FALSE;
            }
            else
            {
                DoIP_AnnouncementTimer = DoIP_ConfigPtr->vehicleAnnouncement->announcementInterval;
            }
        }
        else
        {
            DoIP_AnnouncementTimer--;
        }
    }
    #endif

    #if (DOIP_ALIVE_CHECK_SUPPORT == STD_ON)
    /* Handle alive check */
    DoIP_AliveCheckTimer++;
    if (DoIP_AliveCheckTimer >= DOIP_ALIVE_CHECK_INTERVAL)
    {
        DoIP_AliveCheckTimer = 0;
        for (i = 0; i < DOIP_MAX_TESTER_CONNECTIONS; i++)
        {
            if (DoIP_TesterConnections[i].socketState == DOIP_SOCKET_STATE_ACTIVATED)
            {
                if (DoIP_TesterConnections[i].aliveCheckPending)
                {
                    /* No response to previous alive check - close socket */
                    DoIP_CloseSocket(i);
                }
                else
                {
                    DoIP_SendAliveCheckRequest(i);
                }
            }
        }
    }
    #endif

    SchM_Exit_DoIP();
}

/*================================================================================
**  INTERNAL FUNCTIONS - VEHICLE ANNOUNCEMENT
================================================================================*/

Std_ReturnType DoIP_SendVehicleAnnouncement(void)
{
    PduInfoType pduInfo;
    Std_ReturnType result;
    uint8* dataPtr = DoIP_Buffer;
    uint16 idx;

    /* Build DoIP header */
    DoIP_WriteHeader(
        dataPtr,
        DOIP_PAYLOAD_TYPE_VID_RESPONSE,
        32 /* VIN(17) + LA(2) + EID(6) + GID(6) + FAR(1) = 32 */
    );

    idx = DOIP_HEADER_LENGTH;

    /* VIN */
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[0];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[1];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[2];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[3];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[4];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[5];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[6];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[7];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[8];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[9];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[10];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[11];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[12];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[13];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[14];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[15];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.vin[16];

    /* Logical Address */
    dataPtr[idx++] = (uint8)(DoIP_VehicleAnnouncement.logicalAddress >> 8);
    dataPtr[idx++] = (uint8)(DoIP_VehicleAnnouncement.logicalAddress);

    /* EID */
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[0];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[1];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[2];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[3];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[4];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.eid[5];

    /* GID */
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[0];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[1];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[2];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[3];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[4];
    dataPtr[idx++] = DoIP_VehicleAnnouncement.gid[5];

    /* Further Action Required */
    dataPtr[idx++] = DoIP_VehicleAnnouncement.furtherActionReq;

    /* Sync Status (optional) */
    if (DoIP_VehicleAnnouncement.syncStatus != 0U )
    {
        dataPtr[idx++] = DoIP_VehicleAnnouncement.syncStatus;
    }

    pduInfo.SduDataPtr = dataPtr;
    pduInfo.SduLength = idx;
    pduInfo.MetaDataPtr = NULL_PTR;

    /* Send via UDP broadcast */
    result = SoAd_IfTransmit(DOIP_UDP_SOAD_TX_PDU_ID, &pduInfo);

    return result;
}

static Std_ReturnType DoIP_ProcessVehicleIdRequest(uint16 socketId)
{
    (void)socketId;
    return DoIP_SendVehicleAnnouncement();
}

/*================================================================================
**  INTERNAL FUNCTIONS - ROUTING ACTIVATION
================================================================================*/

Std_ReturnType DoIP_ProcessRoutingActivation(
    uint16 socketId,
    const uint8* requestPtr,
    uint16 requestLength)
{
    uint16 sourceAddress;
    uint8 activationType;
    uint8 reservedByte;
    const uint8* authDataPtr;
    uint16 authLen;
    uint8 responseCode;
    uint8 connectionIdx;
    boolean foundConnection = FALSE;

    /* Parse request */
    sourceAddress = ((uint16)requestPtr[0] << 8) | requestPtr[1];
    activationType = requestPtr[2];
    reservedByte = requestPtr[3]; /* ISO Reserved */
    (void)reservedByte;

    /* Authentication data */
    authDataPtr = &requestPtr[4];
    authLen = requestLength - 7; /* Subtract header (7 bytes) */

    /* Find or allocate tester connection */
    for (connectionIdx = 0; connectionIdx < DOIP_MAX_TESTER_CONNECTIONS; connectionIdx++)
    {
        if (DoIP_TesterConnections[connectionIdx].socketState == DOIP_SOCKET_STATE_DISCONNECTED)
        {
            foundConnection = TRUE;
            break;
        }
        else if (DoIP_TesterConnections[connectionIdx].testerLogicalAddress == sourceAddress)
        {
            /* Existing connection - check if same socket */
            foundConnection = TRUE;
            break;
        }
    }

    if (!foundConnection)
    {
        responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_SA_ACTIVE;
        DoIP_SendRoutingActivationResponse(
            socketId,
            DOIP_LOCAL_LOGICAL_ADDRESS,
            sourceAddress,
            responseCode,
            NULL_PTR
        );
        return E_NOT_OK;
    }

    /* Get response code based on authentication/validation */
    responseCode = DoIP_GetRoutingActivationResponseCode(
        socketId,
        sourceAddress,
        activationType,
        authDataPtr,
        authLen
    );

    /* Update connection state */
    if (responseCode == DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS)
    {
        DoIP_TesterConnections[connectionIdx].testerLogicalAddress = sourceAddress;
        DoIP_TesterConnections[connectionIdx].routingActivationType = activationType;
        DoIP_TesterConnections[connectionIdx].socketState = DOIP_SOCKET_STATE_ACTIVATED;
        DoIP_TesterConnections[connectionIdx].aliveCheckPending = FALSE;
        DoIP_TesterConnections[connectionIdx].aliveCheckTimeout = 0;
    }

    /* Send response */
    DoIP_SendRoutingActivationResponse(
        socketId,
        DOIP_LOCAL_LOGICAL_ADDRESS,
        sourceAddress,
        responseCode,
        NULL_PTR
    );

    return (responseCode == DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS) ? E_OK : E_NOT_OK;
}

static void DoIP_SendRoutingActivationResponse(
    uint16 socketId,
    uint16 logicalAddress,
    uint16 testerAddress,
    uint8 responseCode,
    const uint8* oemDataPtr)
{
    PduInfoType pduInfo;
    uint8* dataPtr = DoIP_Buffer;
    uint16 idx;

    (void)oemDataPtr;

    /* Build header */
    DoIP_WriteHeader(
        dataPtr,
        DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_RESPONSE,
        13 /* LA(2) + TA(2) + RC(1) + IC(4) + reserved(4) = 13 */
    );

    idx = DOIP_HEADER_LENGTH;

    /* Logical Address */
    dataPtr[idx++] = (uint8)(logicalAddress >> 8);
    dataPtr[idx++] = (uint8)(logicalAddress);

    /* Tester Logical Address */
    dataPtr[idx++] = (uint8)(testerAddress >> 8);
    dataPtr[idx++] = (uint8)(testerAddress);

    /* Response Code */
    dataPtr[idx++] = responseCode;

    /* Reserved bytes */
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;

    /* OEM data */
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;
    dataPtr[idx++] = 0;

    pduInfo.SduDataPtr = dataPtr;
    pduInfo.SduLength = idx;
    pduInfo.MetaDataPtr = NULL_PTR;

    SoAd_IfTransmit((PduIdType)socketId, &pduInfo);
}

static uint8 DoIP_GetRoutingActivationResponseCode(
    uint16 socketId,
    uint16 testerAddress,
    uint8 activationType,
    const uint8* authData,
    uint16 authLen)
{
    uint8 i;
    (void)socketId;
    (void)authData;
    (void)authLen;

    /* Check if activation type is supported */
    if (DoIP_ConfigPtr != NULL_PTR)
    {
        for (i = 0; i < DoIP_ConfigPtr->numRoutingActivations; i++)
        {
            if (DoIP_ConfigPtr->routingActivations[i].routingActivationType == activationType)
            {
                /* Check if tester address is valid for this activation type */
                if (DoIP_ConfigPtr->routingActivations[i].testerLogicalAddress == testerAddress)
                {
                    #if (DOIP_ROUTING_ACTIVATION_AUTHENTICATION == STD_ON)
                    if (DoIP_ConfigPtr->routingActivations[i].authenticationRequired)
                    {
                        /* Would validate authentication data here */
                    }
                    #endif

                    return DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS;
                }
            }
        }
    }

    /* Check standard activation types */
    if ((activationType == DOIP_ROUTING_ACTIVATION_DEFAULT) ||
        (activationType == DOIP_ROUTING_ACTIVATION_WWH_OBD) ||
        (activationType == DOIP_ROUTING_ACTIVATION_CDS))
    {
        return DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS;
    }

    return DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNSUPPORTED_RA;
}

/*================================================================================
**  INTERNAL FUNCTIONS - DIAGNOSTIC MESSAGE ROUTING
================================================================================*/

static Std_ReturnType DoIP_ProcessDiagnosticMessage(
    uint16 socketId,
    const uint8* messagePtr,
    uint32 messageLength)
{
    uint16 sourceAddress;
    uint16 targetAddress;
    PduInfoType pduInfo;
    uint8 connectionIdx;
    uint8 taConnectionIdx = 0xFF;
    boolean foundConnection = FALSE;
    PduIdType pduRTargetPduId;

    (void)socketId;

    /* Parse source and target addresses */
    sourceAddress = ((uint16)messagePtr[0] << 8) | messagePtr[1];
    targetAddress = ((uint16)messagePtr[2] << 8) | messagePtr[3];

    /* Find connection for source address */
    for (connectionIdx = 0; connectionIdx < DOIP_MAX_TESTER_CONNECTIONS; connectionIdx++)
    {
        if (DoIP_TesterConnections[connectionIdx].testerLogicalAddress == sourceAddress)
        {
            foundConnection = TRUE;
            break;
        }
    }

    /* Validate source address */
    if (!foundConnection)
    {
        DoIP_SendDiagnosticAck(
            socketId,
            sourceAddress,
            targetAddress,
            DOIP_DIAG_NACK_CODE_INVALID_SA,
            messageLength
        );
        return E_NOT_OK;
    }

    /* Validate target address */
    if (targetAddress == DOIP_LOCAL_LOGICAL_ADDRESS)
    {
        /* Target is local node */
        taConnectionIdx = 0xFF;
    }
    else
    {
        /* Find connection for target address */
        for (connectionIdx = 0; connectionIdx < DOIP_MAX_TESTER_CONNECTIONS; connectionIdx++)
        {
            if (DoIP_TesterConnections[connectionIdx].testerLogicalAddress == targetAddress)
            {
                taConnectionIdx = connectionIdx;
                break;
            }
        }

        if (taConnectionIdx == 0xFF)
        {
            DoIP_SendDiagnosticAck(
                socketId,
                sourceAddress,
                targetAddress,
                DOIP_DIAG_NACK_CODE_UNKNOWN_TA,
                messageLength
            );
            return E_NOT_OK;
        }
    }

    /* Send ACK */
    DoIP_SendDiagnosticAck(
        socketId,
        sourceAddress,
        targetAddress,
        DOIP_DIAG_ACK_CODE_OK,
        messageLength
    );

    /* Route message to target */
    if (taConnectionIdx == 0xFF)
    {
        /* Route to local PduR */
        pduRTargetPduId = DOIP_PDUR_RX_PDU_ID_BASE;
        pduInfo.SduDataPtr = (uint8*)&messagePtr[4]; /* Skip addresses */
        pduInfo.SduLength = (PduLengthType)(messageLength - 4);
        pduInfo.MetaDataPtr = NULL_PTR;

        PduR_DoIPRxIndication(pduRTargetPduId, &pduInfo);
    }
    else
    {
        /* Route to another tester connection via SoAd */
        pduInfo.SduDataPtr = (uint8*)messagePtr;
        pduInfo.SduLength = (PduLengthType)messageLength;
        pduInfo.MetaDataPtr = NULL_PTR;

        SoAd_IfTransmit(
            (PduIdType)taConnectionIdx + DOIP_TCP_SOAD_TX_PDU_ID_BASE,
            &pduInfo
        );
    }

    return E_OK;
}

static void DoIP_SendDiagnosticAck(
    uint16 socketId,
    uint16 sourceAddress,
    uint16 targetAddress,
    uint8 ackCode,
    uint32 previousMsgLength)
{
    PduInfoType pduInfo;
    uint8* dataPtr = DoIP_Buffer;
    uint16 idx;
    uint16 payloadType;

    if (ackCode == DOIP_DIAG_ACK_CODE_OK)
    {
        payloadType = DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE_ACK;
    }
    else
    {
        payloadType = DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE_NACK;
    }

    /* Build header */
    DoIP_WriteHeader(
        dataPtr,
        payloadType,
        5 /* SA(2) + TA(2) + AC(1) = 5 */
    );

    idx = DOIP_HEADER_LENGTH;

    /* Source Address */
    dataPtr[idx++] = (uint8)(sourceAddress >> 8);
    dataPtr[idx++] = (uint8)(sourceAddress);

    /* Target Address */
    dataPtr[idx++] = (uint8)(targetAddress >> 8);
    dataPtr[idx++] = (uint8)(targetAddress);

    /* ACK/NACK Code */
    dataPtr[idx++] = ackCode;

    /* Previous diagnostic message length (for ACK) */
    if (ackCode == DOIP_DIAG_ACK_CODE_OK)
    {
        dataPtr[idx++] = (uint8)(previousMsgLength >> 24);
        dataPtr[idx++] = (uint8)(previousMsgLength >> 16);
        dataPtr[idx++] = (uint8)(previousMsgLength >> 8);
        dataPtr[idx++] = (uint8)(previousMsgLength);
    }

    pduInfo.SduDataPtr = dataPtr;
    pduInfo.SduLength = idx;
    pduInfo.MetaDataPtr = NULL_PTR;

    SoAd_IfTransmit((PduIdType)socketId, &pduInfo);
}

/*================================================================================
**  INTERNAL FUNCTIONS - ALIVE CHECK
================================================================================*/

Std_ReturnType DoIP_SendAliveCheckRequest(uint16 socketId)
{
    PduInfoType pduInfo;
    uint8* dataPtr = DoIP_Buffer;

    /* Build header */
    DoIP_WriteHeader(
        dataPtr,
        DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST,
        0 /* No payload */
    );

    pduInfo.SduDataPtr = dataPtr;
    pduInfo.SduLength = DOIP_HEADER_LENGTH;
    pduInfo.MetaDataPtr = NULL_PTR;

    DoIP_TesterConnections[socketId].aliveCheckPending = TRUE;
    DoIP_TesterConnections[socketId].aliveCheckTimeout = DOIP_ALIVE_CHECK_RESPONSE_TIMEOUT;

    return SoAd_IfTransmit((PduIdType)socketId + DOIP_TCP_SOAD_TX_PDU_ID_BASE, &pduInfo);
}

void DoIP_ProcessAliveCheckResponse(uint16 socketId, const uint8* responsePtr)
{
    uint16 sourceAddress;

    sourceAddress = ((uint16)responsePtr[0] << 8) | responsePtr[1];

    if (DoIP_TesterConnections[socketId].testerLogicalAddress == sourceAddress)
    {
        DoIP_TesterConnections[socketId].aliveCheckPending = FALSE;
        DoIP_TesterConnections[socketId].aliveCheckTimeout = 0;
    }
}

/*================================================================================
**  INTERNAL FUNCTIONS - UTILITIES
================================================================================*/

static uint16 DoIP_GetPayloadType(const uint8* headerPtr)
{
    return (((uint16)headerPtr[DOIP_HDR_IDX_PAYLOAD_TYPE_HI] << 8) |
            headerPtr[DOIP_HDR_IDX_PAYLOAD_TYPE_LO]);
}

static uint32 DoIP_GetPayloadLength(const uint8* headerPtr)
{
    return (((uint32)headerPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_0] << 24) |
            ((uint32)headerPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_1] << 16) |
            ((uint32)headerPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_2] << 8) |
            headerPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_3]);
}

static void DoIP_WriteHeader(uint8* bufferPtr, uint16 payloadType, uint32 payloadLength)
{
    bufferPtr[DOIP_HDR_IDX_PROTOCOL_VER] = DOIP_PROTOCOL_VERSION;
    bufferPtr[DOIP_HDR_IDX_PROTOCOL_VER_INV] = DOIP_PROTOCOL_VERSION_INVERTED;
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_TYPE_HI] = (uint8)(payloadType >> 8);
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_TYPE_LO] = (uint8)(payloadType);
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_0] = (uint8)(payloadLength >> 24);
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_1] = (uint8)(payloadLength >> 16);
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_2] = (uint8)(payloadLength >> 8);
    bufferPtr[DOIP_HDR_IDX_PAYLOAD_LENGTH_3] = (uint8)(payloadLength);
}

static Std_ReturnType DoIP_SendGenericNack(PduIdType TxPduId, uint8 nackCode)
{
    PduInfoType pduInfo;
    uint8* dataPtr = DoIP_Buffer;

    /* Build header */
    DoIP_WriteHeader(
        dataPtr,
        DOIP_PAYLOAD_TYPE_GENERIC_NACK,
        1 /* NACK code */
    );

    /* NACK code */
    dataPtr[DOIP_HEADER_LENGTH] = nackCode;

    pduInfo.SduDataPtr = dataPtr;
    pduInfo.SduLength = DOIP_HEADER_LENGTH + 1;
    pduInfo.MetaDataPtr = NULL_PTR;

    return SoAd_IfTransmit(TxPduId, &pduInfo);
}

static void DoIP_CloseSocket(uint16 socketId)
{
    if (socketId < DOIP_MAX_TESTER_CONNECTIONS)
    {
        SoAd_CloseConnection((SoAd_SoConIdType)socketId);
        DoIP_ResetTesterConnection((uint8)socketId);
    }
}

static void DoIP_ResetTesterConnection(uint8 connectionIdx)
{
    uint8 i;

    DoIP_TesterConnections[connectionIdx].testerLogicalAddress = 0;
    DoIP_TesterConnections[connectionIdx].targetLogicalAddress = 0;
    DoIP_TesterConnections[connectionIdx].socketState = DOIP_SOCKET_STATE_DISCONNECTED;
    DoIP_TesterConnections[connectionIdx].routingActivationType = 0;
    DoIP_TesterConnections[connectionIdx].aliveCheckPending = FALSE;
    DoIP_TesterConnections[connectionIdx].aliveCheckTimeout = 0;

    for (i = 0; i < 6; i++)
    {
        DoIP_TesterConnections[connectionIdx].testerPhysicalAddress[i] = 0;
    }
}
