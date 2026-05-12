/**
 * @file SomeIpIf.c
 * @brief SOME/IP Interface Implementation
 */

#include "SomeIpIf.h"
#include "SomeIpIf_Cfg.h"
#include "Det.h"
#include <string.h>

typedef enum {
    SOMEIPIF_UNINIT = 0,
    SOMEIPIF_INIT
} SomeIpIf_StateType;

static SomeIpIf_StateType SomeIpIf_State = SOMEIPIF_UNINIT;
static uint16 SomeIpIf_SessionCounter = 0x0001U;

void SomeIpIf_Init(const void* ConfigPtr) {
    (void)ConfigPtr;
    SomeIpIf_State = SOMEIPIF_INIT;
    SomeIpIf_SessionCounter = 0x0001U;
}

Std_ReturnType SomeIpIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
#if (SOMEIPIF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpIf_State != SOMEIPIF_INIT) {
        Det_ReportError(SOMEIPIF_MODULE_ID, 0U, 0x01U, SOMEIPIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR) {
        Det_ReportError(SOMEIPIF_MODULE_ID, 0U, 0x01U, SOMEIPIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    (void)TxPduId;
    return E_OK;
}

static void SomeIpIf_BuildHeader(uint8* Header, SomeIpIf_HeaderType* HdrInfo) {
    Header[0] = (uint8)(HdrInfo->ServiceId >> 8);
    Header[1] = (uint8)(HdrInfo->ServiceId);
    Header[2] = (uint8)(HdrInfo->MethodId >> 8);
    Header[3] = (uint8)(HdrInfo->MethodId);
    Header[4] = (uint8)(HdrInfo->Length >> 24);
    Header[5] = (uint8)(HdrInfo->Length >> 16);
    Header[6] = (uint8)(HdrInfo->Length >> 8);
    Header[7] = (uint8)(HdrInfo->Length);
    Header[8] = (uint8)(HdrInfo->ClientId >> 8);
    Header[9] = (uint8)(HdrInfo->ClientId);
    Header[10] = (uint8)(HdrInfo->SessionId >> 8);
    Header[11] = (uint8)(HdrInfo->SessionId);
    Header[12] = SOMEIP_PROTOCOL_VERSION;
    Header[13] = SOMEIP_INTERFACE_VERSION;
    Header[14] = (uint8)HdrInfo->MessageType;
    Header[15] = (uint8)HdrInfo->ReturnCode;
}

Std_ReturnType SomeIpIf_SendRequest(uint16 ServiceId, uint16 MethodId, const uint8* Data, uint16 Length) {
    SomeIpIf_HeaderType header;
    
    if (SomeIpIf_State != SOMEIPIF_INIT) {
        return E_NOT_OK;
    }
    
    header.ServiceId = ServiceId;
    header.MethodId = MethodId;
    header.Length = 8U + Length;
    header.ClientId = 0x0001U;
    header.SessionId = SomeIpIf_SessionCounter++;
    header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    header.MessageType = SOMEIP_REQUEST;
    header.ReturnCode = SOMEIP_OK;
    
    (void)Data;
    return E_OK;
}

Std_ReturnType SomeIpIf_SendResponse(uint16 ServiceId, uint16 ClientId, uint16 SessionId, const uint8* Data, uint16 Length) {
    SomeIpIf_HeaderType header;
    
    header.ServiceId = ServiceId;
    header.MethodId = 0x0000U;
    header.Length = 8U + Length;
    header.ClientId = ClientId;
    header.SessionId = SessionId;
    header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    header.MessageType = SOMEIP_RESPONSE;
    header.ReturnCode = SOMEIP_OK;
    
    (void)Data;
    return E_OK;
}

Std_ReturnType SomeIpIf_SendNotification(uint16 ServiceId, uint16 EventId, const uint8* Data, uint16 Length) {
    SomeIpIf_HeaderType header;
    
    header.ServiceId = ServiceId;
    header.MethodId = EventId;
    header.Length = 8U + Length;
    header.ClientId = 0x0000U;
    header.SessionId = SomeIpIf_SessionCounter++;
    header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    header.MessageType = SOMEIP_NOTIFICATION;
    header.ReturnCode = SOMEIP_OK;
    
    (void)Data;
    return E_OK;
}

void SomeIpIf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduLength >= SOMEIP_HEADER_SIZE)) {
        SomeIpIf_HeaderType header;
        const uint8* data = PduInfoPtr->SduDataPtr;
        
        header.ServiceId = ((uint16)data[0] << 8) | data[1];
        header.MethodId = ((uint16)data[2] << 8) | data[3];
        header.Length = ((uint32)data[4] << 24) | ((uint32)data[5] << 16) |
                       ((uint32)data[6] << 8) | data[7];
        header.ClientId = ((uint16)data[8] << 8) | data[9];
        header.SessionId = ((uint16)data[10] << 8) | data[11];
        header.ProtocolVersion = data[12];
        header.InterfaceVersion = data[13];
        header.MessageType = (SomeIpIf_MessageTypeType)data[14];
        header.ReturnCode = (SomeIpIf_ReturnCodeType)data[15];
    }
    (void)RxPduId;
}

void SomeIpIf_MainFunction(void) {
    /* Periodic tasks */
}
