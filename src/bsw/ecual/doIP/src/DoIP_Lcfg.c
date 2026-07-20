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
**  DoIP_Lcfg.c - AUTOSAR DoIP Link-time Configuration                           **
**                                                                               **
**  Configuration tables for ISO 13400-2 diagnostic over IP protocol             **
**                                                                               **
**********************************************************************************/

#include "DoIP_Cfg.h"

/*================================================================================
**  VEHICLE IDENTIFICATION DATA
================================================================================*/

/* Vehicle Identification Number (VIN) - Example: WBA1234567890ABCD */
static const uint8 DoIP_VIN[DOIP_VIN_LENGTH] =
{
    'W', 'B', 'A', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '0', 'A', 'B', 'C', 'D'
};

/* Entity ID (EID) - MAC address format */
static const uint8 DoIP_EID[DOIP_EID_LENGTH] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05
};

/* Group ID (GID) - Configuration group identifier */
static const uint8 DoIP_GID[DOIP_GID_LENGTH] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05
};

/*================================================================================
**  ROUTING ACTIVATION CONFIGURATION
================================================================================*/

/* Routing activation configurations */
static const DoIP_RoutingActivationConfigType DoIP_RoutingActivations[] =
{
    {
        /* Default routing activation */
        .routingActivationType   = DOIP_ROUTING_ACTIVATION_DEFAULT,
        .testerLogicalAddress    = DOIP_DEFAULT_TESTER_ADDRESS,
        .authenticationRequired  = FALSE,
        .confirmationRequired    = FALSE,
        .oemDataRequired         = FALSE,
        .priority                = 0,
        .timeout                 = 1000
    },
    {
        /* WWH-OBD routing activation */
        .routingActivationType   = DOIP_ROUTING_ACTIVATION_WWH_OBD,
        .testerLogicalAddress    = DOIP_WWH_OBD_TESTER_ADDRESS,
        .authenticationRequired  = FALSE,
        .confirmationRequired    = FALSE,
        .oemDataRequired         = FALSE,
        .priority                = 1,
        .timeout                 = 1000
    },
    {
        /* Central diagnostic system routing activation */
        .routingActivationType   = DOIP_ROUTING_ACTIVATION_CDS,
        .testerLogicalAddress    = DOIP_CDS_TESTER_ADDRESS,
        .authenticationRequired  = TRUE,
        .confirmationRequired    = TRUE,
        .oemDataRequired         = TRUE,
        .priority                = 2,
        .timeout                 = 2000
    },
    {
        /* Central Security routing activation */
        .routingActivationType   = DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY,
        .testerLogicalAddress    = 0x0E03,
        .authenticationRequired  = TRUE,
        .confirmationRequired    = TRUE,
        .oemDataRequired         = TRUE,
        .priority                = 3,
        .timeout                 = 5000
    }
};

#define DOIP_NUM_ROUTING_ACTIVATIONS \
    (sizeof(DoIP_RoutingActivations) / sizeof(DoIP_RoutingActivations[0]))

/*================================================================================
**  SOCKET CONFIGURATION
================================================================================*/

/* Socket connection configurations */
static const DoIP_SocketConfigType DoIP_Sockets[] =
{
    /* UDP Discovery socket */
    {
        .socketId     = 0,
        .isTcp        = FALSE,
        .localPort    = DOIP_UDP_DISCOVERY_PORT,
        .localAddr    = {0x00, 0x00, 0x00, 0x00}, /* INADDR_ANY */
        .remotePort   = 0,
        .remoteAddr   = {0x00, 0x00, 0x00, 0x00},
        .rxPduId      = DOIP_UDP_SOAD_RX_PDU_ID,
        .txPduId      = DOIP_UDP_SOAD_TX_PDU_ID
    },
    /* TCP Data socket - Connection 0 */
    {
        .socketId     = 1,
        .isTcp        = TRUE,
        .localPort    = DOIP_TCP_DATA_PORT,
        .localAddr    = {0x00, 0x00, 0x00, 0x00}, /* INADDR_ANY */
        .remotePort   = 0,
        .remoteAddr   = {0x00, 0x00, 0x00, 0x00},
        .rxPduId      = DOIP_TCP_SOAD_RX_PDU_ID_BASE + 0,
        .txPduId      = DOIP_TCP_SOAD_TX_PDU_ID_BASE + 0
    },
    /* TCP Data socket - Connection 1 */
    {
        .socketId     = 2,
        .isTcp        = TRUE,
        .localPort    = DOIP_TCP_DATA_PORT,
        .localAddr    = {0x00, 0x00, 0x00, 0x00}, /* INADDR_ANY */
        .remotePort   = 0,
        .remoteAddr   = {0x00, 0x00, 0x00, 0x00},
        .rxPduId      = DOIP_TCP_SOAD_RX_PDU_ID_BASE + 1,
        .txPduId      = DOIP_TCP_SOAD_TX_PDU_ID_BASE + 1
    },
    /* TCP Data socket - Connection 2 */
    {
        .socketId     = 3,
        .isTcp        = TRUE,
        .localPort    = DOIP_TCP_DATA_PORT,
        .localAddr    = {0x00, 0x00, 0x00, 0x00}, /* INADDR_ANY */
        .remotePort   = 0,
        .remoteAddr   = {0x00, 0x00, 0x00, 0x00},
        .rxPduId      = DOIP_TCP_SOAD_RX_PDU_ID_BASE + 2,
        .txPduId      = DOIP_TCP_SOAD_TX_PDU_ID_BASE + 2
    },
    /* TCP Data socket - Connection 3 */
    {
        .socketId     = 4,
        .isTcp        = TRUE,
        .localPort    = DOIP_TCP_DATA_PORT,
        .localAddr    = {0x00, 0x00, 0x00, 0x00}, /* INADDR_ANY */
        .remotePort   = 0,
        .remoteAddr   = {0x00, 0x00, 0x00, 0x00},
        .rxPduId      = DOIP_TCP_SOAD_RX_PDU_ID_BASE + 3,
        .txPduId      = DOIP_TCP_SOAD_TX_PDU_ID_BASE + 3
    }
};

#define DOIP_NUM_SOCKETS \
    (sizeof(DoIP_Sockets) / sizeof(DoIP_Sockets[0]))

/*================================================================================
**  TESTER CONNECTION CONFIGURATION
================================================================================*/

/* Tester connection configurations */
static const DoIP_TesterConfigType DoIP_Testers[] =
{
    /* Default tester */
    {
        .testerLogicalAddress = DOIP_DEFAULT_TESTER_ADDRESS,
        .testerHwAddress      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x10},
        .routingActivationRef = 0, /* Index into routing activations */
        .tlsRequired          = FALSE
    },
    /* WWH-OBD tester */
    {
        .testerLogicalAddress = DOIP_WWH_OBD_TESTER_ADDRESS,
        .testerHwAddress      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x11},
        .routingActivationRef = 1,
        .tlsRequired          = FALSE
    },
    /* Central diagnostic system tester */
    {
        .testerLogicalAddress = DOIP_CDS_TESTER_ADDRESS,
        .testerHwAddress      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x12},
        .routingActivationRef = 2,
        .tlsRequired          = TRUE
    },
    /* Central Security tester */
    {
        .testerLogicalAddress = 0x0E03,
        .testerHwAddress      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x13},
        .routingActivationRef = 3,
        .tlsRequired          = TRUE
    }
};

#define DOIP_NUM_TESTERS \
    (sizeof(DoIP_Testers) / sizeof(DoIP_Testers[0]))

/*================================================================================
**  VEHICLE ANNOUNCEMENT CONFIGURATION
================================================================================*/

/* Vehicle announcement configuration */
static const DoIP_VehicleAnnouncementConfigType DoIP_VehicleAnnouncementConfig =
{
    .vin                    = {'W', 'B', 'A', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', '0', 'A', 'B', 'C', 'D'},
    .eid                    = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    .gid                    = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    .logicalAddress         = DOIP_LOCAL_LOGICAL_ADDRESS,
    .furtherActionReq       = DOIP_FURTHER_ACTION_NONE,
    .syncStatus             = DOIP_SYNC_STATUS_COMPLETE,
    .announcementInterval   = DOIP_VEHICLE_ANNOUNCEMENT_INTERVAL,
    .announcementCount      = DOIP_VEHICLE_ANNOUNCEMENT_NUM,
    .useEidAsVin            = DOIP_USE_EID_AS_VIN
};

/*================================================================================
**  MAIN CONFIGURATION STRUCTURE
================================================================================*/

/* Main DoIP configuration */
const DoIP_ConfigType DoIP_Config =
{
    .routingActivations   = DoIP_RoutingActivations,
    .numRoutingActivations = (uint8)DOIP_NUM_ROUTING_ACTIVATIONS,
    .sockets              = DoIP_Sockets,
    .numSockets           = (uint8)DOIP_NUM_SOCKETS,
    .testers              = DoIP_Testers,
    .numTesters           = (uint8)DOIP_NUM_TESTERS,
    .vehicleAnnouncement  = &DoIP_VehicleAnnouncementConfig
};

/*================================================================================
**  LOGICAL ADDRESS TABLE
================================================================================*/

/* Logical address mapping table for diagnostic message routing */
static const uint16 DoIP_LogicalAddressTable[] =
{
    DOIP_LOCAL_LOGICAL_ADDRESS,    /* Local ECU */
    0x0002,                        /* Example ECU 2 */
    0x0003,                        /* Example ECU 3 */
    DOIP_FUNCTIONAL_REQUEST_ADDR   /* Functional request broadcast */
};

#define DOIP_NUM_LOGICAL_ADDRESSES \
    (sizeof(DoIP_LogicalAddressTable) / sizeof(DoIP_LogicalAddressTable[0]))

/*================================================================================
**  PDU ROUTING TABLE
================================================================================*/

/* PDU routing configuration for DoIP <-> PduR interface */
static const PduIdType DoIP_PduRRoutingTable[] =
{
    /* RX PDUs - Incoming diagnostic messages to PduR */
    DOIP_PDUR_RX_PDU_ID_BASE + 0,  /* Tester 0 diagnostic requests */
    DOIP_PDUR_RX_PDU_ID_BASE + 1,  /* Tester 1 diagnostic requests */
    DOIP_PDUR_RX_PDU_ID_BASE + 2,  /* Tester 2 diagnostic requests */
    DOIP_PDUR_RX_PDU_ID_BASE + 3,  /* Tester 3 diagnostic requests */

    /* TX PDUs - Outgoing diagnostic messages from PduR */
    DOIP_PDUR_TX_PDU_ID_BASE + 0,  /* Responses to Tester 0 */
    DOIP_PDUR_TX_PDU_ID_BASE + 1,  /* Responses to Tester 1 */
    DOIP_PDUR_TX_PDU_ID_BASE + 2,  /* Responses to Tester 2 */
    DOIP_PDUR_TX_PDU_ID_BASE + 3   /* Responses to Tester 3 */
};

#define DOIP_NUM_PDUS \
    (sizeof(DoIP_PduRRoutingTable) / sizeof(DoIP_PduRRoutingTable[0]))

/*================================================================================
**  CONNECTION TIMEOUT CONFIGURATION
================================================================================*/

/* Connection timeout values (in milliseconds) */
static const uint16 DoIP_ConnectionTimeouts[] =
{
    DOIP_GENERAL_INACTIVITY_TIMEOUT,  /* General inactivity timeout */
    DOIP_ALIVE_CHECK_RESPONSE_TIMEOUT, /* Alive check response timeout */
    DOIP_INITIAL_INACTIVITY_TIME,      /* Initial inactivity time */
    DOIP_ALIVE_CHECK_INTERVAL          /* Alive check interval */
};

/*================================================================================
**  TARGET ADDRESS CONFIGURATION (FOR GATEWAY NODES)
================================================================================*/

#if (DOIP_NODE_TYPE == DOIP_NODE_TYPE_GATEWAY)

/* Target network information for gateway routing */
typedef struct
{
    uint16 logicalAddress;
    uint16 networkId;
    uint8  protocolType;
    uint8  targetAddr[6];
} DoIP_TargetNetworkConfigType;

static const DoIP_TargetNetworkConfigType DoIP_TargetNetworks[] =
{
    {
        .logicalAddress = 0x0002,
        .networkId      = 1,  /* CAN network */
        .protocolType   = 0,  /* CAN */
        .targetAddr     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    },
    {
        .logicalAddress = 0x0003,
        .networkId      = 2,  /* CAN-FD network */
        .protocolType   = 1,  /* CAN-FD */
        .targetAddr     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    }
};

#define DOIP_NUM_TARGET_NETWORKS \
    (sizeof(DoIP_TargetNetworks) / sizeof(DoIP_TargetNetworks[0]))

#endif /* DOIP_NODE_TYPE_GATEWAY */
