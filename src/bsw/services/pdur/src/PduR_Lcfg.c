/**
 * @file PduR_Lcfg.c
 * @brief PDU Router Link-Time Configuration
 * @version 1.0.0
 * @date 2026-04-23
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: PDU Router (PDUR)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "PduR.h"
#include "PduR_Cfg.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL CONSTANT DEFINITIONS
==================================================================================================*/
#define PDUR_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*--- Destination PDU configurations ---*/

/** @brief Engine Status TX: COM -> CanIf */
static const PduR_DestPduConfigType PduR_EngineStatusTx_Dest[] = {
    {
        .DestPduId = (PduIdType)0U,    /* CanIf TxPduId */
        .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Vehicle Speed TX: COM -> CanIf */
static const PduR_DestPduConfigType PduR_VehicleSpeedTx_Dest[] = {
    {
        .DestPduId = (PduIdType)1U,
        .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Battery Status TX: COM -> CanIf */
static const PduR_DestPduConfigType PduR_BatteryStatusTx_Dest[] = {
    {
        .DestPduId = (PduIdType)2U,
        .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Engine Command RX: CanIf -> COM */
static const PduR_DestPduConfigType PduR_EngineCmdRx_Dest[] = {
    {
        .DestPduId = PDUR_COM_RX_ENGINE_CMD,
        .DestModule = PDUR_MODULE_COM,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Vehicle Command RX: CanIf -> COM */
static const PduR_DestPduConfigType PduR_VehicleCmdRx_Dest[] = {
    {
        .DestPduId = PDUR_COM_RX_VEHICLE_CMD,
        .DestModule = PDUR_MODULE_COM,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Diagnostic Response TX: DCM -> CanIf */
static const PduR_DestPduConfigType PduR_DiagResponseTx_Dest[] = {
    {
        .DestPduId = (PduIdType)5U,
        .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/** @brief Diagnostic Request RX: CanIf -> DCM */
static const PduR_DestPduConfigType PduR_DiagRequestRx_Dest[] = {
    {
        .DestPduId = PDUR_DCM_RX_DIAG_REQUEST,
        .DestModule = PDUR_MODULE_DCM,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0U
    }
};

/*--- Routing path configurations ---*/
static const PduR_RoutingPathConfigType PduR_RoutingPaths[PDUR_NUMBER_OF_ROUTING_PATHS] = {
    /* Path 0: COM TX Engine Status -> CanIf */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_COM_TX_ENGINE_STATUS,
            .SourceModule = PDUR_MODULE_COM,
            .SduLength = 8U
        },
        .DestPdus = PduR_EngineStatusTx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 1: COM TX Vehicle Speed -> CanIf */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_COM_TX_VEHICLE_SPEED,
            .SourceModule = PDUR_MODULE_COM,
            .SduLength = 8U
        },
        .DestPdus = PduR_VehicleSpeedTx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 2: COM TX Battery Status -> CanIf */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_COM_TX_BATTERY_STATUS,
            .SourceModule = PDUR_MODULE_COM,
            .SduLength = 8U
        },
        .DestPdus = PduR_BatteryStatusTx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 3: CanIf RX Engine Command -> COM */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_COM_RX_ENGINE_CMD,
            .SourceModule = PDUR_MODULE_CANIF,
            .SduLength = 8U
        },
        .DestPdus = PduR_EngineCmdRx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 4: CanIf RX Vehicle Command -> COM */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_COM_RX_VEHICLE_CMD,
            .SourceModule = PDUR_MODULE_CANIF,
            .SduLength = 8U
        },
        .DestPdus = PduR_VehicleCmdRx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 5: DCM TX Diag Response -> CanIf */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_DCM_TX_DIAG_RESPONSE,
            .SourceModule = PDUR_MODULE_DCM,
            .SduLength = 8U
        },
        .DestPdus = PduR_DiagResponseTx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    },
    /* Path 6: CanIf RX Diag Request -> DCM */
    {
        .SrcPdu = {
            .SourcePduId = PDUR_DCM_RX_DIAG_REQUEST,
            .SourceModule = PDUR_MODULE_CANIF,
            .SduLength = 8U
        },
        .DestPdus = PduR_DiagRequestRx_Dest,
        .NumDestPdus = 1U,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    }
};

/*--- Routing path group configurations ---*/
static const PduIdType PduR_Group0_PduIds[] = {
    PDUR_COM_TX_ENGINE_STATUS,
    PDUR_COM_TX_VEHICLE_SPEED,
    PDUR_COM_TX_BATTERY_STATUS
};

static const PduIdType PduR_Group1_PduIds[] = {
    PDUR_COM_RX_ENGINE_CMD,
    PDUR_COM_RX_VEHICLE_CMD
};

static const PduIdType PduR_Group2_PduIds[] = {
    PDUR_DCM_TX_DIAG_RESPONSE,
    PDUR_DCM_RX_DIAG_REQUEST
};

static const PduR_RoutingPathGroupConfigType PduR_RoutingPathGroups[PDUR_NUMBER_OF_ROUTING_PATH_GROUPS] = {
    {
        .GroupId = PDUR_ROUTING_PATH_GROUP_0,
        .PduIds = PduR_Group0_PduIds,
        .NumPduIds = 3U,
        .DefaultEnabled = TRUE
    },
    {
        .GroupId = PDUR_ROUTING_PATH_GROUP_1,
        .PduIds = PduR_Group1_PduIds,
        .NumPduIds = 2U,
        .DefaultEnabled = TRUE
    },
    {
        .GroupId = PDUR_ROUTING_PATH_GROUP_2,
        .PduIds = PduR_Group2_PduIds,
        .NumPduIds = 2U,
        .DefaultEnabled = TRUE
    },
    {
        .GroupId = PDUR_ROUTING_PATH_GROUP_3,
        .PduIds = NULL_PTR,
        .NumPduIds = 0U,
        .DefaultEnabled = FALSE
    }
};

/*--- Global PduR configuration ---*/
const PduR_ConfigType PduR_Config = {
    .RoutingPaths = PduR_RoutingPaths,
    .NumRoutingPaths = 7U,
    .RoutingPathGroups = PduR_RoutingPathGroups,
    .NumRoutingPathGroups = 4U,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE
};

#define PDUR_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
