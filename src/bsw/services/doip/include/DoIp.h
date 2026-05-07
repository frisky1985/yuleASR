/**
 * @file DoIp.h
 * @brief Diagnostic over IP module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic over IP (DoIP)
 * Layer: Service Layer
 */

#ifndef DOIP_H
#define DOIP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "DoIp_Cfg.h"
#include "ComStack_Types.h"

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

/*==================================================================================================
*                                    DOIP STATE TYPE
==================================================================================================*/
typedef enum {
    DOIP_STATE_UNINIT = 0,
    DOIP_STATE_INIT,
    DOIP_STATE_ACTIVE
} DoIp_StateType;

/*==================================================================================================
*                                    DOIP CONNECTION STATE TYPE
==================================================================================================*/
typedef enum {
    DOIP_CONNECTION_CLOSED = 0,
    DOIP_CONNECTION_PENDING,
    DOIP_CONNECTION_ESTABLISHED,
    DOIP_CONNECTION_REGISTERED
} DoIp_ConnectionStateType;

/*==================================================================================================
*                                    DOIP ROUTING ACTIVATION TYPE
==================================================================================================*/
typedef enum {
    DOIP_ROUTING_ACTIVATION_DEFAULT = 0x00,
    DOIP_ROUTING_ACTIVATION_WWH_OBD = 0x01,
    DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY = 0xE0
} DoIp_RoutingActivationType;

/*==================================================================================================
*                                    DOIP PAYLOAD TYPE
==================================================================================================*/
typedef enum {
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_REQ        = 0x0001,
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_RES        = 0x0002,
    DOIP_PAYLOAD_ROUTING_ACTIVATION_REQ            = 0x0005,
    DOIP_PAYLOAD_ROUTING_ACTIVATION_RES            = 0x0006,
    DOIP_PAYLOAD_ALIVE_CHECK_REQ                   = 0x0007,
    DOIP_PAYLOAD_ALIVE_CHECK_RES                   = 0x0008,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE                = 0x8001,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_POS_ACK        = 0x8002,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_NEG_ACK        = 0x8003
} DoIp_PayloadType;

/*==================================================================================================
*                                    DOIP GENERIC HEADER TYPE
==================================================================================================*/
typedef struct {
    uint8  ProtocolVersion;
    uint8  InverseProtocolVersion;
    uint16 PayloadType;
    uint32 PayloadLength;
} DoIp_GenericHeaderType;

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
    boolean AliveCheckEnabled;
} DoIp_ConnectionConfigType;

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
} DoIp_RoutingActivationConfigType;

/*==================================================================================================
*                                    DOIP CONFIG TYPE
==================================================================================================*/
typedef struct {
    const DoIp_ConnectionConfigType* Connections;
    uint8 NumConnections;
    const DoIp_RoutingActivationConfigType* RoutingActivations;
    uint8 NumRoutingActivations;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    uint8 DoIpVehicleAnnouncementCount;
    uint16 DoIpVehicleAnnouncementInterval;
} DoIp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DOIP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const DoIp_ConfigType DoIp_Config;

#define DOIP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DOIP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the DoIp module
 * @param ConfigPtr Pointer to configuration structure
 */
void DoIp_Init(const DoIp_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the DoIp module
 */
void DoIp_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void DoIp_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Transmits a diagnostic message via DoIP
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType DoIp_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Receive indication from lower layer (SoAd)
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void DoIp_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Activates a diagnostic routing path
 * @param SourceAddress Source logical address
 * @param TargetAddress Target logical address
 * @param ActivationType Routing activation type
 * @return Result of operation
 */
Std_ReturnType DoIp_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType);

/**
 * @brief Closes an active diagnostic connection
 * @param ConnectionId Connection identifier to close
 * @return Result of operation
 */
Std_ReturnType DoIp_CloseConnection(uint16 ConnectionId);

/**
 * @brief Transmit confirmation callback from SoAd
 * @param TxPduId PDU that was transmitted
 * @param result Transmission result
 */
void DoIp_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Main function for periodic processing
 */
void DoIp_MainFunction(void);

#define DOIP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DOIP_H */
