/**
 * @file Csm_Cfg.h
 * @brief CSM (Crypto Services Manager) 配置头文件
 * 
 * 功能: CSM模块的预处理和配置定义
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef CSM_CFG_H
#define CSM_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define CSM_CFG_VENDOR_ID                       43
#define CSM_CFG_AR_RELEASE_MAJOR_VERSION        4
#define CSM_CFG_AR_RELEASE_MINOR_VERSION        7
#define CSM_CFG_AR_RELEASE_REVISION_VERSION     0
#define CSM_CFG_SW_MAJOR_VERSION                1
#define CSM_CFG_SW_MINOR_VERSION                0
#define CSM_CFG_SW_PATCH_VERSION                0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "Csm_Types.h"

/*==================================================================================================
*                                       配置选项
==================================================================================================*/
/**
 * @brief 开发错误检测
 */
#ifndef CSM_CFG_DEV_ERROR_DETECT
#define CSM_CFG_DEV_ERROR_DETECT                STD_ON
#endif

/**
 * @brief 版本信息API
 */
#define CSM_CFG_VERSION_INFO_API                STD_ON

/**
 * @brief 异步服务支持
 */
#define CSM_CFG_ASYNC_SUPPORT                   STD_ON

/**
 * @brief 服务队列支持
 */
#define CSM_CFG_QUEUE_SUPPORT                   STD_ON

/**
 * @brief 密钥持久化支持
 */
#define CSM_CFG_KEY_PERSISTENCE_SUPPORT         STD_ON

/**
 * @brief 随机数生成器支持
 */
#define CSM_CFG_RANDOM_GENERATOR_SUPPORT        STD_ON

/**
 * @brief Dem诊断事件报告
 */
#define CSM_CFG_DEM_INTEGRATION                 STD_ON

/**
 * @brief 加密服务超时时间 (ms)
 */
#define CSM_CFG_SERVICE_TIMEOUT_MS              1000U

/**
 * @brief 主函数调用周期 (ms)
 */
#define CSM_CFG_MAIN_FUNCTION_PERIOD_MS         10U

/*==================================================================================================
*                                       密钥配置
==================================================================================================*/
/**
 * @brief 预定义密钥ID
 */
#define CSM_KEY_ID_NONE                         0xFFFFFFFFU
#define CSM_KEY_ID_MASTER                       0x00000001U
#define CSM_KEY_ID_SESSION                      0x00000002U
#define CSM_KEY_ID_STORAGE                      0x00000003U
#define CSM_KEY_ID_DIAGNOSTIC                   0x00000004U
#define CSM_KEY_ID_SECURE_BOOT                  0x00000005U
#define CSM_KEY_ID_COMMUNICATION                0x00000006U
#define CSM_KEY_ID_DEBUG                        0x00000007U

/**
 * @brief 预定义密钥元素ID
 */
#define CSM_KEY_ELEMENT_ID_SECRET               0x00000001U
#define CSM_KEY_ELEMENT_ID_PUBLIC               0x00000002U
#define CSM_KEY_ELEMENT_ID_PRIVATE              0x00000003U
#define CSM_KEY_ELEMENT_ID_IV                   0x00000004U
#define CSM_KEY_ELEMENT_ID_SALT                 0x00000005U
#define CSM_KEY_ELEMENT_ID_SEED                 0x00000006U

/*==================================================================================================
*                                       作业配置
==================================================================================================*/
/**
 * @brief 预定义作业ID
 */
#define CSM_JOB_ID_NONE                         0xFFFFFFFFU
#define CSM_JOB_ID_HASH_DEFAULT                 0x00000001U
#define CSM_JOB_ID_ENCRYPT_DEFAULT              0x00000002U
#define CSM_JOB_ID_DECRYPT_DEFAULT              0x00000003U
#define CSM_JOB_ID_MAC_GENERATE_DEFAULT         0x00000004U
#define CSM_JOB_ID_MAC_VERIFY_DEFAULT           0x00000005U
#define CSM_JOB_ID_SIGN_DEFAULT                 0x00000006U
#define CSM_JOB_ID_VERIFY_DEFAULT               0x00000007U
#define CSM_JOB_ID_RANDOM_DEFAULT               0x00000008U

/*==================================================================================================
*                                       算法配置
==================================================================================================*/
/**
 * @brief 默认哈希算法 (SHA-256)
 */
#define CSM_CFG_DEFAULT_HASH_ALGORITHM          CSM_ALGOFAM_SHA2_256

/**
 * @brief 默认加密算法 (AES-128-CBC)
 */
#define CSM_CFG_DEFAULT_CIPHER_ALGORITHM        CSM_ALGOFAM_AES
#define CSM_CFG_DEFAULT_CIPHER_MODE             CSM_ALGOMODE_CBC
#define CSM_CFG_DEFAULT_CIPHER_KEY_LENGTH       128U

/**
 * @brief 默认MAC算法 (HMAC-SHA256)
 */
#define CSM_CFG_DEFAULT_MAC_ALGORITHM           CSM_ALGOFAM_HMAC
#define CSM_CFG_DEFAULT_MAC_HASH_ALGORITHM      CSM_ALGOFAM_SHA2_256

/**
 * @brief 默认签名算法 (ECDSA P-256)
 */
#define CSM_CFG_DEFAULT_SIGNATURE_ALGORITHM     CSM_ALGOFAM_ECDSA
#define CSM_CFG_DEFAULT_SIGNATURE_KEY_LENGTH    256U

/*==================================================================================================
*                                       队列配置
==================================================================================================*/
/**
 * @brief 队列大小
 */
#define CSM_CFG_QUEUE_SIZE                      CSM_MAX_QUEUE_DEPTH

/**
 * @brief 同时处理的最大作业数
 */
#define CSM_CFG_MAX_CONCURRENT_JOBS             4U

/*==================================================================================================
*                                       Dem事件ID (用于Dem集成)
==================================================================================================*/
#if (CSM_CFG_DEM_INTEGRATION == STD_ON)
/**
 * @brief 硬件故障事件ID
 */
#define CSM_DEM_HARDWARE_FAILURE_EVENT_ID       0x01U

/**
 * @brief 密钥无效事件ID
 */
#define CSM_DEM_KEY_INVALID_EVENT_ID            0x02U

/**
 * @brief 服务超时事件ID
 */
#define CSM_DEM_SERVICE_TIMEOUT_EVENT_ID        0x03U

/**
 * @brief 随机数稀缺事件ID
 */
#define CSM_DEM_ENTROPY_EXHAUSTION_EVENT_ID     0x04U
#endif

/*==================================================================================================
*                                       回调函数声明
==================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

/**
 * @brief 密钥写入函数
 * 
 * 用于密钥持久化存储
 * 
 * @param keyId 密钥ID
 * @param elementId 元素ID
 * @param data 数据指针
 * @param length 数据长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_Cfg_KeyWrite(
    uint32 keyId,
    uint32 elementId,
    const uint8* data,
    uint32 length
);

/**
 * @brief 密钥读取函数
 * 
 * 用于从持久化存储读取密钥
 * 
 * @param keyId 密钥ID
 * @param elementId 元素ID
 * @param data 数据缓冲区
 * @param length 长度指针 (输入/输出)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_Cfg_KeyRead(
    uint32 keyId,
    uint32 elementId,
    uint8* data,
    uint32* length
);

/**
 * @brief 硬件加密服务函数
 * 
 * 实际加密操作由硬件完成
 * 
 * @param jobId 作业ID
 * @param serviceType 服务类型
 * @param input 输入数据
 * @param inputLength 输入长度
 * @param output 输出缓冲区
 * @param outputLength 输出长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败, E_BUSY: 忙碌
 */
extern Std_ReturnType Csm_Cfg_HwService(
    uint32 jobId,
    Csm_ServiceType serviceType,
    const uint8* input,
    uint32 inputLength,
    uint8* output,
    uint32* outputLength
);

/**
 * @brief 随机数生成函数
 * 
 * @param data 输出缓冲区
 * @param length 需要的长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_Cfg_RandomGenerate(
    uint8* data,
    uint32 length
);

/**
 * @brief 获取当前时间戳
 * 
 * @return 时间戳 (ms)
 */
extern uint32 Csm_Cfg_GetTimestamp(void);

#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief 默认配置
 */
extern const Csm_ConfigType Csm_Config;

#define CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"

#endif /* CSM_CFG_H */
