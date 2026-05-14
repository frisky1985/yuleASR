/*==================================================================================================
 *                                KEY MANAGER (KeyM) - CONFIGURATION
 *==================================================================================================
 * FILENAME: KeyM_Cfg.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_KeyManager.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header file for Key Manager module
 *==================================================================================================
 */

#ifndef KEYM_CFG_H
#define KEYM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define KEYM_CFG_VENDOR_ID                   (100u)
#define KEYM_CFG_AR_RELEASE_MAJOR_VERSION    (4u)
#define KEYM_CFG_AR_RELEASE_MINOR_VERSION    (7u)
#define KEYM_CFG_AR_RELEASE_REVISION_VERSION (0u)

#define KEYM_CFG_SW_MAJOR_VERSION            (1u)
#define KEYM_CFG_SW_MINOR_VERSION            (0u)
#define KEYM_CFG_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    CONFIGURATION SWITCHES
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#ifndef KEYM_DEV_ERROR_DETECT
#define KEYM_DEV_ERROR_DETECT               (STD_ON)
#endif

/**
 * @brief Version information API enable/disable
 */
#ifndef KEYM_VERSION_INFO_API
#define KEYM_VERSION_INFO_API               (STD_ON)
#endif

/**
 * @brief Asynchronous operations support
 */
#ifndef KEYM_ASYNC_OPERATIONS
#define KEYM_ASYNC_OPERATIONS               (STD_ON)
#endif

/**
 * @brief NvM storage support for persistent keys
 */
#ifndef KEYM_NVM_STORAGE
#define KEYM_NVM_STORAGE                    (STD_ON)
#endif

/**
 * @brief Certificate management support
 */
#ifndef KEYM_CERTIFICATE_MANAGEMENT
#define KEYM_CERTIFICATE_MANAGEMENT         (STD_ON)
#endif

/**
 * @brief Key derivation support
 */
#ifndef KEYM_KEY_DERIVATION
#define KEYM_KEY_DERIVATION                 (STD_OFF)
#endif

/**
 * @brief Key agreement support
 */
#ifndef KEYM_KEY_AGREEMENT
#define KEYM_KEY_AGREEMENT                  (STD_OFF)
#endif

/*==================================================================================================
 *                                    CONFIGURATION PARAMETERS
 *==================================================================================================*/

/**
 * @brief Maximum number of keys managed by KeyM
 */
#ifndef KEYM_NUM_KEYS
#define KEYM_NUM_KEYS                       (8u)
#endif

/**
 * @brief Maximum number of key elements per key
 */
#ifndef KEYM_MAX_KEY_ELEMENTS
#define KEYM_MAX_KEY_ELEMENTS               (8u)
#endif

/**
 * @brief Maximum key length in bytes
 */
#ifndef KEYM_MAX_KEY_LENGTH
#define KEYM_MAX_KEY_LENGTH                 (256u)
#endif

/**
 * @brief Maximum number of certificates
 */
#ifndef KEYM_NUM_CERTIFICATES
#define KEYM_NUM_CERTIFICATES               (4u)
#endif

/**
 * @brief Maximum certificate size in bytes
 */
#ifndef KEYM_MAX_CERT_SIZE
#define KEYM_MAX_CERT_SIZE                  (2048u)
#endif

/**
 * @brief Operation queue size for async operations
 */
#ifndef KEYM_OPERATION_QUEUE_SIZE
#define KEYM_OPERATION_QUEUE_SIZE           (4u)
#endif

/**
 * @brief Default key validity period in seconds (0 = no expiry)
 */
#ifndef KEYM_DEFAULT_KEY_VALIDITY
#define KEYM_DEFAULT_KEY_VALIDITY           (0u)
#endif

/**
 * @brief Maximum key lifetime in seconds (0 = unlimited)
 */
#ifndef KEYM_MAX_KEY_LIFETIME
#define KEYM_MAX_KEY_LIFETIME               (31536000u)  /* 1 year */
#endif

/*==================================================================================================
 *                                    KEY IDENTIFIERS
 *==================================================================================================*/

/* Pre-defined key IDs */
#define KEYM_KEY_ID_MASTER                  (0u)
#define KEYM_KEY_ID_AES_128                 (1u)
#define KEYM_KEY_ID_AES_256                 (2u)
#define KEYM_KEY_ID_HMAC_SHA256             (3u)
#define KEYM_KEY_ID_RSA_2048                (4u)
#define KEYM_KEY_ID_ECC_P256                (5u)
#define KEYM_KEY_ID_SESSION                 (6u)
#define KEYM_KEY_ID_RESERVED                (7u)

/*==================================================================================================
 *                                    CSM JOB IDENTIFIERS
 *==================================================================================================*/

/* CSM Job IDs for crypto operations */
#define KEYM_CSM_JOB_ID_ENCRYPT             (0x100u)
#define KEYM_CSM_JOB_ID_DECRYPT             (0x101u)
#define KEYM_CSM_JOB_ID_MAC_GENERATE        (0x200u)
#define KEYM_CSM_JOB_ID_MAC_VERIFY          (0x201u)
#define KEYM_CSM_JOB_ID_HASH                (0x300u)
#define KEYM_CSM_JOB_ID_RANDOM              (0x400u)
#define KEYM_CSM_JOB_ID_KEY_DERIVE          (0x500u)

/*==================================================================================================
 *                                    KEY CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Key configuration for Master key
 */
#define KEYM_CFG_KEY_MASTER_TYPE            (KEYM_KEY_TYPE_AES)
#define KEYM_CFG_KEY_MASTER_LENGTH          (32u)  /* 256 bits */
#define KEYM_CFG_KEY_MASTER_USAGE           (KEYM_KEY_USAGE_ENCRYPT | KEYM_KEY_USAGE_DECRYPT)

/**
 * @brief Key configuration for AES-128 key
 */
#define KEYM_CFG_KEY_AES_128_TYPE           (KEYM_KEY_TYPE_AES)
#define KEYM_CFG_KEY_AES_128_LENGTH         (16u)  /* 128 bits */
#define KEYM_CFG_KEY_AES_128_USAGE          (KEYM_KEY_USAGE_ENCRYPT | KEYM_KEY_USAGE_DECRYPT | \
                                             KEYM_KEY_USAGE_MAC_GENERATE | KEYM_KEY_USAGE_MAC_VERIFY)

/**
 * @brief Key configuration for AES-256 key
 */
#define KEYM_CFG_KEY_AES_256_TYPE           (KEYM_KEY_TYPE_AES)
#define KEYM_CFG_KEY_AES_256_LENGTH         (32u)  /* 256 bits */
#define KEYM_CFG_KEY_AES_256_USAGE          (KEYM_KEY_USAGE_ENCRYPT | KEYM_KEY_USAGE_DECRYPT | \
                                             KEYM_KEY_USAGE_MAC_GENERATE | KEYM_KEY_USAGE_MAC_VERIFY)

/**
 * @brief Key configuration for HMAC-SHA256 key
 */
#define KEYM_CFG_KEY_HMAC_TYPE              (KEYM_KEY_TYPE_HMAC)
#define KEYM_CFG_KEY_HMAC_LENGTH            (32u)  /* 256 bits */
#define KEYM_CFG_KEY_HMAC_USAGE             (KEYM_KEY_USAGE_MAC_GENERATE | KEYM_KEY_USAGE_MAC_VERIFY)

/**
 * @brief Key configuration for RSA-2048 key
 */
#define KEYM_CFG_KEY_RSA_TYPE               (KEYM_KEY_TYPE_RSA)
#define KEYM_CFG_KEY_RSA_LENGTH             (256u)  /* 2048 bits */
#define KEYM_CFG_KEY_RSA_USAGE              (KEYM_KEY_USAGE_SIGN | KEYM_KEY_USAGE_VERIFY)

/**
 * @brief Key configuration for ECC P-256 key
 */
#define KEYM_CFG_KEY_ECC_TYPE               (KEYM_KEY_TYPE_ECC)
#define KEYM_CFG_KEY_ECC_LENGTH             (32u)  /* 256 bits */
#define KEYM_CFG_KEY_ECC_USAGE              (KEYM_KEY_USAGE_SIGN | KEYM_KEY_USAGE_VERIFY)

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Enable notification callback
 */
#ifndef KEYM_NOTIFICATION_CALLBACK
#define KEYM_NOTIFICATION_CALLBACK          (STD_ON)
#endif

#ifdef __cplusplus
}
#endif

#endif /* KEYM_CFG_H */
