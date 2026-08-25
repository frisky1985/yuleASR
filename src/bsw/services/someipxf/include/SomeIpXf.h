/**
 * @file SomeIpXf.h
 * @brief SOME/IP Transformer module - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: SOME/IP Transformer (SOMEIPXF)
 * Module ID: 0x7BU
 * Layer: Service Layer
 */

#ifndef SOMEIPXF_H
#define SOMEIPXF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "SomeIpXf_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define SOMEIPXF_VENDOR_ID                      (0x01U) /* YuleTech Vendor ID */
#define SOMEIPXF_MODULE_ID                      (0x7BU) /* SOMEIPXF Module ID */
#define SOMEIPXF_INSTANCE_ID                    (0x00U)

#define SOMEIPXF_AR_RELEASE_MAJOR_VERSION       (0x22U)
#define SOMEIPXF_AR_RELEASE_MINOR_VERSION       (0x11U)
#define SOMEIPXF_AR_RELEASE_REVISION_VERSION    (0x00U)

#define SOMEIPXF_SW_MAJOR_VERSION               (0x04U)
#define SOMEIPXF_SW_MINOR_VERSION               (0x07U)
#define SOMEIPXF_SW_PATCH_VERSION               (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define SOMEIPXF_SID_INIT                       (0x01U)
#define SOMEIPXF_SID_DEINIT                     (0x02U)
#define SOMEIPXF_SID_GETVERSIONINFO             (0x03U)
#define SOMEIPXF_SID_TRANSFORM                  (0x04U)
#define SOMEIPXF_SID_DETRANSFORM                (0x05U)
#define SOMEIPXF_SID_TRANSFORMERINIT            (0x06U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define SOMEIPXF_E_PARAM_POINTER                (0x01U)
#define SOMEIPXF_E_PARAM_CONFIG                 (0x02U)
#define SOMEIPXF_E_UNINIT                       (0x03U)
#define SOMEIPXF_E_ALREADY_INITIALIZED          (0x04U)
#define SOMEIPXF_E_INVALID_BUFFER_SIZE          (0x05U)
#define SOMEIPXF_E_SERIALIZATION_ERROR          (0x06U)
#define SOMEIPXF_E_DESERIALIZATION_ERROR        (0x07U)
#define SOMEIPXF_E_INVALID_DATA_TYPE            (0x08U)
#define SOMEIPXF_E_BUFFER_OVERFLOW              (0x09U)
#define SOMEIPXF_E_WRONG_INTERFACE_VERSION      (0x0AU)
#define SOMEIPXF_E_WRONG_MESSAGE_TYPE           (0x0BU)
#define SOMEIPXF_E_UNKNOWN_SERVICE              (0x0CU)
#define SOMEIPXF_E_UNKNOWN_METHOD               (0x0DU)
#define SOMEIPXF_E_WRONG_PROTOCOL_VERSION       (0x0EU)

/*==================================================================================================
*                                    SOME/IP MESSAGE TYPES
==================================================================================================*/
#define SOMEIPXF_MSG_TYPE_REQUEST               (0x00U)
#define SOMEIPXF_MSG_TYPE_REQUEST_NO_RETURN     (0x01U)
#define SOMEIPXF_MSG_TYPE_NOTIFICATION          (0x02U)
#define SOMEIPXF_MSG_TYPE_RESPONSE              (0x80U)
#define SOMEIPXF_MSG_TYPE_ERROR                 (0x81U)

/*==================================================================================================
*                                    SOME/IP RETURN CODES
==================================================================================================*/
#define SOMEIPXF_RET_CODE_OK                    (0x00U)
#define SOMEIPXF_RET_CODE_NOT_OK                (0x01U)
#define SOMEIPXF_RET_CODE_UNKNOWN_SERVICE       (0x02U)
#define SOMEIPXF_RET_CODE_UNKNOWN_METHOD        (0x03U)
#define SOMEIPXF_RET_CODE_NOT_READY             (0x04U)
#define SOMEIPXF_RET_CODE_NOT_REACHABLE         (0x05U)
#define SOMEIPXF_RET_CODE_TIMEOUT               (0x06U)
#define SOMEIPXF_RET_CODE_WRONG_PROTOCOL_VER    (0x07U)
#define SOMEIPXF_RET_CODE_WRONG_INTERFACE_VER   (0x08U)
#define SOMEIPXF_RET_CODE_MALFORMED_MSG         (0x09U)
#define SOMEIPXF_RET_CODE_WRONG_MESSAGE_TYPE    (0x0AU)
#define SOMEIPXF_RET_CODE_E2E_REPEATED          (0x0BU)
#define SOMEIPXF_RET_CODE_E2E_WRONG_SEQ         (0x0CU)
#define SOMEIPXF_RET_CODE_E2E_ERROR             (0x0DU)
#define SOMEIPXF_RET_CODE_E2E_NOT_AVAIL         (0x0EU)
#define SOMEIPXF_RET_CODE_E2E_NO_NEW_DATA       (0x0FU)

/*==================================================================================================
*                                    DATA TYPES FOR SERIALIZATION
==================================================================================================*/
typedef enum {
    SOMEIPXF_DT_BOOLEAN = 0,
    SOMEIPXF_DT_UINT8,
    SOMEIPXF_DT_UINT16,
    SOMEIPXF_DT_UINT32,
    SOMEIPXF_DT_UINT64,
    SOMEIPXF_DT_SINT8,
    SOMEIPXF_DT_SINT16,
    SOMEIPXF_DT_SINT32,
    SOMEIPXF_DT_SINT64,
    SOMEIPXF_DT_FLOAT32,
    SOMEIPXF_DT_FLOAT64,
    SOMEIPXF_DT_STRING,
    SOMEIPXF_DT_ARRAY,
    SOMEIPXF_DT_STRUCT,
    SOMEIPXF_DT_UNION
} SomeIpXf_DataTypeType;

/*==================================================================================================
*                                    STRING CONFIGURATION
==================================================================================================*/
typedef enum {
    SOMEIPXF_STR_UTF8 = 0,
    SOMEIPXF_STR_UTF16,
    SOMEIPXF_STR_UTF32
} SomeIpXf_StringCodingType;

typedef enum {
    SOMEIPXF_STR_LEN_FIXED = 0,
    SOMEIPXF_STR_LEN_SIZE_FIELD_8,
    SOMEIPXF_STR_LEN_SIZE_FIELD_16,
    SOMEIPXF_STR_LEN_SIZE_FIELD_32,
    SOMEIPXF_STR_LEN_TERMINATION
} SomeIpXf_StringLenType;

/*==================================================================================================
*                                    ARRAY CONFIGURATION
==================================================================================================*/
typedef enum {
    SOMEIPXF_ARRAY_LEN_FIXED = 0,
    SOMEIPXF_ARRAY_LEN_SIZE_FIELD_8,
    SOMEIPXF_ARRAY_LEN_SIZE_FIELD_16,
    SOMEIPXF_ARRAY_LEN_SIZE_FIELD_32
} SomeIpXf_ArrayLenType;

/*==================================================================================================
*                                    DATA ELEMENT CONFIG TYPE
==================================================================================================*/
typedef struct {
    SomeIpXf_DataTypeType DataType;
    uint16 BitSize;
    uint16 Alignment;
    boolean IsDynamic;
    boolean IsArray;
    uint16 ArraySize;
    SomeIpXf_ArrayLenType ArrayLenType;
    SomeIpXf_StringCodingType StringCoding;
    SomeIpXf_StringLenType StringLenType;
    uint16 StringMaxLen;
    uint16 WireType; /* For unions */
} SomeIpXf_DataElementConfigType;

/*==================================================================================================
*                                    INTERFACE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 ServiceId;
    uint16 MethodId;
    uint8 InterfaceVersion;
    uint8 ProtocolVersion;
    uint16 DataLength;
    uint8 MessageType;
    uint8 ReturnCode;
} SomeIpXf_InterfaceConfigType;

/*==================================================================================================
*                                    SOME/IP HEADER TYPE
==================================================================================================*/
typedef struct {
    uint16 ServiceId;
    uint16 MethodId;
    uint32 Length;
    uint8 ProtocolVersion;
    uint8 InterfaceVersion;
    uint8 MessageType;
    uint8 ReturnCode;
} SomeIpXf_HeaderType;

/*==================================================================================================
*                                    BUFFER TYPE
==================================================================================================*/
typedef struct {
    uint8* Data;
    uint32 Length;
    uint32 MaxLength;
} SomeIpXf_BufferType;

/*==================================================================================================
*                                    TRANSFORMER CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 TransformerId;
    const SomeIpXf_InterfaceConfigType* InterfaceConfig;
    const SomeIpXf_DataElementConfigType* DataElements;
    uint16 NumDataElements;
    boolean HeaderIncluded;
} SomeIpXf_TransformerConfigType;

/*==================================================================================================
*                                    SOMEIPXF CONFIG TYPE
==================================================================================================*/
typedef struct {
    const SomeIpXf_TransformerConfigType* TransformerConfigs;
    uint16 NumTransformers;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean EnableE2E;
} SomeIpXf_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define SOMEIPXF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const SomeIpXf_ConfigType SomeIpXf_Config;

#define SOMEIPXF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define SOMEIPXF_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_SomeIpXf_00001 */
/**
 * @brief Initializes the SOME/IP Transformer module
 * @param ConfigPtr Pointer to configuration structure
 */
void SomeIpXf_Init(const SomeIpXf_ConfigType* ConfigPtr);

/** @req SWS_SomeIpXf_00002 */
/**
 * @brief Deinitializes the SOME/IP Transformer module
 */
void SomeIpXf_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (SOMEIPXF_VERSION_INFO_API == STD_ON)
/** @req SWS_SomeIpXf_00003 */
void SomeIpXf_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @req SWS_SomeIpXf_00004 */
/**
 * @brief Transforms data to SOME/IP format
 * @param TransformerId Transformer configuration ID
 * @param DataElementId Data element ID
 * @param SourceBuffer Source data buffer
 * @param TargetBuffer Target buffer for serialized data
 * @return Result of operation
 */
Std_ReturnType SomeIpXf_Transform(uint16 TransformerId, uint16 DataElementId,
                                   const SomeIpXf_BufferType* SourceBuffer,
                                   SomeIpXf_BufferType* TargetBuffer);

/** @req SWS_SomeIpXf_00005 */
/**
 * @brief De-transforms data from SOME/IP format
 * @param TransformerId Transformer configuration ID
 * @param DataElementId Data element ID
 * @param SourceBuffer Source buffer with serialized data
 * @param TargetBuffer Target buffer for deserialized data
 * @return Result of operation
 */
Std_ReturnType SomeIpXf_Detransform(uint16 TransformerId, uint16 DataElementId,
                                     const SomeIpXf_BufferType* SourceBuffer,
                                     SomeIpXf_BufferType* TargetBuffer);

/** @req SWS_SomeIpXf_00006 */
/**
 * @brief Initializes transformer for a specific interface
 * @param TransformerId Transformer configuration ID
 * @param HeaderPtr Pointer to SOME/IP header
 * @return Result of operation
 */
Std_ReturnType SomeIpXf_TransformerInit(uint16 TransformerId, SomeIpXf_HeaderType* HeaderPtr);

/** @req SWS_SomeIpXf_00007 */
/**
 * @brief Serializes a boolean value
 * @param Value Boolean value
 * @param Buffer Target buffer
 * @param Offset Bit offset
 * @return Number of bits serialized
 */
uint16 SomeIpXf_SerializeBoolean(boolean Value, uint8* Buffer, uint16 Offset);

/** @req SWS_SomeIpXf_00008 */
/**
 * @brief Deserializes a boolean value
 * @param Buffer Source buffer
 * @param Offset Bit offset
 * @param Value Output value
 * @return Number of bits deserialized
 */
uint16 SomeIpXf_DeserializeBoolean(const uint8* Buffer, uint16 Offset, boolean* Value);

/** @req SWS_SomeIpXf_00009 */
/**
 * @brief Serializes a uint8 value
 * @param Value uint8 value
 * @param Buffer Target buffer
 * @param Offset Bit offset
 * @return Number of bits serialized
 */
uint16 SomeIpXf_SerializeUint8(uint8 Value, uint8* Buffer, uint16 Offset);

/** @req SWS_SomeIpXf_00010 */
/**
 * @brief Deserializes a uint8 value
 * @param Buffer Source buffer
 * @param Offset Bit offset
 * @param Value Output value
 * @return Number of bits deserialized
 */
uint16 SomeIpXf_DeserializeUint8(const uint8* Buffer, uint16 Offset, uint8* Value);

/** @req SWS_SomeIpXf_00011 */
/**
 * @brief Serializes a uint16 value (big-endian)
 * @param Value uint16 value
 * @param Buffer Target buffer
 * @param Offset Bit offset
 * @return Number of bits serialized
 */
uint16 SomeIpXf_SerializeUint16(uint16 Value, uint8* Buffer, uint16 Offset);

/** @req SWS_SomeIpXf_00012 */
/**
 * @brief Deserializes a uint16 value (big-endian)
 * @param Buffer Source buffer
 * @param Offset Bit offset
 * @param Value Output value
 * @return Number of bits deserialized
 */
uint16 SomeIpXf_DeserializeUint16(const uint8* Buffer, uint16 Offset, uint16* Value);

/** @req SWS_SomeIpXf_00013 */
/**
 * @brief Serializes a uint32 value (big-endian)
 * @param Value uint32 value
 * @param Buffer Target buffer
 * @param Offset Bit offset
 * @return Number of bits serialized
 */
uint16 SomeIpXf_SerializeUint32(uint32 Value, uint8* Buffer, uint16 Offset);

/** @req SWS_SomeIpXf_00014 */
/**
 * @brief Deserializes a uint32 value (big-endian)
 * @param Buffer Source buffer
 * @param Offset Bit offset
 * @param Value Output value
 * @return Number of bits deserialized
 */
uint16 SomeIpXf_DeserializeUint32(const uint8* Buffer, uint16 Offset, uint32* Value);

/** @req SWS_SomeIpXf_00015 */
/**
 * @brief Serializes a string
 * @param StringPtr String pointer
 * @param StringLen String length
 * @param Buffer Target buffer
 * @param Config String configuration
 * @return Number of bytes serialized
 */
uint32 SomeIpXf_SerializeString(const uint8* StringPtr, uint32 StringLen,
                                 uint8* Buffer, const SomeIpXf_DataElementConfigType* Config);

/** @req SWS_SomeIpXf_00016 */
/**
 * @brief Deserializes a string
 * @param Buffer Source buffer
 * @param BufferLen Buffer length
 * @param StringPtr Output string pointer
 * @param StringLen Output string length
 * @param Config String configuration
 * @return Number of bytes deserialized
 */
uint32 SomeIpXf_DeserializeString(const uint8* Buffer, uint32 BufferLen,
                                   uint8* StringPtr, uint32* StringLen,
                                   const SomeIpXf_DataElementConfigType* Config);

/** @req SWS_SomeIpXf_00017 */
/**
 * @brief Serializes an array
 * @param ArrayPtr Array pointer
 * @param ArrayLen Array length
 * @param ElementSize Element size in bytes
 * @param Buffer Target buffer
 * @param Config Array configuration
 * @return Number of bytes serialized
 */
uint32 SomeIpXf_SerializeArray(const uint8* ArrayPtr, uint32 ArrayLen, uint32 ElementSize,
                                uint8* Buffer, const SomeIpXf_DataElementConfigType* Config);

/** @req SWS_SomeIpXf_00018 */
/**
 * @brief Deserializes an array
 * @param Buffer Source buffer
 * @param BufferLen Buffer length
 * @param ArrayPtr Output array pointer
 * @param ArrayLen Output array length
 * @param ElementSize Element size in bytes
 * @param Config Array configuration
 * @return Number of bytes deserialized
 */
uint32 SomeIpXf_DeserializeArray(const uint8* Buffer, uint32 BufferLen,
                                  uint8* ArrayPtr, uint32* ArrayLen,
                                  uint32 ElementSize,
                                  const SomeIpXf_DataElementConfigType* Config);

/** @req SWS_SomeIpXf_00019 */
/**
 * @brief Builds SOME/IP header
 * @param Header Header structure
 * @param Buffer Target buffer (must be at least 8 bytes)
 * @return Result of operation
 */
Std_ReturnType SomeIpXf_BuildHeader(const SomeIpXf_HeaderType* Header, uint8* Buffer);

/** @req SWS_SomeIpXf_00020 */
/**
 * @brief Parses SOME/IP header
 * @param Buffer Source buffer (must be at least 8 bytes)
 * @param Header Output header structure
 * @return Result of operation
 */
Std_ReturnType SomeIpXf_ParseHeader(const uint8* Buffer, SomeIpXf_HeaderType* Header);

#define SOMEIPXF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* SOMEIPXF_H */
