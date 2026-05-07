/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : TcpIp Mock Header for Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-01
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TCPIP_H
#define TCPIP_H

#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define TCPIP_VENDOR_ID                 (0x01U)
#define TCPIP_MODULE_ID                 (0x45U)

/*==================================================================================================
*                                    RETURN TYPES
==================================================================================================*/
typedef uint8 TcpIp_ReturnType;

#define TCPIP_OK                        (0x00U)
#define TCPIP_E_NOT_OK                  (0x01U)
#define TCPIP_E_PHYS_ADDR_MISS          (0x02U)
#define TCPIP_E_NOBUFS                  (0x03U)
#define TCPIP_E_TIMEDOUT                (0x04U)
#define TCPIP_E_CONNREFUSED             (0x05U)
#define TCPIP_E_ISCONN                  (0x06U)
#define TCPIP_E_ALREADY                 (0x07U)
#define TCPIP_E_INPROGRESS              (0x08U)
#define TCPIP_E_NOTCONN                 (0x09U)
#define TCPIP_E_CONNABORTED             (0x0AU)
#define TCPIP_E_CONNRESET               (0x0BU)
#define TCPIP_E_PIPE                    (0x0CU)
#define TCPIP_E_DESTUNREACH             (0x0DU)

/*==================================================================================================
*                                    SOCKET TYPES
==================================================================================================*/
typedef uint8 TcpIp_SocketIdType;
typedef uint8 TcpIp_DomainType;
typedef uint8 TcpIp_ProtocolType;
typedef uint8 TcpIp_EventType;
typedef uint8 TcpIp_IpAddrAssignmentType;
typedef uint8 TcpIp_IpAddrStateType;

#define TCPIP_SOCKETID_INVALID          (0xFFU)

#define TCPIP_AF_INET                   (0x02U)
#define TCPIP_AF_INET6                  (0x0AU)

#define TCPIP_SOCK_STREAM               (0x01U)
#define TCPIP_SOCK_DGRAM                (0x02U)

/*==================================================================================================
*                                    IP ADDRESS STATES
==================================================================================================*/
#define TCPIP_IPADDR_STATE_UNASSIGNED   (0x00U)
#define TCPIP_IPADDR_STATE_ASSIGNED     (0x01U)
#define TCPIP_IPADDR_STATE_ONHOLD       (0x02U)

/*==================================================================================================
*                                    ADDRESS STRUCTURE
==================================================================================================*/
typedef struct {
    uint8 domain;
    uint16 port;
    uint8 addr[16];
} TcpIp_SockAddrType;

/*==================================================================================================
*                                    MOCK FUNCTION PROTOTYPES
==================================================================================================*/
TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType Domain, TcpIp_ProtocolType Protocol, TcpIp_SocketIdType* SocketIdPtr);
TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddrPtr);
TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort);
TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* DataPtr, uint16 Length);

#endif /* TCPIP_H */
