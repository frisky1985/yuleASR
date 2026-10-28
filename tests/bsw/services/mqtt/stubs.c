/**
 * @file stubs.c
 * @brief Configurable TcpIp stubs for the Mqtt unit tests.
 *
 * The Mqtt implementation calls into the TcpIp service (SocketCreate /
 * SocketClose on connect/disconnect, Send/Receive/IsConnected from the
 * protocol backend). These stubs use the real TcpIp.h signatures so the
 * production Mqtt.c/Mqtt_Tls.c compile against them unchanged, and record
 * socket activity for the test assertions.
 *
 * Behaviour is configurable so both success and failure paths of the
 * production code can be driven:
 *  - mock_TcpIp_SocketCreate_Fail  -> SocketCreate returns E_NOT_OK
 *  - mock_TcpIp_Send_Fail          -> Send returns error
 *  - mock_TcpIp_Receive_Fail       -> Receive returns error
 *  - mock_TcpIp_IsConnected_Result -> value returned by IsConnected
 *  - mock_TcpIp_InjectRx()         -> data returned by the next Receive calls
 * All bytes passed to Send are appended to mock_TcpIp_SendLog so packet
 * encoding (CONNECT/PUBLISH/SUBSCRIBE/PINGREQ/DISCONNECT) can be asserted.
 */

#include "TcpIp.h"
#include <string.h>

/* ---- Configurable behaviour ---- */
int mock_TcpIp_SocketCreate_Fail = 0;
int mock_TcpIp_Send_Fail = 0;
int mock_TcpIp_Receive_Fail = 0;
int mock_TcpIp_IsConnected_Result = 1;

/* ---- Counters / log ---- */
uint32 mock_TcpIp_SocketCreate_Count = 0U;
uint32 mock_TcpIp_SocketClose_Count = 0U;
uint32 mock_TcpIp_Send_Count = 0U;
uint32 mock_TcpIp_Receive_Count = 0U;
TcpIp_SocketIdType mock_TcpIp_LastSocketId = TCPIP_SOCKETID_INVALID;
uint8 mock_TcpIp_SendLog[4096];
uint32 mock_TcpIp_SendLogLen = 0U;

/* ---- Receive injection ---- */
const uint8* mock_TcpIp_RxData = NULL_PTR;
uint32 mock_TcpIp_RxLen = 0U;
uint32 mock_TcpIp_RxPos = 0U;

void mock_TcpIp_Reset(void) {
    mock_TcpIp_SocketCreate_Fail = 0;
    mock_TcpIp_Send_Fail = 0;
    mock_TcpIp_Receive_Fail = 0;
    mock_TcpIp_IsConnected_Result = 1;
    mock_TcpIp_SocketCreate_Count = 0U;
    mock_TcpIp_SocketClose_Count = 0U;
    mock_TcpIp_Send_Count = 0U;
    mock_TcpIp_Receive_Count = 0U;
    mock_TcpIp_LastSocketId = TCPIP_SOCKETID_INVALID;
    mock_TcpIp_SendLogLen = 0U;
    mock_TcpIp_RxData = NULL_PTR;
    mock_TcpIp_RxLen = 0U;
    mock_TcpIp_RxPos = 0U;
}

void mock_TcpIp_InjectRx(const uint8* data, uint32 len) {
    mock_TcpIp_RxData = data;
    mock_TcpIp_RxLen = len;
    mock_TcpIp_RxPos = 0U;
}

Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* SocketId) {
    mock_TcpIp_SocketCreate_Count++;
    if (mock_TcpIp_SocketCreate_Fail != 0) {
        return E_NOT_OK;
    }
    if (SocketId != NULL_PTR) {
        *SocketId = (TcpIp_SocketIdType)0U;
        mock_TcpIp_LastSocketId = *SocketId;
    }
    return E_OK;
}

void TcpIp_SocketClose(TcpIp_SocketIdType SocketId) {
    mock_TcpIp_SocketClose_Count++;
    if (SocketId == mock_TcpIp_LastSocketId) {
        mock_TcpIp_LastSocketId = TCPIP_SOCKETID_INVALID;
    }
}

boolean TcpIp_IsConnected(TcpIp_SocketIdType SocketId) {
    (void)SocketId;
    return (mock_TcpIp_IsConnected_Result != 0) ? TRUE : FALSE;
}

TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length) {
    (void)SocketId;
    mock_TcpIp_Send_Count++;
    if (mock_TcpIp_Send_Fail != 0) {
        return TCPIP_E_NOT_OK;
    }
    if ((Data != NULL_PTR) && (Length > 0U)) {
        uint32 space = (uint32)sizeof(mock_TcpIp_SendLog) - mock_TcpIp_SendLogLen;
        uint32 copy = ((uint32)Length <= space) ? (uint32)Length : space;
        memcpy(&mock_TcpIp_SendLog[mock_TcpIp_SendLogLen], Data, copy);
        mock_TcpIp_SendLogLen += copy;
    }
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 MaxLen, uint16* ReceivedLen) {
    (void)SocketId;
    mock_TcpIp_Receive_Count++;
    if (mock_TcpIp_Receive_Fail != 0) {
        if (ReceivedLen != NULL_PTR) {
            *ReceivedLen = 0U;
        }
        return TCPIP_E_NOT_OK;
    }
    if (ReceivedLen != NULL_PTR) {
        *ReceivedLen = 0U;
    }
    if ((Buffer != NULL_PTR) && (mock_TcpIp_RxData != NULL_PTR) &&
        (mock_TcpIp_RxPos < mock_TcpIp_RxLen)) {
        uint32 remaining = mock_TcpIp_RxLen - mock_TcpIp_RxPos;
        uint32 copy = ((uint32)MaxLen <= remaining) ? (uint32)MaxLen : remaining;
        memcpy(Buffer, &mock_TcpIp_RxData[mock_TcpIp_RxPos], copy);
        mock_TcpIp_RxPos += copy;
        if (ReceivedLen != NULL_PTR) {
            *ReceivedLen = (uint16)copy;
        }
    }
    return TCPIP_OK;
}
