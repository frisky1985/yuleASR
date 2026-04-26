/******************************************************************************
 * @file    PduR_Cfg.h
 * @brief   PDU Router (PduR) Configuration - AUTOSAR R22-11
 *
 * Configuration header for PduR module.
 * This file contains all configurable parameters for the PDU Router.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x37 (PduR)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef PDUR_CFG_H
#define PDUR_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define PDUR_CFG_VENDOR_ID                      0x01U
#define PDUR_CFG_MODULE_ID                      0x37U
#define PDUR_CFG_AR_MAJOR_VERSION               22U
#define PDUR_CFG_AR_MINOR_VERSION               11U
#define PDUR_CFG_AR_PATCH_VERSION               0U
#define PDUR_CFG_SW_MAJOR_VERSION               1U
#define PDUR_CFG_SW_MINOR_VERSION               0U
#define PDUR_CFG_SW_PATCH_VERSION               0U

/******************************************************************************
 * General Configuration
 ******************************************************************************/

/**
 * @brief Development error detection enable
 * @details If enabled, DET reporting is active for development errors
 */
#define PDUR_DEV_ERROR_DETECT                   STD_ON

/**
 * @brief Version info API enable
 * @details If enabled, PduR_GetVersionInfo API is available
 */
#define PDUR_VERSION_INFO_API                   STD_ON

/**
 * @brief Zero cost operation mode
 * @details If enabled, PduR operates in zero cost mode (no runtime checks)
 */
#define PDUR_ZERO_COST_OPERATION                STD_OFF

/**
 * @brief Single processor configuration
 * @details If enabled, single processor optimizations are applied
 */
#define PDUR_SINGLE_PROCESSOR                   STD_ON

/**
 * @brief Multicore support enable
 * @details If enabled, multicore routing is supported
 */
#define PDUR_MULTICORE_SUPPORT                  STD_OFF

/******************************************************************************
 * Routing Configuration
 ******************************************************************************/

/**
 * @brief Maximum number of routing paths
 * @details This defines the size of the routing table
 */
#define PDUR_CFG_MAX_ROUTING_PATHS              64U

/**
 * @brief Maximum number of PDUs
 */
#define PDUR_CFG_MAX_PDUS                       128U

/**
 * @brief IF routing support
 * @details Enable Interface routing (single frame)
 */
#define PDUR_IF_ROUTING_SUPPORT                 STD_ON

/**
 * @brief TP routing support
 * @details Enable Transport Protocol routing (multi-frame)
 */
#define PDUR_TP_ROUTING_SUPPORT                 STD_ON

/**
 * @brief Gateway operation support
 * @details Enable PDU gateway functionality
 */
#define PDUR_GATEWAY_SUPPORT                    STD_ON

/**
 * @brief Multicast routing support
 * @details Enable routing to multiple destinations
 */
#define PDUR_MULTICAST_SUPPORT                  STD_ON

/**
 * @brief Maximum number of multicast destinations per PDU
 */
#define PDUR_CFG_MAX_MULTICAST_DESTS            4U

/******************************************************************************
 * Module Support Configuration
 ******************************************************************************/

/**
 * @brief COM module support
 * @details Enable COM as upper layer
 */
#define PDUR_COM_SUPPORT                        STD_ON

/**
 * @brief DCM module support
 * @details Enable DCM as upper layer
 */
#define PDUR_DCM_SUPPORT                        STD_ON

/**
 * @brief SoAd module support
 * @details Enable SoAd as lower layer
 */
#define PDUR_SOAD_SUPPORT                       STD_ON

/**
 * @brief CanIf module support
 * @details Enable CanIf as lower layer
 */
#define PDUR_CANIF_SUPPORT                      STD_ON

/**
 * @brief CanTp module support
 * @details Enable CanTp as lower layer
 */
#define PDUR_CANTP_SUPPORT                      STD_ON

/**
 * @brief LinIf module support
 * @details Enable LinIf as lower layer
 */
#define PDUR_LINIF_SUPPORT                      STD_OFF

/**
 * @brief LinTp module support
 * @details Enable LinTp as lower layer
 */
#define PDUR_LINTP_SUPPORT                      STD_OFF

/**
 * @brief FrIf module support
 * @details Enable FrIf as lower layer
 */
#define PDUR_FRIF_SUPPORT                       STD_OFF

/**
 * @brief FrTp module support
 * @details Enable FrTp as lower layer
 */
#define PDUR_FRTP_SUPPORT                       STD_OFF

/**
 * @brief J1939Tp module support
 * @details Enable J1939Tp as lower layer
 */
#define PDUR_J1939TP_SUPPORT                    STD_OFF

/**
 * @brief SecOC module support
 * @details Enable SecOC as upper/lower layer
 */
#define PDUR_SECOC_SUPPORT                      STD_OFF

/**
 * @brief DoIP module support
 * @details Enable DoIP as upper/lower layer
 */
#define PDUR_DOIP_SUPPORT                       STD_OFF

/**
 * @brief SD module support
 * @details Enable SD as upper/lower layer
 */
#define PDUR_SD_SUPPORT                         STD_OFF

/**
 * @brief CDD module support
 * @details Enable CDD as upper/lower layer
 */
#define PDUR_CDD_SUPPORT                        STD_OFF

/******************************************************************************
 * Buffer Configuration
 ******************************************************************************/

/**
 * @brief Buffer support enable
 * @details Enable internal buffering for gateway operations
 */
#define PDUR_BUFFER_SUPPORT                     STD_ON

/**
 * @brief Maximum number of buffers
 */
#define PDUR_CFG_MAX_BUFFERS                    16U

/**
 * @brief Maximum buffer size
 * @details Maximum size of a single buffer in bytes
 */
#define PDUR_CFG_MAX_BUFFER_SIZE                4096U

/**
 * @brief Total buffer pool size
 * @details Total size of all buffers combined
 */
#define PDUR_CFG_TOTAL_BUFFER_SIZE              32768U

/**
 * @brief FIFO buffer support
 * @details Enable FIFO queuing for PDUs
 */
#define PDUR_FIFO_BUFFER_SUPPORT                STD_ON

/**
 * @brief Maximum FIFO depth
 */
#define PDUR_CFG_MAX_FIFO_DEPTH                 8U

/******************************************************************************
 * TP Connection Configuration
 ******************************************************************************/

/**
 * @brief Maximum number of TP connections
 */
#define PDUR_CFG_MAX_TP_CONNECTIONS             16U

/**
 * @brief TP RX timeout in milliseconds
 */
#define PDUR_CFG_TP_RX_TIMEOUT_MS               1000U

/**
 * @brief TP TX timeout in milliseconds
 */
#define PDUR_CFG_TP_TX_TIMEOUT_MS               1000U

/**
 * @brief TP buffer size
 */
#define PDUR_CFG_TP_BUFFER_SIZE                 4096U

/******************************************************************************
 * Main Function Configuration
 ******************************************************************************/

/**
 * @brief Main function period in milliseconds
 */
#define PDUR_CFG_MAIN_FUNCTION_PERIOD_MS        10U

/**
 * @brief Enable routing timeout check
 */
#define PDUR_CFG_ENABLE_ROUTING_TIMEOUT         STD_ON

/**
 * @brief Routing timeout in milliseconds
 */
#define PDUR_CFG_ROUTING_TIMEOUT_MS             500U

/******************************************************************************
 * Safety and Diagnostic Configuration
 ******************************************************************************/

/**
 * @brief Safety checks enable
 * @details Enable runtime safety checks
 */
#define PDUR_SAFETY_CHECKS_ENABLE               STD_ON

/**
 * @brief Maximum consecutive errors before path disable
 */
#define PDUR_CFG_MAX_CONSECUTIVE_ERRORS         10U

/**
 * @brief DEM reporting enable
 * @details Enable DEM error reporting
 */
#define PDUR_DEM_REPORTING_ENABLE               STD_OFF

/**
 * @brief Runtime error detection enable
 */
#define PDUR_RUNTIME_ERROR_DETECT               STD_ON

/******************************************************************************
 * Optimization Configuration
 ******************************************************************************/

/**
 * @brief Use lookup table for routing
 * @details If enabled, routing uses direct lookup table for O(1) access
 */
#define PDUR_USE_ROUTING_LOOKUP_TABLE           STD_ON

/**
 * @brief Use inline functions for critical paths
 */
#define PDUR_USE_INLINE_OPTIMIZATION            STD_ON

/**
 * @brief Enable prefetching for next routing entry
 */
#define PDUR_ENABLE_PREFETCH                    STD_OFF

/******************************************************************************
 * PDU IDs Configuration
 ******************************************************************************/

/* COM PDU IDs */
#define PDUR_COM_TX_PDU_ID_START                0x0000U
#define PDUR_COM_TX_PDU_ID_END                  0x001FU
#define PDUR_COM_RX_PDU_ID_START                0x0020U
#define PDUR_COM_RX_PDU_ID_END                  0x003FU

/* DCM PDU IDs */
#define PDUR_DCM_TX_PDU_ID_START                0x0040U
#define PDUR_DCM_TX_PDU_ID_END                  0x004FU
#define PDUR_DCM_RX_PDU_ID_START                0x0050U
#define PDUR_DCM_RX_PDU_ID_END                  0x005FU

/* SoAd PDU IDs */
#define PDUR_SOAD_TX_PDU_ID_START               0x0060U
#define PDUR_SOAD_TX_PDU_ID_END                 0x007FU
#define PDUR_SOAD_RX_PDU_ID_START               0x0080U
#define PDUR_SOAD_RX_PDU_ID_END                 0x009FU

/* CanIf PDU IDs */
#define PDUR_CANIF_TX_PDU_ID_START              0x00A0U
#define PDUR_CANIF_TX_PDU_ID_END                0x00BFU
#define PDUR_CANIF_RX_PDU_ID_START              0x00C0U
#define PDUR_CANIF_RX_PDU_ID_END                0x00DFU

/* CanTp PDU IDs */
#define PDUR_CANTP_TX_PDU_ID_START              0x00E0U
#define PDUR_CANTP_TX_PDU_ID_END                0x00EFU
#define PDUR_CANTP_RX_PDU_ID_START              0x00F0U
#define PDUR_CANTP_RX_PDU_ID_END                0x00FFU

/******************************************************************************
 * Standard Macros
 ******************************************************************************/
#ifndef STD_ON
#define STD_ON      1U
#endif

#ifndef STD_OFF
#define STD_OFF     0U
#endif

#ifndef NULL_PTR
#define NULL_PTR    ((void *)0)
#endif

/******************************************************************************
 * Validation Macros
 ******************************************************************************/
#if (PDUR_CFG_MAX_ROUTING_PATHS == 0U)
#error "PDUR_CFG_MAX_ROUTING_PATHS must be greater than 0"
#endif

#if (PDUR_CFG_MAX_PDUS == 0U)
#error "PDUR_CFG_MAX_PDUS must be greater than 0"
#endif

#if (PDUR_CFG_MAX_BUFFERS > PDUR_CFG_MAX_ROUTING_PATHS)
#error "PDUR_CFG_MAX_BUFFERS cannot exceed PDUR_CFG_MAX_ROUTING_PATHS"
#endif

#if (PDUR_CFG_MAX_MULTICAST_DESTS < 1U)
#error "PDUR_CFG_MAX_MULTICAST_DESTS must be at least 1"
#endif

/******************************************************************************
 * Configuration Validation Checks
 ******************************************************************************/
#if (PDUR_MULTICAST_SUPPORT == STD_ON) && (PDUR_CFG_MAX_MULTICAST_DESTS < 2U)
#warning "Multicast support enabled but PDUR_CFG_MAX_MULTICAST_DESTS < 2"
#endif

#if (PDUR_BUFFER_SUPPORT == STD_ON) && (PDUR_CFG_MAX_BUFFERS == 0U)
#error "Buffer support enabled but PDUR_CFG_MAX_BUFFERS is 0"
#endif

#if (PDUR_TP_ROUTING_SUPPORT == STD_ON) && (PDUR_CFG_MAX_TP_CONNECTIONS == 0U)
#error "TP routing support enabled but PDUR_CFG_MAX_TP_CONNECTIONS is 0"
#endif

#ifdef __cplusplus
}
#endif

#endif /* PDUR_CFG_H */
