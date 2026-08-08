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
 * @version 1.1.0
 * @implements AUTOSAR_SWS_TcpIp.pdf
 *
 * Integrates lwIP stack with AUTOSAR BSW.  Provides socket-based
 * TCP/UDP communication APIs used by SoAd (Socket Adapter).
 *
 * B1 deep-dive (2026-08-09): API surface expanded towards the AUTOSAR
 * SWS TcpIp interface set (TcpIp_Listen/Connect/Accept/Abort,
 * TcpIp_GetRxBuffer/ReleaseRxBuffer, TcpIp_GetTxBuffer/ReleaseTxBuffer,
 * TcpIp_SetRemoteAddr, TcpIp_ChangeTcpState, TcpIp_GetConnectionState,
 * TcpIp_GetIpAddrState, TcpIp_GetIPv4SubnetMask, TcpIp_Set/GetTcpOption,
 * TcpIp_Set/GetUdpOption, ...).
 *
 * Deviations from the SWS documented for single-interface platforms:
 *  - TcpIp_GetIPv4Addr/GetIPv6Addr/GetLinkState take no IfIdx (single Eth
 *    interface); existing SoAd/MQTT callers rely on this signature.
 *  - TcpIp_Close closes the connection AND releases the socket slot
 *    (established yuleASR behaviour, SoAd/MQTT depend on it).
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
#define TCPIP_SW_MINOR_VERSION                  (0x01U)
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
#define TCPIP_SID_LISTEN                        (0x20U)
#define TCPIP_SID_CONNECT                       (0x21U)
#define TCPIP_SID_ACCEPT                        (0x22U)
#define TCPIP_SID_ABORT                         (0x23U)
#define TCPIP_SID_SETREMOTEADDR                 (0x24U)
#define TCPIP_SID_SETLOCALADDR                  (0x25U)
#define TCPIP_SID_BINDLOCALADDR                 (0x26U)
#define TCPIP_SID_GETLOCALADDR                  (0x27U)
#define TCPIP_SID_GETREMOTEADDR                 (0x28U)
#define TCPIP_SID_GETCONNSTATE                  (0x29U)
#define TCPIP_SID_GETTCPSTATE                   (0x2AU)
#define TCPIP_SID_GETIFSTATE                    (0x2BU)
#define TCPIP_SID_GETIPADDRSTATE                (0x2CU)
#define TCPIP_SID_GETIPV4SUBNETMASK             (0x2DU)
#define TCPIP_SID_CHANGETCPSTATE                (0x2EU)
#define TCPIP_SID_SETRXBUFFER                   (0x2FU)
#define TCPIP_SID_GETRXBUFFER                   (0x30U)
#define TCPIP_SID_RELEASERXBUFFER               (0x31U)
#define TCPIP_SID_GETTXBUFFER                   (0x32U)
#define TCPIP_SID_RELEASETXBUFFER               (0x33U)
#define TCPIP_SID_SETTCPOPTION                  (0x34U)
#define TCPIP_SID_GETTCPOPTION                  (0x35U)
#define TCPIP_SID_SETUDPOPTION                  (0x36U)
#define TCPIP_SID_GETUDPOPTION                  (0x37U)
#define TCPIP_SID_RXINDICATION                  (0x38U)
#define TCPIP_SID_TXCONFIRMATION                (0x39U)
#define TCPIP_SID_SETVLANCONFIG                 (0x3AU)
#define TCPIP_SID_GETVLANCONFIG                 (0x3BU)
#define TCPIP_SID_GETSTATISTICS                 (0x3CU)
#define TCPIP_SID_RESETSTATISTICS               (0x3DU)

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
#define TCPIP_E_INVALID_STATE                   (0x0FU)
#define TCPIP_E_NOBUFS                          (0x10U)
#define TCPIP_E_TIMEDOUT                        (0x11U)
#define TCPIP_E_CONNREFUSED                     (0x12U)
#define TCPIP_E_ISCONN                          (0x13U)
#define TCPIP_E_ALREADY                         (0x14U)
#define TCPIP_E_INPROGRESS                      (0x15U)
#define TCPIP_E_NOTCONN                         (0x16U)
#define TCPIP_E_CONNABORTED                     (0x17U)
#define TCPIP_E_CONNRESET                       (0x18U)
#define TCPIP_E_DESTUNREACH                     (0x19U)

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

/* Interface state (AUTOSAR TcpIp_InterfaceStateType) */
typedef uint8 TcpIp_InterfaceStateType;
#define TCPIP_IFSTATE_DOWN                      (0x00U)
#define TCPIP_IFSTATE_UP                        (0x01U)

/* TCP connection state (AUTOSAR TcpIp_TcpStateType) */
typedef uint8 TcpIp_TcpStateType;
#define TCPIP_TCPSTATE_CLOSED                   (0x00U)
#define TCPIP_TCPSTATE_LISTEN                   (0x01U)
#define TCPIP_TCPSTATE_SYN_SENT                 (0x02U)
#define TCPIP_TCPSTATE_SYN_RECEIVED             (0x03U)
#define TCPIP_TCPSTATE_ESTABLISHED              (0x04U)
#define TCPIP_TCPSTATE_FIN_WAIT_1               (0x05U)
#define TCPIP_TCPSTATE_FIN_WAIT_2               (0x06U)
#define TCPIP_TCPSTATE_CLOSE_WAIT               (0x07U)
#define TCPIP_TCPSTATE_CLOSING                  (0x08U)
#define TCPIP_TCPSTATE_LAST_ACK                 (0x09U)
#define TCPIP_TCPSTATE_TIME_WAIT                (0x0AU)

/* Connection state (AUTOSAR TcpIp_ConnectionStateType) */
typedef uint8 TcpIp_ConnectionStateType;
#define TCPIP_CONNSTATE_CLOSED                  (0x00U)
#define TCPIP_CONNSTATE_OPEN                    (0x01U)
#define TCPIP_CONNSTATE_LISTENING               (0x02U)
#define TCPIP_CONNSTATE_CONNECTED               (0x03U)
#define TCPIP_CONNSTATE_ESTABLISHED             (0x04U)

/* TCP options (subset of AUTOSAR TcpIp_TcpOptionType) */
typedef uint8 TcpIp_TcpOptionType;
#define TCPIP_TCPOPT_REUSEADDR                  (0x01U)
#define TCPIP_TCPOPT_KEEPALIVE                  (0x02U)
#define TCPIP_TCPOPT_NODELAY                    (0x03U)
#define TCPIP_TCPOPT_MAXSEG                     (0x04U)

/* UDP options (subset of AUTOSAR TcpIp_UdpOptionType) */
typedef uint8 TcpIp_UdpOptionType;
#define TCPIP_UDPOPT_REUSEADDR                  (0x01U)
#define TCPIP_UDPOPT_TTL                        (0x02U)
#define TCPIP_UDPOPT_TOS                        (0x03U)

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

/* VLAN configuration (B1): adaption-layer VLAN settings for the
 * single Ethernet interface.  Actual on-wire tag insertion is performed
 * by lwIP (LWIP_VLAN_PCP, per-PCB TCI via netif hints) or by the
 * Ethernet Switch (EthSwt, see B2); this module validates and tracks
 * the interface VLAN membership. */
typedef struct {
    boolean VlanEnabled;       /* Tag outgoing frames with the VLAN id */
    uint16  VlanId;            /* 12-bit VID (0..4095); 0 = untagged */
    uint8   VlanPriority;      /* 802.1p PCP (0..7) */
    boolean DropUntagged;      /* Drop untagged ingress frames */
} TcpIp_VlanConfigType;

/* Statistics (B1): monotonic counters aligned with AUTOSAR
 * TcpIp_GetStatistics-class capability.  Reset via TcpIp_ResetStatistics. */
typedef struct {
    uint32 TxPackets;          /* datagrams/segments handed to the stack  */
    uint32 TxBytes;            /* payload bytes transmitted               */
    uint32 RxPackets;          /* segments/datagrams queued by RxIndication */
    uint32 RxBytes;            /* payload bytes received                  */
    uint32 TxErrors;           /* transmit failures                       */
    uint32 RxErrors;           /* receive errors (invalid socket/input)   */
    uint32 RxOverflows;        /* RX queue / chunk overflow               */
    uint32 TcpActiveOpens;     /* TCP connect() initiations               */
    uint32 TcpPassiveOpens;    /* incoming SYNs accepted on listeners     */
    uint32 TcpEstablishedCount;/* transitions into ESTABLISHED            */
    uint32 TcpCloseCount;      /* TCP connections closed (slot freed)     */
    uint32 SocketCreateCount;  /* TcpIp_Create successes                  */
    uint32 SocketCloseCount;   /* slots released (Close/Abort/pending)    */
} TcpIp_StatisticsType;

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

/** @brief Close a connection and release the socket slot.
 *
 *  yuleASR deviation: closes the connection AND frees the socket slot
 *  (SoAd/MQTT rely on this).  Force=TRUE aborts immediately, Force=FALSE
 *  initiates a graceful close (FIN_WAIT_1, driven by TcpIp_ChangeTcpState
 *  from the lwIP adapter; native simulation completes immediately).
 */
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

/** @brief Create a TCP socket (convenience wrapper used by MQTT/SoAd) */
Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* SocketId);

/** @brief Close a socket (convenience wrapper used by MQTT/SoAd) */
void TcpIp_SocketClose(TcpIp_SocketIdType SocketId);

/** @brief Check whether a socket is connected (convenience wrapper) */
boolean TcpIp_IsConnected(TcpIp_SocketIdType SocketId);

/** @brief Get the IPv4 address of the interface */
TcpIp_ReturnType TcpIp_GetIPv4Addr(TcpIp_Ipv4AddrType* Addr);

/** @brief Get the IPv6 address of the interface */
TcpIp_ReturnType TcpIp_GetIPv6Addr(TcpIp_Ipv6AddrType* Addr);

/** @brief Get the IPv4 subnet mask of the interface (AUTOSAR TcpIp_GetIPv4SubnetMask) */
TcpIp_ReturnType TcpIp_GetIPv4SubnetMask(uint8 IfIdx, TcpIp_Ipv4AddrType* Mask);

/** @brief Get the current link state */
TcpIp_ReturnType TcpIp_GetLinkState(TcpIp_LinkStateType* LinkState);

/** @brief Get the interface state (AUTOSAR TcpIp_GetInterfaceState) */
TcpIp_ReturnType TcpIp_GetInterfaceState(TcpIp_InterfaceStateType* InterfaceState);

/** @brief Get the IP address state (AUTOSAR TcpIp_GetIpAddrState) */
TcpIp_ReturnType TcpIp_GetIpAddrState(uint8 IfIdx, TcpIp_IpAddrStateType* IpAddrState);

/** @brief Reset the TCP/IP stack */
TcpIp_ReturnType TcpIp_Reset(void);

/** @brief Main function called cyclically (polling) */
void TcpIp_MainFunction(void);

/* ---- Socket-class APIs (AUTOSAR SWS TcpIp) ---- */

/** @brief Put a TCP socket into listening state (server).
 *
 *  Extension over the SWS (the SWS models listening implicitly via
 *  OpenSocket + Accept); provided explicitly to drive the connection
 *  state machine.
 */
TcpIp_ReturnType TcpIp_Listen(TcpIp_SocketIdType SocketId, uint8 Backlog);

/** @brief Initiate a connection to a remote address (AUTOSAR TcpIp_Connect).
 *
 *  Native (non-lwIP) builds simulate the handshake and reach
 *  TCPIP_TCPSTATE_ESTABLISHED synchronously.  lwIP builds use
 *  tcp_connect() and the state is driven by the adapter callbacks.
 */
TcpIp_ReturnType TcpIp_Connect(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddr);

/** @brief Accept a pending connection on a listening socket
 *         (AUTOSAR TcpIp_Accept). */
TcpIp_ReturnType TcpIp_Accept(TcpIp_SocketIdType SocketId, TcpIp_SocketIdType* NewSocketId);

/** @brief Abort a connection immediately (AUTOSAR TcpIp_Abort). */
TcpIp_ReturnType TcpIp_Abort(TcpIp_SocketIdType SocketId);

/** @brief Set the remote address of a socket (AUTOSAR TcpIp_SetRemoteAddr). */
TcpIp_ReturnType TcpIp_SetRemoteAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddr);

/** @brief Set the local address of a socket (AUTOSAR TcpIp_SetLocalAddr). */
TcpIp_ReturnType TcpIp_SetLocalAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddr);

/** @brief Bind a socket to a local address (AUTOSAR TcpIp_BindLocalAddr). */
TcpIp_ReturnType TcpIp_BindLocalAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddr);

/** @brief Get the local address of a socket. */
TcpIp_ReturnType TcpIp_GetLocalAddr(TcpIp_SocketIdType SocketId, TcpIp_SockAddrType* LocalAddr);

/** @brief Get the remote address of a socket. */
TcpIp_ReturnType TcpIp_GetRemoteAddr(TcpIp_SocketIdType SocketId, TcpIp_SockAddrType* RemoteAddr);

/** @brief Get the connection state (AUTOSAR TcpIp_GetConnectionState). */
TcpIp_ReturnType TcpIp_GetConnectionState(TcpIp_SocketIdType SocketId, TcpIp_ConnectionStateType* ConnState);

/** @brief Get the TCP protocol state of a socket (AUTOSAR TcpIp_TcpStateType). */
TcpIp_ReturnType TcpIp_GetTcpState(TcpIp_SocketIdType SocketId, TcpIp_TcpStateType* TcpState);

/** @brief Drive the TCP state machine (AUTOSAR TcpIp_ChangeTcpState).
 *
 *  Called by the lower-layer lwIP adapter on connect/accept/close
 *  callbacks; also used by tests to simulate handshake/teardown steps.
 */
TcpIp_ReturnType TcpIp_ChangeTcpState(TcpIp_SocketIdType SocketId, TcpIp_TcpStateType NewState);

/** @brief Attach a static receive buffer to a socket (zero-copy RX model,
 *         SoAd-style). */
TcpIp_ReturnType TcpIp_SetRxBuffer(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 Capacity);

/** @brief Get the oldest received chunk (AUTOSAR TcpIp_GetRxBuffer). */
TcpIp_ReturnType TcpIp_GetRxBuffer(TcpIp_SocketIdType SocketId, uint8** DataPtr, uint16* Length);

/** @brief Release the buffer returned by TcpIp_GetRxBuffer
 *         (AUTOSAR TcpIp_ReleaseRxBuffer). */
TcpIp_ReturnType TcpIp_ReleaseRxBuffer(TcpIp_SocketIdType SocketId);

/** @brief Get a transmit buffer (AUTOSAR TcpIp_GetTxBuffer). */
TcpIp_ReturnType TcpIp_GetTxBuffer(TcpIp_SocketIdType SocketId, uint8** DataPtr, uint16* Length);

/** @brief Commit a transmit buffer for sending (AUTOSAR TcpIp_ReleaseTxBuffer). */
TcpIp_ReturnType TcpIp_ReleaseTxBuffer(TcpIp_SocketIdType SocketId, uint16 Length);

/** @brief Set a TCP option (AUTOSAR TcpIp_SetTcpOption). */
TcpIp_ReturnType TcpIp_SetTcpOption(TcpIp_SocketIdType SocketId, TcpIp_TcpOptionType Option, uint32 Value);

/** @brief Get a TCP option (AUTOSAR TcpIp_GetTcpOption). */
TcpIp_ReturnType TcpIp_GetTcpOption(TcpIp_SocketIdType SocketId, TcpIp_TcpOptionType Option, uint32* Value);

/** @brief Set a UDP option (AUTOSAR TcpIp_SetUdpOption). */
TcpIp_ReturnType TcpIp_SetUdpOption(TcpIp_SocketIdType SocketId, TcpIp_UdpOptionType Option, uint32 Value);

/** @brief Get a UDP option (AUTOSAR TcpIp_GetUdpOption). */
TcpIp_ReturnType TcpIp_GetUdpOption(TcpIp_SocketIdType SocketId, TcpIp_UdpOptionType Option, uint32* Value);

/** @brief RX data ingress hook — called by the lwIP adapter / EthIf Rx path
 *         when data for a local socket arrives (mirrors the AUTOSAR
 *         TcpIp_RxIndication concept). */
TcpIp_ReturnType TcpIp_RxIndication(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length);

/** @brief TX completion hook — called by the lwIP adapter on send
 *         confirmation (AUTOSAR TcpIp_TxConfirmation concept). */
TcpIp_ReturnType TcpIp_TxConfirmation(TcpIp_SocketIdType SocketId, boolean Success);

/* ---- VLAN (B1) ---- */

/** @brief Set the interface VLAN configuration. */
#if (TCPIP_VLAN_SUPPORT == STD_ON)
TcpIp_ReturnType TcpIp_SetVlanConfig(const TcpIp_VlanConfigType* VlanConfigPtr);

/** @brief Get the interface VLAN configuration. */
TcpIp_ReturnType TcpIp_GetVlanConfig(TcpIp_VlanConfigType* VlanConfigPtr);
#endif

/* ---- Statistics (B1) ---- */

/** @brief Get the module statistics counters. */
#if (TCPIP_ENABLE_STATISTICS == STD_ON)
TcpIp_ReturnType TcpIp_GetStatistics(TcpIp_StatisticsType* StatisticsPtr);

/** @brief Reset the module statistics counters to zero. */
TcpIp_ReturnType TcpIp_ResetStatistics(void);
#endif

#endif /* TCPIP_H */
