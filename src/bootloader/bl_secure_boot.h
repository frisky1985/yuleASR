/**
 * @file bl_secure_boot.h
 * @brief Secure Boot Verification Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 符合UNECE R156和ISO/SAE 21434要求的安全启动验证
 * 使用SecOC/CSM/KeyM进行安全验证
 * ASIL-D Safety Level
 */

#ifndef BL_SECURE_BOOT_H
#define BL_SECURE_BOOT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define BL_SECURE_BOOT_MAJOR_VERSION    1
#define BL_SECURE_BOOT_MINOR_VERSION    0
#define BL_SECURE_BOOT_PATCH_VERSION    0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define BL_SB_MAX_CERT_CHAIN_DEPTH      4U
#define BL_SB_MAX_CERT_SIZE             2048
#define BL_SB_SIGNATURE_SIZE            64      /* ECDSA P-256 */
#define BL_SB_HASH_SIZE                 32      /* SHA-256 */
#define BL_SB_MAX_FIRMWARE_SIZE         (256 * 1024 * 1024)

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    BL_SB_OK = 0,
    BL_SB_ERROR_INVALID_PARAM = -1,
    BL_SB_ERROR_NOT_INITIALIZED = -2,
    BL_SB_ERROR_NO_MEMORY = -3,
    BL_SB_ERROR_CRYPTO_FAILURE = -4,
    BL_SB_ERROR_INVALID_SIGNATURE = -5,
    BL_SB_ERROR_INVALID_HASH = -6,
    BL_SB_ERROR_VERSION_ROLLBACK = -7,
    BL_SB_ERROR_CERT_INVALID = -8,
    BL_SB_ERROR_CERT_EXPIRED = -9,
    BL_SB_ERROR_CERT_REVOKED = -10,
    BL_SB_ERROR_CERT_CHAIN_INVALID = -11,
    BL_SB_ERROR_ROOT_CERT_NOT_TRUSTED = -12,
    BL_SB_ERROR_FIRMWARE_CORRUPTED = -13,
    BL_SB_ERROR_INVALID_MAGIC = -14,
    BL_SB_ERROR_PARSE_ERROR = -15,
    BL_SB_ERROR_ROLLBACK_PROTECTION = -16,
    BL_SB_ERROR_VERSION_MISMATCH = -17,
    BL_SB_ERROR_TIME_UNAVAILABLE = -18   /* 无可用时间源, 无法校验有效期/记录时间戳 */
} bl_secure_boot_error_t;

/* ============================================================================
 * 安全启动验证状态
 * ============================================================================ */
typedef enum {
    BL_SB_STATE_UNVERIFIED = 0,         /* 未验证 */
    BL_SB_STATE_VERIFICATION_IN_PROGRESS,
    BL_SB_STATE_SIGNATURE_VALID,
    BL_SB_STATE_HASH_VALID,
    BL_SB_STATE_VERSION_CHECKED,
    BL_SB_STATE_CERT_VALID,
    BL_SB_STATE_VERIFIED,               /* 完全验证通过 */
    BL_SB_STATE_VERIFICATION_FAILED,
    BL_SB_STATE_ROLLBACK_DETECTED
} bl_secure_boot_state_t;

/* ============================================================================
 * 签名类型
 * ============================================================================ */
typedef enum {
    BL_SB_SIGN_NONE = 0,
    BL_SB_SIGN_ECDSA_P256_SHA256,   /* 默认: ECDSA P-256 + SHA-256 */
    BL_SB_SIGN_ECDSA_P384_SHA384,
    BL_SB_SIGN_RSA_PKCS1_SHA256,
    BL_SB_SIGN_RSA_PSS_SHA256,
    BL_SB_SIGN_ED25519,
    BL_SB_SIGN_SM2_SM3              /* 预留: SM2 国密 (Csm 后端接入后启用; 当前 fail-closed) */
} bl_signature_type_t;

/* ============================================================================
 * 哈希类型
 * ============================================================================ */
typedef enum {
    BL_SB_HASH_SHA256 = 0,
    BL_SB_HASH_SHA384,
    BL_SB_HASH_SHA512
} bl_hash_type_t;

/* ============================================================================
 * 固件头结构 (安全启加头部)
 * ============================================================================ */
typedef struct {
    /* 魔法和版本 */
    uint32_t magic;                     /* 0x5342444D "SBDM" */
    uint32_t header_version;            /* 头部版本 */
    uint32_t header_size;               /* 头部大小 */
    
    /* 固件信息 */
    uint32_t firmware_version;          /* 固件版本号 */
    uint32_t hardware_version;          /* 硬件版本号 */
    uint32_t firmware_size;             /* 固件大小 */
    uint32_t entry_point;               /* 入口地址 */
    
    /* 验证信息 */
    bl_signature_type_t sign_type;      /* 签名类型 */
    bl_hash_type_t hash_type;           /* 哈希类型 */
    uint8_t  hash[BL_SB_HASH_SIZE];     /* 固件哈希 */
    uint8_t  signature[BL_SB_SIGNATURE_SIZE]; /* 固件签名 */
    
    /* 证书信息 */
    uint32_t cert_chain_offset;         /* 证书链偏移 */
    uint32_t cert_chain_size;           /* 证书链大小 */
    
    /* 安全属性 */
    uint32_t security_flags;            /* 安全标志 */
    uint32_t min_bootloader_version;    /* 要求的最小bootloader版本 */
    
    /* 元数据 */
    uint64_t build_timestamp;           /* 构建时间戳 */
    uint8_t  build_id[16];              /* 构建ID */
    
    /* 保留字段 */
    uint32_t reserved[8];
    
    /* 头部CRC */
    uint32_t header_crc32;
} bl_firmware_header_t;

/* 安全头部魔法 */
#define BL_SB_FIRMWARE_MAGIC            0x5342444DU  /* "SBDM" */
#define BL_SB_HEADER_VERSION            1U

/* 安全标志 */
#define BL_SB_FLAG_ENCRYPTION_REQUIRED  0x0001
#define BL_SB_FLAG_SECURE_BOOT_REQUIRED 0x0002
#define BL_SB_FLAG_ROLLBACK_PROTECTION  0x0004
#define BL_SB_FLAG_DEBUG_DISABLED       0x0008
#define BL_SB_FLAG_ANTI_TAMPER          0x0010
#define BL_SB_FLAG_VERSION_BOUND_SIGNATURE 0x0020  /* 版本绑定签名 (G4, RS-OTA-04) */

/* ============================================================================
 * 版本绑定签名格式 (RS-OTA-04: 版本号在签名内容内)
 * ============================================================================ */
#define BL_SB_VERSION_BINDING_FORMAT_VERSION  1U

/*
 * 版本绑定结构: 签名覆盖该结构的 SHA-256, 从而将固件版本号绑定进签名内容。
 * 攻击者篡改头部 firmware_version 后, 验签结构随之改变 → 步骤①签名验证失败。
 * 布局固定 (AUTOSAR 风格, 无隐式填充依赖: 4+4+4+32=44 字节)。
 */
typedef struct {
    uint32_t format_version;    /* BL_SB_VERSION_BINDING_FORMAT_VERSION (防混淆) */
    uint32_t firmware_version;  /* 签名内固件版本号 */
    uint32_t firmware_size;     /* payload 大小 (防长度混淆) */
    uint8_t  payload_hash[BL_SB_HASH_SIZE];  /* payload SHA-256 */
} bl_version_binding_t;

/* ============================================================================
 * 版本信息结构
 * ============================================================================ */
typedef struct {
    uint8_t  major;
    uint8_t  minor;
    uint16_t patch;
    uint32_t build;
} bl_version_t;

/* 版本比较结果 */
typedef enum {
    BL_VERSION_EQUAL = 0,
    BL_VERSION_OLDER = -1,
    BL_VERSION_NEWER = 1
} bl_version_compare_t;

/* ============================================================================
 * 证书结构 (简化X.509)
 * ============================================================================ */
typedef struct {
    uint8_t  *data;
    uint32_t size;
    
    /* 解析后的字段 */
    uint8_t  issuer[128];
    uint8_t  subject[128];
    uint64_t valid_from;
    uint64_t valid_until;
    uint8_t  public_key[64];            /* ECDSA P-256公钥 */
    uint8_t  public_key_len;
    
    /* 签名 */
    bl_signature_type_t sign_type;
    uint8_t  signature[BL_SB_SIGNATURE_SIZE];
    
    /* 父证书索引 */
    int8_t   parent_index;
    bool     is_self_signed;
    bool     is_ca;
} bl_certificate_t;

/* ============================================================================
 * 证书链结构
 * ============================================================================ */
typedef struct {
    bl_certificate_t certs[BL_SB_MAX_CERT_CHAIN_DEPTH];
    uint8_t num_certs;
    uint8_t root_cert_index;
} bl_cert_chain_t;

/* ============================================================================
 * 回滚防护信息
 * ============================================================================ */
typedef struct {
    uint32_t min_allowed_version;       /* 允许的最低版本 */
    uint32_t current_version;           /* 当前版本 */
    uint32_t previous_version;          /* 上一版本 */
    uint32_t boot_attempts;             /* 启动尝试次数 */
    uint32_t max_boot_attempts;         /* 最大启动尝试次数 */
    bool     rollback_detected;         /* 回滚检测 */
    bool     version_locked;            /* 版本已锁定 */
    uint64_t version_lock_timestamp;    /* 版本锁定时间 */
} bl_rollback_protection_t;

/* ============================================================================
 * 安全启动配置
 * ============================================================================ */
typedef struct {
    /* 验证选项 */
    bool verify_signature;              /* 验证签名 */
    bool verify_hash;                   /* 验证哈希 */
    bool verify_version;                /* 验证版本 (防回滚) */
    bool verify_cert_chain;             /* 验证证书链 */
    bool verify_cert_validity;          /* 验证证书有效期 */
    
    /* 限制 */
    uint32_t max_boot_attempts;         /* 最大启动尝试次数 */
    uint32_t min_firmware_version;      /* 最小允许版本 */
    
    /* 密钥索引 */
    uint8_t root_ca_key_slot;           /* 根CA密钥槽 */
    uint8_t oem_key_slot;               /* OEM密钥槽 */

    /* 抗回滚计数器 (RS-OTA-01): 启动时从 NVM 加载的快照; 0 表示禁用 */
    uint32_t anti_rollback_counter;
    
    /* 回调 */
    void (*on_verification_progress)(bl_secure_boot_state_t state, uint8_t progress);
    void (*on_verification_complete)(bl_secure_boot_error_t result);
} bl_secure_boot_config_t;

/* ============================================================================
 * 验证上下文
 * ============================================================================ */
typedef struct {
    bl_secure_boot_config_t config;
    bl_secure_boot_state_t  state;
    bl_secure_boot_error_t  last_error;
    
    /* CSM和KeyM接口 */
    void *csm_context;                  /* CSM上下文指针 */
    void *keym_context;                 /* KeyM上下文指针 */
    
    /* 当前验证的固件信息 */
    bl_firmware_header_t current_header;
    bl_version_t current_version;
    
    /* 版本历史 */
    bl_rollback_protection_t rollback_info;
    
    /* 验证统计 */
    uint32_t total_verifications;
    uint32_t failed_verifications;
    uint32_t rollback_attempts_blocked;
    
    bool initialized;
} bl_secure_boot_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化安全启动模块
 * @param ctx 安全启动上下文
 * @param config 配置参数
 * @param csm_ctx CSM上下文 (从crypto_stack)
 * @param keym_ctx KeyM上下文 (从crypto_stack)
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_init(
    bl_secure_boot_context_t *ctx,
    const bl_secure_boot_config_t *config,
    void *csm_ctx,
    void *keym_ctx
);

/**
 * @brief 反初始化安全启动模块
 * @param ctx 安全启动上下文
 */
void bl_secure_boot_deinit(bl_secure_boot_context_t *ctx);

/**
 * @brief 解析固件头部
 * @param ctx 安全启动上下文
 * @param header_data 头部数据
 * @param header_size 头部大小
 * @param header 输出解析后的头部结构
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_parse_header(
    bl_secure_boot_context_t *ctx,
    const uint8_t *header_data,
    uint32_t header_size,
    bl_firmware_header_t *header
);

/**
 * @brief 验证固件头部CRC
 * @param ctx 安全启动上下文
 * @param header 头部结构
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_verify_header_crc(
    bl_secure_boot_context_t *ctx,
    const bl_firmware_header_t *header
);

/**
 * @brief 验证固件签名
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据
 * @param firmware_size 固件大小
 * @param signature 签名数据
 * @param sign_type 签名类型
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_verify_signature(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *signature,
    bl_signature_type_t sign_type
);

/**
 * @brief 验证固件哈希
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据
 * @param firmware_size 固件大小
 * @param expected_hash 预期哈希
 * @param hash_type 哈希类型
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_verify_hash(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    bl_hash_type_t hash_type
);

/**
 * @brief 计算固件哈希
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据
 * @param firmware_size 固件大小
 * @param hash 输出哈希
 * @param hash_type 哈希类型
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_calculate_hash(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    uint8_t *hash,
    bl_hash_type_t hash_type
);

/**
 * @brief 版本比较
 * @param ctx 安全启动上下文
 * @param version1 版本1
 * @param version2 版本2
 * @return BL_VERSION_EQUAL, BL_VERSION_OLDER, BL_VERSION_NEWER
 */
bl_version_compare_t bl_secure_boot_compare_versions(
    uint32_t version1,
    uint32_t version2
);

/**
 * @brief 检查版本防回滚
 * @details 包含抗回滚计数器校验 (RS-OTA-01): 新版本低于计数器
 *          → BL_SB_ERROR_ROLLBACK_PROTECTION; 随后执行现有版本回滚检查。
 * @param ctx 安全启动上下文
 * @param new_version 新版本
 * @param current_version 当前版本
 * @return BL_SB_OK允许升级, BL_SB_ERROR_ROLLBACK_PROTECTION (计数器拒绝),
 *         BL_SB_ERROR_VERSION_ROLLBACK禁止回滚
 */
bl_secure_boot_error_t bl_secure_boot_check_rollback(
    bl_secure_boot_context_t *ctx,
    uint32_t new_version,
    uint32_t current_version
);

/**
 * @brief 更新回滚防护信息
 * @param ctx 安全启动上下文
 * @param new_version 新版本
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_update_rollback_info(
    bl_secure_boot_context_t *ctx,
    uint32_t new_version
);

/**
 * @brief 解析证书链
 * @param ctx 安全启动上下文
 * @param cert_data 证书数据
 * @param cert_size 证书大小
 * @param chain 输出证书链结构
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_parse_cert_chain(
    bl_secure_boot_context_t *ctx,
    const uint8_t *cert_data,
    uint32_t cert_size,
    bl_cert_chain_t *chain
);

/**
 * @brief 验证证书链
 * @param ctx 安全启动上下文
 * @param chain 证书链
 * @param trusted_root_key 可信根公钥
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_verify_cert_chain(
    bl_secure_boot_context_t *ctx,
    const bl_cert_chain_t *chain,
    const uint8_t *trusted_root_key
);

/**
 * @brief 完整的安全启动验证
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据 (包含头部)
 * @param firmware_size 固件大小
 * @return BL_SB_OK验证通过
 */
bl_secure_boot_error_t bl_secure_boot_verify(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size
);

/**
 * @brief 快速验证 (仅验证签名和哈希，不验证证书链)
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据
 * @param firmware_size 固件大小
 * @param expected_hash 预期哈希
 * @param signature 签名
 * @return BL_SB_OK验证通过
 */
bl_secure_boot_error_t bl_secure_boot_verify_fast(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    const uint8_t *signature
);

/**
 * @brief 记录启动尝试
 * @param ctx 安全启动上下文
 * @param success 启动是否成功
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_record_boot_attempt(
    bl_secure_boot_context_t *ctx,
    bool success
);

/**
 * @brief 检查是否需要回滚
 * @param ctx 安全启动上下文
 * @param need_rollback 输出是否需要回滚
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_check_need_rollback(
    bl_secure_boot_context_t *ctx,
    bool *need_rollback
);

/**
 * @brief 获取当前验证状态
 * @param ctx 安全启动上下文
 * @return 当前状态
 */
bl_secure_boot_state_t bl_secure_boot_get_state(
    const bl_secure_boot_context_t *ctx
);

/**
 * @brief 获取最后错误码
 * @param ctx 安全启动上下文
 * @return 最后错误码
 */
bl_secure_boot_error_t bl_secure_boot_get_last_error(
    const bl_secure_boot_context_t *ctx
);

/**
 * @brief 获取版本字符串
 * @param version 版本值
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 版本字符串
 */
const char* bl_secure_boot_version_to_string(
    uint32_t version,
    char *buf,
    uint32_t buf_size
);

/**
 * @brief 验签 3 步强化 (RS-OTA-04): 严格按 ①签名 ②版本绑定 ③完整性 顺序执行
 * @details ① 签名验证: 对版本绑定结构 (含版本号+payload哈希) 验签;
 *          ② 版本绑定校验: 签名内版本 == 头部版本, 且版本 >= 抗回滚计数器;
 *          ③ 完整性哈希: 实际 payload 哈希 == 头部声明哈希。
 *          任一失败即拒绝并返回对应错误码 (调用方应写升级日志)。
 *          内部完成头部解析/CRC 校验, 可独立使用。
 * @param ctx 安全启动上下文
 * @param firmware_data 固件数据 (包含头部)
 * @param firmware_size 固件大小
 * @return BL_SB_OK验证通过
 */
bl_secure_boot_error_t bl_secure_boot_verify_strict(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size
);

/**
 * @brief 3步验签 步骤①: 版本绑定签名验证 (版本号在签名内容内)
 * @details 计算 payload 哈希, 构造版本绑定结构, 对绑定结构验签。
 *          版本号被签名覆盖 → 篡改版本号必然导致验签失败。
 * @param ctx 安全启动上下文
 * @param payload payload 数据 (不含头部)
 * @param payload_size payload 大小
 * @param version 固件版本号 (写入签名内容)
 * @param signature 签名数据
 * @param sign_type 签名类型
 * @return BL_SB_OK成功; BL_SB_ERROR_INVALID_SIGNATURE 签名无效
 */
bl_secure_boot_error_t bl_secure_boot_verify_signature_bound(
    bl_secure_boot_context_t *ctx,
    const uint8_t *payload,
    uint32_t payload_size,
    uint32_t version,
    const uint8_t *signature,
    bl_signature_type_t sign_type
);

/**
 * @brief 3步验签 步骤②: 版本绑定校验
 * @details ① 签名内版本与头部版本一致性 (防版本号篡改);
 *          ② 抗回滚计数器: 头部版本 < 计数器 → BL_SB_ERROR_ROLLBACK_PROTECTION;
 *          ③ 现有版本回滚检查 (与 current_version 比较)。
 * @param ctx 安全启动上下文
 * @param signed_version 从签名内容解析出的版本号
 * @param header_version 头部声明的版本号
 * @return BL_SB_OK成功; BL_SB_ERROR_VERSION_MISMATCH (版本不一致),
 *         BL_SB_ERROR_ROLLBACK_PROTECTION (低于计数器), BL_SB_ERROR_VERSION_ROLLBACK
 */
bl_secure_boot_error_t bl_secure_boot_verify_version_binding(
    bl_secure_boot_context_t *ctx,
    uint32_t signed_version,
    uint32_t header_version
);

/**
 * @brief 3步验签 步骤③: 完整性哈希校验
 * @details 计算 payload 哈希并与头部声明哈希比较 (防篡改/损坏)。
 * @param ctx 安全启动上下文
 * @param payload payload 数据 (不含头部)
 * @param payload_size payload 大小
 * @param expected_hash 头部声明的期望哈希
 * @param hash_type 哈希类型
 * @return BL_SB_OK成功; BL_SB_ERROR_INVALID_HASH 哈希不一致
 */
bl_secure_boot_error_t bl_secure_boot_verify_integrity(
    bl_secure_boot_context_t *ctx,
    const uint8_t *payload,
    uint32_t payload_size,
    const uint8_t *expected_hash,
    bl_hash_type_t hash_type
);

/**
 * @brief 锁定版本 (防止未来回滚到此版本以下)
 * @param ctx 安全启动上下文
 * @param version 要锁定的版本
 * @return BL_SB_OK成功
 */
bl_secure_boot_error_t bl_secure_boot_lock_version(
    bl_secure_boot_context_t *ctx,
    uint32_t version
);

#ifdef __cplusplus
}
#endif

#endif /* BL_SECURE_BOOT_H */
