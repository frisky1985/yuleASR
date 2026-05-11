/**********************************************************************************************************************
 * @file       Crypto_Cfg.h
 * @brief      Crypto Driver Configuration Header
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef CRYPTO_CFG_H
#define CRYPTO_CFG_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/

/* Version Information */
#define CRYPTO_CFG_MAJOR_VERSION            (1U)
#define CRYPTO_CFG_MINOR_VERSION            (0U)
#define CRYPTO_CFG_PATCH_VERSION            (0U)

/* General Configuration */
#define CRYPTO_CFG_VERSION_INFO_API         (STD_ON)
#define CRYPTO_CFG_DEV_ERROR_DETECT         (STD_ON)
#define CRYPTO_CFG_RUNTIME_ERROR_DETECT     (STD_OFF)

/* HSM Support */
#define CRYPTO_CFG_HSM_ENABLED              (STD_ON)
#define CRYPTO_CFG_HSM_FALLBACK_TO_SW       (STD_ON)

/* Driver Capabilities */
#define CRYPTO_CFG_MAX_CHANNELS             (4U)
#define CRYPTO_CFG_QUEUE_SIZE               (8U)
#define CRYPTO_CFG_MAX_KEYS                 (16U)
#define CRYPTO_CFG_MAX_KEY_ELEMENTS         (64U)

/* Buffer Sizes */
#define CRYPTO_CFG_MAX_KEY_SIZE             (128U)
#define CRYPTO_CFG_MAX_IV_SIZE              (16U)
#define CRYPTO_CFG_MAX_TAG_SIZE             (16U)
#define CRYPTO_CFG_MAX_AAD_SIZE             (256U)
#define CRYPTO_CFG_MAX_HASH_SIZE            (64U)
#define CRYPTO_CFG_MAX_SIGNATURE_SIZE       (72U)
#define CRYPTO_CFG_MAX_ECC_KEY_SIZE         (96U)    /* 3*32 for secp256r1 uncompressed */
#define CRYPTO_CFG_MAX_SYMMETRIC_KEY_SIZE   (32U)    /* AES-256 */

/* CCC Digital Key Specific */
#define CRYPTO_CFG_CCC_KEY_SIZE             (16U)    /* AES-128 for CCC */
#define CRYPTO_CFG_CCC_IV_SIZE              (12U)    /* 96-bit IV for GCM */
#define CRYPTO_CFG_CCC_TAG_SIZE             (16U)    /* 128-bit authentication tag */
#define CRYPTO_CFG_CCC_ECDSA_SIG_SIZE       (64U)    /* R+S for secp256r1 */
#define CRYPTO_CFG_CCC_ECC_PUB_KEY_SIZE     (65U)    /* Uncompressed P-256 */
#define CRYPTO_CFG_CCC_ECC_PRIV_KEY_SIZE    (32U)    /* P-256 private key */

/**********************************************************************************************************************
 * HSM CONFIGURATION
 *********************************************************************************************************************/

/* HSM Hardware Configuration */
#define CRYPTO_HSM_INSTANCE_ID              (0U)
#define CRYPTO_HSM_CHANNEL_ID               (0U)
#define CRYPTO_HSM_CMD_TIMEOUT_MS           (1000U)
#define CRYPTO_HSM_RSP_TIMEOUT_MS           (5000U)
#define CRYPTO_HSM_MAX_CONCURRENT_JOBS      (4U)

/* HSM Supported Algorithms */
#define CRYPTO_HSM_SUPPORT_ECDSA            (STD_ON)
#define CRYPTO_HSM_SUPPORT_ECDH             (STD_ON)
#define CRYPTO_HSM_SUPPORT_AES_GCM          (STD_ON)
#define CRYPTO_HSM_SUPPORT_SHA256           (STD_ON)
#define CRYPTO_HSM_SUPPORT_HKDF             (STD_ON)
#define CRYPTO_HSM_SUPPORT_HMAC             (STD_ON)
#define CRYPTO_HSM_SUPPORT_RANDOM           (STD_ON)

/* HSM Security Levels */
#define CRYPTO_HSM_SECURITY_LEVEL_1         (0x01U)  /* Software fallback allowed */
#define CRYPTO_HSM_SECURITY_LEVEL_2         (0x02U)  /* HSM preferred */
#define CRYPTO_HSM_SECURITY_LEVEL_3         (0x03U)  /* HSM required */

/**********************************************************************************************************************
 * KEY CONFIGURATION
 *********************************************************************************************************************/

/* Key IDs for CCC Digital Key */
#define CRYPTO_KEY_ID_MASTER                (0U)
#define CRYPTO_KEY_ID_SIGNING               (1U)
#define CRYPTO_KEY_ID_ENCRYPTION            (2U)
#define CRYPTO_KEY_ID_DEVICE                (3U)
#define CRYPTO_KEY_ID_SESSION               (4U)
#define CRYPTO_KEY_ID_EPHEMERAL             (5U)
#define CRYPTO_KEY_ID_CCC_ROOT              (10U)
#define CRYPTO_KEY_ID_CCC_SUB_CA            (11U)
#define CRYPTO_KEY_ID_CCC_DEVICE_KEY        (12U)
#define CRYPTO_KEY_ID_CCC_AUTH_KEY          (13U)
#define CRYPTO_KEY_ID_CCC_FRIEND_KEY        (14U)

/* Key Element IDs */
#define CRYPTO_KEY_ELEMENT_ID_KEY_MATERIAL  (1U)
#define CRYPTO_KEY_ELEMENT_ID_IV            (2U)
#define CRYPTO_KEY_ELEMENT_ID_SALT          (3U)
#define CRYPTO_KEY_ELEMENT_ID_TAG           (4U)
#define CRYPTO_KEY_ELEMENT_ID_AAD           (5U)
#define CRYPTO_KEY_ELEMENT_ID_PUBLIC_KEY    (10U)
#define CRYPTO_KEY_ELEMENT_ID_PRIVATE_KEY   (11U)
#define CRYPTO_KEY_ELEMENT_ID_SIGNATURE     (20U)

/**********************************************************************************************************************
 * ALGORITHM CONFIGURATION
 **********************************************************************************************************************/

/* ECDSA secp256r1 */
#define CRYPTO_ALG_ECDSA_SECP256R1 \
    { \
        .family = CRYPTO_ALGOFAM_ECDSA, \
        .mode = 0, \
        .keyLength = 256, \
        .curve = CRYPTO_ECC_CURVE_SECP256R1 \
    }

/* ECDH secp256r1 */
#define CRYPTO_ALG_ECDH_SECP256R1 \
    { \
        .family = CRYPTO_ALGOFAM_ECDH, \
        .mode = 0, \
        .keyLength = 256, \
        .curve = CRYPTO_ECC_CURVE_SECP256R1 \
    }

/* AES-128-GCM */
#define CRYPTO_ALG_AES128_GCM \
    { \
        .family = CRYPTO_ALGOFAM_AES, \
        .mode = CRYPTO_ALGOMODE_GCM, \
        .keyLength = 128, \
        .curve = 0 \
    }

/* AES-256-GCM */
#define CRYPTO_ALG_AES256_GCM \
    { \
        .family = CRYPTO_ALGOFAM_AES, \
        .mode = CRYPTO_ALGOMODE_GCM, \
        .keyLength = 256, \
        .curve = 0 \
    }

/* SHA-256 */
#define CRYPTO_ALG_SHA256 \
    { \
        .family = CRYPTO_ALGOFAM_SHA2_256, \
        .mode = 0, \
        .keyLength = 0, \
        .curve = 0 \
    }

/* HMAC-SHA256 */
#define CRYPTO_ALG_HMAC_SHA256 \
    { \
        .family = CRYPTO_ALGOFAM_HMAC, \
        .mode = CRYPTO_ALGOFAM_SHA2_256, \
        .keyLength = 256, \
        .curve = 0 \
    }

/* HKDF-SHA256 */
#define CRYPTO_ALG_HKDF_SHA256 \
    { \
        .family = CRYPTO_ALGOFAM_HKDF, \
        .mode = CRYPTO_ALGOFAM_SHA2_256, \
        .keyLength = 256, \
        .curve = 0 \
    }

/**********************************************************************************************************************
 * CALLBACK CONFIGURATION
 *********************************************************************************************************************/

#define CRYPTO_CFG_CALLBACK_ON_COMPLETE     (STD_ON)
#define CRYPTO_CFG_CALLBACK_ON_PROGRESS     (STD_OFF)

/**********************************************************************************************************************
 * DEBUG CONFIGURATION
 *********************************************************************************************************************/

#define CRYPTO_CFG_DEBUG_LEVEL              (0U)   /* 0=Off, 1=Error, 2=Warning, 3=Info, 4=Debug */

/* External Configuration Structure */
extern const Crypto_ConfigType Crypto_Config;

#endif /* CRYPTO_CFG_H */
