/**
 * @file SoAd.c
 * @brief Socket Adapter
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */
/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : Ethernet
* Dependencies         : TcpIp, PduR, Det
*
* SW Version           : 4.7.0
* Build Version        : YULETECH_AUTOSAR_4.7.0
* Build Date           : 2026-04-29
* Author               : AI Agent (SoAd Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "SoAd.h"
#include "SoAd_Cfg.h"
#include "Det.h"
#include "MemMap.h"
#include <string.h>

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define SOAD_STATE_UNINIT                       (0x00U)
#define SOAD_STATE_INIT                         (0x01U)

/* PDU Header field offsets */
#define SOAD_HEADER_MSG_TYPE_OFFSET             (0U)
#define SOAD_HEADER_MSG_LEN_OFFSET              (1U)
#define SOAD_HEADER_REQUEST_ID_OFFSET           (3U)
#define SOAD_HEADER_PROTOCOL_VER_OFFSET         (7U)
#define SOAD_HEADER_INTERFACE_VER_OFFSET        (8U)
#define SOAD_HEADER_MSG_TYPE_RETURN             (0x80U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    #define SOAD_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(SOAD_MODULE_ID, SOAD_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define SOAD_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#define SOAD_IS_VALID_CON_ID(ConId) \
    (((ConId) < SOAD_NUMBER_OF_CONNECTIONS) ? TRUE : FALSE)

#define SOAD_IS_VALID_SOCK_ID(SockId) \
    (((SockId) < SOAD_NUMBER_OF_SOCKETS) ? TRUE : FALSE)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    uint8 Buffer[SOAD_MAX_PDU_LENGTH + SOAD_MAX_HEADER_LENGTH];
    uint16 Length;
    boolean IsValid;
} SoAd_RxBufferType;

typedef struct {
    uint8 Buffer[SOAD_MAX_PDU_LENGTH + SOAD_MAX_HEADER_LENGTH];
    uint16 Length;
    boolean IsValid;
    boolean IsPending;
} SoAd_TxBufferType;

typedef struct {
    SoAd_ConnStateType State;
    TcpIp_SocketIdType SocketId;
    uint16 ConnGrpId;
    TcpIp_SockAddrType RemoteAddr;
    uint32 ConnectTimeout;
    uint32 DisconnectTimeout;
    boolean CloseRequested;
    boolean AbortRequested;
} SoAd_ConnectionStateType;

typedef struct {
    uint8 State;
    const SoAd_ConfigType* ConfigPtr;
    SoAd_ConnectionStateType ConnStates[SOAD_NUMBER_OF_CONNECTIONS];
    SoAd_RxBufferType RxBuffers[SOAD_NUMBER_OF_CONNECTIONS];
    SoAd_TxBufferType TxBuffers[SOAD_NUMBER_OF_CONNECTIONS];
} SoAd_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC SoAd_InternalStateType SoAd_InternalState;

#define SOAD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType SoAd_FindConnectionBySocket(TcpIp_SocketIdType SocketId, uint16* ConIdPtr);
STATIC Std_ReturnType SoAd_FindConnectionConfig(uint16 SoConId, const SoAd_ConnectionConfigType** ConfigPtr);
STATIC Std_ReturnType SoAd_FindPduRoute(PduIdType PduId, const SoAd_PduRouteConfigType** RoutePtr);
STATIC void SoAd_UpdateConnectionTimeouts(void);
STATIC Std_ReturnType SoAd_ProcessTxBuffer(uint16 SoConId);
STATIC uint16 SoAd_BuildPduHeader(uint8* Buffer, PduIdType PduId, PduLengthType Length);
STATIC Std_ReturnType SoAd_ParsePduHeader(const uint8* Buffer, uint16* HeaderLen, PduIdType* PduId, PduLengthType* PayloadLen);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define SOAD_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find connection ID by socket ID
 */
STATIC Std_ReturnType SoAd_FindConnectionBySocket(TcpIp_SocketIdType SocketId, uint16* ConIdPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 i;

    if (SoAd_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
        {
            if (SoAd_InternalState.ConnStates[i].SocketId == SocketId)
            {
                *ConIdPtr = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find connection configuration by connection ID
 */
STATIC Std_ReturnType SoAd_FindConnectionConfig(uint16 SoConId, const SoAd_ConnectionConfigType** ConfigPtr)
{
    Std_ReturnType result = E_NOT_OK;
    const SoAd_ConfigType* configPtr = SoAd_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (SoConId < configPtr->NumConnectionConfigs))
    {
        *ConfigPtr = &configPtr->ConnectionConfigs[SoConId];
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Find PDU route by PDU ID
 */
STATIC Std_ReturnType SoAd_FindPduRoute(PduIdType PduId, const SoAd_PduRouteConfigType** RoutePtr)
{
    Std_ReturnType result = E_NOT_OK;
    const SoAd_ConfigType* configPtr = SoAd_InternalState.ConfigPtr;
    uint16 i;

    if (configPtr != NULL_PTR)
    {
        for (i = 0U; i < configPtr->NumPduRouteConfigs; i++)
        {
            if (configPtr->PduRouteConfigs[i].TxPduId == PduId)
            {
                *RoutePtr = &configPtr->PduRouteConfigs[i];
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Update connection timeouts
 */
STATIC void SoAd_UpdateConnectionTimeouts(void)
{
    uint16 i;
    SoAd_ConnectionStateType* connPtr;

    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
    {
        connPtr = &SoAd_InternalState.ConnStates[i];

        if (connPtr->State == SOAD_CONN_STATE_CONNECTING)
        {
            if (connPtr->ConnectTimeout > 0U)
            {
                connPtr->ConnectTimeout--;
                if (connPtr->ConnectTimeout == 0U)
                {
                    /* Timeout - close connection */
                    connPtr->State = SOAD_CONN_STATE_CLOSED;
                    (void)TcpIp_Close(connPtr->SocketId, TRUE);
                }
            }
        }
        else if (connPtr->State == SOAD_CONN_STATE_DISCONNECTING)
        {
            if (connPtr->DisconnectTimeout > 0U)
            {
                connPtr->DisconnectTimeout--;
                if (connPtr->DisconnectTimeout == 0U)
                {
                    /* Timeout - abort connection */
                    connPtr->State = SOAD_CONN_STATE_CLOSED;
                    (void)TcpIp_Close(connPtr->SocketId, TRUE);
                }
            }
        }
    }
}

/**
 * @brief   Process TX buffer for a connection
 */
STATIC Std_ReturnType SoAd_ProcessTxBuffer(uint16 SoConId)
{
    Std_ReturnType result = E_NOT_OK;
    SoAd_TxBufferType* txBufPtr;
    SoAd_ConnectionStateType* connPtr;
    TcpIp_ReturnType sendResult;

    if (SoConId < SOAD_NUMBER_OF_CONNECTIONS)
    {
        txBufPtr = &SoAd_InternalState.TxBuffers[SoConId];
        connPtr = &SoAd_InternalState.ConnStates[SoConId];

        if ((txBufPtr->IsValid) && (connPtr->State == SOAD_CONN_STATE_CONNECTED))
        {
            sendResult = TcpIp_Send(connPtr->SocketId, txBufPtr->Buffer, txBufPtr->Length);

            if (sendResult == TCPIP_OK)
            {
                txBufPtr->IsValid = FALSE;
                txBufPtr->IsPending = FALSE;
                result = E_OK;
            }
            else if (sendResult == TCPIP_E_PHYS_ADDR_MISS)
            {
                txBufPtr->IsPending = TRUE;
            }
        }
    }

    return result;
}

/**
 * @brief   Build PDU header
 */
STATIC uint16 SoAd_BuildPduHeader(uint8* Buffer, PduIdType PduId, PduLengthType Length)
{
    uint16 headerLen = 0U;

#if (SOAD_PDU_HEADER_ENABLE == STD_ON)
    if (Buffer != NULL_PTR)
    {
        /* Message Type */
        Buffer[SOAD_HEADER_MSG_TYPE_OFFSET] = 0x01U; /* REQUEST */
        
        /* Message Length (3 bytes, big-endian) */
        Buffer[SOAD_HEADER_MSG_LEN_OFFSET] = (uint8)((Length >> 16) & 0xFFU);
        Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 1U] = (uint8)((Length >> 8) & 0xFFU);
        Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 2U] = (uint8)(Length & 0xFFU);
        
        /* Request ID (4 bytes) */
        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET] = (uint8)((PduId >> 24) & 0xFFU);
        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 1U] = (uint8)((PduId >> 16) & 0xFFU);
        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 2U] = (uint8)((PduId >> 8) & 0xFFU);
        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 3U] = (uint8)(PduId & 0xFFU);
        
        /* Protocol Version */
        Buffer[SOAD_HEADER_PROTOCOL_VER_OFFSET] = 0x01U;
        
        /* Interface Version */
        Buffer[SOAD_HEADER_INTERFACE_VER_OFFSET] = 0x01U;
        
        headerLen = SOAD_PDU_HEADER_LENGTH;
    }
#else
    (void)Buffer;
    (void)PduId;
    (void)Length;
#endif

    return headerLen;
}

/**
 * @brief   Parse PDU header
 */
STATIC Std_ReturnType SoAd_ParsePduHeader(const uint8* Buffer, uint16* HeaderLen, PduIdType* PduId, PduLengthType* PayloadLen)
{
    Std_ReturnType result = E_NOT_OK;

#if (SOAD_PDU_HEADER_ENABLE == STD_ON)
    if ((Buffer != NULL_PTR) && (HeaderLen != NULL_PTR) && (PduId != NULL_PTR) && (PayloadLen != NULL_PTR))
    {
        /* Parse Message Length (3 bytes, big-endian) */
        *PayloadLen = ((PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET] << 16) |
                      ((PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 1U] << 8) |
                      (PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 2U];
        
        /* Parse Request ID (4 bytes) */
        *PduId = ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET] << 24) |
                 ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 1U] << 16) |
                 ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 2U] << 8) |
                 (PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 3U];
        
        *HeaderLen = SOAD_PDU_HEADER_LENGTH;
        result = E_OK;
    }
#else
    (void)Buffer;
    *HeaderLen = 0U;
    *PduId = 0U;
    *PayloadLen = 0U;
    result = E_OK;
#endif

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the Socket Adapter module
 */
void SoAd_Init(const SoAd_ConfigType* ConfigPtr)
{
    uint16 i;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State == SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_INIT, SOAD_E_ALREADY_INITIALIZED);
        return;
    }
#endif

    if (ConfigPtr == NULL_PTR)
    {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
        SOAD_DET_REPORT_ERROR(SOAD_SID_INIT, SOAD_E_PARAM_POINTER);
#endif
        return;
    }

    /* Store configuration pointer */
    SoAd_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize connection states */
    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
    {
        SoAd_InternalState.ConnStates[i].State = SOAD_CONN_STATE_CLOSED;
        SoAd_InternalState.ConnStates[i].SocketId = TCPIP_SOCKETID_INVALID;
        SoAd_InternalState.ConnStates[i].CloseRequested = FALSE;
        SoAd_InternalState.ConnStates[i].AbortRequested = FALSE;
        SoAd_InternalState.ConnStates[i].ConnectTimeout = 0U;
        SoAd_InternalState.ConnStates[i].DisconnectTimeout = 0U;
    }

    /* Initialize buffers */
    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
    {
        SoAd_InternalState.RxBuffers[i].IsValid = FALSE;
        SoAd_InternalState.RxBuffers[i].Length = 0U;
        SoAd_InternalState.TxBuffers[i].IsValid = FALSE;
        SoAd_InternalState.TxBuffers[i].IsPending = FALSE;
        SoAd_InternalState.TxBuffers[i].Length = 0U;
    }

    /* Set module state to initialized */
    SoAd_InternalState.State = SOAD_STATE_INIT;
}

/**
 * @brief   Deinitializes the Socket Adapter module
 */
void SoAd_DeInit(void)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_DEINIT, SOAD_E_UNINIT);
        return;
    }
#endif

    /* Close all connections */
    (void)SoAd_CloseTcpConnection(0U, TRUE);

    /* Clear configuration pointer */
    SoAd_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    SoAd_InternalState.State = SOAD_STATE_UNINIT;
}

/**
 * @brief   Gets version information
 */
#if (SOAD_VERSION_INFO_API == STD_ON)
void SoAd_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_GETVERSIONINFO, SOAD_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = SOAD_VENDOR_ID;
    versioninfo->moduleID = SOAD_MODULE_ID;
    versioninfo->sw_major_version = SOAD_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SOAD_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SOAD_SW_PATCH_VERSION;
}
#endif

/**
 * @brief   Opens a TCP connection
 */
Std_ReturnType SoAd_OpenTcpConnection(uint16 SoConId)
{
    Std_ReturnType result = E_NOT_OK;
    const SoAd_ConnectionConfigType* connConfig;
    SoAd_ConnectionStateType* connState;
    TcpIp_SocketIdType socketId;
    TcpIp_ReturnType tcpResult;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENTCPCONNECTION, SOAD_E_UNINIT);
        return E_NOT_OK;
    }

    if (!SOAD_IS_VALID_CON_ID(SoConId))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENTCPCONNECTION, SOAD_E_INVALID_CONNID);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];

    if (connState->State != SOAD_CONN_STATE_CLOSED)
    {
        return E_NOT_OK;
    }

    if (SoAd_FindConnectionConfig(SoConId, &connConfig) == E_OK)
    {
        /* Create socket */
        tcpResult = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &socketId);
        
        if (tcpResult == TCPIP_OK)
        {
            connState->SocketId = socketId;
            connState->State = SOAD_CONN_STATE_CONNECTING;
            connState->ConnectTimeout = SOAD_CONNECT_TIMEOUT_MS / SOAD_MAIN_FUNCTION_PERIOD_MS;

            /* Bind to local port if specified */
            if (connConfig->RemotePort > 0U)
            {
                TcpIp_SockAddrType localAddr;
                localAddr.domain = TCPIP_AF_INET;
                localAddr.port = connConfig->RemotePort;
                localAddr.addr[0] = 0U; /* INADDR_ANY */
                
                (void)TcpIp_Bind(socketId, &localAddr);
            }

            result = E_OK;
        }
    }

    return result;
}
/**
 * @brief   Opens a UDP connection
 */
Std_ReturnType SoAd_OpenUdpConnection(uint16 SoConId)
{
    Std_ReturnType result = E_NOT_OK;
    const SoAd_ConnectionConfigType* connConfig;
    SoAd_ConnectionStateType* connState;
    TcpIp_SocketIdType socketId;
    TcpIp_ReturnType udpResult;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENUdpConnection, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if (!SOAD_IS_VALID_CON_ID(SoConId))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENUdpConnection, SOAD_E_INVALID_CONNID);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];

    if (connState->State != SOAD_CONN_STATE_CLOSED)
    {
        return E_NOT_OK;
    }

    if (SoAd_FindConnectionConfig(SoConId, &connConfig) == E_OK)
    {
        udpResult = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &socketId);
        if (udpResult == TCPIP_OK)
        {
            connState->SocketId = socketId;
            connState->State = SOAD_CONN_STATE_CONNECTED;
            connState->ConnectTimeout = SOAD_CONNECT_TIMEOUT_MS / SOAD_MAIN_FUNCTION_PERIOD_MS;

            if (connConfig->RemotePort > 0U)
            {
                TcpIp_SockAddrType localAddr;
                localAddr.domain = TCPIP_AF_INET;
                localAddr.port = connConfig->RemotePort;
                localAddr.addr[0] = 0U;
                (void)TcpIp_Bind(socketId, &localAddr);
            }

            result = E_OK;
        }
    }

    return result;
}

/** @brief Close a TCP connection */
Std_ReturnType SoAd_CloseTcpConnection(uint16 SoConId, boolean Force)
{
    SoAd_ConnectionStateType* connState;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_CLOSETCPConnection, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if (!SOAD_IS_VALID_CON_ID(SoConId))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_CLOSETCPConnection, SOAD_E_INVALID_CONNID);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];

    if (connState->SocketId != TCPIP_SOCKETID_INVALID)
    {
        (void)TcpIp_Close(connState->SocketId, Force);
        connState->SocketId = TCPIP_SOCKETID_INVALID;
    }

    connState->State = SOAD_CONN_STATE_CLOSED;
    (void)Force;
    return E_OK;
}

/** @brief Close a UDP connection */
Std_ReturnType SoAd_CloseUdpConnection(uint16 SoConId)
{
    return SoAd_CloseTcpConnection(SoConId, TRUE);
}

/** @brief Send data over a connection */
Std_ReturnType SoAd_Send(uint16 SoConId, const PduInfoType* PduInfoPtr)
{
    SoAd_ConnectionStateType* connState;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_SEND, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_SEND, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (!SOAD_IS_VALID_CON_ID(SoConId))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_SEND, SOAD_E_INVALID_CONNID);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];

    if ((connState->State != SOAD_CONN_STATE_CONNECTED) &&
        (connState->State != SOAD_CONN_STATE_CONNECTING))
    {
        return E_NOT_OK;
    }

    return (TcpIp_Send(connState->SocketId, PduInfoPtr->SduDataPtr, (uint16)PduInfoPtr->SduLength) == TCPIP_OK) ? E_OK : E_NOT_OK;
}

/** @brief Receive data from a connection */
Std_ReturnType SoAd_Receive(uint16 SoConId, PduInfoType* PduInfoPtr, PduLengthType* Length)
{
    SoAd_ConnectionStateType* connState;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_RECEIVE, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if ((PduInfoPtr == NULL_PTR) || (Length == NULL_PTR))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_RECEIVE, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];
    if (connState->State != SOAD_CONN_STATE_CONNECTED)
    {
        return E_NOT_OK;
    }

    {
        uint16 recvLen = 0U;
        TcpIp_ReturnType ret = TcpIp_Receive(connState->SocketId,
            PduInfoPtr->SduDataPtr, (uint16)PduInfoPtr->SduLength, &recvLen);
        *Length = (PduLengthType)recvLen;
        return (ret == TCPIP_OK) ? E_OK : E_NOT_OK;
    }
}

/** @brief Get remote address of a connection */
Std_ReturnType SoAd_GetRemoteAddr(uint16 SoConId, TcpIp_SockAddrType* IpAddrPtr, uint16* PortPtr)
{
    SoAd_ConnectionStateType* connState;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_GETREMOTESADDR, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if ((IpAddrPtr == NULL_PTR) || (PortPtr == NULL_PTR))
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_GETREMOTESADDR, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];
    *IpAddrPtr = connState->RemoteAddr;
    *PortPtr = connState->RemoteAddr.port;
    return E_OK;
}

/** @brief Set remote address of a connection */
Std_ReturnType SoAd_SetRemoteAddr(uint16 SoConId, const TcpIp_SockAddrType* IpAddrPtr)
{
    SoAd_ConnectionStateType* connState;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_SETREMOTESADDR, SOAD_E_UNINIT);
        return E_NOT_OK;
    }
    if (IpAddrPtr == NULL_PTR)
    {
        SOAD_DET_REPORT_ERROR(SOAD_SID_SETREMOTESADDR, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    connState = &SoAd_InternalState.ConnStates[SoConId];
    connState->RemoteAddr = *IpAddrPtr;
    return E_OK;
}

/** @brief Release IP address assignment */
Std_ReturnType SoAd_ReleaseIpAddrAssignment(uint16 LocalAddrId)
{
    (void)LocalAddrId;
    return E_OK;
}

/** @brief Request IP address assignment */
Std_ReturnType SoAd_RequestIpAddrAssignment(uint16 LocalAddrId, TcpIp_IpAddrAssignmentType Type)
{
    (void)LocalAddrId;
    (void)Type;
    return E_OK;
}

/** @brief Request connection mode change */
Std_ReturnType SoAd_RequestConnMode(uint16 SoConId, SoAd_ConnModeRequestType Mode)
{
    (void)SoConId;
    (void)Mode;
    return E_NOT_OK;
}

/** @brief Main function for periodic processing */
void SoAd_MainFunction(void)
{
    if (SoAd_InternalState.State != SOAD_STATE_INIT)
    {
        return;
    }

    SoAd_UpdateConnectionTimeouts();
}

/** @brief RxIndication callback from TcpIp */
void SoAd_RxIndication(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddrPtr,
                       const uint8* BufPtr, uint16 Length)
{
    uint16 conId;
    SoAd_ConnectionStateType* connState;

    (void)RemoteAddrPtr;
    (void)BufPtr;
    (void)Length;

    if (SoAd_FindConnectionBySocket(SocketId, &conId) == E_OK)
    {
        connState = &SoAd_InternalState.ConnStates[conId];
        if (connState->State == SOAD_CONN_STATE_CONNECTED)
        {
        }
    }
}

/** @brief TxConfirmation callback from TcpIp */
void SoAd_TxConfirmation(TcpIp_SocketIdType SocketId, uint16 Length)
{
    (void)SocketId;
    (void)Length;
}

/** @brief TcpIp event callback */
void SoAd_TcpIpEvent(TcpIp_SocketIdType SocketId, TcpIp_EventType Event,
                     TcpIp_ReturnType EventStatus)
{
    (void)SocketId;
    (void)Event;
    (void)EventStatus;
}

/** @brief IP address assignment changed callback */
void SoAd_LocalIpAddrAssignmentChg(uint16 LocalAddrId, TcpIp_IpAddrStateType State)
{
    (void)LocalAddrId;
    (void)State;
}

/* For backward compatibility with test code: */
void SoAd_TriggerTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
}

#define SOAD_STOP_SEC_CODE
#include "MemMap.h"
