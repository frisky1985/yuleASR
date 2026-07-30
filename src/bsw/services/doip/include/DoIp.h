/**
 * @file DoIP.h
 * @brief Diagnostic over IP (DoIP) - ISO 13400-2 compliant header
 * @version 1.0.0
 * @date 2026-05-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AUTOSAR Standard: Diagnostic over IP (DoIP) R22-11
 * Layer: Service Layer
 * Purpose: Ethernet-based diagnostic communication per ISO 13400-2
 */

#ifndef DOIP_H
#define DOIP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "DoIP_Cfg.h"
#include "ComStack_Types.h"

/* SoAd_ModeType forward declaration for DoIP-SoAd interface */
#ifndef SOAD_MODETYPE_DEFINED
#define SOAD_MODETYPE_DEFINED
typedef uint8 SoAd_ModeType;
#endif

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DOIP_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define DOIP_MODULE_ID                  (0x4CU) /* DOIP Module ID */
#define DOIP_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define DOIP_AR_RELEASE_MINOR_VERSION   (0x04U)
#define DOIP_AR_RELEASE_REVISION_VERSION (0x00U)
#define DOIP_SW_MAJOR_VERSION           (0x01U)
#define DOIP_SW_MINOR_VERSION           (0x00U)
#define DOIP_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DOIP_SID_INIT                   (0x01U)
#define DOIP_SID_DEINIT                 (0x02U)
#define DOIP_SID_GETVERSIONINFO         (0x03U)
#define DOIP_SID_IFTRANSMIT             (0x04U)
#define DOIP_SID_IFRXINDICATION         (0x05U)
#define DOIP_SID_MAINFUNCTION           (0x06U)
#define DOIP_SID_ACTIVATEROUTING        (0x07U)
#define DOIP_SID_CLOSECONNECTION        (0x08U)
#define DOIP_SID_SOADTXCONFIRMATION     (0x09U)
#define DOIP_SID_VEHICLEANNOUNCEMENT    (0x0AU)
#define DOIP_SID_ALIVECHECK             (0x0BU)
#define DOIP_SID_ENTITYSTATUS           (0x0CU)
#define DOIP_SID_POWERMODEINFO          (0x0DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DOIP_E_PARAM_POINTER            (0x01U)
#define DOIP_E_PARAM_CONFIG             (0x02U)
#define DOIP_E_UNINIT                   (0x03U)
#define DOIP_E_INVALID_PDU_ID           (0x04U)
#define DOIP_E_INVALID_CONNECTION       (0x05U)
#define DOIP_E_INVALID_ROUTING_ACTIVATION (0x06U)
#define DOIP_E_INVALID_PARAMETER        (0x07U)
#define DOIP_E_INVALID_ENTITY_INDEX     (0x08U)
#define DOIP_E_NW_INACTIVE              (0x09U)

/*==================================================================================================
*                                    DOIP PROTOCOL VERSIONS
==================================================================================================*/
#define DOIP_PROTOCOL_VERSION_ISO13400_2012    (0x01U)
#define DOIP_PROTOCOL_VERSION_ISO13400_2019    (0x02U)
#define DOIP_PROTOCOL_VERSION_DEFAULT          (0x02U)

/*==================================================================================================
*                                    DOIP STATE TYPE
==================================================================================================*/
typedef enum {
    DOIP_STATE_UNINIT = 0,
    DOIP_STATE_INIT,
    DOIP_STATE_ACTIVE,
    DOIP_STATE_BUSY
} DoIP_StateType;

/*==================================================================================================
*                                    DOIP CONNECTION STATE TYPE
==================================================================================================*/
typedef enum {
    DOIP_CONN_STATE_CLOSED = 0,
    DOIP_CONN_STATE_SOCKET_CONNECTED,
    DOIP_CONN_STATE_REGISTERED,
    DOIP_CONN_STATE_AUTHENTICATING,
    DOIP_CONN_STATE_CONFIRMING,
    DOIP_CONN_STATE_ACTIVE,
    DOIP_CONN_STATE_ALIVE_CHECK_PENDING
} DoIP_ConnectionStateType;

/*==================================================================================================
*                                    DOIP ROUTING ACTIVATION RESPONSE CODES
==================================================================================================*/
typedef enum {
    DOIP_RA_RES_CODE_OK = 0x00,                          /* Routing activation accepted */
    DOIP_RA_RES_CODE_DENIED_UNKNOWN_SA = 0x01,           /* Denied: unknown source address */
    DOIP_RA_RES_CODE_DENIED_ALL_SOCKETS = 0x02,          /* Denied: all sockets registered */
    DOIP_RA_RES_CODE_DENIED_DIFF_SA = 0x03,              /* Denied: different source address */
    DOIP_RA_RES_CODE_DENIED_SA_REGISTERED = 0x04,        /* Denied: already registered */
    DOIP_RA_RES_CODE_DENIED_MISSING_AUTH = 0x05,         /* Denied: missing authentication */
    DOIP_RA_RES_CODE_DENIED_REJECTED_AUTH = 0x06,        /* Denied: rejected authentication */
    DOIP_RA_RES_CODE_DENIED_MISSING_CONFIRM = 0x07,      /* Denied: missing confirmation */
    DOIP_RA_RES_CODE_DENIED_REJECTED_CONFIRM = 0x08,     /* Denied: rejected confirmation */
    DOIP_RA_RES_CODE_DENIED_SSL = 0x09,                  /* Denied: SSL/TLS required */
    DOIP_RA_RES_CODE_DENIED_INCOMPATIBLE_TLS = 0x0A,     /* Denied: incompatible TLS version */
    DOIP_RA_RES_CODE_DENIED_CERTIFICATE = 0x0B,          /* Denied: certificate validation */
    DOIP_RA_RES_CODE_DENIED_AUTHENTICATION = 0x0C,       /* Denied: authentication failed */
    DOIP_RA_RES_CODE_DENIED_INVALID_PKEY = 0x0D,         /* Denied: invalid private key */
    DOIP_RA_RES_CODE_DENIED_INVALID_CERT = 0x0E          /* Denied: invalid certificate */
} DoIP_RoutingActivationResCodeType;

/*==================================================================================================
*                                    DOIP DIAGNOSTIC MESSAGE ACK CODES
==================================================================================================*/
typedef enum {
    DOIP_DIAG_ACK_OK = 0x00,
    DOIP_DIAG_ACK_INVALID_SA = 0x02,
    DOIP_DIAG_ACK_UNKNOWN_TA = 0x03,
    DOIP_DIAG_ACK_MESSAGE_TOO_LARGE = 0x04,
    DOIP_DIAG_ACK_OUT_OF_MEMORY = 0x05,
    DOIP_DIAG_ACK_TARGET_UNREACHABLE = 0x06,
    DOIP_DIAG_ACK_UNKNOWN_NETWORK = 0x07,
    DOIP_DIAG_ACK_TRANSPORT_ERROR = 0x08
} DoIP_DiagnosticAckCodeType;

/*==================================================================================================
*                                    DOIP PAYLOAD TYPE
==================================================================================================*/
typedef enum {
    /* Generic NACK */
    DOIP_PAYLOAD_GENERIC_NACK = 0x0000,
    
    /* Vehicle Discovery */
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_REQ = 0x0001,
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_RES = 0x0002,
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_REQ_EID = 0x0003,
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_REQ_VIN = 0x0004,
    
    /* Routing Activation */
    DOIP_PAYLOAD_ROUTING_ACTIVATION_REQ = 0x0005,
    DOIP_PAYLOAD_ROUTING_ACTIVATION_RES = 0x0006,
    
    /* Alive Check */
    DOIP_PAYLOAD_ALIVE_CHECK_REQ = 0x0007,
    DOIP_PAYLOAD_ALIVE_CHECK_RES = 0x0008,
    
    /* Node Discovery */
    DOIP_PAYLOAD_ENTITY_STATUS_REQ = 0x4001,
    DOIP_PAYLOAD_ENTITY_STATUS_RES = 0x4002,
    DOIP_PAYLOAD_DIAG_POWER_MODE_INFO_REQ = 0x4003,
    DOIP_PAYLOAD_DIAG_POWER_MODE_INFO_RES = 0x4004,
    
    /* Diagnostic */
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE = 0x8001,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_POS_ACK = 0x8002,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_NEG_ACK = 0x8003
} DoIP_PayloadType;

/*==================================================================================================
*                                    DOIP GENERIC NACK CODES
==================================================================================================*/
typedef enum {
    DOIP_NACK_INCORRECT_PATTERN_FORMAT = 0x00,
    DOIP_NACK_UNKNOWN_PAYLOAD_TYPE = 0x01,
    DOIP_NACK_MESSAGE_TOO_LARGE = 0x02,
    DOIP_NACK_OUT_OF_MEMORY = 0x03,
    DOIP_NACK_INVALID_PAYLOAD_LENGTH = 0x04
} DoIP_GenericNackCodeType;

/*==================================================================================================
*                                    DOIP POWER MODES
==================================================================================================*/
typedef enum {
    DOIP_POWER_MODE_NOT_READY = 0x00,
    DOIP_POWER_MODE_READY = 0x01,
    DOIP_POWER_MODE_NOT_SUPPORTED = 0x02
} DoIP_PowerModeType;

/*==================================================================================================
*                                    DOIP NODE TYPES
==================================================================================================*/
typedef enum {
    DOIP_NODE_GATEWAY = 0x00,
    DOIP_NODE_DOIP_NODE = 0x01
} DoIP_NodeType;

/*==================================================================================================
*                                    DOIP GENERIC HEADER TYPE
==================================================================================================*/
typedef struct {
    uint8  ProtocolVersion;
    uint8  InverseProtocolVersion;
    uint16 PayloadType;
    uint32 PayloadLength;
} DoIP_GenericHeaderType;

/*==================================================================================================
*                                    DOIP VEHICLE IDENTIFICATION TYPE
==================================================================================================*/
typedef struct {
    uint8  VIN[17];           /* Vehicle Identification Number */
    uint8  LogicalAddress[2]; /* Logical address */
    uint8  EID[6];            /* Entity ID (MAC address) */
    uint8  GID[6];            /* Group ID */
    uint8  FurtherActions;    /* Further action byte */
    uint8  VIN_GID_Status;    /* VIN/GID sync status (ISO 13400-2:2019) */
} DoIP_VehicleIdentificationType;

/*==================================================================================================
*                                    DOIP ENTITY STATUS TYPE
==================================================================================================*/
typedef struct {
    uint8  NodeType;
    uint8  MaxConcurrentSocket;
    uint8  CurrentlyOpenSocket;
    uint32 MaxDataSize;
} DoIP_EntityStatusType;

/*==================================================================================================
*                                    DOIP CONNECTION CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 ConnectionId;
    uint16 SourceAddress;
    uint16 TargetAddress;
    uint16 TesterLogicalAddress;
    uint16 AliveCheckTimeoutMs;
    uint16 GeneralInactivityTimeoutMs;
    uint16 InitialInactivityTimeoutMs;
    boolean AliveCheckEnabled;
    boolean IsServer;
    uint8   ProtocolType;     /* TCP = 0, UDP = 1 */
    uint16  LocalPort;
    uint16  RemotePort;
} DoIP_ConnectionConfigType;

/*==================================================================================================
*                                    DOIP ROUTING ACTIVATION CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8  ActivationType;
    uint16 SourceAddress;
    uint16 TargetAddress;
    uint8  NumAuthReqBytes;
    uint8  NumConfirmReqBytes;
    boolean AuthenticationRequired;
    boolean ConfirmationRequired;
    boolean IsDefault;
    uint8   Priority;         /* Activation priority level */
} DoIP_RoutingActivationConfigType;

/*==================================================================================================
*                                    DOIP ENTITY CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8   VIN[17];
    uint8   EID[6];
    uint8   GID[6];
    uint16  LogicalAddress;
    uint8   NodeType;
    uint8   MaxConcurrentConnections;
    uint32  MaxDataSize;
    boolean UseDiagnosticPowerMode;
    boolean UseCentralSecurity;
} DoIP_EntityConfigType;

/*==================================================================================================
*                                    DOIP CALLBACK FUNCTIONS TYPE
==================================================================================================*/
typedef void (*DoIP_UserVehicleIdResponseFncType)(
    const uint8* VIN,
    uint16 LogicalAddress,
    const uint8* EID,
    const uint8* GID,
    uint8 FurtherActions,
    uint8 VIN_GID_Status
);

typedef void (*DoIP_UserRoutingActivationResponseFncType)(
    uint16 LogicalAddressTester,
    uint16 LogicalAddressDoIP,
    uint8  ResponseCode,
    uint16 RoutingActivationNumber
);

typedef void (*DoIP_UserAliveCheckResponseFncType)(
    uint16 SourceAddress,
    boolean IsAlive
);

/*==================================================================================================
*                                    DOIP CONFIG TYPE
==================================================================================================*/
typedef struct {
    /* Configuration sub-structures */
    const DoIP_GeneralConfigType* GeneralConfig;
    const DoIP_TesterConfigType* TesterConfig;
    const DoIP_TargetConfigType* TargetConfig;
    const DoIP_SoConConfigType* SoConConfig;
    uint8 NumTesters;
    uint8 NumTargets;
    uint8 NumSoCons;
    /* Legacy fields (kept for backward compatibility) */
    const DoIP_EntityConfigType* Entity;
    const DoIP_ConnectionConfigType* Connections;
    uint8 NumConnections;
    const DoIP_RoutingActivationConfigType* RoutingActivations;
    uint8 NumRoutingActivations;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean UseVehicleIdentificationSync;
    uint8 DoIPVehicleAnnouncementCount;
    uint16 DoIPVehicleAnnouncementInterval;
    uint16 DoIPVehicleAnnouncementInitialDelay;
    DoIP_UserVehicleIdResponseFncType UserVehicleIdResponseFnc;
    DoIP_UserRoutingActivationResponseFncType UserRoutingActivationResponseFnc;
    DoIP_UserAliveCheckResponseFncType UserAliveCheckResponseFnc;
} DoIP_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DOIP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const DoIP_ConfigType DoIP_Config;

#define DOIP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DOIP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the DoIP module
 * @param ConfigPtr Pointer to configuration structure
 */
void DoIP_Init(const DoIP_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the DoIP module
 */
void DoIP_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void DoIP_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Transmits a diagnostic message via DoIP
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType DoIP_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Receive indication from lower layer (SoAd)
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void DoIP_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Activates a diagnostic routing path
 * @param SourceAddress Source logical address
 * @param TargetAddress Target logical address
 * @param ActivationType Routing activation type
 * @return Result of operation
 */
Std_ReturnType DoIP_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType);

/**
 * @brief Closes an active diagnostic connection
 * @param ConnectionId Connection identifier to close
 * @return Result of operation
 */
Std_ReturnType DoIP_CloseConnection(uint16 ConnectionId);

/**
 * @brief Triggers vehicle identification announcement
 * @return Result of operation
 */
Std_ReturnType DoIP_VehicleAnnouncement(void);

/**
 * @brief Requests entity status information
 * @param EntityIndex Entity index
 * @return Result of operation
 */
Std_ReturnType DoIP_RequestEntityStatus(uint8 EntityIndex);

/**
 * @brief Gets diagnostic power mode
 * @return Current power mode
 */
DoIP_PowerModeType DoIP_GetPowerMode(void);

/**
 * @brief Sets diagnostic power mode
 * @param PowerMode Power mode to set
 */
void DoIP_SetPowerMode(DoIP_PowerModeType PowerMode);

/**
 * @brief Handles alive check timeout for a connection
 * @param ConnectionId Connection ID
 */
void DoIP_HandleAliveCheckTimeout(uint16 ConnectionId);

/**
 * @brief Transmit confirmation callback from SoAd
 * @param TxPduId PDU that was transmitted
 * @param result Transmission result
 */
void DoIP_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief TCP connection establishment callback
 * @param SoConId Socket connection ID
 * @param Result Connection result
 */
void DoIP_SoConModeChg(uint16 SoConId, SoAd_ModeType Mode);

/**
 * @brief Main function for periodic processing
 */
void DoIP_MainFunction(void);

/**
 * @brief Trigger transmit callback from DCM
 * @param TxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType DoIP_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/**
 * @brief TpRxIndication callback from DCM
 * @param RxPduId PDU ID
 * @param Result Reception result
 */
void DoIP_TpRxIndication(PduIdType RxPduId, Std_ReturnType Result);

/**
 * @brief TpTxConfirmation callback from DCM
 * @param TxPduId PDU ID
 * @param Result Transmission result
 */
void DoIP_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

#define DOIP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DOIP_H */
