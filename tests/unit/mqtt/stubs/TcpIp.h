/**
 * @file TcpIp.h
 * @brief TcpIp module stub for MQTT unit testing
 *
 * Minimal stub providing the TCP/IP socket interface needed by Mqtt.c.
 * All functions are no-ops / return success so tests can focus on MQTT
 * initialization, de-initialization, and state management without
 * requiring a real TCP/IP stack.
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef TCPIP_STUB_H
#define TCPIP_STUB_H

#include "Std_Types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* TcpIp socket type — opaque handle */
typedef uint8 TcpIp_SocketIdType;

#define TCPIP_SOCKETID_INVALID  ((TcpIp_SocketIdType)0xFFU)

static inline Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* socketId)
{
    if (socketId != NULL_PTR)
    {
        *socketId = 0U;
    }
    return E_OK;
}

static inline void TcpIp_SocketClose(TcpIp_SocketIdType socketId)
{
    (void)socketId;
}

static inline Std_ReturnType TcpIp_Send(TcpIp_SocketIdType socketId,
                                         const uint8* data,
                                         uint16 length)
{
    (void)socketId;
    (void)data;
    (void)length;
    return E_OK;
}

static inline Std_ReturnType TcpIp_Receive(TcpIp_SocketIdType socketId,
                                            uint8* buffer,
                                            uint16 bufferSize,
                                            uint16* receivedLength)
{
    (void)socketId;
    (void)buffer;
    (void)bufferSize;
    if (receivedLength != NULL_PTR)
    {
        *receivedLength = 0U;
    }
    return E_OK;
}

static inline boolean TcpIp_IsConnected(TcpIp_SocketIdType socketId)
{
    (void)socketId;
    return TRUE;
}

#endif /* TCPIP_STUB_H */
