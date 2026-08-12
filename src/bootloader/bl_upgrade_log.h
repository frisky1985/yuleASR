/**
 * @file bl_upgrade_log.h
 * @brief Bootloader Upgrade Audit Log Module
 * @version 1.0
 * @date 2026-08-12
 *
 * 升级日志可追溯 (RS-OTA-03 / GB 44496-2024 §7.2, UNECE R156 §7.1.1 SUMS):
 * - 每次升级记录: 时间戳 / 版本号 / 来源 / 签名结果 / 结果状态
 * - NVM 持久化, 可经诊断读取 (Boot_UpgradeLog_Read)
 * - 环形缓冲 (默认 16 条, 可配置), 满则覆盖最旧条目
 * - 每条目独立 CRC32 完整性保护
 *
 * 时间语义: 时间戳经 bl_time 抽象获取; 无可用时间源时写入必须显式报错
 * (BL_UPGRADE_LOG_ERROR_TIME_UNAVAILABLE), 禁止静默写入 0 时间戳。
 * ASIL-D Safety Level
 */

#ifndef BL_UPGRADE_LOG_H
#define BL_UPGRADE_LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define BL_UPGRADE_LOG_MAJOR_VERSION        1
#define BL_UPGRADE_LOG_MINOR_VERSION        0
#define BL_UPGRADE_LOG_PATCH_VERSION        0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define BL_UPGRADE_LOG_MAX_ENTRIES          16U   /* 环形缓冲容量上限 (可裁剪) */
#define BL_UPGRADE_LOG_DEFAULT_CAPACITY     16U   /* 默认容量 */
#define BL_UPGRADE_LOG_MAGIC                0x554C4F47U  /* "ULOG" */
#define BL_UPGRADE_LOG_RECORD_VERSION       1U

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    BL_UPGRADE_LOG_OK = 0,
    BL_UPGRADE_LOG_ERROR_INVALID_PARAM = -1,      /* 非法参数 */
    BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED = -2,    /* 未初始化 */
    BL_UPGRADE_LOG_ERROR_STORAGE_ERROR = -3,      /* NVM 读写/擦除失败 */
    BL_UPGRADE_LOG_ERROR_TIME_UNAVAILABLE = -4,   /* 无可用时间源, 无法记录时间戳 */
    BL_UPGRADE_LOG_ERROR_INDEX_OUT_OF_RANGE = -5, /* 读取索引越界 */
    BL_UPGRADE_LOG_ERROR_ENTRY_CORRUPTED = -6     /* 条目 CRC 校验失败 (完整性受损) */
} bl_upgrade_log_error_t;

/* ============================================================================
 * 升级来源
 * ============================================================================ */
/* 枚举类型刻意匿名 (MISRA 2.3): 字段按 AUTOSAR 风格以显式定长 uint8_t 存储
 * (见 bl_upgrade_log_entry_t), 枚举仅提供取值常量; typedef 名无消费方,
 * 避免产生未使用的类型定义。 */
enum {
    BL_UPGRADE_LOG_SOURCE_OTA = 0,        /* OTA 远程升级 */
    BL_UPGRADE_LOG_SOURCE_DIAGNOSTIC,     /* 诊断仪升级 */
    BL_UPGRADE_LOG_SOURCE_LOCAL,          /* 本地烧录 */
    BL_UPGRADE_LOG_SOURCE_UNKNOWN = 0xFFU /* 未知来源 */
};

/* ============================================================================
 * 签名验证结果
 * ============================================================================ */
enum {
    BL_UPGRADE_LOG_SIG_OK = 0,            /* 签名验证通过 */
    BL_UPGRADE_LOG_SIG_INVALID,           /* 签名验证失败 */
    BL_UPGRADE_LOG_SIG_NOT_VERIFIED       /* 未执行验签 (旧格式/降级路径) */
};

/* ============================================================================
 * 升级结果状态
 * ============================================================================ */
enum {
    BL_UPGRADE_LOG_RESULT_SUCCESS = 0,    /* 升级成功 */
    BL_UPGRADE_LOG_RESULT_FAILED,         /* 升级失败 */
    BL_UPGRADE_LOG_RESULT_ABORTED,        /* 升级中止 */
    BL_UPGRADE_LOG_RESULT_ROLLBACK,       /* 回滚 */
    BL_UPGRADE_LOG_RESULT_TIMEOUT,        /* 超时 */
    BL_UPGRADE_LOG_RESULT_UNKNOWN = 0xFFU /* 未知结果 */
};

/* ============================================================================
 * 升级日志条目
 * ============================================================================ */
typedef struct {
    uint64_t timestamp_ms;        /* 时间戳 (自固定纪元, ms) */
    uint32_t version;             /* 固件版本号 */
    uint8_t  source;              /* 取值见 BL_UPGRADE_LOG_SOURCE_* (匿名枚举) */
    uint8_t  signature_result;    /* 取值见 BL_UPGRADE_LOG_SIG_* (匿名枚举) */
    uint8_t  result;              /* 取值见 BL_UPGRADE_LOG_RESULT_* (匿名枚举) */
    uint8_t  reserved[7];         /* 保留对齐 */
    uint32_t crc32;               /* 条目完整性 (覆盖除 crc32 外字段) */
} bl_upgrade_log_entry_t;

/* ============================================================================
 * 持久化头部 (NVM 布局)
 * ============================================================================ */
typedef struct {
    uint32_t magic;               /* BL_UPGRADE_LOG_MAGIC */
    uint32_t record_version;      /* 记录格式版本 */
    uint32_t capacity;            /* 环形容量 */
    uint32_t count;               /* 有效条目数 */
    uint32_t head;                /* 最旧条目索引 */
    uint32_t crc32;               /* 头部完整性 */
} bl_upgrade_log_header_t;

/* ============================================================================
 * 配置
 * ============================================================================ */
typedef struct {
    uint32_t capacity;            /* 环形容量 (1..BL_UPGRADE_LOG_MAX_ENTRIES; 0 取默认) */
    void    *storage;             /* bl_partition_manager_t* — NVM 持久化用 (Save/Load) */
} bl_upgrade_log_config_t;

/* ============================================================================
 * 上下文
 * ============================================================================ */
typedef struct {
    bl_upgrade_log_config_t  config;
    bl_upgrade_log_header_t  header;
    bl_upgrade_log_entry_t   entries[BL_UPGRADE_LOG_MAX_ENTRIES];
    bool initialized;
} bl_upgrade_log_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化升级日志模块
 * @param ctx 上下文
 * @param config 配置 (可为 NULL, 使用默认容量 16)
 * @return BL_UPGRADE_LOG_OK 成功
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Init(
    bl_upgrade_log_context_t *ctx,
    const bl_upgrade_log_config_t *config
);

/**
 * @brief 反初始化升级日志模块
 * @param ctx 上下文
 */
void Boot_UpgradeLog_Deinit(bl_upgrade_log_context_t *ctx);

/**
 * @brief 写入一条升级日志 (环形缓冲; 满则覆盖最旧条目)
 * @details 要求可用时间源, 否则返回 BL_UPGRADE_LOG_ERROR_TIME_UNAVAILABLE。
 * @param ctx 上下文
 * @param entry 条目内容 (timestamp_ms 由本函数填充; 其余字段由调用方提供)
 * @return BL_UPGRADE_LOG_OK 成功
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Write(
    bl_upgrade_log_context_t *ctx,
    bl_upgrade_log_entry_t *entry
);

/**
 * @brief 读取日志条目 (时间序: index 0 = 最旧, count-1 = 最新)
 * @param ctx 上下文
 * @param index 索引 (0..count-1)
 * @param entry 输出条目
 * @return BL_UPGRADE_LOG_OK 成功; 越界 → INDEX_OUT_OF_RANGE; CRC 损坏 → ENTRY_CORRUPTED
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Read(
    const bl_upgrade_log_context_t *ctx,
    uint32_t index,
    bl_upgrade_log_entry_t *entry
);

/**
 * @brief 获取有效日志条数
 * @param ctx 上下文
 * @param count 输出条数
 * @return BL_UPGRADE_LOG_OK 成功
 */
bl_upgrade_log_error_t Boot_UpgradeLog_GetCount(
    const bl_upgrade_log_context_t *ctx,
    uint32_t *count
);

/**
 * @brief 清空日志 (仅 RAM; 持久化需再调用 Save)
 * @param ctx 上下文
 * @return BL_UPGRADE_LOG_OK 成功
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Clear(bl_upgrade_log_context_t *ctx);

/**
 * @brief 持久化日志到 NVM
 * @param ctx 上下文 (config.storage 需为 bl_partition_manager_t*)
 * @param address NVM 写入地址
 * @return BL_UPGRADE_LOG_OK 成功
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Save(
    bl_upgrade_log_context_t *ctx,
    uint32_t address
);

/**
 * @brief 从 NVM 加载日志
 * @param ctx 上下文 (config.storage 需为 bl_partition_manager_t*)
 * @param address NVM 读取地址
 * @return BL_UPGRADE_LOG_OK 成功; 头部损坏 → STORAGE_ERROR
 */
bl_upgrade_log_error_t Boot_UpgradeLog_Load(
    bl_upgrade_log_context_t *ctx,
    uint32_t address
);

/**
 * @brief 来源枚举转字符串 (诊断输出)
 * @param source 来源枚举值
 * @return 字符串
 */
const char* Boot_UpgradeLog_SourceToString(uint8_t source);

/**
 * @brief 结果枚举转字符串 (诊断输出)
 * @param result 结果枚举值
 * @return 字符串
 */
const char* Boot_UpgradeLog_ResultToString(uint8_t result);

#ifdef __cplusplus
}
#endif

#endif /* BL_UPGRADE_LOG_H */
