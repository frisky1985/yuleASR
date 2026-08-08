/******************************************************************************
 * @file    doip_types.h
 * @brief   ISO 13400-2 DoIP Protocol Type Definitions
 *
 * ISO 13400-2:2019 compliant
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOIP_TYPES_H
#define DOIP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * DoIP Module Version Information
 ******************************************************************************/
#define DOIP_VENDOR_ID                  0x01U
#define DOIP_MODULE_ID                  0x33U
#define DOIP_SW_MAJOR_VERSION           1U
#define DOIP_SW_MINOR_VERSION           0U
#define DOIP_SW_PATCH_VERSION           0U

/******************************************************************************
 * DoIP Protocol Constants (ISO 13400-2:2019)
 ******************************************************************************/

/* Protocol Version */
#define DOIP_PROTOCOL_VERSION           0x02U
#define DOIP_PROTOCOL_VERSION_INV       0xFDU

/* UDP Ports */
#define DOIP_UDP_DISCOVERY_PORT         13400U
#define DOIP_UDP_TEST_EQUIPMENT_PORT    13400U

/* TCP Ports */
#define DOIP_TCP_DATA_PORT              13400U

/* Default Timings (milliseconds) */
#define DOIP_DEFAULT_A_ANNOUNCE_NUM     3U
#define DOIP_DEFAULT_A_ANNOUNCE_TIMER   500U
#define DOIP_DEFAULT_INITIAL_INACTIVITY_TIMER 5000U
#define DOIP_DEFAULT_GENERAL_INACTIVITY_TIMER 300000U
#define DOIP_DEFAULT_ALIVE_CHECK_RESPONSE_TIMEOUT 500U
#define DOIP_DEFAULT_T_TCP_GENERAL_INACTIVITY 300000U
#define DOIP_DEFAULT_T_TCP_ALIVE_CHECK_RESPONSE 500U
#define DOIP_DEFAULT_T_TCP_INITIAL_INACTIVITY 2000U

/******************************************************************************
 * Payload Types (ISO 13400-2:2019 Table 6)
 ******************************************************************************/

typedef enum {
    /* Vehicle Discovery Phase */
    DOIP_PT_VID_REQUEST             = 0x0001U,  /* Vehicle Identification Request */
    DOIP_PT_VID_REQUEST_EID         = 0x0002U,  /* Vehicle ID Request with EID */
    DOIP_PT_VID_REQUEST_VIN         = 0x0003U,  /* Vehicle ID Request with VIN */
    DOIP_PT_VEHICLE_ANNOUNCEMENT    = 0x0004U,  /* Vehicle Announcement/Vehicle Identification Response */
    
    /* Routing Activation Phase */
    DOIP_PT_ROUTING_ACTIVATION_REQ  = 0x0005U,  /* Routing Activation Request */
    DOIP_PT_ROUTING_ACTIVATION_RES  = 0x0006U,  /* Routing Activation Response */
    
    /* Alive Check Phase */
    DOIP_PT_ALIVE_CHECK_REQ         = 0x0007U,  /* Alive Check Request */
    DOIP_PT_ALIVE_CHECK_RES         = 0x0008U,  /* Alive Check Response */
    
    /* Entity Status Phase */
    DOIP_PT_ENTITY_STATUS_REQ       = 0x4001U,  /* DoIP Entity Status Request */
    DOIP_PT_ENTITY_STATUS_RES       = 0x4002U,  /* DoIP Entity Status Response */
    DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_REQ  = 0x4003U,  /* Diagnostic Power Mode Info Request */
    DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_RES  = 0x4004U,  /* Diagnostic Power Mode Info Response */
    
    /* Diagnostic Message Phase */
    DOIP_PT_DIAGNOSTIC_MSG          = 0x8001U,  /* Diagnostic Message */
    DOIP_PT_DIAGNOSTIC_ACK          = 0x8002U,  /* Diagnostic Message Positive Ack */
    DOIP_PT_DIAGNOSTIC_NACK         = 0x8003U,  /* Diagnostic Message Negative Ack */
} DoIp_PayloadType;

/******************************************************************************
 * Routing Activation Types (ISO 13400-2:2019 Table 26)
 ******************************************************************************/

typedef enum {
    DOIP_RA_DEFAULT                 = 0x00U,  /* Default */
    DOIP_RA_WWH_OBD                 = 0x01U,  /* WWH-OBD */
    DOIP_RA_CENTRAL_SECURITY        = 0x02U,  /* Central Security */
    DOIP_RA_CENTRAL_DIAGNOSTICS     = 0x03U,  /* Central Diagnostics */
    DOIP_RA_VEHICLE_MANUFACTURER    = 0xE0U   /* Vehicle Manufacturer Specific (0xE0-0xFF) */
} DoIp_RoutingActivationType;

/******************************************************************************
 * Routing Activation Response Codes (ISO 13400-2:2019 Table 27)
 ******************************************************************************/

typedef enum {
    DOIP_RA_RES_SUCCESS                     = 0x10U,  /* Success */
    DOIP_RA_RES_UNSUPPORTED_SA              = 0x02U,  /* Source Address unsupported */
    DOIP_RA_RES_NO_RESOURCES                = 0x03U,  /* No resources */
    DOIP_RA_RES_SA_CONFIRMATION_TIMEOUT     = 0x04U,  /* SA confirmation timeout */
    DOIP_RA_RES_SA_CONFIRMATION_FAILED      = 0x05U,  /* SA confirmation failed */
    DOIP_RA_RES_RA_TYPE_UNSUPPORTED         = 0x06U,  /* RA type unsupported */
    DOIP_RA_RES_TLS_REQUIRED                = 0x07U,  /* TLS required */
    DOIP_RA_RES_TLS_NOT_SUPPORTED           = 0x08U,  /* TLS not supported */
    DOIP_RA_RES_CONFIRMATION_NOT_RECEIVED   = 0x09U,  /* Confirmation not received */
    DOIP_RA_RES_INVALID_AUTHENTICATION_DATA = 0x0AU,  /* Invalid authentication data */
    DOIP_RA_RES_INVALID_CONFIRMATION_DATA   = 0x0BU,  /* Invalid confirmation data */
    DOIP_RA_RES_VEHICLE_MANUFACTURER_MIN    = 0xF0U   /* Vehicle Manufacturer specific (0xF0-0xFF) */
} DoIp_RoutingActivationResponseCode;

/******************************************************************************
 * Diagnostic Message Ack Codes (ISO 13400-2:2019 Table 29)
 ******************************************************************************/

typedef enum {
    DOIP_DIAG_ACK_OK                = 0x00U,  /* Acknowledged */
} DoIp_DiagnosticAckCode;

/******************************************************************************
 * Diagnostic Message Nack Codes (ISO 13400-2:2019 Table 30)
 ******************************************************************************/

typedef enum {
    DOIP_DIAG_NACK_INVALID_SA       = 0x02U,  /* Invalid Source Address */
    DOIP_DIAG_NACK_UNKNOWN_TA       = 0x03U,  /* Unknown Target Address */
    DOIP_DIAG_NACK_MSG_TOO_LARGE    = 0x04U,  /* Message too large */
    DOIP_DIAG_NACK_OUT_OF_MEMORY    = 0x05U,  /* Out of memory */
    DOIP_DIAG_NACK_TARGET_UNREACHABLE = 0x06U, /* Target unreachable */
    DOIP_DIAG_NACK_UNKNOWN_NETWORK  = 0x07U,  /* Unknown network */
    DOIP_DIAG_NACK_TP_ERROR         = 0x08U   /* Transport protocol error */
} DoIp_DiagnosticNackCode;

/******************************************************************************
 * Power Modes (ISO 13400-2:2019 Table 24)
 ******************************************************************************/

typedef enum {
    DOIP_POWER_MODE_NOT_READY       = 0x00U,  /* Not ready */
    DOIP_POWER_MODE_READY           = 0x01U,  /* Ready */
    DOIP_POWER_MODE_NOT_SUPPORTED   = 0x02U   /* Power mode not supported */
} DoIp_PowerModeType;

/******************************************************************************
 * Node Types
 ******************************************************************************/

typedef enum {
    DOIP_NODE_GATEWAY               = 0x00U,  /* Gateway */
    DOIP_NODE_NODE                  = 0x01U   /* DoIP Node */
} DoIp_NodeType;

/******************************************************************************
 * Connection States
 ******************************************************************************/

typedef enum {
    DOIP_STATE_INIT                 = 0x00U,
    DOIP_STATE_VEHICLE_DISCOVERY    = 0x01U,
    DOIP_STATE_ROUTING_ACTIVATION   = 0x02U,
    DOIP_STATE_REGISTERED           = 0x03U,
    DOIP_STATE_DIAGNOSTIC_SESSION   = 0x04U,
    DOIP_STATE_ALIVE_CHECK          = 0x05U,
    DOIP_STATE_ERROR                = 0xFFU
} DoIp_StateType;

/******************************************************************************
 * DoIP Return Types
 ******************************************************************************/

typedef enum {
    DOIP_OK = 0x00U,
    DOIP_ERROR = 0x01U,
    DOIP_BUSY = 0x02U,
    DOIP_TIMEOUT = 0x03U,
    DOIP_INVALID_PARAMETER = 0x04U,
    DOIP_NOT_INITIALIZED = 0x05U,
    DOIP_NO_BUFFER = 0x06U,
    DOIP_NETWORK_ERROR = 0x07U,
    DOIP_CONNECTION_CLOSED = 0x08U,
    DOIP_ROUTING_REJECTED = 0x09U
} DoIp_ReturnType;

/******************************************************************************
 * DoIP Message Header
 ******************************************************************************/

typedef struct {
    uint8_t protocolVersion;             /* Protocol version (0x02) */
    uint8_t protocolVersionInv;          /* Inverse protocol version (0xFD) */
    uint16_t payloadType;                /* Payload type */
    uint32_t payloadLength;              /* Payload length */
} DoIp_HeaderType;

/******************************************************************************
 * Vehicle Identification Response (Vehicle Announcement)
 ******************************************************************************/

typedef struct {
    uint8_t vin[17];                     /* Vehicle Identification Number */
    uint8_t logicalAddress[2];           /* Logical address */
    uint8_t eid[6];                      /* Entity ID (MAC address) */
    uint8_t gid[6];                      /* Group ID */
    uint8_t furtherAction;               /* Further action byte */
    uint8_t syncStatus;                  /* VIN/GID synchronization status */
} DoIp_VehicleAnnouncementType;

/******************************************************************************
 * Routing Activation Request
 ******************************************************************************/

typedef struct {
    uint16_t sourceAddress;              /* Source address */
    uint8_t activationType;              /* Activation type */
    uint8_t *oemSpecific;                /* OEM specific data (optional) */
    uint16_t oemSpecificLength;          /* OEM specific data length */
} DoIp_RoutingActivationRequestType;

/******************************************************************************
 * Routing Activation Response
 ******************************************************************************/

typedef struct {
    uint16_t logicalAddressTester;       /* Logical address of tester */
    uint16_t logicalAddressDoip;         /* Logical address of DoIP entity */
    uint8_t responseCode;                /* Response code */
    uint8_t *oemSpecific;                /* OEM specific data (optional) */
    uint16_t oemSpecificLength;          /* OEM specific data length */
} DoIp_RoutingActivationResponseType;

/******************************************************************************
 * Diagnostic Message
 ******************************************************************************/

typedef struct {
    uint16_t sourceAddress;              /* Source address */
    uint16_t targetAddress;              /* Target address */
    uint8_t *userData;                   /* User data (UDS message) */
    uint16_t userDataLength;             /* User data length */
} DoIp_DiagnosticMessageType;

/******************************************************************************
 * Diagnostic Ack/Nack
 ******************************************************************************/

typedef struct {
    uint16_t sourceAddress;              /* Source address */
    uint16_t targetAddress;              /* Target address */
    uint8_t ackCode;                     /* Ack code */
    uint8_t *previousData;               /* Previous diagnostic message data */
    uint16_t previousDataLength;         /* Previous data length */
} DoIp_DiagnosticAckType;

/******************************************************************************
 * Entity Status Response
 ******************************************************************************/

typedef struct {
    uint8_t nodeType;                    /* Node type */
    uint8_t maxTcpSockets;               /* Max TCP sockets */
    uint8_t currentTcpSockets;           /* Currently open TCP sockets */
    uint16_t maxDataSize;                /* Max data size */
} DoIp_EntityStatusType;

/******************************************************************************
 * Connection Configuration
 ******************************************************************************/

typedef struct {
    uint8_t connectionId;                /* Connection ID */
    uint16_t localAddress;               /* Local logical address */
    uint32_t ipAddress;                  /* IP address (IPv4) */
    uint16_t port;                       /* Port number */
    bool tlsEnabled;                     /* TLS enabled */
    uint32_t inactivityTimeout;          /* Inactivity timeout (ms) */
    uint32_t aliveCheckTimeout;          /* Alive check timeout (ms) */
    uint32_t generalTimeout;             /* General timeout (ms) */
    uint8_t maxRoutingActivations;       /* Max concurrent routing activations */
} DoIp_ConnectionConfigType;

/******************************************************************************
 * DoIP Configuration
 ******************************************************************************/

typedef struct {
    uint8_t eid[6];                      /* Entity ID (MAC address) */
    uint8_t gid[6];                      /* Group ID */
    uint8_t vin[17];                     /* Vehicle Identification Number */
    uint16_t logicalAddress;             /* Logical address */
    uint8_t furtherAction;               /* Further action byte */
    bool vinSyncRequired;                /* VIN synchronization required */
    DoIp_NodeType nodeType;              /* Node type */
    uint8_t maxTcpConnections;           /* Max TCP connections */
    uint16_t maxDiagnosticSize;          /* Max diagnostic message size */
    bool routingActivationRequired;      /* Routing activation required */
} DoIp_EntityConfigType;

/******************************************************************************
 * Connection Runtime Context
 ******************************************************************************/

typedef struct {
    uint8_t connectionId;                /* Connection ID */
    DoIp_StateType state;                /* Current state */
    uint32_t socketHandle;               /* Socket handle */
    uint16_t registeredSourceAddress;    /* Registered tester source address */
    uint8_t activationType;              /* Activation type used */
    uint32_t lastActivity;               /* Last activity timestamp */
    uint32_t connectionStart;            /* Connection start timestamp */
    bool authenticated;                  /* Authentication status */
    bool confirmed;                      /* Confirmation status */
    uint8_t aliveCheckPending;           /* Alive check pending flag */
    uint8_t announcementCount;           /* Vehicle announcement counter */
} DoIp_ConnectionContextType;

/******************************************************************************
 * DoIP Module Context
 ******************************************************************************/

typedef struct {
    bool initialized;                    /* Initialization flag */
    const DoIp_EntityConfigType *entityConfig;  /* Entity configuration */
    const DoIp_ConnectionConfigType *connections;
    uint8_t numConnections;              /* Number of connections */
    DoIp_ConnectionContextType *connectionContexts;
    uint32_t tcpListenSocket;            /* TCP listen socket */
    uint32_t udpSocket;                  /* UDP socket */
    bool discoveryEnabled;               /* Vehicle discovery enabled */
    bool routingActive;                  /* Routing activation active */
    
    /* Statistics */
    uint32_t rxCount;
    uint32_t txCount;
    uint32_t routingActivations;
    uint32_t diagnosticMsgs;
    
    /* Callbacks */
    void (*onRoutingActivation)(uint8_t connId, uint16_t srcAddr, uint8_t result);
    void (*onDiagnosticMessage)(uint8_t connId, uint16_t srcAddr, uint16_t tgtAddr, 
                                 uint8_t *data, uint16_t len);
    void (*onConnectionClosed)(uint8_t connId);
} DoIp_ContextType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

/* Routing activation callback */
typedef bool (*DoIp_RoutingActivationCallback)(
    uint16_t sourceAddress,
    uint8_t activationType,
    uint8_t *authenticationData,
    uint16_t authDataLength,
    uint8_t *confirmationData,
    uint16_t confirmDataLength
);

/* Diagnostic message callback */
typedef void (*DoIp_DiagnosticCallback)(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint16_t targetAddress,
    uint8_t *data,
    uint16_t length
);

/* Connection callback */
typedef void (*DoIp_ConnectionCallback)(
    uint8_t connectionId,
    bool connected
);

/******************************************************************************
 * Constants
 ******************************************************************************/

#define DOIP_HEADER_LENGTH              8U
#define DOIP_VID_MIN_LENGTH             32U
#define DOIP_VID_MAX_LENGTH             33U
#define DOIP_RA_REQ_MIN_LENGTH          7U
#define DOIP_RA_RES_MIN_LENGTH          9U
#define DOIP_DIAG_MIN_LENGTH            4U
#define DOIP_MAX_OEM_DATA               4U

#ifdef __cplusplus
}
#endif

#endif /* DOIP_TYPES_H */
