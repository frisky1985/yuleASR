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

/*
 * DoIP_Lcfg.c
 * Diagnostic over IP Link-Time Configuration
 */

#include "DoIP.h"
#include "DoIP_Cfg.h"

/*==================================================================================================
 *                                      VEHICLE IDENTIFICATION
 *=================================================================================================*/
/* VIN (Vehicle Identification Number) - 17 characters */
const uint8 DoIP_Vin[DOIP_VIN_LENGTH] = DOIP_VIN;

/* EID (Entity ID) - MAC address format */
const uint8 DoIP_Eid[DOIP_EID_LENGTH] = DOIP_EID;

/* GID (Group ID) - For vehicle identification */
const uint8 DoIP_Gid[DOIP_GID_LENGTH] = DOIP_GID;

/* Entity Logical Address */
const uint16 DoIP_EntityLogicalAddress = DOIP_LOGICAL_ADDRESS;

/*==================================================================================================
 *                                      GENERAL CONFIGURATION
 *=================================================================================================*/
const DoIP_GeneralConfigType DoIP_GeneralConfig =
{
    .LogicalAddress = DOIP_LOGICAL_ADDRESS,
    .Vin = DOIP_VIN,
    .Eid = DOIP_EID,
    .Gid = DOIP_GID,
    .FurtherAction = DOIP_FURTHER_ACTION,
    .MaxConnections = DOIP_MAX_CONNECTIONS,
    .GeneralInactivityTime = DOIP_CFG_GENERAL_INACTIVITY
};

/*==================================================================================================
 *                                      TESTER CONFIGURATION
 *=================================================================================================*/
static const DoIP_TesterConfigType DoIP_Testers[] =
{
    {
        .TesterAddress = 0x0E00U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE,
        .AllowedActivationTypes = DOIP_DEFAULT_ACTIVATION_TYPE
    },
    {
        .TesterAddress = 0x0E01U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE,
        .AllowedActivationTypes = DOIP_DEFAULT_ACTIVATION_TYPE | DOIP_WWH_OBD_ACTIVATION_TYPE
    },
    {
        .TesterAddress = 0x0E02U,
        .AuthenticationRequired = TRUE,
        .ConfirmationRequired = TRUE,
        .AllowedActivationTypes = DOIP_CENTRAL_SECURITY_TYPE
    },
    {
        .TesterAddress = 0x0E03U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE,
        .AllowedActivationTypes = DOIP_DEFAULT_ACTIVATION_TYPE
    }
};

#define DOIP_NUM_TESTERS    (sizeof(DoIP_Testers) / sizeof(DoIP_Testers[0]))

/*==================================================================================================
 *                                      TARGET CONFIGURATION
 *=================================================================================================*/
static const DoIP_TargetConfigType DoIP_Targets[] =
{
    {
        .TargetAddress = 0x0001U,       /* ECU 1 */
        .ProtocolType = 0x01U,          /* CAN */
        .LowerLayerPduId = 0x00U
    },
    {
        .TargetAddress = 0x0E00U,       /* Diagnostic target */
        .ProtocolType = 0x01U,          /* CAN */
        .LowerLayerPduId = 0x01U
    },
    {
        .TargetAddress = 0x0E01U,       /* Diagnostic target */
        .ProtocolType = 0x02U,          /* CAN FD */
        .LowerLayerPduId = 0x02U
    },
    {
        .TargetAddress = 0xE000U,       /* Functional address */
        .ProtocolType = 0x01U,          /* CAN */
        .LowerLayerPduId = 0x03U
    }
};

#define DOIP_NUM_TARGETS    (sizeof(DoIP_Targets) / sizeof(DoIP_Targets[0]))

/*==================================================================================================
 *                                      SOCKET CONNECTION CONFIGURATION
 *=================================================================================================*/
static const DoIP_SoConConfigType DoIP_SoConConfigs[] =
{
    {
        /* UDP Discovery socket */
        .SoConId = DOIP_SOCON_UDP_DISCOVERY,
        .IsTcp = FALSE,
        .IsUdp = TRUE,
        .LocalPort = 13400U,
        .LocalIpAddress = NULL,         /* Any */
        .RemotePort = 0U,
        .RemoteIpAddress = NULL         /* Any */
    },
    {
        /* UDP Test Equipment socket */
        .SoConId = DOIP_SOCON_UDP_TEST_EQUIP,
        .IsTcp = FALSE,
        .IsUdp = TRUE,
        .LocalPort = 13401U,
        .LocalIpAddress = NULL,
        .RemotePort = 0U,
        .RemoteIpAddress = NULL
    },
    {
        /* TCP Data socket */
        .SoConId = DOIP_SOCON_TCP_DATA,
        .IsTcp = TRUE,
        .IsUdp = FALSE,
        .LocalPort = 13400U,
        .LocalIpAddress = NULL,
        .RemotePort = 0U,
        .RemoteIpAddress = NULL
    },
    {
        /* TCP Routing socket */
        .SoConId = DOIP_SOCON_TCP_ROUTING,
        .IsTcp = TRUE,
        .IsUdp = FALSE,
        .LocalPort = 13401U,
        .LocalIpAddress = NULL,
        .RemotePort = 0U,
        .RemoteIpAddress = NULL
    }
};

#define DOIP_NUM_SOCONS     (sizeof(DoIP_SoConConfigs) / sizeof(DoIP_SoConConfigs[0]))

/*==================================================================================================
 *                                      COMPLETE CONFIGURATION
 *=================================================================================================*/
const DoIP_ConfigType DoIP_Config =
{
    .GeneralConfig = &DoIP_GeneralConfig,
    .TesterConfig = DoIP_Testers,
    .TargetConfig = DoIP_Targets,
    .SoConConfig = DoIP_SoConConfigs,
    .NumTesters = DOIP_NUM_TESTERS,
    .NumTargets = DOIP_NUM_TARGETS,
    .NumSoCons = DOIP_NUM_SOCONS
};

/*==================================================================================================
 *                                      STATE VARIABLE
 *=================================================================================================*/
DoIP_StateType DoIP_State = DOIP_STATE_UNINIT;

/*==================================================================================================
 *                                      CALLBACK FUNCTIONS
 *=================================================================================================*/
/**
 * @brief Rx Indication callback for upper layer (DCM)
 * @param SoConId Socket connection ID
 * @param Data Pointer to received data
 * @param Length Data length
 */
void Dcm_DoIPRxIndication(uint16 SoConId, const uint8* Data, uint32 Length)
{
    (void)SoConId;
    (void)Data;
    (void)Length;
    
    /* Forward to DCM for UDS processing */
    /* Dcm_ProcessDoIPMessage(Data, Length); */
}

/**
 * @brief Tx Confirmation callback for upper layer (DCM)
 * @param SoConId Socket connection ID
 * @param Result Transmission result
 */
void Dcm_DoIPTxConfirmation(uint16 SoConId, Std_ReturnType Result)
{
    (void)SoConId;
    (void)Result;
    
    /* Notify DCM about transmission completion */
}

/**
 * @brief Routing activation callback for upper layer (DCM)
 * @param SoConId Socket connection ID
 * @param SourceAddress Tester source address
 * @param Result Activation result
 */
void Dcm_DoIPRoutingActivation(uint16 SoConId, uint16 SourceAddress, Std_ReturnType Result)
{
    (void)SoConId;
    (void)SourceAddress;
    (void)Result;
    
    /* Notify DCM about routing activation */
}
