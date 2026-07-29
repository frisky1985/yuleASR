/**
 * @file IpduM_Cfg.h
 * @brief AUTOSAR I-PDU Multiplexer Configuration Header
 * @version 4.4.0
 * @date 2026-05-05
 */

#ifndef IPDUM_CFG_H
#define IPDUM_CFG_H

/*==================================================================================================
 *                               VERSION INFO
 *=================================================================================================*/

#define IPDUM_CFG_VENDOR_ID                 0x01U
#define IPDUM_CFG_MODULE_ID                 0x4DU

#define IPDUM_CFG_SW_MAJOR_VERSION          4U
#define IPDUM_CFG_SW_MINOR_VERSION          4U
#define IPDUM_CFG_SW_PATCH_VERSION          0U

/*==================================================================================================
 *                              CONFIGURATION SWITCHES
 *=================================================================================================*/

/**
 * @brief Development error detection enable
 */
#define IPDUM_DEV_ERROR_DETECT              STD_ON

/**
 * @brief Version info API enable
 */
#define IPDUM_VERSION_INFO_API              STD_ON

/**
 * @brief Main function period in ms
 */
#define IPDUM_MAIN_FUNCTION_PERIOD_MS       10U

/**
 * @brief Maximum number of multiplexed Tx PDUs
 */
#define IPDUM_MAX_TX_MUX_PDUS               8U

/**
 * @brief Maximum number of multiplexed Rx PDUs
 */
#define IPDUM_MAX_RX_MUX_PDUS               8U

/**
 * @brief Maximum number of dynamic parts per Tx PDU
 */
#define IPDUM_MAX_DYNAMIC_PARTS_PER_TX_PDU  4U

/**
 * @brief Maximum number of dynamic parts per Rx PDU
 */
#define IPDUM_MAX_DYNAMIC_PARTS_PER_RX_PDU  4U

/**
 * @brief Maximum number of static parts per Rx PDU
 */
#define IPDUM_MAX_STATIC_PARTS_PER_RX_PDU   2U

/**
 * @brief Maximum PDU length
 */
#define IPDUM_MAX_PDU_LENGTH                64U

/**
 * @brief Selector field position default
 */
#define IPDUM_SELECTOR_START_BYTE_DEFAULT   0U
#define IPDUM_SELECTOR_START_BIT_DEFAULT    0U
#define IPDUM_SELECTOR_BIT_LENGTH_DEFAULT   8U

/*==================================================================================================
 *                              PDU ID MAPPING
 *=================================================================================================*/

/* Tx Multiplexed PDU IDs */
#define IPDUM_TX_MUX_PDU_0                  0U
#define IPDUM_TX_MUX_PDU_1                  1U
#define IPDUM_TX_MUX_PDU_2                  2U
#define IPDUM_TX_MUX_PDU_3                  3U

/* Rx Multiplexed PDU IDs */
#define IPDUM_RX_MUX_PDU_0                  0U
#define IPDUM_RX_MUX_PDU_1                  1U
#define IPDUM_RX_MUX_PDU_2                  2U
#define IPDUM_RX_MUX_PDU_3                  3U

/* Lower layer PDU IDs (COM layer) */
#define IPDUM_COM_TX_PDU_0                  0U
#define IPDUM_COM_TX_PDU_1                  1U
#define IPDUM_COM_TX_PDU_2                  2U
#define IPDUM_COM_TX_PDU_3                  3U

#define IPDUM_COM_RX_PDU_0                  0U
#define IPDUM_COM_RX_PDU_1                  1U
#define IPDUM_COM_RX_PDU_2                  2U
#define IPDUM_COM_RX_PDU_3                  3U

/* Selector values for dynamic parts */
#define IPDUM_SELECTOR_VALUE_0              0x00U
#define IPDUM_SELECTOR_VALUE_1              0x01U
#define IPDUM_SELECTOR_VALUE_2              0x02U
#define IPDUM_SELECTOR_VALUE_3              0x03U

/*==================================================================================================
 *                              MEMORY MAPPING
 *=================================================================================================*/

#include "MemMap.h"

#endif /* IPDUM_CFG_H */
