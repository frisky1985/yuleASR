/**
 * @file DoIp_Lcfg.c
 * @brief Diagnostic over IP Link-Time Configuration
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic over IP (DoIP)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "DoIp.h"
#include "DoIp_Cfg.h"

/*==================================================================================================
*                                  GLOBAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DOIP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*--- Connection configurations ---*/
static const DoIp_ConnectionConfigType DoIp_Connections[DOIP_MAX_CONNECTIONS] = {
    {
        .ConnectionId = DOIP_CONNECTION_0,
        .SourceAddress = DOIP_LOGICAL_ADDRESS_ECU,
        .TargetAddress = DOIP_LOGICAL_ADDRESS_TESTER,
        .TesterLogicalAddress = DOIP_LOGICAL_ADDRESS_TESTER,
        .AliveCheckTimeoutMs = 500U,
        .GeneralInactivityTimeoutMs = 5000U,
        .AliveCheckEnabled = TRUE
    },
    {
        .ConnectionId = DOIP_CONNECTION_1,
        .SourceAddress = DOIP_LOGICAL_ADDRESS_ECU,
        .TargetAddress = 0x0E01U,
        .TesterLogicalAddress = 0x0E01U,
        .AliveCheckTimeoutMs = 500U,
        .GeneralInactivityTimeoutMs = 5000U,
        .AliveCheckEnabled = TRUE
    },
    {
        .ConnectionId = DOIP_CONNECTION_2,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .TesterLogicalAddress = 0x0000U,
        .AliveCheckTimeoutMs = 0U,
        .GeneralInactivityTimeoutMs = 0U,
        .AliveCheckEnabled = FALSE
    },
    {
        .ConnectionId = DOIP_CONNECTION_3,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .TesterLogicalAddress = 0x0000U,
        .AliveCheckTimeoutMs = 0U,
        .GeneralInactivityTimeoutMs = 0U,
        .AliveCheckEnabled = FALSE
    }
};

/*--- Routing activation configurations ---*/
static const DoIp_RoutingActivationConfigType DoIp_RoutingActivations[DOIP_MAX_ROUTING_ACTIVATIONS] = {
    {
        .ActivationType = DOIP_ROUTING_ACTIVATION_DEFAULT,
        .SourceAddress = DOIP_LOGICAL_ADDRESS_TESTER,
        .TargetAddress = DOIP_LOGICAL_ADDRESS_ECU,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = DOIP_ROUTING_ACTIVATION_WWH_OBD,
        .SourceAddress = DOIP_LOGICAL_ADDRESS_TESTER,
        .TargetAddress = DOIP_LOGICAL_ADDRESS_ECU,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = 0x02U,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = 0x03U,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = 0x04U,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = 0x05U,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = 0x06U,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    },
    {
        .ActivationType = DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY,
        .SourceAddress = 0x0000U,
        .TargetAddress = 0x0000U,
        .NumAuthReqBytes = 0U,
        .NumConfirmReqBytes = 0U,
        .AuthenticationRequired = FALSE,
        .ConfirmationRequired = FALSE
    }
};

/*--- Global DoIp configuration ---*/
const DoIp_ConfigType DoIp_Config = {
    .Connections = DoIp_Connections,
    .NumConnections = 4U,
    .RoutingActivations = DoIp_RoutingActivations,
    .NumRoutingActivations = 8U,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE,
    .DoIpVehicleAnnouncementCount = 3U,
    .DoIpVehicleAnnouncementInterval = 500U
};

#define DOIP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
