/**
 * @file time_filter.h
 * @brief DDS时间过滤器 - TimeBasedFilter QoS策略实现
 * @version 1.0
 * @date 2026-04-24
 * 
 * 车载要求：
 * - 确定性时间管理(精度1us)
 * - 最小时间分隔(minimum_separation) ≥ 1ms
 * - 数据压缩与间隔取样
 * - 时间戳验证
 */

#ifndef TIME_FILTER_H
#define TIME_FILTER_H

#include "../../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/** 最小有效时间分隔(1ms) */
#define TBF_MIN_SEPARATION_US      1000
/** 默认时间分隔 */
#define TBF_DEFAULT_SEPARATION_US  10000
/** 最大压缩缓冲区 */
#define TBF_MAX_COMPRESSION_SLOTS  64
/** 时间戳验证容差(微秒) */
#define TBF_TIMESTAMP_TOLERANCE_US 100
/** 默认压缩窗口大小 */
#define TBF_DEFAULT_WINDOW_SIZE    16

/* ============================================================================
 * 类型定义
 * ============================================================================ */

/** 时间过滤策略 */
typedef enum {
    TBF_POLICY_DROP_MIDDLE = 0,     /**< 丢弃中间值 */
    TBF_POLICY_SAMPLE_LAST,         /**< 保留最新值 */
    TBF_POLICY_SAMPLE_FIRST,        /**< 保留最早值 */
    TBF_POLICY_AVERAGE,             /**< 平均值压缩 */
    TBF_POLICY_PEAK_HOLD,           /**< 峰值保持(车载应用) */
} tbf_policy_t;

/** 时间戳来源 */
typedef enum {
    TBF_TS_SOURCE_SYSTEM = 0,       /**< 系统时钟 */
    TBF_TS_SOURCE_SAMPLE,           /**< 样本内时间戳 */
    TBF_TS_SOURCE_RECEIVE,          /**< 接收时间 */
    TBF_TS_SOURCE_SYNC,             /**< 同步时钟(PTP/gPTP) */
} tbf_timestamp_source_t;

/** 压缩算法类型 */
typedef enum {
    TBF_COMPRESS_NONE = 0,          /**< 无压缩 */
    TBF_COMPRESS_FIXED_WINDOW,      /**< 固定窗口 */
    TBF_COMPRESS_ADAPTIVE,          /**< 自适应窗口 */
    TBF_COMPRESS_DEADBAND,          /**< 死区压缩 */
    TBF_COMPRESS_KALMAN,            /**< 卡尔曼滤波(用于传感器数据) */
} tbf_compression_t;

/** 时间过滤器配置 */
typedef struct {
    uint32_t minimum_separation_us;     /**< 最小时间分隔(微秒) */
    tbf_policy_t policy;                 /**< 压缩策略 */
    tbf_timestamp_source_t ts_source;    /**< 时间戳来源 */
    tbf_compression_t compression;       /**< 压缩算法 */
    union {
        struct {
            uint32_t window_size;        /**< 窗口大小 */
        } fixed_window;
        struct {
            uint32_t min_window;         /**< 最小窗口 */
            uint32_t max_window;         /**< 最大窗口 */
            uint32_t adapt_threshold;    /**< 自适应阈值 */
        } adaptive;
        struct {
            double threshold;            /**< 死区阈值 */
            uint32_t max_skip;           /**< 最大跳过数 */
        } deadband;
        struct {
            double process_noise;        /**< 过程噪声 */
            double measurement_noise;    /**< 测量噪声 */
        } kalman;
    } compression_params;
    bool validate_timestamp;             /**< 启用时间戳验证 */
    uint32_t timestamp_tolerance_us;     /**< 时间戳容差 */
    bool enable_jitter_control;          /**< 启用抖动控制 */
    uint32_t max_jitter_us;              /**< 最大接受抖动 */
} tbf_config_t;

/** 缓存样本节点 */
typedef struct tbf_sample_node {
    uint8_t *data;                       /**< 样本数据 */
    uint32_t size;                       /**< 样本大小 */
    uint64_t timestamp_us;               /**< 时间戳(微秒) */
    uint64_t receive_time_us;            /**< 接收时间 */
    uint32_t sequence_num;               /**< 序列号 */
    struct tbf_sample_node *next;
} tbf_sample_node_t;

/** 时间过滤器状态 */
typedef struct {
    uint64_t last_output_time;           /**< 上次输出时间 */
    uint64_t window_start_time;          /**< 当前窗口开始时间 */
    uint32_t samples_in_window;          /**< 当前窗口样本数 */
    double accumulator;                  /**< 累加器(用于平均算法) */
    uint32_t max_value;                  /**< 窗口内最大值(峰值保持) */
    uint32_t min_value;                  /**< 窗口内最小值(谷值保持) */
    double kalman_estimate;              /**< 卡尔曼估计值 */
    double kalman_error;                 /**< 估计误差 */
    uint32_t last_output_value;          /**< 上次输出值(用于死区) */
    uint32_t skip_count;                 /**< 跳过计数(死区压缩) */
} tbf_state_t;

/** 时间过滤器统计 */
typedef struct {
    uint64_t total_samples;              /**< 总接收样本数 */
    uint64_t output_samples;             /**< 输出样本数 */
    uint64_t dropped_samples;            /**< 丢弃样本数 */
    uint64_t compressed_samples;         /**< 压缩合并样本数 */
    uint32_t avg_jitter_us;              /**< 平均抖动(微秒) */
    uint32_t max_jitter_us;              /**< 最大抖动(微秒) */
    uint32_t timestamp_errors;           /**< 时间戳错误数 */
    uint32_t timeout_count;              /**< 超时次数 */
} tbf_stats_t;

/** 时间过滤器句柄(不透明) */
typedef struct tbf_handle tbf_handle_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化时间过滤器模块
 * @return ETH_OK成功
 */
eth_status_t tbf_init(void);

/**
 * @brief 反初始化时间过滤器模块
 */
void tbf_deinit(void);

/**
 * @brief 创建时间过滤器
 * @param config 过滤器配置
 * @return 过滤器句柄，NULL表示失败
 */
tbf_handle_t* tbf_create(const tbf_config_t *config);

/**
 * @brief 删除时间过滤器
 * @param tbf 过滤器句柄
 * @return ETH_OK成功
 */
eth_status_t tbf_delete(tbf_handle_t *tbf);

/**
 * @brief 处理新样本(过滤决策点)
 * @param tbf 过滤器句柄
 * @param sample 样本数据
 * @param size 样本大小
 * @param timestamp_us 样本时间戳(微秒)
 * @param should_output 输出: 是否应该输出此样本
 * @param output_sample 输出: 如果有压缩，输出压缩后的样本(可为NULL表示无压缩)
 * @return ETH_OK成功
 */
eth_status_t tbf_process_sample(tbf_handle_t *tbf,
                                 const void *sample,
                                 uint32_t size,
                                 uint64_t timestamp_us,
                                 bool *should_output,
                                 void **output_sample);

/**
 * @brief 检查是否到达输出时间
 * @param tbf 过滤器句柄
 * @param current_time_us 当前时间(微秒)
 * @param should_output 输出: 是否到达输出时间
 * @return ETH_OK成功
 */
eth_status_t tbf_check_output_time(tbf_handle_t *tbf,
                                    uint64_t current_time_us,
                                    bool *should_output);

/**
 * @brief 获取窗口内压缩样本
 * @param tbf 过滤器句柄
 * @param current_time_us 当前时间(微秒)
 * @param compressed_sample 输出: 压缩后的样本
 * @param compressed_size 输出: 压缩后大小
 * @return ETH_OK成功，ETH_TIMEOUT窗口未满
 */
eth_status_t tbf_get_compressed_sample(tbf_handle_t *tbf,
                                        uint64_t current_time_us,
                                        void *compressed_sample,
                                        uint32_t *compressed_size);

/**
 * @brief 设置最小时间分隔(动态调整)
 * @param tbf 过滤器句柄
 * @param separation_us 新的时间分隔(微秒)
 * @return ETH_OK成功
 */
eth_status_t tbf_set_separation(tbf_handle_t *tbf, uint32_t separation_us);

/**
 * @brief 获取当前时间分隔
 * @param tbf 过滤器句柄
 * @param separation_us 输出: 当前时间分隔
 * @return ETH_OK成功
 */
eth_status_t tbf_get_separation(tbf_handle_t *tbf, uint32_t *separation_us);

/**
 * @brief 验证时间戳有效性
 * @param timestamp_us 要验证的时间戳
 * @param reference_time_us 参考时间(通常为当前时间)
 * @param tolerance_us 容差范围
 * @return true有效
 */
bool tbf_validate_timestamp(uint64_t timestamp_us,
                            uint64_t reference_time_us,
                            uint32_t tolerance_us);

/**
 * @brief 获取统计信息
 * @param tbf 过滤器句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t tbf_get_stats(tbf_handle_t *tbf, tbf_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param tbf 过滤器句柄
 * @return ETH_OK成功
 */
eth_status_t tbf_reset_stats(tbf_handle_t *tbf);

/**
 * @brief 立即刷新窗口(强制输出当前窗口数据)
 * @param tbf 过滤器句柄
 * @param current_time_us 当前时间
 * @param flushed_sample 输出: 刷新的样本(可为NULL)
 * @param flushed_size 输出: 刷新样本大小
 * @return ETH_OK成功
 */
eth_status_t tbf_flush_window(tbf_handle_t *tbf,
                               uint64_t current_time_us,
                               void *flushed_sample,
                               uint32_t *flushed_size);

/**
 * @brief 设置系统时钟源
 * @param source 时钟源类型
 * @return ETH_OK成功
 */
eth_status_t tbf_set_clock_source(tbf_timestamp_source_t source);

/**
 * @brief 获取当前系统时间(微秒)
 * @return 当前时间戳
 */
uint64_t tbf_get_current_time_us(void);

/**
 * @brief 配置并发压缩(多车载流)
 * @param tbf 过滤器句柄
 * @param enable 启用/禁用
 * @param max_streams 最大流数
 * @return ETH_OK成功
 */
eth_status_t tbf_configure_multistream(tbf_handle_t *tbf,
                                        bool enable,
                                        uint8_t max_streams);

/**
 * @brief 设置ASIL安全模式
 * @param tbf 过滤器句柄
 * @param asil_level ASIL等级(0-4)
 * @return ETH_OK成功
 */
eth_status_t tbf_enable_asil_mode(tbf_handle_t *tbf, uint8_t asil_level);

/**
 * @brief 获取下一个预期输出时间
 * @param tbf 过滤器句柄
 * @param next_output_time_us 输出: 下次输出时间(微秒)
 * @return ETH_OK成功
 */
eth_status_t tbf_get_next_output_time(tbf_handle_t *tbf, uint64_t *next_output_time_us);

/**
 * @brief 计算并发数据流的时间偏移
 * @param tbf 过滤器句柄
 * @param stream_id 流ID
 * @param offset_us 偏移量(微秒)
 * @return ETH_OK成功
 */
eth_status_t tbf_set_stream_offset(tbf_handle_t *tbf, uint8_t stream_id, int32_t offset_us);

#ifdef __cplusplus
}
#endif

#endif /* TIME_FILTER_H */
