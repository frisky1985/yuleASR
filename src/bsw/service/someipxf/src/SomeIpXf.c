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
                        (TargetBuffer->MaxLength >= payloadOffset + 1U))
                    {
                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0] ? 1U : 0U;
                        TargetBuffer->Length = payloadOffset + 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT8:
                    if ((SourceBuffer->Length >= 1U) && 
                        (TargetBuffer->MaxLength >= payloadOffset + 1U))
                    {
                        TargetBuffer->Data[payloadOffset] = SourceBuffer->Data[0];
                        TargetBuffer->Length = payloadOffset + 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT16:
                    if ((SourceBuffer->Length >= 2U) && 
                        (TargetBuffer->MaxLength >= payloadOffset + 2U))
                    {
                        SOMEIPXF_PUT_U16_BE(TargetBuffer->Data, payloadOffset, 
                                           ((uint16)SourceBuffer->Data[0] << 8) | SourceBuffer->Data[1]);
                        TargetBuffer->Length = payloadOffset + 2U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT32:
                    if ((SourceBuffer->Length >= 4U) && 
                        (TargetBuffer->MaxLength >= payloadOffset + 4U))
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
                        
                        if (TargetBuffer->MaxLength >= payloadOffset + strLen + 4U)
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
                    if (TargetBuffer->MaxLength >= payloadOffset + SourceBuffer->Length + 4U)
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
                    if ((SourceBuffer->Length >= payloadOffset + 1U) && 
                        (TargetBuffer->MaxLength >= 1U))
                    {
                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset] ? 1U : 0U;
                        TargetBuffer->Length = 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT8:
                    if ((SourceBuffer->Length >= payloadOffset + 1U) && 
                        (TargetBuffer->MaxLength >= 1U))
                    {
                        TargetBuffer->Data[0] = SourceBuffer->Data[payloadOffset];
                        TargetBuffer->Length = 1U;
                        result = E_OK;
                    }
                    break;

                case SOMEIPXF_DT_UINT16:
                    if ((SourceBuffer->Length >= payloadOffset + 2U) && 
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
                    if ((SourceBuffer->Length >= payloadOffset + 4U) && 
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
                    if (SourceBuffer->Length >= payloadOffset + 4U)
                    {
                        uint32 strLen = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
                        if ((strLen <= SOMEIPXF_MAX_STRING_LENGTH) && 
                            (SourceBuffer->Length >= payloadOffset + 4U + strLen) &&
                            (TargetBuffer->MaxLength >= strLen))
                        {
                            (void)memcpy(TargetBuffer->Data, 
                                        &SourceBuffer->Data[payloadOffset + 4U], strLen);
                            TargetBuffer->Length = strLen;
                            result = E_OK;
                        }
                    }
                    break;

                case SOMEIPXF_DT_ARRAY:
                    if (SourceBuffer->Length >= payloadOffset + 4U)
                    {
                        uint32 arrLen = SOMEIPXF_GET_U32_BE(SourceBuffer->Data, payloadOffset);
                        if ((arrLen <= TargetBuffer->MaxLength) && 
                            (SourceBuffer->Length >= payloadOffset + 4U + arrLen))
                        {
                            (void)memcpy(TargetBuffer->Data, 
                                        &SourceBuffer->Data[payloadOffset + 4U], arrLen);
                            TargetBuffer->Length = arrLen;
                            result = E_OK;
                        }
                    }
                    break;

                default:
                    /* Unsupported data type */
#if (SOMEIPXF_DEV_ERROR_DETECT == STD_ON)
                    SOMEIPXF_DET_REPORT_ERROR(SOMEIPXF_SID_DETRANSFORM, SOMEIPXF_E_INVALID_DATA_TYPE);
#endif
                    break;
            }
        }
    }

    return result;
}

/**
 * @brief   Serializes a boolean value
 */
uint16 SomeIpXf_SerializeBoolean(boolean Value, uint8* Buffer, uint16 Offset)
{
    if (Buffer != NULL_PTR)
    {
        Buffer[Offset / 8U] = Value ? 0x01U : 0x00U;
    }
    return 8U; /* 1 byte */
}

/**
 * @brief   Deserializes a boolean value
 */
uint16 SomeIpXf_DeserializeBoolean(const uint8* Buffer, uint16 Offset, boolean* Value)
{
    if ((Buffer != NULL_PTR) && (Value != NULL_PTR))
    {
        *Value = (Buffer[Offset / 8U] != 0U) ? TRUE : FALSE;
    }
    return 8U; /* 1 byte */
}

/**
 * @brief   Serializes a uint8 value
 */
uint16 SomeIpXf_SerializeUint8(uint8 Value, uint8* Buffer, uint16 Offset)
{
    if (Buffer != NULL_PTR)
    {
        Buffer[Offset / 8U] = Value;
    }
    return 8U; /* 1 byte */
}

/**
 * @brief   Deserializes a uint8 value
 */
uint16 SomeIpXf_DeserializeUint8(const uint8* Buffer, uint16 Offset, uint8* Value)
{
    if ((Buffer != NULL_PTR) && (Value != NULL_PTR))
    {
        *Value = Buffer[Offset / 8U];
    }
    return 8U; /* 1 byte */
}

/**
 * @brief   Serializes a uint16 value (big-endian)
 */
uint16 SomeIpXf_SerializeUint16(uint16 Value, uint8* Buffer, uint16 Offset)
{
    uint16 byteOffset = Offset / 8U;
    if (Buffer != NULL_PTR)
    {
        Buffer[byteOffset] = (uint8)(Value >> 8);
        Buffer[byteOffset + 1U] = (uint8)(Value & 0xFFU);
    }
    return 16U; /* 2 bytes */
}

/**
 * @brief   Deserializes a uint16 value (big-endian)
 */
uint16 SomeIpXf_DeserializeUint16(const uint8* Buffer, uint16 Offset, uint16* Value)
{
    uint16 byteOffset = Offset / 8U;
    if ((Buffer != NULL_PTR) && (Value != NULL_PTR))
    {
        *Value = ((uint16)Buffer[byteOffset] << 8) | (uint16)Buffer[byteOffset + 1U];
    }
    return 16U; /* 2 bytes */
}

/**
 * @brief   Serializes a uint32 value (big-endian)
 */
uint16 SomeIpXf_SerializeUint32(uint32 Value, uint8* Buffer, uint16 Offset)
{
    uint16 byteOffset = Offset / 8U;
    if (Buffer != NULL_PTR)
    {
        Buffer[byteOffset] = (uint8)(Value >> 24);
        Buffer[byteOffset + 1U] = (uint8)(Value >> 16);
        Buffer[byteOffset + 2U] = (uint8)(Value >> 8);
        Buffer[byteOffset + 3U] = (uint8)(Value & 0xFFU);
    }
    return 32U; /* 4 bytes */
}

/**
 * @brief   Deserializes a uint32 value (big-endian)
 */
uint16 SomeIpXf_DeserializeUint32(const uint8* Buffer, uint16 Offset, uint32* Value)
{
    uint16 byteOffset = Offset / 8U;
    if ((Buffer != NULL_PTR) && (Value != NULL_PTR))
    {
        *Value = ((uint32)Buffer[byteOffset] << 24) |
                 ((uint32)Buffer[byteOffset + 1U] << 16) |
                 ((uint32)Buffer[byteOffset + 2U] << 8) |
                 (uint32)Buffer[byteOffset + 3U];
    }
    return 32U; /* 4 bytes */
}

/**
 * @brief   Serializes a string
 */
uint32 SomeIpXf_SerializeString(const uint8* StringPtr, uint32 StringLen,
                                 uint8* Buffer, const SomeIpXf_DataElementConfigType* Config)
{
    uint32 totalLen = 0U;

    if ((StringPtr != NULL_PTR) && (Buffer != NULL_PTR) && (Config != NULL_PTR))
    {
        /* Add size field based on configuration */
        switch (Config->StringLenType)
        {
            case SOMEIPXF_STR_LEN_SIZE_FIELD_8:
                Buffer[0] = (uint8)StringLen;
                totalLen = 1U;
                break;
            case SOMEIPXF_STR_LEN_SIZE_FIELD_16:
                SOMEIPXF_PUT_U16_BE(Buffer, 0, (uint16)StringLen);
                totalLen = 2U;
                break;
            case SOMEIPXF_STR_LEN_SIZE_FIELD_32:
                SOMEIPXF_PUT_U32_BE(Buffer, 0, StringLen);
                totalLen = 4U;
                break;
            case SOMEIPXF_STR_LEN_FIXED:
            case SOMEIPXF_STR_LEN_TERMINATION:
            default:
                break;
        }

        /* Copy string data */
        (void)memcpy(&Buffer[totalLen], StringPtr, StringLen);
        totalLen += StringLen;

        /* Add null termination if needed */
        if (Config->StringLenType == SOMEIPXF_STR_LEN_TERMINATION)
        {
            Buffer[totalLen] = 0U;
            totalLen++;
        }
    }

    return totalLen;
}

/**
 * @brief   Deserializes a string
 */
uint32 SomeIpXf_DeserializeString(const uint8* Buffer, uint32 BufferLen,
                                   uint8* StringPtr, uint32* StringLen,
                                   const SomeIpXf_DataElementConfigType* Config)
{
    uint32 headerLen = 0U;
    uint32 dataLen = 0U;

    if ((Buffer != NULL_PTR) && (StringPtr != NULL_PTR) && (StringLen != NULL_PTR) && (Config != NULL_PTR))
    {
        /* Parse size field based on configuration */
        switch (Config->StringLenType)
        {
            case SOMEIPXF_STR_LEN_SIZE_FIELD_8:
                if (BufferLen >= 1U)
                {
                    dataLen = Buffer[0];
                    headerLen = 1U;
                }
                break;
            case SOMEIPXF_STR_LEN_SIZE_FIELD_16:
                if (BufferLen >= 2U)
                {
                    dataLen = SOMEIPXF_GET_U16_BE(Buffer, 0);
                    headerLen = 2U;
                }
                break;
            case SOMEIPXF_STR_LEN_SIZE_FIELD_32:
                if (BufferLen >= 4U)
                {
                    dataLen = SOMEIPXF_GET_U32_BE(Buffer, 0);
                    headerLen = 4U;
                }
                break;
            case SOMEIPXF_STR_LEN_FIXED:
                dataLen = Config->StringMaxLen;
                headerLen = 0U;
                break;
            case SOMEIPXF_STR_LEN_TERMINATION:
            default:
                /* Find null terminator */
                for (dataLen = 0U; dataLen < BufferLen; dataLen++)
                {
                    if (Buffer[dataLen] == 0U)
                    {
                        break;
                    }
                }
                headerLen = 0U;
                break;
        }

        /* Copy string data */
        if (dataLen > *StringLen)
        {
            dataLen = *StringLen;
        }
        
        if (BufferLen >= headerLen + dataLen)
        {
            (void)memcpy(StringPtr, &Buffer[headerLen], dataLen);
            *StringLen = dataLen;
        }
    }

    return headerLen + dataLen;
}

/**
 * @brief   Serializes an array
 */
uint32 SomeIpXf_SerializeArray(const uint8* ArrayPtr, uint32 ArrayLen, uint32 ElementSize,
                                uint8* Buffer, const SomeIpXf_DataElementConfigType* Config)
{
    uint32 totalLen = 0U;
    uint32 dataLen = ArrayLen * ElementSize;

    if ((ArrayPtr != NULL_PTR) && (Buffer != NULL_PTR) && (Config != NULL_PTR))
    {
        /* Add size field based on configuration */
        switch (Config->ArrayLenType)
        {
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_8:
                Buffer[0] = (uint8)ArrayLen;
                totalLen = 1U;
                break;
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_16:
                SOMEIPXF_PUT_U16_BE(Buffer, 0, (uint16)ArrayLen);
                totalLen = 2U;
                break;
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_32:
                SOMEIPXF_PUT_U32_BE(Buffer, 0, ArrayLen);
                totalLen = 4U;
                break;
            case SOMEIPXF_ARRAY_LEN_FIXED:
            default:
                break;
        }

        /* Copy array data */
        (void)memcpy(&Buffer[totalLen], ArrayPtr, dataLen);
        totalLen += dataLen;
    }

    return totalLen;
}

/**
 * @brief   Deserializes an array
 */
uint32 SomeIpXf_DeserializeArray(const uint8* Buffer, uint32 BufferLen,
                                  uint8* ArrayPtr, uint32* ArrayLen,
                                  uint32 ElementSize,
                                  const SomeIpXf_DataElementConfigType* Config)
{
    uint32 headerLen = 0U;
    uint32 numElements = 0U;

    if ((Buffer != NULL_PTR) && (ArrayPtr != NULL_PTR) && (ArrayLen != NULL_PTR) && (Config != NULL_PTR))
    {
        /* Parse size field based on configuration */
        switch (Config->ArrayLenType)
        {
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_8:
                if (BufferLen >= 1U)
                {
                    numElements = Buffer[0];
                    headerLen = 1U;
                }
                break;
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_16:
                if (BufferLen >= 2U)
                {
                    numElements = SOMEIPXF_GET_U16_BE(Buffer, 0);
                    headerLen = 2U;
                }
                break;
            case SOMEIPXF_ARRAY_LEN_SIZE_FIELD_32:
                if (BufferLen >= 4U)
                {
                    numElements = SOMEIPXF_GET_U32_BE(Buffer, 0);
                    headerLen = 4U;
                }
                break;
            case SOMEIPXF_ARRAY_LEN_FIXED:
                numElements = Config->ArraySize;
                headerLen = 0U;
                break;
            default:
                break;
        }

        /* Copy array data */
        uint32 dataLen = numElements * ElementSize;
        if (dataLen > *ArrayLen * ElementSize)
        {
            numElements = *ArrayLen;
            dataLen = numElements * ElementSize;
        }
        
        if (BufferLen >= headerLen + dataLen)
        {
            (void)memcpy(ArrayPtr, &Buffer[headerLen], dataLen);
            *ArrayLen = numElements;
        }
    }

    return headerLen + (numElements * ElementSize);
}

/**
 * @brief   Builds SOME/IP header
 */
Std_ReturnType SomeIpXf_BuildHeader(const SomeIpXf_HeaderType* Header, uint8* Buffer)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Header != NULL_PTR) && (Buffer != NULL_PTR))
    {
        /* Service ID */
        SOMEIPXF_PUT_U16_BE(Buffer, SOMEIPXF_HDR_SERVICE_ID_OFFSET, Header->ServiceId);
        
        /* Method ID */
        SOMEIPXF_PUT_U16_BE(Buffer, SOMEIPXF_HDR_METHOD_ID_OFFSET, Header->MethodId);
        
        /* Length (8 bytes + payload) */
        SOMEIPXF_PUT_U32_BE(Buffer, SOMEIPXF_HDR_LENGTH_OFFSET, Header->Length);
        
        /* Protocol Version */
        Buffer[SOMEIPXF_HDR_PROTOCOL_VER_OFFSET] = Header->ProtocolVersion;
        
        /* Interface Version */
        Buffer[SOMEIPXF_HDR_INTERFACE_VER_OFFSET] = Header->InterfaceVersion;
        
        /* Message Type */
        Buffer[SOMEIPXF_HDR_MSG_TYPE_OFFSET] = Header->MessageType;
        
        /* Return Code */
        Buffer[SOMEIPXF_HDR_RETURN_CODE_OFFSET] = Header->ReturnCode;
        
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Parses SOME/IP header
 */
Std_ReturnType SomeIpXf_ParseHeader(const uint8* Buffer, SomeIpXf_HeaderType* Header)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Buffer != NULL_PTR) && (Header != NULL_PTR))
    {
        /* Service ID */
        Header->ServiceId = SOMEIPXF_GET_U16_BE(Buffer, SOMEIPXF_HDR_SERVICE_ID_OFFSET);
        
        /* Method ID */
        Header->MethodId = SOMEIPXF_GET_U16_BE(Buffer, SOMEIPXF_HDR_METHOD_ID_OFFSET);
        
        /* Length */
        Header->Length = SOMEIPXF_GET_U32_BE(Buffer, SOMEIPXF_HDR_LENGTH_OFFSET);
        
        /* Protocol Version */
        Header->ProtocolVersion = Buffer[SOMEIPXF_HDR_PROTOCOL_VER_OFFSET];
        
        /* Interface Version */
        Header->InterfaceVersion = Buffer[SOMEIPXF_HDR_INTERFACE_VER_OFFSET];
        
        /* Message Type */
        Header->MessageType = Buffer[SOMEIPXF_HDR_MSG_TYPE_OFFSET];
        
        /* Return Code */
        Header->ReturnCode = Buffer[SOMEIPXF_HDR_RETURN_CODE_OFFSET];
        
        result = E_OK;
    }

    return result;
}

#define SOMEIPXF_STOP_SEC_CODE
#include "MemMap.h"
