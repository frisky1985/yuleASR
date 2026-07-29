/**
 * @file SomeIpXf.c
 * @brief SOME/IP Transformer
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
     5|* Dependencies         : SomeIpTp, Det
     6|*
     7|* SW Version           : 4.7.0
     8|* Build Version        : YULETECH_AUTOSAR_4.7.0
     9|* Build Date           : 2026-04-29
    10|* Author               : AI Agent (SomeIpXf Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "SomeIpXf.h"
    20|#include "SomeIpXf_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|#include <string.h>
    24|
    25|/*==================================================================================================
    26|*                                  LOCAL CONSTANT DEFINITIONS
    27|==================================================================================================*/
    28|#define SOMEIPXF_STATE_UNINIT                   (0x00U)
    29|#define SOMEIPXF_STATE_INIT                     (0x01U)
    30|
    31|/* SOME/IP Header offsets */
    32|#define SOMEIPXF_HDR_SERVICE_ID_OFFSET          (0U)
    33|#define SOMEIPXF_HDR_METHOD_ID_OFFSET           (2U)
    34|#define SOMEIPXF_HDR_LENGTH_OFFSET              (4U)
    35|#define SOMEIPXF_HDR_PROTOCOL_VER_OFFSET        (8U)
    36|#define SOMEIPXF_HDR_INTERFACE_VER_OFFSET       (9U)
    37|#define SOMEIPXF_HDR_MSG_TYPE_OFFSET            (10U)
    38|#define SOMEIPXF_HDR_RETURN_CODE_OFFSET         (11U)
    39|#define SOMEIPXF_HDR_SIZE                       (12U)
    40|
    41|/*==================================================================================================
    42|*                                  LOCAL MACRO DEFINITIONS
    43|==================================================================================================*/
    44|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    45|    #define SOMEIPXF_DET_REPORT_ERROR(ApiId, ErrorId) \
    46|        Det_ReportError(SOMEIPXF_MODULE_ID, SOMEIPXF_INSTANCE_ID, (ApiId), (ErrorId))
    47|#else
    48|    #define SOMEIPXF_DET_REPORT_ERROR(ApiId, ErrorId)
    49|#endif
    50|
    51|#define SOMEIPXF_IS_VALID_TRANSFORMER_ID(Id) \
    52|    (((Id) < SOMEIPXF_NUMBER_OF_TRANSFORMERS) ? TRUE : FALSE)
    53|
    54|/* Big-endian serialization macros */
    55|#define SOMEIPXF_PUT_U16_BE(Buffer, Offset, Value) \
    56|    do { \
    57|        (Buffer)[(Offset)] = (uint8)(((Value) >> 8) & 0xFFU); \
    58|        (Buffer)[(Offset) + 1U] = (uint8)((Value) & 0xFFU); \
    59|    } while(0)
    60|
    61|#define SOMEIPXF_GET_U16_BE(Buffer, Offset) \
    62|    ((((uint16)(Buffer)[(Offset)]) << 8) | ((uint16)(Buffer)[(Offset) + 1U]))
    63|
    64|#define SOMEIPXF_PUT_U32_BE(Buffer, Offset, Value) \
    65|    do { \
    66|        (Buffer)[(Offset)] = (uint8)(((Value) >> 24) & 0xFFU); \
    67|        (Buffer)[(Offset) + 1U] = (uint8)(((Value) >> 16) & 0xFFU); \
    68|        (Buffer)[(Offset) + 2U] = (uint8)(((Value) >> 8) & 0xFFU); \
    69|        (Buffer)[(Offset) + 3U] = (uint8)((Value) & 0xFFU); \
    70|    } while(0)
    71|
    72|#define SOMEIPXF_GET_U32_BE(Buffer, Offset) \
    73|    ((((uint32)(Buffer)[(Offset)]) << 24) | \
    74|     (((uint32)(Buffer)[(Offset) + 1U]) << 16) | \
    75|     (((uint32)(Buffer)[(Offset) + 2U]) << 8) | \
    76|     ((uint32)(Buffer)[(Offset) + 3U]))
    77|
    78|/*==================================================================================================
    79|*                                  LOCAL TYPE DEFINITIONS
    80|==================================================================================================*/
    81|typedef struct {
    82|    uint8 State;
    83|    const SomeIpXf_ConfigType* ConfigPtr;
    84|} SomeIpXf_InternalStateType;
    85|
    86|/*==================================================================================================
    87|*                                  LOCAL VARIABLE DECLARATIONS
    88|==================================================================================================*/
    89|#define SOMEIPXF_START_SEC_VAR_CLEARED_UNSPECIFIED
    90|#include "MemMap.h"
    91|
    92|STATIC SomeIpXf_InternalStateType SomeIpXf_InternalState;
    93|
    94|#define SOMEIPXF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    95|#include "MemMap.h"
    96|
    97|/*==================================================================================================
    98|*                                      LOCAL FUNCTIONS
    99|==================================================================================================*/
   100|#define SOMEIPXF_START_SEC_CODE
   101|#include "MemMap.h"
   102|
   103|/**
   104| * @brief   Check if data element ID is valid
   105| */
   106|STATIC boolean SomeIpXf_IsValidDataElementId(uint16 TransformerId, uint16 DataElementId)
   107|{
   108|    boolean result = FALSE;
   109|    const SomeIpXf_TransformerConfigType* transPtr;
   110|
   111|    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
   112|    {
   113|        if (TransformerId < SomeIpXf_InternalState.ConfigPtr->NumTransformers)
   114|        {
   115|            transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];
   116|            if (DataElementId < transPtr->NumDataElements)
   117|            {
   118|                result = TRUE;
   119|            }
   120|        }
   121|    }
   122|
   123|    return result;
   124|}
   125|
   126|/**
   127| * @brief   Align offset to byte boundary
   128| */
   129|STATIC uint16 SomeIpXf_AlignOffset(uint16 Offset, uint16 Alignment)
   130|{
   131|    uint16 remainder = Offset % Alignment;
   132|    if (remainder != 0U)
   133|    {
   134|        Offset += (Alignment - remainder);
   135|    }
   136|    return Offset;
   137|}
   138|
   139|/*==================================================================================================
   140|*                                      GLOBAL FUNCTIONS
   141|==================================================================================================*/
   142|
   143|/**
   144| * @brief   Initializes the SOME/IP Transformer module
   145| */
   146|void SomeIpXf_Init(const SomeIpXf_ConfigType* ConfigPtr)
   147|{
   148|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   149|    if (SomeIpXf_InternalState.State == SOMEIPXF_STATE_INIT)
   150|    {
   151|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_INIT, SOMEIPXF_E_ALREADY_INITIALIZED);
   152|        return;
   153|    }
   154|
   155|    if (ConfigPtr == NULL_PTR)
   156|    {
   157|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_INIT, SOMEIPXF_E_PARAM_POINTER);
   158|        return;
   159|    }
   160|#endif
   161|
   162|    /* Store configuration pointer */
   163|    SomeIpXf_InternalState.ConfigPtr = ConfigPtr;
   164|
   165|    /* Set module state to initialized */
   166|    SomeIpXf_InternalState.State = SOMEIPXF_STATE_INIT;
   167|}
   168|
   169|/**
   170| * @brief   Deinitializes the SOME/IP Transformer module
   171| */
   172|void SomeIpXf_DeInit(void)
   173|{
   174|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   175|    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
   176|    {
   177|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DEINIT, SOMEIPXF_E_UNINIT);
   178|        return;
   179|    }
   180|#endif
   181|
   182|    /* Clear configuration pointer */
   183|    SomeIpXf_InternalState.ConfigPtr = NULL_PTR;
   184|
   185|    /* Set module state to uninitialized */
   186|    SomeIpXf_InternalState.State = SOMEIPXF_STATE_UNINIT;
   187|}
   188|
   189|/**
   190| * @brief   Gets version information
   191| */
   192|#if (SOMEIPXF_VERSION_INFO_API == STD_ON)
   193|void SomeIpXf_GetVersionInfo(Std_VersionInfoType* versioninfo)
   194|{
   195|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   196|    if (versioninfo == NULL_PTR)
   197|    {
   198|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_GETVERSIONINFO, SOMEIPXF_E_PARAM_POINTER);
   199|        return;
   200|    }
   201|#endif
   202|
   203|    versioninfo->vendorID = SOMEIPXF_VENDOR_ID;
   204|    versioninfo->moduleID = SOMEIPXF_MODULE_ID;
   205|    versioninfo->sw_major_version = SOMEIPXF_SW_MAJOR_VERSION;
   206|    versioninfo->sw_minor_version = SOMEIPXF_SW_MINOR_VERSION;
   207|    versioninfo->sw_patch_version = SOMEIPXF_SW_PATCH_VERSION;
   208|}
   209|#endif
   210|
   211|/**
   212| * @brief   Transforms data to SOME/IP format
   213| */
   214|Std_ReturnType SomeIpXf_Transform(uint16 TransformerId, uint16 DataElementId,
   215|                                   const SomeIpXf_BufferType* SourceBuffer,
   216|                                   SomeIpXf_BufferType* TargetBuffer)
   217|{
   218|    Std_ReturnType result = E_NOT_OK;
   219|    const SomeIpXf_TransformerConfigType* transPtr;
   220|    const SomeIpXf_DataElementConfigType* elemPtr;
   221|    uint16 headerSize = 0U;
   222|    uint32 payloadOffset = 0U;
   223|
   224|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   225|    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
   226|    {
   227|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_UNINIT);
   228|        return E_NOT_OK;
   229|    }
   230|
   231|    if ((SourceBuffer == NULL_PTR) || (TargetBuffer == NULL_PTR))
   232|    {
   233|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_PARAM_POINTER);
   234|        return E_NOT_OK;
   235|    }
   236|
   237|    if (!SOMEIPXF_IS_VALID_TRANSFORMER_ID(TransformerId))
   238|    {
   239|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_PARAM_CONFIG);
   240|        return E_NOT_OK;
   241|    }
   242|#endif
   243|
   244|    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
   245|    {
   246|        transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];
   247|
   248|        /* Add SOME/IP header if enabled */
   249|        if (transPtr->HeaderIncluded)
   250|        {
   251|            if (TargetBuffer->MaxLength >= SOMEIPXF_HDR_SIZE)
   252|            {
   253|                SomeIpXf_HeaderType header;
   254|                header.ServiceId = transPtr->InterfaceConfig->ServiceId;
   255|                header.MethodId = transPtr->InterfaceConfig->MethodId;
   256|                header.Length = (uint32)SourceBuffer->Length;
   257|                header.ProtocolVersion = SOMEIPXF_PROTOCOL_VERSION;
   258|                header.InterfaceVersion = SOMEIPXF_INTERFACE_VERSION;
   259|                header.MessageType = transPtr->InterfaceConfig->MessageType;
   260|                header.ReturnCode = transPtr->InterfaceConfig->ReturnCode;
   261|
   262|                (void)SomeIpXf_BuildHeader(&header, TargetBuffer->Data);
   263|                headerSize = SOMEIPXF_HDR_SIZE;
   264|            }
   265|        }
   266|
   267|        /* Serialize data element */
   268|        if (DataElementId < transPtr->NumDataElements)
   269|        {
   270|            elemPtr = &transPtr->DataElements[DataElementId];
   271|            payloadOffset = headerSize;
   272|
   273|            switch (elemPtr->DataType)
   274|            {
   275|                case SOMEIPXF_DT_BOOLEAN:
   276|                    if ((SourceBuffer->Length >= 1U) && 
   277|                        (TargetBuffer->MaxLength >= payloadOffset + 1U))
   278|                    {
   279|                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0] ? 1U : 0U;
   280|                        TargetBuffer->Length = payloadOffset + 1U;
   281|                        result = E_OK;
   282|                    }
   283|                    break;
   284|
   285|                case SOMEIPXF_DT_UINT8:
   286|                    if ((SourceBuffer->Length >= 1U) && 
   287|                        (TargetBuffer->MaxLength >= payloadOffset + 1U))
   288|                    {
   289|                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0];
   290|                        TargetBuffer->Length = payloadOffset + 1U;
   291|                        result = E_OK;
   292|                    }
   293|                    break;
   294|
   295|                case SOMEIPXF_DT_UINT16:
   296|                    if ((SourceBuffer->Length >= 2U) && 
   297|                        (TargetBuffer->MaxLength >= payloadOffset + 2U))
   298|                    {
   299|                        SOMEIPXF_PUT_U16_BE(TargetBuffer->Data, payloadOffset, 
   300|                                           ((uint16)SourceBuffer->Data[0] << 8) | SourceBuffer->Data[1]);
   301|                        TargetBuffer->Length = payloadOffset + 2U;
   302|                        result = E_OK;
   303|                    }
   304|                    break;
   305|
   306|                case SOMEIPXF_DT_UINT32:
   307|                    if ((SourceBuffer->Length >= 4U) && 
   308|                        (TargetBuffer->MaxLength >= payloadOffset + 4U))
   309|                    {
   310|                        uint32 value = ((uint32)SourceBuffer->Data[0] << 24) |
   311|                                      ((uint32)SourceBuffer->Data[1] << 16) |
   312|                                      ((uint32)SourceBuffer->Data[2] << 8) |
   313|                                      (uint32)SourceBuffer->Data[3];
   314|                        SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, value);
   315|                        TargetBuffer->Length = payloadOffset + 4U;
   316|                        result = E_OK;
   317|                    }
   318|                    break;
   319|
   320|                case SOMEIPXF_DT_STRING:
   321|                    {
   322|                        uint32 strLen = SourceBuffer->Length;
   323|                        if (strLen > SOMEIPXF_MAX_STRING_LENGTH)
   324|                        {
   325|                            strLen = SOMEIPXF_MAX_STRING_LENGTH;
   326|                        }
   327|                        
   328|                        if (TargetBuffer->MaxLength >= payloadOffset + strLen + 4U)
   329|                        {
   330|                            /* Add length field */
   331|                            SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, strLen);
   332|                            /* Copy string data */
   333|                            (void)memcpy(&TargetBuffer->Data[payloadOffset + 4U], 
   334|                                        SourceBuffer->Data, strLen);
   335|                            TargetBuffer->Length = payloadOffset + 4U + strLen;
   336|                            result = E_OK;
   337|                        }
   338|                    }
   339|                    break;
   340|
   341|                case SOMEIPXF_DT_ARRAY:
   342|                    if (TargetBuffer->MaxLength >= payloadOffset + SourceBuffer->Length + 4U)
   343|                    {
   344|                        /* Add length field */
   345|                        SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, 
   346|                                           (uint32)SourceBuffer->Length);
   347|                        /* Copy array data */
   348|                        (void)memcpy(&TargetBuffer->Data[payloadOffset + 4U], 
   349|                                    SourceBuffer->Data, SourceBuffer->Length);
   350|                        TargetBuffer->Length = payloadOffset + 4U + (uint32)SourceBuffer->Length;
   351|                        result = E_OK;
   352|                    }
   353|                    break;
   354|
   355|                default:
   356|                    /* Unsupported data type */
   357|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   358|                    SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_INVALID_DATA_TYPE);
   359|#endif
   360|                    break;
   361|            }
   362|
   363|            /* Update SOME/IP header length field */
   364|            if ((result == E_OK) && (transPtr->HeaderIncluded))
   365|            {
   366|                uint32 payloadLen = TargetBuffer->Length - SOMEIPXF_HDR_SIZE;
   367|                SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, SOMEIPXF_HDR_LENGTH_OFFSET, payloadLen);
   368|            }
   369|        }
   370|    }
   371|
   372|    return result;
   373|}
   374|
   375|/**
   376| * @brief   De-transforms data from SOME/IP format
   377| */
   378|Std_ReturnType SomeIpXf_Detransform(uint16 TransformerId, uint16 DataElementId,
   379|                                     const SomeIpXf_BufferType* SourceBuffer,
   380|                                     SomeIpXf_BufferType* TargetBuffer)
   381|{
   382|    Std_ReturnType result = E_NOT_OK;
   383|    const SomeIpXf_TransformerConfigType* transPtr;
   384|    const SomeIpXf_DataElementConfigType* elemPtr;
   385|    uint16 headerSize = 0U;
   386|    uint32 payloadOffset = 0U;
   387|    SomeIpXf_HeaderType header;
   388|
   389|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   390|    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
   391|    {
   392|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_UNINIT);
   393|        return E_NOT_OK;
   394|    }
   395|
   396|    if ((SourceBuffer == NULL_PTR) || (TargetBuffer == NULL_PTR))
   397|    {
   398|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_PARAM_POINTER);
   399|        return E_NOT_OK;
   400|    }
   401|
   402|    if (!SOMEIPXF_IS_VALID_TRANSFORMER_ID(TransformerId))
   403|    {
   404|        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_PARAM_CONFIG);
   405|        return E_NOT_OK;
   406|    }
   407|#endif
   408|
   409|    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
   410|    {
   411|        transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];
   412|
   413|        /* Parse SOME/IP header if present */
   414|        if (transPtr->HeaderIncluded)
   415|        {
   416|            if (SourceBuffer->Length >= SOMEIPXF_HDR_SIZE)
   417|            {
   418|                if (SomeIpXf_ParseHeader(SourceBuffer->Data, &header) == E_OK)
   419|                {
   420|                    /* Validate header */
   421|                    if (header.ProtocolVersion != SOMEIPXF_PROTOCOL_VERSION)
   422|                    {
   423|#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
   424|                        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, 
   425|                                                   SOMEIPXF_E_WRONG_PROTOCOL_VERSION);
   426|#endif
   427|                        return E_NOT_OK;
   428|                    }
   429|
   430|                    if (header.ReturnCode != SOMEIPXF_RET_CODE_OK)
   431|                    {
   432|                        return E_NOT_OK;
   433|                    }
   434|
   435|                    headerSize = SOMEIPXF_HDR_SIZE;
   436|                }
   437|            }
   438|        }
   439|
   440|        /* Deserialize data element */
   441|        if (DataElementId < transPtr->NumDataElements)
   442|        {
   443|            elemPtr = &transPtr->DataElements[DataElementId];
   444|            payloadOffset = headerSize;
   445|
   446|            switch (elemPtr->DataType)
   447|            {
   448|                case SOMEIPXF_DT_BOOLEAN:
   449|                    if ((SourceBuffer->Length >= payloadOffset + 1U) && 
   450|                        (TargetBuffer->MaxLength >= 1U))
   451|                    {
   452|                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset] ? 1U : 0U;
   453|                        TargetBuffer->Length = 1U;
   454|                        result = E_OK;
   455|                    }
   456|                    break;
   457|
   458|                case SOMEIPXF_DT_UINT8:
   459|                    if ((SourceBuffer->Length >= payloadOffset + 1U) && 
   460|                        (TargetBuffer->MaxLength >= 1U))
   461|                    {
   462|                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset];
   463|                        TargetBuffer->Length = 1U;
   464|                        result = E_OK;
   465|                    }
   466|                    break;
   467|
   468|                case SOMEIPXF_DT_UINT16:
   469|                    if ((SourceBuffer->Length >= payloadOffset + 2U) && 
   470|                        (TargetBuffer->MaxLength >= 2U))
   471|                    {
   472|                        uint16 value = SOMEIPXF_GET_U16_BE(SourceBuffer->Data, payloadOffset);
   473|                        TargetBuffer->Data[0] = (uint8)(value >> 8);
   474|                        TargetBuffer->Data[1] = (uint8)(value & 0xFFU);
   475|                        TargetBuffer->Length = 2U;
   476|                        result = E_OK;
   477|                    }
   478|                    break;
   479|
   480|                case SOMEIPXF_DT_UINT32:
   481|                    if ((SourceBuffer->Length >= payloadOffset + 4U) && 
   482|                        (TargetBuffer->MaxLength >= 4U))
   483|                    {
   484|                        uint32 value = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
   485|                        TargetBuffer->Data[0] = (uint8)(value >> 24);
   486|                        TargetBuffer->Data[1] = (uint8)(value >> 16);
   487|                        TargetBuffer->Data[2] = (uint8)(value >> 8);
   488|                        TargetBuffer->Data[3] = (uint8)(value & 0xFFU);
   489|                        TargetBuffer->Length = 4U;
   490|                        result = E_OK;
   491|                    }
   492|                    break;
   493|
   494|                case SOMEIPXF_DT_STRING:
   495|                    if (SourceBuffer->Length >= payloadOffset + 4U)
   496|                    {
   497|                        uint32 strLen = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
   498|                        if ((strLen <= SOMEIPXF_MAX_STRING_LENGTH) && 
   499|                            (SourceBuffer->Length >= payloadOffset + 4U + strLen) &&
   500|                            (TargetBuffer->MaxLength >= strLen))
   501|