/**
 * @file time_filter.c
 * @brief DDS时间过滤器实现
 * @version 1.0
 * @date 2026-04-24
 */

#define _GNU_SOURCE
#include "time_filter.h"
#include "../../common/log/dds_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * 私有数据结构定义
 * ============================================================================ */

/** 多流配置 */
typedef struct {
    bool enabled;
    uint8_t max_streams;
    int32_t stream_offsets[8];
} tbf_multistream_t;

/** 时间过滤器内部结构 */
struct tbf_handle {
    tbf_config_t config;
    tbf_state_t state;
    tbf_stats_t stats;
    tbf_multistream_t multistream;
    bool asil_enabled;
    uint8_t asil_level;
    
    // 缓冲区
    uint8_t *sample_buffer;
    uint32_t sample_buffer_size;
    tbf_sample_node_t *free_nodes;
    tbf_sample_node_t *active_head;
};

/** 全局时钟源 */
static tbf_timestamp_source_t g_clock_source = TBF_TS_SOURCE_SYSTEM;

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 获取当前系统时间(微秒)
 */
uint64_t tbf_get_current_time_us(void) {
    struct timespec ts;
    
    switch (g_clock_source) {
        case TBF_TS_SOURCE_SYNC:
            // gPTP/同步时钟
            clock_gettime(CLOCK_REALTIME, &ts);
            break;
        case TBF_TS_SOURCE_SYSTEM:
        default:
            clock_gettime(CLOCK_MONOTONIC, &ts);
            break;
    }
    
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/**
 * @brief 卡尔曼滤波更新
 */
static double kalman_update(tbf_state_t *state, double measurement, 
                            double process_noise, double measurement_noise) {
    // 预测
    double predicted_estimate = state->kalman_estimate;
    double predicted_error = state->kalman_error + process_noise;
    
    // 更新
    double kalman_gain = predicted_error / (predicted_error + measurement_noise);
    state->kalman_estimate = predicted_estimate + kalman_gain * (measurement - predicted_estimate);
    state->kalman_error = (1 - kalman_gain) * predicted_error;
    
    return state->kalman_estimate;
}

/**
 * @brief 初始化卡尔曼滤波状态
 */
static void kalman_init(tbf_state_t *state, double initial_value) {
    state->kalman_estimate = initial_value;
    state->kalman_error = 1.0;
}

/* ============================================================================
 * 窗口处理函数
 * ============================================================================ */

/**
 * @brief 检查窗口是否已满
 */
static bool is_window_full(const tbf_handle_t *tbf, uint64_t current_time_us) {
    uint64_t window_duration = current_time_us - tbf->state.window_start_time;
    return window_duration >= tbf->config.minimum_separation_us;
}

/**
 * @brief 处理窗口内样本 - 根据策略进行压缩
 */
static eth_status_t process_window_sample(tbf_handle_t *tbf,
                                          const void *sample,
                                          uint32_t size,
                                          uint64_t timestamp_us) {
    if (!tbf || !sample) return ETH_INVALID_PARAM;
    
    // 提取值(假设数据的前4字节为u32值)
    uint32_t value = 0;
    if (size >= sizeof(uint32_t)) {
        memcpy(&value, sample, sizeof(uint32_t));
    }
    
    switch (tbf->config.policy) {
        case TBF_POLICY_AVERAGE:
            tbf->state.accumulator += value;
            break;
            
        case TBF_POLICY_PEAK_HOLD:
            if (value > tbf->state.max_value) {
                tbf->state.max_value = value;
            }
            if (value < tbf->state.min_value || tbf->state.samples_in_window == 0) {
                tbf->state.min_value = value;
            }
            break;
            
        case TBF_POLICY_SAMPLE_LAST:
            // 覆盖缓冲区
            if (size <= tbf->sample_buffer_size) {
                memcpy(tbf->sample_buffer, sample, size);
            }
            break;
            
        case TBF_POLICY_SAMPLE_FIRST:
            // 只保留第一个
            if (tbf->state.samples_in_window == 0 && size <= tbf->sample_buffer_size) {
                memcpy(tbf->sample_buffer, sample, size);
            }
            break;
            
        default:
            break;
    }
    
    // 卡尔曼滤波处理
    if (tbf->config.compression == TBF_COMPRESS_KALMAN && tbf->state.samples_in_window == 0) {
        kalman_init(&tbf->state, value);
    } else if (tbf->config.compression == TBF_COMPRESS_KALMAN) {
        kalman_update(&tbf->state, value,
                     tbf->config.compression_params.kalman.process_noise,
                     tbf->config.compression_params.kalman.measurement_noise);
    }
    
    tbf->state.samples_in_window++;
    return ETH_OK;
}

/**
 * @brief 生成压缩样本
 */
static eth_status_t generate_compressed_sample(tbf_handle_t *tbf,
                                               void *output,
                                               uint32_t *output_size) {
    if (!tbf || !output || !output_size) return ETH_INVALID_PARAM;
    
    switch (tbf->config.policy) {
        case TBF_POLICY_AVERAGE:
            if (tbf->state.samples_in_window > 0) {
                uint32_t avg_value = (uint32_t)(tbf->state.accumulator / tbf->state.samples_in_window);
                memcpy(output, &avg_value, sizeof(uint32_t));
                *output_size = sizeof(uint32_t);
            }
            break;
            
        case TBF_POLICY_PEAK_HOLD:
            // 输出峰值
            memcpy(output, &tbf->state.max_value, sizeof(uint32_t));
            *output_size = sizeof(uint32_t);
            break;
            
        case TBF_POLICY_SAMPLE_LAST:
        case TBF_POLICY_SAMPLE_FIRST:
        case TBF_POLICY_DROP_MIDDLE:
        default:
            // 复制缓冲区内容
            *output_size = tbf->sample_buffer_size;
            memcpy(output, tbf->sample_buffer, tbf->sample_buffer_size);
            break;
    }
    
    // 卡尔曼滤波输出
    if (tbf->config.compression == TBF_COMPRESS_KALMAN) {
        uint32_t filtered_value = (uint32_t)tbf->state.kalman_estimate;
        memcpy(output, &filtered_value, sizeof(uint32_t));
        *output_size = sizeof(uint32_t);
    }
    
    return ETH_OK;
}

/**
 * @brief 重置窗口状态
 */
static void reset_window_state(tbf_handle_t *tbf, uint64_t new_window_start) {
    tbf->state.window_start_time = new_window_start;
    tbf->state.samples_in_window = 0;
    tbf->state.accumulator = 0.0;
    tbf->state.max_value = 0;
    tbf->state.min_value = UINT32_MAX;
}

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

eth_status_t tbf_init(void) {
    g_clock_source = TBF_TS_SOURCE_SYSTEM;
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "TimeBasedFilter module initialized");
    return ETH_OK;
}

void tbf_deinit(void) {
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "TimeBasedFilter module deinitialized");
}

tbf_handle_t* tbf_create(const tbf_config_t *config) {
    if (!config) return NULL;
    
    // 验证配置
    if (config->minimum_separation_us < TBF_MIN_SEPARATION_US) {
        DDS_LOG_ERROR(DDS_LOG_MODULE_CORE, "TBF", "Invalid minimum_separation: %u us (min: %u)",
                     config->minimum_separation_us, TBF_MIN_SEPARATION_US);
        return NULL;
    }
    
    tbf_handle_t *tbf = (tbf_handle_t*)calloc(1, sizeof(tbf_handle_t));
    if (!tbf) return NULL;
    
    memcpy(&tbf->config, config, sizeof(tbf_config_t));
    
    // 分配样本缓冲区
    tbf->sample_buffer_size = 256; // 默认大小
    tbf->sample_buffer = (uint8_t*)malloc(tbf->sample_buffer_size);
    if (!tbf->sample_buffer) {
        free(tbf);
        return NULL;
    }
    
    // 初始化状态
    tbf->state.window_start_time = tbf_get_current_time_us();
    tbf->state.last_output_time = 0;
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "Created TimeBasedFilter: separation=%u us, policy=%d",
                config->minimum_separation_us, config->policy);
    
    return tbf;
}

eth_status_t tbf_delete(tbf_handle_t *tbf) {
    if (!tbf) return ETH_INVALID_PARAM;
    
    if (tbf->sample_buffer) {
        free(tbf->sample_buffer);
    }
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "Deleted TimeBasedFilter");
    free(tbf);
    return ETH_OK;
}

eth_status_t tbf_process_sample(tbf_handle_t *tbf,
                                 const void *sample,
                                 uint32_t size,
                                 uint64_t timestamp_us,
                                 bool *should_output,
                                 void **output_sample) {
    if (!tbf || !sample || !should_output) {
        return ETH_INVALID_PARAM;
    }
    
    uint64_t current_time = tbf_get_current_time_us();
    *should_output = false;
    
    // 时间戳验证
    if (tbf->config.validate_timestamp) {
        if (!tbf_validate_timestamp(timestamp_us, current_time, 
                                    tbf->config.timestamp_tolerance_us)) {
            tbf->stats.timestamp_errors++;
            DDS_LOG_WARN(DDS_LOG_MODULE_CORE, "TBF", "Timestamp validation failed");
        }
    }
    
    // 检查抖动
    if (tbf->config.enable_jitter_control) {
        int64_t jitter = (int64_t)current_time - (int64_t)(tbf->state.last_output_time + 
                                                          tbf->config.minimum_separation_us);
        if (labs(jitter) > tbf->config.max_jitter_us) {
            DDS_LOG_WARN(DDS_LOG_MODULE_CORE, "TBF", "Jitter exceeded threshold: %ld us", jitter);
        }
    }
    
    // 检查是否到达输出时间
    bool time_to_output = is_window_full(tbf, current_time);
    
    // 处理当前样本
    if (!time_to_output) {
        // 在窗口内，进行压缩
        process_window_sample(tbf, sample, size, timestamp_us);
        tbf->stats.total_samples++;
        tbf->stats.compressed_samples++;
        
        if (tbf->config.policy == TBF_POLICY_DROP_MIDDLE) {
            *should_output = false;
        }
    } else {
        // 窗口已满，输出压缩样本并开启新窗口
        if (tbf->state.samples_in_window > 0) {
            generate_compressed_sample(tbf, tbf->sample_buffer, &size);
            if (output_sample) {
                *output_sample = tbf->sample_buffer;
            }
        }
        
        *should_output = true;
        tbf->state.last_output_time = current_time;
        tbf->stats.output_samples++;
        
        // 重置窗口
        reset_window_state(tbf, current_time);
        process_window_sample(tbf, sample, size, timestamp_us);
    }
    
    return ETH_OK;
}

eth_status_t tbf_check_output_time(tbf_handle_t *tbf,
                                    uint64_t current_time_us,
                                    bool *should_output) {
    if (!tbf || !should_output) return ETH_INVALID_PARAM;
    
    *should_output = is_window_full(tbf, current_time_us);
    return ETH_OK;
}

eth_status_t tbf_get_compressed_sample(tbf_handle_t *tbf,
                                        uint64_t current_time_us,
                                        void *compressed_sample,
                                        uint32_t *compressed_size) {
    if (!tbf || !compressed_sample || !compressed_size) {
        return ETH_INVALID_PARAM;
    }
    
    if (!is_window_full(tbf, current_time_us)) {
        return ETH_TIMEOUT;
    }
    
    if (tbf->state.samples_in_window == 0) {
        return ETH_ERROR;
    }
    
    generate_compressed_sample(tbf, compressed_sample, compressed_size);
    
    // 重置窗口
    tbf->state.last_output_time = current_time_us;
    reset_window_state(tbf, current_time_us);
    
    return ETH_OK;
}

eth_status_t tbf_set_separation(tbf_handle_t *tbf, uint32_t separation_us) {
    if (!tbf || separation_us < TBF_MIN_SEPARATION_US) {
        return ETH_INVALID_PARAM;
    }
    
    tbf->config.minimum_separation_us = separation_us;
    
    // 重新计算窗口
    reset_window_state(tbf, tbf_get_current_time_us());
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "Updated separation: %u us", separation_us);
    return ETH_OK;
}

eth_status_t tbf_get_separation(tbf_handle_t *tbf, uint32_t *separation_us) {
    if (!tbf || !separation_us) return ETH_INVALID_PARAM;
    
    *separation_us = tbf->config.minimum_separation_us;
    return ETH_OK;
}

bool tbf_validate_timestamp(uint64_t timestamp_us,
                            uint64_t reference_time_us,
                            uint32_t tolerance_us) {
    // 检查时间戳在合理范围内
    int64_t diff = (int64_t)timestamp_us - (int64_t)reference_time_us;
    
    // 不接受太未来的时间戳(超过容差)
    if (diff > (int64_t)tolerance_us) {
        return false;
    }
    
    // 不接受太过去的时间戳(超过1秒)
    if (diff < -1000000) {
        return false;
    }
    
    return true;
}

eth_status_t tbf_get_stats(tbf_handle_t *tbf, tbf_stats_t *stats) {
    if (!tbf || !stats) return ETH_INVALID_PARAM;
    
    memcpy(stats, &tbf->stats, sizeof(tbf_stats_t));
    return ETH_OK;
}

eth_status_t tbf_reset_stats(tbf_handle_t *tbf) {
    if (!tbf) return ETH_INVALID_PARAM;
    
    memset(&tbf->stats, 0, sizeof(tbf_stats_t));
    return ETH_OK;
}

eth_status_t tbf_flush_window(tbf_handle_t *tbf,
                               uint64_t current_time_us,
                               void *flushed_sample,
                               uint32_t *flushed_size) {
    if (!tbf) return ETH_INVALID_PARAM;
    
    if (tbf->state.samples_in_window == 0) {
        if (flushed_size) *flushed_size = 0;
        return ETH_OK;
    }
    
    if (flushed_sample && flushed_size) {
        generate_compressed_sample(tbf, flushed_sample, flushed_size);
    }
    
    tbf->state.last_output_time = current_time_us;
    reset_window_state(tbf, current_time_us);
    
    return ETH_OK;
}

eth_status_t tbf_set_clock_source(tbf_timestamp_source_t source) {
    g_clock_source = source;
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "Set clock source: %d", source);
    return ETH_OK;
}

eth_status_t tbf_configure_multistream(tbf_handle_t *tbf,
                                        bool enable,
                                        uint8_t max_streams) {
    if (!tbf || max_streams > 8) return ETH_INVALID_PARAM;
    
    tbf->multistream.enabled = enable;
    tbf->multistream.max_streams = max_streams;
    
    if (enable) {
        memset(tbf->multistream.stream_offsets, 0, sizeof(tbf->multistream.stream_offsets));
    }
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "Multistream %s (max_streams=%d)", enable ? "enabled" : "disabled", max_streams);
    return ETH_OK;
}

eth_status_t tbf_enable_asil_mode(tbf_handle_t *tbf, uint8_t asil_level) {
    if (!tbf || asil_level > 4) return ETH_INVALID_PARAM;
    
    tbf->asil_enabled = true;
    tbf->asil_level = asil_level;
    
    // ASIL模式配置
    tbf->config.validate_timestamp = true;
    tbf->config.enable_jitter_control = true;
    
    switch (asil_level) {
        case 4: // ASIL-D
            tbf->config.timestamp_tolerance_us = 10;  // 10us
            tbf->config.max_jitter_us = 50;
            break;
        case 3: // ASIL-C
            tbf->config.timestamp_tolerance_us = 50;
            tbf->config.max_jitter_us = 100;
            break;
        case 2: // ASIL-B
            tbf->config.timestamp_tolerance_us = 100;
            tbf->config.max_jitter_us = 200;
            break;
        default:
            tbf->config.timestamp_tolerance_us = TBF_TIMESTAMP_TOLERANCE_US;
            tbf->config.max_jitter_us = 500;
            break;
    }
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "TBF", "ASIL mode enabled (level=%d)", asil_level);
    return ETH_OK;
}

eth_status_t tbf_get_next_output_time(tbf_handle_t *tbf, uint64_t *next_output_time_us) {
    if (!tbf || !next_output_time_us) return ETH_INVALID_PARAM;
    
    *next_output_time_us = tbf->state.last_output_time + tbf->config.minimum_separation_us;
    return ETH_OK;
}

eth_status_t tbf_set_stream_offset(tbf_handle_t *tbf, uint8_t stream_id, int32_t offset_us) {
    if (!tbf || stream_id >= 8) return ETH_INVALID_PARAM;
    
    if (!tbf->multistream.enabled) {
        return ETH_ERROR;
    }
    
    tbf->multistream.stream_offsets[stream_id] = offset_us;
    return ETH_OK;
}
