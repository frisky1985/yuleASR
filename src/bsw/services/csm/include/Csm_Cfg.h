/*==================================================================================================
 *                                CRYPTO SERVICES MANAGER (Csm)
 *==================================================================================================
 * FILENAME: Csm_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header file for Crypto Services Manager module
 *==================================================================================================
 */

#ifndef CSM_CFG_H
#define CSM_CFG_H

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CSM_CFG_AR_RELEASE_MAJOR_VERSION    (4u)
#define CSM_CFG_AR_RELEASE_MINOR_VERSION    (7u)
#define CSM_CFG_AR_RELEASE_REVISION_VERSION (0u)

#define CSM_CFG_SW_MAJOR_VERSION            (1u)
#define CSM_CFG_SW_MINOR_VERSION            (0u)
#define CSM_CFG_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#define CSM_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable/disable
 */
#define CSM_VERSION_INFO_API                (STD_ON)

/**
 * @brief Main function period in milliseconds
 */
#define CSM_MAIN_FUNCTION_PERIOD_MS         (10u)

/*==================================================================================================
 *                                    QUEUE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Maximum number of jobs in queue
 */
#define CSM_JOB_QUEUE_SIZE                  (16u)

/**
 * @brief Maximum number of configured jobs
 */
#define CSM_NUM_JOBS                        (8u)

/**
 * @brief Maximum number of configured keys
 */
#define CSM_NUM_KEYS                        (8u)

/*==================================================================================================
 *                                    JOB CONFIGURATION
 *==================================================================================================*/

/* Job IDs */
#define CSM_JOB_ID_ENCRYPT_1                (0u)
#define CSM_JOB_ID_DECRYPT_1                (1u)
#define CSM_JOB_ID_MAC_GENERATE_1           (2u)
#define CSM_JOB_ID_MAC_VERIFY_1             (3u)
#define CSM_JOB_ID_HASH_SHA256              (4u)
#define CSM_JOB_ID_HASH_SHA512              (5u)
#define CSM_JOB_ID_RANDOM_GENERATE          (6u)
#define CSM_JOB_ID_SIGNATURE_VERIFY         (7u)

/* Job priorities (0 = highest) */
#define CSM_JOB_PRIORITY_HIGH               (0u)
#define CSM_JOB_PRIORITY_NORMAL             (5u)
#define CSM_JOB_PRIORITY_LOW                (10u)

/*==================================================================================================
 *                                    KEY CONFIGURATION
 *==================================================================================================*/

/* Key IDs */
#define CSM_KEY_ID_AES_128                  (0u)
#define CSM_KEY_ID_AES_256                  (1u)
#define CSM_KEY_ID_HMAC_SHA256              (2u)
#define CSM_KEY_ID_RSA_PUBLIC               (3u)
#define CSM_KEY_ID_RSA_PRIVATE              (4u)
#define CSM_KEY_ID_ECC_PUBLIC               (5u)
#define CSM_KEY_ID_ECC_PRIVATE              (6u)

/* Key lengths in bits */
#define CSM_KEY_LENGTH_AES_128              (128u)
#define CSM_KEY_LENGTH_AES_256              (256u)
#define CSM_KEY_LENGTH_HMAC_SHA256          (256u)
#define CSM_KEY_LENGTH_RSA_2048             (2048u)
#define CSM_KEY_LENGTH_ECC_P256             (256u)

/*==================================================================================================
 *                                    ALGORITHM CONFIGURATION
 *==================================================================================================*/

/* AES Configuration */
#define CSM_AES_BLOCK_SIZE                  (16u)       /* 128 bits */
#define CSM_AES_IV_SIZE                     (16u)       /* 128 bits */

/* Hash Configuration */
#define CSM_HASH_SHA256_SIZE                (32u)       /* 256 bits */
#define CSM_HASH_SHA512_SIZE                (64u)       /* 512 bits */
#define CSM_HASH_SHA1_SIZE                  (20u)       /* 160 bits */

/* MAC Configuration */
#define CSM_HMAC_SHA256_SIZE                (32u)       /* 256 bits */

/* Random Configuration */
#define CSM_RANDOM_MAX_SIZE                 (256u)      /* Maximum random bytes per request */

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Enable callback notifications
 */
#define CSM_CALLBACK_SUPPORTED              (STD_ON)

/**
 * @brief Enable job retry on failure
 */
#define CSM_RETRY_FAILED_JOBS               (STD_ON)

/**
 * @brief Maximum retry attempts
 */
#define CSM_MAX_RETRY_ATTEMPTS              (3u)

#endif /* CSM_CFG_H */
