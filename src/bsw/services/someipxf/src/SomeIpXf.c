/**
 * @file SomeIpXf.c
 * @brief SOME/IP Transformer
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
* Dependencies         : SomeIpTp, Det
*
* SW Version           : 4.7.0
* Build Version        : YULETECH_AUTOSAR_4.7.0
* Build Date           : 2026-04-29
* Author               : AI Agent (SomeIpXf Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "SomeIpXf.h"
#include "SomeIpXf_Cfg.h"
#include "Det.h"
#include "MemMap.h"
#include <string.h>

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define SOMEIPXF_STATE_UNINIT                   (0x00U)
#define SOMEIPXF_STATE_INIT                     (0x01U)

/* SOME/IP Header offsets */
#define SOMEIPXF_HDR_SERVICE_ID_OFFSET          (0U)
#define SOMEIPXF_HDR_METHOD_ID_OFFSET           (2U)
#define SOMEIPXF_HDR_LENGTH_OFFSET              (4U)
#define SOMEIPXF_HDR_PROTOCOL_VER_OFFSET        (8U)
#define SOMEIPXF_HDR_INTERFACE_VER_OFFSET       (9U)
#define SOMEIPXF_HDR_MSG_TYPE_OFFSET            (10U)
#define SOMEIPXF_HDR_RETURN_CODE_OFFSET         (11U)
#define SOMEIPXF_HDR_SIZE                       (12U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    #define SOMEIPXF_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(SOMEIPXF_MODULE_ID, SOMEIPXF_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define SOMEIPXF_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#define SOMEIPXF_IS_VALID_TRANSFORMER_ID(Id) \
    (((Id) < SOMEIPXF_NUMBER_OF_TRANSFORMERS) ? TRUE : FALSE)

/* Big-endian serialization macros */
#define SOMEIPXF_PUT_U16_BE(Buffer, Offset, Value) \
    do { \
        (Buffer)[(Offset)] = (uint8)(((Value) >> 8) & 0xFFU); \
        (Buffer)[(Offset) + 1U] = (uint8)((Value) & 0xFFU); \
    } while(0)

#define SOMEIPXF_GET_U16_BE(Buffer, Offset) \
    ((((uint16)(Buffer)[(Offset)]) << 8) | ((uint16)(Buffer)[(Offset) + 1U]))

#define SOMEIPXF_PUT_U32_BE(Buffer, Offset, Value) \
    do { \
        (Buffer)[(Offset)] = (uint8)(((Value) >> 24) & 0xFFU); \
        (Buffer)[(Offset) + 1U] = (uint8)(((Value) >> 16) & 0xFFU); \
        (Buffer)[(Offset) + 2U] = (uint8)(((Value) >> 8) & 0xFFU); \
        (Buffer)[(Offset) + 3U] = (uint8)((Value) & 0xFFU); \
    } while(0)

#define SOMEIPXF_GET_U32_BE(Buffer, Offset) \
    ((((uint32)(Buffer)[(Offset)]) << 24) | \
     (((uint32)(Buffer)[(Offset) + 1U]) << 16) | \
     (((uint32)(Buffer)[(Offset) + 2U]) << 8) | \
     ((uint32)(Buffer)[(Offset) + 3U]))

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    uint8 State;
    const SomeIpXf_ConfigType* ConfigPtr;
} SomeIpXf_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define SOMEIPXF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC SomeIpXf_InternalStateType SomeIpXf_InternalState;

#define SOMEIPXF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define SOMEIPXF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Check if data element ID is valid
 */
STATIC boolean SomeIpXf_IsValidDataElementId(uint16 TransformerId, uint16 DataElementId)
{
    boolean result = FALSE;
    const SomeIpXf_TransformerConfigType* transPtr;

    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
    {
        if (TransformerId < SomeIpXf_InternalState.ConfigPtr->NumTransformers)
        {
            transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];
            if (DataElementId < transPtr->NumDataElements)
            {
                result = TRUE;
            }
        }
    }

    return result;
}

/**
 * @brief   Align offset to byte boundary
 */
STATIC uint16 SomeIpXf_AlignOffset(uint16 Offset, uint16 Alignment)
{
    uint16 remainder = Offset % Alignment;
    if (remainder != 0U)
    {
        Offset += (Alignment - remainder);
    }
    return Offset;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the SOME/IP Transformer module
 */
void SomeIpXf_Init(const SomeIpXf_ConfigType* ConfigPtr)
{
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpXf_InternalState.State == SOMEIPXF_STATE_INIT)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_INIT, SOMEIPXF_E_ALREADY_INITIALIZED);
        return;
    }

    if (ConfigPtr == NULL_PTR)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_INIT, SOMEIPXF_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    SomeIpXf_InternalState.ConfigPtr = ConfigPtr;

    /* Set module state to initialized */
    SomeIpXf_InternalState.State = SOMEIPXF_STATE_INIT;
}

/**
 * @brief   Deinitializes the SOME/IP Transformer module
 */
void SomeIpXf_DeInit(void)
{
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DEINIT, SOMEIPXF_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    SomeIpXf_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    SomeIpXf_InternalState.State = SOMEIPXF_STATE_UNINIT;
}

/**
 * @brief   Gets version information
 */
#if (SOMEIPXF_VERSION_INFO_API == STD_ON)
void SomeIpXf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_GETVERSIONINFO, SOMEIPXF_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = SOMEIPXF_VENDOR_ID;
    versioninfo->moduleID = SOMEIPXF_MODULE_ID;
    versioninfo->sw_major_version = SOMEIPXF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SOMEIPXF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SOMEIPXF_SW_PATCH_VERSION;
}
#endif

/**
 * @brief   Transforms data to SOME/IP format
 */
Std_ReturnType SomeIpXf_Transform(uint16 TransformerId, uint16 DataElementId,
                                   const SomeIpXf_BufferType* SourceBuffer,
                                   SomeIpXf_BufferType* TargetBuffer)
{
    Std_ReturnType result = E_NOT_OK;
    const SomeIpXf_TransformerConfigType* transPtr;
    const SomeIpXf_DataElementConfigType* elemPtr;
    uint16 headerSize = 0U;
    uint32 payloadOffset = 0U;

#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_UNINIT);
        return E_NOT_OK;
    }

    if ((SourceBuffer == NULL_PTR) || (TargetBuffer == NULL_PTR))
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (!SOMEIPXF_IS_VALID_TRANSFORMER_ID(TransformerId))
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
#endif

    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
    {
        transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];

        /* Add SOME/IP header if enabled */
        if (transPtr->HeaderIncluded)
        {
            if (TargetBuffer->MaxLength >= SOMEIPXF_HDR_SIZE)
            {
                SomeIpXf_HeaderType header;
                header.ServiceId = transPtr->InterfaceConfig->ServiceId;
                header.MethodId = transPtr->InterfaceConfig->MethodId;
                header.Length = (uint32)SourceBuffer->Length;
                header.ProtocolVersion = SOMEIPXF_PROTOCOL_VERSION;
                header.InterfaceVersion = SOMEIPXF_INTERFACE_VERSION;
                header.MessageType = transPtr->InterfaceConfig->MessageType;
                header.ReturnCode = transPtr->InterfaceConfig->ReturnCode;

                (void)SomeIpXf_BuildHeader(&header, TargetBuffer->Data);
                headerSize = SOMEIPXF_HDR_SIZE;
            }
        }

        /* Serialize data element */
        if (DataElementId < transPtr->NumDataElements)
        {
            elemPtr = &transPtr->DataElements[DataElementId];
            payloadOffset = headerSize;

            switch (elemPtr->DataType)
            {
                case SOMEIPXF_DT_BOOLEAN:
                    if ((SourceBuffer->Length >= 1U) && 
                        (TargetBuffer->MaxLength >= (payloadOffset + 1U)))
                    {
                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0] ? 1U : 0U;
                        TargetBuffer->Length = payloadOffset + 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT8:
                    if ((SourceBuffer->Length >= 1U) && 
                        (TargetBuffer->MaxLength >= (payloadOffset + 1U)))
                    {
                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0];
                        TargetBuffer->Length = payloadOffset + 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT16:
                    if ((SourceBuffer->Length >= 2U) && 
                        (TargetBuffer->MaxLength >= (payloadOffset + 2U)))
                    {
                        SOMEIPXF_PUT_U16_BE(TargetBuffer->Data, payloadOffset, 
                                           ((uint16)SourceBuffer->Data[0] << 8) | SourceBuffer->Data[1]);
                        TargetBuffer->Length = payloadOffset + 2U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT32:
                    if ((SourceBuffer->Length >= 4U) && 
                        (TargetBuffer->MaxLength >= (payloadOffset + 4U)))
                    {
                        uint32 value = ((uint32)SourceBuffer->Data[0] << 24) |
                                      ((uint32)SourceBuffer->Data[1] << 16) |
                                      ((uint32)SourceBuffer->Data[2] << 8) |
                                      (uint32)SourceBuffer->Data[3];
                        SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, value);
                        TargetBuffer->Length = payloadOffset + 4U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_STRING:
                    {
                        uint32 strLen = SourceBuffer->Length;
                        if (strLen > SOMEIPXF_MAX_STRING_LENGTH)
                        {
                            strLen = SOMEIPXF_MAX_STRING_LENGTH;
                        }
                        
                        if (TargetBuffer->MaxLength >= (payloadOffset + strLen + 4U))
                        {
                            /* Add length field */
                            SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, strLen);
                            /* Copy string data */
                            (void)memcpy(&TargetBuffer->Data[payloadOffset + 4U], 
                                        SourceBuffer->Data, strLen);
                            TargetBuffer->Length = payloadOffset + 4U + strLen;
                            result = E_OK;
                        }
                    }
                    break;

                case SOMEIPXF_DT_ARRAY:
                    if (TargetBuffer->MaxLength >= (payloadOffset + SourceBuffer->Length + 4U))
                    {
                        /* Add length field */
                        SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, payloadOffset, 
                                           (uint32)SourceBuffer->Length);
                        /* Copy array data */
                        (void)memcpy(&TargetBuffer->Data[payloadOffset + 4U], 
                                    SourceBuffer->Data, SourceBuffer->Length);
                        TargetBuffer->Length = payloadOffset + 4U + (uint32)SourceBuffer->Length;
                        result = E_OK;
                    }
                    break;

                default:
                    /* Unsupported data type */
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
                    SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_TRANSFORM, SOMEIPXF_E_INVALID_DATA_TYPE);
#endif
                    break;
            }

            /* Update SOME/IP header length field */
            if ((result == E_OK) && (transPtr->HeaderIncluded))
            {
                uint32 payloadLen = TargetBuffer->Length - SOMEIPXF_HDR_SIZE;
                SOMEIPXF_PUT_U32_BE(TargetBuffer->Data, SOMEIPXF_HDR_LENGTH_OFFSET, payloadLen);
            }
        }
    }

    return result;
}

/**
 * @brief   De-transforms data from SOME/IP format
 */
Std_ReturnType SomeIpXf_Detransform(uint16 TransformerId, uint16 DataElementId,
                                     const SomeIpXf_BufferType* SourceBuffer,
                                     SomeIpXf_BufferType* TargetBuffer)
{
    Std_ReturnType result = E_NOT_OK;
    const SomeIpXf_TransformerConfigType* transPtr;
    const SomeIpXf_DataElementConfigType* elemPtr;
    uint16 headerSize = 0U;
    uint32 payloadOffset = 0U;
    SomeIpXf_HeaderType header;

#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpXf_InternalState.State != SOMEIPXF_STATE_INIT)
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_UNINIT);
        return E_NOT_OK;
    }

    if ((SourceBuffer == NULL_PTR) || (TargetBuffer == NULL_PTR))
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (!SOMEIPXF_IS_VALID_TRANSFORMER_ID(TransformerId))
    {
        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
#endif

    if (SomeIpXf_InternalState.ConfigPtr != NULL_PTR)
    {
        transPtr = &SomeIpXf_InternalState.ConfigPtr->TransformerConfigs[TransformerId];

        /* Parse SOME/IP header if present */
        if (transPtr->HeaderIncluded)
        {
            if (SourceBuffer->Length >= SOMEIPXF_HDR_SIZE)
            {
                if (SomeIpXf_ParseHeader(SourceBuffer->Data, &header) == E_OK)
                {
                    /* Validate header */
                    if (header.ProtocolVersion != SOMEIPXF_PROTOCOL_VERSION)
                    {
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
                        SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, 
                                                   SOMEIPXF_E_WRONG_PROTOCOL_VERSION);
#endif
                        return E_NOT_OK;
                    }

                    if (header.ReturnCode != SOMEIPXF_RET_CODE_OK)
                    {
                        return E_NOT_OK;
                    }

                    headerSize = SOMEIPXF_HDR_SIZE;
                }
            }
        }

        /* Deserialize data element */
        if (DataElementId < transPtr->NumDataElements)
        {
            elemPtr = &transPtr->DataElements[DataElementId];
            payloadOffset = headerSize;

            switch (elemPtr->DataType)
            {
                case SOMEIPXF_DT_BOOLEAN:
                    if ((SourceBuffer->Length >= (payloadOffset + 1U)) && 
                        (TargetBuffer->MaxLength >= 1U))
                    {
                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset] ? 1U : 0U;
                        TargetBuffer->Length = 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT8:
                    if ((SourceBuffer->Length >= (payloadOffset + 1U)) && 
                        (TargetBuffer->MaxLength >= 1U))
                    {
                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset];
                        TargetBuffer->Length = 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT16:
                    if ((SourceBuffer->Length >= (payloadOffset + 2U)) && 
                        (TargetBuffer->MaxLength >= 2U))
                    {
                        uint16 value = SOMEIPXF_GET_U16_BE(SourceBuffer->Data, payloadOffset);
                        TargetBuffer->Data[0] = (uint8)(value >> 8);
                        TargetBuffer->Data[1] = (uint8)(value & 0xFFU);
                        TargetBuffer->Length = 2U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT32:
                    if ((SourceBuffer->Length >= (payloadOffset + 4U)) && 
                        (TargetBuffer->MaxLength >= 4U))
                    {
                        uint32 value = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
                        TargetBuffer->Data[0] = (uint8)(value >> 24);
                        TargetBuffer->Data[1] = (uint8)(value >> 16);
                        TargetBuffer->Data[2] = (uint8)(value >> 8);
                        TargetBuffer->Data[3] = (uint8)(value & 0xFFU);
                        TargetBuffer->Length = 4U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_STRING:
                    if (SourceBuffer->Length >= (payloadOffset + 4U))
                    {
                        uint32 strLen = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
                        if ((strLen <= SOMEIPXF_MAX_STRING_LENGTH) &&
                            (SourceBuffer->Length >= (payloadOffset + 4U + strLen)) &&
                            (TargetBuffer->MaxLength >= strLen))
                        {
                            (void)memcpy(TargetBuffer->Data, &SourceBuffer->Data[payloadOffset + 4U], strLen);
                            TargetBuffer->Length = strLen;
                            result = E_OK;
                        }
                    }
                    break;

                case SOMEIPXF_DT_UINT64:
                case SOMEIPXF_DT_SINT64:
                case SOMEIPXF_DT_FLOAT32:
                case SOMEIPXF_DT_FLOAT64:
                    /* Unsupported on de-transform path for now */
                    result = E_NOT_OK;
                    break;

                default:
                    /* Unsupported data type */
                    result = E_NOT_OK;
                    break;
            }
        }
    }

    return result;
}

#define SOMEIPXF_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
