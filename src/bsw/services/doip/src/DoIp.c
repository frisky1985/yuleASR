/**
 * @file DoIP.c
 * @brief Diagnostics over IP
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*
 * DoIP.c
 * Diagnostic over IP Implementation (ISO 13400)
 */

#include "DoIP.h"
#include "DoIP_Cfg.h"
#include <string.h>

/*==================================================================================================
 *                                      LOCAL DEFINES
 *=================================================================================================*/
#define DOIP_HEADER_VERSION_POS         0U
#define DOIP_HEADER_INV_VERSION_POS     1U
#define DOIP_HEADER_TYPE_POS            2U
#define DOIP_HEADER_LENGTH_POS          4U

#define DOIP_ACTIVATION_REQ_SIZE        7U
#define DOIP_ACTIVATION_RES_SIZE        13U
#define DOIP_DIAG_MSG_MIN_SIZE          4U

/*==================================================================================================
 *                                      LOCAL VARIABLES
 *=================================================================================================*/
static DoIP_StateType DoIP_InternalState = DOIP_STATE_UNINIT;
static DoIP_ConnectionType DoIP_Connections[DOIP_MAX_CONNECTIONS];
static uint8 DoIP_AnnouncementCount = 0U;
static uint32 DoIP_AnnouncementTimer = 0U;
static uint8 DoIP_TxBuffer[DOIP_MAX_PAYLOAD_LENGTH + DOIP_HEADER_LENGTH];
static uint8 DoIP_RxBuffer[DOIP_MAX_PAYLOAD_LENGTH + DOIP_HEADER_LENGTH];

/* Vehicle Information (configured via Lcfg) */
extern const uint8 DoIP_Vin[];
extern const uint8 DoIP_Eid[];
extern const uint8 DoIP_Gid[];
extern const uint16 DoIP_EntityLogicalAddress;

/*==================================================================================================
 *                                      LOCAL FUNCTION PROTOTYPES
 *=================================================================================================*/
static void DoIP_ProcessUdpMessage(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
static void DoIP_ProcessTcpMessage(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
static Std_ReturnType DoIP_ValidateGenericHeader(const DoIP_GenericHeaderType* Header);
static void DoIP_HandleDiagnosticMessage(uint16 SoConId, const uint8* Data, uint32 Length);
static void DoIP_HandleEntityStatusReq(uint16 SoConId);
static void DoIP_HandlePowerModeReq(uint16 SoConId);
static uint16 DoIP_FindConnection(uint16 SoConId);
static uint16 DoIP_FindFreeConnection(void);

/*==================================================================================================
 *                                      GLOBAL FUNCTIONS
 *=================================================================================================*/

void DoIP_Init(const void* ConfigPtr)
{
    uint8 i;
    
    (void)ConfigPtr;
    
    /* Initialize connections */
    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    {
        DoIP_Connections[i].State = DOIP_CON_STATE_CLOSED;
        DoIP_Connections[i].SourceAddress = 0U;
        DoIP_Connections[i].TargetAddress = 0U;
        DoIP_Connections[i].InactivityTimer = 0U;
        DoIP_Connections[i].SoConId = 0xFFFFU;
        DoIP_Connections[i].RoutingActivated = FALSE;
    }
    
    DoIP_AnnouncementCount = 0U;
    DoIP_AnnouncementTimer = DOIP_CFG_ANNOUNCE_WAIT;
    DoIP_InternalState = DOIP_STATE_ACTIVE;
    
    /* Initialize SoAd connections */
    SoAd_OpenConnection(DOIP_SOCON_UDP_DISCOVERY);
    SoAd_OpenConnection(DOIP_SOCON_TCP_DATA);
}

void DoIP_DeInit(void)
{
    uint8 i;
    
    /* Close all connections */
    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    {
        if (DoIP_Connections[i].State != DOIP_CON_STATE_CLOSED)
        {
            DoIP_CloseConnection(DoIP_Connections[i].SoConId);
        }
    }
    
    /* Close SoAd connections */
    SoAd_CloseConnection(DOIP_SOCON_UDP_DISCOVERY);
    SoAd_CloseConnection(DOIP_SOCON_TCP_DATA);
    
    DoIP_InternalState = DOIP_STATE_UNINIT;
}

void DoIP_MainFunction(void)
{
    uint8 i;
    
    if (DoIP_InternalState != DOIP_STATE_ACTIVE)
    {
        return;
    }
    
    /* Handle vehicle announcement */
    #if (DOIP_VEHICLE_ANNOUNCEMENT == STD_ON)
    if (DoIP_AnnouncementCount < DOIP_CFG_ANNOUNCE_NUM)
    {
        if (DoIP_AnnouncementTimer > 0U)
        {
            DoIP_AnnouncementTimer--;
        }
        else
        {
            DoIP_SendVehicleAnnouncement();
            DoIP_AnnouncementCount++;
            DoIP_AnnouncementTimer = DOIP_CFG_ANNOUNCE_INTERVAL;
        }
    }
    #endif
    
    /* Handle inactivity timers and alive checks */
    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    {
        if (DoIP_Connections[i].State == DOIP_CON_STATE_ROUTING_ACTIVE)
        {
            if (DoIP_Connections[i].InactivityTimer > 0U)
            {
                DoIP_Connections[i].InactivityTimer--;
            }
            else
            {
                /* Inactivity timeout - send alive check */
                DoIP_SendAliveCheckRequest();
                DoIP_Connections[i].State = DOIP_CON_STATE_ALIVE_CHECK;
                DoIP_Connections[i].InactivityTimer = DOIP_CFG_ALIVE_CHECK_TIMEOUT;
            }
        }
        else if (DoIP_Connections[i].State == DOIP_CON_STATE_ALIVE_CHECK)
        {
            if (DoIP_Connections[i].InactivityTimer > 0U)
            {
                DoIP_Connections[i].InactivityTimer--;
            }
            else
            {
                /* Alive check timeout - close connection */
                DoIP_CloseConnection(DoIP_Connections[i].SoConId);
            }
        }
    }
}

void DoIP_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR))
    {
        return;
    }
    
    if (RxPduId == DOIP_PDU_UDP_RX)
    {
        DoIP_ProcessUdpMessage(RxPduId, PduInfoPtr);
    }
    else if (RxPduId == DOIP_PDU_TCP_RX)
    {
        DoIP_ProcessTcpMessage(RxPduId, PduInfoPtr);
    }
}

void DoIP_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
}

/*==================================================================================================
 *                                      VEHICLE ANNOUNCEMENT FUNCTIONS
 *=================================================================================================*/
void DoIP_SendVehicleAnnouncement(void)
{
    PduInfoType pduInfo;
    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
    uint8 i = 0U;
    
    /* Build Generic Header */
    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_VEHICLE_ANNOUNCE, 
                           DOIP_VIN_LENGTH + 2U + DOIP_EID_LENGTH + DOIP_GID_LENGTH + 1U);
    
    /* VIN */
    memcpy(&payload[i], DoIP_Vin, DOIP_VIN_LENGTH);
    i += DOIP_VIN_LENGTH;
    
    /* Logical Address */
    payload[i++] = (uint8)(DoIP_EntityLogicalAddress >> 8);
    payload[i++] = (uint8)(DoIP_EntityLogicalAddress & 0xFFU);
    
    /* EID */
    memcpy(&payload[i], DoIP_Eid, DOIP_EID_LENGTH);
    i += DOIP_EID_LENGTH;
    
    /* GID */
    memcpy(&payload[i], DoIP_Gid, DOIP_GID_LENGTH);
    i += DOIP_GID_LENGTH;
    
    /* Further Action */
    payload[i++] = DOIP_FURTHER_ACTION;
    
    /* Send via UDP */
    pduInfo.SduDataPtr = DoIP_TxBuffer;
    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
    SoAd_IfTransmit(DOIP_PDU_UDP_TX, &pduInfo);
}

void DoIP_ProcessVehicleIdentificationReq(const uint8* Data, uint32 Length)
{
    (void)Data;
    (void)Length;
    
    /* Send immediate vehicle announcement */
    DoIP_SendVehicleAnnouncement();
}

/*==================================================================================================
 *                                      ROUTING ACTIVATION FUNCTIONS
 *=================================================================================================*/
Std_ReturnType DoIP_ProcessRoutingActivationReq(uint16 SoConId, const uint8* Data, uint32 Length)
{
    uint16 sourceAddress;
    uint8 activationType;
    uint8 responseCode;
    uint16 connIdx;
    
    if (Length < DOIP_ACTIVATION_REQ_SIZE)
    {
        return E_NOT_OK;
    }
    
    /* Extract source address */
    sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
    activationType = Data[2];
    
    /* Validate source address */
    if (!DoIP_ValidateSourceAddress(sourceAddress))
    {
        responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_UNKNOWN_SA;
    }
    /* Validate activation type */
    else if ((activationType != DOIP_DEFAULT_ACTIVATION_TYPE) &&
             (activationType != DOIP_WWH_OBD_ACTIVATION_TYPE))
    {
        responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED;
    }
    else
    {
        /* Find or create connection */
        connIdx = DoIP_FindConnection(SoConId);
        if (connIdx >= DOIP_MAX_CONNECTIONS)
        {
            connIdx = DoIP_FindFreeConnection();
        }
        
        if (connIdx >= DOIP_MAX_CONNECTIONS)
        {
            /* All sockets in use */
            responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_ALL_SOCKETS_INUSE;
        }
        else
        {
            /* Activate routing */
            DoIP_Connections[connIdx].State = DOIP_CON_STATE_ROUTING_ACTIVE;
            DoIP_Connections[connIdx].SourceAddress = sourceAddress;
            DoIP_Connections[connIdx].SoConId = SoConId;
            DoIP_Connections[connIdx].RoutingActivated = TRUE;
            DoIP_Connections[connIdx].InactivityTimer = DOIP_CFG_GENERAL_INACTIVITY;
            
            responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_OK;
        }
    }
    
    DoIP_SendRoutingActivationResponse(SoConId, responseCode);
    return E_OK;
}

void DoIP_SendRoutingActivationResponse(uint16 SoConId, uint8 ResponseCode)
{
    PduInfoType pduInfo;
    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
    uint8 i = 0U;
    uint16 testerAddress = 0U;
    uint16 connIdx;
    
    /* Find connection to get tester address */
    connIdx = DoIP_FindConnection(SoConId);
    if (connIdx < DOIP_MAX_CONNECTIONS)
    {
        testerAddress = DoIP_Connections[connIdx].SourceAddress;
    }
    
    /* Build Generic Header */
    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_ROUTING_ACTIVATION_RES, DOIP_ACTIVATION_RES_SIZE);
    
    /* Tester Logical Address */
    payload[i++] = (uint8)(testerAddress >> 8);
    payload[i++] = (uint8)(testerAddress & 0xFFU);
    
    /* Entity Logical Address */
    payload[i++] = (uint8)(DoIP_EntityLogicalAddress >> 8);
    payload[i++] = (uint8)(DoIP_EntityLogicalAddress & 0xFFU);
    
    /* Response Code */
    payload[i++] = ResponseCode;
    
    /* Reserved (ISO reserved) */
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    
    /* OEM Specific (optional) */
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    payload[i++] = 0x00U;
    
    /* Send via TCP */
    pduInfo.SduDataPtr = DoIP_TxBuffer;
    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
}

/*==================================================================================================
 *                                      DIAGNOSTIC MESSAGE FUNCTIONS
 *=================================================================================================*/
Std_ReturnType DoIP_ProcessDiagnosticMessage(uint16 SoConId, const uint8* Data, uint32 Length)
{
    uint16 sourceAddress;
    uint16 targetAddress;
    uint16 connIdx;
    
    if (Length < DOIP_DIAG_MSG_MIN_SIZE)
    {
        return E_NOT_OK;
    }
    
    /* Extract addresses */
    sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
    targetAddress = ((uint16)Data[2] << 8) | (uint16)Data[3];
    
    /* Validate connection */
    connIdx = DoIP_FindConnection(SoConId);
    if (connIdx >= DOIP_MAX_CONNECTIONS)
    {
        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress, 
                                DOIP_DIAG_NACK_INVALID_SA);
        return E_NOT_OK;
    }
    
    /* Validate source address matches connection */
    if (DoIP_Connections[connIdx].SourceAddress != sourceAddress)
    {
        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress,
                                DOIP_DIAG_NACK_INVALID_SA);
        return E_NOT_OK;
    }
    
    /* Validate target address */
    if (!DoIP_ValidateTargetAddress(targetAddress))
    {
        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress,
                                DOIP_DIAG_NACK_UNKNOWN_TA);
        return E_NOT_OK;
    }
    
    /* Send positive acknowledgment */
    DoIP_SendDiagnosticAck(SoConId, sourceAddress, targetAddress, 0x00U);
    
    /* Forward to DCM */
    DoIP_UL_RXINDICATION(SoConId, &Data[DOIP_DIAG_MSG_MIN_SIZE], Length - DOIP_DIAG_MSG_MIN_SIZE);
    
    /* Reset inactivity timer */
    DoIP_ResetInactivityTimer(SoConId);
    
    return E_OK;
}

void DoIP_SendDiagnosticAck(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 AckCode)
{
    PduInfoType pduInfo;
    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
    uint8 i = 0U;
    
    (void)AckCode;
    
    /* Build Generic Header */
    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_DIAGNOSTIC_ACK, 5U);
    
    /* Source Address */
    payload[i++] = (uint8)(SourceAddress >> 8);
    payload[i++] = (uint8)(SourceAddress & 0xFFU);
    
    /* Target Address */
    payload[i++] = (uint8)(TargetAddress >> 8);
    payload[i++] = (uint8)(TargetAddress & 0xFFU);
    
    /* Ack Code */
    payload[i++] = 0x00U;
    
    /* Send via TCP */
    pduInfo.SduDataPtr = DoIP_TxBuffer;
    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
}

void DoIP_SendDiagnosticNack(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 NackCode)
{
    PduInfoType pduInfo;
    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
    uint8 i = 0U;
    
    /* Build Generic Header */
    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_DIAGNOSTIC_NACK, 5U);
    
    /* Source Address */
    payload[i++] = (uint8)(SourceAddress >> 8);
    payload[i++] = (uint8)(SourceAddress & 0xFFU);
    
    /* Target Address */
    payload[i++] = (uint8)(TargetAddress >> 8);
    payload[i++] = (uint8)(TargetAddress & 0xFFU);
    
    /* Nack Code */
    payload[i++] = NackCode;
    
    /* Send via TCP */
    pduInfo.SduDataPtr = DoIP_TxBuffer;
    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
}

/*==================================================================================================
 *                                      ALIVE CHECK FUNCTIONS
 *=================================================================================================*/
void DoIP_SendAliveCheckRequest(void)
{
    PduInfoType pduInfo;
    uint8 i;
    
    /* Build Generic Header */
    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_ALIVE_CHECK_REQ, 0U);
    
    /* Send to all active connections */
    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    {
        if (DoIP_Connections[i].State == DOIP_CON_STATE_ROUTING_ACTIVE)
        {
            pduInfo.SduDataPtr = DoIP_TxBuffer;
            pduInfo.SduLength = DOIP_HEADER_LENGTH;
            SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
            DoIP_Connections[i].State = DOIP_CON_STATE_ALIVE_CHECK;
        }
    }
}

void DoIP_ProcessAliveCheckResponse(uint16 SoConId, const uint8* Data)
{
    uint16 connIdx;
    uint16 sourceAddress;
    
    (void)Data;
    
    connIdx = DoIP_FindConnection(SoConId);
    if (connIdx < DOIP_MAX_CONNECTIONS)
    {
        sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
        
        if ((DoIP_Connections[connIdx].SourceAddress == sourceAddress) &&
            (DoIP_Connections[connIdx].State == DOIP_CON_STATE_ALIVE_CHECK))
        {
            DoIP_Connections[connIdx].State = DOIP_CON_STATE_ROUTING_ACTIVE;
            DoIP_Connections[connIdx].InactivityTimer = DOIP_CFG_GENERAL_INACTIVITY;
        }
    }
}

/*==================================================================================================
 *                                      CONNECTION MANAGEMENT
 *=================================================================================================*/
void DoIP_CloseConnection(uint16 SoConId)
{
    uint16 connIdx;
    
    connIdx = DoIP_FindConnection(SoConId);
    if (connIdx < DOIP_MAX_CONNECTIONS)
    {
        DoIP_Connections[connIdx].State = DOIP_CON_STATE_CLOSED;
