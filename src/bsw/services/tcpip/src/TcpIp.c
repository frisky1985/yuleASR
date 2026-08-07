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
 * Wraps lwIP raw/sequential API into AUTOSAR TcpIp API surface.
 * Provides socket management, send/receive, address lookup, link state.
 */

#include "TcpIp.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define TCPIP_STATE_UNINIT                      (0x00U)
#define TCPIP_STATE_INIT                        (0x01U)

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

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/* Socket entry tracking an lwIP PCB */
typedef struct {
    boolean             InUse;
    TcpIp_SockTypeType  SockType;   /* TCPIP_SOCK_STREAM or TCPIP_SOCK_DGRAM */
    TcpIp_DomainType    Domain;     /* TCPIP_AF_INET or TCPIP_AF_INET6       */
    uint16              LocalPort;
    uint8               RemoteAddr[16];
    uint16              RemotePort;
    boolean             IsConnected;
    void*               Pcb;        /* Opaque pointer to lwIP pcb (tcp_pcb/udp_pcb) */
} TcpIp_SocketEntryType;

/* Internal module state */
typedef struct {
    uint8              State;
    const TcpIp_ConfigType* ConfigPtr;
    TcpIp_SocketEntryType Sockets[TCPIP_MAX_SOCKETS];
    TcpIp_LinkStateType   LinkState;
    uint32                Ipv4Addr;
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
    TcpIp_InternalState.LinkState = TCPIP_LINK_STATE_DOWN;
    TcpIp_InternalState.Ipv4Addr  = TCPIP_DEFAULT_IPV4_ADDR;

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
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (SocketId == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_PARAM_POINTER);
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
    return TCPIP_OK;
}

/**
 * @brief Close and destroy a socket.
 */
TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Force)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    /* lwIP PCB free */
#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    if (entry->Pcb != NULL_PTR)
    {
        if (entry->SockType == TCPIP_SOCK_STREAM)
        {
            tcp_close((struct tcp_pcb*)entry->Pcb);
        }
        else
        {
            udp_remove((struct udp_pcb*)entry->Pcb);
        }
    }
#endif

    (void)memset(entry, 0, sizeof(TcpIp_SocketEntryType));
    (void)Force;  /* AUTOSAR: Force argument reserved for future use */
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
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Addr == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    entry->LocalPort = Addr->port;
    (void)memcpy(entry->RemoteAddr, Addr->addr, TCPIP_IPV4_ADDR_LEN);

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    {
        ip_addr_t ip;
        IP4_ADDR(&ip, Addr->addr[0], Addr->addr[1], Addr->addr[2], Addr->addr[3]);
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
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if (Data == NULL_PTR)
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
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
                return TCPIP_E_NOT_OK;
            }
        }
        else
        {
            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, Length, PBUF_POOL);
            if (p == NULL_PTR)
            {
                return TCPIP_E_NOT_OK;
            }
            (void)memcpy(p->payload, Data, Length);
            err = udp_send((struct udp_pcb*)entry->Pcb, p);
            pbuf_free(p);
            if (err != ERR_OK)
            {
                return TCPIP_E_NOT_OK;
            }
        }
    }
#else
    (void)Data;
    (void)Length;
#endif

    return TCPIP_OK;
}

/**
 * @brief Transmit — alias for TcpIp_Send for higher-layer compatibility.
 */
TcpIp_ReturnType TcpIp_Transmit(TcpIp_SocketIdType SocketId, const uint8* Data, uint16 Length)
{
    return TcpIp_Send(SocketId, Data, Length);
}

/**
 * @brief Receive data from a socket.
 */
TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId, uint8* Buffer, uint16 MaxLen, uint16* ReceivedLen)
{
    TcpIp_SocketEntryType* entry;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (!TCPIP_IS_INIT())
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_UNINIT);
        return TCPIP_E_NOT_OK;
    }
    if ((Buffer == NULL_PTR) || (ReceivedLen == NULL_PTR))
    {
        TCPIP_DET_REPORT_ERROR(TcpIP_SID_INIT, TCPIP_E_PARAM_POINTER);
        return TCPIP_E_NOT_OK;
    }
#endif

    entry = TcpIp_LocalFindSocket(SocketId);
    if (entry == NULL_PTR)
    {
        return TCPIP_E_NOT_OK;
    }

    *ReceivedLen = 0U;

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    /* For the raw API the application receives data via callbacks.
     * This function acts as a poll to drain any queued data.
     * In a full implementation the recv callback would buffer data
     * and TcpIp_Receive would copy from that buffer.
     */
    (void)MaxLen;
    return TCPIP_OK;
#else
    (void)MaxLen;
    return TCPIP_OK;
#endif
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
    if ((entry != NULL_PTR) && entry->InUse && entry->IsConnected)
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
 * @brief Reset the TCP/IP stack (tear down and re-init).
 */
TcpIp_ReturnType TcpIp_Reset(void)
{
    TcpIp_ConfigType defaultConfig;

    if (TcpIp_InternalState.State == TCPIP_STATE_INIT)
    {
        TcpIp_DeInit();
    }

    /* Re-init with saved or default config */
    if (TcpIp_InternalState.ConfigPtr != NULL_PTR)
    {
        TcpIp_Init(TcpIp_InternalState.ConfigPtr);
    }
    else
    {
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
 * @brief Main function — called periodically to poll link state etc.
 */
void TcpIp_MainFunction(void)
{
    if (TcpIp_InternalState.State != TCPIP_STATE_INIT)
    {
        return;
    }

    /* Poll link state (stub — reports UP if init'd) */
    TcpIp_InternalState.LinkState = TCPIP_LINK_STATE_UP;

#if defined(TCPIP_ENABLE_LWIP) && (TCPIP_ENABLE_LWIP == STD_ON)
    /* lwIP periodic duties: ARP timer, TCP timer, etc. */
    sys_check_timeouts();
#endif
}
