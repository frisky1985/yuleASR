/**
 * @file TcpIp.h
 * @brief TCP/IP Stack Interface Stub
 */

#ifndef TCPIP_H
#define TCPIP_H

#include "Std_Types.h"

#define TCPIP_MODULE_ID             0x55U
#define TCPIP_VENDOR_ID             0x0055U

typedef uint8 TcpIp_SocketIdType;
typedef uint8 TcpIp_ProtocolType;

#define TCPIP_IPPROTO_TCP           0x06U
#define TCPIP_IPPROTO_UDP           0x11U

typedef struct {
    uint32 addr;
    uint16 port;
} TcpIp_SockAddrType;

typedef uint16 TcpIp_LengthType;

#endif /* TCPIP_H */