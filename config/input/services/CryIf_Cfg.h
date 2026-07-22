/**
 * @file CryIf_Cfg.h
 * @brief Crypto Interface Configuration Header
 * @version 1.0.0
 * @date 2026-05-01
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details Configuration parameters for CRYIF module
 */

#ifndef CRYIF_CFG_H
#define CRYIF_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "CryIf_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CRYIF_CFG_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define CRYIF_CFG_AR_RELEASE_MINOR_VERSION      (0x04U)
#define CRYIF_CFG_AR_RELEASE_REVISION_VERSION   (0x00U)
#define CRYIF_CFG_SW_MAJOR_VERSION              (0x01U)
#define CRYIF_CFG_SW_MINOR_VERSION              (0x00U)
#define CRYIF_CFG_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/** @brief Development error detection enable/disable */
#ifndef CRYIF_DEV_ERROR_DETECT
#define CRYIF_DEV_ERROR_DETECT                  (STD_ON)
#endif

/** @brief Version info API enable/disable */
#ifndef CRYIF_VERSION_INFO_API
#define CRYIF_VERSION_INFO_API                  (STD_ON)
#endif

/** @brief Key element copy API enable/disable */
#ifndef CRYIF_KEY_ELEMENT_COPY_API
#define CRYIF_KEY_ELEMENT_COPY_API              (STD_ON)
#endif

/** @brief Key valid check API enable/disable */
#ifndef CRYIF_KEY_VALID_CHECK_API
#define CRYIF_KEY_VALID_CHECK_API               (STD_ON)
#endif

/*==================================================================================================
*                                    CONFIGURATION PARAMETERS
==================================================================================================*/

/** @brief Maximum number of channels */
#define CRYIF_CFG_MAX_CHANNEL_COUNT             (0x04U)

/** @brief Maximum number of keys */
#define CRYIF_CFG_MAX_KEY_COUNT                 (0x08U)

/** @brief Maximum number of jobs */
#define CRYIF_CFG_MAX_JOB_COUNT                 (0x10U)

/** @brief Maximum buffer size for crypto operations */
#define CRYIF_CFG_MAX_BUFFER_SIZE               (0x400U)

/** @brief Maximum key element size */
#define CRYIF_CFG_MAX_KEY_ELEMENT_SIZE          (0x100U)

/** @brief Number of configured channels */
#define CRYIF_CFG_NUM_CHANNELS                  (0x02U)

/** @brief Number of configured keys */
#define CRYIF_CFG_NUM_KEYS                      (0x04U)

/** @brief Main function period in milliseconds */
#define CRYIF_CFG_MAIN_FUNCTION_PERIOD_MS       (0x0AU)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/** @brief Channel 0 - Primary Hardware Channel */
#define CRYIF_CFG_CHANNEL_0_ID                  (0x00U)
#define CRYIF_CFG_CHANNEL_0_DRIVER_INDEX        (0x00U)
#define CRYIF_CFG_CHANNEL_0_DRIVER_OBJ          (0x00U)
#define CRYIF_CFG_CHANNEL_0_MAX_KEY_SIZE        (0x100U)
#define CRYIF_CFG_CHANNEL_0_MAX_JOB_SIZE        (0x400U)

/** @brief Channel 1 - Secondary Hardware Channel */
#define CRYIF_CFG_CHANNEL_1_ID                  (0x01U)
#define CRYIF_CFG_CHANNEL_1_DRIVER_INDEX        (0x00U)
#define CRYIF_CFG_CHANNEL_1_DRIVER_OBJ          (0x01U)
#define CRYIF_CFG_CHANNEL_1_MAX_KEY_SIZE        (0x100U)
#define CRYIF_CFG_CHANNEL_1_MAX_JOB_SIZE        (0x400U)

/*==================================================================================================
*                                    KEY CONFIGURATION
==================================================================================================*/

/** @brief Key 0 - Master Key */
#define CRYIF_CFG_KEY_0_ID                      (0x00U)
#define CRYIF_CFG_KEY_0_CRYPTO_KEY_ID           (0x00U)
#define CRYIF_CFG_KEY_0_DRIVER_INDEX            (0x00U)
#define CRYIF_CFG_KEY_0_SECURITY_LEVEL          (CRYIF_SEC_LEVEL_3)

/** @brief Key 1 - Session Key */
#define CRYIF_CFG_KEY_1_ID                      (0x01U)
#define CRYIF_CFG_KEY_1_CRYPTO_KEY_ID           (0x01U)
#define CRYIF_CFG_KEY_1_DRIVER_INDEX            (0x00U)
#define CRYIF_CFG_KEY_1_SECURITY_LEVEL          (CRYIF_SEC_LEVEL_2)

/** @brief Key 2 - Application Key */
#define CRYIF_CFG_KEY_2_ID                      (0x02U)
#define CRYIF_CFG_KEY_2_CRYPTO_KEY_ID           (0x02U)
#define CRYIF_CFG_KEY_2_DRIVER_INDEX            (0x00U)
#define CRYIF_CFG_KEY_2_SECURITY_LEVEL          (CRYIF_SEC_LEVEL_1)

/** @brief Key 3 - Debug Key */
#define CRYIF_CFG_KEY_3_ID                      (0x03U)
#define CRYIF_CFG_KEY_3_CRYPTO_KEY_ID           (0x03U)
#define CRYIF_CFG_KEY_3_DRIVER_INDEX            (0x00U)
#define CRYIF_CFG_KEY_3_SECURITY_LEVEL          (CRYIF_SEC_LEVEL_NONE)

/*==================================================================================================
*                                    KEY ELEMENT CONFIGURATION
==================================================================================================*/

/** @brief Key Element IDs as per AutoSAR Crypto standard */
#define CRYIF_KEY_ELEMENT_ID_IV                 (0x01U)
#define CRYIF_KEY_ELEMENT_ID_KEY                (0x02U)
#define CRYIF_KEY_ELEMENT_ID_SALT               (0x03U)
#define CRYIF_KEY_ELEMENT_ID_ITERATIONS         (0x04U)
#define CRYIF_KEY_ELEMENT_ID_ALGORITHM          (0x05U)
#define CRYIF_KEY_ELEMENT_ID_SEED_STATE         (0x06U)
#define CRYIF_KEY_ELEMENT_ID_DRBG_STATE         (0x07U)
#define CRYIF_KEY_ELEMENT_ID_MAC                (0x08U)
#define CRYIF_KEY_ELEMENT_ID_SIGNATURE          (0x09U)
#define CRYIF_KEY_ELEMENT_ID_PUBKEY             (0x0AU)
#define CRYIF_KEY_ELEMENT_ID_PRIVKEY            (0x0BU)
#define CRYIF_KEY_ELEMENT_ID_CERTIFICATE        (0x0CU)
#define CRYIF_KEY_ELEMENT_ID_CERTIFICATE_DATA   (0x0DU)
#define CRYIF_KEY_ELEMENT_ID_CERTIFICATE_SIGN   (0x0EU)
#define CRYIF_KEY_ELEMENT_ID_CERTIFICATE_ID     (0x0FU)

/*==================================================================================================
*                                    SECURITY LEVEL MAPPING
==================================================================================================*/

/** @brief Security level to CSM security service mapping */
#define CRYIF_CFG_SEC_LEVEL_TO_CSM(level)       ((level) * 0x10U)

/** @brief Security level validation macro */
#define CRYIF_CFG_IS_VALID_SEC_LEVEL(level)     \
    (((level) >= CRYIF_SEC_LEVEL_NONE) && ((level) <= CRYIF_SEC_LEVEL_7))

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/** @brief Enable notification callbacks */
#define CRYIF_CFG_NOTIFICATION_ENABLED          (STD_ON)

/** @brief Job completion callback function name */
#define CRYIF_CFG_JOB_CALLBACK                  CryIf_JobNotificationCallback

/** @brief Key operation callback function name */
#define CRYIF_CFG_KEY_CALLBACK                  CryIf_KeyNotificationCallback

/*==================================================================================================
*                                    DEBUG CONFIGURATION
==================================================================================================*/

/** @brief Enable debug logging */
#ifndef CRYIF_DEBUG_ENABLED
#define CRYIF_DEBUG_ENABLED                     (STD_OFF)
#endif

/** @brief Debug print macro */
#if defined(CRYIF_DEBUG_ENABLED) && (CRYIF_DEBUG_ENABLED == STD_ON)
    #include <stdio.h>
    #define CRYIF_DBG_PRINT(fmt, ...)           printf("[CRYIF] " fmt "\n", ##__VA_ARGS__)
#else
    #define CRYIF_DBG_PRINT(fmt, ...)           ((void)0)
#endif

/*==================================================================================================
*                                    EXTERNAL CONFIGURATION
==================================================================================================*/

/** @brief External configuration structure declaration */
extern const CryIf_ConfigType CryIf_Config;

#endif /* CRYIF_CFG_H */
