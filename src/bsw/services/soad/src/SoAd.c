/**
 * @file SoAd.c
 * @brief Socket Adapter
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : Ethernet
     5|* Dependencies         : TcpIp, PduR, Det
     6|*
     7|* SW Version           : 4.7.0
     8|* Build Version        : YULETECH_AUTOSAR_4.7.0
     9|* Build Date           : 2026-04-29
    10|* Author               : AI Agent (SoAd Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "SoAd.h"
    20|#include "SoAd_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|#include <string.h>
    24|
    25|/*==================================================================================================
    26|*                                  LOCAL CONSTANT DEFINITIONS
    27|==================================================================================================*/
    28|#define SOAD_STATE_UNINIT                       (0x00U)
    29|#define SOAD_STATE_INIT                         (0x01U)
    30|
    31|/* PDU Header field offsets */
    32|#define SOAD_HEADER_MSG_TYPE_OFFSET             (0U)
    33|#define SOAD_HEADER_MSG_LEN_OFFSET              (1U)
    34|#define SOAD_HEADER_REQUEST_ID_OFFSET           (3U)
    35|#define SOAD_HEADER_PROTOCOL_VER_OFFSET         (7U)
    36|#define SOAD_HEADER_INTERFACE_VER_OFFSET        (8U)
    37|#define SOAD_HEADER_MSG_TYPE_RETURN             (0x80U)
    38|
    39|/*==================================================================================================
    40|*                                  LOCAL MACRO DEFINITIONS
    41|==================================================================================================*/
    42|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    43|    #define SOAD_DET_REPORT_ERROR(ApiId, ErrorId) \
    44|        Det_ReportError(SOAD_MODULE_ID, SOAD_INSTANCE_ID, (ApiId), (ErrorId))
    45|#else
    46|    #define SOAD_DET_REPORT_ERROR(ApiId, ErrorId)
    47|#endif
    48|
    49|#define SOAD_IS_VALID_CON_ID(ConId) \
    50|    (((ConId) < SOAD_NUMBER_OF_CONNECTIONS) ? TRUE : FALSE)
    51|
    52|#define SOAD_IS_VALID_SOCK_ID(SockId) \
    53|    (((SockId) < SOAD_NUMBER_OF_SOCKETS) ? TRUE : FALSE)
    54|
    55|/*==================================================================================================
    56|*                                  LOCAL TYPE DEFINITIONS
    57|==================================================================================================*/
    58|typedef struct {
    59|    uint8 Buffer[SOAD_MAX_PDU_LENGTH + SOAD_MAX_HEADER_LENGTH];
    60|    uint16 Length;
    61|    boolean IsValid;
    62|} SoAd_RxBufferType;
    63|
    64|typedef struct {
    65|    uint8 Buffer[SOAD_MAX_PDU_LENGTH + SOAD_MAX_HEADER_LENGTH];
    66|    uint16 Length;
    67|    boolean IsValid;
    68|    boolean IsPending;
    69|} SoAd_TxBufferType;
    70|
    71|typedef struct {
    72|    SoAd_ConnStateType State;
    73|    TcpIp_SocketIdType SocketId;
    74|    uint16 ConnGrpId;
    75|    TcpIp_SockAddrType RemoteAddr;
    76|    uint32 ConnectTimeout;
    77|    uint32 DisconnectTimeout;
    78|    boolean CloseRequested;
    79|    boolean AbortRequested;
    80|} SoAd_ConnectionStateType;
    81|
    82|typedef struct {
    83|    uint8 State;
    84|    const SoAd_ConfigType* ConfigPtr;
    85|    SoAd_ConnectionStateType ConnStates[SOAD_NUMBER_OF_CONNECTIONS];
    86|    SoAd_RxBufferType RxBuffers[SOAD_NUMBER_OF_CONNECTIONS];
    87|    SoAd_TxBufferType TxBuffers[SOAD_NUMBER_OF_CONNECTIONS];
    88|} SoAd_InternalStateType;
    89|
    90|/*==================================================================================================
    91|*                                  LOCAL VARIABLE DECLARATIONS
    92|==================================================================================================*/
    93|#define SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED
    94|#include "MemMap.h"
    95|
    96|STATIC SoAd_InternalStateType SoAd_InternalState;
    97|
    98|#define SOAD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    99|#include "MemMap.h"
   100|
   101|/*==================================================================================================
   102|*                                  LOCAL FUNCTION PROTOTYPES
   103|==================================================================================================*/
   104|STATIC Std_ReturnType SoAd_FindConnectionBySocket(TcpIp_SocketIdType SocketId, uint16* ConIdPtr);
   105|STATIC Std_ReturnType SoAd_FindConnectionConfig(uint16 SoConId, const SoAd_ConnectionConfigType** ConfigPtr);
   106|STATIC Std_ReturnType SoAd_FindPduRoute(PduIdType PduId, const SoAd_PduRouteConfigType** RoutePtr);
   107|STATIC void SoAd_UpdateConnectionTimeouts(void);
   108|STATIC Std_ReturnType SoAd_ProcessTxBuffer(uint16 SoConId);
   109|STATIC uint16 SoAd_BuildPduHeader(uint8* Buffer, PduIdType PduId, PduLengthType Length);
   110|STATIC Std_ReturnType SoAd_ParsePduHeader(const uint8* Buffer, uint16* HeaderLen, PduIdType* PduId, PduLengthType* PayloadLen);
   111|
   112|/*==================================================================================================
   113|*                                      LOCAL FUNCTIONS
   114|==================================================================================================*/
   115|#define SOAD_START_SEC_CODE
   116|#include "MemMap.h"
   117|
   118|/**
   119| * @brief   Find connection ID by socket ID
   120| */
   121|STATIC Std_ReturnType SoAd_FindConnectionBySocket(TcpIp_SocketIdType SocketId, uint16* ConIdPtr)
   122|{
   123|    Std_ReturnType result = E_NOT_OK;
   124|    uint16 i;
   125|
   126|    if (SoAd_InternalState.ConfigPtr != NULL_PTR)
   127|    {
   128|        for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
   129|        {
   130|            if (SoAd_InternalState.ConnStates[i].SocketId == SocketId)
   131|            {
   132|                *ConIdPtr = i;
   133|                result = E_OK;
   134|                break;
   135|            }
   136|        }
   137|    }
   138|
   139|    return result;
   140|}
   141|
   142|/**
   143| * @brief   Find connection configuration by connection ID
   144| */
   145|STATIC Std_ReturnType SoAd_FindConnectionConfig(uint16 SoConId, const SoAd_ConnectionConfigType** ConfigPtr)
   146|{
   147|    Std_ReturnType result = E_NOT_OK;
   148|    const SoAd_ConfigType* configPtr = SoAd_InternalState.ConfigPtr;
   149|
   150|    if ((configPtr != NULL_PTR) && (SoConId < configPtr->NumConnectionConfigs))
   151|    {
   152|        *ConfigPtr = &configPtr->ConnectionConfigs[SoConId];
   153|        result = E_OK;
   154|    }
   155|
   156|    return result;
   157|}
   158|
   159|/**
   160| * @brief   Find PDU route by PDU ID
   161| */
   162|STATIC Std_ReturnType SoAd_FindPduRoute(PduIdType PduId, const SoAd_PduRouteConfigType** RoutePtr)
   163|{
   164|    Std_ReturnType result = E_NOT_OK;
   165|    const SoAd_ConfigType* configPtr = SoAd_InternalState.ConfigPtr;
   166|    uint16 i;
   167|
   168|    if (configPtr != NULL_PTR)
   169|    {
   170|        for (i = 0U; i < configPtr->NumPduRouteConfigs; i++)
   171|        {
   172|            if (configPtr->PduRouteConfigs[i].TxPduId == PduId)
   173|            {
   174|                *RoutePtr = &configPtr->PduRouteConfigs[i];
   175|                result = E_OK;
   176|                break;
   177|            }
   178|        }
   179|    }
   180|
   181|    return result;
   182|}
   183|
   184|/**
   185| * @brief   Update connection timeouts
   186| */
   187|STATIC void SoAd_UpdateConnectionTimeouts(void)
   188|{
   189|    uint16 i;
   190|    SoAd_ConnectionStateType* connPtr;
   191|
   192|    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
   193|    {
   194|        connPtr = &SoAd_InternalState.ConnStates[i];
   195|
   196|        if (connPtr->State == SOAD_CONN_STATE_CONNECTING)
   197|        {
   198|            if (connPtr->ConnectTimeout > 0U)
   199|            {
   200|                connPtr->ConnectTimeout--;
   201|                if (connPtr->ConnectTimeout == 0U)
   202|                {
   203|                    /* Timeout - close connection */
   204|                    connPtr->State = SOAD_CONN_STATE_CLOSED;
   205|                    (void)TcpIp_Close(connPtr->SocketId, TRUE);
   206|                }
   207|            }
   208|        }
   209|        else if (connPtr->State == SOAD_CONN_STATE_DISCONNECTING)
   210|        {
   211|            if (connPtr->DisconnectTimeout > 0U)
   212|            {
   213|                connPtr->DisconnectTimeout--;
   214|                if (connPtr->DisconnectTimeout == 0U)
   215|                {
   216|                    /* Timeout - abort connection */
   217|                    connPtr->State = SOAD_CONN_STATE_CLOSED;
   218|                    (void)TcpIp_Close(connPtr->SocketId, TRUE);
   219|                }
   220|            }
   221|        }
   222|    }
   223|}
   224|
   225|/**
   226| * @brief   Process TX buffer for a connection
   227| */
   228|STATIC Std_ReturnType SoAd_ProcessTxBuffer(uint16 SoConId)
   229|{
   230|    Std_ReturnType result = E_NOT_OK;
   231|    SoAd_TxBufferType* txBufPtr;
   232|    SoAd_ConnectionStateType* connPtr;
   233|    TcpIp_ReturnType sendResult;
   234|
   235|    if (SoConId < SOAD_NUMBER_OF_CONNECTIONS)
   236|    {
   237|        txBufPtr = &SoAd_InternalState.TxBuffers[SoConId];
   238|        connPtr = &SoAd_InternalState.ConnStates[SoConId];
   239|
   240|        if ((txBufPtr->IsValid) && (connPtr->State == SOAD_CONN_STATE_CONNECTED))
   241|        {
   242|            sendResult = TcpIp_Send(connPtr->SocketId, txBufPtr->Buffer, txBufPtr->Length);
   243|
   244|            if (sendResult == TCPIP_OK)
   245|            {
   246|                txBufPtr->IsValid = FALSE;
   247|                txBufPtr->IsPending = FALSE;
   248|                result = E_OK;
   249|            }
   250|            else if (sendResult == TCPIP_E_PHYS_ADDR_MISS)
   251|            {
   252|                txBufPtr->IsPending = TRUE;
   253|            }
   254|        }
   255|    }
   256|
   257|    return result;
   258|}
   259|
   260|/**
   261| * @brief   Build PDU header
   262| */
   263|STATIC uint16 SoAd_BuildPduHeader(uint8* Buffer, PduIdType PduId, PduLengthType Length)
   264|{
   265|    uint16 headerLen = 0U;
   266|
   267|#if (SOAD_PDU_HEADER_ENABLE == STD_ON)
   268|    if (Buffer != NULL_PTR)
   269|    {
   270|        /* Message Type */
   271|        Buffer[SOAD_HEADER_MSG_TYPE_OFFSET] = 0x01U; /* REQUEST */
   272|        
   273|        /* Message Length (3 bytes, big-endian) */
   274|        Buffer[SOAD_HEADER_MSG_LEN_OFFSET] = (uint8)((Length >> 16) & 0xFFU);
   275|        Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 1U] = (uint8)((Length >> 8) & 0xFFU);
   276|        Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 2U] = (uint8)(Length & 0xFFU);
   277|        
   278|        /* Request ID (4 bytes) */
   279|        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET] = (uint8)((PduId >> 24) & 0xFFU);
   280|        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 1U] = (uint8)((PduId >> 16) & 0xFFU);
   281|        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 2U] = (uint8)((PduId >> 8) & 0xFFU);
   282|        Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 3U] = (uint8)(PduId & 0xFFU);
   283|        
   284|        /* Protocol Version */
   285|        Buffer[SOAD_HEADER_PROTOCOL_VER_OFFSET] = 0x01U;
   286|        
   287|        /* Interface Version */
   288|        Buffer[SOAD_HEADER_INTERFACE_VER_OFFSET] = 0x01U;
   289|        
   290|        headerLen = SOAD_PDU_HEADER_LENGTH;
   291|    }
   292|#else
   293|    (void)Buffer;
   294|    (void)PduId;
   295|    (void)Length;
   296|#endif
   297|
   298|    return headerLen;
   299|}
   300|
   301|/**
   302| * @brief   Parse PDU header
   303| */
   304|STATIC Std_ReturnType SoAd_ParsePduHeader(const uint8* Buffer, uint16* HeaderLen, PduIdType* PduId, PduLengthType* PayloadLen)
   305|{
   306|    Std_ReturnType result = E_NOT_OK;
   307|
   308|#if (SOAD_PDU_HEADER_ENABLE == STD_ON)
   309|    if ((Buffer != NULL_PTR) && (HeaderLen != NULL_PTR) && (PduId != NULL_PTR) && (PayloadLen != NULL_PTR))
   310|    {
   311|        /* Parse Message Length (3 bytes, big-endian) */
   312|        *PayloadLen = ((PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET] << 16) |
   313|                      ((PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 1U] << 8) |
   314|                      (PduLengthType)Buffer[SOAD_HEADER_MSG_LEN_OFFSET + 2U];
   315|        
   316|        /* Parse Request ID (4 bytes) */
   317|        *PduId = ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET] << 24) |
   318|                 ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 1U] << 16) |
   319|                 ((PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 2U] << 8) |
   320|                 (PduIdType)Buffer[SOAD_HEADER_REQUEST_ID_OFFSET + 3U];
   321|        
   322|        *HeaderLen = SOAD_PDU_HEADER_LENGTH;
   323|        result = E_OK;
   324|    }
   325|#else
   326|    (void)Buffer;
   327|    *HeaderLen = 0U;
   328|    *PduId = 0U;
   329|    *PayloadLen = 0U;
   330|    result = E_OK;
   331|#endif
   332|
   333|    return result;
   334|}
   335|
   336|/*==================================================================================================
   337|*                                      GLOBAL FUNCTIONS
   338|==================================================================================================*/
   339|
   340|/**
   341| * @brief   Initializes the Socket Adapter module
   342| */
   343|void SoAd_Init(const SoAd_ConfigType* ConfigPtr)
   344|{
   345|    uint16 i;
   346|
   347|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
   348|    if (SoAd_InternalState.State == SOAD_STATE_INIT)
   349|    {
   350|        SOAD_DET_REPORT_ERROR(SOAD_SID_INIT, SOAD_E_ALREADY_INITIALIZED);
   351|        return;
   352|    }
   353|#endif
   354|
   355|    if (ConfigPtr == NULL_PTR)
   356|    {
   357|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
   358|        SOAD_DET_REPORT_ERROR(SOAD_SID_INIT, SOAD_E_PARAM_POINTER);
   359|#endif
   360|        return;
   361|    }
   362|
   363|    /* Store configuration pointer */
   364|    SoAd_InternalState.ConfigPtr = ConfigPtr;
   365|
   366|    /* Initialize connection states */
   367|    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
   368|    {
   369|        SoAd_InternalState.ConnStates[i].State = SOAD_CONN_STATE_CLOSED;
   370|        SoAd_InternalState.ConnStates[i].SocketId = TCPIP_SOCKETID_INVALID;
   371|        SoAd_InternalState.ConnStates[i].CloseRequested = FALSE;
   372|        SoAd_InternalState.ConnStates[i].AbortRequested = FALSE;
   373|        SoAd_InternalState.ConnStates[i].ConnectTimeout = 0U;
   374|        SoAd_InternalState.ConnStates[i].DisconnectTimeout = 0U;
   375|    }
   376|
   377|    /* Initialize buffers */
   378|    for (i = 0U; i < SOAD_NUMBER_OF_CONNECTIONS; i++)
   379|    {
   380|        SoAd_InternalState.RxBuffers[i].IsValid = FALSE;
   381|        SoAd_InternalState.RxBuffers[i].Length = 0U;
   382|        SoAd_InternalState.TxBuffers[i].IsValid = FALSE;
   383|        SoAd_InternalState.TxBuffers[i].IsPending = FALSE;
   384|        SoAd_InternalState.TxBuffers[i].Length = 0U;
   385|    }
   386|
   387|    /* Set module state to initialized */
   388|    SoAd_InternalState.State = SOAD_STATE_INIT;
   389|}
   390|
   391|/**
   392| * @brief   Deinitializes the Socket Adapter module
   393| */
   394|void SoAd_DeInit(void)
   395|{
   396|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
   397|    if (SoAd_InternalState.State != SOAD_STATE_INIT)
   398|    {
   399|        SOAD_DET_REPORT_ERROR(SOAD_SID_DEINIT, SOAD_E_UNINIT);
   400|        return;
   401|    }
   402|#endif
   403|
   404|    /* Close all connections */
   405|    (void)SoAd_CloseTcpConnection(0U, TRUE);
   406|
   407|    /* Clear configuration pointer */
   408|    SoAd_InternalState.ConfigPtr = NULL_PTR;
   409|
   410|    /* Set module state to uninitialized */
   411|    SoAd_InternalState.State = SOAD_STATE_UNINIT;
   412|}
   413|
   414|/**
   415| * @brief   Gets version information
   416| */
   417|#if (SOAD_VERSION_INFO_API == STD_ON)
   418|void SoAd_GetVersionInfo(Std_VersionInfoType* versioninfo)
   419|{
   420|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
   421|    if (versioninfo == NULL_PTR)
   422|    {
   423|        SOAD_DET_REPORT_ERROR(SOAD_SID_GETVERSIONINFO, SOAD_E_PARAM_POINTER);
   424|        return;
   425|    }
   426|#endif
   427|
   428|    versioninfo->vendorID = SOAD_VENDOR_ID;
   429|    versioninfo->moduleID = SOAD_MODULE_ID;
   430|    versioninfo->sw_major_version = SOAD_SW_MAJOR_VERSION;
   431|    versioninfo->sw_minor_version = SOAD_SW_MINOR_VERSION;
   432|    versioninfo->sw_patch_version = SOAD_SW_PATCH_VERSION;
   433|}
   434|#endif
   435|
   436|/**
   437| * @brief   Opens a TCP connection
   438| */
   439|Std_ReturnType SoAd_OpenTcpConnection(uint16 SoConId)
   440|{
   441|    Std_ReturnType result = E_NOT_OK;
   442|    const SoAd_ConnectionConfigType* connConfig;
   443|    SoAd_ConnectionStateType* connState;
   444|    TcpIp_SocketIdType socketId;
   445|    TcpIp_ReturnType tcpResult;
   446|
   447|#if (SOAD_DEV_ERROR_DETECT == STD_ON)
   448|    if (SoAd_InternalState.State != SOAD_STATE_INIT)
   449|    {
   450|        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENTCPCONNECTION, SOAD_E_UNINIT);
   451|        return E_NOT_OK;
   452|    }
   453|
   454|    if (!SOAD_IS_VALID_CON_ID(SoConId))
   455|    {
   456|        SOAD_DET_REPORT_ERROR(SOAD_SID_OPENTCPCONNECTION, SOAD_E_INVALID_CONNID);
   457|        return E_NOT_OK;
   458|    }
   459|#endif
   460|
   461|    connState = &SoAd_InternalState.ConnStates[SoConId];
   462|
   463|    if (connState->State != SOAD_CONN_STATE_CLOSED)
   464|    {
   465|        return E_NOT_OK;
   466|    }
   467|
   468|    if (SoAd_FindConnectionConfig(SoConId, &connConfig) == E_OK)
   469|    {
   470|        /* Create socket */
   471|        tcpResult = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &socketId);
   472|        
   473|        if (tcpResult == TCPIP_OK)
   474|        {
   475|            connState->SocketId = socketId;
   476|            connState->State = SOAD_CONN_STATE_CONNECTING;
   477|            connState->ConnectTimeout = SOAD_CONNECT_TIMEOUT_MS / SOAD_MAIN_FUNCTION_PERIOD_MS;
   478|
   479|            /* Bind to local port if specified */
   480|            if (connConfig->RemotePort > 0U)
   481|            {
   482|                TcpIp_SockAddrType localAddr;
   483|                localAddr.domain = TCPIP_AF_INET;
   484|                localAddr.port = connConfig->RemotePort;
   485|                localAddr.addr[0] = 0U; /* INADDR_ANY */
   486|                
   487|                (void)TcpIp_Bind(socketId, &localAddr);
   488|            }
   489|
   490|            result = E_OK;
   491|        }
   492|    }
   493|
   494|    return result;
   495|}
   496|
   497|/**
   498| * @brief   Opens a UDP connection
   499| */
   500|Std_ReturnType SoAd_OpenUdpConnection(uint16 SoConId)
   501|