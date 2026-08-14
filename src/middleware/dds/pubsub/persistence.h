/**
 * @file persistence.h
 * @brief DDS持久化服务 - Durability QoS实现
 * @version 1.0
 * @date 2026-04-24
 * 
 * 车载要求：
 * - TRANSIENT_LOCAL/低延迟恢复
 * - TRANSIENT/内存存储
 * - PERSISTENT/非易失性存储
 * - 在线恢复/热备份
 * - ASIL-D安全等级支持
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "../../common/types/eth_types.h"
#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/** 最大存储记录数 */
#define PER_MAX_RECORDS             10000
/** 默认存储目录 */
#define PER_DEFAULT_STORAGE_DIR     "/var/lib/dds/persistence"
/** 最大记录大小 */
#define PER_MAX_RECORD_SIZE         65536
/** 刷新间隔(ms) */
#define PER_FLUSH_INTERVAL_MS       100
/** 最大恢复时间(ms) */
#define PER_MAX_RECOVERY_TIME_MS    5000
/** 双写缓冲区数量 */
#define PER_DOUBLE_BUFFER_COUNT     2
/** CRC32校验和大小 */
#define PER_CHECKSUM_SIZE           4

/* ============================================================================
 * 类型定义
 * ============================================================================ */

/** 持久化等级 */
typedef enum {
    PER_LEVEL_TRANSIENT_LOCAL = 0,      /**< TRANSIENT_LOCAL - 内存持久化，仅对同一进程 */
    PER_LEVEL_TRANSIENT,                 /**< TRANSIENT - 进程间共享的内存持久化 */
    PER_LEVEL_PERSISTENT,                /**< PERSISTENT - 非易失性存储 */
} per_durability_level_t;

/** 存储类型 */
typedef enum {
    PER_STORAGE_MEMORY = 0,              /**< 内存存储 */
    PER_STORAGE_FILE,                    /**< 文件存储 */
    PER_STORAGE_DATABASE,                /**< 数据库存储 */
    PER_STORAGE_NVRAM,                   /**< 非易失性RAM(车载) */
} per_storage_type_t;

/** 持久化状态 */
typedef enum {
    PER_STATE_IDLE = 0,                  /**< 空闲 */
    PER_STATE_WRITING,                   /**< 正在写入 */
    PER_STATE_FLUSHING,                  /**< 正在刷盘 */
    PER_STATE_RECOVERING,                /**< 正在恢复 */
    PER_STATE_ERROR,                     /**< 错误状态 */
} per_state_t;

/** 存储记录头 */
typedef struct {
    uint64_t sequence_num;               /**< 序列号 */
    uint64_t timestamp_us;               /**< 时间戳(微秒) */
    uint32_t data_size;                  /**< 数据大小 */
    uint32_t checksum;                   /**< CRC32校验和 */
    dds_guid_t writer_guid;              /**< 写入者GUID */
    per_durability_level_t level;        /**< 持久化等级 */
    uint8_t flags;                       /**< 标记位 */
} per_record_header_t;

/** 存储记录 */
typedef struct per_record {
    per_record_header_t header;
    uint8_t *data;
    struct per_record *next;
    struct per_record *prev;
} per_record_t;

/** 持久化配置 */
typedef struct {
    per_durability_level_t level;        /**< 持久化等级 */
    per_storage_type_t storage_type;     /**< 存储类型 */
    char storage_path[256];              /**< 存储路径 */
    uint32_t max_records;                /**< 最大记录数 */
    uint32_t max_record_size;            /**< 最大单记录大小 */
    uint32_t auto_flush_interval_ms;     /**< 自动刷盘间隔 */
    bool enable_checksum;                /**< 启用校验和 */
    bool enable_compression;             /**< 启用压缩 */
    bool enable_encryption;              /**< 启用加密 */
    uint32_t recovery_timeout_ms;        /**< 恢复超时 */
    bool enable_hot_standby;             /**< 启用热备份 */
} per_config_t;

/** 历史缓存策略 */
typedef enum {
    PER_HISTORY_KEEP_ALL = 0,            /**< 保留所有 */
    PER_HISTORY_KEEP_LAST,               /**< 保留最后N个 */
} per_history_policy_t;

/** 持久化请求 */
typedef struct {
    dds_guid_t reader_guid;              /**< 请求者GUID */
    uint64_t start_sequence;             /**< 起始序列号 */
    uint32_t max_samples;                /**< 最大样本数 */
    per_durability_level_t min_level;    /**< 最小持久化等级 */
} per_request_t;

/** 恢复状态 */
typedef struct {
    uint32_t total_records;              /**< 总记录数 */
    uint32_t recovered_records;          /**< 已恢复记录数 */
    uint32_t failed_records;             /**< 失败记录数 */
    uint64_t recovery_time_ms;           /**< 恢复耗时(ms) */
    bool complete;                       /**< 是否完成 */
} per_recovery_status_t;

/** 持久化统计 */
typedef struct {
    uint64_t total_writes;               /**< 总写入次数 */
    uint64_t total_reads;                /**< 总读取次数 */
    uint64_t total_flushes;              /**< 总刷盘次数 */
    uint64_t bytes_written;              /**< 写入字节数 */
    uint64_t bytes_read;                 /**< 读取字节数 */
    uint32_t current_records;            /**< 当前记录数 */
    uint32_t checksum_errors;            /**< 校验错误数 */
    uint32_t write_errors;               /**< 写入错误数 */
    uint32_t read_errors;                /**< 读取错误数 */
    uint32_t recovery_count;             /**< 恢复次数 */
} per_stats_t;

/** 持久化服务句柄(不透明) */
typedef struct per_handle per_handle_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化持久化服务模块
 * @return ETH_OK成功
 */
eth_status_t per_init(void);

/**
 * @brief 反初始化持久化服务模块
 */
void per_deinit(void);

/**
 * @brief 创建持久化服务
 * @param topic_name 主题名称
 * @param config 持久化配置
 * @return 持久化服务句柄
 */
per_handle_t* per_create(const char *topic_name, const per_config_t *config);

/**
 * @brief 删除持久化服务
 * @param per 持久化服务句柄
 * @param flush 是否先刷盘
 * @return ETH_OK成功
 */
eth_status_t per_delete(per_handle_t *per, bool flush);

/**
 * @brief 写入样本到持久化存储
 * @param per 持久化服务句柄
 * @param writer_guid 写入者GUID
 * @param data 样本数据
 * @param size 样本大小
 * @param sequence_num 序列号
 * @return ETH_OK成功
 */
eth_status_t per_write_sample(per_handle_t *per,
                               const dds_guid_t *writer_guid,
                               const void *data,
                               uint32_t size,
                               uint64_t sequence_num);

/**
 * @brief 从持久化存储读取样本
 * @param per 持久化服务句柄
 * @param reader_guid 读取者GUID
 * @param start_sequence 起始序列号
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param actual_size 实际读取大小输出
 * @param records_read 读取记录数输出
 * @return ETH_OK成功
 */
eth_status_t per_read_samples(per_handle_t *per,
                               const dds_guid_t *reader_guid,
                               uint64_t start_sequence,
                               void *buffer,
                               uint32_t buffer_size,
                               uint32_t *actual_size,
                               uint32_t *records_read);

/**
 * @brief 执行刷盘(确保数据写入持久介质)
 * @param per 持久化服务句柄
 * @param async 是否异步刷盘
 * @return ETH_OK成功
 */
eth_status_t per_flush(per_handle_t *per, bool async);

/**
 * @brief 执行在线恢复
 * @param per 持久化服务句柄
 * @param request 恢复请求
 * @param status 恢复状态输出
 * @return ETH_OK成功
 */
eth_status_t per_recover(per_handle_t *per,
                          const per_request_t *request,
                          per_recovery_status_t *status);

/**
 * @brief 检查是否需要恢复
 * @param per 持久化服务句柄
 * @param needs_recovery 输出: 是否需要恢复
 * @return ETH_OK成功
 */
eth_status_t per_check_recovery_needed(per_handle_t *per, bool *needs_recovery);

/**
 * @brief 清理过期记录
 * @param per 持久化服务句柄
 * @param keep_count 保留记录数
 * @return ETH_OK成功
 */
eth_status_t per_cleanup_old_records(per_handle_t *per, uint32_t keep_count);

/**
 * @brief 获取最新序列号
 * @param per 持久化服务句柄
 * @param sequence_num 输出: 最新序列号
 * @return ETH_OK成功
 */
eth_status_t per_get_last_sequence(per_handle_t *per, uint64_t *sequence_num);

/**
 * @brief 设置历史缓存策略
 * @param per 持久化服务句柄
 * @param policy 历史策略
 * @param depth 历史深度(用于KEEP_LAST)
 * @return ETH_OK成功
 */
eth_status_t per_set_history_policy(per_handle_t *per, 
                                     per_history_policy_t policy, 
                                     uint32_t depth);

/**
 * @brief 获取持久化统计信息
 * @param per 持久化服务句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t per_get_stats(per_handle_t *per, per_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param per 持久化服务句柄
 * @return ETH_OK成功
 */
eth_status_t per_reset_stats(per_handle_t *per);

/**
 * @brief 设置ASIL安全模式
 * @param per 持久化服务句柄
 * @param asil_level ASIL等级(0-4)
 * @return ETH_OK成功
 */
eth_status_t per_enable_asil_mode(per_handle_t *per, uint8_t asil_level);

/**
 * @brief 备份持久化数据
 * @param per 持久化服务句柄
 * @param backup_path 备份路径
 * @return ETH_OK成功
 */
eth_status_t per_backup(per_handle_t *per, const char *backup_path);

/**
 * @brief 从备份恢复数据
 * @param per 持久化服务句柄
 * @param backup_path 备份路径
 * @return ETH_OK成功
 */
eth_status_t per_restore(per_handle_t *per, const char *backup_path);

/**
 * @brief 计算数据CRC32校验和
 * @param data 数据指针
 * @param size 数据大小
 * @return CRC32值
 */
uint32_t per_calc_crc32(const void *data, uint32_t size);

/**
 * @brief 验证记录完整性
 * @param record 记录指针
 * @return true有效
 */
bool per_validate_record(const per_record_t *record);

/**
 * @brief 启用热备份模式
 * @param per 持久化服务句柄
 * @param standby_path 备份路径
 * @param sync_interval_ms 同步间隔
 * @return ETH_OK成功
 */
eth_status_t per_enable_hot_standby(per_handle_t *per, 
                                     const char *standby_path,
                                     uint32_t sync_interval_ms);

/**
 * @brief 执行热备份同步
 * @param per 持久化服务句柄
 * @return ETH_OK成功
 */
eth_status_t per_sync_hot_standby(per_handle_t *per);

#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCE_H */
