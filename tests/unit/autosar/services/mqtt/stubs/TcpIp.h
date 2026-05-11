/**
 * @file TcpIp.h
 * @brief TcpIp模块测试桩
 */

#ifndef TCPIP_H
#define TCPIP_H

#include "Std_Types.h"

typedef uint8 TcpIp_SocketIdType;
#define TCPIP_SOCKETID_INVALID 0xFFU

#define E_OK 0U
#define E_NOT_OK 1U

static inline Std_ReturnType TcpIp_Send(TcpIp_SocketIdType socketId, const uint8* data, uint16 length)
{
    (void)socketId;
    (void)data;
    (void)length;
    return E_OK;
}

static inline Std_ReturnType TcpIp_Receive(TcpIp_SocketIdType socketId, uint8* buffer, uint16 bufferSize, uint16* receivedLength)
{
    (void)socketId;
    (void)buffer;
    (void)bufferSize;
    if (receivedLength != NULL) {
        *receivedLength = 0;
    }
    return E_OK;
}

static inline boolean TcpIp_IsConnected(TcpIp_SocketIdType socketId)
{
    (void)socketId;
    return TRUE;
}

static inline Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* socketId)
{
    if (socketId != NULL) {
        *socketId = 0;
    }
    return E_OK;
}

static inline void TcpIp_SocketClose(TcpIp_SocketIdType socketId)
{
    (void)socketId;
}

#endif /* TCPIP_H */
