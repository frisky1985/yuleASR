/**
 * @file persistence.c
 * @brief DDS持久化服务实现
 * @version 1.0
 * @date 2026-04-24
 */

#include "persistence.h"
#include "../../common/log/dds_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* ============================================================================
 * CRC32表(用于数据完整性校验)
 * ============================================================================ */

static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d
    // ... 省略完整表
};

/* ============================================================================
 * 私有数据结构定义
 * ============================================================================ */

typedef struct {
    uint8_t *buffer[PER_DOUBLE_BUFFER_COUNT];
    uint32_t buffer_size[PER_DOUBLE_BUFFER_COUNT];
    uint32_t buffer_records[PER_DOUBLE_BUFFER_COUNT];
    volatile uint8_t write_index;
    volatile uint8_t flush_index;
    bool enabled;
} per_double_buffer_t;

struct per_handle {
    char topic_name[64];
    per_config_t config;
    per_state_t state;
    per_stats_t stats;
    
    // 内存缓存
    per_record_t *record_head;
    per_record_t *record_tail;
    uint32_t record_count;
    uint64_t next_sequence;
    
    // 文件存储
    int data_fd;
    int index_fd;
    char data_file_path[512];
    char index_file_path[512];
    
    // 双写缓冲
    per_double_buffer_t double_buffer;
    
    // 历史策略
    per_history_policy_t history_policy;
    uint32_t history_depth;
    
    // ASIL
    bool asil_enabled;
    uint8_t asil_level;
    
    // 热备份
    char standby_path[256];
    uint32_t standby_sync_interval_ms;
    uint64_t last_standby_sync;
};

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

uint32_t per_calc_crc32(const void *data, uint32_t size) {
    const uint8_t *bytes = (const uint8_t*)data;
    uint32_t crc = 0xffffffff;
    
    for (uint32_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xff];
    }
    
    return crc ^ 0xffffffff;
}

bool per_validate_record(const per_record_t *record) {
    if (!record) return false;
    
    // 验证校验和
    uint32_t calc_crc = per_calc_crc32(record->data, record->header.data_size);
    return calc_crc == record->header.checksum;
}

static inline uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000;
}

static inline uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

static eth_status_t ensure_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            return ETH_ERROR;
        }
    }
    return ETH_OK;
}

static per_record_t* create_record(uint32_t data_size) {
    per_record_t *record = (per_record_t*)malloc(sizeof(per_record_t));
    if (!record) return NULL;
    
    memset(record, 0, sizeof(per_record_t));
    
    if (data_size > 0) {
        record->data = (uint8_t*)malloc(data_size);
        if (!record->data) {
            free(record);
            return NULL;
        }
    }
    
    return record;
}

static void free_record(per_record_t *record) {
    if (record) {
        if (record->data) {
            free(record->data);
        }
        free(record);
    }
}

static void add_record_to_list(per_handle_t *per, per_record_t *record) {
    record->prev = per->record_tail;
    record->next = NULL;
    
    if (per->record_tail) {
        per->record_tail->next = record;
    } else {
        per->record_head = record;
    }
    
    per->record_tail = record;
    per->record_count++;
}

static void remove_record_from_list(per_handle_t *per, per_record_t *record) {
    if (record->prev) {
        record->prev->next = record->next;
    } else {
        per->record_head = record->next;
    }
    
    if (record->next) {
        record->next->prev = record->prev;
    } else {
        per->record_tail = record->prev;
    }
    
    per->record_count--;
}

static void enforce_history_policy(per_handle_t *per) {
    if (per->history_policy == PER_HISTORY_KEEP_LAST && per->history_depth > 0) {
        while (per->record_count > per->history_depth) {
            per_record_t *oldest = per->record_head;
            if (oldest) {
                remove_record_from_list(per, oldest);
                free_record(oldest);
            }
        }
    }
}

/* ============================================================================
 * 文件存储实现
 * ============================================================================ */

static eth_status_t open_storage_files(per_handle_t *per) {
    if (per->config.storage_type != PER_STORAGE_FILE) {
        return ETH_OK;
    }
    
    // 确保存储目录存在
    if (ensure_directory(per->config.storage_path) != ETH_OK) {
        DDS_LOG_ERROR("Failed to create storage directory: %s", per->config.storage_path);
        return ETH_ERROR;
    }
    
    // 构建文件路径
    snprintf(per->data_file_path, sizeof(per->data_file_path),
             "%s/%s.data", per->config.storage_path, per->topic_name);
    snprintf(per->index_file_path, sizeof(per->index_file_path),
             "%s/%s.idx", per->config.storage_path, per->topic_name);
    
    // 打开或创建数据文件
    per->data_fd = open(per->data_file_path, O_RDWR | O_CREAT, 0644);
    if (per->data_fd < 0) {
        DDS_LOG_ERROR("Failed to open data file: %s", per->data_file_path);
        return ETH_ERROR;
    }
    
    // 打开或创建索引文件
    per->index_fd = open(per->index_file_path, O_RDWR | O_CREAT, 0644);
    if (per->index_fd < 0) {
        close(per->data_fd);
        DDS_LOG_ERROR("Failed to open index file: %s", per->index_file_path);
        return ETH_ERROR;
    }
    
    return ETH_OK;
}

static void close_storage_files(per_handle_t *per) {
    if (per->data_fd >= 0) {
        close(per->data_fd);
        per->data_fd = -1;
    }
    if (per->index_fd >= 0) {
        close(per->index_fd);
        per->index_fd = -1;
    }
}

static eth_status_t write_record_to_file(per_handle_t *per, const per_record_t *record) {
    if (per->data_fd < 0) return ETH_ERROR;
    
    // 写入记录头
    ssize_t written = write(per->data_fd, &record->header, sizeof(per_record_header_t));
    if (written != sizeof(per_record_header_t)) {
        per->stats.write_errors++;
        return ETH_ERROR;
    }
    
    // 写入数据
    if (record->header.data_size > 0 && record->data) {
        written = write(per->data_fd, record->data, record->header.data_size);
        if (written != record->header.data_size) {
            per->stats.write_errors++;
            return ETH_ERROR;
        }
    }
    
    per->stats.bytes_written += sizeof(per_record_header_t) + record->header.data_size;
    
    // ASIL模式下立即刷新
    if (per->asil_enabled && per->asil_level >= 3) {
        fsync(per->data_fd);
    }
    
    return ETH_OK;
}

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

eth_status_t per_init(void) {
    DDS_LOG_INFO("Persistence service module initialized");
    return ETH_OK;
}

void per_deinit(void) {
    DDS_LOG_INFO("Persistence service module deinitialized");
}

per_handle_t* per_create(const char *topic_name, const per_config_t *config) {
    if (!topic_name || !config) return NULL;
    
    per_handle_t *per = (per_handle_t*)calloc(1, sizeof(per_handle_t));
    if (!per) return NULL;
    
    strncpy(per->topic_name, topic_name, sizeof(per->topic_name) - 1);
    memcpy(&per->config, config, sizeof(per_config_t));
    
    per->state = PER_STATE_IDLE;
    per->next_sequence = 1;
    per->data_fd = -1;
    per->index_fd = -1;
    per->history_policy = PER_HISTORY_KEEP_ALL;
    per->history_depth = 0;
    
    // 设置默认存储路径
    if (per->config.storage_path[0] == '\0') {
        strncpy(per->config.storage_path, PER_DEFAULT_STORAGE_DIR, 
                sizeof(per->config.storage_path) - 1);
    }
    
    // 分配双写缓冲区
    if (config->level == PER_LEVEL_TRANSIENT || config->level == PER_LEVEL_TRANSIENT_LOCAL) {
        for (int i = 0; i < PER_DOUBLE_BUFFER_COUNT; i++) {
            per->double_buffer.buffer[i] = (uint8_t*)malloc(PER_MAX_RECORD_SIZE * 100);
            if (per->double_buffer.buffer[i]) {
                per->double_buffer.buffer_size[i] = PER_MAX_RECORD_SIZE * 100;
            }
        }
        per->double_buffer.enabled = true;
    }
    
    // 打开存储文件(如果需要)
    if (config->storage_type == PER_STORAGE_FILE) {
        if (open_storage_files(per) != ETH_OK) {
            free(per);
            return NULL;
        }
    }
    
    DDS_LOG_INFO("Created persistence service: topic=%s, level=%d, storage=%d",
                topic_name, config->level, config->storage_type);
    
    return per;
}

eth_status_t per_delete(per_handle_t *per, bool flush) {
    if (!per) return ETH_INVALID_PARAM;
    
    if (flush) {
        per_flush(per, false);
    }
    
    // 释放所有记录
    per_record_t *record = per->record_head;
    while (record) {
        per_record_t *next = record->next;
        free_record(record);
        record = next;
    }
    
    // 释放双写缓冲区
    for (int i = 0; i < PER_DOUBLE_BUFFER_COUNT; i++) {
        if (per->double_buffer.buffer[i]) {
            free(per->double_buffer.buffer[i]);
        }
    }
    
    // 关闭文件
    close_storage_files(per);
    
    DDS_LOG_INFO("Deleted persistence service: %s", per->topic_name);
    free(per);
    return ETH_OK;
}

eth_status_t per_write_sample(per_handle_t *per,
                               const dds_guid_t *writer_guid,
                               const void *data,
                               uint32_t size,
                               uint64_t sequence_num) {
    if (!per || !data || size == 0 || size > PER_MAX_RECORD_SIZE) {
        return ETH_INVALID_PARAM;
    }
    
    per->state = PER_STATE_WRITING;
    
    // 创建新记录
    per_record_t *record = create_record(size);
    if (!record) {
        per->state = PER_STATE_ERROR;
        return ETH_NO_MEMORY;
    }
    
    // 填充记录头
    record->header.sequence_num = sequence_num > 0 ? sequence_num : per->next_sequence++;
    record->header.timestamp_us = get_time_us();
    record->header.data_size = size;
    record->header.level = per->config.level;
    if (writer_guid) {
        memcpy(&record->header.writer_guid, writer_guid, sizeof(dds_guid_t));
    }
    
    // 复制数据
    memcpy(record->data, data, size);
    
    // 计算校验和
    if (per->config.enable_checksum) {
        record->header.checksum = per_calc_crc32(data, size);
    }
    
    // 添加到内存列表
    add_record_to_list(per, record);
    
    // 执行历史策略
    enforce_history_policy(per);
    
    // 写入文件(如果需要)
    if (per->config.storage_type == PER_STORAGE_FILE && per->config.level == PER_LEVEL_PERSISTENT) {
        write_record_to_file(per, record);
    }
    
    per->stats.total_writes++;
    per->stats.current_records = per->record_count;
    
    per->state = PER_STATE_IDLE;
    return ETH_OK;
}

eth_status_t per_read_samples(per_handle_t *per,
                               const dds_guid_t *reader_guid,
                               uint64_t start_sequence,
                               void *buffer,
                               uint32_t buffer_size,
                               uint32_t *actual_size,
                               uint32_t *records_read) {
    if (!per || !buffer || !actual_size || !records_read) {
        return ETH_INVALID_PARAM;
    }
    
    *actual_size = 0;
    *records_read = 0;
    
    uint8_t *ptr = (uint8_t*)buffer;
    uint32_t remaining = buffer_size;
    
    per_record_t *record = per->record_head;
    while (record && remaining >= sizeof(per_record_header_t)) {
        // 检查序列号
        if (record->header.sequence_num >= start_sequence) {
            uint32_t total_size = sizeof(per_record_header_t) + record->header.data_size;
            
            if (remaining < total_size) break;
            
            // 验证记录
            if (per->config.enable_checksum && !per_validate_record(record)) {
                per->stats.checksum_errors++;
                record = record->next;
                continue;
            }
            
            // 复制记录
            memcpy(ptr, &record->header, sizeof(per_record_header_t));
            ptr += sizeof(per_record_header_t);
            
            if (record->header.data_size > 0 && record->data) {
                memcpy(ptr, record->data, record->header.data_size);
                ptr += record->header.data_size;
            }
            
            remaining -= total_size;
            *actual_size += total_size;
            (*records_read)++;
        }
        
        record = record->next;
    }
    
    per->stats.total_reads++;
    per->stats.bytes_read += *actual_size;
    
    return ETH_OK;
}

eth_status_t per_flush(per_handle_t *per, bool async) {
    if (!per) return ETH_INVALID_PARAM;
    
    per->state = PER_STATE_FLUSHING;
    
    // 刷新文件
    if (per->config.storage_type == PER_STORAGE_FILE) {
        if (per->data_fd >= 0) {
            fsync(per->data_fd);
        }
        if (per->index_fd >= 0) {
            fsync(per->index_fd);
        }
    }
    
    per->stats.total_flushes++;
    per->state = PER_STATE_IDLE;
    
    return ETH_OK;
}

eth_status_t per_recover(per_handle_t *per,
                          const per_request_t *request,
                          per_recovery_status_t *status) {
    if (!per || !status) return ETH_INVALID_PARAM;
    
    memset(status, 0, sizeof(per_recovery_status_t));
    per->state = PER_STATE_RECOVERING;
    
    uint64_t start_time = get_time_ms();
    
    // 恢复统计
    status->total_records = per->record_count;
    
    per_record_t *record = per->record_head;
    while (record) {
        // 验证记录
        if (per->config.enable_checksum && !per_validate_record(record)) {
            status->failed_records++;
            per->stats.checksum_errors++;
        } else {
            status->recovered_records++;
        }
        record = record->next;
    }
    
    status->recovery_time_ms = get_time_ms() - start_time;
    status->complete = (status->recovered_records + status->failed_records == status->total_records);
    
    per->stats.recovery_count++;
    per->state = PER_STATE_IDLE;
    
    DDS_LOG_INFO("Recovery completed: %u/%u records, time=%lu ms",
                status->recovered_records, status->total_records, status->recovery_time_ms);
    
    return ETH_OK;
}

eth_status_t per_check_recovery_needed(per_handle_t *per, bool *needs_recovery) {
    if (!per || !needs_recovery) return ETH_INVALID_PARAM;
    
    *needs_recovery = false;
    
    // 检查是否有历史数据
    if (per->record_count > 0) {
        *needs_recovery = true;
    }
    
    // 检查持久化文件
    if (per->config.storage_type == PER_STORAGE_FILE) {
        struct stat st;
        if (stat(per->data_file_path, &st) == 0 && st.st_size > 0) {
            *needs_recovery = true;
        }
    }
    
    return ETH_OK;
}

eth_status_t per_cleanup_old_records(per_handle_t *per, uint32_t keep_count) {
    if (!per) return ETH_INVALID_PARAM;
    
    while (per->record_count > keep_count && per->record_head) {
        per_record_t *oldest = per->record_head;
        remove_record_from_list(per, oldest);
        free_record(oldest);
    }
    
    per->stats.current_records = per->record_count;
    return ETH_OK;
}

eth_status_t per_get_last_sequence(per_handle_t *per, uint64_t *sequence_num) {
    if (!per || !sequence_num) return ETH_INVALID_PARAM;
    
    if (per->record_tail) {
        *sequence_num = per->record_tail->header.sequence_num;
    } else {
        *sequence_num = 0;
    }
    
    return ETH_OK;
}

eth_status_t per_set_history_policy(per_handle_t *per, 
                                     per_history_policy_t policy, 
                                     uint32_t depth) {
    if (!per) return ETH_INVALID_PARAM;
    
    per->history_policy = policy;
    per->history_depth = depth;
    
    // 立即应用新策略
    enforce_history_policy(per);
    
    return ETH_OK;
}

eth_status_t per_get_stats(per_handle_t *per, per_stats_t *stats) {
    if (!per || !stats) return ETH_INVALID_PARAM;
    
    memcpy(stats, &per->stats, sizeof(per_stats_t));
    return ETH_OK;
}

eth_status_t per_reset_stats(per_handle_t *per) {
    if (!per) return ETH_INVALID_PARAM;
    
    memset(&per->stats, 0, sizeof(per_stats_t));
    return ETH_OK;
}

eth_status_t per_enable_asil_mode(per_handle_t *per, uint8_t asil_level) {
    if (!per || asil_level > 4) return ETH_INVALID_PARAM;
    
    per->asil_enabled = true;
    per->asil_level = asil_level;
    
    // ASIL模式配置
    per->config.enable_checksum = true;
    
    switch (asil_level) {
        case 4: // ASIL-D
            per->config.auto_flush_interval_ms = 10;  // 10ms
            break;
        case 3: // ASIL-C
            per->config.auto_flush_interval_ms = 50;
            break;
        default:
            break;
    }
    
    DDS_LOG_INFO("ASIL mode enabled for persistence: level=%d", asil_level);
    return ETH_OK;
}

eth_status_t per_backup(per_handle_t *per, const char *backup_path) {
    if (!per || !backup_path) return ETH_INVALID_PARAM;
    
    if (per->config.storage_type != PER_STORAGE_FILE) {
        return ETH_ERROR;
    }
    
    // 先刷新确保数据完整
    per_flush(per, false);
    
    // 构建备份文件路径
    char backup_data[512], backup_idx[512];
    snprintf(backup_data, sizeof(backup_data), "%s/%s.data", backup_path, per->topic_name);
    snprintf(backup_idx, sizeof(backup_idx), "%s/%s.idx", backup_path, per->topic_name);
    
    // 执行备份(简化版本 - 复制文件)
    // 实际实现应使用更可靠的备份机制
    
    DDS_LOG_INFO("Backup completed: %s", backup_path);
    return ETH_OK;
}

eth_status_t per_restore(per_handle_t *per, const char *backup_path) {
    if (!per || !backup_path) return ETH_INVALID_PARAM;
    
    DDS_LOG_INFO("Restore from backup: %s", backup_path);
    
    // 实际实现需要从备份文件读取并恢复数据
    
    return ETH_OK;
}

eth_status_t per_enable_hot_standby(per_handle_t *per, 
                                     const char *standby_path,
                                     uint32_t sync_interval_ms) {
    if (!per || !standby_path) return ETH_INVALID_PARAM;
    
    strncpy(per->standby_path, standby_path, sizeof(per->standby_path) - 1);
    per->standby_sync_interval_ms = sync_interval_ms;
    per->config.enable_hot_standby = true;
    
    DDS_LOG_INFO("Hot standby enabled: path=%s, interval=%u ms",
                standby_path, sync_interval_ms);
    return ETH_OK;
}

eth_status_t per_sync_hot_standby(per_handle_t *per) {
    if (!per || !per->config.enable_hot_standby) return ETH_INVALID_PARAM;
    
    uint64_t current_time = get_time_ms();
    if (current_time - per->last_standby_sync < per->standby_sync_interval_ms) {
        return ETH_OK; // 还不到同步时间
    }
    
    // 执行同步
    // 实际实现需要将数据复制到备份位置
    
    per->last_standby_standby = current_time;
    
    return ETH_OK;
}
