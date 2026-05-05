/*==================================================================================================
 *                              CRYPTO INTERFACE (CryIf)
 *==================================================================================================
 * FILENAME: CryIf_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header file for Crypto Interface module
 *==================================================================================================
 */

#ifndef CRYIF_CFG_H
#define CRYIF_CFG_H

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CRYIF_CFG_AR_RELEASE_MAJOR_VERSION    (4u)
#define CRYIF_CFG_AR_RELEASE_MINOR_VERSION    (7u)
#define CRYIF_CFG_AR_RELEASE_REVISION_VERSION (0u)

#define CRYIF_CFG_SW_MAJOR_VERSION            (1u)
#define CRYIF_CFG_SW_MINOR_VERSION            (0u)
#define CRYIF_CFG_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#define CRYIF_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable/disable
 */
#define CRYIF_VERSION_INFO_API                (STD_ON)

/**
 * @brief Main function period in milliseconds
 */
#define CRYIF_MAIN_FUNCTION_PERIOD_MS         (10u)

/*==================================================================================================
 *                                    CHANNEL CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Maximum number of channels
 */
#define CRYIF_MAX_CHANNEL_COUNT               (16u)

/**
 * @brief Number of configured channels
 */
#define CRYIF_NUM_CHANNELS                    (8u)

/* Channel IDs */
#define CRYIF_CHANNEL_ID_AES_ENCRYPT          (0u)
#define CRYIF_CHANNEL_ID_AES_DECRYPT          (1u)
#define CRYIF_CHANNEL_ID_MAC_GENERATE         (2u)
#define CRYIF_CHANNEL_ID_MAC_VERIFY           (3u)
#define CRYIF_CHANNEL_ID_HASH_SHA256          (4u)
#define CRYIF_CHANNEL_ID_HASH_SHA512          (5u)
#define CRYIF_CHANNEL_ID_RANDOM               (6u)
#define CRYIF_CHANNEL_ID_SIGNATURE            (7u)

/* Channel priorities (0 = highest) */
#define CRYIF_CHANNEL_PRIORITY_HIGH           (0u)
#define CRYIF_CHANNEL_PRIORITY_NORMAL         (5u)
#define CRYIF_CHANNEL_PRIORITY_LOW            (10u)

/*==================================================================================================
 *                                    KEY CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Maximum number of keys
 */
#define CRYIF_MAX_KEY_COUNT                   (16u)

/**
 * @brief Number of configured keys
 */
#define CRYIF_NUM_KEYS                        (8u)

/* Key IDs */
#define CRYIF_KEY_ID_AES_128                  (0u)
#define CRYIF_KEY_ID_AES_256                  (1u)
#define CRYIF_KEY_ID_HMAC_SHA256              (2u)
#define CRYIF_KEY_ID_RSA_PUBLIC               (3u)
#define CRYIF_KEY_ID_RSA_PRIVATE              (4u)
#define CRYIF_KEY_ID_ECC_PUBLIC               (5u)
#define CRYIF_KEY_ID_ECC_PRIVATE              (6u)
#define CRYIF_KEY_ID_RANDOM_SEED              (7u)

/* Key element IDs */
#define CRYIF_KEY_ELEMENT_ID_KEY              (1u)
#define CRYIF_KEY_ELEMENT_ID_IV               (2u)
#define CRYIF_KEY_ELEMENT_ID_SEED             (3u)
#define CRYIF_KEY_ELEMENT_ID_SALT             (4u)
#define CRYIF_KEY_ELEMENT_ID_ITERATIONS       (5u)

/* Key lengths in bytes */
#define CRYIF_KEY_LENGTH_AES_128              (16u)       /* 128 bits */
#define CRYIF_KEY_LENGTH_AES_192              (24u)       /* 192 bits */
#define CRYIF_KEY_LENGTH_AES_256              (32u)       /* 256 bits */
#define CRYIF_KEY_LENGTH_HMAC_SHA256          (32u)       /* 256 bits */
#define CRYIF_KEY_LENGTH_RSA_1024             (128u)      /* 1024 bits */
#define CRYIF_KEY_LENGTH_RSA_2048             (256u)      /* 2048 bits */
#define CRYIF_KEY_LENGTH_ECC_P256             (32u)       /* 256 bits */
#define CRYIF_KEY_LENGTH_MAX                  (256u)      /* Maximum key length */

/*==================================================================================================
 *                                    ALGORITHM CONFIGURATION
 *==================================================================================================*/

/* AES Configuration */
#define CRYIF_AES_BLOCK_SIZE                  (16u)       /* 128 bits */
#define CRYIF_AES_IV_SIZE                     (16u)       /* 128 bits */
#define CRYIF_AES_GCM_TAG_SIZE                (16u)       /* 128 bits */
#define CRYIF_AES_CCM_TAG_SIZE                (16u)       /* 128 bits */

/* Hash Configuration */
#define CRYIF_HASH_SHA256_SIZE                (32u)       /* 256 bits */
#define CRYIF_HASH_SHA512_SIZE                (64u)       /* 512 bits */
#define CRYIF_HASH_SHA1_SIZE                  (20u)       /* 160 bits */
#define CRYIF_HASH_SHA224_SIZE                (28u)       /* 224 bits */
#define CRYIF_HASH_SHA384_SIZE                (48u)       /* 384 bits */

/* MAC Configuration */
#define CRYIF_HMAC_SHA256_SIZE                (32u)       /* 256 bits */
#define CRYIF_CMAC_SIZE                       (16u)       /* 128 bits */

/* Random Configuration */
#define CRYIF_RANDOM_MAX_SIZE                 (256u)      /* Maximum random bytes per request */
#define CRYIF_RANDOM_SEED_SIZE                (32u)       /* Seed size */

/* RSA Configuration */
#define CRYIF_RSA_SIGNATURE_SIZE_1024         (128u)      /* 1024 bits */
#define CRYIF_RSA_SIGNATURE_SIZE_2048         (256u)      /* 2048 bits */

/*==================================================================================================
 *                                    FEATURE ENABLE/DISABLE
 *==================================================================================================*/

/**
 * @brief Enable AES encryption support
 */
#define CRYIF_AES_SUPPORT                     (STD_ON)

/**
 * @brief Enable SHA-256 hash support
 */
#define CRYIF_SHA256_SUPPORT                  (STD_ON)

/**
 * @brief Enable HMAC support
 */
#define CRYIF_HMAC_SUPPORT                    (STD_ON)

/**
 * @brief Enable RSA support
 */
#define CRYIF_RSA_SUPPORT                     (STD_ON)

/**
 * @brief Enable random number generation
 */
#define CRYIF_RANDOM_GENERATE_SUPPORT         (STD_ON)

/**
 * @brief Enable key derivation
 */
#define CRYIF_KEY_DERIVE_SUPPORT              (STD_ON)

/**
 * @brief Enable key exchange
 */
#define CRYIF_KEY_EXCHANGE_SUPPORT            (STD_ON)

/**
 * @brief Enable certificate operations
 */
#define CRYIF_CERTIFICATE_SUPPORT             (STD_OFF)

/**
 * @brief Enable streaming operations
 */
#define CRYIF_STREAMING_SUPPORT               (STD_ON)

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Enable callback notifications
 */
#define CRYIF_CALLBACK_SUPPORTED              (STD_ON)

/**
 * @brief Enable asynchronous operations
 */
#define CRYIF_ASYNC_OPERATIONS                (STD_OFF)

#endif /* CRYIF_CFG_H */
