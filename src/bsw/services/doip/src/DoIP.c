/**
 * @file DoIP.c
 * @brief Diagnostics over IP
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*
     2| * DoIP.c
     3| * Diagnostic over IP Implementation (ISO 13400)
     4| */
     5|
     6|#include "DoIP.h"
     7|#include "DoIP_Cfg.h"
     8|#include <string.h>
     9|
    10|/*==================================================================================================
    11| *                                      LOCAL DEFINES
    12| *=================================================================================================*/
    13|#define DOIP_HEADER_VERSION_POS         0U
    14|#define DOIP_HEADER_INV_VERSION_POS     1U
    15|#define DOIP_HEADER_TYPE_POS            2U
    16|#define DOIP_HEADER_LENGTH_POS          4U
    17|
    18|#define DOIP_ACTIVATION_REQ_SIZE        7U
    19|#define DOIP_ACTIVATION_RES_SIZE        13U
    20|#define DOIP_DIAG_MSG_MIN_SIZE          4U
    21|
    22|/*==================================================================================================
    23| *                                      LOCAL VARIABLES
    24| *=================================================================================================*/
    25|static DoIP_StateType DoIP_InternalState = DOIP_STATE_UNINIT;
    26|static DoIP_ConnectionType DoIP_Connections[DOIP_MAX_CONNECTIONS];
    27|static uint8 DoIP_AnnouncementCount = 0U;
    28|static uint32 DoIP_AnnouncementTimer = 0U;
    29|static uint8 DoIP_TxBuffer[DOIP_MAX_PAYLOAD_LENGTH + DOIP_HEADER_LENGTH];
    30|static uint8 DoIP_RxBuffer[DOIP_MAX_PAYLOAD_LENGTH + DOIP_HEADER_LENGTH];
    31|
    32|/* Vehicle Information (configured via Lcfg) */
    33|extern const uint8 DoIP_Vin[];
    34|extern const uint8 DoIP_Eid[];
    35|extern const uint8 DoIP_Gid[];
    36|extern const uint16 DoIP_EntityLogicalAddress;
    37|
    38|/*==================================================================================================
    39| *                                      LOCAL FUNCTION PROTOTYPES
    40| *=================================================================================================*/
    41|static void DoIP_ProcessUdpMessage(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
    42|static void DoIP_ProcessTcpMessage(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
    43|static Std_ReturnType DoIP_ValidateGenericHeader(const DoIP_GenericHeaderType* Header);
    44|static void DoIP_HandleDiagnosticMessage(uint16 SoConId, const uint8* Data, uint32 Length);
    45|static void DoIP_HandleEntityStatusReq(uint16 SoConId);
    46|static void DoIP_HandlePowerModeReq(uint16 SoConId);
    47|static uint16 DoIP_FindConnection(uint16 SoConId);
    48|static uint16 DoIP_FindFreeConnection(void);
    49|
    50|/*==================================================================================================
    51| *                                      GLOBAL FUNCTIONS
    52| *=================================================================================================*/
    53|
    54|void DoIP_Init(const void* ConfigPtr)
    55|{
    56|    uint8 i;
    57|    
    58|    (void)ConfigPtr;
    59|    
    60|    /* Initialize connections */
    61|    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    62|    {
    63|        DoIP_Connections[i].State = DOIP_CON_STATE_CLOSED;
    64|        DoIP_Connections[i].SourceAddress = 0U;
    65|        DoIP_Connections[i].TargetAddress = 0U;
    66|        DoIP_Connections[i].InactivityTimer = 0U;
    67|        DoIP_Connections[i].SoConId = 0xFFFFU;
    68|        DoIP_Connections[i].RoutingActivated = FALSE;
    69|    }
    70|    
    71|    DoIP_AnnouncementCount = 0U;
    72|    DoIP_AnnouncementTimer = DOIP_CFG_ANNOUNCE_WAIT;
    73|    DoIP_InternalState = DOIP_STATE_ACTIVE;
    74|    
    75|    /* Initialize SoAd connections */
    76|    SoAd_OpenConnection(DOIP_SOCON_UDP_DISCOVERY);
    77|    SoAd_OpenConnection(DOIP_SOCON_TCP_DATA);
    78|}
    79|
    80|void DoIP_DeInit(void)
    81|{
    82|    uint8 i;
    83|    
    84|    /* Close all connections */
    85|    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
    86|    {
    87|        if (DoIP_Connections[i].State != DOIP_CON_STATE_CLOSED)
    88|        {
    89|            DoIP_CloseConnection(DoIP_Connections[i].SoConId);
    90|        }
    91|    }
    92|    
    93|    /* Close SoAd connections */
    94|    SoAd_CloseConnection(DOIP_SOCON_UDP_DISCOVERY);
    95|    SoAd_CloseConnection(DOIP_SOCON_TCP_DATA);
    96|    
    97|    DoIP_InternalState = DOIP_STATE_UNINIT;
    98|}
    99|
   100|void DoIP_MainFunction(void)
   101|{
   102|    uint8 i;
   103|    
   104|    if (DoIP_InternalState != DOIP_STATE_ACTIVE)
   105|    {
   106|        return;
   107|    }
   108|    
   109|    /* Handle vehicle announcement */
   110|    #if (DOIP_VEHICLE_ANNOUNCEMENT == STD_ON)
   111|    if (DoIP_AnnouncementCount < DOIP_CFG_ANNOUNCE_NUM)
   112|    {
   113|        if (DoIP_AnnouncementTimer > 0U)
   114|        {
   115|            DoIP_AnnouncementTimer--;
   116|        }
   117|        else
   118|        {
   119|            DoIP_SendVehicleAnnouncement();
   120|            DoIP_AnnouncementCount++;
   121|            DoIP_AnnouncementTimer = DOIP_CFG_ANNOUNCE_INTERVAL;
   122|        }
   123|    }
   124|    #endif
   125|    
   126|    /* Handle inactivity timers and alive checks */
   127|    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
   128|    {
   129|        if (DoIP_Connections[i].State == DOIP_CON_STATE_ROUTING_ACTIVE)
   130|        {
   131|            if (DoIP_Connections[i].InactivityTimer > 0U)
   132|            {
   133|                DoIP_Connections[i].InactivityTimer--;
   134|            }
   135|            else
   136|            {
   137|                /* Inactivity timeout - send alive check */
   138|                DoIP_SendAliveCheckRequest();
   139|                DoIP_Connections[i].State = DOIP_CON_STATE_ALIVE_CHECK;
   140|                DoIP_Connections[i].InactivityTimer = DOIP_CFG_ALIVE_CHECK_TIMEOUT;
   141|            }
   142|        }
   143|        else if (DoIP_Connections[i].State == DOIP_CON_STATE_ALIVE_CHECK)
   144|        {
   145|            if (DoIP_Connections[i].InactivityTimer > 0U)
   146|            {
   147|                DoIP_Connections[i].InactivityTimer--;
   148|            }
   149|            else
   150|            {
   151|                /* Alive check timeout - close connection */
   152|                DoIP_CloseConnection(DoIP_Connections[i].SoConId);
   153|            }
   154|        }
   155|    }
   156|}
   157|
   158|void DoIP_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
   159|{
   160|    if ((PduInfoPtr == NULL) || (PduInfoPtr->SduDataPtr == NULL))
   161|    {
   162|        return;
   163|    }
   164|    
   165|    if (RxPduId == DOIP_PDU_UDP_RX)
   166|    {
   167|        DoIP_ProcessUdpMessage(RxPduId, PduInfoPtr);
   168|    }
   169|    else if (RxPduId == DOIP_PDU_TCP_RX)
   170|    {
   171|        DoIP_ProcessTcpMessage(RxPduId, PduInfoPtr);
   172|    }
   173|}
   174|
   175|void DoIP_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
   176|{
   177|    (void)TxPduId;
   178|    (void)result;
   179|}
   180|
   181|/*==================================================================================================
   182| *                                      VEHICLE ANNOUNCEMENT FUNCTIONS
   183| *=================================================================================================*/
   184|void DoIP_SendVehicleAnnouncement(void)
   185|{
   186|    PduInfoType pduInfo;
   187|    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
   188|    uint8 i = 0U;
   189|    
   190|    /* Build Generic Header */
   191|    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_VEHICLE_ANNOUNCE, 
   192|                           DOIP_VIN_LENGTH + 2U + DOIP_EID_LENGTH + DOIP_GID_LENGTH + 1U);
   193|    
   194|    /* VIN */
   195|    memcpy(&payload[i], DoIP_Vin, DOIP_VIN_LENGTH);
   196|    i += DOIP_VIN_LENGTH;
   197|    
   198|    /* Logical Address */
   199|    payload[i++] = (uint8)(DoIP_EntityLogicalAddress >> 8);
   200|    payload[i++] = (uint8)(DoIP_EntityLogicalAddress & 0xFFU);
   201|    
   202|    /* EID */
   203|    memcpy(&payload[i], DoIP_Eid, DOIP_EID_LENGTH);
   204|    i += DOIP_EID_LENGTH;
   205|    
   206|    /* GID */
   207|    memcpy(&payload[i], DoIP_Gid, DOIP_GID_LENGTH);
   208|    i += DOIP_GID_LENGTH;
   209|    
   210|    /* Further Action */
   211|    payload[i++] = DOIP_FURTHER_ACTION;
   212|    
   213|    /* Send via UDP */
   214|    pduInfo.SduDataPtr = DoIP_TxBuffer;
   215|    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
   216|    SoAd_IfTransmit(DOIP_PDU_UDP_TX, &pduInfo);
   217|}
   218|
   219|void DoIP_ProcessVehicleIdentificationReq(const uint8* Data, uint32 Length)
   220|{
   221|    (void)Data;
   222|    (void)Length;
   223|    
   224|    /* Send immediate vehicle announcement */
   225|    DoIP_SendVehicleAnnouncement();
   226|}
   227|
   228|/*==================================================================================================
   229| *                                      ROUTING ACTIVATION FUNCTIONS
   230| *=================================================================================================*/
   231|Std_ReturnType DoIP_ProcessRoutingActivationReq(uint16 SoConId, const uint8* Data, uint32 Length)
   232|{
   233|    uint16 sourceAddress;
   234|    uint8 activationType;
   235|    uint8 responseCode;
   236|    uint16 connIdx;
   237|    
   238|    if (Length < DOIP_ACTIVATION_REQ_SIZE)
   239|    {
   240|        return E_NOT_OK;
   241|    }
   242|    
   243|    /* Extract source address */
   244|    sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
   245|    activationType = Data[2];
   246|    
   247|    /* Validate source address */
   248|    if (!DoIP_ValidateSourceAddress(sourceAddress))
   249|    {
   250|        responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_UNKNOWN_SA;
   251|    }
   252|    /* Validate activation type */
   253|    else if ((activationType != DOIP_DEFAULT_ACTIVATION_TYPE) &&
   254|             (activationType != DOIP_WWH_OBD_ACTIVATION_TYPE))
   255|    {
   256|        responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED;
   257|    }
   258|    else
   259|    {
   260|        /* Find or create connection */
   261|        connIdx = DoIP_FindConnection(SoConId);
   262|        if (connIdx >= DOIP_MAX_CONNECTIONS)
   263|        {
   264|            connIdx = DoIP_FindFreeConnection();
   265|        }
   266|        
   267|        if (connIdx >= DOIP_MAX_CONNECTIONS)
   268|        {
   269|            /* All sockets in use */
   270|            responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_ALL_SOCKETS_INUSE;
   271|        }
   272|        else
   273|        {
   274|            /* Activate routing */
   275|            DoIP_Connections[connIdx].State = DOIP_CON_STATE_ROUTING_ACTIVE;
   276|            DoIP_Connections[connIdx].SourceAddress = sourceAddress;
   277|            DoIP_Connections[connIdx].SoConId = SoConId;
   278|            DoIP_Connections[connIdx].RoutingActivated = TRUE;
   279|            DoIP_Connections[connIdx].InactivityTimer = DOIP_CFG_GENERAL_INACTIVITY;
   280|            
   281|            responseCode = DOIP_ROUTING_ACTIVATION_RES_CODE_OK;
   282|        }
   283|    }
   284|    
   285|    DoIP_SendRoutingActivationResponse(SoConId, responseCode);
   286|    return E_OK;
   287|}
   288|
   289|void DoIP_SendRoutingActivationResponse(uint16 SoConId, uint8 ResponseCode)
   290|{
   291|    PduInfoType pduInfo;
   292|    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
   293|    uint8 i = 0U;
   294|    uint16 testerAddress = 0U;
   295|    uint16 connIdx;
   296|    
   297|    /* Find connection to get tester address */
   298|    connIdx = DoIP_FindConnection(SoConId);
   299|    if (connIdx < DOIP_MAX_CONNECTIONS)
   300|    {
   301|        testerAddress = DoIP_Connections[connIdx].SourceAddress;
   302|    }
   303|    
   304|    /* Build Generic Header */
   305|    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_ROUTING_ACTIVATION_RES, DOIP_ACTIVATION_RES_SIZE);
   306|    
   307|    /* Tester Logical Address */
   308|    payload[i++] = (uint8)(testerAddress >> 8);
   309|    payload[i++] = (uint8)(testerAddress & 0xFFU);
   310|    
   311|    /* Entity Logical Address */
   312|    payload[i++] = (uint8)(DoIP_EntityLogicalAddress >> 8);
   313|    payload[i++] = (uint8)(DoIP_EntityLogicalAddress & 0xFFU);
   314|    
   315|    /* Response Code */
   316|    payload[i++] = ResponseCode;
   317|    
   318|    /* Reserved (ISO reserved) */
   319|    payload[i++] = 0x00U;
   320|    payload[i++] = 0x00U;
   321|    payload[i++] = 0x00U;
   322|    payload[i++] = 0x00U;
   323|    
   324|    /* OEM Specific (optional) */
   325|    payload[i++] = 0x00U;
   326|    payload[i++] = 0x00U;
   327|    payload[i++] = 0x00U;
   328|    payload[i++] = 0x00U;
   329|    
   330|    /* Send via TCP */
   331|    pduInfo.SduDataPtr = DoIP_TxBuffer;
   332|    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
   333|    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
   334|}
   335|
   336|/*==================================================================================================
   337| *                                      DIAGNOSTIC MESSAGE FUNCTIONS
   338| *=================================================================================================*/
   339|Std_ReturnType DoIP_ProcessDiagnosticMessage(uint16 SoConId, const uint8* Data, uint32 Length)
   340|{
   341|    uint16 sourceAddress;
   342|    uint16 targetAddress;
   343|    uint16 connIdx;
   344|    
   345|    if (Length < DOIP_DIAG_MSG_MIN_SIZE)
   346|    {
   347|        return E_NOT_OK;
   348|    }
   349|    
   350|    /* Extract addresses */
   351|    sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
   352|    targetAddress = ((uint16)Data[2] << 8) | (uint16)Data[3];
   353|    
   354|    /* Validate connection */
   355|    connIdx = DoIP_FindConnection(SoConId);
   356|    if (connIdx >= DOIP_MAX_CONNECTIONS)
   357|    {
   358|        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress, 
   359|                                DOIP_DIAG_NACK_INVALID_SA);
   360|        return E_NOT_OK;
   361|    }
   362|    
   363|    /* Validate source address matches connection */
   364|    if (DoIP_Connections[connIdx].SourceAddress != sourceAddress)
   365|    {
   366|        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress,
   367|                                DOIP_DIAG_NACK_INVALID_SA);
   368|        return E_NOT_OK;
   369|    }
   370|    
   371|    /* Validate target address */
   372|    if (!DoIP_ValidateTargetAddress(targetAddress))
   373|    {
   374|        DoIP_SendDiagnosticNack(SoConId, sourceAddress, targetAddress,
   375|                                DOIP_DIAG_NACK_UNKNOWN_TA);
   376|        return E_NOT_OK;
   377|    }
   378|    
   379|    /* Send positive acknowledgment */
   380|    DoIP_SendDiagnosticAck(SoConId, sourceAddress, targetAddress, 0x00U);
   381|    
   382|    /* Forward to DCM */
   383|    DoIP_UL_RXINDICATION(SoConId, &Data[DOIP_DIAG_MSG_MIN_SIZE], Length - DOIP_DIAG_MSG_MIN_SIZE);
   384|    
   385|    /* Reset inactivity timer */
   386|    DoIP_ResetInactivityTimer(SoConId);
   387|    
   388|    return E_OK;
   389|}
   390|
   391|void DoIP_SendDiagnosticAck(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 AckCode)
   392|{
   393|    PduInfoType pduInfo;
   394|    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
   395|    uint8 i = 0U;
   396|    
   397|    (void)AckCode;
   398|    
   399|    /* Build Generic Header */
   400|    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_DIAGNOSTIC_ACK, 5U);
   401|    
   402|    /* Source Address */
   403|    payload[i++] = (uint8)(SourceAddress >> 8);
   404|    payload[i++] = (uint8)(SourceAddress & 0xFFU);
   405|    
   406|    /* Target Address */
   407|    payload[i++] = (uint8)(TargetAddress >> 8);
   408|    payload[i++] = (uint8)(TargetAddress & 0xFFU);
   409|    
   410|    /* Ack Code */
   411|    payload[i++] = 0x00U;
   412|    
   413|    /* Send via TCP */
   414|    pduInfo.SduDataPtr = DoIP_TxBuffer;
   415|    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
   416|    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
   417|}
   418|
   419|void DoIP_SendDiagnosticNack(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 NackCode)
   420|{
   421|    PduInfoType pduInfo;
   422|    uint8* payload = &DoIP_TxBuffer[DOIP_HEADER_LENGTH];
   423|    uint8 i = 0U;
   424|    
   425|    /* Build Generic Header */
   426|    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_DIAGNOSTIC_NACK, 5U);
   427|    
   428|    /* Source Address */
   429|    payload[i++] = (uint8)(SourceAddress >> 8);
   430|    payload[i++] = (uint8)(SourceAddress & 0xFFU);
   431|    
   432|    /* Target Address */
   433|    payload[i++] = (uint8)(TargetAddress >> 8);
   434|    payload[i++] = (uint8)(TargetAddress & 0xFFU);
   435|    
   436|    /* Nack Code */
   437|    payload[i++] = NackCode;
   438|    
   439|    /* Send via TCP */
   440|    pduInfo.SduDataPtr = DoIP_TxBuffer;
   441|    pduInfo.SduLength = DOIP_HEADER_LENGTH + i;
   442|    SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
   443|}
   444|
   445|/*==================================================================================================
   446| *                                      ALIVE CHECK FUNCTIONS
   447| *=================================================================================================*/
   448|void DoIP_SendAliveCheckRequest(void)
   449|{
   450|    PduInfoType pduInfo;
   451|    uint8 i;
   452|    
   453|    /* Build Generic Header */
   454|    DoIP_BuildGenericHeader(DoIP_TxBuffer, DOIP_PT_ALIVE_CHECK_REQ, 0U);
   455|    
   456|    /* Send to all active connections */
   457|    for (i = 0U; i < DOIP_MAX_CONNECTIONS; i++)
   458|    {
   459|        if (DoIP_Connections[i].State == DOIP_CON_STATE_ROUTING_ACTIVE)
   460|        {
   461|            pduInfo.SduDataPtr = DoIP_TxBuffer;
   462|            pduInfo.SduLength = DOIP_HEADER_LENGTH;
   463|            SoAd_IfTransmit(DOIP_PDU_TCP_TX, &pduInfo);
   464|            DoIP_Connections[i].State = DOIP_CON_STATE_ALIVE_CHECK;
   465|        }
   466|    }
   467|}
   468|
   469|void DoIP_ProcessAliveCheckResponse(uint16 SoConId, const uint8* Data)
   470|{
   471|    uint16 connIdx;
   472|    uint16 sourceAddress;
   473|    
   474|    (void)Data;
   475|    
   476|    connIdx = DoIP_FindConnection(SoConId);
   477|    if (connIdx < DOIP_MAX_CONNECTIONS)
   478|    {
   479|        sourceAddress = ((uint16)Data[0] << 8) | (uint16)Data[1];
   480|        
   481|        if ((DoIP_Connections[connIdx].SourceAddress == sourceAddress) &&
   482|            (DoIP_Connections[connIdx].State == DOIP_CON_STATE_ALIVE_CHECK))
   483|        {
   484|            DoIP_Connections[connIdx].State = DOIP_CON_STATE_ROUTING_ACTIVE;
   485|            DoIP_Connections[connIdx].InactivityTimer = DOIP_CFG_GENERAL_INACTIVITY;
   486|        }
   487|    }
   488|}
   489|
   490|/*==================================================================================================
   491| *                                      CONNECTION MANAGEMENT
   492| *=================================================================================================*/
   493|void DoIP_CloseConnection(uint16 SoConId)
   494|{
   495|    uint16 connIdx;
   496|    
   497|    connIdx = DoIP_FindConnection(SoConId);
   498|    if (connIdx < DOIP_MAX_CONNECTIONS)
   499|    {
   500|        DoIP_Connections[connIdx].State = DOIP_CON_STATE_CLOSED;
   501|