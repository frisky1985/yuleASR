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
 * @file SomeIp.c
 * @brief SOME/IP Protocol Implementation
 */

#include "SomeIp.h"
#include "Det.h"
#include <string.h>

/* Internal State */
static boolean SomeIp_Initialized = FALSE;
static const SomeIp_ConfigType* SomeIp_ConfigPtr = NULL_PTR;
static SomeIp_SessionIdType SomeIp_CurrentSessionId = 0;

/* Version Info */
#define SOMEIP_VENDOR_ID                   0x0001
#define SOMEIP_INSTANCE_ID                 0x00

void SomeIp_Init(const SomeIp_ConfigType* ConfigPtr)
{
#if (STD_ON == SOMEIP_DEV_ERROR_DETECT)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(SOMEIP_MODULE_ID, SOMEIP_INSTANCE_ID, SOMEIP_INIT_SID, SOMEIP_E_INVALID_POINTER);
        return;
    }
#endif

    SomeIp_ConfigPtr = ConfigPtr;
    SomeIp_CurrentSessionId = 0;
    SomeIp_Initialized = TRUE;
}

void SomeIp_DeInit(void)
{
    if (!SomeIp_Initialized)
    {
        return;
    }

    SomeIp_ConfigPtr = NULL_PTR;
    SomeIp_Initialized = FALSE;
}

void SomeIp_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    if (VersionInfoPtr != NULL_PTR)
    {
        VersionInfoPtr->vendorID = SOMEIP_VENDOR_ID;
        VersionInfoPtr->moduleID = SOMEIP_MODULE_ID;
        VersionInfoPtr->sw_major_version = SOMEIP_SW_MAJOR_VERSION;
        VersionInfoPtr->sw_minor_version = SOMEIP_SW_MINOR_VERSION;
        VersionInfoPtr->sw_patch_version = SOMEIP_SW_PATCH_VERSION;
    }
}

Std_ReturnType SomeIp_SendRequest(
    SomeIp_ClientIdType ClientId,
    SomeIp_ServiceIdType ServiceId,
    SomeIp_MethodIdType MethodId,
    const uint8* Payload,
    SomeIp_LengthType PayloadLength
)
{
    SomeIp_MessageType message;
    
    if (!SomeIp_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Build header */
    message.Header.MessageId = SomeIp_CreateMessageId(ServiceId, MethodId);
    message.Header.Length = 8U + PayloadLength; /* RequestId(4) + Protocol(1) + Interface(1) + MsgType(1) + Return(1) + Payload */
    message.Header.RequestId = SomeIp_CreateRequestId(ClientId, ++SomeIp_CurrentSessionId);
    message.Header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    message.Header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    message.Header.MessageType = SOMEIP_MSG_REQUEST;
    message.Header.ReturnCode = SOMEIP_RET_OK;
    
/*     message.Payload = (uint8*)Payload; */
/*     message.PayloadLength = PayloadLength; */
    
    /* NOTE: Socket send pending network stack integration */
    
    return E_OK;
}

Std_ReturnType SomeIp_SendResponse(
    SomeIp_RequestIdType RequestId,
    const uint8* Payload,
    SomeIp_LengthType PayloadLength,
    SomeIp_ReturnCodeType ReturnCode
)
{
    SomeIp_MessageType message;
    
    if (!SomeIp_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Build header (MessageId will be filled from original request) */
    message.Header.Length = 8U + PayloadLength;
    message.Header.RequestId = RequestId;
    message.Header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    message.Header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    message.Header.MessageType = (ReturnCode == SOMEIP_RET_OK) ? SOMEIP_MSG_RESPONSE : SOMEIP_MSG_ERROR;
    message.Header.ReturnCode = ReturnCode;
    
/*     message.Payload = (uint8*)Payload; */
/*     message.PayloadLength = PayloadLength; */
    
    /* NOTE: Socket send pending network stack integration */
    
    return E_OK;
}

Std_ReturnType SomeIp_SendNotification(
    SomeIp_ServiceIdType ServiceId,
    SomeIp_MethodIdType EventId,
    const uint8* Payload,
    SomeIp_LengthType PayloadLength
)
{
    SomeIp_MessageType message;
    
    if (!SomeIp_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Build header */
    message.Header.MessageId = SomeIp_CreateMessageId(ServiceId, EventId);
    message.Header.Length = 8U + PayloadLength;
    message.Header.RequestId = 0; /* Notifications don't use RequestId */
    message.Header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    message.Header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    message.Header.MessageType = SOMEIP_MSG_NOTIFICATION;
    message.Header.ReturnCode = SOMEIP_RET_OK;
    
/*     message.Payload = (uint8*)Payload; */
/*     message.PayloadLength = PayloadLength; */
    
    /* NOTE: Send to all subscribed clients pending network stack integration */
    
    return E_OK;
}

void SomeIp_RxIndication(const uint8* Data, uint32 Length)
{
    SomeIp_MessageType message;
    
    if (!SomeIp_Initialized || (Data == NULL_PTR) || Length < SOMEIP_HEADER_SIZE)
    {
        return;
    }
    
    /* Parse header */
    if (SomeIp_ParseHeader(Data, &message.Header) != E_OK)
    {
        return;
    }
    
    /* Extract payload */
    message.Payload = (uint8*)(Data + SOMEIP_HEADER_SIZE);
    message.PayloadLength = message.Header.Length - 8U;
    
    /* Process message */
    SomeIp_ProcessMessage(&message);
}

Std_ReturnType SomeIp_ProcessMessage(const SomeIp_MessageType* MessagePtr)
{
    SomeIp_ServiceIdType serviceId;
    SomeIp_MethodIdType methodId;
    
    if (MessagePtr == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* Extract service and method IDs */
    SomeIp_ExtractIds(MessagePtr->Header.MessageId, &serviceId, &methodId);
    
    /* Process based on message type */
    switch (MessagePtr->Header.MessageType)
    {
        case SOMEIP_MSG_REQUEST:
        case SOMEIP_MSG_REQUEST_NO_RETURN:
            /* Handle request */
            /* NOTE: Service handler dispatch managed by application registration */
            break;
            
        case SOMEIP_MSG_NOTIFICATION:
            /* Handle notification */
            /* NOTE: Notification callback dispatch managed by application registration */
            break;
            
        case SOMEIP_MSG_RESPONSE:
            /* Handle response */
            /* NOTE: Response callback dispatch managed by application registration */
            break;
            
        case SOMEIP_MSG_ERROR:
            /* Handle error */
            break;
            
        default:
            return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType SomeIp_ParseHeader(const uint8* Data, SomeIp_HeaderType* HeaderPtr)
{
    if ((Data == NULL_PTR) || (HeaderPtr == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    /* Parse header fields (big-endian) */
    HeaderPtr->MessageId = ((uint32)Data[0] << 24) |
                           ((uint32)Data[1] << 16) |
                           ((uint32)Data[2] << 8) |
                           (uint32)Data[3];
    
    HeaderPtr->Length = ((uint32)Data[4] << 24) |
                        ((uint32)Data[5] << 16) |
                        ((uint32)Data[6] << 8) |
                        (uint32)Data[7];
    
    HeaderPtr->RequestId = ((uint32)Data[8] << 24) |
                           ((uint32)Data[9] << 16) |
                           ((uint32)Data[10] << 8) |
                           (uint32)Data[11];
    
    HeaderPtr->ProtocolVersion = Data[12];
    HeaderPtr->InterfaceVersion = Data[13];
    HeaderPtr->MessageType = Data[14];
    HeaderPtr->ReturnCode = Data[15];
    
    /* Validate protocol version */
    if (HeaderPtr->ProtocolVersion != SOMEIP_PROTOCOL_VERSION)
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType SomeIp_SerializeHeader(const SomeIp_HeaderType* HeaderPtr, uint8* Data)
{
    if ((HeaderPtr == NULL_PTR) || (Data == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    /* Serialize header fields (big-endian) */
    Data[0] = (uint8)(HeaderPtr->MessageId >> 24);
    Data[1] = (uint8)(HeaderPtr->MessageId >> 16);
    Data[2] = (uint8)(HeaderPtr->MessageId >> 8);
    Data[3] = (uint8)(HeaderPtr->MessageId);
    
    Data[4] = (uint8)(HeaderPtr->Length >> 24);
    Data[5] = (uint8)(HeaderPtr->Length >> 16);
    Data[6] = (uint8)(HeaderPtr->Length >> 8);
    Data[7] = (uint8)(HeaderPtr->Length);
    
    Data[8] = (uint8)(HeaderPtr->RequestId >> 24);
    Data[9] = (uint8)(HeaderPtr->RequestId >> 16);
    Data[10] = (uint8)(HeaderPtr->RequestId >> 8);
    Data[11] = (uint8)(HeaderPtr->RequestId);
    
    Data[12] = HeaderPtr->ProtocolVersion;
    Data[13] = HeaderPtr->InterfaceVersion;
    Data[14] = HeaderPtr->MessageType;
    Data[15] = HeaderPtr->ReturnCode;
    
    return E_OK;
}

SomeIp_MessageIdType SomeIp_CreateMessageId(SomeIp_ServiceIdType ServiceId, SomeIp_MethodIdType MethodId)
{
    return ((uint32)ServiceId << 16) | (uint32)MethodId;
}

SomeIp_RequestIdType SomeIp_CreateRequestId(SomeIp_ClientIdType ClientId, SomeIp_SessionIdType SessionId)
{
    return ((uint32)ClientId << 16) | (uint32)SessionId;
}

void SomeIp_ExtractIds(SomeIp_MessageIdType MessageId, SomeIp_ServiceIdType* ServiceId, SomeIp_MethodIdType* MethodId)
{
    if (ServiceId != NULL_PTR)
    {
        *ServiceId = (SomeIp_ServiceIdType)(MessageId >> 16);
    }
    if (MethodId != NULL_PTR)
    {
        *MethodId = (SomeIp_MethodIdType)(MessageId & 0xFFFFU);
    }
}

void SomeIp_TxConfirmation(SomeIp_RequestIdType RequestId)
{
    /* Handle transmission confirmation */
    (void)RequestId;
}
