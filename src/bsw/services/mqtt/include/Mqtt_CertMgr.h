/** @file Mqtt_CertMgr.h
 * @brief MQTT证书管理模块头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 提供证书存储、加载、验证和更新功能
 * 支持多种存储介质(NVM、RAM、文件系统)
 */

#ifndef MQTT_CERTMGR_H
#define MQTT_CERTMGR_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt_Tls.h"

/*============================================================================
 * 证书存储介质
 *===========================================================================*/
typedef enum {
    MQTT_CERT_STORAGE_RAM = 0,      /**< RAM存储(不持久) */
    MQTT_CERT_STORAGE_NVM,          /**< 非易失性存储 */
    MQTT_CERT_STORAGE_FILESYSTEM,   /**< 文件系统 */
    MQTT_CERT_STORAGE_SECURE_ELEMENT /**< 安全芯片 */
} Mqtt_CertStorageType;

/*============================================================================
 * 证书类型
 *===========================================================================*/
typedef enum {
    MQTT_CERT_TYPE_CA_ROOT = 0,     /**< 根CA证书 */
    MQTT_CERT_TYPE_CA_INTERMEDIATE, /**< 中间CA证书 */
    MQTT_CERT_TYPE_SERVER,          /**< 服务器证书 */
    MQTT_CERT_TYPE_CLIENT,          /**< 客户端证书 */
    MQTT_CERT_TYPE_SELF_SIGNED      /**< 自签名证书 */
} Mqtt_CertType;

/*============================================================================
 * 证书库配置
 *===========================================================================*/
#define MQTT_CERTMGR_MAX_CERTS          (8U)
#define MQTT_CERTMGR_MAX_CERT_SIZE      (4096U)
#define MQTT_CERTMGR_MAX_CHAIN_DEPTH    (4U)

/*============================================================================
 * 证书元数据结构
 *===========================================================================*/

typedef struct {
    char commonName[64];            /**< 通用名称 */
    char organization[64];          /**< 组织名称 */
    char organizationalUnit[64];    /**< 组织单位 */
    char country[4];                /**< 国家代码 */
    char state[32];                 /**< 省/州 */
    char locality[32];              /**< 城市 */
    char email[64];                 /**< 电子邮件 */
} Mqtt_CertSubjectType;

typedef struct {
    uint32 notBefore;               /**< 生效时间(Unix时间戳) */
    uint32 notAfter;                /**< 过期时间(Unix时间戳) */
    uint32 serialNumber;            /**< 序列号 */
    char issuerKeyId[64];           /**< 颁发者密钥ID */
    char subjectKeyId[64];          /**< 主体密钥ID */
    boolean isCA;                   /**< 是否为CA证书 */
    uint8 keyUsage;                 /**< 密钥用途位图 */
    uint8 extKeyUsage;              /**< 扩展密钥用途 */
} Mqtt_CertExtensionsType;

typedef struct {
    Mqtt_CertType type;                     /**< 证书类型 */
    char alias[32];                         /**< 证书别名 */
    Mqtt_CertSubjectType subject;           /**< 主体信息 */
    Mqtt_CertSubjectType issuer;            /**< 颁发者信息 */
    Mqtt_CertExtensionsType extensions;     /**< 扩展信息 */
    uint8* data;                            /**< 证书数据指针 */
    uint32 dataLen;                         /**< 数据长度 */
    Mqtt_CertStorageType storage;           /**< 存储介质 */
    boolean isLoaded;                       /**< 是否已加载 */
} Mqtt_CertEntryType;

/*============================================================================
 * 证书验证结果
 *===========================================================================*/
typedef enum {
    MQTT_CERT_STATUS_VALID = 0,             /**< 证书有效 */
    MQTT_CERT_STATUS_EXPIRED,               /**< 证书已过期 */
    MQTT_CERT_STATUS_NOT_YET_VALID,         /**< 证书尚未生效 */
    MQTT_CERT_STATUS_REVOKED,               /**< 证书已撤销 */
    MQTT_CERT_STATUS_SELF_SIGNED,           /**< 自签名证书 */
    MQTT_CERT_STATUS_CHAIN_BROKEN,          /**< 证书链断裂 */
    MQTT_CERT_STATUS_UNTRUSTED,             /**< 证书不可信 */
    MQTT_CERT_STATUS_INVALID_FORMAT,        /**< 格式无效 */
    MQTT_CERT_STATUS_SIGNATURE_FAILED       /**< 签名验证失败 */
} Mqtt_CertStatusType;

/*============================================================================
 * 证书管理配置
 *===========================================================================*/
typedef struct {
    boolean autoReload;                     /**< 自动重新加载 */
    uint32 checkIntervalMs;                 /**< 证书检查间隔(毫秒) */
    boolean strictValidation;               /**< 严格验证模式 */
    uint32 expiryWarningDays;               /**< 过期预警天数 */
} Mqtt_CertMgrConfigType;

/*============================================================================
 * 回调函数类型
 *===========================================================================*/
typedef void (*Mqtt_CertExpiryCallbackType)(
    const char* alias,
    uint32 daysUntilExpiry
);

typedef void (*Mqtt_CertReloadCallbackType)(
    const char* alias,
    Mqtt_ReturnType result
);

/*============================================================================
 * 初始化和配置
 *===========================================================================*/

/**
 * @brief 初始化证书管理器
 * @param config 证书管理配置
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_Init(
    const Mqtt_CertMgrConfigType* config
);

/**
 * @brief 反初始化证书管理器
 */
extern void Mqtt_CertMgr_DeInit(void);

/*============================================================================
 * 证书存储操作
 *===========================================================================*/

/**
 * @brief 添加证书到存储
 * @param entry 证书条目信息(数据将被复制)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_AddCert(
    const Mqtt_CertEntryType* entry
);

/**
 * @brief 从存储中删除证书
 * @param alias 证书别名
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_RemoveCert(
    const char* alias
);

/**
 * @brief 获取证书信息
 * @param alias 证书别名
 * @param entry 输出缓冲区
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_GetCert(
    const char* alias,
    Mqtt_CertEntryType* entry
);

/**
 * @brief 更新存储中的证书
 * @param alias 证书别名
 * @param newData 新证书数据
 * @param dataLen 数据长度
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_UpdateCert(
    const char* alias,
    const uint8* newData,
    uint32 dataLen
);

/*============================================================================
 * 证书加载功能
 *===========================================================================*/

/**
 * @brief 为TLS连接加载证书
 * @param alias 证书别名
 * @param cert 输出的证书结构(用于TLS配置)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_LoadCertForTls(
    const char* alias,
    Mqtt_CertificateType* cert
);

/**
 * @brief 为TLS连接加载私钥
 * @param alias 私钥别名
 * @param password 私钥密码(如有)
 * @param key 输出的私钥结构
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_LoadKeyForTls(
    const char* alias,
    const char* password,
    Mqtt_PrivateKeyType* key
);

/**
 * @brief 为TLS连接加载CA证书库
 * @param caAlias CA证书别名(支持通配符*)
 * @param trustStore 输出的可信证书库结构
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_LoadTrustStore(
    const char* caAlias,
    Mqtt_TrustStoreType* trustStore
);

/*============================================================================
 * 证书验证功能
 *===========================================================================*/

/**
 * @brief 验证证书有效性
 * @param alias 证书别名
 * @param status 验证状态(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_ValidateCert(
    const char* alias,
    Mqtt_CertStatusType* status
);

/**
 * @brief 验证证书链
 * @param certAlias 服务器/客户端证书别名
 * @param caAlias CA证书别名
 * @param isValid 是否有效(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_ValidateCertChain(
    const char* certAlias,
    const char* caAlias,
    boolean* isValid
);

/**
 * @brief 检查证书是否将过期
 * @param alias 证书别名
 * @param warningDays 预警天数
 * @param isExpiringSoon 是否即将过期(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_CheckExpiry(
    const char* alias,
    uint32 warningDays,
    boolean* isExpiringSoon
);

/*============================================================================
 * 证书列表操作
 *===========================================================================*/

/**
 * @brief 获取存储中的证书数量
 * @return 证书数量
 */
extern uint8 Mqtt_CertMgr_GetCertCount(void);

/**
 * @brief 通过索引获取证书别名
 * @param index 索引
 * @param alias 输出缓冲区
 * @param aliasSize 缓冲区大小
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_GetCertAliasByIndex(
    uint8 index,
    char* alias,
    uint32 aliasSize
);

/**
 * @brief 查找符合条件的证书
 * @param type 证书类型
 * @param alias 输出缓冲区
 * @param aliasSize 缓冲区大小
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_FindCertByType(
    Mqtt_CertType type,
    char* alias,
    uint32 aliasSize
);

/*============================================================================
 * 自动管理功能
 *===========================================================================*/

/**
 * @brief 设置证书过期回调
 * @param callback 过期预警回调函数
 */
extern void Mqtt_CertMgr_SetExpiryCallback(
    Mqtt_CertExpiryCallbackType callback
);

/**
 * @brief 设置证书重新加载回调
 * @param callback 重新加载回调函数
 */
extern void Mqtt_CertMgr_SetReloadCallback(
    Mqtt_CertReloadCallbackType callback
);

/**
 * @brief 运行证书管理任务(检查过期等)
 * 应在主循环中定期调用
 */
extern void Mqtt_CertMgr_MainFunction(void);

/*============================================================================
 * 证书导入导出
 *===========================================================================*/

/**
 * @brief 从PEM格式字符串导入证书
 * @param pemData PEM格式数据
 * @param alias 证书别名
 * @param type 证书类型
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_ImportFromPem(
    const char* pemData,
    const char* alias,
    Mqtt_CertType type
);

/**
 * @brief 导出证书为PEM格式
 * @param alias 证书别名
 * @param pemBuffer 输出缓冲区
 * @param bufferSize 缓冲区大小
 * @param writtenSize 实际写入长度(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_CertMgr_ExportToPem(
    const char* alias,
    char* pemBuffer,
    uint32 bufferSize,
    uint32* writtenSize
);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_CERTMGR_H */
