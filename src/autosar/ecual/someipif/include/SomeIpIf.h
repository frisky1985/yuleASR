/**
 * @file SomeIpIf.h
 * @brief SOME/IP Interface Module
 * @version 1.0.0
 */

#ifndef SOMEIPIF_H
#define SOMEIPIF_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define SOMEIPIF_MODULE_ID          0x80U
#define SOMEIPIF_VENDOR_ID          0x0001U

/* Error Codes */
#define SOMEIPIF_E_NO_ERROR         0x00U
#define SOMEIPIF_E_PARAM_POINTER    0x01U
#define SOMEIPIF_E_UNINIT           0x02U
#define SOMEIPIF_E_INVALID_ID       0x03U

/* SOME/IP Message Types */
typedef enum {
    SOMEIP_REQUEST = 0x00,
    SOMEIP_REQUEST_NO_RETURN = 0x01,
    SOMEIP_NOTIFICATION = 0x02,
    SOMEIP_RESPONSE = 0x80,
    SOMEIP_ERROR = 0x81
} SomeIpIf_MessageTypeType;

/* SOME/IP Return Codes */
typedef enum {
    SOMEIP_OK = 0x00,
    SOMEIP_NOT_OK = 0x01,
    SOMEIP_UNKNOWN_SERVICE = 0x02,
    SOMEIP_UNKNOWN_METHOD = 0x03,
    SOMEIP_NOT_READY = 0x04,
    SOMEIP_NOT_REACHABLE = 0x05,
    SOMEIP_TIMEOUT = 0x06,
    SOMEIP_WRONG_PROTOCOL_VERSION = 0x07,
    SOMEIP_WRONG_INTERFACE_VERSION = 0x08,
    SOMEIP_MALFORMED_MESSAGE = 0x09,
    SOMEIP_WRONG_MESSAGE_TYPE = 0x0A
} SomeIpIf_ReturnCodeType;

/* SOME/IP Header */
typedef struct {
    uint16 ServiceId;
    uint16 MethodId;
    uint32 Length;
    uint16 ClientId;
    uint16 SessionId;
    uint8 ProtocolVersion;
    uint8 InterfaceVersion;
    SomeIpIf_MessageTypeType MessageType;
    SomeIpIf_ReturnCodeType ReturnCode;
} SomeIpIf_HeaderType;

/* Endpoint Configuration */
typedef struct {
    uint32 IpAddress;
    uint16 Port;
    uint8 ConnectionType; /* TCP/UDP */
} SomeIpIf_EndpointType;

/* Service Configuration */
typedef struct {
    uint16 ServiceId;
    uint16 InstanceId;
    SomeIpIf_EndpointType Endpoint;
    boolean IsReliable;
} SomeIpIf_ServiceConfigType;

/* Function Prototypes */
void SomeIpIf_Init(const void* ConfigPtr);
Std_ReturnType SomeIpIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType SomeIpIf_SendRequest(uint16 ServiceId, uint16 MethodId, const uint8* Data, uint16 Length);
Std_ReturnType SomeIpIf_SendResponse(uint16 ServiceId, uint16 ClientId, uint16 SessionId, const uint8* Data, uint16 Length);
Std_ReturnType SomeIpIf_SendNotification(uint16 ServiceId, uint16 EventId, const uint8* Data, uint16 Length);
void SomeIpIf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void SomeIpIf_MainFunction(void);

#endif
