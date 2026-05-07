/**
 * @file PduR.c
 * @brief PDU Router
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : N/A (Service Layer)
     5|* Dependencies         : Com, CanIf, Dcm, MemIf
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-15
    10|* Author               : AI Agent (PduR Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "PduR.h"
    20|#include "PduR_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|
    24|/*==================================================================================================
    25|*                                  LOCAL CONSTANT DEFINITIONS
    26|==================================================================================================*/
    27|#define PDUR_INSTANCE_ID                (0x00U)
    28|
    29|/* Development error codes */
    30|#define PDUR_E_PARAM_POINTER            (0x01U)
    31|#define PDUR_E_INVALID_PDU_ID           (0x02U)
    32|#define PDUR_E_UNINIT                   (0x03U)
    33|#define PDUR_E_INVALID_REQUEST          (0x04U)
    34|#define PDUR_E_ROUTING_PATH_NOT_FOUND   (0x05U)
    35|#define PDUR_E_FIFO_FULL                (0x06U)
    36|
    37|/* Module state */
    38|#define PDUR_STATE_UNINIT               (0x00U)
    39|#define PDUR_STATE_INIT                 (0x01U)
    40|
    41|/*==================================================================================================
    42|*                                  LOCAL MACRO DEFINITIONS
    43|==================================================================================================*/
    44|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    45|    #define PDUR_DET_REPORT_ERROR(ApiId, ErrorId) \
    46|        Det_ReportError(PDUR_MODULE_ID, PDUR_INSTANCE_ID, (ApiId), (ErrorId))
    47|#else
    48|    #define PDUR_DET_REPORT_ERROR(ApiId, ErrorId)
    49|#endif
    50|
    51|/*==================================================================================================
    52|*                                  LOCAL TYPE DEFINITIONS
    53|==================================================================================================*/
    54|/* FIFO entry structure for queued routing */
    55|typedef struct
    56|{
    57|    PduInfoType PduInfo;
    58|    uint8 SduData[PDUR_MAX_DESTINATIONS_PER_PATH * 8U]; /* Configurable buffer */
    59|    boolean IsValid;
    60|} PduR_FifoEntryType;
    61|
    62|/* FIFO queue structure */
    63|typedef struct
    64|{
    65|    PduR_FifoEntryType Entries[PDUR_MAX_FIFO_DEPTH];
    66|    uint8 Head;
    67|    uint8 Tail;
    68|    uint8 Count;
    69|} PduR_FifoQueueType;
    70|
    71|/* Routing path runtime state */
    72|typedef struct
    73|{
    74|    PduR_FifoQueueType FifoQueue;
    75|    boolean IsEnabled;
    76|} PduR_RoutingPathStateType;
    77|
    78|/* Module internal state */
    79|typedef struct
    80|{
    81|    uint8 State;
    82|    const PduR_ConfigType* ConfigPtr;
    83|    PduR_RoutingPathStateType PathStates[PDUR_NUMBER_OF_ROUTING_PATHS];
    84|} PduR_InternalStateType;
    85|
    86|/*==================================================================================================
    87|*                                  LOCAL VARIABLE DECLARATIONS
    88|==================================================================================================*/
    89|#define PDUR_START_SEC_VAR_CLEARED_UNSPECIFIED
    90|#include "MemMap.h"
    91|
    92|STATIC PduR_InternalStateType PduR_InternalState;
    93|
    94|#define PDUR_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    95|#include "MemMap.h"
    96|
    97|/*==================================================================================================
    98|*                                  LOCAL FUNCTION PROTOTYPES
    99|==================================================================================================*/
   100|STATIC Std_ReturnType PduR_FindRoutingPath(PduIdType PduId, uint8 ModuleType, uint8* PathIndex);
   101|STATIC Std_ReturnType PduR_RoutePdu(const PduR_RoutingPathConfigType* PathPtr, const PduInfoType* PduInfoPtr);
   102|STATIC Std_ReturnType PduR_RouteToDestination(const PduR_DestPduConfigType* DestPtr, const PduInfoType* PduInfoPtr);
   103|STATIC void PduR_FifoInit(PduR_FifoQueueType* FifoPtr);
   104|STATIC Std_ReturnType PduR_FifoPush(PduR_FifoQueueType* FifoPtr, const PduInfoType* PduInfoPtr);
   105|STATIC Std_ReturnType PduR_FifoPop(PduR_FifoQueueType* FifoPtr, PduInfoType* PduInfoPtr);
   106|STATIC boolean PduR_FifoIsEmpty(const PduR_FifoQueueType* FifoPtr);
   107|STATIC boolean PduR_FifoIsFull(const PduR_FifoQueueType* FifoPtr);
   108|
   109|/*==================================================================================================
   110|*                                      LOCAL FUNCTIONS
   111|==================================================================================================*/
   112|#define PDUR_START_SEC_CODE
   113|#include "MemMap.h"
   114|
   115|/**
   116| * @brief   Find routing path for given PDU ID and module type
   117| * @param   PduId       - PDU identifier
   118| * @param   ModuleType  - Source/Destination module type
   119| * @param   PathIndex   - Output parameter for found path index
   120| * @return  E_OK if path found, E_NOT_OK otherwise
   121| */
   122|STATIC Std_ReturnType PduR_FindRoutingPath(PduIdType PduId, uint8 ModuleType, uint8* PathIndex)
   123|{
   124|    Std_ReturnType result = E_NOT_OK;
   125|    uint8 i;
   126|    const PduR_ConfigType* configPtr = PduR_InternalState.ConfigPtr;
   127|
   128|    if (configPtr != NULL_PTR)
   129|    {
   130|        for (i = 0U; i < configPtr->NumRoutingPaths; i++)
   131|        {
   132|            /* Check if this path matches the source PDU and module */
   133|            if ((configPtr->RoutingPaths[i].SrcPdu.SourcePduId == PduId) &&
   134|                (configPtr->RoutingPaths[i].SrcPdu.SourceModule == ModuleType))
   135|            {
   136|                *PathIndex = i;
   137|                result = E_OK;
   138|                break;
   139|            }
   140|        }
   141|    }
   142|
   143|    return result;
   144|}
   145|
   146|/**
   147| * @brief   Route PDU to all destinations in a routing path
   148| * @param   PathPtr     - Pointer to routing path configuration
   149| * @param   PduInfoPtr  - Pointer to PDU data
   150| * @return  E_OK if routing successful, E_NOT_OK otherwise
   151| */
   152|STATIC Std_ReturnType PduR_RoutePdu(const PduR_RoutingPathConfigType* PathPtr, const PduInfoType* PduInfoPtr)
   153|{
   154|    Std_ReturnType result = E_OK;
   155|    uint8 i;
   156|
   157|    if ((PathPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
   158|    {
   159|        for (i = 0U; i < PathPtr->NumDestPdus; i++)
   160|        {
   161|            if (PduR_RouteToDestination(&PathPtr->DestPdus[i], PduInfoPtr) != E_OK)
   162|            {
   163|                result = E_NOT_OK;
   164|            }
   165|        }
   166|    }
   167|    else
   168|    {
   169|        result = E_NOT_OK;
   170|    }
   171|
   172|    return result;
   173|}
   174|
   175|/**
   176| * @brief   Route PDU to a specific destination
   177| * @param   DestPtr     - Pointer to destination PDU configuration
   178| * @param   PduInfoPtr  - Pointer to PDU data
   179| * @return  E_OK if routing successful, E_NOT_OK otherwise
   180| */
   181|STATIC Std_ReturnType PduR_RouteToDestination(const PduR_DestPduConfigType* DestPtr, const PduInfoType* PduInfoPtr)
   182|{
   183|    Std_ReturnType result = E_NOT_OK;
   184|
   185|    if ((DestPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
   186|    {
   187|        switch (DestPtr->DestModule)
   188|        {
   189|            case PDUR_MODULE_CANIF:
   190|                /* Route to CanIf for transmission */
   191|                result = CanIf_Transmit(DestPtr->DestPduId, PduInfoPtr);
   192|                break;
   193|
   194|            case PDUR_MODULE_COM:
   195|                /* Route to Com (RxIndication) */
   196|                Com_RxIndication(DestPtr->DestPduId, PduInfoPtr);
   197|                result = E_OK;
   198|                break;
   199|
   200|            case PDUR_MODULE_DCM:
   201|                /* Route to Dcm (RxIndication) */
   202|                Dcm_RxIndication(DestPtr->DestPduId, PduInfoPtr);
   203|                result = E_OK;
   204|                break;
   205|
   206|            default:
   207|                /* Unknown destination module */
   208|                result = E_NOT_OK;
   209|                break;
   210|        }
   211|    }
   212|
   213|    return result;
   214|}
   215|
   216|/**
   217| * @brief   Initialize FIFO queue
   218| * @param   FifoPtr - Pointer to FIFO queue
   219| */
   220|STATIC void PduR_FifoInit(PduR_FifoQueueType* FifoPtr)
   221|{
   222|    uint8 i;
   223|
   224|    if (FifoPtr != NULL_PTR)
   225|    {
   226|        FifoPtr->Head = 0U;
   227|        FifoPtr->Tail = 0U;
   228|        FifoPtr->Count = 0U;
   229|
   230|        for (i = 0U; i < PDUR_MAX_FIFO_DEPTH; i++)
   231|        {
   232|            FifoPtr->Entries[i].IsValid = FALSE;
   233|        }
   234|    }
   235|}
   236|
   237|/**
   238| * @brief   Push PDU into FIFO queue
   239| * @param   FifoPtr     - Pointer to FIFO queue
   240| * @param   PduInfoPtr  - Pointer to PDU data
   241| * @return  E_OK if successful, E_NOT_OK if FIFO full
   242| */
   243|STATIC Std_ReturnType PduR_FifoPush(PduR_FifoQueueType* FifoPtr, const PduInfoType* PduInfoPtr)
   244|{
   245|    Std_ReturnType result = E_NOT_OK;
   246|
   247|    if ((FifoPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
   248|    {
   249|        if (!PduR_FifoIsFull(FifoPtr))
   250|        {
   251|            PduR_FifoEntryType* entryPtr = &FifoPtr->Entries[FifoPtr->Tail];
   252|
   253|            /* Copy PDU info */
   254|            entryPtr->PduInfo.SduLength = PduInfoPtr->SduLength;
   255|            entryPtr->PduInfo.MetaDataPtr = NULL_PTR;
   256|
   257|            /* Copy data if present */
   258|            if ((PduInfoPtr->SduDataPtr != NULL_PTR) && (PduInfoPtr->SduLength > 0U))
   259|            {
   260|                uint8 copyLength = (PduInfoPtr->SduLength < (PDUR_MAX_DESTINATIONS_PER_PATH * 8U)) ?
   261|                                   PduInfoPtr->SduLength : (PDUR_MAX_DESTINATIONS_PER_PATH * 8U);
   262|
   263|                (void)memcpy(entryPtr->SduData, PduInfoPtr->SduDataPtr, copyLength);
   264|                entryPtr->PduInfo.SduDataPtr = entryPtr->SduData;
   265|            }
   266|            else
   267|            {
   268|                entryPtr->PduInfo.SduDataPtr = NULL_PTR;
   269|            }
   270|
   271|            entryPtr->IsValid = TRUE;
   272|
   273|            /* Update tail pointer */
   274|            FifoPtr->Tail = (FifoPtr->Tail + 1U) % PDUR_MAX_FIFO_DEPTH;
   275|            FifoPtr->Count++;
   276|
   277|            result = E_OK;
   278|        }
   279|    }
   280|
   281|    return result;
   282|}
   283|
   284|/**
   285| * @brief   Pop PDU from FIFO queue
   286| * @param   FifoPtr     - Pointer to FIFO queue
   287| * @param   PduInfoPtr  - Output pointer for PDU data
   288| * @return  E_OK if successful, E_NOT_OK if FIFO empty
   289| */
   290|STATIC Std_ReturnType PduR_FifoPop(PduR_FifoQueueType* FifoPtr, PduInfoType* PduInfoPtr)
   291|{
   292|    Std_ReturnType result = E_NOT_OK;
   293|
   294|    if ((FifoPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
   295|    {
   296|        if (!PduR_FifoIsEmpty(FifoPtr))
   297|        {
   298|            PduR_FifoEntryType* entryPtr = &FifoPtr->Entries[FifoPtr->Head];
   299|
   300|            if (entryPtr->IsValid)
   301|            {
   302|                /* Copy PDU info */
   303|                PduInfoPtr->SduLength = entryPtr->PduInfo.SduLength;
   304|                PduInfoPtr->SduDataPtr = entryPtr->PduInfo.SduDataPtr;
   305|                PduInfoPtr->MetaDataPtr = NULL_PTR;
   306|
   307|                entryPtr->IsValid = FALSE;
   308|
   309|                /* Update head pointer */
   310|                FifoPtr->Head = (FifoPtr->Head + 1U) % PDUR_MAX_FIFO_DEPTH;
   311|                FifoPtr->Count--;
   312|
   313|                result = E_OK;
   314|            }
   315|        }
   316|    }
   317|
   318|    return result;
   319|}
   320|
   321|/**
   322| * @brief   Check if FIFO queue is empty
   323| * @param   FifoPtr - Pointer to FIFO queue
   324| * @return  TRUE if empty, FALSE otherwise
   325| */
   326|STATIC boolean PduR_FifoIsEmpty(const PduR_FifoQueueType* FifoPtr)
   327|{
   328|    boolean result = TRUE;
   329|
   330|    if (FifoPtr != NULL_PTR)
   331|    {
   332|        result = (FifoPtr->Count == 0U) ? TRUE : FALSE;
   333|    }
   334|
   335|    return result;
   336|}
   337|
   338|/**
   339| * @brief   Check if FIFO queue is full
   340| * @param   FifoPtr - Pointer to FIFO queue
   341| * @return  TRUE if full, FALSE otherwise
   342| */
   343|STATIC boolean PduR_FifoIsFull(const PduR_FifoQueueType* FifoPtr)
   344|{
   345|    boolean result = TRUE;
   346|
   347|    if (FifoPtr != NULL_PTR)
   348|    {
   349|        result = (FifoPtr->Count >= PDUR_MAX_FIFO_DEPTH) ? TRUE : FALSE;
   350|    }
   351|
   352|    return result;
   353|}
   354|
   355|/*==================================================================================================
   356|*                                      GLOBAL FUNCTIONS
   357|==================================================================================================*/
   358|
   359|/**
   360| * @brief   Initializes the PDU Router module
   361| * @param   ConfigPtr - Pointer to configuration structure
   362| * @return  None
   363| */
   364|void PduR_Init(const PduR_ConfigType* ConfigPtr)
   365|{
   366|    uint8 i;
   367|
   368|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   369|    if (ConfigPtr == NULL_PTR)
   370|    {
   371|        PDUR_DET_REPORT_ERROR(0x01U, PDUR_E_PARAM_POINTER);
   372|        return;
   373|    }
   374|#endif
   375|
   376|    /* Store configuration pointer */
   377|    PduR_InternalState.ConfigPtr = ConfigPtr;
   378|
   379|    /* Initialize all routing path states */
   380|    for (i = 0U; i < PDUR_NUMBER_OF_ROUTING_PATHS; i++)
   381|    {
   382|        PduR_InternalState.PathStates[i].IsEnabled = TRUE;
   383|        PduR_FifoInit(&PduR_InternalState.PathStates[i].FifoQueue);
   384|    }
   385|
   386|    /* Set module state to initialized */
   387|    PduR_InternalState.State = PDUR_STATE_INIT;
   388|}
   389|
   390|/**
   391| * @brief   Deinitializes the PDU Router module
   392| * @param   None
   393| * @return  None
   394| */
   395|void PduR_DeInit(void)
   396|{
   397|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   398|    if (PduR_InternalState.State != PDUR_STATE_INIT)
   399|    {
   400|        PDUR_DET_REPORT_ERROR(0x02U, PDUR_E_UNINIT);
   401|        return;
   402|    }
   403|#endif
   404|
   405|    /* Clear configuration pointer */
   406|    PduR_InternalState.ConfigPtr = NULL_PTR;
   407|
   408|    /* Set module state to uninitialized */
   409|    PduR_InternalState.State = PDUR_STATE_UNINIT;
   410|}
   411|
   412|/**
   413| * @brief   Transmits a PDU (downward routing)
   414| * @param   TxPduId     - PDU identifier
   415| * @param   PduInfoPtr  - Pointer to PDU data
   416| * @return  E_OK if transmission successful, E_NOT_OK otherwise
   417| */
   418|Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
   419|{
   420|    Std_ReturnType result = E_NOT_OK;
   421|    uint8 pathIndex;
   422|
   423|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   424|    if (PduR_InternalState.State != PDUR_STATE_INIT)
   425|    {
   426|        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_UNINIT);
   427|        return E_NOT_OK;
   428|    }
   429|
   430|    if (PduInfoPtr == NULL_PTR)
   431|    {
   432|        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_PARAM_POINTER);
   433|        return E_NOT_OK;
   434|    }
   435|#endif
   436|
   437|    /* Find routing path for this TxPduId from COM/DCM module */
   438|    if (PduR_FindRoutingPath(TxPduId, PDUR_MODULE_COM, &pathIndex) == E_OK)
   439|    {
   440|        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
   441|        result = PduR_RoutePdu(pathPtr, PduInfoPtr);
   442|    }
   443|    else if (PduR_FindRoutingPath(TxPduId, PDUR_MODULE_DCM, &pathIndex) == E_OK)
   444|    {
   445|        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
   446|        result = PduR_RoutePdu(pathPtr, PduInfoPtr);
   447|    }
   448|    else
   449|    {
   450|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   451|        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_ROUTING_PATH_NOT_FOUND);
   452|#endif
   453|    }
   454|
   455|    return result;
   456|}
   457|
   458|/**
   459| * @brief   RxIndication callback from lower layer (upward routing)
   460| * @param   RxPduId     - PDU identifier
   461| * @param   PduInfoPtr  - Pointer to PDU data
   462| * @return  None
   463| */
   464|void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
   465|{
   466|    uint8 pathIndex;
   467|
   468|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   469|    if (PduR_InternalState.State != PDUR_STATE_INIT)
   470|    {
   471|        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_UNINIT);
   472|        return;
   473|    }
   474|
   475|    if (PduInfoPtr == NULL_PTR)
   476|    {
   477|        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_PARAM_POINTER);
   478|        return;
   479|    }
   480|#endif
   481|
   482|    /* Find routing path for this RxPduId from CanIf module */
   483|    if (PduR_FindRoutingPath(RxPduId, PDUR_MODULE_CANIF, &pathIndex) == E_OK)
   484|    {
   485|        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
   486|        (void)PduR_RoutePdu(pathPtr, PduInfoPtr);
   487|    }
   488|    else
   489|    {
   490|#if (PDUR_DEV_ERROR_DETECT == STD_ON)
   491|        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_ROUTING_PATH_NOT_FOUND);
   492|#endif
   493|    }
   494|}
   495|
   496|/**
   497| * @brief   TxConfirmation callback from lower layer
   498| * @param   TxPduId - PDU identifier
   499| * @return  None
   500| */
   501|