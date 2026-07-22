/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : lwIP, Std_Types
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file TcpIp.h
 * @brief TCP/IP Stack Interface — AUTOSAR TcpIp Adaption Layer
 * @version 1.0.0
 * @implements AUTOSAR_SWS_TcpIp.pdf
 *
 * Integrates lwIP stack with AUTOSAR BSW.  Provides socket-based
 * TCP/UDP communication APIs used by SoAd (Socket Adapter).
 */

#ifndef TCPIP_H
#define TCPIP_H

#include "Std_Types.h"
#include "TcpIp_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define TCPIP_AR_RELEASE_MAJOR_VERSION          (0x04U)
#define TCPIP_AR_RELEASE_MINOR_VERSION          (0x04U)
#define TCPIP_AR_RELEASE_REVISION_VERSION       (0x00U)
#define TCPIP_SW_MAJOR_VERSION                  (0x01U)
#define TCPIP_SW_MINOR_VERSION                  (0x00U)
#define TCPIP_SW_PATCH_VERSION                  (0x00U)

/*==================================================================================================
 *                                    MODULE / SERVICE IDs
 *==================================================================================================*/
#define TCPIP_MODULE_ID                         (0x55U)
#define TCPIP_VENDOR_ID                         (0x0001U)
#define TCPIP_INSTANCE_ID                       (0x00U)

#define TcpIP_SID_INIT                          (0x01U)
#define TcpIP_SID_DEINIT                        (0x02U)
#define TcpIP_SID_GETVERSIONINFO                (0x03U)
#define TCPIP_SID_TRANSMIT                      (0x10U)
#define TCPIP_SID_RECEIVE                       (0x11U)
#define TCPIP_SID_OPENSOCKET                    (0x12U)
#define TCPIP_SID_CLOSESOCKET                   (0x13U)
#define TCPIP_SID_GETLINKSTATE                  (0x14U)
#define TCPIP_SID_RESET                         (0x15U)
#define TCPIP_SID_GETIPV4ADDR                   (0x16U)
#define TCPIP_SID_GETIPV6ADDR                   (0x17U)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define TCPIP_E_PARAM_POINTER                   (0x01U)
#define TCPIP_E_PARAM_CONFIG                    (0x02U)
#define TCPIP_E_UNINIT                          (0x03U)
#define TCPIP_E_ALREADY_INITIALIZED             (0x04U)
#define TCPIP_E_INVALID_SOCKET                  (0x05U)
#define TCPIP_E_INVALID_PROTOCOL                (0x06U)
#define TCPIP_E_INVALID_ADDRESS                 (0x07U)
#define TCPIP_E_CONNECTION_FAILED               (0x08U)
#define TCPIP_E_SEND_FAILED                     (0x09U)
#define TCPIP_E_RECEIVE_FAILED                  (0x0AU)
#define TCPIP_E_BUFFER_OVERFLOW                 (0x0BU)
#define TCPIP_E_NOT_SUPPORTED                   (0x0CU)
#define TCPIP_E_TIMEOUT                         (0x0EU)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/* Socket ID — maps to an lwIP PCB index */
typedef uint8 TcpIp_SocketIdType;

/* Protocol type */
typedef uint8 TcpIp_ProtocolType;

#define TCPIP_IPPROTO_TCP                       (0x06U)
#define TCPIP_IPPROTO_UDP                       (0x11U)

/* Socket type (for socket() API) */
typedef uint8 TcpIp_SockTypeType;
#define TCPIP_SOCK_STREAM                       (0x01U)   /* TCP  */
#define TCPIP_SOCK_DGRAM                        (0x02U)   /* UDP  */

/* Address family */
typedef uint8 TcpIp_DomainType;
#define TCPIP_AF_INET                           (0x02U)   /* IPv4 */
#define TCPIP_AF_INET6                          (0x0AU)   /* IPv6 */

/* IP address length (bytes) */
#define TCPIP_IPV4_ADDR_LEN                     (4U)
#define TCPIP_IPV6_ADDR_LEN                     (16U)

/* Socket address structure */
typedef struct {
    TcpIp_DomainType   domain;       /* TCPIP_AF_INET or TCPIP_AF_INET6 */
    uint16             port;         /* Port number (network byte order) */
    uint8              addr[16];     /* IPv4 (4 bytes) or IPv6 (16 bytes) */
} TcpIp_SockAddrType;

/* Return type */
typedef uint8 TcpIp_ReturnType;
#define TCPIP_OK                                (0x00U)
#define TCPIP_E_NOT_OK                          (0x01U)
#define TCPIP_E_PHYS_ADDR_MISS                  (0x02U)
#define TCPIP_E_TIMEOUT_VAL                     (0x03U)

/* Invalid socket ID */
#define TCPIP_SOCKETID_INVALID                  ((TcpIp_SocketIdType)0xFFU)

/* Link state */
typedef uint8 TcpIp_LinkStateType;
#define TCPIP_LINK_STATE_DOWN                   (0x00U)
#define TCPIP_LINK_STATE_UP                     (0x01U)

/* IP address state */
typedef uint8 TcpIp_IpAddrStateType;
#define TCPIP_IPADDR_STATE_UNASSIGNED           (0x00U)
#define TCPIP_IPADDR_STATE_ASSIGNED             (0x01U)

/* IPv4 address (32-bit) */
typedef uint32 TcpIp_Ipv4AddrType;

/* IPv6 address (128-bit, 4 x uint32) */
typedef struct {
    uint32 addr[4];
} TcpIp_Ipv6AddrType;

/* IP address assignment type */
typedef uint8 TcpIp_IpAddrAssignmentType;
#define TCPIP_IPADDR_ASSIGNMENT_STATIC          (0x00U)
#define TCPIP_IPADDR_ASSIGNMENT_DHCP            (0x01U)
#define TCPIP_IPADDR_ASSIGNMENT_AUTOIP          (0x02U)

/* TCP/IP event type */
typedef uint8 TcpIp_EventType;
#define TCPIP_EVENT_CONNECTED                   (0x00U)
#define TCPIP_EVENT_DISCONNECTED                (0x01U)
#define TCPIP_EVENT_TIMEOUT                     (0x02U)
#define TCPIP_EVENT_DATA_AVAILABLE              (0x03U)
#define TCPIP_EVENT_ERROR                       (0x04U)

/* Configuration type */
typedef struct {
    uint8  NumSockets;              /* Max number of sockets */
    uint8  NumTcpPbufs;             /* Max TCP pbuf count   */
    uint16 TcpRcvBufSize;           /* TCP receive buffer   */
    uint16 TcpSndBufSize;           /* TCP send buffer      */
    uint16 UdpRcvBufSize;           /* UDP receive buffer   */
    uint8  EthLinkCheckIntervalMs;  /* Link polling ms      */
} TcpIp_ConfigType;

/*==================================================================================================
 *                                    FUNCTION DECLARATIONS
 *==================================================================================================*/

/** @brief Initialize the TCP/IP stack (lwIP init) */
void TcpIp_Init(const TcpIp_ConfigType* ConfigPtr);

/** @brief De-initialize the TCP/IP stack */
void TcpIp_DeInit(void);

/** @brief Get version information */
#if (TCPIP_VERSION_INFO_API == STD_ON)
void TcpIp_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @brief Create a socket (lwIP pcb allocation) */
TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType domain, TcpIp_SockTypeType type, TcpIp_SocketIdType* SocketId);

/** @brief Close and destroy a socket */
TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Force);

/** @brief Bind a socket to a local address */
TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* Addr);

/** @brief Send data over a connected socket */
TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length);

/** @brief Transmit data (generic, used by higher layers) */
TcpIp_ReturnType TcpIp_Transmit(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length);

/** @brief Receive data from a socket */
TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 MaxLen, uint16* ReceivedLen);

/** @brief Open a new socket of given protocol on given port (convenience) */
TcpIp_ReturnType TcpIp_OpenSocket(TcpIp_ProtocolType Protocol, uint16 Port, TcpIp_SocketIdType* SocketId);

/** @brief Close a socket by ID */
TcpIp_ReturnType TcpIp_CloseSocket(TcpIp_SocketIdType SocketId);

/** @brief Get the IPv4 address of the interface */
TcpIp_ReturnType TcpIp_GetIPv4Addr(TcpIp_Ipv4AddrType* Addr);

/** @brief Get the IPv6 address of the interface */
TcpIp_ReturnType TcpIp_GetIPv6Addr(TcpIp_Ipv6AddrType* Addr);

/** @brief Get the current link state */
TcpIp_ReturnType TcpIp_GetLinkState(TcpIp_LinkStateType* LinkState);

/** @brief Reset the TCP/IP stack */
TcpIp_ReturnType TcpIp_Reset(void);

/** @brief Main function called cyclically (polling) */
void TcpIp_MainFunction(void);

#endif /* TCPIP_H */
