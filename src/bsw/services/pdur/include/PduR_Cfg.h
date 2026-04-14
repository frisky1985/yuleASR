/**
 * @file PduR_Cfg.h
 * @brief PDU Router configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef PDUR_CFG_H
#define PDUR_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define PDUR_DEV_ERROR_DETECT           (STD_ON)
#define PDUR_VERSION_INFO_API           (STD_ON)

/*==================================================================================================
*                                    ROUTING PATH CONFIGURATION
==================================================================================================*/
#define PDUR_NUMBER_OF_ROUTING_PATHS    (16U)
#define PDUR_NUMBER_OF_ROUTING_PATH_GROUPS (4U)
#define PDUR_MAX_DESTINATIONS_PER_PATH  (4U)

/*==================================================================================================
*                                    MODULE IDs
==================================================================================================*/
#define PDUR_MODULE_CANIF               (0x3CU)
#define PDUR_MODULE_CANTP               (0x3DU)
#define PDUR_MODULE_LINIF               (0x3EU)
#define PDUR_MODULE_COM                 (0x64U)
#define PDUR_MODULE_DCM                 (0x29U)
#define PDUR_MODULE_SOAD                (0x43U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define PDUR_COM_TX_ENGINE_STATUS       ((PduIdType)0U)
#define PDUR_COM_TX_VEHICLE_SPEED       ((PduIdType)1U)
#define PDUR_COM_TX_BATTERY_STATUS      ((PduIdType)2U)
#define PDUR_COM_RX_ENGINE_CMD          ((PduIdType)3U)
#define PDUR_COM_RX_VEHICLE_CMD         ((PduIdType)4U)
#define PDUR_DCM_TX_DIAG_RESPONSE       ((PduIdType)5U)
#define PDUR_DCM_RX_DIAG_REQUEST        ((PduIdType)6U)

/*==================================================================================================
*                                    ROUTING PATH GROUP IDs
==================================================================================================*/
#define PDUR_ROUTING_PATH_GROUP_0       (0U)
#define PDUR_ROUTING_PATH_GROUP_1       (1U)
#define PDUR_ROUTING_PATH_GROUP_2       (2U)
#define PDUR_ROUTING_PATH_GROUP_3       (3U)

/*==================================================================================================
*                                    GATEWAY OPERATIONS
==================================================================================================*/
#define PDUR_GATEWAY_OPERATION_ENABLED  (STD_ON)
#define PDUR_FIFO_DEPTH                 (8U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define PDUR_MAIN_FUNCTION_PERIOD_MS    (10U)

#endif /* PDUR_CFG_H */
