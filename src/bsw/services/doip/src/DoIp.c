/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : Dcm, SoAd, Det, MemMap
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-24
* Author               : AI Agent (DoIp Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include <string.h>
#include "DoIp.h"
#include "DoIp_Cfg.h"
#include "Det.h"
#include "MemMap.h"

/*==================================================================================================
*                             FORWARD DECLARATIONS FOR UPPER/LOWER LAYERS
==================================================================================================*/
extern void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern Std_ReturnType SoAd_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DOIP_INSTANCE_ID                (0x00U)

#define DOIP_PROTOCOL_VERSION           (0x02U)
#define DOIP_PROTOCOL_VERSION_INVERT    (0xFDU)

#define DOIP_GENERIC_HEADER_LENGTH      (8U)
#define DOIP_DIAG_MSG_HEADER_LENGTH     (4U) /* SA + TA */

/* Module state */
#define DOIP_STATE_UNINIT               (0x00U)
#define DOIP_STATE_INIT                 (0x01U)
#define DOIP_STATE_ACTIVE               (0x02U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    #define DOIP_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define DOIP_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Connection runtime state */
typedef struct
{
    DoIp_ConnectionStateType State;
    uint16 SourceAddress;
    uint16 TargetAddress;
    uint16 TesterLogicalAddress;
    uint16 AliveCheckTimer;
    uint16 InactivityTimer;
    boolean IsActive;
} DoIp_ConnectionStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const DoIp_ConfigType* ConfigPtr;
    DoIp_ConnectionStateType Connections[DOIP_MAX_CONNECTIONS];
    uint8 TxBuffer[DOIP_GENERIC_HEADER_LENGTH + DOIP_DIAG_MSG_HEADER_LENGTH + DOIP_MAX_DIAG_REQUEST_LENGTH];
} DoIp_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define DOIP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC DoIp_InternalStateType DoIp_InternalState;

#define DOIP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType DoIp_ParseGenericHeader(const uint8* DataPtr, DoIp_GenericHeaderType* HeaderPtr);
STATIC Std_ReturnType DoIp_BuildGenericHeader(uint8* BufferPtr, uint16 PayloadType, uint32 PayloadLength);
STATIC Std_ReturnType DoIp_FindConnection(uint16 SourceAddress, uint16 TargetAddress, uint8* ConnectionIndex);
STATIC Std_ReturnType DoIp_FindRoutingActivation(uint8 ActivationType, uint8* ActivationIndex);
STATIC void DoIp_HandleRoutingActivationRequest(const uint8* PayloadPtr, uint32 PayloadLength, uint8 ConnIndex);
STATIC void DoIp_HandleDiagnosticMessage(const uint8* PayloadPtr, uint32 PayloadLength, uint8 ConnIndex);
STATIC void DoIp_SendRoutingActivationResponse(uint8 ConnIndex, uint8 ResponseCode);
STATIC void DoIp_ForwardToDcm(const uint8* DataPtr, uint16 Length, uint16 SourceAddress, uint16 TargetAddress);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DOIP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Parse DoIP Generic Header from raw data
 * @param   DataPtr     - Pointer to raw data buffer
 * @param   HeaderPtr   - Output pointer for parsed header
 * @return  E_OK if parsing successful, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoIp_ParseGenericHeader(const uint8* DataPtr, DoIp_GenericHeaderType* HeaderPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if ((DataPtr != NULL_PTR) && (HeaderPtr != NULL_PTR))
    {
        HeaderPtr->ProtocolVersion = DataPtr[0];
        HeaderPtr->InverseProtocolVersion = DataPtr[1];
        HeaderPtr->PayloadType = (uint16)(((uint16)DataPtr[2] << 8) | (uint16)DataPtr[3]);
        HeaderPtr->PayloadLength = (uint32)(((uint32)DataPtr[4] << 24) |
                                            ((uint32)DataPtr[5] << 16) |
                                            ((uint32)DataPtr[6] << 8)  |
                                            ((uint32)DataPtr[7]));
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Build DoIP Generic Header into buffer
 * @param   BufferPtr       - Pointer to output buffer
 * @param   PayloadType     - DoIP payload type
 * @param   PayloadLength   - Length of payload
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoIp_BuildGenericHeader(uint8* BufferPtr, uint16 PayloadType, uint32 PayloadLength)
{
    Std_ReturnType result = E_NOT_OK;

    if (BufferPtr != NULL_PTR)
    {
        BufferPtr[0] = DOIP_PROTOCOL_VERSION;
        BufferPtr[1] = DOIP_PROTOCOL_VERSION_INVERT;
        BufferPtr[2] = (uint8)(PayloadType >> 8);
        BufferPtr[3] = (uint8)(PayloadType & 0xFFU);
        BufferPtr[4] = (uint8)(PayloadLength >> 24);
        BufferPtr[5] = (uint8)(PayloadLength >> 16);
        BufferPtr[6] = (uint8)(PayloadLength >> 8);
        BufferPtr[7] = (uint8)(PayloadLength & 0xFFU);
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Find connection by source and target address
 * @param   SourceAddress   - Source logical address
 * @param   TargetAddress   - Target logical address
 * @param   ConnectionIndex - Output parameter for found connection index
 * @return  E_OK if found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoIp_FindConnection(uint16 SourceAddress, uint16 TargetAddress, uint8* ConnectionIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const DoIp_ConfigType* configPtr = DoIp_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (ConnectionIndex != NULL_PTR))
    {
        for (i = 0U; i < configPtr->NumConnections; i++)
        {
            if ((configPtr->Connections[i].SourceAddress == SourceAddress) &&
                (configPtr->Connections[i].TargetAddress == TargetAddress))
            {
                *ConnectionIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find routing activation by activation type
 * @param   ActivationType     - Routing activation type code
 * @param   ActivationIndex    - Output parameter for found activation index
 * @return  E_OK if found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoIp_FindRoutingActivation(uint8 ActivationType, uint8* ActivationIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const DoIp_ConfigType* configPtr = DoIp_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (ActivationIndex != NULL_PTR))
    {
        for (i = 0U; i < configPtr->NumRoutingActivations; i++)
        {
            if (configPtr->RoutingActivations[i].ActivationType == ActivationType)
            {
                *ActivationIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Handle routing activation request
 * @param   PayloadPtr      - Pointer to payload data
 * @param   PayloadLength   - Length of payload
 * @param   ConnIndex       - Connection index
 */
STATIC void DoIp_HandleRoutingActivationRequest(const uint8* PayloadPtr, uint32 PayloadLength, uint8 ConnIndex)
{
    uint8 activationType;
    uint8 activationIndex;
    uint16 sourceAddress;

    if ((PayloadPtr != NULL_PTR) && (PayloadLength >= 7U) && (ConnIndex < DOIP_MAX_CONNECTIONS))
    {
        sourceAddress = (uint16)(((uint16)PayloadPtr[0] << 8) | (uint16)PayloadPtr[1]);
        activationType = PayloadPtr[2];

        if (DoIp_FindRoutingActivation(activationType, &activationIndex) == E_OK)
        {
            const DoIp_RoutingActivationConfigType* raPtr = &DoIp_InternalState.ConfigPtr->RoutingActivations[activationIndex];

            /* Update connection state */
            DoIp_InternalState.Connections[ConnIndex].State = DOIP_CONNECTION_REGISTERED;
            DoIp_InternalState.Connections[ConnIndex].SourceAddress = raPtr->SourceAddress;
            DoIp_InternalState.Connections[ConnIndex].TargetAddress = raPtr->TargetAddress;
            DoIp_InternalState.Connections[ConnIndex].TesterLogicalAddress = sourceAddress;
            DoIp_InternalState.Connections[ConnIndex].IsActive = TRUE;
            DoIp_InternalState.Connections[ConnIndex].InactivityTimer = 0U;

            /* Send positive response */
            DoIp_SendRoutingActivationResponse(ConnIndex, 0x00U);
        }
        else
        {
            /* Unknown activation type - send negative response */
            DoIp_SendRoutingActivationResponse(ConnIndex, 0x06U);
        }
    }
}

/**
 * @brief   Handle diagnostic message
 * @param   PayloadPtr      - Pointer to payload data
 * @param   PayloadLength   - Length of payload
 * @param   ConnIndex       - Connection index
 */
STATIC void DoIp_HandleDiagnosticMessage(const uint8* PayloadPtr, uint32 PayloadLength, uint8 ConnIndex)
{
    uint16 sourceAddress;
    uint16 targetAddress;
    uint16 diagDataLength;

    if ((PayloadPtr != NULL_PTR) && (PayloadLength >= 4U) && (ConnIndex < DOIP_MAX_CONNECTIONS))
    {
        sourceAddress = (uint16)(((uint16)PayloadPtr[0] << 8) | (uint16)PayloadPtr[1]);
        targetAddress = (uint16)(((uint16)PayloadPtr[2] << 8) | (uint16)PayloadPtr[3]);
        diagDataLength = (uint16)(PayloadLength - 4U);

        /* Reset inactivity timer */
        DoIp_InternalState.Connections[ConnIndex].InactivityTimer = 0U;

        /* Forward diagnostic data to DCM */
        if (diagDataLength > 0U)
        {
            DoIp_ForwardToDcm(&PayloadPtr[4], diagDataLength, sourceAddress, targetAddress);
        }
    }
}

/**
 * @brief   Send routing activation response
 * @param   ConnIndex       - Connection index
 * @param   ResponseCode    - Response code (0x00 = accepted)
 */
STATIC void DoIp_SendRoutingActivationResponse(uint8 ConnIndex, uint8 ResponseCode)
{
    uint8 txBuffer[DOIP_GENERIC_HEADER_LENGTH + 13U];
    PduInfoType pduInfo;
    uint16 testerAddr;
    uint16 targetAddr;

    if ((ConnIndex < DOIP_MAX_CONNECTIONS) && (DoIp_InternalState.ConfigPtr != NULL_PTR))
    {
        testerAddr = DoIp_InternalState.Connections[ConnIndex].TesterLogicalAddress;
        targetAddr = DoIp_InternalState.Connections[ConnIndex].TargetAddress;

        /* Build generic header: Payload Type 0x0006, Length = 13 */
        (void)DoIp_BuildGenericHeader(txBuffer, 0x0006U, 13U);

        /* Build payload */
        txBuffer[8] = (uint8)(testerAddr >> 8);
        txBuffer[9] = (uint8)(testerAddr & 0xFFU);
        txBuffer[10] = (uint8)(targetAddr >> 8);
        txBuffer[11] = (uint8)(targetAddr & 0xFFU);
        txBuffer[12] = ResponseCode;
        /* Reserved OEM specific bytes */
        txBuffer[13] = 0x00U;
        txBuffer[14] = 0x00U;
        txBuffer[15] = 0x00U;
        txBuffer[16] = 0x00U;
        txBuffer[17] = 0x00U;
        txBuffer[18] = 0x00U;
        txBuffer[19] = 0x00U;
        txBuffer[20] = 0x00U;

        pduInfo.SduDataPtr = txBuffer;
        pduInfo.SduLength = 21U;
        pduInfo.MetaDataPtr = NULL_PTR;

        (void)SoAd_Transmit(DOIP_SOAD_TX_PDU_ID, &pduInfo);
    }
}

/**
 * @brief   Forward diagnostic data to DCM
 * @param   DataPtr         - Pointer to diagnostic data
 * @param   Length          - Length of diagnostic data
 * @param   SourceAddress   - Source logical address
 * @param   TargetAddress   - Target logical address
 */
STATIC void DoIp_ForwardToDcm(const uint8* DataPtr, uint16 Length, uint16 SourceAddress, uint16 TargetAddress)
{
    PduInfoType pduInfo;
    uint8 dcmBuffer[DOIP_MAX_DIAG_REQUEST_LENGTH];

    if ((DataPtr != NULL_PTR) && (Length > 0U) && (Length <= DOIP_MAX_DIAG_REQUEST_LENGTH))
    {
        (void)memcpy(dcmBuffer, DataPtr, Length);
        pduInfo.SduDataPtr = dcmBuffer;
        pduInfo.SduLength = Length;
        pduInfo.MetaDataPtr = NULL_PTR;

        (void)SourceAddress;
        (void)TargetAddress;

        Dcm_RxIndication(DOIP_DCM_RX_DIAG_RESPONSE, &pduInfo);
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the DoIp module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
 */
void DoIp_Init(const DoIp_ConfigType* ConfigPtr)
{
    uint8 i;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_INIT, DOIP_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    DoIp_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize all connection states */
    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    {
        DoIp_InternalState.Connections[i].State = DOIP_CONNECTION_CLOSED;
        DoIp_InternalState.Connections[i].SourceAddress = 0U;
        DoIp_InternalState.Connections[i].TargetAddress = 0U;
        DoIp_InternalState.Connections[i].TesterLogicalAddress = 0U;
        DoIp_InternalState.Connections[i].AliveCheckTimer = 0U;
        DoIp_InternalState.Connections[i].InactivityTimer = 0U;
        DoIp_InternalState.Connections[i].IsActive = FALSE;
    }

    /* Set module state to initialized */
    DoIp_InternalState.State = DOIP_STATE_INIT;
}

/**
 * @brief   Deinitializes the DoIp module
 * @param   None
 * @return  None
 */
void DoIp_DeInit(void)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_DEINIT, DOIP_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    DoIp_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    DoIp_InternalState.State = DOIP_STATE_UNINIT;
}

/**
 * @brief   Transmits a diagnostic message via DoIP
 * @param   TxPduId     - PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if transmission successful, E_NOT_OK otherwise
 */
Std_ReturnType DoIp_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint8* txBuffer = DoIp_InternalState.TxBuffer;
    PduInfoType txPduInfo;
    uint16 sourceAddress;
    uint16 targetAddress;
    uint32 payloadLength;

    (void)TxPduId;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_IFTRANSMIT, DOIP_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_IFTRANSMIT, DOIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((PduInfoPtr != NULL_PTR) && (DoIp_InternalState.ConfigPtr != NULL_PTR))
    {
        /* Use first configured connection's addresses */
        sourceAddress = DoIp_InternalState.ConfigPtr->Connections[0].SourceAddress;
        targetAddress = DoIp_InternalState.ConfigPtr->Connections[0].TargetAddress;

        /* Calculate payload length: SA(2) + TA(2) + UDS data */
        payloadLength = DOIP_DIAG_MSG_HEADER_LENGTH + PduInfoPtr->SduLength;

        /* Build generic header for Diagnostic Message (0x8001) */
        (void)DoIp_BuildGenericHeader(txBuffer, 0x8001U, payloadLength);

        /* Build diagnostic message header */
        txBuffer[8] = (uint8)(sourceAddress >> 8);
        txBuffer[9] = (uint8)(sourceAddress & 0xFFU);
        txBuffer[10] = (uint8)(targetAddress >> 8);
        txBuffer[11] = (uint8)(targetAddress & 0xFFU);

        /* Copy diagnostic data */
        if ((PduInfoPtr->SduDataPtr != NULL_PTR) && (PduInfoPtr->SduLength > 0U))
        {
            uint16 copyLength = (PduInfoPtr->SduLength < DOIP_MAX_DIAG_REQUEST_LENGTH) ?
                                PduInfoPtr->SduLength : DOIP_MAX_DIAG_REQUEST_LENGTH;
            (void)memcpy(&txBuffer[12], PduInfoPtr->SduDataPtr, copyLength);
        }

        txPduInfo.SduDataPtr = txBuffer;
        txPduInfo.SduLength = (PduLengthType)(DOIP_GENERIC_HEADER_LENGTH + payloadLength);
        txPduInfo.MetaDataPtr = NULL_PTR;

        result = SoAd_Transmit(DOIP_SOAD_TX_PDU_ID, &txPduInfo);
    }

    return result;
}

/**
 * @brief   Receive indication from lower layer
 * @param   RxPduId     - Received PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  None
 */
void DoIp_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    DoIp_GenericHeaderType header;
    uint8 connIndex;

    (void)RxPduId;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_IFRXINDICATION, DOIP_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_IFRXINDICATION, DOIP_E_PARAM_POINTER);
        return;
    }
#endif

    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR) &&
        (PduInfoPtr->SduLength >= DOIP_GENERIC_HEADER_LENGTH))
    {
        if (DoIp_ParseGenericHeader(PduInfoPtr->SduDataPtr, &header) == E_OK)
        {
            /* Validate protocol version */
            if ((header.ProtocolVersion == DOIP_PROTOCOL_VERSION) &&
                (header.InverseProtocolVersion == DOIP_PROTOCOL_VERSION_INVERT))
            {
                /* Use connection 0 as default for received messages */
                connIndex = 0U;

                switch (header.PayloadType)
                {
                    case DOIP_PAYLOAD_ROUTING_ACTIVATION_REQ:
                        DoIp_HandleRoutingActivationRequest(
                            &PduInfoPtr->SduDataPtr[DOIP_GENERIC_HEADER_LENGTH],
                            header.PayloadLength, connIndex);
                        break;

                    case DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE:
                        DoIp_HandleDiagnosticMessage(
                            &PduInfoPtr->SduDataPtr[DOIP_GENERIC_HEADER_LENGTH],
                            header.PayloadLength, connIndex);
                        break;

                    case DOIP_PAYLOAD_ALIVE_CHECK_RES:
                        /* Reset alive check timer */
                        if (connIndex < DOIP_MAX_CONNECTIONS)
                        {
                            DoIp_InternalState.Connections[connIndex].AliveCheckTimer = 0U;
                        }
                        break;

                    default:
                        /* Unsupported payload type - ignore or send NACK */
                        break;
                }
            }
        }
    }
}

/**
 * @brief   Activates a diagnostic routing path
 * @param   SourceAddress   - Source logical address
 * @param   TargetAddress   - Target logical address
 * @param   ActivationType  - Routing activation type
 * @return  E_OK if activation successful, E_NOT_OK otherwise
 */
Std_ReturnType DoIp_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 connIndex;
    uint8 activationIndex;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_ACTIVATEROUTING, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (DoIp_FindConnection(SourceAddress, TargetAddress, &connIndex) == E_OK)
    {
        if (DoIp_FindRoutingActivation(ActivationType, &activationIndex) == E_OK)
        {
            const DoIp_RoutingActivationConfigType* raPtr =
                &DoIp_InternalState.ConfigPtr->RoutingActivations[activationIndex];

            DoIp_InternalState.Connections[connIndex].State = DOIP_CONNECTION_REGISTERED;
            DoIp_InternalState.Connections[connIndex].SourceAddress = raPtr->SourceAddress;
            DoIp_InternalState.Connections[connIndex].TargetAddress = raPtr->TargetAddress;
            DoIp_InternalState.Connections[connIndex].IsActive = TRUE;
            DoIp_InternalState.Connections[connIndex].InactivityTimer = 0U;

            result = E_OK;
        }
        else
        {
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
            DOIP_DET_REPORT_ERROR(DOIP_SID_ACTIVATEROUTING, DOIP_E_INVALID_ROUTING_ACTIVATION);
#endif
        }
    }
    else
    {
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
        DOIP_DET_REPORT_ERROR(DOIP_SID_ACTIVATEROUTING, DOIP_E_INVALID_CONNECTION);
#endif
    }

    return result;
}

/**
 * @brief   Closes an active diagnostic connection
 * @param   ConnectionId - Connection identifier to close
 * @return  E_OK if closed, E_NOT_OK otherwise
 */
Std_ReturnType DoIp_CloseConnection(uint16 ConnectionId)
{
    Std_ReturnType result = E_NOT_OK;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_CLOSECONNECTION, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (ConnectionId < DOIP_MAX_CONNECTIONS)
    {
        DoIp_InternalState.Connections[ConnectionId].State = DOIP_CONNECTION_CLOSED;
        DoIp_InternalState.Connections[ConnectionId].IsActive = FALSE;
        DoIp_InternalState.Connections[ConnectionId].AliveCheckTimer = 0U;
        DoIp_InternalState.Connections[ConnectionId].InactivityTimer = 0U;
        result = E_OK;
    }
    else
    {
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
        DOIP_DET_REPORT_ERROR(DOIP_SID_CLOSECONNECTION, DOIP_E_INVALID_CONNECTION);
#endif
    }

    return result;
}

/**
 * @brief   Transmit confirmation callback from SoAd
 * @param   TxPduId - PDU identifier
 * @param   result  - Transmission result
 * @return  None
 */
void DoIp_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;

#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIp_InternalState.State == DOIP_STATE_UNINIT)
    {
        DOIP_DET_REPORT_ERROR(DOIP_SID_SOADTXCONFIRMATION, DOIP_E_UNINIT);
        return;
    }
#endif

    /* Forward confirmation to DCM */
    Dcm_TxConfirmation(DOIP_DCM_TX_DIAG_REQUEST, result);
}

/**
 * @brief   Main function for periodic processing
 * @param   None
 * @return  None
 */
void DoIp_MainFunction(void)
{
    uint8 i;
    const DoIp_ConfigType* configPtr = DoIp_InternalState.ConfigPtr;

    if (DoIp_InternalState.State == DOIP_STATE_INIT)
    {
        for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
        {
            DoIp_ConnectionStateType* connPtr = &DoIp_InternalState.Connections[i];

            if (connPtr->IsActive)
            {
                /* Handle inactivity timeout */
                if (configPtr != NULL_PTR)
                {
                    connPtr->InactivityTimer += DOIP_MAIN_FUNCTION_PERIOD_MS;

                    if (connPtr->InactivityTimer >= configPtr->Connections[i].GeneralInactivityTimeoutMs)
                    {
                        /* Close connection due to inactivity */
                        connPtr->State = DOIP_CONNECTION_CLOSED;
                        connPtr->IsActive = FALSE;
                    }

                    /* Handle alive check */
                    if (configPtr->Connections[i].AliveCheckEnabled)
                    {
                        connPtr->AliveCheckTimer += DOIP_MAIN_FUNCTION_PERIOD_MS;

                        if (connPtr->AliveCheckTimer >= configPtr->Connections[i].AliveCheckTimeoutMs)
                        {
                            /* Send alive check request */
                            uint8 aliveBuffer[DOIP_GENERIC_HEADER_LENGTH];
                            PduInfoType pduInfo;

                            (void)DoIp_BuildGenericHeader(aliveBuffer, DOIP_PAYLOAD_ALIVE_CHECK_REQ, 0U);
                            pduInfo.SduDataPtr = aliveBuffer;
                            pduInfo.SduLength = DOIP_GENERIC_HEADER_LENGTH;
                            pduInfo.MetaDataPtr = NULL_PTR;

                            (void)SoAd_Transmit(DOIP_SOAD_TX_PDU_ID, &pduInfo);
                            connPtr->AliveCheckTimer = 0U;
                        }
                    }
                }
            }
        }
    }
}

#define DOIP_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
