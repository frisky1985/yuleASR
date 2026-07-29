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

/**
 * @file DoIP_Cfg.h
 * @brief Diagnostic over IP configuration header - ISO 13400-2 compliant
 * @version 1.0.0
 * @date 2026-05-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DOIP_CFG_H
#define DOIP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DOIP_DEV_ERROR_DETECT           (STD_ON)
#define DOIP_VERSION_INFO_API           (STD_ON)
#define DOIP_DCM_SUPPORT                (STD_ON)
#define DOIP_SOAD_SUPPORT               (STD_ON)

/*==================================================================================================
*                                    PROTOCOL CONFIGURATION
==================================================================================================*/
#define DOIP_PROTOCOL_VERSION           (0x02U)    /* ISO 13400-2:2019 */
#define DOIP_PROTOCOL_VERSION_INVERT    (0xFDU)    /* ~0x02 */
#define DOIP_HEADER_LENGTH              (8U)       /* Generic header length */
#define DOIP_DIAG_MSG_HEADER_LENGTH     (4U)       /* SA(2) + TA(2) */
#define DOIP_ROUTING_ACTIVATION_REQ_LEN (7U)       /* SA(2) + ActType(1) + Res(4) */
#define DOIP_ROUTING_ACTIVATION_RES_LEN (13U)      /* TA(2) + LA(2) + ResCode(1) + Res(4) + OEM(4) */

/*==================================================================================================
*                                    CONNECTION CONFIGURATION
==================================================================================================*/
#define DOIP_MAX_CONNECTIONS            (4U)
#define DOIP_MAX_ROUTING_ACTIVATIONS    (8U)
#define DOIP_MAX_TESTERS                (4U)
#define DOIP_MAX_ENTITY                 (1U)

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/
#define DOIP_MAX_DIAG_REQUEST_LENGTH    (4096U)
#define DOIP_MAX_DIAG_RESPONSE_LENGTH   (4096U)
#define DOIP_MAX_VEHICLE_ID_LENGTH      (32U)
#define DOIP_MAX_ALIVE_CHECK_LENGTH     (8U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION (ms)
==================================================================================================*/
#define DOIP_MAIN_FUNCTION_PERIOD_MS            (10U)
#define DOIP_GENERAL_INACTIVITY_TIMEOUT_MS      (180000U)   /* 3 minutes */
#define DOIP_INITIAL_INACTIVITY_TIMEOUT_MS      (2000U)     /* 2 seconds */
#define DOIP_ALIVE_CHECK_TIMEOUT_MS             (500U)      /* 500ms */
#define DOIP_ALIVE_CHECK_RESPONSE_TIMEOUT_MS    (5000U)     /* 5 seconds */
#define DOIP_VEH_ANNOUNCEMENT_INTERVAL_MS       (500U)      /* 500ms */
#define DOIP_VEH_ANNOUNCEMENT_INITIAL_DELAY_MS  (500U)      /* 500ms */

/*==================================================================================================
*                                    VEHICLE ANNOUNCEMENT
==================================================================================================*/
#define DOIP_VEHICLE_ANNOUNCEMENT_COUNT         (3U)
#define DOIP_VEHICLE_ANNOUNCEMENT_INTERVAL      (50U)   /* 500ms in 10ms ticks */
#define DOIP_VEHICLE_ANNOUNCEMENT_INITIAL_DELAY (50U)   /* 500ms in 10ms ticks */

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define DOIP_DCM_TX_DIAG_REQUEST        ((PduIdType)0U)
#define DOIP_DCM_RX_DIAG_RESPONSE       ((PduIdType)1U)
#define DOIP_SOAD_TX_PDU_ID             ((PduIdType)0U)
#define DOIP_SOAD_RX_PDU_ID             ((PduIdType)1U)

/*==================================================================================================
*                                    CONNECTION IDs
==================================================================================================*/
#define DOIP_CONNECTION_0               (0U)
#define DOIP_CONNECTION_1               (1U)
#define DOIP_CONNECTION_2               (2U)
#define DOIP_CONNECTION_3               (3U)

/*==================================================================================================
*                                    SOCKET CONNECTION IDs
==================================================================================================*/
#define DOIP_SOCON_TCP_DATA_0           (0U)
#define DOIP_SOCON_TCP_DATA_1           (1U)
#define DOIP_SOCON_TCP_DATA_2           (2U)
#define DOIP_SOCON_TCP_DATA_3           (3U)
#define DOIP_SOCON_UDP_DISCOVERY        (4U)
#define DOIP_SOCON_UDP_TEST_EQUIP       (5U)

/*==================================================================================================
*                                    LOGICAL ADDRESSES
==================================================================================================*/
#define DOIP_LOGICAL_ADDRESS_ECU        (0x0001U)
#define DOIP_LOGICAL_ADDRESS_TESTER_1   (0x0E00U)
#define DOIP_LOGICAL_ADDRESS_TESTER_2   (0x0E01U)
#define DOIP_LOGICAL_ADDRESS_TESTER_3   (0x0E02U)
#define DOIP_LOGICAL_ADDRESS_TESTER_4   (0x0E03U)
#define DOIP_LOGICAL_ADDRESS_BROADCAST  (0xFFFFU)

/*==================================================================================================
*                                    ROUTING ACTIVATION IDs
==================================================================================================*/
#define DOIP_ROUTING_ACTIVATION_0       (0U)
#define DOIP_ROUTING_ACTIVATION_1       (1U)
#define DOIP_ROUTING_ACTIVATION_2       (2U)
#define DOIP_ROUTING_ACTIVATION_3       (3U)

/*==================================================================================================
*                                    ROUTING ACTIVATION TYPES (ISO 13400-2)
==================================================================================================*/
#define DOIP_ROUTING_ACTIVATION_DEFAULT             (0x00U)
#define DOIP_ROUTING_ACTIVATION_WWH_OBD             (0x01U)
#define DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY    (0xE0U)
#define DOIP_ROUTING_ACTIVATION_ADSB                (0xE1U)

/*==================================================================================================
*                                    PORT CONFIGURATION
==================================================================================================*/
#define DOIP_PORT_TCP_DATA              (13400U)
#define DOIP_PORT_UDP_DISCOVERY         (13400U)
#define DOIP_PORT_UDP_TEST_EQUIP        (13401U)

/*==================================================================================================
*                                    FURTHER ACTION BYTES
==================================================================================================*/
#define DOIP_FURTHER_ACTION_NO_FURTHER  (0x00U)
#define DOIP_FURTHER_ACTION_CENTRAL_SEC (0x10U)

/*==================================================================================================
*                                    VIN/GID STATUS
==================================================================================================*/
#define DOIP_VIN_GID_STATUS_VIN_INVALID     (0x00U)
#define DOIP_VIN_GID_STATUS_VIN_VALID       (0x01U)
#define DOIP_VIN_GID_STATUS_GID_SYNCHRONIZED (0x02U)
#define DOIP_VIN_GID_STATUS_GID_NOT_SYNC    (0x04U)

/*==================================================================================================
*                                    FEATURE ENABLES
==================================================================================================*/
#define DOIP_VEHICLE_DISCOVERY_ENABLED      (STD_ON)
#define DOIP_ROUTING_ACTIVATION_ENABLED     (STD_ON)
#define DOIP_DIAGNOSTIC_MESSAGE_ENABLED     (STD_ON)
#define DOIP_ALIVE_CHECK_ENABLED            (STD_ON)
#define DOIP_ENTITY_STATUS_ENABLED          (STD_ON)
#define DOIP_DIAGNOSTIC_POWER_MODE_ENABLED  (STD_ON)
#define DOIP_AUTHENTICATION_REQUIRED        (STD_OFF)
#define DOIP_CONFIRMATION_REQUIRED          (STD_OFF)
#define DOIP_USE_SECURE_CONNECTIONS         (STD_OFF)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/
#define DOIP_USER_VEHICLE_ID_RESPONSE_FNC       NULL_PTR
#define DOIP_USER_ROUTING_ACTIVATION_RESPONSE_FNC NULL_PTR
#define DOIP_USER_ALIVE_CHECK_RESPONSE_FNC      NULL_PTR

#endif /* DOIP_CFG_H */

/* Default configuration values for Lcfg */
#ifndef DOIP_CFG_GENERAL_INACTIVITY
#define DOIP_CFG_GENERAL_INACTIVITY            300000U  /* 5 minutes timeout */
#endif
#ifndef DOIP_DEFAULT_ACTIVATION_TYPE
#define DOIP_DEFAULT_ACTIVATION_TYPE           0x01U
#endif
#ifndef DOIP_WWH_OBD_ACTIVATION_TYPE
#define DOIP_WWH_OBD_ACTIVATION_TYPE           0x02U
#endif
#ifndef DOIP_CENTRAL_SECURITY_TYPE
#define DOIP_CENTRAL_SECURITY_TYPE             0x04U
#endif
#ifndef DOIP_VIN_LENGTH
#define DOIP_VIN_LENGTH                        17U
#endif
#ifndef DOIP_EID_LENGTH
#define DOIP_EID_LENGTH                        6U
#endif
#ifndef DOIP_GID_LENGTH
#define DOIP_GID_LENGTH                        6U
#endif
#ifndef DOIP_LOGICAL_ADDRESS
#define DOIP_LOGICAL_ADDRESS                   0x0E00U
#endif
#ifndef DOIP_FURTHER_ACTION
#define DOIP_FURTHER_ACTION                    0x00U
#endif
#ifndef DOIP_MAX_CONNECTIONS
#define DOIP_MAX_CONNECTIONS                   4U
#endif
#ifndef DOIP_VIN
#define DOIP_VIN                               "YULETECHASR000001"
#endif
#ifndef DOIP_EID
#define DOIP_EID                               {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E}
#endif
#ifndef DOIP_GID
#define DOIP_GID                               {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#endif

/* Test configuration types */
#ifndef DOIP_TESTER_CONFIG_TYPE_DEFINED
#define DOIP_TESTER_CONFIG_TYPE_DEFINED
typedef struct {
    uint16 TesterAddress;
    boolean AuthenticationRequired;
    boolean ConfirmationRequired;
    uint8 AllowedActivationTypes;
} DoIP_TesterConfigType;

typedef struct {
    uint16 TargetAddress;
    uint8 ProtocolType;
    uint16 LowerLayerPduId;
} DoIP_TargetConfigType;

typedef struct {
    uint16 LogicalAddress;
    const uint8* Vin;
    const uint8* Eid;
    const uint8* Gid;
    uint8 FurtherAction;
    uint8 MaxConnections;
    uint32 GeneralInactivityTime;
} DoIP_GeneralConfigType;
#endif
