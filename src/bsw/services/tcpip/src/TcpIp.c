/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : lwIP, Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file TcpIp.c
 * @brief TCP/IP Stack — lwIP Integration + AUTOSAR TcpIp Adaption Layer
 * @req SHALL_TCPIP_ADAPTION - AUTOSAR TcpIp adaptation over lwIP
 *
 * Wraps lwIP raw/sequential API into the AUTOSAR TcpIp API surface.
 * Provides socket management, send/receive, address lookup, link state,
 * connection state machine, zero-copy RX/TX buffers and options.
 *
 * B1 deep-dive (2026-08-09):
 *  - API surface expanded to the AUTOSAR SWS TcpIp interface set
 *    (Listen/Connect/Accept/Abort, Get/ReleaseRxBuffer, Get/ReleaseTxBuffer,
 *     SetRemoteAddr, ChangeTcpState, GetConnectionState, GetIpAddrState,
 *     GetIPv4SubnetMask, Set/GetTcpOption, Set/GetUdpOption, RxIndication,
 *     TxConfirmation).
 *  - Connection state machine (TCPIP_TCPSTATE_* / TCPIP_CONNSTATE_*).
 *  - Native (non-lwIP) builds simulate the transport; the lwIP guarded
 *    path drives real PCBs (tcp_listen_with_backlog, tcp_connect, tcp_abort,
 *    tcp_write/udp_send, ...).
 */

#include "TcpIp.h"
#include "Det.h"
#include <string.h>

/* lwIP headers — only when the platform build links lwIP
 * (TCPIP_ENABLE_LWIP is a platform build macro, see TcpIp_Cfg.h). */
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip6_addr.h"
#include "lwip/timeouts.h"
#endif

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define TCPIP_STATE_UNINIT                      (0x00U)
#define TCPIP_STATE_INIT                        (0x01U)

#define TCPIP_IF_IDX_SINGLE                     (0x00U)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    #define TCPIP_DET_REPORT_ERROR(api, err) \
        Det_ReportError(TCPIP_MODULE_ID, TCPIP_INSTANCE_ID, (api), (err))
#else
    #define TCPIP_DET_REPORT_ERROR(api, err)
#endif

#define TCPIP_IS_VALID_SOCKET(id) \
    (((id) < TCPIP_MAX_SOCKETS) ? TRUE : FALSE)

#define TCPIP_IS_INIT() \
    (TcpIp_InternalState.State == TCPIP_STATE_INIT)

#if (TCPIP_ENABLE_STATISTICS == STD_ON)
    #define TCPIP_STATS_INC(field) \
        do { TcpIp_InternalState.Statistics.field = (uint32)(TcpIp_InternalState.Statistics.field + 1U); } while (0)
    #define TCPIP_STATS_ADD(field, value) \
        do { TcpIp_InternalState.Statistics.field = (uint32)(TcpIp_InternalState.Statistics.field + (uint32)(value)); } while (0)
#else
    #define TCPIP_STATS_INC(field)
    #define TCPIP_STATS_ADD(field, value)
#endif

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/* Socket entry tracking an lwIP PCB + connection state */
typedef struct {
    boolean             InUse;
    TcpIp_SockTypeType  SockType;   /* TCPIP_SOCK_STREAM or TCPIP_SOCK_DGRAM */
    TcpIp_DomainType    Domain;     /* TCPIP_AF_INET or TCPIP_AF_INET6       */
    uint16              LocalPort;
    uint8               LocalAddr[16];
    uint16              RemotePort;
    uint8               RemoteAddr[16];
    boolean             IsConnected;
    void*               Pcb;        /* Opaque pointer to lwIP pcb (tcp_pcb/udp_pcb) */

    /* B1: connection state machine */
    TcpIp_TcpStateType        TcpState;
    TcpIp_ConnectionStateType ConnState;
    uint8                     Backlog;
    boolean                   CloseInProgress;
    uint8                     PendingCount;
    TcpIp_SocketIdType        PendingSockets[TCPIP_MAX_PENDING_CONNECTIONS];

    /* B1: receive buffers (user-attached preferred + pool-backed ring) */
    uint8*                    RxUserBuf;
    uint16                    RxUserCapacity;
    uint16                    RxUserLen;
    boolean                   RxUserPending;
    uint8                     RxPool[TCPIP_MAX_RX_BUFFERS][TCPIP_PBUF_POOL_BUF_SIZE];
    uint16                    RxPoolLen[TCPIP_MAX_RX_BUFFERS];
    uint8                     RxHead;
    uint8                     RxTail;
    uint8                     RxCount;
    boolean                   RxChunkInUse;   /* head chunk held by GetRxBuffer */

    /* B1: transmit buffer (internal pool chunk, zero-copy TX model) */
    uint8                     TxBuf[TCPIP_PBUF_POOL_BUF_SIZE];
    uint16                    TxLen;

    /* B1: socket options */
    boolean                   TcpReuseAddr;
    boolean                   TcpKeepAlive;
    boolean                   TcpNoDelay;
    uint16                    TcpMaxSeg;
    uint8                     UdpTtl;
    uint8                     UdpTos;
} TcpIp_SocketEntryType;

/* Internal module state */
typedef struct {
    uint8              State;
    const TcpIp_ConfigType* ConfigPtr;
    TcpIp_SocketEntryType Sockets[TCPIP_MAX_SOCKETS];
    TcpIp_LinkStateType   LinkState;
    TcpIp_InterfaceStateType InterfaceState;
    TcpIp_IpAddrStateType   IpAddrState;
    uint32                Ipv4Addr;
    uint32                Ipv4Mask;
    uint32                Ipv4Gateway;
#if (TCPIP_VLAN_SUPPORT == STD_ON)
    TcpIp_VlanConfigType  VlanConfig;
#endif
#if (TCPIP_ENABLE_STATISTICS == STD_ON)
    TcpIp_StatisticsType  Statistics;
#endif
} TcpIp_InternalStateType;

/*==================================================================================================
 *                                    LOCAL DATA
 *==================================================================================================*/
static TcpIp_InternalStateType TcpIp_InternalState;

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static TcpIp_SocketEntryType* TcpIp_LocalFindSocket(TcpIp_SocketIdType SocketId);
static TcpIp_SocketIdType TcpIp_LocalAllocSlot(void);
static void TcpIp_LocalFreeSlot(TcpIp_SocketIdType SocketId);
static void TcpIp_LocalUpdateConnState(TcpIp_SocketEntryType* entry);
static boolean TcpIp_LocalIsValidTransition(TcpIp_TcpStateType From, TcpIp_TcpStateType To);
static void TcpIp_LocalClearPending(TcpIp_SocketEntryType* entry);
static TcpIp_ReturnType TcpIp_LocalAbortConnection(TcpIp_SocketEntryType* entry);
static TcpIp_ReturnType TcpIp_LocalCommitTx(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length);
static uint8* TcpIp_LocalRxPeekData(TcpIp_SocketEntryType* entry);
static uint16 TcpIp_LocalRxPeekLen(TcpIp_SocketEntryType* entry);
static void TcpIp_LocalRxDropHead(TcpIp_SocketEntryType* entry);
static void TcpIp_LocalRxFlush(TcpIp_SocketEntryType* entry);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
static err_t TcpIp_LwipConnected(void* Arg, struct tcp_pcb* Pcb, err_t Err);
static void TcpIp_LocalToIpAddr(const TcpIp_SockAddrType* Addr, ip_addr_t* Ip);
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Find a socket entry by socket ID.
 */
static TcpIp_SocketEntryType* TcpIp_LocalFindSocket(TcpIp_SocketIdType SocketId)
{
    if (!TCPIP_IS_VALID_SOCKET(SocketId))
    {
        return NULL_PTR;
    }
    if (!TcpIp_InternalState.Sockets[SocketId].InUse)
    {
        return NULL_PTR;
    }
    return &TcpIp_InternalState.Sockets[SocketId];
}

/**
 * @brief Allocate a free socket slot.
 * @return SocketId on success, TCPIP_SOCKETID_INVALID on failure.
 */
static TcpIp_SocketIdType TcpIp_LocalAllocSlot(void)
{
    TcpIp_SocketIdType i;
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (!TcpIp_InternalState.Sockets[i].InUse)
        {
            (void)memset(&TcpIp_InternalState.Sockets[i], 0, sizeof(TcpIp_SocketEntryType));
            TcpIp_InternalState.Sockets[i].InUse = TRUE;
            return i;
        }
    }
    return TCPIP_SOCKETID_INVALID;
}

/**
 * @brief Free a socket slot (state must have been torn down).
 */
static void TcpIp_LocalFreeSlot(TcpIp_SocketIdType SocketId)
{
    if (TCPIP_IS_VALID_SOCKET(SocketId))
    {
        TcpIp_SocketEntryType* entry = &TcpIp_InternalState.Sockets[SocketId];
        if (entry->InUse)
        {
            if (entry->SockType == TCPIP_SOCK_STREAM)
            {
                TCPIP_STATS_INC(TcpCloseCount);
            }
            TCPIP_STATS_INC(SocketCloseCount);
        }
        (void)memset(entry, 0, sizeof(TcpIp_SocketEntryType));
    }
}

/**
 * @brief Derive the connection state from the TCP protocol state.
 */
static void TcpIp_LocalUpdateConnState(TcpIp_SocketEntryType* entry)
{
    switch (entry->TcpState)
    {
        case TCPIP_TCPSTATE_LISTEN:
            entry->ConnState = TCPIP_CONNSTATE_LISTENING;
            entry->IsConnected = FALSE;
            break;
        case TCPIP_TCPSTATE_SYN_SENT:
        case TCPIP_TCPSTATE_SYN_RECEIVED:
            entry->ConnState = TCPIP_CONNSTATE_CONNECTED;
            entry->IsConnected = FALSE;
            break;
        case TCPIP_TCPSTATE_ESTABLISHED:
        case TCPIP_TCPSTATE_CLOSE_WAIT:   /* peer closed, local side may still send */
            entry->ConnState = TCPIP_CONNSTATE_ESTABLISHED;
            entry->IsConnected = TRUE;
            break;
        case TCPIP_TCPSTATE_FIN_WAIT_1:
        case TCPIP_TCPSTATE_FIN_WAIT_2:
        case TCPIP_TCPSTATE_CLOSING:
        case TCPIP_TCPSTATE_LAST_ACK:
        case TCPIP_TCPSTATE_TIME_WAIT:
            entry->ConnState = TCPIP_CONNSTATE_CONNECTED;
            entry->IsConnected = FALSE;
            break;
        case TCPIP_TCPSTATE_CLOSED:
        default:
            entry->ConnState = TCPIP_CONNSTATE_CLOSED;
            entry->IsConnected = FALSE;
            break;
    }
}

/**
 * @brief Validate a TCP state transition (RFC 793-ish subset).
 */
static boolean TcpIp_LocalIsValidTransition(TcpIp_TcpStateType From, TcpIp_TcpStateType To)
{
    boolean valid = FALSE;

    if (From == To)
    {
        valid = TRUE;   /* idempotent */
    }
    else
    {
        switch (From)
        {
            case TCPIP_TCPSTATE_CLOSED:
                valid = ((To == TCPIP_TCPSTATE_LISTEN) ||
                         (To == TCPIP_TCPSTATE_SYN_SENT)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_LISTEN:
                valid = ((To == TCPIP_TCPSTATE_SYN_RECEIVED) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_SYN_SENT:
                valid = ((To == TCPIP_TCPSTATE_ESTABLISHED) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_SYN_RECEIVED:
                valid = ((To == TCPIP_TCPSTATE_ESTABLISHED) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_ESTABLISHED:
                valid = ((To == TCPIP_TCPSTATE_FIN_WAIT_1) ||
                         (To == TCPIP_TCPSTATE_CLOSE_WAIT) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_FIN_WAIT_1:
                valid = ((To == TCPIP_TCPSTATE_FIN_WAIT_2) ||
                         (To == TCPIP_TCPSTATE_CLOSING) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_FIN_WAIT_2:
                valid = ((To == TCPIP_TCPSTATE_TIME_WAIT) ||
                         (To == TCPIP_TCPSTATE_CLOSED)) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_CLOSE_WAIT:
                valid = (To == TCPIP_TCPSTATE_LAST_ACK) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_CLOSING:
                valid = (To == TCPIP_TCPSTATE_TIME_WAIT) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_LAST_ACK:
                valid = (To == TCPIP_TCPSTATE_CLOSED) ? TRUE : FALSE;
                break;
            case TCPIP_TCPSTATE_TIME_WAIT:
                valid = (To == TCPIP_TCPSTATE_CLOSED) ? TRUE : FALSE;
                break;
            default:
                valid = FALSE;
                break;
        }
    }
    return valid;
}

/**
 * @brief Drop all pending connections of a listener (children are freed).
 */
static void TcpIp_LocalClearPending(TcpIp_SocketEntryType* entry)
{
    uint8 i;
    for (i = 0U; i < entry->PendingCount; i++)
    {
        if (TCPIP_IS_VALID_SOCKET(entry->PendingSockets[i]))
        {
            TcpIp_LocalFreeSlot(entry->PendingSockets[i]);
        }
    }
    entry->PendingCount = 0U;
}

/**
 * @brief Tear down the lwIP PCB of an entry (compile-time conditional).
 */
static void TcpIp_LocalFreePcb(TcpIp_SocketEntryType* entry)
{
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (entry->Pcb != NULL_PTR)
    {
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            (void)tcp_close((struct tcp_pcb*)entry->Pcb);
        }
        else
        {
            udp_remove((struct udp_pcb*)entry->Pcb);
        }
        entry->Pcb = NULL_PTR;
    }
#else
    (void)entry;
#endif
}

/**
 * @brief Abort a connection immediately (local side; slot not freed).
 */
static TcpIp_ReturnType TcpIp_LocalAbortConnection(TcpIp_SocketEntryType* entry)
{
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (entry->Pcb != NULL_PTR)
    {
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            tcp_abort((struct tcp_pcb*)entry->Pcb);
        }
        else
        {
            udp_remove((struct udp_pcb*)entry->Pcb);
        }
        entry->Pcb = NULL_PTR;
    }
#endif
    TcpIp_LocalClearPending(entry);
    entry->TcpState = TCPIP_TCPSTATE_CLOSED;
    entry->CloseInProgress = FALSE;
    TcpIp_LocalRxFlush(entry);
    TcpIp_LocalUpdateConnState(entry);
    return TCPIP_OK;
}

/**
 * @brief Commit TX data: lwIP real send when available, otherwise a
 *        native simulation (statistics-only sink).
 */
static TcpIp_ReturnType TcpIp_LocalCommitTx(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length)
{
    TcpIp_SocketEntryType* entry;
    TcpIp_ReturnType result = TCPIP_OK;

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        TCPIP_STATS_INC(TxErrors);
        return TCPIP_E_NOT_OK;
    }

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    {
        err_t err;
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            err = tcp_write((struct tcp_pcb*)entry->Pcb, Data, Length, TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK)
            {
                TCPIP_STATS_INC(TxErrors);
                result = TCPIP_E_NOT_OK;
            }
        }
        else
        {
            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, Length, PBUF_POOL);
            if (p == NULL_PTR)
            {
                TCPIP_STATS_INC(TxErrors);
                result = TCPIP_E_NOBUFS;
            }
            else
            {
                (void)memcpy(p->payload, Data, Length);
                err = udp_send((struct udp_pcb*)entry->Pcb, p);
                pbuf_free(p);
                if (err != ERR_OK)
                {
                    TCPIP_STATS_INC(TxErrors);
                    result = TCPIP_E_NOT_OK;
                }
            }
        }
    }
#else
    /* Native simulation: data is accepted into the TX pool (already copied
     * by the caller when using the GetTxBuffer model).  No hardware effect. */
    (void)SocketId;
    (void)Data;
    (void)Length;
#endif

    if (result == TCPIP_OK)
    {
        TCPIP_STATS_INC(TxPackets);
        TCPIP_STATS_ADD(TxBytes, Length);
    }

    return result;
}

/**
 * @brief Peek the oldest received chunk (user buffer first, then pool).
 */
static uint8* TcpIp_LocalRxPeekData(TcpIp_SocketEntryType* entry)
{
    uint8* result = NULL_PTR;
    if (entry->RxUserPending)
    {
        result = entry->RxUserBuf;
    }
    else if (entry->RxCount > 0U)
    {
        result = &entry->RxPool[entry->RxHead][0];
    }
    return result;
}

/**
 * @brief Peek the length of the oldest received chunk.
 */
static uint16 TcpIp_LocalRxPeekLen(TcpIp_SocketEntryType* entry)
{
    uint16 result = 0U;
    if (entry->RxUserPending)
    {
        result = entry->RxUserLen;
    }
    else if (entry->RxCount > 0U)
    {
        result = entry->RxPoolLen[entry->RxHead];
    }
    return result;
}

/**
 * @brief Drop the oldest received chunk (after consume/release).
 */
static void TcpIp_LocalRxDropHead(TcpIp_SocketEntryType* entry)
{
    if (entry->RxUserPending)
    {
        entry->RxUserPending = FALSE;
        entry->RxUserLen = 0U;
    }
    else if (entry->RxCount > 0U)
    {
        entry->RxHead = (uint8)((entry->RxHead + 1U) % TCPIP_MAX_RX_BUFFERS);
        entry->RxCount--;
    }
    entry->RxChunkInUse = FALSE;
}

/**
 * @brief Flush all queued RX data of a socket.
 */
static void TcpIp_LocalRxFlush(TcpIp_SocketEntryType* entry)
{
    entry->RxUserPending = FALSE;
    entry->RxUserLen = 0U;
    entry->RxHead = 0U;
    entry->RxTail = 0U;
    entry->RxCount = 0U;
    entry->RxChunkInUse = FALSE;
}

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)

/**
 * @brief Convert an AUTOSAR socket address into an lwIP ip_addr_t.
 */
static void TcpIp_LocalToIpAddr(const TcpIp_SockAddrType* Addr, ip_addr_t* Ip)
{
    if (Addr->domain == TCPIP_AF_INET6)
    {
#if LWIP_IPV6
        u32_t words[4];
        uint8 i;
        for (i = 0U; i < 4U; i++)
        {
            words[i] = (((u32_t)Addr->addr[(uint8)(i * 4U)]) << 24) |
                       (((u32_t)Addr->addr[(uint8)((i * 4U) + 1U)]) << 16) |
                       (((u32_t)Addr->addr[(uint8)((i * 4U) + 2U)]) << 8) |
                       ((u32_t)Addr->addr[(uint8)((i * 4U) + 3U)]);
        }
        IP6_ADDR(Ip, words[0], words[1], words[2], words[3]);
#else
        /* lwIP build without IPv6: address cannot be represented. */
        ip_addr_set_zero(Ip);
#endif
    }
    else
    {
        IP4_ADDR(Ip, Addr->addr[0], Addr->addr[1], Addr->addr[2], Addr->addr[3]);
    }
}

/**
 * @brief lwIP tcp_connect() completion callback — drives the adaption
 *        layer state machine (SYN_SENT -> ESTABLISHED / CLOSED).
 */
static err_t TcpIp_LwipConnected(void* Arg, struct tcp_pcb* Pcb, err_t Err)
{
    TcpIp_SocketIdType socketId = (TcpIp_SocketIdType)(uintptr_t)Arg;
    (void)Pcb;
    if (Err == ERR_OK)
    {
        (void)TcpIp_ChangeTcpState(socketId, TCPIP_TCPSTATE_ESTABLISHED);
    }
    else
    {
        (void)TcpIp_ChangeTcpState(socketId, TCPIP_TCPSTATE_CLOSED);
    }
    return ERR_OK;
}

#endif /* TCPIP_ENABLE_LWIP */

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialize the TCP/IP stack.
 *
 * Initialises lwIP (if TCPIP_ENABLE_LWIP is defined from project config)
 * and the internal socket table.
 */
void TcpIp_Init(const TcpIp_ConfigType* ConfigPtr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_InternalState.State == TCPIP_STATE_INIT)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_PARAM_POINTER);
        return;
    }
#endif

    TcpIp_InternalState.ConfigPtr = ConfigPtr;
    TcpIp_InternalState.State     = TCPIP_STATE_INIT;
    TcpIp_InternalState.LinkState = TCPIP_LINK_STATE_UP;
    TcpIp_InternalState.InterfaceState = TCPIP_IFSTATE_UP;
    TcpIp_InternalState.IpAddrState = TCPIP_IPADDR_STATE_ASSIGNED;
    TcpIp_InternalState.Ipv4Addr  = TCPIP_DEFAULT_IPV4_ADDR;
    TcpIp_InternalState.Ipv4Mask  = TCPIP_DEFAULT_IPV4_MASK;
    TcpIp_InternalState.Ipv4Gateway = TCPIP_DEFAULT_IPV4_GW;

#if (TCPIP_VLAN_SUPPORT == STD_ON)
    TcpIp_InternalState.VlanConfig.VlanEnabled = FALSE;
    TcpIp_InternalState.VlanConfig.VlanId = (uint16)TCPIP_DEFAULT_VLAN_ID;
    TcpIp_InternalState.VlanConfig.VlanPriority = (uint8)TCPIP_DEFAULT_VLAN_PRIORITY;
    TcpIp_InternalState.VlanConfig.DropUntagged = FALSE;
#endif

    /* Clear socket table */
    (void)memset(TcpIp_InternalState.Sockets, 0, sizeof(TcpIp_InternalState.Sockets));

    /* lwIP initialisation — when lwIP is available the linker resolves
     * these symbols from the lwIP library.  For standalone / mock builds
     * we leave them as no-ops.
     */
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    lwip_init();
#endif
}

/**
 * @brief De-initialize the TCP/IP stack.
 */
void TcpIp_DeInit(void)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_InternalState.State != TCPIP_STATE_INIT)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_DEINIT, TCPIP_E_UNINIT);
        return;
    }
#endif

    /* Close all sockets */
    {
        TcpIp_SocketIdType i;
        for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
        {
            if ((TcpIp_InternalState.Sockets[i].InUse) != 0U)
            {
                (void)TcpIp_Close(i, TRUE);
            }
        }
    }

    TcpIp_InternalState.State     = TCPIP_STATE_UNINIT;
    TcpIp_InternalState.ConfigPtr = NULL_PTR;
    TcpIp_InternalState.LinkState = TCPIP_LINK_STATE_DOWN;
    TcpIp_InternalState.InterfaceState = TCPIP_IFSTATE_DOWN;
}

/**
 * @brief Get version information.
 */
#if (TCPIP_VERSION_INFO_API == STD_ON)
void TcpIp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_GETVERSIONINFO, TCPIP_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID         = TCPIP_VENDOR_ID;
    versioninfo->moduleID         = TCPIP_MODULE_ID;
    versioninfo->sw_major_version = TCPIP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = TCPIP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = TCPIP_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Create a socket.
 *
 * On lwIP-enabled builds this allocates a tcp_pcb or udp_pcb.
 * When lwIP is absent it allocates a slot in the internal table only.
 */
TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType domain, TcpIp_SockTypeType type, TcpIp_SocketIdType* SocketId)
{
    TcpIp_SocketIdType slot;
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_OPENSOCKET, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (SocketId == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_OPENSOCKET, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    slot = TcpIp_LocalAllocSlot();
    if (slot == TCPIP_SOCKETID_INVALID)
    {
        return TCPIP_E_NOT_OK;
    }

    entry = &TcpIp_InternalState.Sockets[slot];
    entry->SockType = type;
    entry->Domain   = domain;
    entry->IsConnected = FALSE;
    entry->TcpState = TCPIP_TCPSTATE_CLOSED;
    entry->ConnState = TCPIP_CONNSTATE_CLOSED;
    entry->UdpTtl = 0U;

    /* lwIP PCB allocation — compile-time conditional */
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (type == TCPIP_SOCK_STREAM)
    {
        struct tcp_pcb* pcb = tcp_new();
        if (pcb == NULL_PTR)
        {
            entry->InUse = FALSE;
            return TCPIP_E_NOT_OK;
        }
        entry->Pcb = (void*)pcb;
    }
    else
    {
        struct udp_pcb* pcb = udp_new();
        if (pcb == NULL_PTR)
        {
            entry->InUse = FALSE;
            return TCPIP_E_NOT_OK;
        }
        entry->Pcb = (void*)pcb;
    }
#else
    (void)domain;
    (void)type;
    entry->Pcb = NULL_PTR;
#endif

    *SocketId = slot;
    TCPIP_STATS_INC(SocketCreateCount);
    return TCPIP_OK;
}

/**
 * @brief Close a connection and release the socket slot.
 *
 * Force=TRUE aborts immediately; Force=FALSE initiates a graceful close
 * (FIN_WAIT_1; native builds advance it in TcpIp_MainFunction, lwIP builds
 * hand the PCB to tcp_close()).
 */
TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Force)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_CLOSESOCKET, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    if (Force == TRUE)
    {
        (void)TcpIp_LocalAbortConnection(entry);
        TcpIp_LocalFreePcb(entry);
        TcpIp_LocalFreeSlot(SocketId);
        return TCPIP_OK;
    }

    /* Graceful close */
    if (entry->CloseInProgress)
    {
        return TCPIP_OK;   /* already closing */
    }
    if (entry->TcpState == TCPIP_TCPSTATE_ESTABLISHED)
    {
        entry->TcpState = TCPIP_TCPSTATE_FIN_WAIT_1;
        entry->CloseInProgress = TRUE;
        TcpIp_LocalUpdateConnState(entry);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
        /* Hand the PCB to lwIP: tcp_close() manages the FIN sequence and
         * frees the PCB when the handshake completes. */
        if (entry->Pcb != NULL_PTR)
        {
            (void)tcp_close((struct tcp_pcb*)entry->Pcb);
            entry->Pcb = NULL_PTR;
        }
#endif
        return TCPIP_OK;
    }

    /* Not connected: free the slot directly. */
    TcpIp_LocalClearPending(entry);
    TcpIp_LocalFreePcb(entry);
    TcpIp_LocalFreeSlot(SocketId);
    return TCPIP_OK;
}

/**
 * @brief Bind a socket to a local address / port.
 */
TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* Addr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_BINDLOCALADDR, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Addr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_BINDLOCALADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    entry->LocalPort = Addr->port;
    (void)memcpy(entry->LocalAddr, Addr->addr, TCPIP_IPV6_ADDR_LEN);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    {
        ip_addr_t ip;
        TcpIp_LocalToIpAddr(Addr, &ip);
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            if (tcp_bind((struct tcp_pcb*)entry->Pcb, &ip, Addr->port) != ERR_OK)
            {
                return TCPIP_E_NOT_OK;
            }
        }
        else
        {
            if (udp_bind((struct udp_pcb*)entry->Pcb, &ip, Addr->port) != ERR_OK)
            {
                return TCPIP_E_NOT_OK;
            }
        }
    }
#else
    (void)Addr;
#endif

    return TCPIP_OK;
}

/**
 * @brief Send data on a connected socket.
 */
TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_TRANSMIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Data == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_TRANSMIT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    return TcpIp_LocalCommitTx(SocketId, Data, Length);
}

/**
 * @brief Transmit — alias for TcpIp_Send for higher-layer compatibility.
 */
TcpIp_ReturnType TcpIp_Transmit(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length)
{
    return TcpIp_Send(SocketId, Data, Length);
}

/**
 * @brief Receive data from a socket (drains the queued RX chunk).
 */
TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 MaxLen, uint16* ReceivedLen)
{
    TcpIp_SocketEntryType* entry;
    uint16 chunkLen;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RECEIVE, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if ((Buffer == NULL_PTR) || (ReceivedLen == NULL_PTR))
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RECEIVE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    *ReceivedLen = 0U;

    if ((!entry->RxUserPending) && (entry->RxCount == 0U))
    {
        return TCPIP_OK;   /* no data */
    }

    chunkLen = TcpIp_LocalRxPeekLen(entry);
    if (MaxLen < chunkLen)
    {
        return TCPIP_E_BUFFER_OVERFLOW;   /* data not consumed */
    }

    (void)memcpy(Buffer, TcpIp_LocalRxPeekData(entry), chunkLen);
    *ReceivedLen = chunkLen;
    TcpIp_LocalRxDropHead(entry);
    return TCPIP_OK;
}

/**
 * @brief Open a socket of a given protocol on a given port.
 *
 * Convenience wrapper: Create + Bind in one call.
 */
TcpIp_ReturnType TcpIp_OpenSocket(TcpIp_ProtocolType Protocol, uint16 Port, TcpIp_SocketIdType* SocketId)
{
    TcpIp_ReturnType result;
    TcpIp_SockTypeType sockType;
    TcpIp_SockAddrType addr;

    if (Protocol == TCPIP_IPPROTO_TCP)
    {
        sockType = TCPIP_SOCK_STREAM;
    }
    else if (Protocol == TCPIP_IPPROTO_UDP)
    {
        sockType = TCPIP_SOCK_DGRAM;
    }
    else
    {
        return TCPIP_E_NOT_OK;
    }

    result = TcpIp_Create(TCPIP_AF_INET, sockType, SocketId);
    if (result != TCPIP_OK)
    {
        return result;
    }

    (void)memset(&addr, 0, sizeof(addr));
    addr.domain = TCPIP_AF_INET;
    addr.port   = Port;

    result = TcpIp_Bind(*SocketId, &addr);
    if (result != TCPIP_OK)
    {
        (void)TcpIp_Close(*SocketId, TRUE);
        return result;
    }

    return TCPIP_OK;
}

/**
 * @brief Close a socket by ID.
 */
TcpIp_ReturnType TcpIp_CloseSocket(TcpIp_SocketIdType SocketId)
{
    return TcpIp_Close(SocketId, TRUE);
}

/**
 * @brief Create a TCP socket (convenience wrapper).
 */
Std_ReturnType TcpIp_SocketCreate(TcpIp_SocketIdType* SocketId)
{
    TcpIp_ReturnType result;

    if (SocketId == NULL_PTR)
    {
        return E_NOT_OK;
    }

    result = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, SocketId);
    return (result == TCPIP_OK) ? E_OK : E_NOT_OK;
}

/**
 * @brief Close a socket (convenience wrapper).
 */
void TcpIp_SocketClose(TcpIp_SocketIdType SocketId)
{
    (void)TcpIp_CloseSocket(SocketId);
}

/**
 * @brief Check whether a socket is connected (convenience wrapper).
 */
boolean TcpIp_IsConnected(TcpIp_SocketIdType SocketId)
{
    TcpIp_SocketEntryType* entry;

    if (!TCPIP_IS_VALID_SOCKET(SocketId))
    {
        return FALSE;
    }

    entry = TcpIp_LocalFindSocket(SocketId);
    if ((entry != NULL_PTR) && (entry->IsConnected))
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Get the IPv4 address.
 */
TcpIp_ReturnType TcpIp_GetIPv4Addr(TcpIp_Ipv4AddrType* Addr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (Addr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETIPV4ADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    *Addr = TcpIp_InternalState.Ipv4Addr;
    return TCPIP_OK;
}

/**
 * @brief Get the IPv6 address (stub — fe80::1 unless DHCPv6 is enabled).
 */
TcpIp_ReturnType TcpIp_GetIPv6Addr(TcpIp_Ipv6AddrType* Addr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (Addr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETIPV6ADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

#if defined(TCPIP_ENABLE_IPV6) && (TCPIP_ENABLE_IPV6 == STD_ON)
    Addr->addr[0] = 0xFE800000UL;
    Addr->addr[1] = 0x00000000UL;
    Addr->addr[2] = 0x00000000UL;
    Addr->addr[3] = 0x00000001UL;
    return TCPIP_OK;
#else
    (void)Addr;
    return TCPIP_E_NOT_SUPPORTED;
#endif
}

/**
 * @brief Get the IPv4 subnet mask of an interface.
 */
TcpIp_ReturnType TcpIp_GetIPv4SubnetMask(uint8 IfIdx, TcpIp_Ipv4AddrType* Mask)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (Mask == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETIPV4SUBNETMASK, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    if (IfIdx != TCPIP_IF_IDX_SINGLE)
    {
        return TCPIP_E_NOT_OK;
    }

    *Mask = TcpIp_InternalState.Ipv4Mask;
    return TCPIP_OK;
}

/**
 * @brief Get the current link state.
 */
TcpIp_ReturnType TcpIp_GetLinkState(TcpIp_LinkStateType* LinkState)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (LinkState == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETLINKSTATE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    *LinkState = TcpIp_InternalState.LinkState;
    return TCPIP_OK;
}

/**
 * @brief Get the interface state.
 */
TcpIp_ReturnType TcpIp_GetInterfaceState(TcpIp_InterfaceStateType* InterfaceState)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (InterfaceState == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETIFSTATE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    *InterfaceState = TcpIp_InternalState.InterfaceState;
    return TCPIP_OK;
}

/**
 * @brief Get the IP address state of an interface.
 */
TcpIp_ReturnType TcpIp_GetIpAddrState(uint8 IfIdx, TcpIp_IpAddrStateType* IpAddrState)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (IpAddrState == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETIPADDRSTATE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    if (IfIdx != TCPIP_IF_IDX_SINGLE)
    {
        return TCPIP_E_NOT_OK;
    }

    *IpAddrState = TcpIp_InternalState.IpAddrState;
    return TCPIP_OK;
}

/**
 * @brief Reset the TCP/IP stack (tear down and re-init).
 */
TcpIp_ReturnType TcpIp_Reset(void)
{
    const TcpIp_ConfigType* savedConfig;

    if (TcpIp_InternalState.State != TCPIP_STATE_INIT)
    {
        return TCPIP_E_NOT_OK;
    }

    savedConfig = TcpIp_InternalState.ConfigPtr;

    TcpIp_DeInit();

    if (savedConfig != NULL_PTR)
    {
        TcpIp_Init(savedConfig);
    }
    else
    {
        TcpIp_ConfigType defaultConfig;
        (void)memset(&defaultConfig, 0, sizeof(defaultConfig));
        defaultConfig.NumSockets           = TCPIP_MAX_SOCKETS;
        defaultConfig.NumTcpPbufs          = TCPIP_MAX_TCP_PBUFS;
        defaultConfig.TcpRcvBufSize        = TCPIP_TCP_RCV_BUF_SIZE;
        defaultConfig.TcpSndBufSize        = TCPIP_TCP_SND_BUF_SIZE;
        defaultConfig.UdpRcvBufSize        = TCPIP_UDP_RCV_BUF_SIZE;
        defaultConfig.EthLinkCheckIntervalMs = TCPIP_ETH_LINK_CHECK_INTERVAL_MS;
        TcpIp_Init(&defaultConfig);
    }

    return TCPIP_OK;
}

/**
 * @brief Main function — called periodically to poll link state and
 *        advance native graceful-close sequences.
 */
void TcpIp_MainFunction(void)
{
    TcpIp_SocketIdType i;

    if (TcpIp_InternalState.State != TCPIP_STATE_INIT)
    {
        return;
    }

    /* Poll link state (stub — reports UP if init'd) */
    TcpIp_InternalState.LinkState = TCPIP_LINK_STATE_UP;
    TcpIp_InternalState.InterfaceState = TCPIP_IFSTATE_UP;

    /* Advance native graceful close: FIN_WAIT_1 -> FIN_WAIT_2 -> TIME_WAIT
     * -> CLOSED (slot released).  lwIP builds complete the close inside the
     * stack (tcp_close) and never set CloseInProgress on the adaption side. */
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        TcpIp_SocketEntryType* entry = &TcpIp_InternalState.Sockets[i];
        if ((entry->InUse) && (entry->CloseInProgress))
        {
            if (entry->TcpState == TCPIP_TCPSTATE_FIN_WAIT_1)
            {
                entry->TcpState = TCPIP_TCPSTATE_FIN_WAIT_2;
                TcpIp_LocalUpdateConnState(entry);
            }
            else if (entry->TcpState == TCPIP_TCPSTATE_FIN_WAIT_2)
            {
                entry->TcpState = TCPIP_TCPSTATE_TIME_WAIT;
                TcpIp_LocalUpdateConnState(entry);
            }
            else if (entry->TcpState == TCPIP_TCPSTATE_TIME_WAIT)
            {
                (void)TcpIp_Close(i, TRUE);
            }
        }
    }

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    /* lwIP periodic duties: ARP timer, TCP timer, etc. */
    sys_check_timeouts();
#endif
}

/*==================================================================================================
 *                              SOCKET-CLASS APIs (AUTOSAR SWS TcpIp)
 *==================================================================================================*/

/**
 * @brief Put a TCP socket into listening state (server).
 */
TcpIp_ReturnType TcpIp_Listen(TcpIp_SocketIdType SocketId, uint8 Backlog)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_LISTEN, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if (entry->SockType != TCPIP_SOCK_STREAM)
    {
        return TCPIP_E_INVALID_PROTOCOL;
    }
    if (entry->TcpState == TCPIP_TCPSTATE_LISTEN)
    {
        /* Re-listen: update backlog only */
        entry->Backlog = Backlog;
        return TCPIP_OK;
    }
    if (entry->TcpState != TCPIP_TCPSTATE_CLOSED)
    {
        return TCPIP_E_INVALID_STATE;
    }

    {
        uint8 effectiveBacklog = Backlog;
        if (effectiveBacklog > (uint8)TCPIP_MAX_PENDING_CONNECTIONS)
        {
            effectiveBacklog = (uint8)TCPIP_MAX_PENDING_CONNECTIONS;
        }
        entry->Backlog = effectiveBacklog;
    }
    entry->TcpState = TCPIP_TCPSTATE_LISTEN;
    TcpIp_LocalUpdateConnState(entry);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (entry->Pcb != NULL_PTR)
    {
        struct tcp_pcb* listenPcb = tcp_listen_with_backlog((struct tcp_pcb*)entry->Pcb, Backlog);
        if (listenPcb == NULL_PTR)
        {
            return TCPIP_E_NOT_OK;   /* pcb unchanged on failure */
        }
        entry->Pcb = (void*)listenPcb;
    }
#endif

    return TCPIP_OK;
}

/**
 * @brief Initiate a connection to a remote address.
 */
TcpIp_ReturnType TcpIp_Connect(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_CONNECT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (RemoteAddr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_CONNECT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if ((entry->TcpState != TCPIP_TCPSTATE_CLOSED) || (entry->IsConnected))
    {
        return TCPIP_E_ISCONN;
    }

    entry->RemotePort = RemoteAddr->port;
    (void)memcpy(entry->RemoteAddr, RemoteAddr->addr, TCPIP_IPV6_ADDR_LEN);
    entry->TcpState = TCPIP_TCPSTATE_SYN_SENT;
    TcpIp_LocalUpdateConnState(entry);
    TCPIP_STATS_INC(TcpActiveOpens);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    {
        err_t err;
        ip_addr_t ip;
        TcpIp_LocalToIpAddr(RemoteAddr, &ip);
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            tcp_arg((struct tcp_pcb*)entry->Pcb, (void*)(uintptr_t)SocketId);
            err = tcp_connect((struct tcp_pcb*)entry->Pcb, &ip, RemoteAddr->port, TcpIp_LwipConnected);
            if (err != ERR_OK)
            {
                entry->TcpState = TCPIP_TCPSTATE_CLOSED;
                TcpIp_LocalUpdateConnState(entry);
                return TCPIP_E_NOT_OK;
            }
        }
        else
        {
            err = udp_connect((struct udp_pcb*)entry->Pcb, &ip, RemoteAddr->port);
            if (err != ERR_OK)
            {
                entry->TcpState = TCPIP_TCPSTATE_CLOSED;
                TcpIp_LocalUpdateConnState(entry);
                return TCPIP_E_NOT_OK;
            }
        }
    }
#else
    /* Native simulation: complete the handshake synchronously. */
    if (entry->SockType == TCPIP_SOCK_STREAM)
    {
        entry->TcpState = TCPIP_TCPSTATE_ESTABLISHED;
        TcpIp_LocalUpdateConnState(entry);
        TCPIP_STATS_INC(TcpEstablishedCount);
    }
#endif

    return TCPIP_OK;
}

/**
 * @brief Accept a pending connection on a listening socket.
 */
TcpIp_ReturnType TcpIp_Accept(TcpIp_SocketIdType SocketId, TcpIp_SocketIdType* NewSocketId)
{
    TcpIp_SocketEntryType* entry;
    TcpIp_SocketIdType child;
    uint8 i;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_ACCEPT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (NewSocketId == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_ACCEPT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if (entry->TcpState != TCPIP_TCPSTATE_LISTEN)
    {
        return TCPIP_E_INVALID_STATE;
    }
    if (entry->PendingCount == 0U)
    {
        return TCPIP_E_NOT_OK;   /* nothing pending */
    }

    /* FIFO pop */
    child = entry->PendingSockets[0];
    for (i = 1U; i < entry->PendingCount; i++)
    {
        entry->PendingSockets[i - 1U] = entry->PendingSockets[i];
    }
    entry->PendingCount--;

    *NewSocketId = child;
    return TCPIP_OK;
}

/**
 * @brief Abort a connection immediately.
 */
TcpIp_ReturnType TcpIp_Abort(TcpIp_SocketIdType SocketId)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_ABORT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    (void)TcpIp_LocalAbortConnection(entry);
    TcpIp_LocalFreePcb(entry);
    TcpIp_LocalFreeSlot(SocketId);
    return TCPIP_OK;
}

/**
 * @brief Set the remote address of a socket.
 */
TcpIp_ReturnType TcpIp_SetRemoteAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETREMOTEADDR, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (RemoteAddr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETREMOTEADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    entry->RemotePort = RemoteAddr->port;
    (void)memcpy(entry->RemoteAddr, RemoteAddr->addr, TCPIP_IPV6_ADDR_LEN);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (entry->SockType == TCPIP_SOCK_DGRAM)
    {
        /* UDP: set the default peer for sends (sendto semantics). */
        ip_addr_t ip;
        TcpIp_LocalToIpAddr(RemoteAddr, &ip);
        (void)udp_connect((struct udp_pcb*)entry->Pcb, &ip, RemoteAddr->port);
    }
#endif

    return TCPIP_OK;
}

/**
 * @brief Set the local address of a socket (stored; TcpIp_Bind applies it).
 */
TcpIp_ReturnType TcpIp_SetLocalAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETLOCALADDR, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (LocalAddr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETLOCALADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    entry->LocalPort = LocalAddr->port;
    (void)memcpy(entry->LocalAddr, LocalAddr->addr, TCPIP_IPV6_ADDR_LEN);
    return TCPIP_OK;
}

/**
 * @brief Bind a socket to a local address (AUTOSAR name).
 */
TcpIp_ReturnType TcpIp_BindLocalAddr(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddr)
{
    return TcpIp_Bind(SocketId, LocalAddr);
}

/**
 * @brief Get the local address of a socket.
 */
TcpIp_ReturnType TcpIp_GetLocalAddr(TcpIp_SocketIdType SocketId, TcpIp_SockAddrType* LocalAddr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETLOCALADDR, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (LocalAddr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETLOCALADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    LocalAddr->domain = entry->Domain;
    LocalAddr->port   = entry->LocalPort;
    (void)memcpy(LocalAddr->addr, entry->LocalAddr, TCPIP_IPV6_ADDR_LEN);
    return TCPIP_OK;
}

/**
 * @brief Get the remote address of a socket.
 */
TcpIp_ReturnType TcpIp_GetRemoteAddr(TcpIp_SocketIdType SocketId, TcpIp_SockAddrType* RemoteAddr)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETREMOTEADDR, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (RemoteAddr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETREMOTEADDR, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    RemoteAddr->domain = entry->Domain;
    RemoteAddr->port   = entry->RemotePort;
    (void)memcpy(RemoteAddr->addr, entry->RemoteAddr, TCPIP_IPV6_ADDR_LEN);
    return TCPIP_OK;
}

/**
 * @brief Get the connection state of a socket.
 */
TcpIp_ReturnType TcpIp_GetConnectionState(TcpIp_SocketIdType SocketId, TcpIp_ConnectionStateType* ConnState)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETCONNSTATE, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (ConnState == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETCONNSTATE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    *ConnState = entry->ConnState;
    return TCPIP_OK;
}

/**
 * @brief Get the TCP protocol state of a socket.
 */
TcpIp_ReturnType TcpIp_GetTcpState(TcpIp_SocketIdType SocketId, TcpIp_TcpStateType* TcpState)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTCPSTATE, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (TcpState == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTCPSTATE, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    *TcpState = entry->TcpState;
    return TCPIP_OK;
}

/**
 * @brief Drive the TCP state machine.
 */
TcpIp_ReturnType TcpIp_ChangeTcpState(TcpIp_SocketIdType SocketId, TcpIp_TcpStateType NewState)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_CHANGETCPSTATE, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    /* LISTEN -> SYN_RECEIVED: incoming SYN on a listener — allocate a
     * child socket and queue it as a pending connection. */
    if ((entry->TcpState == TCPIP_TCPSTATE_LISTEN) &&
        (NewState == TCPIP_TCPSTATE_SYN_RECEIVED))
    {
        TcpIp_SocketIdType child;
        TcpIp_SocketEntryType* childEntry;

        if (entry->PendingCount >= TCPIP_MAX_PENDING_CONNECTIONS)
        {
            return TCPIP_E_NOBUFS;
        }

        child = TcpIp_LocalAllocSlot();
        if (child == TCPIP_SOCKETID_INVALID)
        {
            return TCPIP_E_NOBUFS;
        }
        childEntry = &TcpIp_InternalState.Sockets[child];
        childEntry->SockType = TCPIP_SOCK_STREAM;
        childEntry->Domain   = entry->Domain;
        childEntry->LocalPort = entry->LocalPort;
        (void)memcpy(childEntry->LocalAddr, entry->LocalAddr, TCPIP_IPV6_ADDR_LEN);
        childEntry->TcpState = TCPIP_TCPSTATE_SYN_RECEIVED;
        childEntry->ConnState = TCPIP_CONNSTATE_CONNECTED;
        TCPIP_STATS_INC(TcpPassiveOpens);

        entry->PendingSockets[entry->PendingCount] = child;
        entry->PendingCount++;

        return TCPIP_OK;
    }

    if (!TcpIp_LocalIsValidTransition(entry->TcpState, NewState))
    {
        return TCPIP_E_INVALID_STATE;
    }

    entry->TcpState = NewState;
    if (NewState == TCPIP_TCPSTATE_ESTABLISHED)
    {
        TCPIP_STATS_INC(TcpEstablishedCount);
    }
    if (NewState == TCPIP_TCPSTATE_CLOSED)
    {
        entry->CloseInProgress = FALSE;
    }
    TcpIp_LocalUpdateConnState(entry);
    return TCPIP_OK;
}

/**
 * @brief Attach a static receive buffer to a socket.
 */
TcpIp_ReturnType TcpIp_SetRxBuffer(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 Capacity)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETRXBUFFER, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Buffer == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETRXBUFFER, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    entry->RxUserBuf = Buffer;
    entry->RxUserCapacity = Capacity;
    return TCPIP_OK;
}

/**
 * @brief Get the oldest received chunk (zero-copy RX model).
 */
TcpIp_ReturnType TcpIp_GetRxBuffer(TcpIp_SocketIdType SocketId, uint8** DataPtr, uint16* Length)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETRXBUFFER, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if ((DataPtr == NULL_PTR) || (Length == NULL_PTR))
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETRXBUFFER, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if (entry->RxChunkInUse)
    {
        return TCPIP_E_NOBUFS;   /* previous buffer not released */
    }
    if ((!entry->RxUserPending) && (entry->RxCount == 0U))
    {
        return TCPIP_E_NOT_OK;   /* no data */
    }

    *DataPtr = TcpIp_LocalRxPeekData(entry);
    *Length = TcpIp_LocalRxPeekLen(entry);
    entry->RxChunkInUse = TRUE;
    return TCPIP_OK;
}

/**
 * @brief Release the buffer returned by TcpIp_GetRxBuffer.
 */
TcpIp_ReturnType TcpIp_ReleaseRxBuffer(TcpIp_SocketIdType SocketId)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RELEASERXBUFFER, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if ((!entry->RxUserPending) && (entry->RxCount == 0U))
    {
        return TCPIP_E_NOT_OK;   /* nothing to release */
    }

    TcpIp_LocalRxDropHead(entry);
    return TCPIP_OK;
}

/**
 * @brief Get a transmit buffer (zero-copy TX model).
 */
TcpIp_ReturnType TcpIp_GetTxBuffer(TcpIp_SocketIdType SocketId, uint8** DataPtr, uint16* Length)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTXBUFFER, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if ((DataPtr == NULL_PTR) || (Length == NULL_PTR))
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTXBUFFER, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    *DataPtr = &entry->TxBuf[0];
    *Length = (uint16)TCPIP_PBUF_POOL_BUF_SIZE;
    return TCPIP_OK;
}

/**
 * @brief Commit a transmit buffer for sending.
 */
TcpIp_ReturnType TcpIp_ReleaseTxBuffer(TcpIp_SocketIdType SocketId, uint16 Length)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RELEASETXBUFFER, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }
    if (Length > (uint16)TCPIP_PBUF_POOL_BUF_SIZE)
    {
        return TCPIP_E_BUFFER_OVERFLOW;
    }

    entry->TxLen = Length;
    return TcpIp_LocalCommitTx(SocketId, entry->TxBuf, Length);
}

/**
 * @brief Set a TCP option.
 */
TcpIp_ReturnType TcpIp_SetTcpOption(TcpIp_SocketIdType SocketId, TcpIp_TcpOptionType Option, uint32 Value)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETTCPOPTION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    switch (Option)
    {
        case TCPIP_TCPOPT_REUSEADDR:
            entry->TcpReuseAddr = (Value != 0U) ? TRUE : FALSE;
            break;
        case TCPIP_TCPOPT_KEEPALIVE:
            entry->TcpKeepAlive = (Value != 0U) ? TRUE : FALSE;
            break;
        case TCPIP_TCPOPT_NODELAY:
            entry->TcpNoDelay = (Value != 0U) ? TRUE : FALSE;
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
            if (entry->Pcb != NULL_PTR)
            {
                if (entry->TcpNoDelay)
                {
                    tcp_nagle_disable((struct tcp_pcb*)entry->Pcb);
                }
                else
                {
                    tcp_nagle_enable((struct tcp_pcb*)entry->Pcb);
                }
            }
#endif
            break;
        case TCPIP_TCPOPT_MAXSEG:
            entry->TcpMaxSeg = (uint16)Value;
            break;
        default:
            return TCPIP_E_NOT_OK;
    }
    return TCPIP_OK;
}

/**
 * @brief Get a TCP option.
 */
TcpIp_ReturnType TcpIp_GetTcpOption(TcpIp_SocketIdType SocketId, TcpIp_TcpOptionType Option, uint32* Value)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTCPOPTION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Value == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETTCPOPTION, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    switch (Option)
    {
        case TCPIP_TCPOPT_REUSEADDR:
            *Value = entry->TcpReuseAddr ? 1UL : 0UL;
            break;
        case TCPIP_TCPOPT_KEEPALIVE:
            *Value = entry->TcpKeepAlive ? 1UL : 0UL;
            break;
        case TCPIP_TCPOPT_NODELAY:
            *Value = entry->TcpNoDelay ? 1UL : 0UL;
            break;
        case TCPIP_TCPOPT_MAXSEG:
            *Value = (uint32)entry->TcpMaxSeg;
            break;
        default:
            return TCPIP_E_NOT_OK;
    }
    return TCPIP_OK;
}

/**
 * @brief Set a UDP option.
 */
TcpIp_ReturnType TcpIp_SetUdpOption(TcpIp_SocketIdType SocketId, TcpIp_UdpOptionType Option, uint32 Value)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETUDPOPTION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    switch (Option)
    {
        case TCPIP_UDPOPT_REUSEADDR:
            /* Stored only: reuse-addr semantics are handled by the socket
             * table (unique slot per socket) and lwIP port binding. */
            break;
        case TCPIP_UDPOPT_TTL:
            entry->UdpTtl = (uint8)Value;
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
            if (entry->Pcb != NULL_PTR)
            {
                ((struct udp_pcb*)entry->Pcb)->ttl = (u8_t)Value;
            }
#endif
            break;
        case TCPIP_UDPOPT_TOS:
            entry->UdpTos = (uint8)Value;
            break;
        default:
            return TCPIP_E_NOT_OK;
    }
    return TCPIP_OK;
}

/**
 * @brief Get a UDP option.
 */
TcpIp_ReturnType TcpIp_GetUdpOption(TcpIp_SocketIdType SocketId, TcpIp_UdpOptionType Option, uint32* Value)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETUDPOPTION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Value == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETUDPOPTION, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    switch (Option)
    {
        case TCPIP_UDPOPT_REUSEADDR:
            *Value = 0UL;
            break;
        case TCPIP_UDPOPT_TTL:
            *Value = (uint32)entry->UdpTtl;
            break;
        case TCPIP_UDPOPT_TOS:
            *Value = (uint32)entry->UdpTos;
            break;
        default:
            return TCPIP_E_NOT_OK;
    }
    return TCPIP_OK;
}

/**
 * @brief RX data ingress hook — called by the lwIP adapter / EthIf Rx path
 *        when data for a local socket arrives.
 */
TcpIp_ReturnType TcpIp_RxIndication(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RXINDICATION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Data == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RXINDICATION, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        TCPIP_STATS_INC(RxErrors);
        return TCPIP_E_NOT_OK;
    }
    if (Length == 0U)
    {
        return TCPIP_OK;
    }
    if (Length > (uint16)TCPIP_PBUF_POOL_BUF_SIZE)
    {
        TCPIP_STATS_INC(RxOverflows);
        return TCPIP_E_BUFFER_OVERFLOW;
    }

    /* Preferred path: user-attached receive buffer when free. */
    if ((entry->RxUserBuf != NULL_PTR) && (!entry->RxUserPending))
    {
        if (Length <= entry->RxUserCapacity)
        {
            (void)memcpy(entry->RxUserBuf, Data, Length);
            entry->RxUserLen = Length;
            entry->RxUserPending = TRUE;
            TCPIP_STATS_INC(RxPackets);
            TCPIP_STATS_ADD(RxBytes, Length);
            return TCPIP_OK;
        }
        /* User buffer too small: fall through to the internal pool. */
    }

    /* Pool-backed ring queue (depth TCPIP_MAX_RX_BUFFERS per socket). */
    if (entry->RxCount >= (uint8)TCPIP_MAX_RX_BUFFERS)
    {
        TCPIP_STATS_INC(RxOverflows);
        return TCPIP_E_BUFFER_OVERFLOW;   /* queue full */
    }
    (void)memcpy(&entry->RxPool[entry->RxTail][0], Data, Length);
    entry->RxPoolLen[entry->RxTail] = Length;
    entry->RxTail = (uint8)((entry->RxTail + 1U) % TCPIP_MAX_RX_BUFFERS);
    entry->RxCount++;
    TCPIP_STATS_INC(RxPackets);
    TCPIP_STATS_ADD(RxBytes, Length);
    return TCPIP_OK;
}

/**
 * @brief TX completion hook — called by the lwIP adapter on send
 *        confirmation.
 */
TcpIp_ReturnType TcpIp_TxConfirmation(TcpIp_SocketIdType SocketId, boolean Success)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_TXCONFIRMATION, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    (void)Success;   /* status recorded by statistics (B1 commit 4) */
    return TCPIP_OK;
}

/*==================================================================================================
 *                                      VLAN (B1)
 *==================================================================================================*/

#if (TCPIP_VLAN_SUPPORT == STD_ON)

/**
 * @brief Set the interface VLAN configuration.
 *
 * The adaption layer validates and stores the membership.  On-wire tag
 * insertion is delegated to lwIP (LWIP_VLAN_PCP, per-PCB TCI set by the
 * EthIf/lwIP port from this configuration) or to the Ethernet Switch
 * driver (EthSwt, B2).
 */
TcpIp_ReturnType TcpIp_SetVlanConfig(const TcpIp_VlanConfigType* VlanConfigPtr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETVLANCONFIG, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (VlanConfigPtr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_SETVLANCONFIG, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    if (VlanConfigPtr->VlanId > 4095U)
    {
        return TCPIP_E_PARAM_CONFIG;   /* 12-bit VID */
    }
    if (VlanConfigPtr->VlanPriority > 7U)
    {
        return TCPIP_E_PARAM_CONFIG;   /* 3-bit PCP */
    }

    TcpIp_InternalState.VlanConfig.VlanEnabled = VlanConfigPtr->VlanEnabled;
    TcpIp_InternalState.VlanConfig.VlanId = VlanConfigPtr->VlanId;
    TcpIp_InternalState.VlanConfig.VlanPriority = VlanConfigPtr->VlanPriority;
    TcpIp_InternalState.VlanConfig.DropUntagged = VlanConfigPtr->DropUntagged;

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON) && defined(LWIP_VLAN_PCP) && (LWIP_VLAN_PCP == 1)
    /* lwIP per-PCB VLAN TCI: applied through netif hints by the lwIP
     * port (EthIf).  This module stores the config; the actual TCI is
     * consumed by LWIP_VLAN_PCP-enabled builds at transmit time. */
#endif

    return TCPIP_OK;
}

/**
 * @brief Get the interface VLAN configuration.
 */
TcpIp_ReturnType TcpIp_GetVlanConfig(TcpIp_VlanConfigType* VlanConfigPtr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETVLANCONFIG, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (VlanConfigPtr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETVLANCONFIG, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    *VlanConfigPtr = TcpIp_InternalState.VlanConfig;
    return TCPIP_OK;
}

#endif /* TCPIP_VLAN_SUPPORT */

/*==================================================================================================
 *                                  STATISTICS (B1)
 *==================================================================================================*/

#if (TCPIP_ENABLE_STATISTICS == STD_ON)

/**
 * @brief Get the module statistics counters.
 */
TcpIp_ReturnType TcpIp_GetStatistics(TcpIp_StatisticsType* StatisticsPtr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETSTATISTICS, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (StatisticsPtr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_GETSTATISTICS, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    *StatisticsPtr = TcpIp_InternalState.Statistics;
    return TCPIP_OK;
}

/**
 * @brief Reset the module statistics counters to zero.
 */
TcpIp_ReturnType TcpIp_ResetStatistics(void)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TCPIP_SID_RESETSTATISTICS, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    (void)memset(&TcpIp_InternalState.Statistics, 0, sizeof(TcpIp_InternalState.Statistics));
    return TCPIP_OK;
}

#endif /* TCPIP_ENABLE_STATISTICS */
