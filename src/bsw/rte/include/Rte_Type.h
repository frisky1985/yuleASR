/**
 * @file Rte_Type.h
 * @brief RTE Type Definitions following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Common type definitions for RTE interfaces
 */

#ifndef RTE_TYPE_H
#define RTE_TYPE_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Rte_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define RTE_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define RTE_MODULE_ID                   (0x70U) /* RTE Module ID */
#define RTE_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define RTE_AR_RELEASE_MINOR_VERSION    (0x04U)
#define RTE_AR_RELEASE_REVISION_VERSION (0x00U)
#define RTE_SW_MAJOR_VERSION            (0x01U)
#define RTE_SW_MINOR_VERSION            (0x00U)
#define RTE_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    RTE STATUS TYPE
==================================================================================================*/
typedef enum {
    RTE_E_OK = 0,
    RTE_E_NOK,
    RTE_E_INVALID,
    RTE_E_TIMEOUT,
    RTE_E_LIMIT,
    RTE_E_NO_DATA,
    RTE_E_TRANSMIT_ACK,
    RTE_E_RECEIVER_ERROR,
    RTE_E_COM_STOPPED,
    RTE_E_COM_RESTARTED,
    RTE_E_SEG_FAULT,
    RTE_E_OUT_OF_RANGE,
    RTE_E_SERIALIZATION_ERROR,
    RTE_E_HARD_TRANSFORMER_ERROR,
    RTE_E_SOFT_TRANSFORMER_ERROR,
    RTE_E_COM_BUSY,
    RTE_E_LOST_DATA,
    RTE_E_MAX_AGE_EXCEEDED,
    RTE_E_NEVER_RECEIVED,
    RTE_E_UNCONNECTED,
    RTE_E_IN_EXCLUSIVE_AREA,
    RTE_E_INTER_PARTITION_ROUTING_NOT_AVAILABLE,
    RTE_E_OK_PENDING,
    RTE_E_OK_RECEIVED,
    RTE_E_OK_SENDING,
    RTE_E_OK_SENT,
    RTE_E_OK_NO_DATA,
    RTE_E_OK_DATA_LOST
} Rte_StatusType;

/*==================================================================================================
*                                    RTE TRANSMISSION MODE TYPE
==================================================================================================*/
typedef enum {
    RTE_MODE_SYNCHRONOUS = 0,
    RTE_MODE_ASYNCHRONOUS
} Rte_TransmissionModeType;

/*==================================================================================================
*                                    RTE TRANSFORMATION STATUS
==================================================================================================*/
typedef enum {
    RTE_TRANSFORMER_GOOD = 0,
    RTE_TRANSFORMER_LIMIT,
    RTE_TRANSFORMER_SOMEIPXF_E_UNKNOWN_SERVICE,
    RTE_TRANSFORMER_SOMEIPXF_E_UNKNOWN_METHOD,
    RTE_TRANSFORMER_SOMEIPXF_E_NOT_READY,
    RTE_TRANSFORMER_SOMEIPXF_E_NOT_REACHABLE,
    RTE_TRANSFORMER_SOMEIPXF_E_TIMEOUT,
    RTE_TRANSFORMER_SOMEIPXF_E_WRONG_PROTOCOL_VERSION,
    RTE_TRANSFORMER_SOMEIPXF_E_WRONG_INTERFACE_VERSION,
    RTE_TRANSFORMER_SOMEIPXF_E_MALFORMED_MESSAGE,
    RTE_TRANSFORMER_SOMEIPXF_E_WRONG_MESSAGE_TYPE,
    RTE_TRANSFORMER_SOMEIPXF_E_E2E_REPEATED,
    RTE_TRANSFORMER_SOMEIPXF_E_E2E_WRONG_SEQUENCE,
    RTE_TRANSFORMER_SOMEIPXF_E_E2E,
    RTE_TRANSFORMER_SOMEIPXF_E_E2E_NOT_AVAILABLE,
    RTE_TRANSFORMER_SOMEIPXF_E_E2E_NO_NEW_DATA,
    RTE_TRANSFORMER_SOMEIPXF_E_WRONG_CLIENT_ID,
    RTE_TRANSFORMER_SOMEIPXF_E_WRONG_SESSION_ID
} Rte_TransformerResultType;

/*==================================================================================================
*                                    RTE APPLICATION MODE TYPE
==================================================================================================*/
typedef enum {
    RTE_MODE_APPMODE_NONE = 0,
    RTE_MODE_APPMODE_APP
} Rte_AppModeType;

/*==================================================================================================
*                                    RTE COMPONENT INSTANCE HANDLE
==================================================================================================*/
typedef uint16 Rte_InstanceHandleType;

/*==================================================================================================
*                                    RTE COMPONENT INSTANCE
==================================================================================================*/
typedef struct {
    Rte_InstanceHandleType handle;
    uint8 swcId;
    uint8 instanceId;
} Rte_ComponentInstanceType;

/*==================================================================================================
*                                    RTE EVENT TYPE
==================================================================================================*/
typedef uint8 Rte_EventType;

/*==================================================================================================
*                                    RTE EVENT MASKS
==================================================================================================*/
#define RTE_EVENT_NONE                  (0x00U)
#define RTE_EVENT_DATA_RECEIVED         (0x01U)
#define RTE_EVENT_DATA_SEND_COMPLETED   (0x02U)
#define RTE_EVENT_OPERATION_INVOKED     (0x04U)
#define RTE_EVENT_OPERATION_COMPLETED   (0x08U)
#define RTE_EVENT_MODE_SWITCHED         (0x10U)
#define RTE_EVENT_TIMING                (0x20U)
#define RTE_EVENT_ERROR                 (0x40U)
#define RTE_EVENT_ALL                   (0xFFU)

/*==================================================================================================
*                                    RTE ACTIVATION REASON TYPE
==================================================================================================*/
typedef uint8 Rte_ActivationReasonType;

/*==================================================================================================
*                                    RTE DATA HANDLE TYPE
==================================================================================================*/
typedef uint16 Rte_DataHandleType;

/*==================================================================================================
*                                    RTE CLIENT SERVER OPERATION HANDLE
==================================================================================================*/
typedef uint16 Rte_CSOperationHandleType;

/*==================================================================================================
*                                    RTE MODE HANDLE TYPE
==================================================================================================*/
typedef uint16 Rte_ModeHandleType;

/*==================================================================================================
*                                    RTE BUFFER TYPE
==================================================================================================*/
typedef struct {
    uint8* data;
    uint16 length;
    uint16 maxLength;
} Rte_BufferType;

/*==================================================================================================
*                                    RTE PARAMETER HANDLE TYPE
==================================================================================================*/
typedef uint16 Rte_ParameterHandleType;

/*==================================================================================================
*                                    RTE EXCLUSIVE AREA HANDLE TYPE
==================================================================================================*/
typedef uint16 Rte_ExclusiveAreaHandleType;

/*==================================================================================================
*                                    RTE IRV HANDLE TYPE (Inter-Runnable Variable)
==================================================================================================*/
typedef uint16 Rte_IrvHandleType;

/*==================================================================================================
*                                    RTE MEASUREMENT HANDLE TYPE
==================================================================================================*/
typedef uint16 Rte_MeasurementHandleType;

/*==================================================================================================
*                                    RTE CALPRM HANDLE TYPE (Calibration Parameter)
==================================================================================================*/
typedef uint16 Rte_CalPrmHandleType;

/*==================================================================================================
*                                    RTE PIM HANDLE TYPE (Per Instance Memory)
==================================================================================================*/
typedef uint16 Rte_PimHandleType;

/*==================================================================================================
*                                    RTE CALLBACK FUNCTION TYPES
==================================================================================================*/
typedef void (*Rte_CallbackType)(void);
typedef void (*Rte_DataReceivedCallbackType)(Rte_DataHandleType dataHandle);
typedef void (*Rte_OperationCompletedCallbackType)(Rte_CSOperationHandleType opHandle, Rte_StatusType status);
typedef void (*Rte_ModeSwitchedCallbackType)(Rte_ModeHandleType modeHandle, uint32 mode);

/*==================================================================================================
*                                    RTE BOOLEAN DATA TYPE
==================================================================================================*/
typedef boolean Rte_Boolean;

/*==================================================================================================
*                                    RTE INTEGER DATA TYPES
==================================================================================================*/
typedef sint8 Rte_SInt8;
typedef sint16 Rte_SInt16;
typedef sint32 Rte_SInt32;
typedef sint64 Rte_SInt64;

typedef uint8 Rte_UInt8;
typedef uint16 Rte_UInt16;
typedef uint32 Rte_UInt32;
typedef uint64 Rte_UInt64;

/*==================================================================================================
*                                    RTE FLOATING POINT DATA TYPES
==================================================================================================*/
typedef float32 Rte_Float32;
typedef float64 Rte_Float64;

/*==================================================================================================
*                                    RTE STRING DATA TYPE
==================================================================================================*/
typedef struct {
    uint8 data[RTE_MAX_STRING_LENGTH];
    uint16 length;
} Rte_StringType;

/*==================================================================================================
*                                    RTE ARRAY DATA TYPE
==================================================================================================*/
typedef struct {
    uint8* data;
    uint16 length;
    uint16 elementSize;
} Rte_ArrayType;

/*==================================================================================================
*                                    RTE BYTE ARRAY TYPE
==================================================================================================*/
typedef struct {
    uint8 data[RTE_MAX_BYTE_ARRAY_LENGTH];
    uint16 length;
} Rte_ByteArrayType;

/*==================================================================================================
*                                    RTE TIME TYPE
==================================================================================================*/
typedef uint32 Rte_TimeType;

/*==================================================================================================
*                                    RTE TIMESTAMP TYPE
==================================================================================================*/
typedef struct {
    Rte_TimeType seconds;
    uint32 nanoseconds;
} Rte_TimestampType;

/*==================================================================================================
*                                    RTE DATE AND TIME TYPE
==================================================================================================*/
typedef struct {
    Rte_UInt16 year;
    Rte_UInt8 month;
    Rte_UInt8 day;
    Rte_UInt8 hour;
    Rte_UInt8 minute;
    Rte_UInt8 second;
    Rte_UInt32 nanosecond;
} Rte_DateAndTimeType;

/*==================================================================================================
*                                    RTE COM SPEC TYPE
==================================================================================================*/
typedef struct {
    boolean used;
    uint32 aliveTimeout;
    uint32 queueLength;
    boolean handleNeverReceived;
    boolean updateIndication;
    Rte_TransmissionModeType transmissionMode;
} Rte_ComSpecType;

/*==================================================================================================
*                                    RTE PR PORT COM SPEC TYPE
==================================================================================================*/
typedef struct {
    boolean used;
    uint32 aliveTimeout;
    boolean handleNeverReceived;
    boolean updateIndication;
} Rte_PrPortComSpecType;

/*==================================================================================================
*                                    RTE PV PORT COM SPEC TYPE
==================================================================================================*/
typedef struct {
    boolean used;
    uint32 aliveTimeout;
    boolean handleNeverReceived;
    boolean updateIndication;
    boolean handleOutOfRange;
} Rte_PvPortComSpecType;

/*==================================================================================================
*                                    RTE SENDER RECEIVER COMMUNICATION TYPE
==================================================================================================*/
typedef enum {
    RTE_SR_COMMUNICATION_NONE = 0,
    RTE_SR_COMMUNICATION_DATA_ELEMENT,
    RTE_SR_COMMUNICATION_INVALIDATION,
    RTE_SR_COMMUNICATION_FILTER,
    RTE_SR_COMMUNICATION_QUEUE,
    RTE_SR_COMMUNICATION_HANDLE_NEVER_RECEIVED
} Rte_SRCommunicationType;

/*==================================================================================================
*                                    RTE CLIENT SERVER COMMUNICATION TYPE
==================================================================================================*/
typedef enum {
    RTE_CS_COMMUNICATION_NONE = 0,
    RTE_CS_COMMUNICATION_SYNCHRONOUS,
    RTE_CS_COMMUNICATION_ASYNCHRONOUS
} Rte_CSCommunicationType;

/*==================================================================================================
*                                    RTE MODE COMMUNICATION TYPE
==================================================================================================*/
typedef enum {
    RTE_MODE_COMMUNICATION_NONE = 0,
    RTE_MODE_COMMUNICATION_SYNCHRONOUS,
    RTE_MODE_COMMUNICATION_ASYNCHRONOUS
} Rte_ModeCommunicationType;

/*==================================================================================================
*                                    RTE TRIGGER COMMUNICATION TYPE
==================================================================================================*/
typedef enum {
    RTE_TRIGGER_COMMUNICATION_NONE = 0,
    RTE_TRIGGER_COMMUNICATION_INTERNAL,
    RTE_TRIGGER_COMMUNICATION_EXTERNAL
} Rte_TriggerCommunicationType;

#endif /* RTE_TYPE_H */
