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

/**
 * @file Csm_Types.h
 * @brief CSM (Crypto Services Manager) 类型定义头文件
 * 
 * 功能: 定义CSM模块使用的所有数据类型、枚举和结构体
 * 符合AUTOSAR 4.7.0标准
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef CSM_TYPES_H
#define CSM_TYPES_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define CSM_TYPES_VENDOR_ID                     43
#define CSM_TYPES_AR_RELEASE_MAJOR_VERSION      4
#define CSM_TYPES_AR_RELEASE_MINOR_VERSION      7
#define CSM_TYPES_AR_RELEASE_REVISION_VERSION   0
#define CSM_TYPES_SW_MAJOR_VERSION              1
#define CSM_TYPES_SW_MINOR_VERSION              0
#define CSM_TYPES_SW_PATCH_VERSION              0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief CSM模块ID (用于Det)
 */
#define CSM_MODULE_ID                           0x70U

/**
 * @brief 最大密钥数量
 */
#define CSM_MAX_KEYS                            32U

/**
 * @brief 最大作业数量
 */
#define CSM_MAX_JOBS                            32U

/**
 * @brief 最大队列深度
 */
#define CSM_MAX_QUEUE_DEPTH                     8U

/**
 * @brief 最大密钥元素数量
 */
#define CSM_MAX_KEY_ELEMENTS                    8U

/**
 * @brief 算法输入/输出最大长度
 */
#define CSM_MAX_DATA_LENGTH                     256U

/**
 * @brief 密钥最大长度
 */
#define CSM_MAX_KEY_LENGTH                      64U

/**
 * @brief MAC最大长度
 */
#define CSM_MAX_MAC_LENGTH                      32U

/**
 * @brief 签名最大长度
 */
#define CSM_MAX_SIGNATURE_LENGTH                128U

/**
 * @brief 哈希最大长度 (SHA-512)
 */
#define CSM_MAX_HASH_LENGTH                     64U

/*==================================================================================================
*                                       错误码定义
==================================================================================================*/
/**
 * @brief CSM错误码
 */
#define CSM_E_NO_ERROR                          0x00U
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
#define CSM_E_HARDWARE_FAILURE                  0x10U
#define CSM_E_ENTROPY_EXHAUSTION                0x11U
#define CSM_E_KEY_WRITE_FAIL                    0x12U
#define CSM_E_KEY_READ_FAIL                     0x13U
#define CSM_E_KEY_EMPTY                         0x14U

/*==================================================================================================
*                                       算法家族定义
==================================================================================================*/
/**
 * @brief 加密算法家族
 */
typedef enum
{
    CSM_ALGOFAM_NOT_SET = 0,            /* 未设置 */
    CSM_ALGOFAM_SHA1,                   /* SHA-1 */
    CSM_ALGOFAM_SHA2_224,               /* SHA-224 */
    CSM_ALGOFAM_SHA2_256,               /* SHA-256 */
    CSM_ALGOFAM_SHA2_384,               /* SHA-384 */
    CSM_ALGOFAM_SHA2_512,               /* SHA-512 */
    CSM_ALGOFAM_SHA3_224,               /* SHA3-224 */
    CSM_ALGOFAM_SHA3_256,               /* SHA3-256 */
    CSM_ALGOFAM_SHA3_384,               /* SHA3-384 */
    CSM_ALGOFAM_SHA3_512,               /* SHA3-512 */
    CSM_ALGOFAM_AES,                    /* AES */
    CSM_ALGOFAM_DES,                    /* DES */
    CSM_ALGOFAM_3DES,                   /* 3DES */
    CSM_ALGOFAM_RSA,                    /* RSA */
    CSM_ALGOFAM_ECDSA,                  /* ECDSA */
    CSM_ALGOFAM_ECDH,                   /* ECDH */
    CSM_ALGOFAM_HMAC,                   /* HMAC */
    CSM_ALGOFAM_CMAC,                   /* CMAC */
    CSM_ALGOFAM_GCM,                    /* GCM */
    CSM_ALGOFAM_CCM,                    /* CCM */
    CSM_ALGOFAM_CHACHA20_POLY1305,      /* ChaCha20-Poly1305 */
    CSM_ALGOFAM_DRBG,                   /* DRBG */
    CSM_ALGOFAM_PADDING_PKCS7,          /* PKCS#7 Padding */
    CSM_ALGOFAM_PADDING_ONEWITHZEROS,   /* One with Zeros Padding */
    /* 国密算法族 (SM2/SM3 就绪框架, SM2-SM3 完整实现见 CSM 后端依赖说明):
     * 当前无 SM 后端 (mbedtls 无 SM2/SM3; S32K312 HSM/HSE 固件不支持),
     * 算法族枚举先行落地供配置/分发引用, 实际调用 fail-closed 返回
     * CSM_ERROR_ALGO_NOT_SUPPORTED。接入 GmSSL 或 SM 版 HSM 固件后启用。*/
    CSM_ALGOFAM_SM2,                    /* SM2 (国密椭圆曲线公钥密码算法) */
    CSM_ALGOFAM_SM3                     /* SM3 (国密密码杂凑算法, 256-bit) */
} Csm_AlgorithmFamilyType;

/**
 * @brief 算法模式
 */
typedef enum
{
    CSM_ALGOMODE_NOT_SET = 0,           /* 未设置 */
    CSM_ALGOMODE_ECB,                   /* 电子密码本模式 */
    CSM_ALGOMODE_CBC,                   /* 密码块链接模式 */
    CSM_ALGOMODE_CFB,                   /* 密文反馈模式 */
    CSM_ALGOMODE_OFB,                   /* 输出反馈模式 */
    CSM_ALGOMODE_CTR,                   /* 计数器模式 */
    CSM_ALGOMODE_GCM,                   /* GCM模式 */
    CSM_ALGOMODE_CCM,                   /* CCM模式 */
    CSM_ALGOMODE_XTS,                   /* XTS模式 */
    CSM_ALGOMODE_RSA_PKCS1_V15,         /* RSA PKCS#1 v1.5 */
    CSM_ALGOMODE_RSA_PSS,               /* RSA-PSS */
    CSM_ALGOMODE_RSA_OAEP               /* RSA-OAEP */
} Csm_AlgorithmModeType;

/**
 * @brief 算法分类
 */
typedef enum
{
    CSM_ALGOCLASS_NOT_SET = 0,          /* 未设置 */
    CSM_ALGOCLASS_HASH,                 /* 哈希 */
    CSM_ALGOCLASS_MAC,                  /* MAC */
    CSM_ALGOCLASS_CIPHER,               /* 对称加密 */
    CSM_ALGOCLASS_SIGNATURE,            /* 数字签名 */
    CSM_ALGOCLASS_RANDOM,               /* 随机数生成 */
    CSM_ALGOCLASS_KEYEXCHANGE,          /* 密钥交换 */
    CSM_ALGOCLASS_PADDING               /* 填充 */
} Csm_AlgorithmClassType;

/*==================================================================================================
*                                       密钥相关类型定义
==================================================================================================*/
/**
 * @brief 密钥状态
 */
typedef enum
{
    CSM_KEY_STATUS_INVALID = 0,         /* 无效 */
    CSM_KEY_STATUS_VALID,               /* 有效 */
    CSM_KEY_STATUS_EMPTY,               /* 空 */
    CSM_KEY_STATUS_UPDATE_IN_PROGRESS   /* 更新中 */
} Csm_KeyStatusType;

/**
 * @brief 密钥元素类型
 */
typedef enum
{
    CSM_KEY_ELEMENT_TYPE_SECRET = 0,    /* 密钥 */
    CSM_KEY_ELEMENT_TYPE_PUBLIC,        /* 公钥 */
    CSM_KEY_ELEMENT_TYPE_PRIVATE,       /* 私钥 */
    CSM_KEY_ELEMENT_TYPE_IV,            /* 初始化向量 */
    CSM_KEY_ELEMENT_TYPE_SALT,          /* 盐值 */
    CSM_KEY_ELEMENT_TYPE_SEED,          /* 种子 */
    CSM_KEY_ELEMENT_TYPE_LABEL,         /* 标签 */
    CSM_KEY_ELEMENT_TYPE_CONTEXT        /* 上下文 */
} Csm_KeyElementType;

/**
 * @brief 密钥使用权限
 */
typedef uint32 Csm_KeyUsageType;
#define CSM_KEY_USAGE_NONE                  0x00000000U
#define CSM_KEY_USAGE_ENCRYPT               0x00000001U
#define CSM_KEY_USAGE_DECRYPT               0x00000002U
#define CSM_KEY_USAGE_SIGN                  0x00000004U
#define CSM_KEY_USAGE_VERIFY                0x00000008U
#define CSM_KEY_USAGE_HASH                  0x00000010U
#define CSM_KEY_USAGE_MAC_GENERATE          0x00000020U
#define CSM_KEY_USAGE_MAC_VERIFY            0x00000040U
#define CSM_KEY_USAGE_KEY_EXCHANGE          0x00000080U
#define CSM_KEY_USAGE_DERIVE                0x00000100U
#define CSM_KEY_USAGE_RANDOM_GENERATE       0x00000200U

/**
 * @brief 密钥元素配置
 */
typedef struct
{
    uint32 elementId;                   /* 元素ID */
    Csm_KeyElementType elementType;     /* 元素类型 */
    uint32 maxLength;                   /* 最大长度 */
    boolean readAllowed;                /* 允许读取 */
    boolean writeAllowed;               /* 允许写入 */
    boolean partialAccessAllowed;       /* 允许部分访问 */
} Csm_KeyElementConfigType;

/**
 * @brief 密钥元素数据
 */
typedef struct
{
    uint8 data[CSM_MAX_KEY_LENGTH];     /* 数据 */
    uint32 length;                      /* 实际长度 */
    boolean valid;                      /* 数据有效标志 */
} Csm_KeyElementDataType;

/**
 * @brief 密钥配置
 */
typedef struct
{
    uint32 keyId;                       /* 密钥ID */
    Csm_KeyUsageType allowedUsage;      /* 允许的使用方式 */
    const Csm_KeyElementConfigType* elements; /* 元素配置数组 */
    uint8 numElements;                  /* 元素数量 */
    uint32 cryptoKeyType;               /* 密钥类型 */
} Csm_KeyConfigType;

/**
 * @brief 密钥运行时数据
 */
typedef struct
{
    uint32 keyId;                       /* 密钥ID */
    Csm_KeyStatusType status;           /* 状态 */
    Csm_KeyElementDataType elements[CSM_MAX_KEY_ELEMENTS]; /* 元素数据 */
    uint8 numElements;                  /* 实际元素数量 */
    uint32 referenceCount;              /* 引用计数 */
} Csm_KeyType;

/*==================================================================================================
*                                       作业相关类型定义
==================================================================================================*/
/**
 * @brief 作业状态
 */
typedef enum
{
    CSM_JOB_STATE_IDLE = 0,             /* 空闲 */
    CSM_JOB_STATE_QUEUED,               /* 已入队 */
    CSM_JOB_STATE_PROCESSING,           /* 处理中 */
    CSM_JOB_STATE_WAITING,              /* 等待 */
    CSM_JOB_STATE_RESULT_READY          /* 结果就绪 */
} Csm_JobStateType;

/**
 * @brief 作业优先级
 */
typedef enum
{
    CSM_JOB_PRIORITY_LOW = 0,           /* 低优先级 */
    CSM_JOB_PRIORITY_NORMAL,            /* 普通优先级 */
    CSM_JOB_PRIORITY_HIGH,              /* 高优先级 */
    CSM_JOB_PRIORITY_IMMEDIATE          /* 立即执行 */
} Csm_JobPriorityType;

/**
 * @brief 服务类型
 */
typedef enum
{
    CSM_SERVICE_NONE = 0,               /* 无 */
    CSM_SERVICE_HASH,                   /* 哈希计算 */
    CSM_SERVICE_MAC_GENERATE,           /* MAC生成 */
    CSM_SERVICE_MAC_VERIFY,             /* MAC验证 */
    CSM_SERVICE_ENCRYPT,                /* 加密 */
    CSM_SERVICE_DECRYPT,                /* 解密 */
    CSM_SERVICE_SIGNATURE_GENERATE,     /* 签名生成 */
    CSM_SERVICE_SIGNATURE_VERIFY,       /* 签名验证 */
    CSM_SERVICE_RANDOM_GENERATE,        /* 随机数生成 */
    CSM_SERVICE_KEY_GENERATE,           /* 密钥生成 */
    CSM_SERVICE_KEY_DERIVE,             /* 密钥派生 */
    CSM_SERVICE_KEY_EXCHANGE            /* 密钥交换 */
} Csm_ServiceType;

/**
 * @brief 算法配置
 */
typedef struct
{
    Csm_AlgorithmFamilyType family;     /* 算法家族 */
    Csm_AlgorithmModeType mode;         /* 算法模式 */
    Csm_AlgorithmClassType classType;   /* 算法分类 */
    uint32 keyLength;                   /* 密钥长度 (bits) */
    const void* secondaryFamily;        /* 辅助算法 (如HMAC中的哈希算法) */
} Csm_AlgorithmType;

/**
 * @brief 作业配置
 */
typedef struct
{
    uint32 jobId;                       /* 作业ID */
    Csm_ServiceType serviceType;        /* 服务类型 */
    Csm_JobPriorityType priority;       /* 优先级 */
    uint32 keyId;                       /* 关联的密钥ID */
    Csm_AlgorithmType algorithm;        /* 算法配置 */
    boolean asynchronous;               /* 异步执行 */
    uint32 callbackId;                  /* 回调ID */
} Csm_JobConfigType;

/**
 * @brief 作业运行时数据
 */
typedef struct
{
    uint32 jobId;                       /* 作业ID */
    Csm_JobStateType state;             /* 状态 */
    Csm_ServiceType service;            /* 当前服务 */
    Csm_AlgorithmType algorithm;        /* 算法配置 */
    uint32 keyId;                       /* 密钥ID */
    uint8 inputData[CSM_MAX_DATA_LENGTH];   /* 输入数据 */
    uint32 inputLength;                 /* 输入长度 */
    uint8 outputData[CSM_MAX_DATA_LENGTH];  /* 输出数据 */
    uint32 outputLength;                /* 输出长度 */
    uint32 resultLength;                /* 结果长度 */
    Std_ReturnType result;              /* 执行结果 */
    boolean verifyResult;               /* 验证结果 */
    uint32 timestamp;                   /* 时间戳 */
    void* callbackContext;              /* 回调上下文 */
} Csm_JobType;

/*==================================================================================================
*                                       队列类型定义
==================================================================================================*/
/**
 * @brief 队列项
 */
typedef struct
{
    uint32 jobId;                       /* 作业ID */
    Csm_JobPriorityType priority;       /* 优先级 */
    uint32 timestamp;                   /* 入队时间戳 */
} Csm_QueueItemType;

/**
 * @brief 服务队列
 */
typedef struct
{
    Csm_QueueItemType items[CSM_MAX_QUEUE_DEPTH];   /* 队列项 */
    uint8 head;                         /* 队头索引 */
    uint8 tail;                         /* 队尾索引 */
    uint8 count;                        /* 当前数量 */
} Csm_QueueType;

/*==================================================================================================
*                                       回调类型定义
==================================================================================================*/
/**
 * @brief 作业完成回调函数类型
 */
typedef void (*Csm_CallbackType)(
    uint32 jobId,                       /* 作业ID */
    Std_ReturnType result,              /* 执行结果 */
    const uint8* outputPtr,             /* 输出数据指针 */
    uint32 outputLength,                /* 输出长度 */
    void* userContext                   /* 用户上下文 */
);

/**
 * @brief 密钥状态变更回调函数类型
 */
typedef void (*Csm_KeyCallbackType)(
    uint32 keyId,                       /* 密钥ID */
    Csm_KeyStatusType newStatus,        /* 新状态 */
    void* userContext                   /* 用户上下文 */
);

/*==================================================================================================
*                                       结果类型定义
==================================================================================================*/
/**
 * @brief 操作结果
 */
typedef struct
{
    Std_ReturnType result;              /* 结果状态 */
    uint32 length;                      /* 输出长度 */
    boolean verifyOK;                   /* 验证通过 */
} Csm_ResultType;

/**
 * @brief CSM全局配置
 */
typedef struct
{
    const Csm_KeyConfigType* keys;      /* 密钥配置数组 */
    uint8 numKeys;                      /* 密钥数量 */
    const Csm_JobConfigType* jobs;      /* 作业配置数组 */
    uint8 numJobs;                      /* 作业数量 */
    boolean useAsyncMode;               /* 使用异步模式 */
    uint32 queueProcessingPeriod;       /* 队列处理周期 (ms) */
    boolean devErrorDetect;             /* 开发错误检测 */
} Csm_ConfigType;


/*==================================================================================================
 *                          SECOC COMPATIBILITY DEFINITIONS
 *================================================================================================*/
#ifndef CSM_JOB_ID_MAC_GENERATE_1
#define CSM_JOB_ID_MAC_GENERATE_1               (1U)
#endif
#ifndef CSM_JOB_ID_MAC_VERIFY_1
#define CSM_JOB_ID_MAC_VERIFY_1                 (2U)
#endif
#ifndef CSM_OPERATIONMODE_STREAMSTART
#define CSM_OPERATIONMODE_STREAMSTART           (0x01U)
#endif

/* MAC verification result */
typedef uint8 Csm_VerifyResultType;
#ifndef CSM_E_VER_OK
#define CSM_E_VER_OK                            (0U)
#endif
#ifndef CSM_E_VER_NOT_OK
#define CSM_E_VER_NOT_OK                        (1U)
#endif
#endif /* CSM_TYPES_H */
