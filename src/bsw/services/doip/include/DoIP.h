/*
 * DoIP.h
 * Diagnostic over IP (ISO 13400)
 */

#ifndef DOIP_H
#define DOIP_H

#include "Std_Types.h"
#include "SoAd.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 *=================================================================================================*/
/* DoIP Protocol Version */
#define DOIP_PROTOCOL_VERSION           0x02U
#define DOIP_PROTOCOL_VERSION_INV       0xFDU

/* DoIP Payload Types (ISO 13400-2:2019) */
#define DOIP_PT_VEHICLE_ANNOUNCE        0x0001U
#define DOIP_PT_VEHICLE_ID_REQ          0x0002U
#define DOIP_PT_VEHICLE_ID_REQ_EID      0x0003U
#define DOIP_PT_VEHICLE_ID_REQ_VIN      0x0004U
#define DOIP_PT_VEHICLE_ANNOUNCE_ACK    0x0005U
#define DOIP_PT_ROUTING_ACTIVATION_REQ  0x0006U
#define DOIP_PT_ROUTING_ACTIVATION_RES  0x0007U
#define DOIP_PT_ALIVE_CHECK_REQ         0x0008U
#define DOIP_PT_ALIVE_CHECK_RES         0x0009U
#define DOIP_PT_ENTITY_STATUS_REQ       0x4001U
#define DOIP_PT_ENTITY_STATUS_RES       0x4002U
#define DOIP_PT_POWER_MODE_REQ          0x4003U
#define DOIP_PT_POWER_MODE_RES          0x4004U
#define DOIP_PT_DIAGNOSTIC_MSG          0x8001U
#define DOIP_PT_DIAGNOSTIC_ACK          0x8002U
#define DOIP_PT_DIAGNOSTIC_NACK         0x8003U

/* DoIP Routing Activation Response Codes */
#define DOIP_ROUTING_ACTIVATION_RES_CODE_OK                 0x00U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED             0x01U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_UNKNOWN_SA         0x02U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_ALL_SOCKETS_INUSE  0x03U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_SA_MISMATCH        0x04U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_MISSING_AUTH       0x05U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_REJECTED_AUTH      0x06U
#define DOIP_ROUTING_ACTIVATION_RES_CODE_CONFIRMATION       0x10U

/* DoIP Diagnostic NACK Codes */
#define DOIP_DIAG_NACK_INVALID_SA           0x02U
#define DOIP_DIAG_NACK_UNKNOWN_TA           0x03U
#define DOIP_DIAG_NACK_MESSAGE_TOO_LARGE    0x04U
#define DOIP_DIAG_NACK_OUT_OF_MEMORY        0x05U
#define DOIP_DIAG_NACK_TARGET_UNREACHABLE   0x06U
#define DOIP_DIAG_NACK_UNKNOWN_NETWORK      0x07U
#define DOIP_DIAG_NACK_TRANSPORT_ERROR      0x08U

/* DoIP Connection States */
#define DOIP_CON_STATE_CLOSED               0x00U
#define DOIP_CON_STATE_CONNECTING           0x01U
#define DOIP_CON_STATE_ROUTING_ACTIVE       0x02U
#define DOIP_CON_STATE_ALIVE_CHECK          0x03U

/* DoIP Header Sizes */
#define DOIP_HEADER_LENGTH                  8U
#define DOIP_GENERIC_HEADER_LENGTH          8U
#define DOIP_VIN_LENGTH                     17U
#define DOIP_EID_LENGTH                     6U
#define DOIP_GID_LENGTH                     6U

/* Timing Parameters (in ms) */
#define DOIP_TIMING_ANNOUNCE_WAIT           500U
#define DOIP_TIMING_ANNOUNCE_INTERVAL       500U
#define DOIP_TIMING_ANNOUNCE_NUM            3U
#define DOIP_TIMING_INITIAL_INACTIVITY      2000U
#define DOIP_TIMING_GENERAL_INACTIVITY      300000U
#define DOIP_TIMING_ALIVE_CHECK_TIMEOUT     500U

/*==================================================================================================
 *                                      TYPE DEFINITIONS
 *=================================================================================================*/
/* DoIP Generic Header Structure */
typedef struct
{
    uint8   ProtocolVersion;
    uint8   InverseProtocolVersion;
    uint16  PayloadType;
    uint32  PayloadLength;
} DoIP_GenericHeaderType;

/* DoIP Routing Activation Request */
typedef struct
{
    uint16  SourceAddress;
    uint8   ActivationType;
    uint8   Reserved[4];
    uint8   OemSpecific[4];
} DoIP_RoutingActivationReqType;

/* DoIP Routing Activation Response */
typedef struct
{
    uint16  TesterLogicalAddress;
    uint16  EntityLogicalAddress;
    uint8   ResponseCode;
    uint8   Reserved[4];
    uint8   OemSpecific[4];
} DoIP_RoutingActivationResType;

/* DoIP Diagnostic Message */
typedef struct
{
    uint16  SourceAddress;
    uint16  TargetAddress;
    uint8*  Payload;
    uint32  PayloadLength;
} DoIP_DiagnosticMessageType;

/* DoIP Diagnostic Ack/Nack */
typedef struct
{
    uint16  SourceAddress;
    uint16  TargetAddress;
    uint8   AckCode;
    uint8   PreviousDiagnosticMessage[DOIP_HEADER_LENGTH];
} DoIP_DiagnosticAckType;

/* DoIP Vehicle Announcement */
typedef struct
{
    uint8   VIN[DOIP_VIN_LENGTH];
    uint8   LogicalAddress[2];
    uint8   EID[DOIP_EID_LENGTH];
    uint8   GID[DOIP_GID_LENGTH];
    uint8   FurtherAction;
    uint8   VIN_GID_SyncStatus;
} DoIP_VehicleAnnouncementType;

/* DoIP Alive Check Response */
typedef struct
{
    uint8   SourceAddress[2];
} DoIP_AliveCheckResType;

/* DoIP Connection Info */
typedef struct
{
    uint8   State;
    uint16  SourceAddress;
    uint16  TargetAddress;
    uint32  InactivityTimer;
    uint16  SoConId;
    boolean RoutingActivated;
} DoIP_ConnectionType;

/* DoIP State Type */
typedef enum
{
    DOIP_STATE_UNINIT = 0,
    DOIP_STATE_INIT,
    DOIP_STATE_ACTIVE,
    DOIP_STATE_SHUTDOWN
} DoIP_StateType;

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 *=================================================================================================*/
extern DoIP_StateType DoIP_State;

/*==================================================================================================
 *                                      FUNCTION PROTOTYPES
 *=================================================================================================*/
/* Initialization */
void DoIP_Init(const void* ConfigPtr);
void DoIP_DeInit(void);

/* Main Functions */
void DoIP_MainFunction(void);
void DoIP_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void DoIP_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* Vehicle Announcement */
void DoIP_SendVehicleAnnouncement(void);
void DoIP_ProcessVehicleIdentificationReq(const uint8* Data, uint32 Length);

/* Routing Activation */
Std_ReturnType DoIP_ProcessRoutingActivationReq(uint16 SoConId, const uint8* Data, uint32 Length);
void DoIP_SendRoutingActivationResponse(uint16 SoConId, uint8 ResponseCode);

/* Diagnostic Message */
Std_ReturnType DoIP_ProcessDiagnosticMessage(uint16 SoConId, const uint8* Data, uint32 Length);
void DoIP_SendDiagnosticAck(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 AckCode);
void DoIP_SendDiagnosticNack(uint16 SoConId, uint16 SourceAddress, uint16 TargetAddress, uint8 NackCode);

/* Alive Check */
void DoIP_SendAliveCheckRequest(void);
void DoIP_ProcessAliveCheckResponse(uint16 SoConId, const uint8* Data);

/* Connection Management */
void DoIP_CloseConnection(uint16 SoConId);
void DoIP_ResetInactivityTimer(uint16 SoConId);

/* Header Handling */
Std_ReturnType DoIP_ParseGenericHeader(const uint8* Data, DoIP_GenericHeaderType* Header);
void DoIP_BuildGenericHeader(uint8* Buffer, uint16 PayloadType, uint32 PayloadLength);

/* Utility Functions */
boolean DoIP_ValidateSourceAddress(uint16 SourceAddress);
boolean DoIP_ValidateTargetAddress(uint16 TargetAddress);
uint16 DoIP_GetConnectionSourceAddress(uint16 SoConId);

#endif /* DOIP_H */
