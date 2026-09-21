/**
 * @file stubs.c
 * @brief Minimal TcpIp stubs for the Mqtt unit test.
 *
 * The Mqtt implementation calls into the TcpIp service (SocketCreate /
 * SocketClose on connect/disconnect, Send/Receive/IsConnected from the
 * protocol backend). These stubs use the real TcpIp.h signatures so the
 * production Mqtt.c/Mqtt_Tls.c compile against them unchanged, and record
 * socket activity for the test assertions.
 */

#include "TcpIp.h"
#include <string.h>

uint32 mock_TcpIp_SocketCreate_Count = 0U;
uint32 mock_TcpIp_SocketClose_Count = 0U;
TcpIp_SocketIdType mock_TcpIp_LastSocketId = TCPIP_SOCKETID_INVALID;

Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* SocketId) {
    mock_TcpIp_SocketCreate_Count++;
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
    return TRUE;
}

TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length) {
    (void)SocketId;
    (void)Data;
    (void)Length;
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 MaxLen, uint16* ReceivedLen) {
    (void)SocketId;
    (void)Buffer;
    (void)MaxLen;
    if (ReceivedLen != NULL_PTR) {
        *ReceivedLen = 0U;
    }
    return TCPIP_OK;
}
