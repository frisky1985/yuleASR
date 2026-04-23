/**
 * @file DoIp_Cfg.h
 * @brief Diagnostic over IP configuration header
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DOIP_CFG_H
#define DOIP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DOIP_DEV_ERROR_DETECT           (STD_ON)
#define DOIP_VERSION_INFO_API           (STD_ON)

/*==================================================================================================
*                                    CONNECTION CONFIGURATION
==================================================================================================*/
#define DOIP_MAX_CONNECTIONS            (4U)
#define DOIP_MAX_ROUTING_ACTIVATIONS    (8U)

/*==================================================================================================
*                                    DIAGNOSTIC MESSAGE LENGTH
==================================================================================================*/
#define DOIP_MAX_DIAG_REQUEST_LENGTH    (4096U)
#define DOIP_MAX_DIAG_RESPONSE_LENGTH   (4096U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define DOIP_MAIN_FUNCTION_PERIOD_MS    (10U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define DOIP_DCM_TX_DIAG_REQUEST        ((PduIdType)0U)
#define DOIP_DCM_RX_DIAG_RESPONSE       ((PduIdType)1U)
#define DOIP_SOAD_TX_PDU_ID             ((PduIdType)0U)
#define DOIP_SOAD_RX_PDU_ID             ((PduIdType)0U)

/*==================================================================================================
*                                    CONNECTION IDs
==================================================================================================*/
#define DOIP_CONNECTION_0               (0U)
#define DOIP_CONNECTION_1               (1U)
#define DOIP_CONNECTION_2               (2U)
#define DOIP_CONNECTION_3               (3U)

/*==================================================================================================
*                                    LOGICAL ADDRESSES
==================================================================================================*/
#define DOIP_LOGICAL_ADDRESS_ECU        (0x0001U)
#define DOIP_LOGICAL_ADDRESS_TESTER     (0x0E00U)

/*==================================================================================================
*                                    ROUTING ACTIVATION IDs
==================================================================================================*/
#define DOIP_ROUTING_ACTIVATION_0       (0U)
#define DOIP_ROUTING_ACTIVATION_1       (1U)

#endif /* DOIP_CFG_H */
