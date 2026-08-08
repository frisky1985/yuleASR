/**********************************************************************************************************************
 * @file       CryptoStack_Types.h
 * @brief      Crypto Stack 公共类型定义头文件
 *
 * 功能: 定义CSM, CRYIF, Crypto各层共享的数据类型、枚举和结构体
 * 符合AUTOSAR 4.7.0标准 - Crypto Stack规范
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef CRYPTOSTACK_TYPES_H
#define CRYPTOSTACK_TYPES_H

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define CRYPTOSTACK_TYPES_VENDOR_ID                    0x2025U  /* YuleTech */
#define CRYPTOSTACK_TYPES_MODULE_ID                    0xF0U    /* Crypto Stack Common */
#define CRYPTOSTACK_TYPES_AR_RELEASE_MAJOR_VERSION     4U
#define CRYPTOSTACK_TYPES_AR_RELEASE_MINOR_VERSION     7U
#define CRYPTOSTACK_TYPES_AR_RELEASE_REVISION_VERSION  0U
#define CRYPTOSTACK_TYPES_SW_MAJOR_VERSION             1U
#define CRYPTOSTACK_TYPES_SW_MINOR_VERSION             0U
#define CRYPTOSTACK_TYPES_SW_PATCH_VERSION             0U

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS - 通用限制
 *********************************************************************************************************************/
#define CRYPTO_STACK_MAX_JOBS                           32U
#define CRYPTO_STACK_MAX_KEYS                           64U
#define CRYPTO_STACK_MAX_KEY_ELEMENTS                   16U
#define CRYPTO_STACK_MAX_QUEUE_SIZE                     16U
#define CRYPTO_STACK_MAX_DATA_LENGTH                    1024U
#define CRYPTO_STACK_MAX_KEY_LENGTH                     128U
#define CRYPTO_STACK_MAX_IV_LENGTH                      16U
#define CRYPTO_STACK_MAX_SALT_LENGTH                    32U
#define CRYPTO_STACK_MAX_TAG_LENGTH                     16U
#define CRYPTO_STACK_MAX_HASH_LENGTH                    64U   /* SHA-512 */
#define CRYPTO_STACK_MAX_SIGNATURE_LENGTH               144U  /* RSA-4096 or ECDSA P-521 */
#define CRYPTO_STACK_MAX_RANDOM_LENGTH                  256U
#define CRYPTO_STACK_MAX_ECC_CURVE_LENGTH               66U   /* P-521 */

/**********************************************************************************************************************
 * ALGORITHM FAMILY DEFINITIONS (算法家族)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_ALGOFAM_NOT_SET = 0,         /* 未设置 */
    /* Hash算法 */
    CRYPTO_ALGOFAM_SHA1,                /* SHA-1 (160-bit) */
    CRYPTO_ALGOFAM_SHA2_224,            /* SHA-224 */
    CRYPTO_ALGOFAM_SHA2_256,            /* SHA-256 */
    CRYPTO_ALGOFAM_SHA2_384,            /* SHA-384 */
    CRYPTO_ALGOFAM_SHA2_512,            /* SHA-512 */
    CRYPTO_ALGOFAM_SHA3_224,            /* SHA3-224 */
    CRYPTO_ALGOFAM_SHA3_256,            /* SHA3-256 */
    CRYPTO_ALGOFAM_SHA3_384,            /* SHA3-384 */
    CRYPTO_ALGOFAM_SHA3_512,            /* SHA3-512 */
    CRYPTO_ALGOFAM_SHA2_512_224,        /* SHA-512/224 */
    CRYPTO_ALGOFAM_SHA2_512_256,        /* SHA-512/256 */
    CRYPTO_ALGOFAM_SHAKE128,            /* SHAKE128 */
    CRYPTO_ALGOFAM_SHAKE256,            /* SHAKE256 */
    /* BLAKE2哈希算法 */
    CRYPTO_ALGOFAM_BLAKE2B,             /* BLAKE2b (64-bit, max 512-bit) */
    CRYPTO_ALGOFAM_BLAKE2S,             /* BLAKE2s (32-bit, max 256-bit) */
    CRYPTO_ALGOFAM_BLAKE2BP,            /* BLAKE2b parallel */
    CRYPTO_ALGOFAM_BLAKE2SP,            /* BLAKE2s parallel */
    /* 对称加密 */
    CRYPTO_ALGOFAM_AES,                 /* AES (128/192/256-bit) */
    CRYPTO_ALGOFAM_DES,                 /* DES (56-bit) */
    CRYPTO_ALGOFAM_3DES,                /* Triple DES (112/168-bit) */
    CRYPTO_ALGOFAM_CHACHA20,            /* ChaCha20 */
    /* 非对称加密 */
    CRYPTO_ALGOFAM_RSA,                 /* RSA */
    CRYPTO_ALGOFAM_ECC,                 /* 椭圆曲线密码 */
    CRYPTO_ALGOFAM_ECCSEC_P256,         /* ECC SEC P-256 (NIST P-256) */
    /* 数字签名 */
    CRYPTO_ALGOFAM_ECDSA,               /* ECDSA */
    CRYPTO_ALGOFAM_EDDSA,               /* EdDSA (Ed25519/Ed448) */
    /* 密钥交换 */
    CRYPTO_ALGOFAM_ECDH,                /* ECDH 密钥协商 */
    CRYPTO_ALGOFAM_ECIES,               /* ECIES 加密方案 */
    /* MAC算法 */
    CRYPTO_ALGOFAM_HMAC,                /* HMAC */
    CRYPTO_ALGOFAM_CMAC,                /* CMAC (AES-CMAC) */
    CRYPTO_ALGOFAM_GMAC,                /* GMAC (AES-GMAC) */
    CRYPTO_ALGOFAM_POLY1305,            /* Poly1305 */
    /* 认证加密模式 */
    CRYPTO_ALGOFAM_GCM,                 /* Galois/Counter Mode */
    CRYPTO_ALGOFAM_CCM,                 /* Counter with CBC-MAC */
    CRYPTO_ALGOFAM_CHACHA20_POLY1305,   /* AEAD模式 */
    /* 密钥派生 */
    CRYPTO_ALGOFAM_HKDF,                /* HKDF (RFC 5869) */
    CRYPTO_ALGOFAM_PBKDF2,              /* PBKDF2 */
    CRYPTO_ALGOFAM_SCRYPT,              /* scrypt */
    CRYPTO_ALGOFAM_ARGON2,              /* Argon2 */
    /* 填充模式 */
    CRYPTO_ALGOFAM_PADDING_PKCS7,       /* PKCS#7填充 */
    CRYPTO_ALGOFAM_PADDING_ONEWITHZEROS,/* One with Zeros填充 */
    CRYPTO_ALGOFAM_PADDING_PSS,         /* PSS填充 */
    CRYPTO_ALGOFAM_PADDING_OAEP,        /* OAEP填充 */
    /* 随机数生成 */
    CRYPTO_ALGOFAM_CTRDRBG,             /* CTR-DRBG */
    CRYPTO_ALGOFAM_HASHDRBG,            /* Hash-DRBG */
    CRYPTO_ALGOFAM_HMACDRBG             /* HMAC-DRBG */
} Crypto_AlgorithmFamilyType;

/**********************************************************************************************************************
 * ALGORITHM MODE DEFINITIONS (算法模式)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_ALGOMODE_NOT_SET = 0,        /* 未设置 */
    /* 分组密码模式 */
    CRYPTO_ALGOMODE_ECB,                /* 电子密码本模式 */
    CRYPTO_ALGOMODE_CBC,                /* 密码块链接模式 */
    CRYPTO_ALGOMODE_CFB,                /* 密文反馈模式 */
    CRYPTO_ALGOMODE_OFB,                /* 输出反馈模式 */
    CRYPTO_ALGOMODE_CTR,                /* 计数器模式 */
    /* 认证加密模式 */
    CRYPTO_ALGOMODE_GCM,                /* Galois/Counter Mode */
    CRYPTO_ALGOMODE_CCM,                /* Counter with CBC-MAC */
    CRYPTO_ALGOMODE_XTS,                /* XEX基于密文的密文盘模式 */
    /* RSA填充模式 */
    CRYPTO_ALGOMODE_RSA_PKCS1_V15,      /* RSA PKCS#1 v1.5 */
    CRYPTO_ALGOMODE_RSA_PSS,            /* RSA-PSS */
    CRYPTO_ALGOMODE_RSA_OAEP            /* RSA-OAEP */
} Crypto_AlgorithmModeType;

/**********************************************************************************************************************
 * ALGORITHM CLASSIFICATION (算法分类)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_ALGOCLASS_NOT_SET = 0,       /* 未设置 */
    CRYPTO_ALGOCLASS_HASH,              /* 哈希函数 */
    CRYPTO_ALGOCLASS_MAC,               /* 消息认证码 */
    CRYPTO_ALGOCLASS_CIPHER,            /* 对称加密 */
    CRYPTO_ALGOCLASS_PUBLIC_KEY,        /* 非对称加密/公钥加密 */
    CRYPTO_ALGOCLASS_SIGNATURE,         /* 数字签名 */
    CRYPTO_ALGOCLASS_RANDOM,            /* 随机数生成 */
    CRYPTO_ALGOCLASS_KEYEXCHANGE,       /* 密钥交换 */
    CRYPTO_ALGOCLASS_KEYDERIVE,         /* 密钥派生 */
    CRYPTO_ALGOCLASS_PADDING            /* 填充 */
} Crypto_AlgorithmClassType;

/**********************************************************************************************************************
 * OPERATION MODE DEFINITIONS (操作模式)
 *********************************************************************************************************************/
#define CRYPTO_OPERATIONMODE_START          0x01U   /* 启动阶段 */
#define CRYPTO_OPERATIONMODE_UPDATE         0x02U   /* 更新阶段 */
#define CRYPTO_OPERATIONMODE_STREAMSTART    0x03U   /* 流式启动 (START|UPDATE) */
#define CRYPTO_OPERATIONMODE_FINISH         0x04U   /* 完成阶段 */
#define CRYPTO_OPERATIONMODE_SINGLECALL     0x07U   /* 单次调用 (START|UPDATE|FINISH) */

/**********************************************************************************************************************
 * SERVICE TYPE DEFINITIONS (服务类型)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_SERVICE_NONE = 0,                    /* 无服务 */
    CRYPTO_SERVICE_HASH,                        /* 哈希计算 */
    CRYPTO_SERVICE_MACGENERATE,                 /* MAC生成 */
    CRYPTO_SERVICE_MACVERIFY,                   /* MAC验证 */
    CRYPTO_SERVICE_ENCRYPT,                     /* 加密 */
    CRYPTO_SERVICE_DECRYPT,                     /* 解密 */
    CRYPTO_SERVICE_AEADENCRYPT,                 /* AEAD加密 */
    CRYPTO_SERVICE_AEADDECRYPT,                 /* AEAD解密 */
    CRYPTO_SERVICE_SIGNATUREGENERATE,           /* 签名生成 */
    CRYPTO_SERVICE_SIGNATUREVERIFY,             /* 签名验证 */
    CRYPTO_SERVICE_RANDOMGENERATE,              /* 随机数生成 */
    CRYPTO_SERVICE_KEYGENERATE,                 /* 密钥生成 */
    CRYPTO_SERVICE_KEYDERIVE,                   /* 密钥派生 */
    CRYPTO_SERVICE_KEYEXCHANGECALCPUBVAL,       /* 密钥交换-计算公共值 */
    CRYPTO_SERVICE_KEYEXCHANGECALCSECRET        /* 密钥交换-计算共享秘密 */
} Crypto_ServiceType;

/**********************************************************************************************************************
 * ECC CURVE DEFINITIONS (椭圆曲线定义)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_ECC_CURVE_NONE = 0,          /* 无/不适用 */
    /* NIST曲线 */
    CRYPTO_ECC_CURVE_SECP256R1,         /* NIST P-256 (prime256v1) */
    CRYPTO_ECC_CURVE_SECP384R1,         /* NIST P-384 */
    CRYPTO_ECC_CURVE_SECP521R1,         /* NIST P-521 */
    CRYPTO_ECC_CURVE_SECP256K1,         /* secp256k1 (Bitcoin) */
    /* Brainpool曲线 */
    CRYPTO_ECC_CURVE_BRAINPOOLP256R1,   /* Brainpool P-256r1 */
    CRYPTO_ECC_CURVE_BRAINPOOLP384R1,   /* Brainpool P-384r1 */
    CRYPTO_ECC_CURVE_BRAINPOOLP512R1,   /* Brainpool P-512r1 */
    /* Edward曲线 */
    CRYPTO_ECC_CURVE_ED25519,           /* Curve25519 */
    CRYPTO_ECC_CURVE_ED448              /* Curve448 */
} Crypto_EccCurveType;

/**********************************************************************************************************************
 * KEY ELEMENT DEFINITIONS (密钥元素类型)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_KEYELEMENT_TYPE_SECRET = 0,      /* 秘密密钥材料 */
    CRYPTO_KEYELEMENT_TYPE_PUBLIC,          /* 公钥 */
    CRYPTO_KEYELEMENT_TYPE_PRIVATE,         /* 私钥 */
    CRYPTO_KEYELEMENT_TYPE_IV,              /* 初始化向量 */
    CRYPTO_KEYELEMENT_TYPE_NONCE,           /* 随机数 */
    CRYPTO_KEYELEMENT_TYPE_SALT,            /* 盐值 */
    CRYPTO_KEYELEMENT_TYPE_SEED,            /* 种子 */
    CRYPTO_KEYELEMENT_TYPE_LABEL,           /* 标签 (HKDF等) */
    CRYPTO_KEYELEMENT_TYPE_CONTEXT,         /* 上下文信息 */
    CRYPTO_KEYELEMENT_TYPE_TAG,             /* 认证标签 */
    CRYPTO_KEYELEMENT_TYPE_AUTH_DATA,       /* 附加认证数据 (AAD) */
    CRYPTO_KEYELEMENT_TYPE_DIGEST,          /* 摘要值 */
    CRYPTO_KEYELEMENT_TYPE_SIGNATURE        /* 签名值 */
} Crypto_KeyElementType;

/* 标准密钥元素ID */
#define CRYPTO_KEYELEMENT_IV                    0x01U
#define CRYPTO_KEYELEMENT_SALT                  0x02U
#define CRYPTO_KEYELEMENT_SEED                  0x03U
#define CRYPTO_KEYELEMENT_DIGEST                0x04U
#define CRYPTO_KEYELEMENT_TAG                   0x05U
#define CRYPTO_KEYELEMENT_NONCE                 0x06U
#define CRYPTO_KEYELEMENT_AAD                   0x07U
#define CRYPTO_KEYELEMENT_KDF_INFO              0x10U
#define CRYPTO_KEYELEMENT_KDF_SALT              0x11U
#define CRYPTO_KEYELEMENT_KEY                   0x20U
#define CRYPTO_KEYELEMENT_PUBLIC_KEY            0x21U
#define CRYPTO_KEYELEMENT_PRIVATE_KEY           0x22U
#define CRYPTO_KEYELEMENT_SHARED_SECRET         0x23U
#define CRYPTO_KEYELEMENT_SIGNATURE             0x30U

/**********************************************************************************************************************
 * KEY STATUS DEFINITIONS (密钥状态)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_KEYSTATUS_INVALID = 0,       /* 密钥无效 */
    CRYPTO_KEYSTATUS_VALID,             /* 密钥有效 */
    CRYPTO_KEYSTATUS_EMPTY,             /* 密钥为空 */
    CRYPTO_KEYSTATUS_UPDATE_IN_PROGRESS,/* 密钥正在更新 */
    CRYPTO_KEYSTATUS_MARKED_FOR_DELETION/* 密钥已标记删除 */
} Crypto_KeyStatusType;

/**********************************************************************************************************************
 * KEY USAGE FLAGS (密钥使用权限)
 *********************************************************************************************************************/
typedef uint32 Crypto_KeyUsageType;
#define CRYPTO_KEYUSAGE_NONE                    0x00000000U
#define CRYPTO_KEYUSAGE_ENCRYPT                 0x00000001U  /* 允许加密 */
#define CRYPTO_KEYUSAGE_DECRYPT                 0x00000002U  /* 允许解密 */
#define CRYPTO_KEYUSAGE_SIGN                    0x00000004U  /* 允许签名 */
#define CRYPTO_KEYUSAGE_VERIFY                  0x00000008U  /* 允许验证 */
#define CRYPTO_KEYUSAGE_HASH                    0x00000010U  /* 允许哈希 */
#define CRYPTO_KEYUSAGE_MAC_GENERATE            0x00000020U  /* 允许MAC生成 */
#define CRYPTO_KEYUSAGE_MAC_VERIFY              0x00000040U  /* 允许MAC验证 */
#define CRYPTO_KEYUSAGE_KEY_EXCHANGE            0x00000080U  /* 允许密钥交换 */
#define CRYPTO_KEYUSAGE_KEY_DERIVE              0x00000100U  /* 允许密钥派生 */
#define CRYPTO_KEYUSAGE_RANDOM_GENERATE         0x00000200U  /* 允许随机数生成 */
#define CRYPTO_KEYUSAGE_AGREEMENT               0x00000400U  /* 允许密钥协商 */
#define CRYPTO_KEYUSAGE_KEY_GENERATE            0x00000800U  /* 允许密钥生成 */
#define CRYPTO_KEYUSAGE_KEY_IMPORT              0x00001000U  /* 允许密钥导入 */
#define CRYPTO_KEYUSAGE_KEY_EXPORT              0x00002000U  /* 允许密钥导出 */

/**********************************************************************************************************************
 * JOB STATE DEFINITIONS (作业状态)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_JOBSTATE_IDLE = 0,           /* 空闲状态 */
    CRYPTO_JOBSTATE_QUEUED,             /* 已入队 */
    CRYPTO_JOBSTATE_PROCESSING,         /* 正在处理 */
    CRYPTO_JOBSTATE_WAITING,            /* 等待资源 */
    CRYPTO_JOBSTATE_RESULT_READY,       /* 结果就绪 */
    CRYPTO_JOBSTATE_CANCELED            /* 已取消 */
} Crypto_JobStateType;

/**********************************************************************************************************************
 * PROCESSING TYPE DEFINITIONS (处理类型)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_PROCESSING_ASYNC = 0,        /* 异步处理 */
    CRYPTO_PROCESSING_SYNC              /* 同步处理 */
} Crypto_ProcessingType;

/**********************************************************************************************************************
 * RESULT TYPE DEFINITIONS (结果类型)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_OPRESULT_OK = 0,             /* 操作成功 */
    CRYPTO_OPRESULT_NOT_OK,             /* 操作失败 */
    CRYPTO_OPRESULT_BUSY,               /* 正在忙碌 */
    CRYPTO_OPRESULT_BUSY_RETRY_LATER,   /* 忙碌，稍后重试 */
    CRYPTO_OPRESULT_CANCELLED,          /* 已取消 */
    CRYPTO_OPRESULT_ENTROPY_EXHAUSTED   /* 熵耗尽 */
} Crypto_OperationResultType;

/* T3 fix (2026-08-08): Crypto_OperationModeType was referenced by the
 * blake2 AUTOSAR integration (blake2_autosar.c) but defined only in the
 * MCAL Crypto_Types.h, whose CRYPTO_ALGOFAM_* macros collide with the
 * enum constants below. Defined here so the classic crypto-stack headers
 * are self-contained. */
typedef uint32 Crypto_OperationModeType;

typedef enum {
    CRYPTO_VERIFY_FAILED = 0,           /* 验证失败 */
    CRYPTO_VERIFY_PASSED,               /* 验证通过 */
    CRYPTO_VERIFY_IN_PROGRESS,          /* 验证进行中 */
    CRYPTO_VERIFY_NOT_OK                /* 验证未完成 */
} Crypto_VerifyResultType;

/**********************************************************************************************************************
 * CALLBACK TYPE DEFINITIONS (回调函数类型)
 *********************************************************************************************************************/
typedef void (*Crypto_JobCallbackType)(
    uint32 jobId,                       /* 作业ID */
    Crypto_OperationResultType result,  /* 操作结果 */
    void* userData                      /* 用户数据 */
);

typedef void (*Crypto_KeyCallbackType)(
    uint32 keyId,                       /* 密钥ID */
    Crypto_KeyStatusType status,        /* 密钥状态 */
    void* userData                      /* 用户数据 */
);

/**********************************************************************************************************************
 * BASIC TYPEDEFS (基础类型别名)
 *********************************************************************************************************************/
typedef uint32 Crypto_JobIdType;
typedef uint32 Crypto_KeyIdType;
typedef uint32 Crypto_KeyElementIdType;
typedef uint32 Crypto_QueueIdType;
typedef uint32 Crypto_ChannelIdType;
typedef sint8  Crypto_JobPriorityType;

/**********************************************************************************************************************
 * ALGORITHM INFORMATION TYPE (算法信息)
 *********************************************************************************************************************/
typedef struct {
    Crypto_AlgorithmFamilyType  family;         /* 算法家族 */
    Crypto_AlgorithmModeType    mode;           /* 算法模式 */
    Crypto_AlgorithmClassType   classType;      /* 算法分类 */
    uint32                      keyLength;      /* 密钥长度 (比特) */
    Crypto_EccCurveType         curve;          /* ECC曲线 (如果适用) */
    Crypto_AlgorithmFamilyType  secondaryFamily;/* 辅助算法 (如HMAC中的哈希) */
} Crypto_AlgorithmInfoType;

/**********************************************************************************************************************
 * KEY ELEMENT CONFIGURATION TYPE (密钥元素配置)
 *********************************************************************************************************************/
typedef struct {
    Crypto_KeyElementIdType     id;                 /* 元素ID */
    Crypto_KeyElementType       type;               /* 元素类型 */
    uint32                      maxLength;          /* 最大长度 */
    boolean                     readAccess;         /* 读访问 */
    boolean                     writeAccess;        /* 写访问 */
    boolean                     partialAccess;      /* 部分访问 */
} Crypto_KeyElementConfigType;

/**********************************************************************************************************************
 * KEY ELEMENT RUNTIME TYPE (密钥元素运行时数据)
 *********************************************************************************************************************/
typedef struct {
    uint8*                      dataPtr;            /* 数据指针 */
    uint32                      dataLength;         /* 数据长度 */
    uint32                      maxLength;          /* 最大长度 */
    boolean                     valid;              /* 数据有效标志 */
} Crypto_KeyElementDataType;

/**********************************************************************************************************************
 * KEY CONFIGURATION TYPE (密钥配置)
 *********************************************************************************************************************/
typedef struct {
    Crypto_KeyIdType                keyId;              /* 密钥ID */
    Crypto_KeyUsageType             allowedUsage;       /* 允许的使用方式 */
    const Crypto_KeyElementConfigType* elements;        /* 元素配置数组 */
    uint8                           numElements;        /* 元素数量 */
    Crypto_ProcessingType           processingType;     /* 处理类型 */
} Crypto_KeyConfigType;

/**********************************************************************************************************************
 * KEY RUNTIME TYPE (密钥运行时数据)
 *********************************************************************************************************************/
typedef struct {
    Crypto_KeyIdType            keyId;              /* 密钥ID */
    Crypto_KeyStatusType        status;             /* 状态 */
    Crypto_KeyElementDataType   elements[CRYPTO_STACK_MAX_KEY_ELEMENTS]; /* 元素数据 */
    uint8                       numElements;        /* 实际元素数量 */
    uint32                      referenceCount;     /* 引用计数 */
} Crypto_KeyType;

/**********************************************************************************************************************
 * JOB PRIMITIVE INFO TYPE (作业原语信息)
 *********************************************************************************************************************/
typedef struct {
    uint32                      callbackId;                 /* 回调ID */
    Crypto_AlgorithmInfoType    algorithm;                  /* 算法信息 */
    Crypto_ServiceType          service;                    /* 服务类型 */
    Crypto_ProcessingType       processingType;             /* 处理类型 */
    boolean                     callbackUpdateNotification; /* 回调更新通知 */
} Crypto_JobPrimitiveInfoType;

/**********************************************************************************************************************
 * JOB INFO TYPE (作业信息)
 *********************************************************************************************************************/
typedef struct {
    Crypto_JobIdType            jobId;              /* 作业ID */
    Crypto_JobPriorityType      priority;           /* 优先级 */
} Crypto_JobInfoType;

/**********************************************************************************************************************
 * JOB PRIMITIVE INPUT/OUTPUT TYPE (作业原语输入输出)
 *********************************************************************************************************************/
typedef struct {
    /* 输入 */
    const uint8*                inputPtr;               /* 主输入缓冲区 */
    uint32                      inputLength;            /* 主输入长度 */
    const uint8*                secondaryInputPtr;      /* 次级输入缓冲区 */
    uint32                      secondaryInputLength;   /* 次级输入长度 */
    const uint8*                tertiaryInputPtr;       /* 三级输入缓冲区 */
    uint32                      tertiaryInputLength;    /* 三级输入长度 */

    /* 输出 */
    uint8*                      outputPtr;              /* 主输出缓冲区 */
    uint32*                     outputLengthPtr;        /* 主输出长度指针 */
    uint8*                      secondaryOutputPtr;     /* 次级输出缓冲区 */
    uint32*                     secondaryOutputLengthPtr; /* 次级输出长度指针 */

    /* 验证结果 */
    Crypto_VerifyResultType*    verifyPtr;              /* 验证结果指针 */
} Crypto_JobPrimitiveInputOutputType;

/**********************************************************************************************************************
 * JOB REDIRECTION INFO TYPE (作业重定向信息)
 *********************************************************************************************************************/
typedef struct {
    uint32                      inputKeyId;             /* 输入密钥ID */
    uint32                      secondaryInputKeyId;    /* 次级输入密钥ID */
    uint32                      tertiaryInputKeyId;     /* 三级输入密钥ID */
    uint32                      outputKeyId;            /* 输出密钥ID */
    uint32                      secondaryOutputKeyId;   /* 次级输出密钥ID */
} Crypto_JobRedirectionInfoType;

/**********************************************************************************************************************
 * CRYPTO JOB TYPE (核心密码作业结构体)
 *
 * 这是Crypto Stack中最重要的结构体，用于在CSM-CRYIF-Crypto层之间传递密码请求
 *********************************************************************************************************************/
typedef struct {
    uint32                              jobId;                      /* 作业ID */
    Crypto_JobStateType                 jobState;                   /* 作业状态 */
    Crypto_JobPrimitiveInputOutputType* jobPrimitiveInputOutput;    /* 输入输出 */
    const Crypto_JobPrimitiveInfoType*  jobPrimitiveInfo;           /* 原语信息 */
    const Crypto_JobInfoType*           jobInfo;                    /* 作业信息 */
    Crypto_KeyIdType                    cryptoKeyId;                /* 密钥ID */
    Crypto_KeyIdType                    targetCryptoKeyId;          /* 目标密钥ID (用于密钥派生/交换) */
    const Crypto_JobRedirectionInfoType* jobRedirectionInfoRef;     /* 重定向信息 */
} Crypto_JobType;

/**********************************************************************************************************************
 * QUEUE ELEMENT TYPE (队列元素)
 *********************************************************************************************************************/
typedef struct Crypto_QueueElementStruct {
    Crypto_JobType*                     jobPtr;         /* 作业指针 */
    struct Crypto_QueueElementStruct*   nextPtr;        /* 下一个元素 */
    uint32                              timestamp;      /* 入队时间戳 */
} Crypto_QueueElementType;

/**********************************************************************************************************************
 * QUEUE TYPE (队列结构)
 *********************************************************************************************************************/
typedef struct {
    Crypto_QueueElementType*    headPtr;            /* 队头 */
    Crypto_QueueElementType*    tailPtr;            /* 队尾 */
    uint8                       count;              /* 当前数量 */
    uint8                       maxCount;           /* 最大容量 */
} Crypto_QueueType;

/**********************************************************************************************************************
 * CRYPTO STACK CONFIGURATION TYPE (Crypto栈配置)
 *********************************************************************************************************************/
typedef struct {
    const Crypto_KeyConfigType*         keys;               /* 密钥配置数组 */
    uint8                               numKeys;            /* 密钥数量 */
    uint8                               numChannels;        /* 通道数量 */
    uint8                               queueSize;          /* 队列大小 */
    boolean                             devErrorDetect;     /* 开发错误检测 */
    boolean                             versionInfoApi;     /* 版本信息API */
} CryptoStack_ConfigType;

/**********************************************************************************************************************
 * RESULT TYPE (操作结果结构体)
 *********************************************************************************************************************/
typedef struct {
    Crypto_OperationResultType  operationResult;    /* 操作结果 */
    uint32                      outputLength;       /* 输出长度 */
    Crypto_VerifyResultType     verifyResult;       /* 验证结果 */
} Crypto_ResultType;

/**********************************************************************************************************************
 * CRYPTO STACK LAYER IDENTIFICATION (Crypto栈层标识)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_LAYER_CSM = 0,               /* CSM层 (Crypto Services Manager) */
    CRYPTO_LAYER_CRYIF,                 /* CRYIF层 (Crypto Interface) */
    CRYPTO_LAYER_CRYPTO                 /* Crypto层 (Crypto Driver) */
} Crypto_LayerType;

/**********************************************************************************************************************
 * ERROR TYPE DEFINITIONS (错误类型)
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_ERROR_NONE = 0,              /* 无错误 */
    CRYPTO_ERROR_GENERAL,               /* 通用错误 */
    CRYPTO_ERROR_NOT_INITIALIZED,       /* 未初始化 */
    CRYPTO_ERROR_ALREADY_INITIALIZED,   /* 已初始化 */
    CRYPTO_ERROR_PARAM_POINTER,         /* 指针参数错误 */
    CRYPTO_ERROR_PARAM_KEY_ID,          /* 密钥ID错误 */
    CRYPTO_ERROR_PARAM_KEY_ELEMENT_ID,  /* 密钥元素ID错误 */
    CRYPTO_ERROR_PARAM_JOB_ID,          /* 作业ID错误 */
    CRYPTO_ERROR_PARAM_ALGORITHM,       /* 算法参数错误 */
    CRYPTO_ERROR_PARAM_MODE,            /* 模式参数错误 */
    CRYPTO_ERROR_PARAM_LENGTH,          /* 长度参数错误 */
    CRYPTO_ERROR_KEY_NOT_AVAILABLE,     /* 密钥不可用 */
    CRYPTO_ERROR_KEY_NOT_VALID,         /* 密钥无效 */
    CRYPTO_ERROR_KEY_SIZE_MISMATCH,     /* 密钥大小不匹配 */
    CRYPTO_ERROR_KEY_EMPTY,             /* 密钥为空 */
    CRYPTO_ERROR_KEY_WRITE_FAIL,        /* 密钥写入失败 */
    CRYPTO_ERROR_KEY_READ_FAIL,         /* 密钥读取失败 */
    CRYPTO_ERROR_JOB_BUSY,              /* 作业忙碌 */
    CRYPTO_ERROR_QUEUE_FULL,            /* 队列满 */
    CRYPTO_ERROR_SERVICE_NOT_SUPPORTED, /* 服务不支持 */
    CRYPTO_ERROR_ENTROPY_EXHAUSTION,    /* 熵耗尽 */
    CRYPTO_ERROR_HARDWARE_FAILURE,      /* 硬件故障 */
    CRYPTO_ERROR_BUFFER_TOO_SMALL,      /* 缓冲区过小 */
    CRYPTO_ERROR_VERIFICATION_FAILED,   /* 验证失败 */
    CRYPTO_ERROR_CANCELATION_FAILED     /* 取消失败 */
} Crypto_ErrorType;

#endif /* CRYPTOSTACK_TYPES_H */


/*==================================================================================================
 *                                     AES MODE DEFINITIONS
 * Following definitions are added for complete AES algorithm support
==================================================================================================*/

/* AES Operation Modes */
#define CRYPTO_AES_MODE_ECB                 (0x01U)
#define CRYPTO_AES_MODE_CBC                 (0x02U)
#define CRYPTO_AES_MODE_CFB                 (0x03U)
#define CRYPTO_AES_MODE_OFB                 (0x04U)
#define CRYPTO_AES_MODE_CTR                 (0x05U)
#define CRYPTO_AES_MODE_GCM                 (0x06U)
#define CRYPTO_AES_MODE_CCM                 (0x07U)
#define CRYPTO_AES_MODE_XTS                 (0x08U)
#define CRYPTO_AES_MODE_KW                  (0x09U)

/* AES Key Lengths */
#define CRYPTO_AES_KEYSIZE_128              (128U)
#define CRYPTO_AES_KEYSIZE_192              (192U)
#define CRYPTO_AES_KEYSIZE_256              (256U)

/*==================================================================================================
 *                                     HASH ALGORITHM DEFINITIONS
==================================================================================================*/

/* Hash Algorithm Families */
#define CRYPTO_ALGOFAM_SHA1                 (0x10U)
#define CRYPTO_ALGOFAM_SHA224               (0x11U)
#define CRYPTO_ALGOFAM_SHA256               (0x12U)
#define CRYPTO_ALGOFAM_SHA384               (0x13U)
#define CRYPTO_ALGOFAM_SHA512               (0x14U)
#define CRYPTO_ALGOFAM_SHA3_224             (0x15U)
#define CRYPTO_ALGOFAM_SHA3_256             (0x16U)
#define CRYPTO_ALGOFAM_SHA3_384             (0x17U)
#define CRYPTO_ALGOFAM_SHA3_512             (0x18U)
#define CRYPTO_ALGOFAM_BLAKE2B              (0x20U)
#define CRYPTO_ALGOFAM_BLAKE2S              (0x21U)

/* Hash Digest Sizes */
#define CRYPTO_SHA1_DIGEST_SIZE             (20U)
#define CRYPTO_SHA224_DIGEST_SIZE           (28U)
#define CRYPTO_SHA256_DIGEST_SIZE           (32U)
#define CRYPTO_SHA384_DIGEST_SIZE           (48U)
#define CRYPTO_SHA512_DIGEST_SIZE           (64U)

/*==================================================================================================
 *                                     CIPHER CONFIGURATION
==================================================================================================*/

/* Cipher Padding Modes */
typedef enum {
    CRYPTO_PADDING_NONE = 0,                /* No padding */
    CRYPTO_PADDING_PKCS7,                   /* PKCS#7 padding */
    CRYPTO_PADDING_ZERO,                    /* Zero padding */
    CRYPTO_PADDING_ANSIX923,                /* ANSI X.923 padding */
    CRYPTO_PADDING_ISO10126                 /* ISO/IEC 10126 padding */
} Crypto_PaddingModeType;

/*==================================================================================================
 *                                     HASH CONFIGURATION
==================================================================================================*/

typedef enum {
    CRYPTO_HASH_MODE_SHA1 = 0,
    CRYPTO_HASH_MODE_SHA224,
    CRYPTO_HASH_MODE_SHA256,
    CRYPTO_HASH_MODE_SHA384,
    CRYPTO_HASH_MODE_SHA512,
    CRYPTO_HASH_MODE_SHA3_224,
    CRYPTO_HASH_MODE_SHA3_256,
    CRYPTO_HASH_MODE_SHA3_384,
    CRYPTO_HASH_MODE_SHA3_512,
    CRYPTO_HASH_MODE_BLAKE2B,
    CRYPTO_HASH_MODE_BLAKE2S
} Crypto_HashModeType;
