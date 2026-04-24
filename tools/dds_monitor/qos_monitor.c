/*******************************************************************************
 * @file qos_monitor.c
 * @brief DDS QoS性能监控实现 - 低开销设计
 ******************************************************************************/

#include "qos_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/*==============================================================================
 * 常量和宏
 *============================================================================*/
#define DDS_QOS_MAGIC           0x514F5300  /* "QOS" */
#define MAX_CALLBACKS           32
#define GRAPH_HISTORY_SIZE      100
#define DEFAULT_SAMPLE_US       1000        /* 1ms采样间隔 */
#define LOW_OVERHEAD_US         10000       /* 10ms低开销模式 */

/*==============================================================================
 * 环形缓冲区结构
 *============================================================================*/
typedef struct {
    uint64_t*   data;
    uint32_t    size;
    uint32_t    head;
    uint32_t    count;
    pthread_mutex_t mutex;
} ring_buffer_t;

/*==============================================================================
 * 监控器结构
 *============================================================================*/
struct dds_qos_monitor {
    uint32_t            magic;
    dds_qos_obj_info_t  info;
    
    /* 采样数据 */
    ring_buffer_t       latency_samples;
    ring_buffer_t       throughput_samples;
    
    /* 序列号跟踪 */
    uint64_t            last_sequence;
    uint64_t            expected_sequence;
    
    /* 状态 */
    dds_qos_status_t    current_status;
    pthread_mutex_t     status_mutex;
    
    /* 连接状态 */
    volatile bool       connected;
    uint64_t            connection_start_time;
    uint64_t            total_uptime;
    uint64_t            total_downtime;
    
    /* 性能预算 */
    uint64_t            latency_budget_us;
    double              throughput_budget_mbps;
    uint32_t            budget_violations;
    
    /* 历史数据（用于图表） */
    uint64_t            history_timestamps[GRAPH_HISTORY_SIZE];
    double              history_values[GRAPH_HISTORY_SIZE];
    uint32_t            history_count;
    uint32_t            history_index;
    
    /* 统计更新时间 */
    uint64_t            last_stats_update;
};

/*==============================================================================
 * 全局状态
 *============================================================================*/
typedef struct {
    bool                    initialized;
    dds_qos_config_t        config;
    
    pthread_mutex_t         monitors_mutex;
    dds_qos_monitor_t*      monitors[DDS_QOS_MAX_ENTITIES];
    uint32_t                monitor_count;
    
    pthread_mutex_t         callbacks_mutex;
    struct {
        dds_qos_callback_t          callback;
        uint64_t                    interval_us;
        uint64_t                    last_call;
        void*                       user_data;
    } callbacks[MAX_CALLBACKS];
    
    struct {
        dds_qos_alert_callback_t    callback;
        void*                       user_data;
    } alert_callbacks[MAX_CALLBACKS];
    uint32_t alert_callback_count;
    
    pthread_t               worker_thread;
    volatile bool           worker_running;
} qos_global_state_t;

static qos_global_state_t g_qos_state = {0};

/*==============================================================================
 * 内部函数前向声明
 *============================================================================*/
static void* qos_worker_thread(void* arg);
static void update_latency_stats(dds_qos_monitor_t* monitor);
static void update_throughput_stats(dds_qos_monitor_t* monitor);
static void update_loss_stats(dds_qos_monitor_t* monitor);
static void update_jitter_stats(dds_qos_monitor_t* monitor);
static void update_availability_stats(dds_qos_monitor_t* monitor);
static uint64_t get_timestamp_us(void);
static void check_alerts(dds_qos_monitor_t* monitor);

/*==============================================================================
 * 环形缓冲区操作
 *============================================================================*/

static int ring_buffer_init(ring_buffer_t* rb, uint32_t size) {
    rb->data = calloc(size, sizeof(uint64_t));
    if (!rb->data) return -1;
    rb->size = size;
    rb->head = 0;
    rb->count = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    return 0;
}

static void ring_buffer_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
}

static void ring_buffer_push(ring_buffer_t* rb, uint64_t value) {
    pthread_mutex_lock(&rb->mutex);
    rb->data[rb->head] = value;
    rb->head = (rb->head + 1) % rb->size;
    if (rb->count < rb->size) rb->count++;
    pthread_mutex_unlock(&rb->mutex);
}

static uint32_t ring_buffer_get(ring_buffer_t* rb, uint64_t* values, uint32_t max_count) {
    pthread_mutex_lock(&rb->mutex);
    uint32_t count = (rb->count < max_count) ? rb->count : max_count;
    for (uint32_t i = 0; i < count; i++) {
        int idx = (rb->head - rb->count + i + rb->size) % rb->size;
        values[i] = rb->data[idx];
    }
    pthread_mutex_unlock(&rb->mutex);
    return count;
}

/*==============================================================================
 * 时间工具
 *============================================================================*/

static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/*==============================================================================
 * 初始化和生命周期
 *============================================================================*/

int dds_qos_monitor_init(const dds_qos_config_t* config) {
    if (g_qos_state.initialized) {
        return 0;
    }
    
    /* 复制配置 */
    if (config) {
        memcpy(&g_qos_state.config, config, sizeof(dds_qos_config_t));
    } else {
        /* 默认配置 */
        g_qos_state.config.sample_window_size = DDS_QOS_SAMPLE_WINDOW_SIZE;
        g_qos_state.config.sampling_interval_us = DEFAULT_SAMPLE_US;
        g_qos_state.config.report_interval_us = 1000000;  /* 1秒 */
        g_qos_state.config.latency_warning_us = DDS_QOS_LATENCY_WARNING_US;
        g_qos_state.config.latency_critical_us = DDS_QOS_LATENCY_CRITICAL_US;
        g_qos_state.config.loss_warning_ppm = DDS_QOS_LOSS_WARNING_PPM;
        g_qos_state.config.loss_critical_ppm = DDS_QOS_LOSS_CRITICAL_PPM;
        g_qos_state.config.enable_low_overhead = false;
        g_qos_state.config.cpu_limit_percent = 1;  /* <1% CPU */
    }
    
    pthread_mutex_init(&g_qos_state.monitors_mutex, NULL);
    pthread_mutex_init(&g_qos_state.callbacks_mutex, NULL);
    
    /* 启动工作线程 */
    g_qos_state.worker_running = true;
    pthread_create(&g_qos_state.worker_thread, NULL, qos_worker_thread, NULL);
    
    g_qos_state.initialized = true;
    return 0;
}

void dds_qos_monitor_deinit(void) {
    if (!g_qos_state.initialized) {
        return;
    }
    
    /* 停止工作线程 */
    g_qos_state.worker_running = false;
    pthread_join(g_qos_state.worker_thread, NULL);
    
    /* 清理所有监控器 */
    pthread_mutex_lock(&g_qos_state.monitors_mutex);
    for (uint32_t i = 0; i < g_qos_state.monitor_count; i++) {
        dds_qos_monitor_destroy(g_qos_state.monitors[i]);
    }
    g_qos_state.monitor_count = 0;
    pthread_mutex_unlock(&g_qos_state.monitors_mutex);
    
    pthread_mutex_destroy(&g_qos_state.monitors_mutex);
    pthread_mutex_destroy(&g_qos_state.callbacks_mutex);
    
    g_qos_state.initialized = false;
}

dds_qos_monitor_t* dds_qos_monitor_create(const dds_qos_obj_info_t* info) {
    if (!g_qos_state.initialized) {
        return NULL;
    }
    
    dds_qos_monitor_t* monitor = calloc(1, sizeof(dds_qos_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    monitor->magic = DDS_QOS_MAGIC;
    if (info) {
        memcpy(&monitor->info, info, sizeof(dds_qos_obj_info_t));
    }
    
    /* 初始化环形缓冲区 */
    uint32_t sample_size = g_qos_state.config.sample_window_size;
    if (g_qos_state.config.enable_low_overhead) {
        sample_size = sample_size / 10;  /* 减少采样数以降低开销 */
    }
    
    ring_buffer_init(&monitor->latency_samples, sample_size);
    ring_buffer_init(&monitor->throughput_samples, sample_size);
    
    pthread_mutex_init(&monitor->status_mutex, NULL);
    
    monitor->last_sequence = 0;
    monitor->expected_sequence = 1;
    monitor->connected = false;
    monitor->connection_start_time = 0;
    
    /* 添加到全局列表 */
    pthread_mutex_lock(&g_qos_state.monitors_mutex);
    if (g_qos_state.monitor_count < DDS_QOS_MAX_ENTITIES) {
        g_qos_state.monitors[g_qos_state.monitor_count++] = monitor;
    }
    pthread_mutex_unlock(&g_qos_state.monitors_mutex);
    
    return monitor;
}

void dds_qos_monitor_destroy(dds_qos_monitor_t* monitor) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) {
        return;
    }
    
    /* 从全局列表移除 */
    pthread_mutex_lock(&g_qos_state.monitors_mutex);
    for (uint32_t i = 0; i < g_qos_state.monitor_count; i++) {
        if (g_qos_state.monitors[i] == monitor) {
            g_qos_state.monitors[i] = g_qos_state.monitors[--g_qos_state.monitor_count];
            break;
        }
    }
    pthread_mutex_unlock(&g_qos_state.monitors_mutex);
    
    /* 清理资源 */
    ring_buffer_destroy(&monitor->latency_samples);
    ring_buffer_destroy(&monitor->throughput_samples);
    pthread_mutex_destroy(&monitor->status_mutex);
    
    monitor->magic = 0;
    free(monitor);
}

/*==============================================================================
 * 数据采集
 *============================================================================*/

void dds_qos_record_latency(dds_qos_monitor_t* monitor, uint64_t latency_us) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    ring_buffer_push(&monitor->latency_samples, latency_us);
    
    /* 记录历史 */
    monitor->history_timestamps[monitor->history_index] = get_timestamp_us();
    monitor->history_values[monitor->history_index] = (double)latency_us;
    monitor->history_index = (monitor->history_index + 1) % GRAPH_HISTORY_SIZE;
    if (monitor->history_count < GRAPH_HISTORY_SIZE) monitor->history_count++;
    
    /* 检查预算 */
    if (monitor->latency_budget_us > 0 && latency_us > monitor->latency_budget_us) {
        __sync_fetch_and_add(&monitor->budget_violations, 1);
    }
}

void dds_qos_record_send(dds_qos_monitor_t* monitor, uint64_t timestamp_us, uint32_t size) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    (void)timestamp_us;
    ring_buffer_push(&monitor->throughput_samples, (uint64_t)size);
    
    pthread_mutex_lock(&monitor->status_mutex);
    monitor->current_status.throughput.total_messages++;
    monitor->current_status.throughput.total_bytes += size;
    pthread_mutex_unlock(&monitor->status_mutex);
}

void dds_qos_record_receive(dds_qos_monitor_t* monitor, uint64_t timestamp_us,
                            uint32_t size, uint64_t sequence_num) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    (void)timestamp_us;
    (void)size;
    
    /* 更新序列号统计 */
    pthread_mutex_lock(&monitor->status_mutex);
    
    monitor->current_status.loss.expected_sequences++;
    
    if (sequence_num > monitor->last_sequence) {
        /* 正常顺序 */
        uint64_t gap = sequence_num - monitor->last_sequence - 1;
        if (gap > 0 && monitor->last_sequence != 0) {
            monitor->current_status.loss.lost_sequences += gap;
        }
        monitor->last_sequence = sequence_num;
        monitor->current_status.loss.received_sequences++;
        monitor->current_status.loss.consecutive_received++;
        monitor->current_status.loss.consecutive_losses = 0;
        
        if (monitor->current_status.loss.consecutive_received > 
            monitor->current_status.loss.max_consecutive_received) {
            monitor->current_status.loss.max_consecutive_received = 
                monitor->current_status.loss.consecutive_received;
        }
    } else if (sequence_num == monitor->last_sequence) {
        /* 重复 */
        monitor->current_status.loss.duplicate_sequences++;
    } else {
        /* 乱序 */
        monitor->current_status.loss.out_of_order_sequences++;
    }
    
    pthread_mutex_unlock(&monitor->status_mutex);
}

void dds_qos_record_connection_state(dds_qos_monitor_t* monitor, bool connected) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    uint64_t now = get_timestamp_us();
    
    pthread_mutex_lock(&monitor->status_mutex);
    
    if (monitor->connected != connected) {
        if (connected) {
            /* 连接建立 */
            monitor->connection_start_time = now;
            monitor->current_status.availability.down_to_up_count++;
            monitor->current_status.availability.is_available = true;
        } else {
            /* 连接断开 */
            uint64_t uptime = now - monitor->connection_start_time;
            monitor->total_uptime += uptime;
            monitor->current_status.availability.up_to_down_count++;
            monitor->current_status.availability.is_available = false;
        }
        monitor->connected = connected;
    }
    
    pthread_mutex_unlock(&monitor->status_mutex);
}

/*==============================================================================
 * 统计更新
 *============================================================================*/

static int compare_uint64(const void* a, const void* b) {
    uint64_t va = *(const uint64_t*)a;
    uint64_t vb = *(const uint64_t*)b;
    return (va < vb) ? -1 : (va > vb) ? 1 : 0;
}

static void update_latency_stats(dds_qos_monitor_t* monitor) {
    uint64_t samples[DDS_QOS_SAMPLE_WINDOW_SIZE];
    uint32_t count = ring_buffer_get(&monitor->latency_samples, samples, 
                                     DDS_QOS_SAMPLE_WINDOW_SIZE);
    
    if (count == 0) return;
    
    /* 排序以计算百分位 */
    qsort(samples, count, sizeof(uint64_t), compare_uint64);
    
    dds_latency_stats_t* stats = &monitor->current_status.latency;
    
    /* 基础统计 */
    stats->min_us = samples[0];
    stats->max_us = samples[count - 1];
    
    double sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    stats->avg_us = sum / count;
    
    /* 百分位数 */
    stats->p50_us = samples[count * 50 / 100];
    stats->p90_us = samples[count * 90 / 100];
    stats->p95_us = samples[count * 95 / 100];
    stats->p99_us = samples[count * 99 / 100];
    stats->p999_us = samples[count * 999 / 1000];
    if (count >= 10000) {
        stats->p9999_us = samples[count * 9999 / 10000];
    }
    
    /* 标准差 */
    double variance = 0;
    for (uint32_t i = 0; i < count; i++) {
        double diff = samples[i] - stats->avg_us;
        variance += diff * diff;
    }
    stats->std_dev_us = sqrt(variance / count);
    
    stats->sample_count = count;
    stats->last_sample_time = get_timestamp_us();
    
    /* 更新历史极值 */
    if (stats->history_min_us == 0 || stats->min_us < stats->history_min_us) {
        stats->history_min_us = stats->min_us;
    }
    if (stats->max_us > stats->history_max_us) {
        stats->history_max_us = stats->max_us;
    }
}

static void update_throughput_stats(dds_qos_monitor_t* monitor) {
    dds_throughput_stats_t* stats = &monitor->current_status.throughput;
    uint64_t now = get_timestamp_us();
    uint64_t duration = now - monitor->last_stats_update;
    
    if (duration > 0) {
        stats->current_msg_per_sec = (double)stats->total_messages * 1000000.0 / duration;
        stats->current_bytes_per_sec = (double)stats->total_bytes * 1000000.0 / duration;
        stats->current_mbps = stats->current_bytes_per_sec * 8.0 / 1000000.0;
        
        /* 更新峰值 */
        if (stats->current_msg_per_sec > stats->peak_msg_per_sec) {
            stats->peak_msg_per_sec = stats->current_msg_per_sec;
        }
        if (stats->current_bytes_per_sec > stats->peak_bytes_per_sec) {
            stats->peak_bytes_per_sec = stats->current_bytes_per_sec;
            stats->peak_mbps = stats->peak_bytes_per_sec * 8.0 / 1000000.0;
        }
        
        /* 平均速率 */
        uint64_t total_duration = stats->total_duration_us + duration;
        if (total_duration > 0) {
            stats->avg_msg_per_sec = (double)stats->total_messages * 1000000.0 / total_duration;
            stats->avg_bytes_per_sec = (double)stats->total_bytes * 1000000.0 / total_duration;
        }
        
        stats->measurement_interval_us = duration;
    }
}

static void update_loss_stats(dds_qos_monitor_t* monitor) {
    dds_loss_stats_t* stats = &monitor->current_status.loss;
    
    if (stats->expected_sequences > 0) {
        stats->loss_rate_ppm = (uint32_t)(stats->lost_sequences * 1000000ULL / 
                                          stats->expected_sequences);
    }
    
    if (stats->received_sequences > 0) {
        stats->duplicate_rate_ppm = (uint32_t)(stats->duplicate_sequences * 1000000ULL /
                                               stats->received_sequences);
        stats->ooo_rate_ppm = (uint32_t)(stats->out_of_order_sequences * 1000000ULL /
                                         stats->received_sequences);
    }
    
    /* 健康评分 */
    uint32_t score = 100;
    if (stats->loss_rate_ppm > 1000) score -= 20;
    if (stats->loss_rate_ppm > 10000) score -= 40;
    if (stats->consecutive_losses > 10) score -= 20;
    stats->health_score = score;
    stats->connection_healthy = (score >= 70);
}

static void update_jitter_stats(dds_qos_monitor_t* monitor) {
    dds_jitter_stats_t* stats = &monitor->current_status.jitter;
    
    /* 基于延迟样本计算抖动 */
    if (monitor->latency_samples.count < 2) return;
    
    uint64_t samples[DDS_QOS_SAMPLE_WINDOW_SIZE];
    uint32_t count = ring_buffer_get(&monitor->latency_samples, samples, 
                                     DDS_QOS_SAMPLE_WINDOW_SIZE);
    
    /* 计算相邻样本的差值 */
    uint64_t sum_diff = 0;
    uint64_t max_diff = 0;
    uint64_t min_diff = UINT64_MAX;
    
    for (uint32_t i = 1; i < count && i < 1000; i++) {  /* 限制计算量 */
        uint64_t diff = (samples[i] > samples[i-1]) ? 
                        (samples[i] - samples[i-1]) : 
                        (samples[i-1] - samples[i]);
        sum_diff += diff;
        if (diff > max_diff) max_diff = diff;
        if (diff < min_diff) min_diff = diff;
    }
    
    if (count > 1) {
        stats->avg_jitter_us = (double)sum_diff / (count - 1);
        stats->max_jitter_us = max_diff;
        stats->min_jitter_us = (min_diff == UINT64_MAX) ? 0 : min_diff;
    }
    
    /* 稳定性评分 */
    stats->stability_score = (stats->avg_jitter_us < 100) ? 100 :
                              (stats->avg_jitter_us < 500) ? 80 :
                              (stats->avg_jitter_us < 1000) ? 60 : 40;
    stats->is_stable = (stats->stability_score >= 70);
    
    stats->sample_count = count;
}

static void update_availability_stats(dds_qos_monitor_t* monitor) {
    dds_availability_stats_t* stats = &monitor->current_status.availability;
    uint64_t now = get_timestamp_us();
    
    /* 更新当前运行时间 */
    if (monitor->connected && monitor->connection_start_time > 0) {
        uint64_t current_uptime = now - monitor->connection_start_time;
        uint64_t total_uptime = monitor->total_uptime + current_uptime;
        uint64_t total_time = total_uptime + monitor->total_downtime;
        
        if (total_time > 0) {
            stats->availability_percent = (double)total_uptime * 100.0 / total_time;
        }
        
        /* 计算MTBF/MTTR */
        uint32_t failure_count = stats->up_to_down_count;
        if (failure_count > 0) {
            stats->mtbf_us = (double)total_uptime / failure_count;
        }
        
        uint32_t recovery_count = stats->down_to_up_count;
        if (recovery_count > 0) {
            stats->mttr_us = (double)monitor->total_downtime / recovery_count;
        }
    }
}

/*==============================================================================
 * 工作线程
 *============================================================================*/

static void* qos_worker_thread(void* arg) {
    (void)arg;
    
    uint64_t last_update = get_timestamp_us();
    uint64_t interval = g_qos_state.config.sampling_interval_us;
    
    if (g_qos_state.config.enable_low_overhead) {
        interval = LOW_OVERHEAD_US;  /* 10ms间隔降低CPU使用 */
    }
    
    while (g_qos_state.worker_running) {
        uint64_t now = get_timestamp_us();
        
        if (now - last_update >= interval) {
            pthread_mutex_lock(&g_qos_state.monitors_mutex);
            
            for (uint32_t i = 0; i < g_qos_state.monitor_count; i++) {
                dds_qos_monitor_t* monitor = g_qos_state.monitors[i];
                
                pthread_mutex_lock(&monitor->status_mutex);
                
                update_latency_stats(monitor);
                update_throughput_stats(monitor);
                update_loss_stats(monitor);
                update_jitter_stats(monitor);
                update_availability_stats(monitor);
                
                /* 计算综合评分 */
                dds_qos_status_t* status = &monitor->current_status;
                status->latency_score = (status->latency.p99_us < 1000) ? 100 :
                                        (status->latency.p99_us < 5000) ? 80 :
                                        (status->latency.p99_us < 10000) ? 60 : 40;
                
                status->throughput_score = (status->throughput.current_mbps > 100) ? 100 :
                                           (status->throughput.current_mbps > 50) ? 80 :
                                           (status->throughput.current_mbps > 10) ? 60 : 40;
                
                status->reliability_score = status->loss.health_score;
                
                status->overall_score = (status->latency_score + 
                                         status->throughput_score + 
                                         status->reliability_score) / 3;
                
                /* 检查告警 */
                check_alerts(monitor);
                
                pthread_mutex_unlock(&monitor->status_mutex);
            }
            
            pthread_mutex_unlock(&g_qos_state.monitors_mutex);
            
            /* 调用回调 */
            pthread_mutex_lock(&g_qos_state.callbacks_mutex);
            for (int i = 0; i < MAX_CALLBACKS; i++) {
                if (g_qos_state.callbacks[i].callback &&
                    now - g_qos_state.callbacks[i].last_call >= 
                    g_qos_state.callbacks[i].interval_us) {
                    
                    /* 调用所有监控器的回调 */
                    for (uint32_t j = 0; j < g_qos_state.monitor_count; j++) {
                        dds_qos_monitor_t* monitor = g_qos_state.monitors[j];
                        pthread_mutex_lock(&monitor->status_mutex);
                        g_qos_state.callbacks[i].callback(&monitor->current_status,
                                                          g_qos_state.callbacks[i].user_data);
                        pthread_mutex_unlock(&monitor->status_mutex);
                    }
                    
                    g_qos_state.callbacks[i].last_call = now;
                }
            }
            pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
            
            last_update = now;
        }
        
        /* 自适应休眠 */
        uint64_t sleep_us = interval / 10;
        if (sleep_us > 1000) sleep_us = 1000;
        usleep(sleep_us);
    }
    
    return NULL;
}

/*==============================================================================
 * 告警检查
 *============================================================================*/

static void check_alerts(dds_qos_monitor_t* monitor) {
    dds_qos_status_t* status = &monitor->current_status;
    
    /* 重置告警状态 */
    status->has_warnings = false;
    status->has_errors = false;
    status->has_critical = false;
    status->warning_count = 0;
    status->error_count = 0;
    status->critical_count = 0;
    
    /* 检查延迟 */
    if (status->latency.p99_us > g_qos_state.config.latency_critical_us) {
        status->has_critical = true;
        status->critical_count++;
        
        pthread_mutex_lock(&g_qos_state.callbacks_mutex);
        for (uint32_t i = 0; i < g_qos_state.alert_callback_count; i++) {
            if (g_qos_state.alert_callbacks[i].callback) {
                g_qos_state.alert_callbacks[i].callback(
                    DDS_QOS_METRIC_LATENCY, 3,
                    "Latency p99 exceeds critical threshold",
                    g_qos_state.alert_callbacks[i].user_data
                );
            }
        }
        pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
    } else if (status->latency.p99_us > g_qos_state.config.latency_warning_us) {
        status->has_warnings = true;
        status->warning_count++;
    }
    
    /* 检查丢包率 */
    if (status->loss.loss_rate_ppm > g_qos_state.config.loss_critical_ppm) {
        status->has_critical = true;
        status->critical_count++;
    } else if (status->loss.loss_rate_ppm > g_qos_state.config.loss_warning_ppm) {
        status->has_warnings = true;
        status->warning_count++;
    }
}

/*==============================================================================
 * 状态查询
 *============================================================================*/

void dds_qos_get_status(dds_qos_monitor_t* monitor, dds_qos_status_t* status) {
    if (!monitor || !status || monitor->magic != DDS_QOS_MAGIC) return;
    
    pthread_mutex_lock(&monitor->status_mutex);
    memcpy(status, &monitor->current_status, sizeof(dds_qos_status_t));
    pthread_mutex_unlock(&monitor->status_mutex);
}

void dds_qos_get_latency_stats(dds_qos_monitor_t* monitor, dds_latency_stats_t* stats) {
    if (!monitor || !stats || monitor->magic != DDS_QOS_MAGIC) return;
    
    pthread_mutex_lock(&monitor->status_mutex);
    memcpy(stats, &monitor->current_status.latency, sizeof(dds_latency_stats_t));
    pthread_mutex_unlock(&monitor->status_mutex);
}

void dds_qos_get_throughput_stats(dds_qos_monitor_t* monitor, dds_throughput_stats_t* stats) {
    if (!monitor || !stats || monitor->magic != DDS_QOS_MAGIC) return;
    
    pthread_mutex_lock(&monitor->status_mutex);
    memcpy(stats, &monitor->current_status.throughput, sizeof(dds_throughput_stats_t));
    pthread_mutex_unlock(&monitor->status_mutex);
}

void dds_qos_get_loss_stats(dds_qos_monitor_t* monitor, dds_loss_stats_t* stats) {
    if (!monitor || !stats || monitor->magic != DDS_QOS_MAGIC) return;
    
    pthread_mutex_lock(&monitor->status_mutex);
    memcpy(stats, &monitor->current_status.loss, sizeof(dds_loss_stats_t));
    pthread_mutex_unlock(&monitor->status_mutex);
}

/*==============================================================================
 * 回调注册
 *============================================================================*/

int dds_qos_register_callback(dds_qos_callback_t callback, uint64_t interval_ms, void* user_data) {
    if (!callback) return -1;
    
    pthread_mutex_lock(&g_qos_state.callbacks_mutex);
    
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_qos_state.callbacks[i].callback == NULL) {
            g_qos_state.callbacks[i].callback = callback;
            g_qos_state.callbacks[i].interval_us = interval_ms * 1000;
            g_qos_state.callbacks[i].last_call = 0;
            g_qos_state.callbacks[i].user_data = user_data;
            pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
    return -1;  /* 回调表已满 */
}

void dds_qos_unregister_callback(dds_qos_callback_t callback) {
    pthread_mutex_lock(&g_qos_state.callbacks_mutex);
    
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_qos_state.callbacks[i].callback == callback) {
            g_qos_state.callbacks[i].callback = NULL;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
}

int dds_qos_register_alert_callback(dds_qos_alert_callback_t callback, void* user_data) {
    if (!callback) return -1;
    
    pthread_mutex_lock(&g_qos_state.callbacks_mutex);
    
    if (g_qos_state.alert_callback_count < MAX_CALLBACKS) {
        g_qos_state.alert_callbacks[g_qos_state.alert_callback_count].callback = callback;
        g_qos_state.alert_callbacks[g_qos_state.alert_callback_count].user_data = user_data;
        g_qos_state.alert_callback_count++;
        pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
    return -1;
}

void dds_qos_unregister_alert_callback(dds_qos_alert_callback_t callback) {
    pthread_mutex_lock(&g_qos_state.callbacks_mutex);
    
    for (uint32_t i = 0; i < g_qos_state.alert_callback_count; i++) {
        if (g_qos_state.alert_callbacks[i].callback == callback) {
            /* 移动后续元素 */
            for (uint32_t j = i; j < g_qos_state.alert_callback_count - 1; j++) {
                g_qos_state.alert_callbacks[j] = g_qos_state.alert_callbacks[j + 1];
            }
            g_qos_state.alert_callback_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_qos_state.callbacks_mutex);
}

/*==============================================================================
 * 图表生成（简化版本）
 *============================================================================*/

int dds_qos_generate_graph(dds_qos_monitor_t* monitor,
                           dds_qos_metric_type_t metric,
                           const dds_graph_config_t* config,
                           char* buffer, size_t buffer_size) {
    if (!monitor || !config || !buffer || buffer_size == 0 || monitor->magic != DDS_QOS_MAGIC) {
        return -1;
    }
    
    (void)metric;
    
    int written = 0;
    
    /* 标题 */
    written += snprintf(buffer + written, buffer_size - written,
                        "=== %s ===\n", config->title);
    
    /* Y轴标签 */
    if (config->show_labels) {
        written += snprintf(buffer + written, buffer_size - written,
                            "%s\n", config->y_label);
    }
    
    /* 简单的折线图 */
    if (monitor->history_count > 0) {
        double max_val = config->y_max;
        double min_val = config->y_min;
        
        /* 自动缩放 */
        if (config->auto_scale) {
            max_val = monitor->history_values[0];
            min_val = monitor->history_values[0];
            for (uint32_t i = 1; i < monitor->history_count; i++) {
                if (monitor->history_values[i] > max_val) max_val = monitor->history_values[i];
                if (monitor->history_values[i] < min_val) min_val = monitor->history_values[i];
            }
            if (max_val == min_val) max_val = min_val + 1;
        }
        
        /* 绘制图表 */
        uint32_t points = (monitor->history_count < config->width) ? 
                          monitor->history_count : config->width;
        
        for (int row = config->height; row >= 0; row--) {
            double threshold = min_val + (max_val - min_val) * row / config->height;
            written += snprintf(buffer + written, buffer_size - written, "%6.0f | ", threshold);
            
            for (uint32_t col = 0; col < points; col++) {
                uint32_t idx = (monitor->history_index + col) % GRAPH_HISTORY_SIZE;
                double val = monitor->history_values[idx];
                if (val >= threshold) {
                    written += snprintf(buffer + written, buffer_size - written, "*");
                } else {
                    written += snprintf(buffer + written, buffer_size - written, " ");
                }
            }
            written += snprintf(buffer + written, buffer_size - written, "\n");
        }
        
        /* X轴 */
        written += snprintf(buffer + written, buffer_size - written, "       +");
        for (uint32_t i = 0; i < points; i++) {
            written += snprintf(buffer + written, buffer_size - written, "-");
        }
        written += snprintf(buffer + written, buffer_size - written, "\n");
        
        if (config->show_labels) {
            written += snprintf(buffer + written, buffer_size - written,
                                "        %s\n", config->x_label);
        }
    }
    
    return written;
}

/*==============================================================================
 * 报告生成（简化版本）
 *============================================================================*/

int dds_qos_generate_text_report(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size) {
    if (!monitor || !buffer || buffer_size == 0 || monitor->magic != DDS_QOS_MAGIC) {
        return -1;
    }
    
    dds_qos_status_t status;
    dds_qos_get_status(monitor, &status);
    
    int written = 0;
    
    written += snprintf(buffer + written, buffer_size - written,
                        "========================================\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "DDS QoS Report: %s\n", monitor->info.name);
    written += snprintf(buffer + written, buffer_size - written,
                        "========================================\n\n");
    
    /* 综合评分 */
    written += snprintf(buffer + written, buffer_size - written,
                        "Overall Score: %u/100\n", status.overall_score);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Latency:     %u/100\n", status.latency_score);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Throughput:  %u/100\n", status.throughput_score);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Reliability: %u/100\n\n", status.reliability_score);
    
    /* 延迟统计 */
    written += snprintf(buffer + written, buffer_size - written,
                        "Latency Statistics:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Min: %lu us\n", (unsigned long)status.latency.min_us);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Max: %lu us\n", (unsigned long)status.latency.max_us);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Avg: %.2f us\n", status.latency.avg_us);
    written += snprintf(buffer + written, buffer_size - written,
                        "  P99: %lu us\n", (unsigned long)status.latency.p99_us);
    written += snprintf(buffer + written, buffer_size - written,
                        "  P999: %lu us\n\n", (unsigned long)status.latency.p999_us);
    
    /* 吞吐量 */
    written += snprintf(buffer + written, buffer_size - written,
                        "Throughput:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Current: %.2f Mbps\n", status.throughput.current_mbps);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Peak:    %.2f Mbps\n", status.throughput.peak_mbps);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Messages: %lu\n\n", (unsigned long)status.throughput.total_messages);
    
    /* 丢包率 */
    written += snprintf(buffer + written, buffer_size - written,
                        "Loss Statistics:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Loss Rate: %u ppm\n", status.loss.loss_rate_ppm);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Health Score: %u/100\n", status.loss.health_score);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Connection: %s\n\n", 
                        status.loss.connection_healthy ? "HEALTHY" : "DEGRADED");
    
    /* 告警状态 */
    if (status.has_warnings || status.has_errors || status.has_critical) {
        written += snprintf(buffer + written, buffer_size - written,
                            "Alerts:\n");
        if (status.has_critical) {
            written += snprintf(buffer + written, buffer_size - written,
                                "  [!] CRITICAL: %u issues\n", status.critical_count);
        }
        if (status.has_errors) {
            written += snprintf(buffer + written, buffer_size - written,
                                "  [!] ERRORS: %u issues\n", status.error_count);
        }
        if (status.has_warnings) {
            written += snprintf(buffer + written, buffer_size - written,
                                "  [!] WARNINGS: %u issues\n", status.warning_count);
        }
    }
    
    written += snprintf(buffer + written, buffer_size - written,
                        "========================================\n");
    
    return written;
}

int dds_qos_generate_json_report(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size) {
    if (!monitor || !buffer || buffer_size == 0 || monitor->magic != DDS_QOS_MAGIC) {
        return -1;
    }
    
    dds_qos_status_t status;
    dds_qos_get_status(monitor, &status);
    
    int written = snprintf(buffer, buffer_size,
        "{\n"
        "  \"name\": \"%s\",\n"
        "  \"overall_score\": %u,\n"
        "  \"latency\": {\n"
        "    \"min_us\": %lu,\n"
        "    \"max_us\": %lu,\n"
        "    \"avg_us\": %.2f,\n"
        "    \"p99_us\": %lu\n"
        "  },\n"
        "  \"throughput\": {\n"
        "    \"current_mbps\": %.2f,\n"
        "    \"peak_mbps\": %.2f,\n"
        "    \"total_messages\": %lu\n"
        "  },\n"
        "  \"loss\": {\n"
        "    \"rate_ppm\": %u,\n"
        "    \"health_score\": %u\n"
        "  }\n"
        "}\n",
        monitor->info.name,
        status.overall_score,
        (unsigned long)status.latency.min_us,
        (unsigned long)status.latency.max_us,
        status.latency.avg_us,
        (unsigned long)status.latency.p99_us,
        status.throughput.current_mbps,
        status.throughput.peak_mbps,
        (unsigned long)status.throughput.total_messages,
        status.loss.loss_rate_ppm,
        status.loss.health_score
    );
    
    return written;
}

/*==============================================================================
 * 其他接口
 *============================================================================*/

void dds_qos_set_budgets(dds_qos_monitor_t* monitor, uint64_t latency_budget_us,
                         double throughput_budget_mbps) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    monitor->latency_budget_us = latency_budget_us;
    monitor->throughput_budget_mbps = throughput_budget_mbps;
}

uint32_t dds_qos_check_budget_violations(dds_qos_monitor_t* monitor) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return 0;
    return monitor->budget_violations;
}

void dds_qos_reset_stats(dds_qos_monitor_t* monitor) {
    if (!monitor || monitor->magic != DDS_QOS_MAGIC) return;
    
    pthread_mutex_lock(&monitor->status_mutex);
    memset(&monitor->current_status, 0, sizeof(dds_qos_status_t));
    pthread_mutex_unlock(&monitor->status_mutex);
}

int dds_qos_get_all_monitors(dds_qos_monitor_t** monitors, uint32_t max_count, 
                              uint32_t* actual_count) {
    if (!monitors || !actual_count || max_count == 0) return -1;
    
    pthread_mutex_lock(&g_qos_state.monitors_mutex);
    
    *actual_count = (g_qos_state.monitor_count < max_count) ? 
                    g_qos_state.monitor_count : max_count;
    
    for (uint32_t i = 0; i < *actual_count; i++) {
        monitors[i] = g_qos_state.monitors[i];
    }
    
    pthread_mutex_unlock(&g_qos_state.monitors_mutex);
    
    return 0;
}

int dds_qos_get_recommendations(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size) {
    (void)monitor;
    (void)buffer;
    (void)buffer_size;
    return 0;
}

int dds_qos_export_csv(dds_qos_monitor_t* monitor, const char* filename) {
    (void)monitor;
    (void)filename;
    return 0;
}

int dds_qos_export_uds_format(dds_qos_monitor_t* monitor, uint8_t* buffer, uint32_t* size) {
    (void)monitor;
    (void)buffer;
    (void)size;
    return 0;
}

int dds_qos_generate_histogram(dds_qos_monitor_t* monitor, dds_qos_metric_type_t metric,
                               char* buffer, size_t buffer_size) {
    (void)monitor;
    (void)metric;
    (void)buffer;
    (void)buffer_size;
    return 0;
}

int dds_qos_generate_multi_graph(dds_qos_monitor_t** monitors, uint32_t monitor_count,
                                  dds_qos_metric_type_t metric,
                                  char* buffer, size_t buffer_size) {
    (void)monitors;
    (void)monitor_count;
    (void)metric;
    (void)buffer;
    (void)buffer_size;
    return 0;
}
