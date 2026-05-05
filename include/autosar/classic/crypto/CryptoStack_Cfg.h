/**********************************************************************************************************************
 * @file       CryptoStack_Cfg.h
 * @brief      Crypto Stack 栈级配置头文件
 *
 * 功能: 定义Crypto Stack全层级别的配置选项和宏定义
 * 用于CSM, CRYIF, Crypto层的统一配置
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef CRYPTOSTACK_CFG_H
#define CRYPTOSTACK_CFG_H

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define CRYPTOSTACK_CFG_VENDOR_ID                       0x2025U  /* YuleTech */
#define CRYPTOSTACK_CFG_MODULE_ID                       0xF1U    /* Crypto Stack Config */
#define CRYPTOSTACK_CFG_AR_RELEASE_MAJOR_VERSION        4U
#define CRYPTOSTACK_CFG_AR_RELEASE_MINOR_VERSION        7U
#define CRYPTOSTACK_CFG_AR_RELEASE_REVISION_VERSION     0U
#define CRYPTOSTACK_CFG_SW_MAJOR_VERSION                1U
#define CRYPTOSTACK_CFG_SW_MINOR_VERSION                0U
#define CRYPTOSTACK_CFG_SW_PATCH_VERSION                0U

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "CryptoStack_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS - 版本检查
 *********************************************************************************************************************/
#define CRYPTOSTACK_CFG_MAJOR_VERSION                   1U
#define CRYPTOSTACK_CFG_MINOR_VERSION                   0U
#define CRYPTOSTACK_CFG_PATCH_VERSION                   0U

/**********************************************************************************************************************
 * GENERAL CONFIGURATION (通用配置)
 *********************************************************************************************************************/

/* 开发错误检测使能 */
#define CRYPTO_STACK_DEV_ERROR_DETECT                   STD_ON

/* 版本信息API使能 */
#define CRYPTO_STACK_VERSION_INFO_API                   STD_ON

/* 调试模式使能 (额外的调试输出) */
#define CRYPTO_STACK_DEBUG_MODE                         STD_OFF

/* 日志功能使能 */
#define CRYPTO_STACK_LOG_ENABLE                         STD_OFF

/* 安全启动模式 (增加安全检查) */
#define CRYPTO_STACK_SECURE_BOOT                        STD_OFF

/**********************************************************************************************************************
 * CSM LAYER CONFIGURATION (CSM层配置)
 *********************************************************************************************************************/

/* CSM模块ID */
#define CSM_MODULE_ID                                   0x70U

/* 最大作业数量 */
#define CSM_MAX_JOBS                                    16U

/* 最大密钥数量 */
#define CSM_MAX_KEYS                                    32U

/* 最大密钥元素数量 */
#define CSM_MAX_KEY_ELEMENTS                            8U

/* 最大队列深度 */
#define CSM_MAX_QUEUE_DEPTH                             8U

/* 最大通道数量 */
#define CSM_MAX_CHANNELS                                4U

/* 启用异步处理 */
#define CSM_ASYNC_MODE_ENABLE                           STD_ON

/* 启用同步处理 */
#define CSM_SYNC_MODE_ENABLE                            STD_ON

/* 队列处理周期 (ms) */
#define CSM_QUEUE_PROCESSING_PERIOD                     10U

/* 启用密钥使用计数 */
#define CSM_KEY_USAGE_COUNT_ENABLE                      STD_OFF

/* 启用密钥有效期检查 */
#define CSM_KEY_VALIDITY_PERIOD_ENABLE                  STD_OFF

/**********************************************************************************************************************
 * CRYIF LAYER CONFIGURATION (CRYIF层配置)
 *********************************************************************************************************************/

/* CRYIF模块ID */
#define CRYIF_MODULE_ID                                 0x71U

/* 最大Crypto驱动实例数 */
#define CRYIF_MAX_CRYPTO_DRIVERS                        4U

/* 最大通道数量 */
#define CRYIF_MAX_CHANNELS                              8U

/* 启用键值映射 */
#define CRYIF_KEY_MAPPING_ENABLE                        STD_ON

/* 启用算法映射 */
#define CRYIF_ALGORITHM_MAPPING_ENABLE                  STD_ON

/* 启用错误转换 */
#define CRYIF_ERROR_TRANSLATION_ENABLE                  STD_ON

/**********************************************************************************************************************
 * CRYPTO DRIVER LAYER CONFIGURATION (Crypto驱动层配置)
 *********************************************************************************************************************/

/* CRYPTO模块ID */
#define CRYPTO_MODULE_ID                                0x6EU

/* 最大密钥数量 */
#define CRYPTO_MAX_KEYS                                 64U

/* 最大密钥元素数量 */
#define CRYPTO_MAX_KEY_ELEMENTS                         16U

/* 最大通道数量 */
#define CRYPTO_MAX_CHANNELS                             8U

/* 最大并行作业数 */
#define CRYPTO_MAX_PARALLEL_JOBS                        4U

/* 最大数据长度 (字节) */
#define CRYPTO_MAX_DATA_LENGTH                          1024U

/* 最大密钥长度 (字节) */
#define CRYPTO_MAX_KEY_LENGTH                           128U

/* 最大IV长度 (字节) */
#define CRYPTO_MAX_IV_LENGTH                            16U

/* 最大哈希长度 (字节) - SHA-512 */
#define CRYPTO_MAX_HASH_LENGTH                          64U

/* 最大签名长度 (字节) - 支持RSA-4096或ECDSA P-521 */
#define CRYPTO_MAX_SIGNATURE_LENGTH                     144U

/* 最大随机数长度 (字节) */
#define CRYPTO_MAX_RANDOM_LENGTH                        256U

/**********************************************************************************************************************
 * ALGORITHM SUPPORT CONFIGURATION (算法支持配置)
 *********************************************************************************************************************/

/* AES加密支持 */
#define CRYPTO_STACK_SUPPORT_AES                        STD_ON
#define CRYPTO_STACK_AES_128_ENABLE                     STD_ON
#define CRYPTO_STACK_AES_192_ENABLE                     STD_ON
#define CRYPTO_STACK_AES_256_ENABLE                     STD_ON

/* AES模式支持 */
#define CRYPTO_STACK_SUPPORT_AES_ECB                    STD_ON
#define CRYPTO_STACK_SUPPORT_AES_CBC                    STD_ON
#define CRYPTO_STACK_SUPPORT_AES_CTR                    STD_ON
#define CRYPTO_STACK_SUPPORT_AES_GCM                    STD_ON
#define CRYPTO_STACK_SUPPORT_AES_CCM                    STD_ON
#define CRYPTO_STACK_SUPPORT_AES_CFB                    STD_OFF
#define CRYPTO_STACK_SUPPORT_AES_OFB                    STD_OFF

/* 哈希算法支持 */
#define CRYPTO_STACK_SUPPORT_SHA1                       STD_OFF   /* 不安全，默认关闭 */
#define CRYPTO_STACK_SUPPORT_SHA2_224                   STD_ON
#define CRYPTO_STACK_SUPPORT_SHA2_256                   STD_ON
#define CRYPTO_STACK_SUPPORT_SHA2_384                   STD_ON
#define CRYPTO_STACK_SUPPORT_SHA2_512                   STD_ON
#define CRYPTO_STACK_SUPPORT_SHA3                       STD_OFF

/* MAC算法支持 */
#define CRYPTO_STACK_SUPPORT_HMAC                       STD_ON
#define CRYPTO_STACK_SUPPORT_CMAC                       STD_ON
#define CRYPTO_STACK_SUPPORT_GMAC                       STD_ON

/* 数字签名支持 */
#define CRYPTO_STACK_SUPPORT_RSA                        STD_OFF
#define CRYPTO_STACK_SUPPORT_ECDSA                      STD_ON
#define CRYPTO_STACK_SUPPORT_EDDSA                      STD_OFF

/* ECC曲线支持 */
#define CRYPTO_STACK_SUPPORT_ECC_SECP256R1              STD_ON
#define CRYPTO_STACK_SUPPORT_ECC_SECP384R1              STD_ON
#define CRYPTO_STACK_SUPPORT_ECC_SECP521R1              STD_OFF
#define CRYPTO_STACK_SUPPORT_ECC_SECP256K1              STD_OFF
#define CRYPTO_STACK_SUPPORT_ECC_BRAINPOOLP256R1        STD_OFF
#define CRYPTO_STACK_SUPPORT_ECC_BRAINPOOLP384R1        STD_OFF
#define CRYPTO_STACK_SUPPORT_ECC_ED25519                STD_OFF

/* 密钥交换支持 */
#define CRYPTO_STACK_SUPPORT_ECDH                       STD_ON
#define CRYPTO_STACK_SUPPORT_ECIES                      STD_OFF

/* 密钥派生支持 */
#define CRYPTO_STACK_SUPPORT_HKDF                       STD_ON
#define CRYPTO_STACK_SUPPORT_PBKDF2                     STD_OFF

/* 随机数生成支持 */
#define CRYPTO_STACK_SUPPORT_CTR_DRBG                   STD_ON
#define CRYPTO_STACK_SUPPORT_HASH_DRBG                  STD_OFF

/**********************************************************************************************************************
 * HARDWARE ACCELERATION CONFIGURATION (硬件加速配置)
 *********************************************************************************************************************/

/* 硬件加速使能 */
#define CRYPTO_STACK_HW_ACCELERATION_ENABLE             STD_ON

/* HSM (Hardware Security Module) 支持 */
#define CRYPTO_STACK_HSM_ENABLE                         STD_OFF

/* SHE (Secure Hardware Extension) 支持 */
#define CRYPTO_STACK_SHE_ENABLE                         STD_OFF

/* TPM (Trusted Platform Module) 支持 */
#define CRYPTO_STACK_TPM_ENABLE                         STD_OFF

/* AES-NI (Intel AES New Instructions) 支持 */
#define CRYPTO_STACK_AESNI_ENABLE                       STD_OFF

/* ARM Crypto Extensions 支持 */
#define CRYPTO_STACK_ARM_CRYPTO_ENABLE                  STD_OFF

/**********************************************************************************************************************
 * BUFFER SIZE CONFIGURATION (缓冲区大小配置)
 *********************************************************************************************************************/

/* 工作缓冲区大小 (用于内部计算) */
#define CRYPTO_STACK_WORK_BUFFER_SIZE                   2048U

/* 密钥存储缓冲区大小 */
#define CRYPTO_STACK_KEY_STORAGE_BUFFER_SIZE            4096U

/* 队列项目缓冲区大小 */
#define CRYPTO_STACK_QUEUE_BUFFER_SIZE                  (CSM_MAX_QUEUE_DEPTH * sizeof(Crypto_JobType))

/**********************************************************************************************************************
 * TIMEOUT CONFIGURATION (超时配置)
 *********************************************************************************************************************/

/* 操作超时 (ms) - 同步操作 */
#define CRYPTO_STACK_SYNC_OPERATION_TIMEOUT             5000U

/* 操作超时 (ms) - 异步操作 */
#define CRYPTO_STACK_ASYNC_OPERATION_TIMEOUT            30000U

/* 随机数生成超时 (ms) */
#define CRYPTO_STACK_RANDOM_GENERATION_TIMEOUT          100U

/* 密钥生成超时 (ms) */
#define CRYPTO_STACK_KEY_GENERATION_TIMEOUT             5000U

/* 密钥导入超时 (ms) */
#define CRYPTO_STACK_KEY_IMPORT_TIMEOUT                 1000U

/* 密钥导出超时 (ms) */
#define CRYPTO_STACK_KEY_EXPORT_TIMEOUT                 1000U

/**********************************************************************************************************************
 * SECURITY LEVEL CONFIGURATION (安全等级配置)
 *********************************************************************************************************************/

/* 安全等级 (0-5, 数字越高越安全) */
#define CRYPTO_STACK_SECURITY_LEVEL                     2U

/* 最小随机数熵要求 (比特) */
#define CRYPTO_STACK_MIN_ENTROPY_BITS                   128U

/* 启用密钥淡出检测 */
#define CRYPTO_STACK_KEY_SCRUBBING_ENABLE               STD_ON

/* 密钥淡出模式 (0=不填充, 1=零填充, 2=随机填充) */
#define CRYPTO_STACK_KEY_SCRUBBING_MODE                 1U

/* 启用内存清零 */
#define CRYPTO_STACK_MEMORY_CLEAR_ENABLE                STD_ON

/* 启用运行时完整性检查 */
#define CRYPTO_STACK_RUNTIME_INTEGRITY_CHECK            STD_OFF

/**********************************************************************************************************************
 * CALLBACK CONFIGURATION (回调配置)
 *********************************************************************************************************************/

/* 启用作业完成回调 */
#define CRYPTO_STACK_JOB_CALLBACK_ENABLE                STD_ON

/* 启用密钥状态变更回调 */
#define CRYPTO_STACK_KEY_CALLBACK_ENABLE                STD_OFF

/* 启用错误通知回调 */
#define CRYPTO_STACK_ERROR_CALLBACK_ENABLE              STD_OFF

/* 启用进度通知回调 */
#define CRYPTO_STACK_PROGRESS_CALLBACK_ENABLE           STD_OFF

/**********************************************************************************************************************
 * COMPATIBILITY CONFIGURATION (兼容性配置)
 *********************************************************************************************************************/

/* 启用旧版API支持 */
#define CRYPTO_STACK_LEGACY_API_ENABLE                  STD_OFF

/* 启用AUTOSAR 4.6兼容性模式 */
#define CRYPTO_STACK_AUTOSAR_46_COMPATIBILITY           STD_OFF

/* 启用自定义扩展 */
#define CRYPTO_STACK_CUSTOM_EXTENSIONS_ENABLE           STD_OFF

/**********************************************************************************************************************
 * ASSERTION MACROS (断言宏)
 *********************************************************************************************************************/
#if (CRYPTO_STACK_DEV_ERROR_DETECT == STD_ON)
    #define CRYPTO_STACK_ASSERT(condition, errorCode) \
        do { \
            if (!(condition)) { \
                CryptoStack_ReportError(__FILE__, __LINE__, errorCode); \
            } \
        } while(0)
    
    #define CRYPTO_STACK_ASSERT_PTR_VALID(ptr) \
        CRYPTO_STACK_ASSERT((ptr) != NULL_PTR, CRYPTO_ERROR_PARAM_POINTER)
    
    #define CRYPTO_STACK_ASSERT_KEY_ID_VALID(keyId) \
        CRYPTO_STACK_ASSERT((keyId) < CRYPTO_MAX_KEYS, CRYPTO_ERROR_PARAM_KEY_ID)
    
    #define CRYPTO_STACK_ASSERT_JOB_ID_VALID(jobId) \
        CRYPTO_STACK_ASSERT((jobId) < CSM_MAX_JOBS, CRYPTO_ERROR_PARAM_JOB_ID)
#else
    #define CRYPTO_STACK_ASSERT(condition, errorCode)         ((void)0)
    #define CRYPTO_STACK_ASSERT_PTR_VALID(ptr)                ((void)0)
    #define CRYPTO_STACK_ASSERT_KEY_ID_VALID(keyId)           ((void)0)
    #define CRYPTO_STACK_ASSERT_JOB_ID_VALID(jobId)           ((void)0)
#endif

/**********************************************************************************************************************
 * UTILITY MACROS (工具宏)
 *********************************************************************************************************************/

/* 计算数组大小 */
#define CRYPTO_STACK_ARRAY_SIZE(array)                  (sizeof(array) / sizeof((array)[0]))

/* 字节数据清零 */
#define CRYPTO_STACK_ZEROIZE(ptr, size) \
    do { \
        if ((ptr) != NULL_PTR && (size) > 0U) { \
            for (uint32 i = 0U; i < (size); i++) { \
                ((uint8*)(ptr))[i] = 0U; \
            } \
        } \
    } while(0)

/* 安全内存拷贝 */
#define CRYPTO_STACK_MEMCPY(dst, src, size) \
    do { \
        if ((dst) != NULL_PTR && (src) != NULL_PTR && (size) > 0U) { \
            for (uint32 i = 0U; i < (size); i++) { \
                ((uint8*)(dst))[i] = ((const uint8*)(src))[i]; \
            } \
        } \
    } while(0)

/* 安全内存比较 (防步时攻击) */
#define CRYPTO_STACK_MEMCMP_CONST_TIME(a, b, size) \
    ({ \
        uint8 result = 0U; \
        for (uint32 i = 0U; i < (size); i++) { \
            result |= ((const uint8*)(a))[i] ^ ((const uint8*)(b))[i]; \
        } \
        result; \
    })

/**********************************************************************************************************************
 * ERROR CODE MAPPING (错误码映射)
 *********************************************************************************************************************/

/* CSM错误码 */
#define CSM_E_NO_ERROR                          E_OK
#define CSM_E_NOT_INITIALIZED                   0x01U
#define CSM_E_ALREADY_INITIALIZED               0x02U
#define CSM_E_PARAM_POINTER                     0x03U
#define CSM_E_PARAM_KEY_ID                      0x04U
#define CSM_E_PARAM_KEY_ELEMENT_ID              0x05U
#define CSM_E_PARAM_JOB_ID                      0x06U
#define CSM_E_PARAM_ALGORITHM                   0x07U
#define CSM_E_PARAM_MODE                        0x08U
#define CSM_E_PARAM_LENGTH                      0x09U
#define CSM_E_KEY_NOT_AVAILABLE                 0x0AU
#define CSM_E_KEY_NOT_VALID                     0x0BU
#define CSM_E_KEY_SIZE_MISMATCH                 0x0CU
#define CSM_E_JOB_BUSY                          0x0DU
#define CSM_E_QUEUE_FULL                        0x0EU
#define CSM_E_SERVICE_NOT_SUPPORTED             0x0FU

/* CRYIF错误码 */
#define CRYIF_E_NO_ERROR                        E_OK
#define CRYIF_E_PARAM_POINTER                   0x01U
#define CRYIF_E_PARAM_HANDLE                    0x02U
#define CRYIF_E_PARAM_KEY_ID                    0x03U
#define CRYIF_E_PARAM_ALGORITHM                 0x04U
#define CRYIF_E_PARAM_KEY_FORMAT                0x05U

/* CRYPTO错误码 */
#define CRYPTO_E_NO_ERROR                       E_OK
#define CRYPTO_E_BUSY                           0x01U
#define CRYPTO_E_SMALL_BUFFER                   0x02U
#define CRYPTO_E_ENTROPY_EXHAUSTION             0x03U
#define CRYPTO_E_QUEUE_FULL                     0x04U
#define CRYPTO_E_JOB_CANCELED                   0x05U
#define CRYPTO_E_KEY_NOT_VALID                  0x06U
#define CRYPTO_E_KEY_SIZE_MISMATCH              0x07U
#define CRYPTO_E_KEY_READ_FAIL                  0x0AU
#define CRYPTO_E_KEY_WRITE_FAIL                 0x0BU
#define CRYPTO_E_KEY_NOT_AVAILABLE              0x0CU

/**********************************************************************************************************************
 * EXTERN DECLARATIONS (外部声明)
 *********************************************************************************************************************/
#if (CRYPTO_STACK_DEV_ERROR_DETECT == STD_ON)
extern void CryptoStack_ReportError(
    const char* file,
    uint32 line,
    Crypto_ErrorType errorCode
);
#endif

/**********************************************************************************************************************
 * DEFAULT CONFIGURATION STRUCTURE (默认配置结构体声明)
 *********************************************************************************************************************/
extern const CryptoStack_ConfigType CryptoStack_DefaultConfig;

#endif /* CRYPTOSTACK_CFG_H */
