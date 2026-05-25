/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**********************************************************************************
**                                                                               **
**  DoIP_Cfg.h - AUTOSAR DoIP Configuration Header                               **
**                                                                               **
**  Configuration for ISO 13400-2 diagnostic over IP protocol                    **
**                                                                               **
**********************************************************************************/

#ifndef DOIP_CFG_H
#define DOIP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*================================================================================
**  GENERAL CONFIGURATION OPTIONS
================================================================================*/

/* Development error detection */
#ifndef DOIP_DEV_ERROR_DETECT
#define DOIP_DEV_ERROR_DETECT             (STD_ON)
#endif

/* Version info API */
#ifndef DOIP_VERSION_INFO_API
#define DOIP_VERSION_INFO_API             (STD_ON)
#endif

/* Routing activation authentication */
#ifndef DOIP_ROUTING_ACTIVATION_AUTHENTICATION
#define DOIP_ROUTING_ACTIVATION_AUTHENTICATION    (STD_ON)
#endif

/* Routing activation confirmation */
#ifndef DOIP_ROUTING_ACTIVATION_CONFIRMATION
#define DOIP_ROUTING_ACTIVATION_CONFIRMATION      (STD_ON)
#endif

/* Alive check support */
#ifndef DOIP_ALIVE_CHECK_SUPPORT
#define DOIP_ALIVE_CHECK_SUPPORT          (STD_ON)
#endif

/* Node type */
#ifndef DOIP_NODE_TYPE
#define DOIP_NODE_TYPE                    DOIP_NODE_TYPE_GATEWAY
#endif

/* Vehicle announcement support */
#ifndef DOIP_VEHICLE_ANNOUNCEMENT
#define DOIP_VEHICLE_ANNOUNCEMENT         (STD_ON)
#endif

/* Use EID as VIN */
#ifndef DOIP_USE_EID_AS_VIN
#define DOIP_USE_EID_AS_VIN               (STD_OFF)
#endif

/* OBD support */
#ifndef DOIP_OBD_SUPPORT
#define DOIP_OBD_SUPPORT                  (STD_ON)
#endif

/*================================================================================
**  PROTOCOL CONFIGURATION
================================================================================*/

/* DoIP protocol version */
#define DOIP_PROTOCOL_VERSION             (0x03U)
#define DOIP_PROTOCOL_VERSION_INVERTED    (0xFCU)

/* DoIP port numbers */
#define DOIP_UDP_DISCOVERY_PORT           (13400U)
#define DOIP_TCP_DATA_PORT                (13400U)
#define DOIP_UDP_TEST_EQUIPMENT_PORT      (49152U)

/* Timeouts (in milliseconds) */
#define DOIP_GENERAL_INACTIVITY_TIMEOUT   (300000U)  /* 5 minutes */
#define DOIP_VEHICLE_ANNOUNCEMENT_INTERVAL (500U)    /* 500ms */
#define DOIP_VEHICLE_ANNOUNCEMENT_NUM     (3U)
#define DOIP_ALIVE_CHECK_RESPONSE_TIMEOUT (500U)    /* 500ms */
#define DOIP_INITIAL_INACTIVITY_TIME      (500U)
#define DOIP_ALIVE_CHECK_INTERVAL         (15000U)  /* 15 seconds */

/* Buffer sizes */
#define DOIP_BUFFER_SIZE                  (4096U)
#define DOIP_MAX_TESTER_CONNECTIONS       (4U)
#define DOIP_MAX_SIMULTANEOUS_REQUESTS    (2U)
#define DOIP_VIN_LENGTH                   (17U)
#define DOIP_EID_LENGTH                   (6U)
#define DOIP_GID_LENGTH                   (6U)

/* Maximum diagnostic message size */
#define DOIP_MAX_DIAG_MESSAGE_SIZE        (4095U)

/* Routing activation options */
#define DOIP_ROUTING_ACTIVATION_RESERVED  (0x00U)
#define DOIP_ROUTING_ACTIVATION_OEM       (0xFFFFFFFFU)

/* Further action codes */
#define DOIP_FURTHER_ACTION_NONE          (0x00U)
#define DOIP_FURTHER_ACTION_CENTRAL_SEC   (0x10U)

/* Sync status */
#define DOIP_SYNC_STATUS_COMPLETE         (0x00U)
#define DOIP_SYNC_STATUS_INCOMPLETE       (0x10U)

/*================================================================================
**  PDU CONFIGURATION
================================================================================*/

/* SoAd PDU IDs */
#define DOIP_UDP_SOAD_RX_PDU_ID           (0U)
#define DOIP_UDP_SOAD_TX_PDU_ID           (1U)
#define DOIP_TCP_SOAD_RX_PDU_ID_BASE      (2U)
#define DOIP_TCP_SOAD_TX_PDU_ID_BASE      (6U)

/* PduR PDU IDs */
#define DOIP_PDUR_RX_PDU_ID_BASE          (0U)
#define DOIP_PDUR_TX_PDU_ID_BASE          (10U)

/*================================================================================
**  NETWORK CONFIGURATION
================================================================================*/

/* Logical address configuration */
#define DOIP_LOCAL_LOGICAL_ADDRESS        (0x0001U)
#define DOIP_FUNCTIONAL_REQUEST_ADDR      (0xE400U)

/* Test equipment addresses */
#define DOIP_DEFAULT_TESTER_ADDRESS       (0x0E00U)
#define DOIP_WWH_OBD_TESTER_ADDRESS       (0x0E01U)
#define DOIP_CDS_TESTER_ADDRESS           (0x0E02U)

/*================================================================================
**  PAYLOAD TYPES
================================================================================*/

/* Payload types - Generic */
#define DOIP_PAYLOAD_TYPE_GENERIC_NACK    (0x0000U)
#define DOIP_PAYLOAD_TYPE_VID_REQUEST     (0x0001U)
#define DOIP_PAYLOAD_TYPE_VID_RESPONSE    (0x0004U)
#define DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_REQUEST  (0x0005U)
#define DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_RESPONSE (0x0006U)

/* Payload types - Alive check */
#define DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST  (0x0007U)
#define DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE (0x0008U)

/* Payload types - Diagnostic */
#define DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE        (0x8001U)
#define DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE_ACK    (0x8002U)
#define DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE_NACK   (0x8003U)

/* Payload types - Entity status */
#define DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST     (0x4001U)
#define DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE    (0x4002U)

/* Payload types - Power mode */
#define DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST   (0x4003U)
#define DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE  (0x4004U)

/* Payload types - Diagnostic power mode */
#define DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POWER_MODE_REQUEST  (0x4003U)
#define DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POWER_MODE_RESPONSE (0x4004U)

/* Generic NACK codes */
#define DOIP_NACK_CODE_INCORRECT_PATTERN_FORMAT     (0x00U)
#define DOIP_NACK_CODE_UNKNOWN_PAYLOAD_TYPE         (0x01U)
#define DOIP_NACK_CODE_MESSAGE_TOO_LARGE            (0x02U)
#define DOIP_NACK_CODE_OUT_OF_MEMORY                (0x03U)
#define DOIP_NACK_CODE_INVALID_PAYLOAD_LENGTH       (0x04U)

/*================================================================================
**  TYPE DEFINITIONS
================================================================================*/

/* Forward declaration */
struct DoIP_ConfigType;

/* Node type enumeration */
typedef enum
{
    DOIP_NODE_TYPE_GATEWAY = 0,
    DOIP_NODE_TYPE_NODE    = 1
} DoIP_NodeType;

/* Routing activation configuration */
typedef struct
{
    uint8                   routingActivationType;
    uint16                  testerLogicalAddress;
    boolean                 authenticationRequired;
    boolean                 confirmationRequired;
    boolean                 oemDataRequired;
    uint8                   priority;
    uint32                  timeout;
} DoIP_RoutingActivationConfigType;

/* Socket connection configuration */
typedef struct
{
    uint16                  socketId;
    boolean                 isTcp;
    uint16                  localPort;
    uint8                   localAddr[4];
    uint16                  remotePort;
    uint8                   remoteAddr[4];
    uint16                  rxPduId;
    uint16                  txPduId;
} DoIP_SocketConfigType;

/* Tester connection configuration */
typedef struct
{
    uint16                  testerLogicalAddress;
    uint8                   testerHwAddress[6];
    uint16                  routingActivationRef;
    boolean                 tlsRequired;
} DoIP_TesterConfigType;

/* Vehicle announcement configuration */
typedef struct
{
    uint8                   vin[17];
    uint8                   eid[6];
    uint8                   gid[6];
    uint16                  logicalAddress;
    uint8                   furtherActionReq;
    uint8                   syncStatus;
    uint16                  announcementInterval;
    uint8                   announcementCount;
    boolean                 useEidAsVin;
} DoIP_VehicleAnnouncementConfigType;

/* Main configuration structure */
typedef struct DoIP_ConfigType
{
    const DoIP_RoutingActivationConfigType* routingActivations;
    uint8                                   numRoutingActivations;
    const DoIP_SocketConfigType*            sockets;
    uint8                                   numSockets;
    const DoIP_TesterConfigType*            testers;
    uint8                                   numTesters;
    const DoIP_VehicleAnnouncementConfigType* vehicleAnnouncement;
} DoIP_ConfigType;

/*================================================================================
**  EXTERNAL CONFIGURATION REFERENCES
================================================================================*/

extern const DoIP_ConfigType DoIP_Config;

#ifdef __cplusplus
}
#endif

#endif /* DOIP_CFG_H */
