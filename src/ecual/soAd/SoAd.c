/******************************************************************************
 * @file    SoAd.c
 * @brief   Socket Adapter (SoAd) Implementation - AUTOSAR R22-11
 *
 * Implementation of the SoAd module providing Socket management and
 * PDU routing between PduR and TcpIp modules.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x36 (SoAd)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/soAd/SoAd.h"
#include "ecual/soAd/SoAd_Cfg.h"
#include <string.h>

/******************************************************************************
 * Macros
 ******************************************************************************/
#define SOAD_START_SEC_VAR_INIT_UNSPECIFIED
/* VAR_INIT section start - to be placed by linker */

#define SOAD_END_SEC_VAR_INIT_UNSPECIFIED
/* VAR_INIT section end */

#define SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED
/* VAR_CLEARED section start */

#define SOAD_END_SEC_VAR_CLEARED_UNSPECIFIED
/* VAR_CLEARED section end */

#define SOAD_START_SEC_CODE
/* CODE section start */

#define SOAD_END_SEC_CODE
/* CODE section end */

/******************************************************************************
 * Internal State Definitions
 ******************************************************************************/

/** SoAd Module State */
typedef enum {
    SOAD_STATE_UNINIT = 0,
    SOAD_STATE_INIT,
    SOAD_STATE_ACTIVE
} SoAd_StateType;

/** Socket Connection Runtime Data */
typedef struct {
    SoAd_SoConStateType State;
    tcpip_socket_id_t TcpIpSocketId;
    SoAd_SocketAddrType LocalAddr;
    SoAd_SocketAddrType RemoteAddr;
    boolean IsServer;
    boolean IsOpen;
    uint32 BytesSent;
    uint32 BytesReceived;
    uint32 ConnectTime;
    uint32 LastActivity;
    uint16 ErrorCount;
    uint8 ConnRetryCount;
    uint32 StateTimer;
} SoAd_SoConRuntimeType;

/** PDU Route Runtime Data */
typedef struct {
    boolean IsEnabled;
    uint16 PendingTxPdus;
    uint16 PendingRxPdus;
} SoAd_PduRouteRuntimeType;

/** Routing Group Runtime Data */
typedef struct {
    boolean IsEnabled;
    boolean IsOpen;
} SoAd_RoutingGroupRuntimeType;

/******************************************************************************
 * Module Variables
 ******************************************************************************/

/** Module state */
static SoAd_StateType SoAd_State = SOAD_STATE_UNINIT;

/** Module configuration pointer */
static const SoAd_ConfigType *SoAd_ConfigPtr = NULL;

/** Socket connection runtime data */
static SoAd_SoConRuntimeType SoAd_SoConRuntime[SOAD_CFG_MAX_SOCKET_CONNECTIONS];

/** PDU route runtime data */
static SoAd_PduRouteRuntimeType SoAd_PduRouteRuntime[SOAD_CFG_MAX_PDU_ROUTES];

/** Routing group runtime data */
static SoAd_RoutingGroupRuntimeType SoAd_RoutingGroupRuntime[SOAD_CFG_MAX_ROUTING_GROUPS];

/** TX buffer for PDU transmission */
static uint8 SoAd_TxBuffer[SOAD_CFG_MAX_PDU_SIZE];

/** RX buffer for PDU reception */
static uint8 SoAd_RxBuffer[SOAD_CFG_SOCKET_RX_BUFFER_SIZE];

/** Buffer for PDU header */
static uint8 SoAd_HeaderBuffer[SOAD_CFG_PDU_HEADER_SIZE];

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static Std_ReturnType SoAd_CreateSocket(SoAd_SoConIdType SoConId);
static Std_ReturnType SoAd_BindSocket(SoAd_SoConIdType SoConId);
static Std_ReturnType SoAd_ConnectSocket(SoAd_SoConIdType SoConId);
static Std_ReturnType SoAd_ListenSocket(SoAd_SoConIdType SoConId);
static void SoAd_CloseSocketInternal(SoAd_SoConIdType SoConId, boolean Abort);
static void SoAd_ProcessSocketStateMachine(SoAd_SoConIdType SoConId);
static void SoAd_ProcessRxData(SoAd_SoConIdType SoConId);
static void SoAd_ProcessTxData(SoAd_SoConIdType SoConId);
static const SoAd_SocketConnectionCfgType *SoAd_GetSoConCfg(SoAd_SoConIdType SoConId);
static const SoAd_PduRouteCfgType *SoAd_GetPduRouteCfg(uint16 TxPduId);
static SoAd_PduRouteIdType SoAd_FindPduRouteByTxPduId(uint16 TxPduId);
static SoAd_PduRouteIdType SoAd_FindPduRouteBySoConId(SoAd_SoConIdType SoConId, boolean IsRx);
static void SoAd_UpdateSocketState(SoAd_SoConIdType SoConId, SoAd_SoConStateType NewState);
static boolean SoAd_IsRoutingEnabled(SoAd_PduRouteIdType RouteId);
static Std_ReturnType SoAd_SendPduData(SoAd_SoConIdType SoConId, const SoAd_PduInfoType *PduInfoPtr);

/******************************************************************************
 * Internal Functions
 ******************************************************************************/

/**
 * @brief Get Socket connection configuration
 */
static const SoAd_SocketConnectionCfgType *SoAd_GetSoConCfg(SoAd_SoConIdType SoConId)
{
    const SoAd_SocketConnectionCfgType *ConfigPtr = NULL;
    uint16 i;

    if ((SoAd_ConfigPtr != NULL) && (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS)) {
        for (i = 0U; i < SoAd_ConfigPtr->NumSocketConnections; i++) {
            if (SoAd_ConfigPtr->SocketConnections[i].SoConId == SoConId) {
                ConfigPtr = &SoAd_ConfigPtr->SocketConnections[i];
                break;
            }
        }
    }
    return ConfigPtr;
}

/**
 * @brief Get PDU route configuration by TxPduId
 */
static const SoAd_PduRouteCfgType *SoAd_GetPduRouteCfg(uint16 TxPduId)
{
    const SoAd_PduRouteCfgType *ConfigPtr = NULL;
    uint16 i;

    if ((SoAd_ConfigPtr != NULL) && (TxPduId < SOAD_INVALID_PDU_ID)) {
        for (i = 0U; i < SoAd_ConfigPtr->NumPduRoutes; i++) {
            if (SoAd_ConfigPtr->PduRoutes[i].TxPduId == TxPduId) {
                ConfigPtr = &SoAd_ConfigPtr->PduRoutes[i];
                break;
            }
        }
    }
    return ConfigPtr;
}

/**
 * @brief Find PDU route ID by TxPduId
 */
static SoAd_PduRouteIdType SoAd_FindPduRouteByTxPduId(uint16 TxPduId)
{
    SoAd_PduRouteIdType RouteId = SOAD_INVALID_PDU_ROUTE_ID;
    uint16 i;

    if (SoAd_ConfigPtr != NULL) {
        for (i = 0U; i < SoAd_ConfigPtr->NumPduRoutes; i++) {
            if (SoAd_ConfigPtr->PduRoutes[i].TxPduId == TxPduId) {
                RouteId = SoAd_ConfigPtr->PduRoutes[i].RouteId;
                break;
            }
        }
    }
    return RouteId;
}

/**
 * @brief Find PDU route ID by Socket connection ID
 */
static SoAd_PduRouteIdType SoAd_FindPduRouteBySoConId(SoAd_SoConIdType SoConId, boolean IsRx)
{
    SoAd_PduRouteIdType RouteId = SOAD_INVALID_PDU_ROUTE_ID;
    uint16 i;

    if (SoAd_ConfigPtr != NULL) {
        for (i = 0U; i < SoAd_ConfigPtr->NumPduRoutes; i++) {
            if (SoAd_ConfigPtr->PduRoutes[i].SoConId == SoConId) {
                RouteId = SoAd_ConfigPtr->PduRoutes[i].RouteId;
                break;
            }
        }
    }
    return RouteId;
}

/**
 * @brief Check if routing is enabled for a PDU route
 */
static boolean SoAd_IsRoutingEnabled(SoAd_PduRouteIdType RouteId)
{
    boolean Enabled = FALSE;

    if ((RouteId < SOAD_CFG_MAX_PDU_ROUTES) && (SoAd_State == SOAD_STATE_ACTIVE)) {
        if (SoAd_PduRouteRuntime[RouteId].IsEnabled) {
            Enabled = TRUE;
        }
    }
    return Enabled;
}

/**
 * @brief Update socket connection state
 */
static void SoAd_UpdateSocketState(SoAd_SoConIdType SoConId, SoAd_SoConStateType NewState)
{
    if (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        SoAd_SoConRuntime[SoConId].State = NewState;
        SoAd_SoConRuntime[SoConId].StateTimer = 0U;

#if (SOAD_UL_SO_CON_MODE_CHG == STD_ON)
        /* Notify upper layer of state change */
        SoAd_SoConModeChg(SoConId, NewState);
#endif
    }
}

/**
 * @brief Create socket for a connection
 */
static Std_ReturnType SoAd_CreateSocket(SoAd_SoConIdType SoConId)
{
    Std_ReturnType RetVal = E_NOT_OK;
    const SoAd_SocketConnectionCfgType *SoConCfg;
    tcpip_socket_type_t SocketType;
    tcpip_error_t Error;

    SoConCfg = SoAd_GetSoConCfg(SoConId);
    if (SoConCfg != NULL) {
        /* Determine socket type */
        if (SoConCfg->SocketType == SOAD_SOCKET_TYPE_STREAM) {
            SocketType = TCPIP_SOCK_STREAM;
        } else {
            SocketType = TCPIP_SOCK_DGRAM;
        }

        /* Create socket */
        Error = tcpip_socket_create(SocketType, &SoAd_SoConRuntime[SoConId].TcpIpSocketId);
        if (Error == TCPIP_OK) {
            RetVal = E_OK;
        } else {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SO_CON, SOAD_E_SOCKET_ERROR);
#endif
        }
    }
    return RetVal;
}

/**
 * @brief Bind socket to local address
 */
static Std_ReturnType SoAd_BindSocket(SoAd_SoConIdType SoConId)
{
    Std_ReturnType RetVal = E_NOT_OK;
    const SoAd_SocketConnectionCfgType *SoConCfg;
    tcpip_error_t Error;
    tcpip_port_t AssignedPort;

    SoConCfg = SoAd_GetSoConCfg(SoConId);
    if ((SoConCfg != NULL) && (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS)) {
        Error = tcpip_socket_bind(SoAd_SoConRuntime[SoConId].TcpIpSocketId,
                                   SoConCfg->LocalIpAddr,
                                   SoConCfg->LocalPort,
                                   &AssignedPort);
        if (Error == TCPIP_OK) {
            SoAd_SoConRuntime[SoConId].LocalAddr.addr = SoConCfg->LocalIpAddr;
            SoAd_SoConRuntime[SoConId].LocalAddr.port = AssignedPort;
            RetVal = E_OK;
        } else {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SO_CON, SOAD_E_SOCKET_ERROR);
#endif
        }
    }
    return RetVal;
}

/**
 * @brief Connect socket to remote address
 */
static Std_ReturnType SoAd_ConnectSocket(SoAd_SoConIdType SoConId)
{
    Std_ReturnType RetVal = E_NOT_OK;
    const SoAd_SocketConnectionCfgType *SoConCfg;
    tcpip_error_t Error;
    tcpip_sockaddr_t RemoteAddr;

    SoConCfg = SoAd_GetSoConCfg(SoConId);
    if ((SoConCfg != NULL) && (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS)) {
        RemoteAddr.addr = SoConCfg->RemoteIpAddr;
        RemoteAddr.port = SoConCfg->RemotePort;

        Error = tcpip_socket_connect(SoAd_SoConRuntime[SoConId].TcpIpSocketId, &RemoteAddr);
        if (Error == TCPIP_OK) {
            SoAd_SoConRuntime[SoConId].RemoteAddr.addr = SoConCfg->RemoteIpAddr;
            SoAd_SoConRuntime[SoConId].RemoteAddr.port = SoConCfg->RemotePort;
            RetVal = E_OK;
        } else {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SO_CON, SOAD_E_SOCKET_ERROR);
#endif
        }
    }
    return RetVal;
}

/**
 * @brief Put socket in listening state (server)
 */
static Std_ReturnType SoAd_ListenSocket(SoAd_SoConIdType SoConId)
{
    Std_ReturnType RetVal = E_NOT_OK;
    tcpip_error_t Error;

    if (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        Error = tcpip_socket_listen(SoAd_SoConRuntime[SoConId].TcpIpSocketId, 5U);
        if (Error == TCPIP_OK) {
            RetVal = E_OK;
        }
    }
    return RetVal;
}

/**
 * @brief Close socket internally
 */
static void SoAd_CloseSocketInternal(SoAd_SoConIdType SoConId, boolean Abort)
{
    if (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        if (SoAd_SoConRuntime[SoConId].TcpIpSocketId != 0xFFU) {
            if (Abort == TRUE) {
                (void)tcpip_socket_abort(SoAd_SoConRuntime[SoConId].TcpIpSocketId);
            } else {
                (void)tcpip_socket_close(SoAd_SoConRuntime[SoConId].TcpIpSocketId);
            }
            SoAd_SoConRuntime[SoConId].TcpIpSocketId = 0xFFU;
        }
        SoAd_SoConRuntime[SoConId].IsOpen = FALSE;
        SoAd_UpdateSocketState(SoConId, SOAD_SOCON_CLOSED);
    }
}

/**
 * @brief Send PDU data over socket
 */
static Std_ReturnType SoAd_SendPduData(SoAd_SoConIdType SoConId, const SoAd_PduInfoType *PduInfoPtr)
{
    Std_ReturnType RetVal = E_NOT_OK;
    tcpip_error_t Error;
    uint16_t SentLen = 0U;

    if ((SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS) && (PduInfoPtr != NULL)) {
        if (SoAd_SoConRuntime[SoConId].State == SOAD_SOCON_CONNECTED) {
            Error = tcpip_socket_send(SoAd_SoConRuntime[SoConId].TcpIpSocketId,
                                       PduInfoPtr->SduDataPtr,
                                       PduInfoPtr->SduLength,
                                       &SentLen);
            if (Error == TCPIP_OK) {
                SoAd_SoConRuntime[SoConId].BytesSent += SentLen;
                SoAd_SoConRuntime[SoConId].LastActivity = 0U; /* Reset activity timer */
                RetVal = E_OK;
            }
        }
    }
    return RetVal;
}

/**
 * @brief Process socket state machine
 */
static void SoAd_ProcessSocketStateMachine(SoAd_SoConIdType SoConId)
{
    const SoAd_SocketConnectionCfgType *SoConCfg;

    SoConCfg = SoAd_GetSoConCfg(SoConId);
    if (SoConCfg == NULL) {
        return;
    }

    switch (SoAd_SoConRuntime[SoConId].State) {
        case SOAD_SOCON_CLOSED:
            /* Check if auto-connect is enabled */
            if ((SoConCfg->AutoConnect == TRUE) && (SoConCfg->IsServer == FALSE)) {
                SoAd_UpdateSocketState(SoConId, SOAD_SOCON_INITIALIZED);
            }
            break;

        case SOAD_SOCON_INITIALIZED:
            /* Create socket */
            if (SoAd_CreateSocket(SoConId) == E_OK) {
                /* Bind socket */
                if (SoAd_BindSocket(SoConId) == E_OK) {
                    if (SoConCfg->IsServer == TRUE) {
                        /* Server - start listening */
                        if (SoAd_ListenSocket(SoConId) == E_OK) {
                            SoAd_UpdateSocketState(SoConId, SOAD_SOCON_LISTENING);
                        }
                    } else {
                        /* Client - connect to server */
                        if (SoAd_ConnectSocket(SoConId) == E_OK) {
                            SoAd_UpdateSocketState(SoConId, SOAD_SOCON_CONNECTING);
                        }
                    }
                }
            }
            break;

        case SOAD_SOCON_CONNECTING:
            /* Wait for connection establishment - handled by callback */
            SoAd_SoConRuntime[SoConId].StateTimer += SOAD_MAIN_FUNCTION_PERIOD_MS;
            if (SoAd_SoConRuntime[SoConId].StateTimer >= SoConCfg->ConnTimeoutMs) {
                /* Connection timeout - retry or error */
                SoAd_SoConRuntime[SoConId].ConnRetryCount++;
                if (SoAd_SoConRuntime[SoConId].ConnRetryCount < SOAD_CFG_MAX_CONN_RETRIES) {
                    SoAd_CloseSocketInternal(SoConId, TRUE);
                    SoAd_UpdateSocketState(SoConId, SOAD_SOCON_INITIALIZED);
                } else {
                    SoAd_UpdateSocketState(SoConId, SOAD_SOCON_ERROR);
                }
            }
            break;

        case SOAD_SOCON_LISTENING:
            /* Server waiting for connection - handled by callback */
            break;

        case SOAD_SOCON_CONNECTED:
            /* Connection established - check for timeout */
            SoAd_SoConRuntime[SoConId].LastActivity += SOAD_MAIN_FUNCTION_PERIOD_MS;
            if (SoConCfg->EnableKeepAlive == TRUE) {
                if (SoAd_SoConRuntime[SoConId].LastActivity >= (SoConCfg->KeepAliveTime * 1000U)) {
                    /* Keep-alive timeout - could send probe or close */
                    SoAd_SoConRuntime[SoConId].ErrorCount++;
                }
            }
            break;

        case SOAD_SOCON_ERROR:
            /* Error state - try to recover */
            SoAd_CloseSocketInternal(SoConId, TRUE);
            SoAd_SoConRuntime[SoConId].ConnRetryCount = 0U;
            SoAd_UpdateSocketState(SoConId, SOAD_SOCON_CLOSED);
            break;

        default:
            /* Do nothing */
            break;
    }
}

/**
 * @brief Process received data
 */
static void SoAd_ProcessRxData(SoAd_SoConIdType SoConId)
{
    SoAd_PduRouteIdType RouteId;
    tcpip_error_t Error;
    uint16_t RecvLen = 0U;
    SoAd_PduInfoType PduInfo;

    if (SoConId >= SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        return;
    }

    RouteId = SoAd_FindPduRouteBySoConId(SoConId, TRUE);
    if (RouteId == SOAD_INVALID_PDU_ROUTE_ID) {
        return;
    }

    /* Receive data from socket */
    Error = tcpip_socket_recv(SoAd_SoConRuntime[SoConId].TcpIpSocketId,
                               SoAd_RxBuffer,
                               SOAD_CFG_SOCKET_RX_BUFFER_SIZE,
                               &RecvLen);

    if ((Error == TCPIP_OK) && (RecvLen > 0U)) {
        SoAd_SoConRuntime[SoConId].BytesReceived += RecvLen;
        SoAd_SoConRuntime[SoConId].LastActivity = 0U;

        /* Prepare PDU info */
        PduInfo.SduDataPtr = SoAd_RxBuffer;
        PduInfo.SduLength = RecvLen;
        PduInfo.RouteId = RouteId;

        /* Route to upper layer */
        if (SoAd_IsRoutingEnabled(RouteId)) {
#if (SOAD_UL_IF_RX_INDICATION == STD_ON)
            SoAd_IfRxIndication(SoAd_ConfigPtr->PduRoutes[RouteId].RxPduId, &PduInfo);
#endif
        }
    }
}

/**
 * @brief Process transmit data (retry pending)
 */
static void SoAd_ProcessTxData(SoAd_SoConIdType SoConId)
{
    /* TX processing - handle pending confirmations or retries */
    /* Implementation depends on buffer management strategy */
    (void)SoConId;
}

/******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Initialize SoAd module
 */
Std_ReturnType SoAd_Init(const SoAd_ConfigType *ConfigPtr)
{
    Std_ReturnType RetVal = E_NOT_OK;
    uint16 i;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_INIT, SOAD_E_INVALID_POINTER);
        return E_NOT_OK;
    }

    if (SoAd_State != SOAD_STATE_UNINIT) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_INIT, SOAD_E_ALREADY_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    /* Save configuration pointer */
    SoAd_ConfigPtr = ConfigPtr;

    /* Initialize socket connection runtime data */
    for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
        SoAd_SoConRuntime[i].State = SOAD_SOCON_UNINIT;
        SoAd_SoConRuntime[i].TcpIpSocketId = 0xFFU;
        SoAd_SoConRuntime[i].LocalAddr.addr = 0U;
        SoAd_SoConRuntime[i].LocalAddr.port = 0U;
        SoAd_SoConRuntime[i].RemoteAddr.addr = 0U;
        SoAd_SoConRuntime[i].RemoteAddr.port = 0U;
        SoAd_SoConRuntime[i].IsServer = FALSE;
        SoAd_SoConRuntime[i].IsOpen = FALSE;
        SoAd_SoConRuntime[i].BytesSent = 0U;
        SoAd_SoConRuntime[i].BytesReceived = 0U;
        SoAd_SoConRuntime[i].ConnectTime = 0U;
        SoAd_SoConRuntime[i].LastActivity = 0U;
        SoAd_SoConRuntime[i].ErrorCount = 0U;
        SoAd_SoConRuntime[i].ConnRetryCount = 0U;
        SoAd_SoConRuntime[i].StateTimer = 0U;
    }

    /* Initialize PDU route runtime data */
    for (i = 0U; i < SOAD_CFG_MAX_PDU_ROUTES; i++) {
        SoAd_PduRouteRuntime[i].IsEnabled = TRUE; /* Enabled by default */
        SoAd_PduRouteRuntime[i].PendingTxPdus = 0U;
        SoAd_PduRouteRuntime[i].PendingRxPdus = 0U;
    }

    /* Initialize routing group runtime data */
    for (i = 0U; i < SOAD_CFG_MAX_ROUTING_GROUPS; i++) {
        SoAd_RoutingGroupRuntime[i].IsEnabled = TRUE; /* Enabled by default */
        SoAd_RoutingGroupRuntime[i].IsOpen = FALSE;
    }

    /* Set module state to initialized */
    SoAd_State = SOAD_STATE_INIT;

    /* Initialize all socket connections to CLOSED state */
    for (i = 0U; i < SoAd_ConfigPtr->NumSocketConnections; i++) {
        SoAd_SoConIdType SoConId = SoAd_ConfigPtr->SocketConnections[i].SoConId;
        if (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
            SoAd_SoConRuntime[SoConId].State = SOAD_SOCON_CLOSED;
            SoAd_SoConRuntime[SoConId].IsServer = SoAd_ConfigPtr->SocketConnections[i].IsServer;
        }
    }

    SoAd_State = SOAD_STATE_ACTIVE;
    RetVal = E_OK;

    return RetVal;
}

/**
 * @brief Deinitialize SoAd module
 */
void SoAd_DeInit(void)
{
    uint16 i;

    if (SoAd_State == SOAD_STATE_ACTIVE) {
        /* Close all socket connections */
        for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
            if (SoAd_SoConRuntime[i].State != SOAD_SOCON_UNINIT) {
                SoAd_CloseSocketInternal((SoAd_SoConIdType)i, TRUE);
            }
        }

        /* Clear configuration pointer */
        SoAd_ConfigPtr = NULL;

        /* Set module state to uninitialized */
        SoAd_State = SOAD_STATE_UNINIT;
    }
}

/**
 * @brief Get version information
 */
void SoAd_GetVersionInfo(Std_VersionInfoType *versioninfo)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_VERSION_INFO, SOAD_E_INVALID_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = SOAD_VENDOR_ID;
    versioninfo->moduleID = SOAD_MODULE_ID;
    versioninfo->sw_major_version = SOAD_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SOAD_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SOAD_SW_PATCH_VERSION;
}

/**
 * @brief SoAd main function
 */
void SoAd_MainFunction(void)
{
    uint16 i;

    if (SoAd_State != SOAD_STATE_ACTIVE) {
        return;
    }

    /* Process all socket connections */
    for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
        if (SoAd_SoConRuntime[i].State != SOAD_SOCON_UNINIT) {
            /* Process state machine */
            SoAd_ProcessSocketStateMachine((SoAd_SoConIdType)i);

            /* Process RX/TX data */
            if (SoAd_SoConRuntime[i].State == SOAD_SOCON_CONNECTED) {
                SoAd_ProcessRxData((SoAd_SoConIdType)i);
                SoAd_ProcessTxData((SoAd_SoConIdType)i);
            }
        }
    }
}

/**
 * @brief Open Socket connection
 */
Std_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType SoConId)
{
    Std_ReturnType RetVal = E_NOT_OK;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_State != SOAD_STATE_ACTIVE) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SO_CON, SOAD_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (SoConId >= SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SO_CON, SOAD_E_INVALID_SOCKET);
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConRuntime[SoConId].State == SOAD_SOCON_CLOSED) {
        SoAd_UpdateSocketState(SoConId, SOAD_SOCON_INITIALIZED);
        SoAd_SoConRuntime[SoConId].IsOpen = TRUE;
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Close Socket connection
 */
Std_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType SoConId, boolean Abort)
{
    Std_ReturnType RetVal = E_NOT_OK;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_State != SOAD_STATE_ACTIVE) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_CLOSE_SO_CON, SOAD_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (SoConId >= SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_CLOSE_SO_CON, SOAD_E_INVALID_SOCKET);
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConRuntime[SoConId].State != SOAD_SOCON_UNINIT) {
        SoAd_CloseSocketInternal(SoConId, Abort);
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Get Socket connection state
 */
Std_ReturnType SoAd_GetSoConState(SoAd_SoConIdType SoConId, SoAd_SoConStateType *StatePtr)
{
    Std_ReturnType RetVal = E_NOT_OK;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_State != SOAD_STATE_ACTIVE) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_SO_CON_STATE, SOAD_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (StatePtr == NULL) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_SO_CON_STATE, SOAD_E_INVALID_POINTER);
        return E_NOT_OK;
    }

    if (SoConId >= SOAD_CFG_MAX_SOCKET_CONNECTIONS) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_SO_CON_STATE, SOAD_E_INVALID_SOCKET);
        return E_NOT_OK;
    }
#endif

    *StatePtr = SoAd_SoConRuntime[SoConId].State;
    RetVal = E_OK;

    return RetVal;
}

/**
 * @brief Set Socket connection mode
 */
Std_ReturnType SoAd_SetSoConMode(SoAd_SoConIdType SoConId, SoAd_SoConModeType Mode)
{
    /* Mode change implementation - typically requires reconnection */
    (void)SoConId;
    (void)Mode;
    return E_OK;
}

/**
 * @brief Request IP address assignment
 */
Std_ReturnType SoAd_RequestIpAddrAssignment(SoAd_SoConIdType SoConId)
{
    /* Trigger DHCP or manual IP assignment */
    (void)SoConId;
    return E_OK;
}

/**
 * @brief Release IP address assignment
 */
Std_ReturnType SoAd_ReleaseIpAddrAssignment(SoAd_SoConIdType SoConId)
{
    /* Release IP address */
    (void)SoConId;
    return E_OK;
}

/**
 * @brief Get local address
 */
Std_ReturnType SoAd_GetLocalAddr(SoAd_SoConIdType SoConId, 
                                  SoAd_SocketAddrType *LocalAddrPtr,
                                  uint32 *NetmaskPtr,
                                  uint32 *DefaultRouterPtr)
{
    Std_ReturnType RetVal = E_NOT_OK;

    if ((SoAd_State == SOAD_STATE_ACTIVE) && (SoConId < SOAD_CFG_MAX_SOCKET_CONNECTIONS)) {
        if (LocalAddrPtr != NULL) {
            *LocalAddrPtr = SoAd_SoConRuntime[SoConId].LocalAddr;
            RetVal = E_OK;
        }

        if (NetmaskPtr != NULL) {
            *NetmaskPtr = 0xFFFFFF00U; /* Default /24 */
        }

        if (DefaultRouterPtr != NULL) {
            *DefaultRouterPtr = 0U;
        }
    }

    return RetVal;
}

/**
 * @brief Get physical address (MAC)
 */
Std_ReturnType SoAd_GetPhysAddr(SoAd_SoConIdType SoConId, SoAd_PhysAddrType *PhysAddrPtr)
{
    /* Get MAC address from EthIf */
    (void)SoConId;
    if (PhysAddrPtr != NULL) {
        (*PhysAddrPtr)[0] = 0x00U;
        (*PhysAddrPtr)[1] = 0x11U;
        (*PhysAddrPtr)[2] = 0x22U;
        (*PhysAddrPtr)[3] = 0x33U;
        (*PhysAddrPtr)[4] = 0x44U;
        (*PhysAddrPtr)[5] = 0x55U;
    }
    return E_OK;
}

/**
 * @brief Interface Transmit
 */
Std_ReturnType SoAd_IfTransmit(uint16 TxPduId, const SoAd_PduInfoType *PduInfoPtr)
{
    Std_ReturnType RetVal = E_NOT_OK;
    const SoAd_PduRouteCfgType *PduRouteCfg;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_State != SOAD_STATE_ACTIVE) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL) {
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find PDU route configuration */
    PduRouteCfg = SoAd_GetPduRouteCfg(TxPduId);
    if (PduRouteCfg != NULL) {
        /* Check if routing is enabled */
        if (SoAd_IsRoutingEnabled(PduRouteCfg->RouteId)) {
            /* Check socket state */
            if (SoAd_SoConRuntime[PduRouteCfg->SoConId].State == SOAD_SOCON_CONNECTED) {
                /* Send data */
                if (SoAd_SendPduData(PduRouteCfg->SoConId, PduInfoPtr) == E_OK) {
                    RetVal = E_OK;

                    /* Notify upper layer of TX confirmation */
#if (SOAD_UL_IF_TX_CONFIRMATION == STD_ON)
                    SoAd_IfTxConfirmation(TxPduId);
#endif
                }
            }
        }
    } else {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_INVALID_PDUID);
#endif
    }

    return RetVal;
}

/**
 * @brief TP Transmit
 */
Std_ReturnType SoAd_TpTransmit(uint16 TxPduId, 
                                const SoAd_PduInfoType *PduInfoPtr,
                                const uint8 *RetryInfoPtr,
                                uint16 TpDataLength)
{
    /* TP transmission with segmentation support */
    (void)TxPduId;
    (void)PduInfoPtr;
    (void)RetryInfoPtr;
    (void)TpDataLength;
    return E_NOT_OK;
}

/**
 * @brief TP Provide RX Buffer
 */
Std_ReturnType SoAd_TpProvideRxBuffer(uint16 RxPduId, 
                                       uint16 TpSduLength,
                                       SoAd_PduInfoType *PduInfoPtr)
{
    (void)RxPduId;
    (void)TpSduLength;
    (void)PduInfoPtr;
    return E_NOT_OK;
}

/**
 * @brief TP RX Indication
 */
void SoAd_TpRxIndication(uint16 RxPduId, Std_ReturnType Result)
{
    (void)RxPduId;
    (void)Result;
}

/******************************************************************************
 * Routing Group API
 ******************************************************************************/

/**
 * @brief Enable routing group
 */
Std_ReturnType SoAd_EnableRouting(uint16 RoutingGroup)
{
    Std_ReturnType RetVal = E_NOT_OK;

    if ((SoAd_State == SOAD_STATE_ACTIVE) && (RoutingGroup < SOAD_CFG_MAX_ROUTING_GROUPS)) {
        SoAd_RoutingGroupRuntime[RoutingGroup].IsEnabled = TRUE;
        
        /* Enable all PDU routes in this group */
        if (SoAd_ConfigPtr != NULL) {
            uint16 i;
            for (i = 0U; i < SoAd_ConfigPtr->NumPduRoutes; i++) {
                if (SoAd_ConfigPtr->PduRoutes[i].RoutingGroup == RoutingGroup) {
                    SoAd_PduRouteRuntime[SoAd_ConfigPtr->PduRoutes[i].RouteId].IsEnabled = TRUE;
                }
            }
        }
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Disable routing group
 */
Std_ReturnType SoAd_DisableRouting(uint16 RoutingGroup)
{
    Std_ReturnType RetVal = E_NOT_OK;

    if ((SoAd_State == SOAD_STATE_ACTIVE) && (RoutingGroup < SOAD_CFG_MAX_ROUTING_GROUPS)) {
        SoAd_RoutingGroupRuntime[RoutingGroup].IsEnabled = FALSE;
        
        /* Disable all PDU routes in this group */
        if (SoAd_ConfigPtr != NULL) {
            uint16 i;
            for (i = 0U; i < SoAd_ConfigPtr->NumPduRoutes; i++) {
                if (SoAd_ConfigPtr->PduRoutes[i].RoutingGroup == RoutingGroup) {
                    SoAd_PduRouteRuntime[SoAd_ConfigPtr->PduRoutes[i].RouteId].IsEnabled = FALSE;
                }
            }
        }
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Open routing group
 */
Std_ReturnType SoAd_OpenRoutingGroup(uint16 RoutingGroup)
{
    Std_ReturnType RetVal = E_NOT_OK;

    if ((SoAd_State == SOAD_STATE_ACTIVE) && (RoutingGroup < SOAD_CFG_MAX_ROUTING_GROUPS)) {
        SoAd_RoutingGroupRuntime[RoutingGroup].IsOpen = TRUE;
        
        /* Open all socket connections in this group */
        if (SoAd_ConfigPtr != NULL) {
            uint16 i;
            for (i = 0U; i < SoAd_ConfigPtr->NumSocketConnections; i++) {
                if (SoAd_ConfigPtr->SocketConnections[i].RoutingGroup == RoutingGroup) {
                    (void)SoAd_OpenSoCon(SoAd_ConfigPtr->SocketConnections[i].SoConId);
                }
            }
        }
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Close routing group
 */
Std_ReturnType SoAd_CloseRoutingGroup(uint16 RoutingGroup)
{
    Std_ReturnType RetVal = E_NOT_OK;

    if ((SoAd_State == SOAD_STATE_ACTIVE) && (RoutingGroup < SOAD_CFG_MAX_ROUTING_GROUPS)) {
        SoAd_RoutingGroupRuntime[RoutingGroup].IsOpen = FALSE;
        
        /* Close all socket connections in this group */
        if (SoAd_ConfigPtr != NULL) {
            uint16 i;
            for (i = 0U; i < SoAd_ConfigPtr->NumSocketConnections; i++) {
                if (SoAd_ConfigPtr->SocketConnections[i].RoutingGroup == RoutingGroup) {
                    (void)SoAd_CloseSoCon(SoAd_ConfigPtr->SocketConnections[i].SoConId, FALSE);
                }
            }
        }
        RetVal = E_OK;
    }

    return RetVal;
}

/**
 * @brief Set PDU routing status
 */
Std_ReturnType SoAd_SetPduRoutingStatus(uint16 PduId, boolean Status)
{
    Std_ReturnType RetVal = E_NOT_OK;
    SoAd_PduRouteIdType RouteId;

    RouteId = SoAd_FindPduRouteByTxPduId(PduId);
    if ((RouteId != SOAD_INVALID_PDU_ROUTE_ID) && (RouteId < SOAD_CFG_MAX_PDU_ROUTES)) {
        SoAd_PduRouteRuntime[RouteId].IsEnabled = Status;
        RetVal = E_OK;
    }

    return RetVal;
}

/******************************************************************************
 * Upper Layer Callback Functions (implemented by PduR)
 ******************************************************************************/
__attribute__((weak)) void SoAd_IfTxConfirmation(uint16 TxPduId)
{
    (void)TxPduId;
}

__attribute__((weak)) void SoAd_IfRxIndication(uint16 RxPduId, const SoAd_PduInfoType *PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
}

__attribute__((weak)) void SoAd_TpTxConfirmation(uint16 TxPduId, Std_ReturnType Result)
{
    (void)TxPduId;
    (void)Result;
}

__attribute__((weak)) Std_ReturnType SoAd_TpStartOfReception(uint16 RxPduId,
                                                               uint16 TpSduLength,
                                                               uint16 *BufferSizePtr)
{
    (void)RxPduId;
    (void)TpSduLength;
    (void)BufferSizePtr;
    return E_NOT_OK;
}

__attribute__((weak)) Std_ReturnType SoAd_TpCopyRxData(uint16 RxPduId,
                                                        const SoAd_PduInfoType *PduInfoPtr,
                                                        uint16 *BufferSizePtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
    (void)BufferSizePtr;
    return E_NOT_OK;
}

__attribute__((weak)) Std_ReturnType SoAd_TpCopyTxData(uint16 TxPduId,
                                                        const SoAd_PduInfoType *PduInfoPtr,
                                                        const uint8 *RetryInfoPtr,
                                                        uint16 *AvailableDataPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    (void)RetryInfoPtr;
    (void)AvailableDataPtr;
    return E_NOT_OK;
}

__attribute__((weak)) void SoAd_SoConModeChg(SoAd_SoConIdType SoConId, SoAd_SoConStateType NewMode)
{
    (void)SoConId;
    (void)NewMode;
}

/******************************************************************************
 * TcpIp Callback Functions
 ******************************************************************************/

/**
 * @brief TCP Accepted callback
 */
void SoAd_TcpAccepted(tcpip_socket_id_t SocketId, const tcpip_sockaddr_t *RemoteAddr)
{
    uint16 i;

    /* Find socket connection by TcpIp socket ID */
    for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
        if (SoAd_SoConRuntime[i].TcpIpSocketId == SocketId) {
            SoAd_SoConRuntime[i].RemoteAddr.addr = RemoteAddr->addr;
            SoAd_SoConRuntime[i].RemoteAddr.port = RemoteAddr->port;
            SoAd_UpdateSocketState((SoAd_SoConIdType)i, SOAD_SOCON_CONNECTED);
            break;
        }
    }
}

/**
 * @brief TCP Connected callback
 */
void SoAd_TcpConnected(tcpip_socket_id_t SocketId, tcpip_error_t Result)
{
    uint16 i;

    for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
        if (SoAd_SoConRuntime[i].TcpIpSocketId == SocketId) {
            if (Result == TCPIP_OK) {
                SoAd_UpdateSocketState((SoAd_SoConIdType)i, SOAD_SOCON_CONNECTED);
            } else {
                SoAd_UpdateSocketState((SoAd_SoConIdType)i, SOAD_SOCON_ERROR);
            }
            break;
        }
    }
}

/**
 * @brief TcpIp Event callback
 */
void SoAd_TcpIpEvent(tcpip_socket_id_t SocketId, uint8 Event, void *UserData)
{
    (void)UserData;

    switch (Event) {
        case TCPIP_SOCK_EVT_CONNECTED:
            SoAd_TcpConnected(SocketId, TCPIP_OK);
            break;

        case TCPIP_SOCK_EVT_DISCONNECTED:
        {
            uint16 i;
            for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
                if (SoAd_SoConRuntime[i].TcpIpSocketId == SocketId) {
                    SoAd_UpdateSocketState((SoAd_SoConIdType)i, SOAD_SOCON_CLOSED);
                    break;
                }
            }
            break;
        }

        case TCPIP_SOCK_EVT_DATA_RECEIVED:
            /* Data received - will be processed in MainFunction */
            break;

        case TCPIP_SOCK_EVT_ERROR:
        {
            uint16 i;
            for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
                if (SoAd_SoConRuntime[i].TcpIpSocketId == SocketId) {
                    SoAd_UpdateSocketState((SoAd_SoConIdType)i, SOAD_SOCON_ERROR);
                    break;
                }
            }
            break;
        }

        default:
            /* Do nothing */
            break;
    }
}

/**
 * @brief UDP Receive callback
 */
void SoAd_UdpRecv(tcpip_socket_id_t SocketId, 
                  const uint8 *Data, 
                  uint16 Length,
                  const tcpip_sockaddr_t *RemoteAddr)
{
    uint16 i;
    SoAd_PduRouteIdType RouteId;
    SoAd_PduInfoType PduInfo;

    for (i = 0U; i < SOAD_CFG_MAX_SOCKET_CONNECTIONS; i++) {
        if (SoAd_SoConRuntime[i].TcpIpSocketId == SocketId) {
            RouteId = SoAd_FindPduRouteBySoConId((SoAd_SoConIdType)i, TRUE);
            if (RouteId != SOAD_INVALID_PDU_ROUTE_ID) {
                PduInfo.SduDataPtr = (uint8 *)Data;
                PduInfo.SduLength = Length;
                PduInfo.RouteId = RouteId;

                if (SoAd_IsRoutingEnabled(RouteId)) {
#if (SOAD_UL_IF_RX_INDICATION == STD_ON)
                    SoAd_IfRxIndication(SoAd_ConfigPtr->PduRoutes[RouteId].RxPduId, &PduInfo);
#endif
                }
            }
            break;
        }
    }

    (void)RemoteAddr;
}

/******************************************************************************
 * Tx Confirmation from Lower Layer
 ******************************************************************************/

void SoAd_TxConfirmation(uint16 TxPduId, Std_ReturnType Result)
{
    (void)TxPduId;
    (void)Result;
}

void SoAd_RxIndication(uint16 RxPduId, const SoAd_PduInfoType *PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
}
