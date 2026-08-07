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

/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: SecOC_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header file for Secure Onboard Communication module
 *==================================================================================================
 */

#ifndef SECOC_CFG_H
#define SECOC_CFG_H

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define SECOC_CFG_AR_RELEASE_MAJOR_VERSION    (4u)
#define SECOC_CFG_AR_RELEASE_MINOR_VERSION    (7u)
#define SECOC_CFG_AR_RELEASE_REVISION_VERSION (0u)

#define SECOC_CFG_SW_MAJOR_VERSION            (1u)
#define SECOC_CFG_SW_MINOR_VERSION            (0u)
#define SECOC_CFG_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#define SECOC_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable/disable
 */
#define SECOC_VERSION_INFO_API                (STD_ON)

/**
 * @brief Main function RX period in milliseconds
 */
#define SECOC_MAIN_FUNCTION_PERIOD_RX_MS      (10u)

/**
 * @brief Main function TX period in milliseconds
 */
#define SECOC_MAIN_FUNCTION_PERIOD_TX_MS      (10u)

/*==================================================================================================
 *                                    PDU CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Number of transmit PDUs
 */
#define SECOC_NUM_TX_PDUS                     (4u)

/**
 * @brief Number of receive PDUs
 */
#define SECOC_NUM_RX_PDUS                     (4u)

/**
 * @brief Maximum PDU length
 */
#define SECOC_MAX_PDU_LENGTH                  256U

/*==================================================================================================
 *                                    TX PDU IDs
 *==================================================================================================*/
#define SECOC_TX_PDU_ID_0                     (0u)
#define SECOC_TX_PDU_ID_1                     (1u)
#define SECOC_TX_PDU_ID_2                     (2u)
#define SECOC_TX_PDU_ID_3                     (3u)

/*==================================================================================================
 *                                    RX PDU IDs
 *==================================================================================================*/
#define SECOC_RX_PDU_ID_0                     (0u)
#define SECOC_RX_PDU_ID_1                     (1u)
#define SECOC_RX_PDU_ID_2                     (2u)
#define SECOC_RX_PDU_ID_3                     (3u)

/*==================================================================================================
 *                                    AUTHENTICATION CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Authentication algorithm
 */
#define SECOC_AUTH_ALGORITHM                  SECOC_HMAC_SHA256

/**
 * @brief Authentication info length in bytes
 */
#define SECOC_AUTH_INFO_LENGTH                (16u)

/**
 * @brief Freshness value type
 */
#define SECOC_FRESHNESS_VALUE_TYPE            SECOC_COUNTER

/**
 * @brief Freshness value length in bits
 */
#define SECOC_FRESHNESS_VALUE_LENGTH          (32u)

/**
 * @brief Freshness value transmitted length in bits
 */
#define SECOC_FRESHNESS_VALUE_TX_LENGTH       (16u)

/**
 * @brief Data ID length in bytes
 */
#define SECOC_DATA_ID_LENGTH                  (1u)

/*==================================================================================================
 *                                    FRESHNESS VALUE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Maximum freshness value
 */
#define SECOC_MAX_FRESHNESS_VALUE             (0xFFFFFFFFu)

/**
 * @brief Freshness value reset threshold
 */
#define SECOC_FRESHNESS_RESET_THRESHOLD       (0xF0000000u)

/**
 * @brief Enable freshness value sync
 */
#define SECOC_ENABLE_FRESHNESS_SYNC           (STD_ON)

/*==================================================================================================
 *                                    VERIFICATION CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Allow status override
 */
#define SECOC_OVERRIDE_STATUS_ALLOWED         (STD_ON)

/**
 * @brief Default verification status
 */
#define SECOC_DEFAULT_VERIFICATION_STATUS     SECOC_UNVERIFIED

/**
 * @brief Verification retry attempts
 */
#define SECOC_VERIFICATION_RETRY_COUNT        (3u)

/**
 * @brief Verification timeout in milliseconds
 */
#define SECOC_VERIFICATION_TIMEOUT_MS         (100u)

/*==================================================================================================
 *                                    CRYPTO CONFIGURATION
 *==================================================================================================*/

/**
 * @brief CSM job ID for authentication
 */
#define SECOC_CSM_JOB_ID_AUTH                 (CSM_JOB_ID_MAC_GENERATE_1)

/**
 * @brief CSM job ID for verification
 */
#define SECOC_CSM_JOB_ID_VERIFY               (CSM_JOB_ID_MAC_VERIFY_1)

/**
 * @brief Maximum crypto operations per main function
 */
#define SECOC_MAX_CRYPTO_OPERATIONS           (4u)

#endif /* SECOC_CFG_H */

/*==================================================================================================
 *                                    AUTH / FRESHNESS LENGTHS
 *==================================================================================================*/
#ifndef SECOC_AUTH_LENGTH_4
#define SECOC_AUTH_LENGTH_4                   (4u)
#endif
#ifndef SECOC_AUTH_LENGTH_8
#define SECOC_AUTH_LENGTH_8                   (8u)
#endif
#ifndef SECOC_FRESHNESS_LENGTH_3
#define SECOC_FRESHNESS_LENGTH_3              (3u)
#endif
#ifndef SECOC_FRESHNESS_LENGTH_4
#define SECOC_FRESHNESS_LENGTH_4              (4u)
#endif
#ifndef SECOC_MAX_PDUS
#define SECOC_MAX_PDUS                        (SECOC_NUM_TX_PDUS)
#endif
