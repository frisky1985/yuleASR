/** @file SomeIp.h - AUTOSAR SOME/IP stub (matches real module header) */
#ifndef SOMEIP_H
#define SOMEIP_H
#include "Std_Types.h"
#include "ComStack_Types.h"

#define SOMEIP_AR_RELEASE_MAJOR_VERSION    4
#define SOMEIP_AR_RELEASE_MINOR_VERSION    0
#define SOMEIP_AR_RELEASE_REVISION_VERSION 3
#define SOMEIP_SW_MAJOR_VERSION            1
#define SOMEIP_SW_MINOR_VERSION            0
#define SOMEIP_SW_PATCH_VERSION            0
#define SOMEIP_MODULE_ID                   0x70

#define SOMEIP_INIT_SID                    0x01
#define SOMEIP_DEINIT_SID                  0x02
#define SOMEIP_GETVERSIONINFO_SID          0x03
#define SOMEIP_SENDREQUEST_SID             0x04
#define SOMEIP_SENDRESPONSE_SID            0x05
#define SOMEIP_SENDNOTIFICATION_SID        0x06
#define SOMEIP_HANDLEMESSAGE_SID           0x07

#define SOMEIP_E_NOT_INITIALIZED           0x01
#define SOMEIP_E_INVALID_POINTER           0x02
#define SOMEIP_E_INVALID_PARAMETER         0x03
#define SOMEIP_E_INVALID_MESSAGE_ID        0x04
#define SOMEIP_E_INVALID_REQUEST_ID        0x05
#define SOMEIP_E_INVALID_SESSION_ID        0x06
#define SOMEIP_E_INVALID_CLIENT_ID         0x07
#define SOMEIP_E_WRONG_INTERFACE_VERSION   0x08
#define SOMEIP_E_WRONG_MESSAGE_TYPE        0x09
#define SOMEIP_E_WRONG_RETURN_CODE         0x0A
#define SOMEIP_E_MALFORMED_MESSAGE         0x0B
#define SOMEIP_E_MESSAGE_TOO_LARGE         0x0C
#define SOMEIP_E_NO_FREE_BUFFER            0x0D
#define SOMEIP_E_UNKNOWN_SERVICE           0x0E
#define SOMEIP_E_UNKNOWN_METHOD            0x0F
#define SOMEIP_E_NOT_READY                 0x10

#define SOMEIP_PROTOCOL_VERSION            0x01
#define SOMEIP_INTERFACE_VERSION           0x01
#define SOMEIP_MAGIC_COOKIE                0xFFFF0000
#define SOMEIP_HEADER_SIZE                 16

typedef uint8 SomeIp_MessageTypeType;
#define SOMEIP_MSG_REQUEST                 0x00
#define SOMEIP_MSG_REQUEST_NO_RETURN       0x01
#define SOMEIP_MSG_NOTIFICATION            0x02
#define SOMEIP_MSG_RESPONSE                0x80
#define SOMEIP_MSG_ERROR                   0x81

typedef uint8 SomeIp_ReturnCodeType;
#define SOMEIP_RET_OK                      0x00
#define SOMEIP_RET_NOT_OK                  0x01
#define SOMEIP_RET_UNKNOWN_SERVICE         0x02
#define SOMEIP_RET_UNKNOWN_METHOD          0x03
#define SOMEIP_RET_NOT_READY               0x04
#define SOMEIP_RET_NOT_REACHABLE           0x05
#define SOMEIP_RET_TIMEOUT                 0x06
#define SOMEIP_RET_WRONG_PROTOCOL_VERSION  0x07
#define SOMEIP_RET_WRONG_INTERFACE_VERSION 0x08
#define SOMEIP_RET_MALFORMED_MESSAGE       0x09
#define SOMEIP_RET_WRONG_MESSAGE_TYPE      0x0A

typedef uint16 SomeIp_ServiceIdType;
typedef uint16 SomeIp_MethodIdType;
typedef uint32 SomeIp_MessageIdType;
typedef uint16 SomeIp_ClientIdType;
typedef uint16 SomeIp_SessionIdType;
typedef uint32 SomeIp_RequestIdType;
typedef uint8  SomeIp_ProtocolVersionType;
typedef uint8  SomeIp_InterfaceVersionType;
typedef uint32 SomeIp_LengthType;

typedef struct {
    SomeIp_MessageIdType MessageId;
    SomeIp_LengthType Length;
    SomeIp_RequestIdType RequestId;
    SomeIp_ProtocolVersionType ProtocolVersion;
    SomeIp_InterfaceVersionType InterfaceVersion;
    SomeIp_MessageTypeType MessageType;
    SomeIp_ReturnCodeType ReturnCode;
} SomeIp_HeaderType;

typedef struct {
    SomeIp_HeaderType Header;
    uint8* Payload;
    SomeIp_LengthType PayloadLength;
} SomeIp_MessageType;

typedef struct {
    SomeIp_ServiceIdType ServiceId;
    SomeIp_MethodIdType MethodId;
    SomeIp_MessageTypeType MessageType;
    uint8* Callback;
} SomeIp_ServiceConfigType;

typedef struct {
    SomeIp_ClientIdType ClientId;
    SomeIp_SessionIdType SessionId;
} SomeIp_ClientConfigType;

typedef struct {
    const SomeIp_ServiceConfigType* Services;
    uint16 NumServices;
    const SomeIp_ClientConfigType* Clients;
    uint16 NumClients;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} SomeIp_ConfigType;

extern void SomeIp_Init(const SomeIp_ConfigType* ConfigPtr);
extern void SomeIp_DeInit(void);
extern void SomeIp_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
extern Std_ReturnType SomeIp_SendRequest(
    SomeIp_ClientIdType, SomeIp_ServiceIdType, SomeIp_MethodIdType,
    const uint8*, SomeIp_LengthType);
extern Std_ReturnType SomeIp_SendResponse(
    SomeIp_RequestIdType, const uint8*, SomeIp_LengthType, SomeIp_ReturnCodeType);
extern Std_ReturnType SomeIp_SendNotification(
    SomeIp_ServiceIdType, SomeIp_MethodIdType, const uint8*, SomeIp_LengthType);
extern void SomeIp_RxIndication(const uint8*, uint32);
extern void SomeIp_TxConfirmation(SomeIp_RequestIdType);
extern Std_ReturnType SomeIp_ProcessMessage(const SomeIp_MessageType*);
extern Std_ReturnType SomeIp_ParseHeader(const uint8*, SomeIp_HeaderType*);
extern Std_ReturnType SomeIp_SerializeHeader(const SomeIp_HeaderType*, uint8*);
extern SomeIp_MessageIdType SomeIp_CreateMessageId(SomeIp_ServiceIdType, SomeIp_MethodIdType);
extern SomeIp_RequestIdType SomeIp_CreateRequestId(SomeIp_ClientIdType, SomeIp_SessionIdType);
#endif
