/**=================================================================================================
 * @file Crypto_Cfg.h
 * @brief Hardware Crypto Driver configuration header
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AUTOSAR Standard: Crypto Driver Configuration
 * Layer: MCAL (Microcontroller Driver Layer)
 *==================================================================================================*/

#ifndef CRYPTO_CFG_H
#define CRYPTO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CRYPTO_CFG_VENDOR_ID                   (0x64U)
#define CRYPTO_CFG_MODULE_ID                   (0x78U)
#define CRYPTO_CFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define CRYPTO_CFG_AR_RELEASE_MINOR_VERSION    (0x07U)
#define CRYPTO_CFG_AR_RELEASE_REVISION_VERSION (0x00U)

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable
 */
#ifndef CRYPTO_DEV_ERROR_DETECT
#define CRYPTO_DEV_ERROR_DETECT                (STD_ON)
#endif

/**
 * @brief Version info API enable
 */
#ifndef CRYPTO_VERSION_INFO_API
#define CRYPTO_VERSION_INFO_API                (STD_ON)
#endif

/**
 * @brief Enable hardware acceleration
 */
#ifndef CRYPTO_HW_ACCELERATION_ENABLED
#define CRYPTO_HW_ACCELERATION_ENABLED         (STD_ON)
#endif

/**
 * @brief Enable AES hardware support
 */
#ifndef CRYPTO_AES_HW_SUPPORT
#define CRYPTO_AES_HW_SUPPORT                  (STD_ON)
#endif

/**
 * @brief Enable SHA-256 hardware support
 */
#ifndef CRYPTO_SHA256_HW_SUPPORT
#define CRYPTO_SHA256_HW_SUPPORT               (STD_ON)
#endif

/**
 * @brief Enable HMAC hardware support
 */
#ifndef CRYPTO_HMAC_HW_SUPPORT
#define CRYPTO_HMAC_HW_SUPPORT                 (STD_ON)
#endif

/**
 * @brief Enable RSA hardware support
 */
#ifndef CRYPTO_RSA_HW_SUPPORT
#define CRYPTO_RSA_HW_SUPPORT                  (STD_ON)
#endif

/**
 * @brief Enable ECC hardware support
 */
#ifndef CRYPTO_ECC_HW_SUPPORT
#define CRYPTO_ECC_HW_SUPPORT                  (STD_ON)
#endif

/**
 * @brief Enable True Random Number Generator (TRNG)
 */
#ifndef CRYPTO_TRNG_HW_SUPPORT
#define CRYPTO_TRNG_HW_SUPPORT                 (STD_ON)
#endif

/**
 * @brief Enable pseudo RNG (PRNG) support
 */
#ifndef CRYPTO_PRNG_SUPPORT
#define CRYPTO_PRNG_SUPPORT                    (STD_ON)
#endif

/**
 * @brief Asynchronous operation support
 */
#ifndef CRYPTO_ASYNC_OPERATION_SUPPORT
#define CRYPTO_ASYNC_OPERATION_SUPPORT         (STD_ON)
#endif

/**
 * @brief Job cancellation support
 */
#ifndef CRYPTO_JOB_CANCELATION_SUPPORT
#define CRYPTO_JOB_CANCELATION_SUPPORT         (STD_ON)
#endif

/**
 * @brief Key storage support
 */
#ifndef CRYPTO_KEY_STORAGE_SUPPORT
#define CRYPTO_KEY_STORAGE_SUPPORT             (STD_ON)
#endif

/**
 * @brief Enable secure key storage (HSM/TEE)
 */
#ifndef CRYPTO_SECURE_KEY_STORAGE
#define CRYPTO_SECURE_KEY_STORAGE              (STD_OFF)
#endif

/*==================================================================================================
 *                                    INSTANCE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Number of Crypto driver instances
 */
#define CRYPTO_NUM_INSTANCES                   (1U)

/**
 * @brief Number of driver objects
 */
#define CRYPTO_NUM_DRIVER_OBJECTS              (4U)

/**
 * @brief Number of channels
 */
#define CRYPTO_NUM_CHANNELS                    (8U)

/**
 * @brief Number of keys
 */
#define CRYPTO_NUM_KEYS                        (16U)

/**
 * @brief Maximum number of jobs in queue
 */
#define CRYPTO_MAX_JOB_QUEUE_SIZE              (16U)

/**
 * @brief Number of key elements per key
 */
#define CRYPTO_NUM_KEY_ELEMENTS                (5U)

/*==================================================================================================
 *                                    DRIVER OBJECT IDs
 *==================================================================================================*/
#define CRYPTO_DRIVER_OBJECT_AES_ID            (0U)
#define CRYPTO_DRIVER_OBJECT_HASH_ID           (1U)
#define CRYPTO_DRIVER_OBJECT_HMAC_ID           (2U)
#define CRYPTO_DRIVER_OBJECT_RSA_ID            (3U)

/*==================================================================================================
 *                                    CHANNEL IDs
 *==================================================================================================*/
#define CRYPTO_CHANNEL_AES_0                   (0U)
#define CRYPTO_CHANNEL_AES_1                   (1U)
#define CRYPTO_CHANNEL_HASH_0                  (2U)
#define CRYPTO_CHANNEL_HMAC_0                  (3U)
#define CRYPTO_CHANNEL_RSA_0                   (4U)
#define CRYPTO_CHANNEL_RNG_0                   (5U)
#define CRYPTO_CHANNEL_ECC_0                   (6U)
#define CRYPTO_CHANNEL_GCM_0                   (7U)

/*==================================================================================================
 *                                    KEY IDs
 *==================================================================================================*/
#define CRYPTO_KEY_ID_AES_MASTER               (0U)
#define CRYPTO_KEY_ID_AES_SESSION              (1U)
#define CRYPTO_KEY_ID_HMAC_MASTER              (2U)
#define CRYPTO_KEY_ID_RSA_PRIVATE              (3U)
#define CRYPTO_KEY_ID_RSA_PUBLIC               (4U)
#define CRYPTO_KEY_ID_ECC_PRIVATE              (5U)
#define CRYPTO_KEY_ID_ECC_PUBLIC               (6U)
#define CRYPTO_KEY_ID_RNG_SEED                 (7U)
#define CRYPTO_KEY_ID_AES_STORAGE              (8U)
#define CRYPTO_KEY_ID_HMAC_STORAGE             (9U)
#define CRYPTO_KEY_ID_DERIVE_BASE              (10U)
#define CRYPTO_KEY_ID_DERIVED_1                (11U)
#define CRYPTO_KEY_ID_DERIVED_2                (12U)
#define CRYPTO_KEY_ID_CERT_ROOT                (13U)
#define CRYPTO_KEY_ID_CERT_DEVICE              (14U)
#define CRYPTO_KEY_ID_RESERVED                 (15U)

/*==================================================================================================
 *                                    KEY ELEMENT IDs
 *==================================================================================================*/
#define CRYPTO_KEY_ELEMENT_AES_KEY             (1U)
#define CRYPTO_KEY_ELEMENT_AES_IV              (2U)
#define CRYPTO_KEY_ELEMENT_HMAC_KEY            (3U)
#define CRYPTO_KEY_ELEMENT_RSA_MOD_N           (4U)
#define CRYPTO_KEY_ELEMENT_RSA_PUB_EXP_E       (5U)
#define CRYPTO_KEY_ELEMENT_RSA_PRIV_EXP_D      (6U)
#define CRYPTO_KEY_ELEMENT_ECC_PUB_X           (7U)
#define CRYPTO_KEY_ELEMENT_ECC_PUB_Y           (8U)
#define CRYPTO_KEY_ELEMENT_ECC_PRIV_D          (9U)
#define CRYPTO_KEY_ELEMENT_GCM_TAG             (10U)
#define CRYPTO_KEY_ELEMENT_SALT                (11U)
#define CRYPTO_KEY_ELEMENT_SEED                (12U)

/*==================================================================================================
 *                                    ALGORITHM CONFIGURATION
 *==================================================================================================*/

/**
 * @brief AES configuration
 */
#define CRYPTO_AES_MAX_KEY_SIZE                (256U)      /* Maximum AES key size in bits */
#define CRYPTO_AES_SUPPORT_ECB                 (STD_ON)
#define CRYPTO_AES_SUPPORT_CBC                 (STD_ON)
#define CRYPTO_AES_SUPPORT_CTR                 (STD_ON)
#define CRYPTO_AES_SUPPORT_GCM                 (STD_ON)
#define CRYPTO_AES_SUPPORT_CCM                 (STD_OFF)

/**
 * @brief SHA configuration
 */
#define CRYPTO_SHA_SUPPORT_SHA1                (STD_OFF)
#define CRYPTO_SHA_SUPPORT_SHA224              (STD_OFF)
#define CRYPTO_SHA_SUPPORT_SHA256              (STD_ON)
#define CRYPTO_SHA_SUPPORT_SHA384              (STD_OFF)
#define CRYPTO_SHA_SUPPORT_SHA512              (STD_OFF)

/**
 * @brief RSA configuration
 */
#define CRYPTO_RSA_MAX_KEY_SIZE                (2048U)     /* Maximum RSA key size in bits */
#define CRYPTO_RSA_SUPPORT_PKCS1_V15           (STD_ON)
#define CRYPTO_RSA_SUPPORT_PKCS1_V21           (STD_OFF)
#define CRYPTO_RSA_SUPPORT_PSS                 (STD_ON)

/**
 * @brief ECC configuration
 */
#define CRYPTO_ECC_SUPPORT_P256                (STD_ON)
#define CRYPTO_ECC_SUPPORT_P384                (STD_OFF)
#define CRYPTO_ECC_SUPPORT_P521                (STD_OFF)

/**
 * @brief TRNG configuration
 */
#define CRYPTO_TRNG_ENTROPY_BITS               (256U)
#define CRYPTO_TRNG_POLLING_TIMEOUT_US         (10000U)

/*==================================================================================================
 *                                    TIMING CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Main function period (in ms)
 */
#define CRYPTO_MAIN_FUNCTION_PERIOD_MS         (10U)

/**
 * @brief Hardware timeout (in us)
 */
#define CRYPTO_HW_TIMEOUT_US                   (1000000U)  /* 1 second */

/**
 * @brief Async job timeout (in ms)
 */
#define CRYPTO_ASYNC_JOB_TIMEOUT_MS            (5000U)

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Notification function prototype
 */
typedef void (*Crypto_NotificationFuncType)(uint32 jobId, Std_ReturnType result);

/**
 * @brief Enable notification callbacks
 */
#ifndef CRYPTO_NOTIFICATIONS_ENABLED
#define CRYPTO_NOTIFICATIONS_ENABLED           (STD_ON)
#endif

/*==================================================================================================
 *                                    EXTERNAL DECLARATIONS
 *==================================================================================================*/
#define CRYPTO_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Crypto_MemMap.h"

/**
 * @brief Crypto driver configuration
 */
extern const Crypto_ConfigType Crypto_Config;

#define CRYPTO_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Crypto_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_CFG_H */
