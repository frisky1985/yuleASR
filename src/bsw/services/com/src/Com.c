/**
 * @file Com.c
 * @brief AUTOSAR COM Module
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
     5|* Dependencies         : PduR, RTE
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-15
    10|* Author               : AI Agent (Com Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "Com.h"
    20|#include "Com_Cfg.h"
    21|#include "PduR.h"
    22|#include "Det.h"
    23|#include "MemMap.h"
    24|#include "string.h"
    25|
    26|/*==================================================================================================
    27|*                                  LOCAL CONSTANT DEFINITIONS
    28|==================================================================================================*/
    29|#define COM_INSTANCE_ID                 (0x00U)
    30|
    31|/* Module state */
    32|#define COM_STATE_UNINIT                (0x00U)
    33|#define COM_STATE_INIT                  (0x01U)
    34|
    35|/* Signal update flags */
    36|#define COM_SIGNAL_UPDATED              (0x01U)
    37|#define COM_SIGNAL_NOT_UPDATED          (0x00U)
    38|
    39|/* IPDU transmission states */
    40|#define COM_TX_IDLE                     (0x00U)
    41|#define COM_TX_PENDING                  (0x01U)
    42|#define COM_TX_ACTIVE                   (0x02U)
    43|
    44|/*==================================================================================================
    45|*                                  LOCAL MACRO DEFINITIONS
    46|==================================================================================================*/
    47|#if (COM_DEV_ERROR_DETECT == STD_ON)
    48|    #define COM_DET_REPORT_ERROR(ApiId, ErrorId) \
    49|        Det_ReportError(COM_MODULE_ID, COM_INSTANCE_ID, (ApiId), (ErrorId))
    50|#else
    51|    #define COM_DET_REPORT_ERROR(ApiId, ErrorId)
    52|#endif
    53|
    54|/* Extract bit from byte array */
    55|#define COM_GET_BIT(ByteArray, BitPosition) \
    56|    (((ByteArray)[(BitPosition) / 8U] >> (7U - ((BitPosition) % 8U))) & 0x01U)
    57|
    58|/* Set bit in byte array */
    59|#define COM_SET_BIT(ByteArray, BitPosition, Value) \
    60|    do { \
    61|        uint16 byteIdx = (BitPosition) / 8U; \
    62|        uint8 bitIdx = 7U - ((BitPosition) % 8U); \
    63|        if (Value) { \
    64|            (ByteArray)[byteIdx] |= (1U << bitIdx); \
    65|        } else { \
    66|            (ByteArray)[byteIdx] &= ~(1U << bitIdx); \
    67|        } \
    68|    } while(0)
    69|
    70|/*==================================================================================================
    71|*                                  LOCAL TYPE DEFINITIONS
    72|==================================================================================================*/
    73|/* IPDU runtime state */
    74|typedef struct
    75|{
    76|    uint8 TxState;
    77|    uint8 RepetitionCount;
    78|    uint32 TimeCounter;
    79|    boolean Updated;
    80|    boolean GroupEnabled;
    81|} Com_IPduStateType;
    82|
    83|/* Signal runtime state */
    84|typedef struct
    85|{
    86|    boolean Updated;
    87|    boolean FilterPassed;
    88|    uint32 LastValue;
    89|} Com_SignalStateType;
    90|
    91|/* Module internal state */
    92|typedef struct
    93|{
    94|    uint8 State;
    95|    const Com_ConfigType* ConfigPtr;
    96|    Com_IPduStateType IPduStates[COM_NUM_OF_IPDUS];
    97|    Com_SignalStateType SignalStates[COM_NUM_OF_SIGNALS];
    98|    uint8 IPduBuffer[COM_NUM_OF_IPDUS][COM_MAX_IPDU_BUFFER_SIZE];
    99|    uint8 ShadowBuffer[COM_MAX_IPDU_BUFFER_SIZE];
   100|    Com_IpduGroupVector IPduGroupVector;
   101|} Com_InternalStateType;
   102|
   103|/*==================================================================================================
   104|*                                  LOCAL VARIABLE DECLARATIONS
   105|==================================================================================================*/
   106|#define COM_START_SEC_VAR_CLEARED_UNSPECIFIED
   107|#include "MemMap.h"
   108|
   109|STATIC Com_InternalStateType Com_InternalState;
   110|
   111|#define COM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   112|#include "MemMap.h"
   113|
   114|/*==================================================================================================
   115|*                                  LOCAL FUNCTION PROTOTYPES
   116|==================================================================================================*/
   117|STATIC void Com_PackSignal(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr, uint8* IPduDataPtr);
   118|STATIC void Com_UnpackSignal(const Com_SignalConfigType* SignalPtr, const uint8* IPduDataPtr, void* SignalDataPtr);
   119|STATIC boolean Com_ApplyFilter(const Com_SignalConfigType* SignalPtr, uint32 NewValue);
   120|STATIC uint32 Com_GetSignalValueAsUint32(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr);
   121|STATIC void Com_SetSignalValueFromUint32(const Com_SignalConfigType* SignalPtr, void* SignalDataPtr, uint32 Value);
   122|STATIC Std_ReturnType Com_TransmitIPdu(PduIdType PduId);
   123|STATIC const Com_SignalConfigType* Com_GetSignalConfig(Com_SignalIdType SignalId);
   124|STATIC const Com_IPduConfigType* Com_GetIPduConfig(PduIdType PduId);
   125|
   126|/*==================================================================================================
   127|*                                      LOCAL FUNCTIONS
   128|==================================================================================================*/
   129|#define COM_START_SEC_CODE
   130|#include "MemMap.h"
   131|
   132|/**
   133| * @brief   Pack signal data into IPDU buffer
   134| */
   135|STATIC void Com_PackSignal(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr, uint8* IPduDataPtr)
   136|{
   137|    uint32 value;
   138|    uint16 startByte;
   139|    uint8 startBit;
   140|    uint8 bitSize;
   141|    uint8 i;
   142|
   143|    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR) && (IPduDataPtr != NULL_PTR))
   144|    {
   145|        value = Com_GetSignalValueAsUint32(SignalPtr, SignalDataPtr);
   146|        bitSize = SignalPtr->BitSize;
   147|
   148|        if (SignalPtr->Endianness == COM_LITTLE_ENDIAN)
   149|        {
   150|            startByte = SignalPtr->BitPosition / 8U;
   151|            startBit = SignalPtr->BitPosition % 8U;
   152|
   153|            for (i = 0U; i < bitSize; i++)
   154|            {
   155|                uint16 byteIdx = startByte + ((startBit + i) / 8U);
   156|                uint8 bitIdx = (startBit + i) % 8U;
   157|                uint8 bitValue = (value >> i) & 0x01U;
   158|
   159|                if (bitValue)
   160|                {
   161|                    IPduDataPtr[byteIdx] |= (1U << bitIdx);
   162|                }
   163|                else
   164|                {
   165|                    IPduDataPtr[byteIdx] &= ~(1U << bitIdx);
   166|                }
   167|            }
   168|        }
   169|        else /* COM_BIG_ENDIAN */
   170|        {
   171|            startByte = SignalPtr->BitPosition / 8U;
   172|            startBit = 7U - (SignalPtr->BitPosition % 8U);
   173|
   174|            for (i = 0U; i < bitSize; i++)
   175|            {
   176|                uint16 byteIdx = startByte + ((startBit + i) / 8U);
   177|                uint8 bitIdx = 7U - ((startBit + i) % 8U);
   178|                uint8 bitValue = (value >> (bitSize - 1U - i)) & 0x01U;
   179|
   180|                if (bitValue)
   181|                {
   182|                    IPduDataPtr[byteIdx] |= (1U << bitIdx);
   183|                }
   184|                else
   185|                {
   186|                    IPduDataPtr[byteIdx] &= ~(1U << bitIdx);
   187|                }
   188|            }
   189|        }
   190|    }
   191|}
   192|
   193|/**
   194| * @brief   Unpack signal data from IPDU buffer
   195| */
   196|STATIC void Com_UnpackSignal(const Com_SignalConfigType* SignalPtr, const uint8* IPduDataPtr, void* SignalDataPtr)
   197|{
   198|    uint32 value = 0U;
   199|    uint16 startByte;
   200|    uint8 startBit;
   201|    uint8 bitSize;
   202|    uint8 i;
   203|
   204|    if ((SignalPtr != NULL_PTR) && (IPduDataPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
   205|    {
   206|        bitSize = SignalPtr->BitSize;
   207|
   208|        if (SignalPtr->Endianness == COM_LITTLE_ENDIAN)
   209|        {
   210|            startByte = SignalPtr->BitPosition / 8U;
   211|            startBit = SignalPtr->BitPosition % 8U;
   212|
   213|            for (i = 0U; i < bitSize; i++)
   214|            {
   215|                uint16 byteIdx = startByte + ((startBit + i) / 8U);
   216|                uint8 bitIdx = (startBit + i) % 8U;
   217|                uint8 bitValue = (IPduDataPtr[byteIdx] >> bitIdx) & 0x01U;
   218|
   219|                value |= ((uint32)bitValue << i);
   220|            }
   221|        }
   222|        else /* COM_BIG_ENDIAN */
   223|        {
   224|            startByte = SignalPtr->BitPosition / 8U;
   225|            startBit = 7U - (SignalPtr->BitPosition % 8U);
   226|
   227|            for (i = 0U; i < bitSize; i++)
   228|            {
   229|                uint16 byteIdx = startByte + ((startBit + i) / 8U);
   230|                uint8 bitIdx = 7U - ((startBit + i) % 8U);
   231|                uint8 bitValue = (IPduDataPtr[byteIdx] >> bitIdx) & 0x01U;
   232|
   233|                value |= ((uint32)bitValue << (bitSize - 1U - i));
   234|            }
   235|        }
   236|
   237|        Com_SetSignalValueFromUint32(SignalPtr, SignalDataPtr, value);
   238|    }
   239|}
   240|
   241|/**
   242| * @brief   Apply filter algorithm to signal value
   243| */
   244|STATIC boolean Com_ApplyFilter(const Com_SignalConfigType* SignalPtr, uint32 NewValue)
   245|{
   246|    boolean result = TRUE;
   247|
   248|    if (SignalPtr != NULL_PTR)
   249|    {
   250|        switch (SignalPtr->FilterAlgorithm)
   251|        {
   252|            case COM_ALWAYS:
   253|                result = TRUE;
   254|                break;
   255|
   256|            case COM_NEVER:
   257|                result = FALSE;
   258|                break;
   259|
   260|            case COM_MASKED_NEW_EQUALS_X:
   261|                result = ((NewValue & SignalPtr->FilterMask) == SignalPtr->FilterX);
   262|                break;
   263|
   264|            case COM_MASKED_NEW_DIFFERS_X:
   265|                result = ((NewValue & SignalPtr->FilterMask) != SignalPtr->FilterX);
   266|                break;
   267|
   268|            case COM_MASKED_NEW_DIFFERS_MASKED_OLD:
   269|                result = ((NewValue & SignalPtr->FilterMask) !=
   270|                         (Com_InternalState.SignalStates[SignalPtr->SignalId].LastValue & SignalPtr->FilterMask));
   271|                break;
   272|
   273|            default:
   274|                result = TRUE;
   275|                break;
   276|        }
   277|    }
   278|
   279|    return result;
   280|}
   281|
   282|/**
   283| * @brief   Convert signal data to uint32 for processing
   284| */
   285|STATIC uint32 Com_GetSignalValueAsUint32(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr)
   286|{
   287|    uint32 value = 0U;
   288|    const uint8* dataPtr = (const uint8*)SignalDataPtr;
   289|
   290|    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
   291|    {
   292|        if (SignalPtr->BitSize <= 8U)
   293|        {
   294|            value = (uint32)(*dataPtr);
   295|        }
   296|        else if (SignalPtr->BitSize <= 16U)
   297|        {
   298|            value = (uint32)(*((const uint16*)SignalDataPtr));
   299|        }
   300|        else if (SignalPtr->BitSize <= 32U)
   301|        {
   302|            value = *((const uint32*)SignalDataPtr);
   303|        }
   304|    }
   305|
   306|    return value;
   307|}
   308|
   309|/**
   310| * @brief   Convert uint32 value to signal data
   311| */
   312|STATIC void Com_SetSignalValueFromUint32(const Com_SignalConfigType* SignalPtr, void* SignalDataPtr, uint32 Value)
   313|{
   314|    uint8* dataPtr = (uint8*)SignalDataPtr;
   315|
   316|    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
   317|    {
   318|        if (SignalPtr->BitSize <= 8U)
   319|        {
   320|            *dataPtr = (uint8)Value;
   321|        }
   322|        else if (SignalPtr->BitSize <= 16U)
   323|        {
   324|            *((uint16*)SignalDataPtr) = (uint16)Value;
   325|        }
   326|        else if (SignalPtr->BitSize <= 32U)
   327|        {
   328|            *((uint32*)SignalDataPtr) = Value;
   329|        }
   330|    }
   331|}
   332|
   333|/**
   334| * @brief   Transmit IPDU via PduR
   335| */
   336|STATIC Std_ReturnType Com_TransmitIPdu(PduIdType PduId)
   337|{
   338|    Std_ReturnType result = E_NOT_OK;
   339|    PduInfoType pduInfo;
   340|    const Com_IPduConfigType* ipduConfig;
   341|
   342|    ipduConfig = Com_GetIPduConfig(PduId);
   343|
   344|    if (ipduConfig != NULL_PTR)
   345|    {
   346|        pduInfo.SduDataPtr = Com_InternalState.IPduBuffer[PduId];
   347|        pduInfo.SduLength = ipduConfig->DataLength;
   348|        pduInfo.MetaDataPtr = NULL_PTR;
   349|
   350|        result = PduR_Transmit(PduId, &pduInfo);
   351|
   352|        if (result == E_OK)
   353|        {
   354|            Com_InternalState.IPduStates[PduId].TxState = COM_TX_PENDING;
   355|        }
   356|    }
   357|
   358|    return result;
   359|}
   360|
   361|/**
   362| * @brief   Get signal configuration by ID
   363| */
   364|STATIC const Com_SignalConfigType* Com_GetSignalConfig(Com_SignalIdType SignalId)
   365|{
   366|    const Com_SignalConfigType* result = NULL_PTR;
   367|
   368|    if ((SignalId < COM_NUM_OF_SIGNALS) && (Com_InternalState.ConfigPtr != NULL_PTR))
   369|    {
   370|        result = &Com_InternalState.ConfigPtr->Signals[SignalId];
   371|    }
   372|
   373|    return result;
   374|}
   375|
   376|/**
   377| * @brief   Get IPDU configuration by PduId
   378| */
   379|STATIC const Com_IPduConfigType* Com_GetIPduConfig(PduIdType PduId)
   380|{
   381|    const Com_IPduConfigType* result = NULL_PTR;
   382|    uint8 i;
   383|
   384|    if ((PduId < COM_NUM_OF_IPDUS) && (Com_InternalState.ConfigPtr != NULL_PTR))
   385|    {
   386|        for (i = 0U; i < Com_InternalState.ConfigPtr->NumIPdus; i++)
   387|        {
   388|            if (Com_InternalState.ConfigPtr->IPdus[i].PduId == PduId)
   389|            {
   390|                result = &Com_InternalState.ConfigPtr->IPdus[i];
   391|                break;
   392|            }
   393|        }
   394|    }
   395|
   396|    return result;
   397|}
   398|
   399|/*==================================================================================================
   400|*                                      GLOBAL FUNCTIONS
   401|==================================================================================================*/
   402|
   403|/**
   404| * @brief   Initializes the COM module
   405| */
   406|void Com_Init(const Com_ConfigType* config)
   407|{
   408|    uint8 i;
   409|    uint8 j;
   410|
   411|#if (COM_DEV_ERROR_DETECT == STD_ON)
   412|    if (config == NULL_PTR)
   413|    {
   414|        COM_DET_REPORT_ERROR(COM_SERVICE_ID_INIT, COM_E_PARAM_POINTER);
   415|        return;
   416|    }
   417|#endif
   418|
   419|    /* Store configuration pointer */
   420|    Com_InternalState.ConfigPtr = config;
   421|
   422|    /* Initialize IPDU states and buffers */
   423|    for (i = 0U; i < COM_NUM_OF_IPDUS; i++)
   424|    {
   425|        Com_InternalState.IPduStates[i].TxState = COM_TX_IDLE;
   426|        Com_InternalState.IPduStates[i].RepetitionCount = 0U;
   427|        Com_InternalState.IPduStates[i].TimeCounter = 0U;
   428|        Com_InternalState.IPduStates[i].Updated = FALSE;
   429|        Com_InternalState.IPduStates[i].GroupEnabled = TRUE;
   430|
   431|        /* Clear IPDU buffer */
   432|        for (j = 0U; j < COM_MAX_IPDU_BUFFER_SIZE; j++)
   433|        {
   434|            Com_InternalState.IPduBuffer[i][j] = 0U;
   435|        }
   436|    }
   437|
   438|    /* Initialize signal states */
   439|    for (i = 0U; i < COM_NUM_OF_SIGNALS; i++)
   440|    {
   441|        Com_InternalState.SignalStates[i].Updated = FALSE;
   442|        Com_InternalState.SignalStates[i].FilterPassed = FALSE;
   443|        Com_InternalState.SignalStates[i].LastValue = 0U;
   444|    }
   445|
   446|    /* Clear IPDU group vector */
   447|    for (i = 0U; i < ((COM_NUM_OF_IPDU_GROUPS + 7U) / 8U); i++)
   448|    {
   449|        Com_InternalState.IPduGroupVector[i] = 0xFFU; /* Enable all groups by default */
   450|    }
   451|
   452|    /* Set module state to initialized */
   453|    Com_InternalState.State = COM_STATE_INIT;
   454|}
   455|
   456|/**
   457| * @brief   Deinitializes the COM module
   458| */
   459|void Com_DeInit(void)
   460|{
   461|#if (COM_DEV_ERROR_DETECT == STD_ON)
   462|    if (Com_InternalState.State != COM_STATE_INIT)
   463|    {
   464|        COM_DET_REPORT_ERROR(COM_SERVICE_ID_DEINIT, COM_E_UNINIT);
   465|        return;
   466|    }
   467|#endif
   468|
   469|    /* Clear configuration pointer */
   470|    Com_InternalState.ConfigPtr = NULL_PTR;
   471|
   472|    /* Set module state to uninitialized */
   473|    Com_InternalState.State = COM_STATE_UNINIT;
   474|}
   475|
   476|/**
   477| * @brief   Send signal
   478| */
   479|uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
   480|{
   481|    uint8 result = COM_SERVICE_NOT_OK;
   482|    const Com_SignalConfigType* signalConfig;
   483|    const Com_IPduConfigType* ipduConfig;
   484|    uint32 newValue;
   485|
   486|#if (COM_DEV_ERROR_DETECT == STD_ON)
   487|    if (Com_InternalState.State != COM_STATE_INIT)
   488|    {
   489|        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_UNINIT);
   490|        return COM_SERVICE_NOT_OK;
   491|    }
   492|
   493|    if (SignalDataPtr == NULL_PTR)
   494|    {
   495|        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_PARAM_POINTER);
   496|        return COM_SERVICE_NOT_OK;
   497|    }
   498|
   499|    if (SignalId >= COM_NUM_OF_SIGNALS)
   500|    {
   501|