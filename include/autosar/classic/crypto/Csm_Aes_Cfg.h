/**********************************************************************************************************************
 * @file       Csm_Aes_Cfg.h
 * @brief      CSM (Crypto Services Manager) AES配置
 *
 * 功能: 配置CSM层AES服务
 *       定义AES密钥、算法和作业配置
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef CSM_AES_CFG_H
#define CSM_AES_CFG_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "CryptoStack_Types.h"

/**********************************************************************************************************************
 * 版本信息
 *********************************************************************************************************************/
#define CSM_AES_CFG_VENDOR_ID                   0x2025U
#define CSM_AES_CFG_MODULE_ID                   0xF2U
#define CSM_AES_CFG_SW_MAJOR_VERSION            1U
#define CSM_AES_CFG_SW_MINOR_VERSION            0U
#define CSM_AES_CFG_SW_PATCH_VERSION            0U

/**********************************************************************************************************************
 * AES服务配置
 *********************************************************************************************************************/

/* AES-ECB服务ID */
#define CSM_AES_ECB_ENCRYPT_SERVICE_ID          0x0100U
#define CSM_AES_ECB_DECRYPT_SERVICE_ID          0x0101U

/* AES-CBC服务ID */
#define CSM_AES_CBC_ENCRYPT_SERVICE_ID          0x0110U
#define CSM_AES_CBC_DECRYPT_SERVICE_ID          0x0111U

/* AES-CFB服务ID */
#define CSM_AES_CFB_ENCRYPT_SERVICE_ID          0x0120U
#define CSM_AES_CFB_DECRYPT_SERVICE_ID          0x0121U

/* AES-OFB服务ID */
#define CSM_AES_OFB_ENCRYPT_SERVICE_ID          0x0130U
#define CSM_AES_OFB_DECRYPT_SERVICE_ID          0x0131U

/* AES-CTR服务ID */
#define CSM_AES_CTR_ENCRYPT_SERVICE_ID          0x0140U
#define CSM_AES_CTR_DECRYPT_SERVICE_ID          0x0141U

/* AES-GCM服务ID (AEAD) */
#define CSM_AES_GCM_ENCRYPT_SERVICE_ID          0x0150U
#define CSM_AES_GCM_DECRYPT_SERVICE_ID          0x0151U

/* AES-CCM服务ID (AEAD) */
#define CSM_AES_CCM_ENCRYPT_SERVICE_ID          0x0160U
#define CSM_AES_CCM_DECRYPT_SERVICE_ID          0x0161U

/**********************************************************************************************************************
 * AES密钥配置
 *********************************************************************************************************************/

/* AES-128密钥 */
#define CSM_AES_KEY_128_ID                      1U
#define CSM_AES_KEY_128_LENGTH                  16U

/* AES-192密钥 */
#define CSM_AES_KEY_192_ID                      2U
#define CSM_AES_KEY_192_LENGTH                  24U

/* AES-256密钥 */
#define CSM_AES_KEY_256_ID                      3U
#define CSM_AES_KEY_256_LENGTH                  32U

/* AES IV */
#define CSM_AES_IV_ELEMENT_ID                   0x01U
#define CSM_AES_IV_LENGTH                       16U

/* AES Nonce (用于GCM/CCM) */
#define CSM_AES_NONCE_ELEMENT_ID                0x02U
#define CSM_AES_NONCE_MAX_LENGTH                16U

/* AES AAD (附加认证数据) */
#define CSM_AES_AAD_ELEMENT_ID                  0x03U
#define CSM_AES_AAD_MAX_LENGTH                  1024U

/* AES Tag (认证标签) */
#define CSM_AES_TAG_ELEMENT_ID                  0x04U
#define CSM_AES_TAG_MAX_LENGTH                  16U

/**********************************************************************************************************************
 * AES算法配置
 *********************************************************************************************************************/

/* 预定义AES算法配置 */
#define CSM_AES_128_ECB_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_ECB,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 128U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_128_CBC_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_CBC,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 128U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_128_CTR_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_CTR,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 128U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_128_GCM_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_GCM,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 128U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_128_CCM_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_CCM,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 128U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_256_CBC_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_CBC,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 256U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

#define CSM_AES_256_GCM_ALGORITHM                                   \
{                                                                   \
    .family = CRYPTO_ALGOFAM_AES,                                   \
    .mode = CRYPTO_ALGOMODE_GCM,                                    \
    .classType = CRYPTO_ALGOCLASS_CIPHER,                           \
    .keyLength = 256U,                                              \
    .curve = CRYPTO_ECC_CURVE_NONE,                                 \
    .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET                       \
}

/**********************************************************************************************************************
 * 作业配置
 *********************************************************************************************************************/

/* AES加密作业配置 */
#define CSM_AES_ENCRYPT_JOB_CFG                                     \
{                                                                   \
    .jobId = 0U,                                                    \
    .priority = 1                                                   \
}

/* AES解密作业配置 */
#define CSM_AES_DECRYPT_JOB_CFG                                     \
{                                                                   \
    .jobId = 0U,                                                    \
    .priority = 1                                                   \
}

/**********************************************************************************************************************
 * 服务原语信息
 *********************************************************************************************************************/

/* AES-128-CBC加密服务 */
#define CSM_AES_128_CBC_ENCRYPT_PRIMITIVE                           \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_128_CBC_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_ENCRYPT,                              \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/* AES-128-CBC解密服务 */
#define CSM_AES_128_CBC_DECRYPT_PRIMITIVE                           \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_128_CBC_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_DECRYPT,                              \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/* AES-128-GCM AEAD加密服务 */
#define CSM_AES_128_GCM_AEAD_ENCRYPT_PRIMITIVE                      \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_128_GCM_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_AEADENCRYPT,                          \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/* AES-128-GCM AEAD解密服务 */
#define CSM_AES_128_GCM_AEAD_DECRYPT_PRIMITIVE                      \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_128_GCM_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_AEADDECRYPT,                          \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/* AES-256-GCM AEAD加密服务 */
#define CSM_AES_256_GCM_AEAD_ENCRYPT_PRIMITIVE                      \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_256_GCM_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_AEADENCRYPT,                          \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/* AES-256-GCM AEAD解密服务 */
#define CSM_AES_256_GCM_AEAD_DECRYPT_PRIMITIVE                      \
{                                                                   \
    .callbackId = 0U,                                               \
    .algorithm = CSM_AES_256_GCM_ALGORITHM,                         \
    .service = CRYPTO_SERVICE_AEADDECRYPT,                          \
    .processingType = CRYPTO_PROCESSING_SYNC,                       \
    .primitiveCallbackUpdateNotification = FALSE                    \
}

/**********************************************************************************************************************
 * 密钥元素配置
 *********************************************************************************************************************/

#define CSM_AES_KEY_ELEMENT_IV_CONFIG                               \
{                                                                   \
    .id = CSM_AES_IV_ELEMENT_ID,                                    \
    .type = CRYPTO_KEYELEMENT_TYPE_IV,                              \
    .maxLength = CSM_AES_IV_LENGTH,                                 \
    .readAccess = TRUE,                                             \
    .writeAccess = TRUE,                                            \
    .partialAccess = FALSE                                          \
}

#define CSM_AES_KEY_ELEMENT_NONCE_CONFIG                            \
{                                                                   \
    .id = CSM_AES_NONCE_ELEMENT_ID,                                 \
    .type = CRYPTO_KEYELEMENT_TYPE_NONCE,                           \
    .maxLength = CSM_AES_NONCE_MAX_LENGTH,                          \
    .readAccess = TRUE,                                             \
    .writeAccess = TRUE,                                            \
    .partialAccess = FALSE                                          \
}

#define CSM_AES_KEY_ELEMENT_AAD_CONFIG                              \
{                                                                   \
    .id = CSM_AES_AAD_ELEMENT_ID,                                   \
    .type = CRYPTO_KEYELEMENT_TYPE_AUTH_DATA,                       \
    .maxLength = CSM_AES_AAD_MAX_LENGTH,                            \
    .readAccess = TRUE,                                             \
    .writeAccess = TRUE,                                            \
    .partialAccess = FALSE                                          \
}

#define CSM_AES_KEY_ELEMENT_TAG_CONFIG                              \
{                                                                   \
    .id = CSM_AES_TAG_ELEMENT_ID,                                   \
    .type = CRYPTO_KEYELEMENT_TYPE_TAG,                             \
    .maxLength = CSM_AES_TAG_MAX_LENGTH,                            \
    .readAccess = TRUE,                                             \
    .writeAccess = TRUE,                                            \
    .partialAccess = FALSE                                          \
}

/**********************************************************************************************************************
 * 功能宏和内联函数
 *********************************************************************************************************************/

/**
 * @brief 检查是否是AES服务
 */
#define Csm_IsAesService(service)                                   \
    (((service) == CRYPTO_SERVICE_ENCRYPT) ||                       \
     ((service) == CRYPTO_SERVICE_DECRYPT) ||                       \
     ((service) == CRYPTO_SERVICE_AEADENCRYPT) ||                   \
     ((service) == CRYPTO_SERVICE_AEADDECRYPT))

/**
 * @brief 检查是否是AES模式
 */
#define Csm_IsAesMode(mode)                                         \
    (((mode) == CRYPTO_ALGOMODE_ECB) ||                             \
     ((mode) == CRYPTO_ALGOMODE_CBC) ||                             \
     ((mode) == CRYPTO_ALGOMODE_CFB) ||                             \
     ((mode) == CRYPTO_ALGOMODE_OFB) ||                             \
     ((mode) == CRYPTO_ALGOMODE_CTR) ||                             \
     ((mode) == CRYPTO_ALGOMODE_GCM) ||                             \
     ((mode) == CRYPTO_ALGOMODE_CCM))

/**
 * @brief 获取默认AES密钥长度
 */
#define Csm_GetAesDefaultKeyLength(algorithm)                       \
    (((algorithm)->keyLength) / 8U)

#endif /* CSM_AES_CFG_H */
