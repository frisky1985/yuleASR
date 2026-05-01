/**
 * @file NvM.c
 * @brief Non-Volatile RAM Manager
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
     5|* Dependencies         : MemIf, Fee, Ea
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-15
    10|* Author               : AI Agent (NvM Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "NvM.h"
    20|#include "NvM_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|
    24|/*==================================================================================================
    25|*                                  LOCAL CONSTANT DEFINITIONS
    26|==================================================================================================*/
    27|#define NVM_INSTANCE_ID                 (0x00U)
    28|
    29|/* Development error codes */
    30|#define NVM_E_PARAM_POINTER             (0x01U)
    31|#define NVM_E_PARAM_BLOCK_ID            (0x02U)
    32|#define NVM_E_NOT_INITIALIZED           (0x03U)
    33|#define NVM_E_BLOCK_PENDING             (0x04U)
    34|#define NVM_E_BLOCK_CONFIG              (0x05U)
    35|
    36|/* Module state */
    37|#define NVM_STATE_UNINIT                (0x00U)
    38|#define NVM_STATE_IDLE                  (0x01U)
    39|#define NVM_STATE_BUSY                  (0x02U)
    40|
    41|/* Job types */
    42|#define NVM_JOB_TYPE_NONE               (0x00U)
    43|#define NVM_JOB_TYPE_READ               (0x01U)
    44|#define NVM_JOB_TYPE_WRITE              (0x02U)
    45|#define NVM_JOB_TYPE_RESTORE            (0x03U)
    46|#define NVM_JOB_TYPE_ERASE              (0x04U)
    47|
    48|/* Job states */
    49|#define NVM_JOB_STATE_IDLE              (0x00U)
    50|#define NVM_JOB_STATE_PENDING           (0x01U)
    51|#define NVM_JOB_STATE_PROCESSING        (0x02U)
    52|
    53|/* CRC calculation constants */
    54|#define NVM_CRC8_POLYNOMIAL             (0x1DU)
    55|#define NVM_CRC16_POLYNOMIAL            (0x1021U)
    56|#define NVM_CRC32_POLYNOMIAL            (0x04C11DB7U)
    57|
    58|/*==================================================================================================
    59|*                                  LOCAL MACRO DEFINITIONS
    60|==================================================================================================*/
    61|#if (NVM_DEV_ERROR_DETECT == STD_ON)
    62|    #define NVM_DET_REPORT_ERROR(ApiId, ErrorId) \
    63|        Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, (ApiId), (ErrorId))
    64|#else
    65|    #define NVM_DET_REPORT_ERROR(ApiId, ErrorId)
    66|#endif
    67|
    68|/*==================================================================================================
    69|*                                  LOCAL TYPE DEFINITIONS
    70|==================================================================================================*/
    71|/* Job queue entry */
    72|typedef struct
    73|{
    74|    NvM_BlockIdType BlockId;
    75|    uint8 JobType;
    76|    uint8 JobState;
    77|    void* DataPtr;
    78|    NvM_RequestResultType Result;
    79|    uint8 RetryCount;
    80|} NvM_JobQueueEntryType;
    81|
    82|/* Block runtime state */
    83|typedef struct
    84|{
    85|    NvM_RequestResultType LastResult;
    86|    uint8 JobPending;
    87|    uint8 WriteCounter;
    88|    boolean DataValid;
    89|    boolean DataChanged;
    90|} NvM_BlockStateType;
    91|
    92|/* Module internal state */
    93|typedef struct
    94|{
    95|    uint8 State;
    96|    const NvM_ConfigType* ConfigPtr;
    97|
    98|    /* Standard job queue */
    99|    NvM_JobQueueEntryType StandardQueue[NVM_SIZE_STANDARD_JOB_QUEUE];
   100|    uint8 StandardQueueHead;
   101|    uint8 StandardQueueTail;
   102|    uint8 StandardQueueCount;
   103|
   104|    /* Immediate job queue (high priority) */
   105|    NvM_JobQueueEntryType ImmediateQueue[NVM_SIZE_IMMEDIATE_JOB_QUEUE];
   106|    uint8 ImmediateQueueHead;
   107|    uint8 ImmediateQueueTail;
   108|    uint8 ImmediateQueueCount;
   109|
   110|    /* Block states */
   111|    NvM_BlockStateType BlockStates[NVM_NUM_OF_NVRAM_BLOCKS];
   112|
   113|    /* Current active job */
   114|    NvM_JobQueueEntryType* CurrentJob;
   115|} NvM_InternalStateType;
   116|
   117|/*==================================================================================================
   118|*                                  LOCAL VARIABLE DECLARATIONS
   119|==================================================================================================*/
   120|#define NVM_START_SEC_VAR_CLEARED_UNSPECIFIED
   121|#include "MemMap.h"
   122|
   123|STATIC NvM_InternalStateType NvM_InternalState;
   124|
   125|#define NVM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   126|#include "MemMap.h"
   127|
   128|/*==================================================================================================
   129|*                                  LOCAL FUNCTION PROTOTYPES
   130|==================================================================================================*/
   131|STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry);
   132|STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry);
   133|STATIC boolean NvM_QueueIsEmpty(uint8 Count);
   134|STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize);
   135|
   136|STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId);
   137|STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId);
   138|
   139|STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr);
   140|STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr);
   141|STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr);
   142|
   143|STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType);
   144|STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length);
   145|STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length);
   146|STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length);
   147|
   148|STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr);
   149|
   150|/*==================================================================================================
   151|*                                      LOCAL FUNCTIONS
   152|==================================================================================================*/
   153|#define NVM_START_SEC_CODE
   154|#include "MemMap.h"
   155|
   156|/**
   157| * @brief   Push job into queue
   158| */
   159|STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry)
   160|{
   161|    Std_ReturnType result = E_NOT_OK;
   162|
   163|    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
   164|    {
   165|        if (*Count < MaxSize)
   166|        {
   167|            Queue[*Tail] = *Entry;
   168|            *Tail = (*Tail + 1U) % MaxSize;
   169|            (*Count)++;
   170|            result = E_OK;
   171|        }
   172|    }
   173|
   174|    return result;
   175|}
   176|
   177|/**
   178| * @brief   Pop job from queue
   179| */
   180|STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry)
   181|{
   182|    Std_ReturnType result = E_NOT_OK;
   183|
   184|    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
   185|    {
   186|        if (*Count > 0U)
   187|        {
   188|            *Entry = Queue[*Head];
   189|            *Head = (*Head + 1U) % MaxSize;
   190|            (*Count)--;
   191|            result = E_OK;
   192|        }
   193|    }
   194|
   195|    return result;
   196|}
   197|
   198|/**
   199| * @brief   Check if queue is empty
   200| */
   201|STATIC boolean NvM_QueueIsEmpty(uint8 Count)
   202|{
   203|    return (Count == 0U) ? TRUE : FALSE;
   204|}
   205|
   206|/**
   207| * @brief   Check if queue is full
   208| */
   209|STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize)
   210|{
   211|    return (Count >= MaxSize) ? TRUE : FALSE;
   212|}
   213|
   214|/**
   215| * @brief   Get block descriptor for given block ID
   216| */
   217|STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId)
   218|{
   219|    const NvM_BlockDescriptorType* result = NULL_PTR;
   220|    uint8 i;
   221|
   222|    if (NvM_InternalState.ConfigPtr != NULL_PTR)
   223|    {
   224|        for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
   225|        {
   226|            if (NvM_InternalState.ConfigPtr->BlockDescriptors[i].BlockId == BlockId)
   227|            {
   228|                result = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];
   229|                break;
   230|            }
   231|        }
   232|    }
   233|
   234|    return result;
   235|}
   236|
   237|/**
   238| * @brief   Validate block ID
   239| */
   240|STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId)
   241|{
   242|    Std_ReturnType result = E_NOT_OK;
   243|
   244|    if ((BlockId > 0U) && (BlockId < NVM_NUM_OF_NVRAM_BLOCKS))
   245|    {
   246|        if (NvM_GetBlockDescriptor(BlockId) != NULL_PTR)
   247|        {
   248|            result = E_OK;
   249|        }
   250|    }
   251|
   252|    return result;
   253|}
   254|
   255|/**
   256| * @brief   Calculate CRC8
   257| */
   258|STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length)
   259|{
   260|    uint8 crc = 0xFFU;
   261|    uint16 i;
   262|    uint8 bit;
   263|
   264|    for (i = 0U; i < Length; i++)
   265|    {
   266|        crc ^= DataPtr[i];
   267|        for (bit = 0U; bit < 8U; bit++)
   268|        {
   269|            if ((crc & 0x80U) != 0U)
   270|            {
   271|                crc = (crc << 1U) ^ NVM_CRC8_POLYNOMIAL;
   272|            }
   273|            else
   274|            {
   275|                crc = crc << 1U;
   276|            }
   277|        }
   278|    }
   279|
   280|    return crc;
   281|}
   282|
   283|/**
   284| * @brief   Calculate CRC16
   285| */
   286|STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length)
   287|{
   288|    uint16 crc = 0xFFFFU;
   289|    uint16 i;
   290|    uint8 bit;
   291|
   292|    for (i = 0U; i < Length; i++)
   293|    {
   294|        crc ^= ((uint16)DataPtr[i] << 8U);
   295|        for (bit = 0U; bit < 8U; bit++)
   296|        {
   297|            if ((crc & 0x8000U) != 0U)
   298|            {
   299|                crc = (crc << 1U) ^ NVM_CRC16_POLYNOMIAL;
   300|            }
   301|            else
   302|            {
   303|                crc = crc << 1U;
   304|            }
   305|        }
   306|    }
   307|
   308|    return crc;
   309|}
   310|
   311|/**
   312| * @brief   Calculate CRC32
   313| */
   314|STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length)
   315|{
   316|    uint32 crc = 0xFFFFFFFFU;
   317|    uint16 i;
   318|    uint8 bit;
   319|
   320|    for (i = 0U; i < Length; i++)
   321|    {
   322|        crc ^= ((uint32)DataPtr[i] << 24U);
   323|        for (bit = 0U; bit < 8U; bit++)
   324|        {
   325|            if ((crc & 0x80000000U) != 0U)
   326|            {
   327|                crc = (crc << 1U) ^ NVM_CRC32_POLYNOMIAL;
   328|            }
   329|            else
   330|            {
   331|                crc = crc << 1U;
   332|            }
   333|        }
   334|    }
   335|
   336|    return crc;
   337|}
   338|
   339|/**
   340| * @brief   Calculate CRC based on type
   341| */
   342|STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType)
   343|{
   344|    uint32 crc = 0U;
   345|    const uint8* dataPtr = (const uint8*)DataPtr;
   346|
   347|    switch (CrcType)
   348|    {
   349|        case NVM_CRC_8:
   350|            crc = (uint32)NvM_CalculateCrc8(dataPtr, Length);
   351|            break;
   352|
   353|        case NVM_CRC_16:
   354|            crc = (uint32)NvM_CalculateCrc16(dataPtr, Length);
   355|            break;
   356|
   357|        case NVM_CRC_32:
   358|            crc = NvM_CalculateCrc32(dataPtr, Length);
   359|            break;
   360|
   361|        case NVM_CRC_NONE:
   362|        default:
   363|            crc = 0U;
   364|            break;
   365|    }
   366|
   367|    return crc;
   368|}
   369|
   370|/**
   371| * @brief   Copy ROM default data to RAM
   372| */
   373|STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr)
   374|{
   375|    const NvM_BlockDescriptorType* blockDesc = NvM_GetBlockDescriptor(BlockId);
   376|
   377|    if ((blockDesc != NULL_PTR) && (DestPtr != NULL_PTR) && (blockDesc->RomBlockData != NULL_PTR))
   378|    {
   379|        (void)memcpy(DestPtr, blockDesc->RomBlockData, blockDesc->NvBlockLength);
   380|        NvM_InternalState.BlockStates[BlockId].DataValid = TRUE;
   381|    }
   382|}
   383|
   384|/**
   385| * @brief   Process read job
   386| */
   387|STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr)
   388|{
   389|    const NvM_BlockDescriptorType* blockDesc;
   390|    Std_ReturnType memIfResult;
   391|    uint16 blockNumber;
   392|
   393|    if (JobPtr != NULL_PTR)
   394|    {
   395|        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);
   396|
   397|        if (blockDesc != NULL_PTR)
   398|        {
   399|            blockNumber = blockDesc->BlockBaseNumber;
   400|
   401|            /* Call MemIf to read from NV memory */
   402|            memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
   403|                                     (uint8*)JobPtr->DataPtr, blockDesc->NvBlockLength);
   404|
   405|            if (memIfResult == E_OK)
   406|            {
   407|                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
   408|                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
   409|            }
   410|            else
   411|            {
   412|                /* Read failed immediately, try to restore from ROM */
   413|                NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
   414|                JobPtr->Result = NVM_REQ_OK;
   415|                JobPtr->JobState = NVM_JOB_STATE_IDLE;
   416|                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_OK;
   417|                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
   418|            }
   419|        }
   420|        else
   421|        {
   422|            JobPtr->Result = NVM_REQ_NOT_OK;
   423|            JobPtr->JobState = NVM_JOB_STATE_IDLE;
   424|        }
   425|    }
   426|}
   427|
   428|/**
   429| * @brief   Process write job
   430| */
   431|STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr)
   432|{
   433|    const NvM_BlockDescriptorType* blockDesc;
   434|    Std_ReturnType memIfResult;
   435|    uint16 blockNumber;
   436|
   437|    if (JobPtr != NULL_PTR)
   438|    {
   439|        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);
   440|
   441|        if (blockDesc != NULL_PTR)
   442|        {
   443|            blockNumber = blockDesc->BlockBaseNumber;
   444|
   445|            /* Call MemIf to write to NV memory */
   446|            memIfResult = MemIf_Write(blockDesc->DeviceId, blockNumber,
   447|                                      (uint8*)JobPtr->DataPtr);
   448|
   449|            if (memIfResult == E_OK)
   450|            {
   451|                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
   452|                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
   453|            }
   454|            else
   455|            {
   456|                JobPtr->Result = NVM_REQ_NOT_OK;
   457|                JobPtr->JobState = NVM_JOB_STATE_IDLE;
   458|                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
   459|                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
   460|            }
   461|        }
   462|        else
   463|        {
   464|            JobPtr->Result = NVM_REQ_NOT_OK;
   465|            JobPtr->JobState = NVM_JOB_STATE_IDLE;
   466|        }
   467|    }
   468|}
   469|
   470|/**
   471| * @brief   Process restore job
   472| */
   473|STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr)
   474|{
   475|    if (JobPtr != NULL_PTR)
   476|    {
   477|        /* Copy ROM data to RAM */
   478|        NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
   479|
   480|        JobPtr->Result = NVM_REQ_OK;
   481|        JobPtr->JobState = NVM_JOB_STATE_IDLE;
   482|        NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_OK;
   483|        NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
   484|    }
   485|}
   486|
   487|/*==================================================================================================
   488|*                                      GLOBAL FUNCTIONS
   489|==================================================================================================*/
   490|
   491|/**
   492| * @brief   Initializes the NvM module
   493| * @param   ConfigPtr - Pointer to configuration structure
   494| * @return  None
   495| */
   496|void NvM_Init(const NvM_ConfigType* ConfigPtr)
   497|{
   498|    uint8 i;
   499|
   500|#if (NVM_DEV_ERROR_DETECT == STD_ON)
   501|