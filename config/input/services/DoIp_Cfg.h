/*
 * DoIP_Cfg.h
 * Diagnostic over IP Configuration Header
 */

#ifndef DOIP_CFG_H
#define DOIP_CFG_H

#include "Std_Types.h"

/*==================================================================================================
 *                                      VERSION INFORMATION
 *=================================================================================================*/
#define DOIP_CFG_VENDOR_ID              0x00U
#define DOIP_CFG_MODULE_ID              0x34U
#define DOIP_CFG_SW_MAJOR_VERSION       1U
#define DOIP_CFG_SW_MINOR_VERSION       0U
#define DOIP_CFG_SW_PATCH_VERSION       0U

/*==================================================================================================
 *                                      PRE-COMPILE CONFIGURATION
 *=================================================================================================*/
/* General Configuration */
#define DOIP_VERSION_INFO_API           STD_ON
#define DOIP_DEV_ERROR_DETECT           STD_ON

/* Feature Switches */
#define DOIP_VEHICLE_ANNOUNCEMENT       STD_ON
#define DOIP_NODE_TYPE                  0U      /* 0=DoIP Node, 1=DoIP Gateway */
#define DOIP_ENTITY_STATUS_SUPPORT      STD_ON
#define DOIP_POWER_MODE_SUPPORT         STD_ON

/* Maximum Configuration */
#define DOIP_MAX_CONNECTIONS            4U
#define DOIP_MAX_TESTER_CONNECTIONS     2U
#define DOIP_MAX_DIAGNOSTIC_MESSAGES    8U
#define DOIP_MAX_PAYLOAD_LENGTH         4096U

/* Logical Addresses */
#define DOIP_LOGICAL_ADDRESS            0x0E00U
#define DOIP_FUNCTIONAL_ADDRESS         0xE000U

/* Vehicle Identification */
#define DOIP_VIN                        "W0L000051T2123456"
#define DOIP_EID                        {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U}
#define DOIP_GID                        {0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU}

/* Further Action Byte */
#define DOIP_FURTHER_ACTION             0x00U   /* No further action required */

/* Activation Types Supported */
#define DOIP_DEFAULT_ACTIVATION_TYPE    0xE0U   /* Default OEM */
#define DOIP_WWH_OBD_ACTIVATION_TYPE    0xE1U   /* WWH-OBD */
#define DOIP_CENTRAL_SECURITY_TYPE      0xE2U   /* Central Security */

/* Timeout Configuration (in ms) */
#define DOIP_CFG_ANNOUNCE_WAIT          500U
#define DOIP_CFG_ANNOUNCE_INTERVAL      500U
#define DOIP_CFG_ANNOUNCE_NUM           3U
#define DOIP_CFG_INITIAL_INACTIVITY     2000U
#define DOIP_CFG_GENERAL_INACTIVITY     300000U
#define DOIP_CFG_ALIVE_CHECK_TIMEOUT    500U

/*==================================================================================================
 *                                      PDU CONFIGURATION
 *=================================================================================================*/
/* SoAd Socket Connection IDs */
#define DOIP_SOCON_UDP_DISCOVERY        0U
#define DOIP_SOCON_UDP_TEST_EQUIP       1U
#define DOIP_SOCON_TCP_DATA             2U
#define DOIP_SOCON_TCP_ROUTING          3U

/* PDU IDs */
#define DOIP_PDU_UDP_RX                 0U
#define DOIP_PDU_UDP_TX                 1U
#define DOIP_PDU_TCP_RX                 2U
#define DOIP_PDU_TCP_TX                 3U

/*==================================================================================================
 *                                      CALLBACK CONFIGURATION
 *=================================================================================================*/
/* Upper Layer Callbacks */
#define DOIP_UL_RXINDICATION            Dcm_DoIPRxIndication
#define DOIP_UL_TXCONFIRMATION          Dcm_DoIPTxConfirmation
#define DOIP_UL_ACTIVATION_CALLBACK     Dcm_DoIPRoutingActivation

/*==================================================================================================
 *                                      ADDRESS VALIDATION
 *=================================================================================================*/
/* Valid Tester Source Addresses */
#define DOIP_VALID_TESTER_ADDRESSES     {0x0E00U, 0x0E01U, 0x0E02U, 0x0E03U}

/* Valid Target Addresses */
#define DOIP_VALID_TARGET_ADDRESSES     {0x0001U, 0x0E00U, 0x0E01U, 0xE000U}

/*==================================================================================================
 *                                      TYPE DEFINITIONS
 *=================================================================================================*/
/* DoIP Configuration Structure */
typedef struct
{
    uint16  LogicalAddress;
    uint8   Vin[17];
    uint8   Eid[6];
    uint8   Gid[6];
    uint8   FurtherAction;
    uint16  MaxConnections;
    uint32  GeneralInactivityTime;
} DoIP_GeneralConfigType;

/* Tester Configuration */
typedef struct
{
    uint16  TesterAddress;
    boolean AuthenticationRequired;
    boolean ConfirmationRequired;
    uint8   AllowedActivationTypes;
} DoIP_TesterConfigType;

/* Target Address Configuration */
typedef struct
{
    uint16  TargetAddress;
    uint8   ProtocolType;       /* CAN, CANFD, LIN, etc. */
    uint16  LowerLayerPduId;
} DoIP_TargetConfigType;

/* Socket Connection Configuration */
typedef struct
{
    uint16  SoConId;
    boolean IsTcp;
    boolean IsUdp;
    uint16  LocalPort;
    uint8*  LocalIpAddress;
    uint16  RemotePort;
    uint8*  RemoteIpAddress;
} DoIP_SoConConfigType;

/* Complete DoIP Configuration */
typedef struct
{
    const DoIP_GeneralConfigType*   GeneralConfig;
    const DoIP_TesterConfigType*    TesterConfig;
    const DoIP_TargetConfigType*    TargetConfig;
    const DoIP_SoConConfigType*     SoConConfig;
    uint8                           NumTesters;
    uint8                           NumTargets;
    uint8                           NumSoCons;
} DoIP_ConfigType;

#endif /* DOIP_CFG_H */
