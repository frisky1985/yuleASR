/**
 * @file rtps_state.c
 * @brief RTPS协议状态机实现
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现Writer/Reader状态管理、心跳机制和网络状态管理
 */

#include "rtps_state.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 查找匹配的Reader
 */
static rtps_matched_reader_t* find_matched_reader(
    rtps_writer_state_machine_t *writer,
    const rtps_guid_t *reader_guid)
{
    for (uint32_t i = 0; i < writer->matched_reader_count; i++) {
        if (rtps_guid_equal(&writer->matched_readers[i].remote_guid, reader_guid)) {
            return &writer->matched_readers[i];
        }
    }
    return NULL;
}

/**
 * @brief 查找匹配的Writer
 */
static rtps_matched_writer_t* find_matched_writer(
    rtps_reader_state_machine_t *reader,
    const rtps_guid_t *writer_guid)
{
    for (uint32_t i = 0; i < reader->matched_writer_count; i++) {
        if (rtps_guid_equal(&reader->matched_writers[i].remote_guid, writer_guid)) {
            return &reader->matched_writers[i];
        }
    }
    return NULL;
}

/**
 * @brief 创建缓存变化
 */
static rtps_cached_change_t* create_cached_change(
    const rtps_sequence_number_t *seq,
    const uint8_t *data,
    uint32_t len)
{
    rtps_cached_change_t *change = (rtps_cached_change_t *)malloc(
        sizeof(rtps_cached_change_t));
    if (change == NULL) {
        return NULL;
    }
    
    change->data = (uint8_t *)malloc(len);
    if (change->data == NULL) {
        free(change);
        return NULL;
    }
    
    change->seq_number = *seq;
    memcpy(change->data, data, len);
    change->data_len = len;
    change->is_inline_qos = false;
    change->ref_count = 1;
    change->next = NULL;
    
    return change;
}

/**
 * @brief 释放缓存变化
 */
static void release_cached_change(rtps_cached_change_t *change)
{
    if (change == NULL) {
        return;
    }
    
    change->ref_count--;
    if (change->ref_count == 0) {
        if (change->data != NULL) {
            free(change->data);
        }
        free(change);
    }
}

/**
 * @brief 添加变化到历史缓存
 */
static eth_status_t add_to_history_cache(
    rtps_writer_state_machine_t *writer,
    rtps_cached_change_t *change)
{
    /* 检查缓存是否已满 */
    if (writer->history_cache_size >= writer->history_cache_max) {
        /* 移除最早的变化 */
        rtps_cached_change_t *old = writer->history_cache;
        writer->history_cache = old->next;
        release_cached_change(old);
        writer->history_cache_size--;
    }
    
    /* 添加到链表尾部（按序列号排序） */
    rtps_cached_change_t **current = &writer->history_cache;
    while (*current != NULL) {
        if (rtps_seqnum_compare(&(*current)->seq_number, &change->seq_number) > 0) {
            break;
        }
        current = &(*current)->next;
    }
    
    change->next = *current;
    *current = change;
    change->ref_count++;
    writer->history_cache_size++;
    
    return ETH_OK;
}

/**
 * @brief 查找历史缓存中的变化
 */
static rtps_cached_change_t* find_in_history_cache(
    rtps_writer_state_machine_t *writer,
    const rtps_sequence_number_t *seq)
{
    rtps_cached_change_t *current = writer->history_cache;
    while (current != NULL) {
        if (rtps_seqnum_compare(&current->seq_number, seq) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief 设置位图位
 */
static void set_bitmap_bit(rtps_sequence_number_set_t *set, uint32_t bit, bool value)
{
    if (bit >= set->num_bits) {
        return;
    }
    
    uint32_t word_idx = bit / 32;
    uint32_t bit_idx = bit % 32;
    
    if (value) {
        set->bitmap[word_idx] |= (1U << (31 - bit_idx));
    } else {
        set->bitmap[word_idx] &= ~(1U << (31 - bit_idx));
    }
}

/**
 * @brief 获取位图位
 */
static bool get_bitmap_bit(const rtps_sequence_number_set_t *set, uint32_t bit)
{
    if (bit >= set->num_bits) {
        return false;
    }
    
    uint32_t word_idx = bit / 32;
    uint32_t bit_idx = bit % 32;
    
    return (set->bitmap[word_idx] & (1U << (31 - bit_idx))) != 0;
}

/**
 * @brief 计算两个序列号的差值
 */
static int64_t seqnum_diff(const rtps_sequence_number_t *seq1,
                           const rtps_sequence_number_t *seq2)
{
    int64_t v1 = ((int64_t)seq1->high << 32) | seq1->low;
    int64_t v2 = ((int64_t)seq2->high << 32) | seq2->low;
    return v1 - v2;
}

/* ============================================================================
 * Writer状态机实现
 * ============================================================================ */

eth_status_t rtps_writer_sm_init(rtps_writer_state_machine_t *writer,
                                  const rtps_guid_t *guid,
                                  uint32_t history_depth)
{
    if (writer == NULL || guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(writer, 0, sizeof(rtps_writer_state_machine_t));
    
    writer->guid = *guid;
    writer->state = RTPS_WRITER_STATE_IDLE;
    
    /* 初始化序列号 */
    writer->last_seq.high = 0;
    writer->last_seq.low = 0;
    writer->max_seq.high = 0;
    writer->max_seq.low = 0;
    
    /* 设置历史缓存 */
    writer->history_cache_max = (history_depth > 0 && history_depth <= RTPS_WRITER_MAX_CACHED_CHANGES) 
                                ? history_depth : RTPS_WRITER_MAX_CACHED_CHANGES;
    writer->history_cache = NULL;
    writer->history_cache_size = 0;
    
    /* 心跳状态 */
    writer->heartbeat_count = 0;
    writer->last_heartbeat_send_time = 0;
    
    return ETH_OK;
}

void rtps_writer_sm_deinit(rtps_writer_state_machine_t *writer)
{
    if (writer == NULL) {
        return;
    }
    
    /* 清理历史缓存 */
    while (writer->history_cache != NULL) {
        rtps_cached_change_t *temp = writer->history_cache;
        writer->history_cache = temp->next;
        release_cached_change(temp);
    }
    
    writer->state = RTPS_WRITER_STATE_IDLE;
}

eth_status_t rtps_writer_sm_match_reader(rtps_writer_state_machine_t *writer,
                                          const rtps_guid_t *reader_guid)
{
    if (writer == NULL || reader_guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查是否已存在 */
    if (find_matched_reader(writer, reader_guid) != NULL) {
        return ETH_OK; /* 已存在 */
    }
    
    /* 检查空位 */
    if (writer->matched_reader_count >= RTPS_MAX_MATCHED_READERS) {
        return ETH_ERROR;
    }
    
    /* 添加新的匹配Reader */
    rtps_matched_reader_t *reader = &writer->matched_readers[writer->matched_reader_count++];
    memset(reader, 0, sizeof(rtps_matched_reader_t));
    reader->remote_guid = *reader_guid;
    reader->state = RTPS_CONNECTION_MATCHED;
    reader->last_sn.high = 0;
    reader->last_sn.low = 0;
    reader->active = true;
    
    /* 更新Writer状态 */
    if (writer->state == RTPS_WRITER_STATE_IDLE) {
        writer->state = RTPS_WRITER_STATE_ANNOUNCING;
    }
    
    return ETH_OK;
}

eth_status_t rtps_writer_sm_unmatch_reader(rtps_writer_state_machine_t *writer,
                                            const rtps_guid_t *reader_guid)
{
    if (writer == NULL || reader_guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < writer->matched_reader_count; i++) {
        if (rtps_guid_equal(&writer->matched_readers[i].remote_guid, reader_guid)) {
            /* 移除（用最后一个元素填充） */
            writer->matched_readers[i] = writer->matched_readers[--writer->matched_reader_count];
            return ETH_OK;
        }
    }
    
    return ETH_ERROR;
}

eth_status_t rtps_writer_sm_write(rtps_writer_state_machine_t *writer,
                                   const uint8_t *data,
                                   uint32_t len,
                                   rtps_sequence_number_t *seq_number)
{
    if (writer == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    /* 分配新序列号 */
    rtps_seqnum_increment(&writer->last_seq);
    writer->max_seq = writer->last_seq;
    
    if (seq_number != NULL) {
        *seq_number = writer->last_seq;
    }
    
    /* 创建缓存变化 */
    rtps_cached_change_t *change = create_cached_change(&writer->last_seq, data, len);
    if (change == NULL) {
        return ETH_NO_MEMORY;
    }
    
    /* 添加到历史缓存 */
    eth_status_t status = add_to_history_cache(writer, change);
    if (status != ETH_OK) {
        release_cached_change(change);
        return status;
    }
    
    /* 更新状态 */
    writer->state = RTPS_WRITER_STATE_PUSHING;
    writer->data_sent_count++;
    
    release_cached_change(change);
    return ETH_OK;
}

eth_status_t rtps_writer_sm_handle_acknack(rtps_writer_state_machine_t *writer,
                                            const rtps_guid_t *reader_guid,
                                            const rtps_sequence_number_set_t *ack_bitmap,
                                            uint64_t current_time_ms)
{
    if (writer == NULL || reader_guid == NULL || ack_bitmap == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    rtps_matched_reader_t *reader = find_matched_reader(writer, reader_guid);
    if (reader == NULL) {
        return ETH_ERROR; /* 未知的Reader */
    }
    
    /* 更新Reader状态 */
    reader->last_activity_time = current_time_ms;
    reader->ack_nack_count++;
    reader->active = true;
    
    /* 更新ACK位图 */
    reader->last_sn = ack_bitmap->bitmap_base;
    
    /* 检查是否需要重传 */
    bool need_retransmit = false;
    for (uint32_t i = 0; i < ack_bitmap->num_bits && i < 256; i++) {
        if (!get_bitmap_bit(ack_bitmap, i)) {
            /* 这个序列号缺失 */
            need_retransmit = true;
            break;
        }
    }
    
    if (need_retransmit) {
        writer->state = RTPS_WRITER_STATE_MUST_REFRESH;
    }
    
    return ETH_OK;
}

eth_status_t rtps_writer_sm_process(rtps_writer_state_machine_t *writer,
                                     uint64_t current_time_ms,
                                     bool *need_heartbeat)
{
    if (writer == NULL || need_heartbeat == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *need_heartbeat = false;
    
    /* 检查是否需要发送心跳 */
    if (writer->matched_reader_count > 0) {
        uint64_t time_since_last_hb = current_time_ms - writer->last_heartbeat_send_time;
        
        if (time_since_last_hb >= RTPS_HEARTBEAT_PERIOD_MS) {
            *need_heartbeat = true;
            writer->last_heartbeat_send_time = current_time_ms;
            writer->heartbeat_count++;
            writer->heartbeat_sent_count++;
        }
    }
    
    /* 检查Reader超时 */
    for (uint32_t i = 0; i < writer->matched_reader_count; i++) {
        rtps_matched_reader_t *reader = &writer->matched_readers[i];
        if (reader->active) {
            uint64_t inactive_time = current_time_ms - reader->last_activity_time;
            if (inactive_time > RTPS_HEARTBEAT_TIMEOUT_MS) {
                reader->state = RTPS_CONNECTION_STALE;
                reader->active = false;
            }
        }
    }
    
    /* 状态转换 */
    switch (writer->state) {
        case RTPS_WRITER_STATE_PUSHING:
            /* 数据发送后，等待ACK */
            writer->state = RTPS_WRITER_STATE_WAITING_ACK;
            break;
            
        case RTPS_WRITER_STATE_MUST_REFRESH:
            /* 重传完成后，返回到等待状态 */
            writer->state = RTPS_WRITER_STATE_WAITING_ACK;
            break;
            
        case RTPS_WRITER_STATE_WAITING_ACK:
            /* 检查所有Reader都ACK后，返回空闲 */
            {
                bool all_acked = true;
                for (uint32_t i = 0; i < writer->matched_reader_count; i++) {
                    if (rtps_seqnum_compare(&writer->matched_readers[i].last_sn, 
                                             &writer->max_seq) < 0) {
                        all_acked = false;
                        break;
                    }
                }
                if (all_acked) {
                    writer->state = RTPS_WRITER_STATE_IDLE;
                }
            }
            break;
            
        default:
            break;
    }
    
    return ETH_OK;
}

eth_status_t rtps_writer_sm_get_requested_changes(rtps_writer_state_machine_t *writer,
                                                    const rtps_guid_t *reader_guid,
                                                    rtps_cached_change_t **changes,
                                                    uint32_t max_changes,
                                                    uint32_t *actual_changes)
{
    if (writer == NULL || changes == NULL || actual_changes == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *actual_changes = 0;
    
    rtps_matched_reader_t *reader = NULL;
    if (reader_guid != NULL) {
        reader = find_matched_reader(writer, reader_guid);
        if (reader == NULL) {
            return ETH_ERROR;
        }
    }
    
    /* 遍历历史缓存，查找需要重传的变化 */
    rtps_cached_change_t *current = writer->history_cache;
    while (current != NULL && *actual_changes < max_changes) {
        if (reader != NULL) {
            /* 检查Reader是否已ACK这个序列号 */
            if (rtps_seqnum_compare(&current->seq_number, &reader->last_sn) > 0) {
                changes[(*actual_changes)++] = current;
                current->ref_count++;
            }
        } else {
            changes[(*actual_changes)++] = current;
            current->ref_count++;
        }
        current = current->next;
    }
    
    return ETH_OK;
}

/* ============================================================================
 * Reader状态机实现
 * ============================================================================ */

eth_status_t rtps_reader_sm_init(rtps_reader_state_machine_t *reader,
                                  const rtps_guid_t *guid,
                                  uint32_t queue_size)
{
    if (reader == NULL || guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(reader, 0, sizeof(rtps_reader_state_machine_t));
    
    reader->guid = *guid;
    reader->state = RTPS_READER_STATE_IDLE;
    
    /* 初始化序列号 */
    reader->next_expected_seq.high = 0;
    reader->next_expected_seq.low = 1;
    reader->highest_seq.high = 0;
    reader->highest_seq.low = 0;
    
    /* 设置接收队列 */
    reader->receive_queue_max = (queue_size > 0 && queue_size <= RTPS_READER_MAX_CACHED_CHANGES)
                                 ? queue_size : RTPS_READER_MAX_CACHED_CHANGES;
    reader->receive_queue = NULL;
    reader->receive_queue_size = 0;
    
    /* ACKNACK状态 */
    reader->acknack_count = 0;
    reader->must_send_acknack = false;
    reader->acknack_suppressed = false;
    
    return ETH_OK;
}

void rtps_reader_sm_deinit(rtps_reader_state_machine_t *reader)
{
    if (reader == NULL) {
        return;
    }
    
    /* 清理接收队列 */
    while (reader->receive_queue != NULL) {
        rtps_cached_change_t *temp = reader->receive_queue;
        reader->receive_queue = temp->next;
        release_cached_change(temp);
    }
    
    reader->state = RTPS_READER_STATE_IDLE;
}

eth_status_t rtps_reader_sm_match_writer(rtps_reader_state_machine_t *reader,
                                          const rtps_guid_t *writer_guid)
{
    if (reader == NULL || writer_guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查是否已存在 */
    if (find_matched_writer(reader, writer_guid) != NULL) {
        return ETH_OK;
    }
    
    /* 检查空位 */
    if (reader->matched_writer_count >= RTPS_MAX_MATCHED_WRITERS) {
        return ETH_ERROR;
    }
    
    /* 添加新的匹配Writer */
    rtps_matched_writer_t *writer = &reader->matched_writers[reader->matched_writer_count++];
    memset(writer, 0, sizeof(rtps_matched_writer_t));
    writer->remote_guid = *writer_guid;
    writer->state = RTPS_CONNECTION_MATCHED;
    writer->next_expected_seq.high = 0;
    writer->next_expected_seq.low = 1;
    writer->active = true;
    writer->last_available_seq.high = 0;
    writer->last_available_seq.low = 0;
    
    /* 更新Reader状态 */
    if (reader->state == RTPS_READER_STATE_IDLE) {
        reader->state = RTPS_READER_STATE_WAITING_DATA;
    }
    
    return ETH_OK;
}

eth_status_t rtps_reader_sm_unmatch_writer(rtps_reader_state_machine_t *reader,
                                            const rtps_guid_t *writer_guid)
{
    if (reader == NULL || writer_guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < reader->matched_writer_count; i++) {
        if (rtps_guid_equal(&reader->matched_writers[i].remote_guid, writer_guid)) {
            reader->matched_writers[i] = reader->matched_writers[--reader->matched_writer_count];
            return ETH_OK;
        }
    }
    
    return ETH_ERROR;
}

eth_status_t rtps_reader_sm_handle_data(rtps_reader_state_machine_t *reader,
                                         const rtps_guid_t *writer_guid,
                                         const rtps_sequence_number_t *seq_number,
                                         const uint8_t *data,
                                         uint32_t len,
                                         uint64_t current_time_ms,
                                         bool *need_acknack)
{
    if (reader == NULL || writer_guid == NULL || seq_number == NULL || data == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *need_acknack = false;
    
    /* 查找匹配的Writer */
    rtps_matched_writer_t *writer = find_matched_writer(reader, writer_guid);
    if (writer == NULL) {
        return ETH_ERROR; /* 未知的Writer */
    }
    
    /* 更新Writer状态 */
    writer->last_activity_time = current_time_ms;
    writer->active = true;
    
    /* 更新最高序列号 */
    if (rtps_seqnum_compare(seq_number, &writer->last_available_seq) > 0) {
        writer->last_available_seq = *seq_number;
    }
    
    /* 检查序列号是否期望 */
    int cmp = rtps_seqnum_compare(seq_number, &reader->next_expected_seq);
    
    if (cmp == 0) {
        /* 正是期望的序列号 */
        rtps_cached_change_t *change = create_cached_change(seq_number, data, len);
        if (change == NULL) {
            return ETH_NO_MEMORY;
        }
        
        /* 添加到接收队列 */
        change->next = reader->receive_queue;
        reader->receive_queue = change;
        reader->receive_queue_size++;
        
        rtps_seqnum_increment(&reader->next_expected_seq);
        reader->data_received_count++;
        
        /* 检查是否有丢失的数据 */
        if (rtps_seqnum_compare(&reader->next_expected_seq, &writer->last_available_seq) < 0) {
            reader->state = RTPS_READER_STATE_REQUESTING;
            *need_acknack = true;
        }
        
    } else if (cmp > 0) {
        /* 序列号超前，发生丢失 */
        reader->state = RTPS_READER_STATE_REQUESTING;
        
        /* 更新丢失变化集 */
        int64_t diff = seqnum_diff(seq_number, &reader->next_expected_seq);
        if (diff > 0 && diff < 256) {
            for (int64_t i = 0; i < diff; i++) {
                set_bitmap_bit(&writer->missing_changes, (uint32_t)i, true);
            }
        }
        
        *need_acknack = true;
        
    } else {
        /* 旧数据，忽略 */
    }
    
    return ETH_OK;
}

eth_status_t rtps_reader_sm_handle_heartbeat(rtps_reader_state_machine_t *reader,
                                              const rtps_guid_t *writer_guid,
                                              const rtps_sequence_number_t *first_sn,
                                              const rtps_sequence_number_t *last_sn,
                                              uint64_t current_time_ms,
                                              bool *need_acknack)
{
    if (reader == NULL || writer_guid == NULL || first_sn == NULL || last_sn == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *need_acknack = false;
    
    /* 查找匹配的Writer */
    rtps_matched_writer_t *writer = find_matched_writer(reader, writer_guid);
    if (writer == NULL) {
        return ETH_ERROR;
    }
    
    /* 更新Writer状态 */
    writer->last_heartbeat_time = current_time_ms;
    writer->last_activity_time = current_time_ms;
    writer->active = true;
    writer->heartbeat_count++;
    
    /* 更新最后可用序列号 */
    if (rtps_seqnum_compare(last_sn, &writer->last_available_seq) > 0) {
        writer->last_available_seq = *last_sn;
    }
    
    /* 检查是否有遗失数据 */
    if (rtps_seqnum_compare(&reader->next_expected_seq, last_sn) < 0) {
        /* 需要请求丢失的数据 */
        reader->state = RTPS_READER_STATE_REQUESTING;
        *need_acknack = true;
        
        /* 更新丢失位图 */
        writer->missing_changes.bitmap_base = reader->next_expected_seq;
        int64_t diff = seqnum_diff(last_sn, &reader->next_expected_seq);
        writer->missing_changes.num_bits = (diff > 256) ? 256 : (uint32_t)diff;
    }
    
    return ETH_OK;
}

eth_status_t rtps_reader_sm_process(rtps_reader_state_machine_t *reader,
                                     uint64_t current_time_ms,
                                     bool *need_acknack)
{
    if (reader == NULL || need_acknack == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *need_acknack = false;
    
    /* 检查Writer超时 */
    for (uint32_t i = 0; i < reader->matched_writer_count; i++) {
        rtps_matched_writer_t *writer = &reader->matched_writers[i];
        if (writer->active) {
            uint64_t inactive_time = current_time_ms - writer->last_activity_time;
            if (inactive_time > RTPS_HEARTBEAT_TIMEOUT_MS) {
                writer->state = RTPS_CONNECTION_STALE;
                writer->active = false;
            }
        }
    }
    
    /* 检查是否需要发送ACKNACK */
    if (reader->state == RTPS_READER_STATE_REQUESTING ||
        reader->state == RTPS_READER_STATE_MUST_ACK) {
        
        /* 检查抑制时间 */
        if (!reader->acknack_suppressed) {
            uint64_t time_since_last = current_time_ms - reader->last_acknack_send_time;
            if (time_since_last >= RTPS_NACK_RESPONSE_DELAY_MS) {
                *need_acknack = true;
                reader->last_acknack_send_time = current_time_ms;
                reader->acknack_count++;
                reader->acknack_sent_count++;
                reader->acknack_suppressed = true;
            }
        } else {
            /* 检查抑制是否过期 */
            uint64_t suppress_time = current_time_ms - reader->last_acknack_send_time;
            if (suppress_time >= RTPS_NACK_SUPPRESSION_DURATION_MS) {
                reader->acknack_suppressed = false;
            }
        }
    }
    
    return ETH_OK;
}

eth_status_t rtps_reader_sm_read(rtps_reader_state_machine_t *reader,
                                  uint8_t *data,
                                  uint32_t max_len,
                                  uint32_t *actual_len,
                                  rtps_sequence_number_t *seq_number)
{
    if (reader == NULL || data == NULL || actual_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *actual_len = 0;
    
    /* 查找最小的序列号（按顺序提供） */
    rtps_cached_change_t **prev = &reader->receive_queue;
    rtps_cached_change_t *found = NULL;
    rtps_cached_change_t **found_prev = NULL;
    
    while (*prev != NULL) {
        if (found == NULL || 
            rtps_seqnum_compare(&(*prev)->seq_number, &found->seq_number) < 0) {
            found = *prev;
            found_prev = prev;
        }
        prev = &(*prev)->next;
    }
    
    if (found == NULL) {
        return ETH_TIMEOUT; /* 无数据 */
    }
    
    /* 复制数据 */
    if (found->data_len > max_len) {
        return ETH_ERROR; /* 缓冲区不足 */
    }
    
    memcpy(data, found->data, found->data_len);
    *actual_len = found->data_len;
    
    if (seq_number != NULL) {
        *seq_number = found->seq_number;
    }
    
    /* 从队列移除 */
    *found_prev = found->next;
    reader->receive_queue_size--;
    
    release_cached_change(found);
    
    return ETH_OK;
}

eth_status_t rtps_reader_sm_build_acknack(rtps_reader_state_machine_t *reader,
                                           const rtps_guid_t *writer_guid,
                                           uint8_t *buffer,
                                           uint32_t max_len,
                                           uint32_t *actual_len)
{
    if (reader == NULL || writer_guid == NULL || buffer == NULL || actual_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (max_len < 32) {
        return ETH_ERROR;
    }
    
    rtps_matched_writer_t *writer = find_matched_writer(reader, writer_guid);
    if (writer == NULL) {
        return ETH_ERROR;
    }
    
    uint32_t pos = 0;
    
    /* Reader ID */
    memcpy(&buffer[pos], reader->guid.entity_id, RTPS_ENTITY_ID_SIZE);
    pos += RTPS_ENTITY_ID_SIZE;
    
    /* Writer ID */
    memcpy(&buffer[pos], writer_guid->entity_id, RTPS_ENTITY_ID_SIZE);
    pos += RTPS_ENTITY_ID_SIZE;
    
    /* ACK位图基准序列号 */
    buffer[pos++] = reader->next_expected_seq.high >> 24;
    buffer[pos++] = reader->next_expected_seq.high >> 16;
    buffer[pos++] = reader->next_expected_seq.high >> 8;
    buffer[pos++] = reader->next_expected_seq.high;
    buffer[pos++] = reader->next_expected_seq.low >> 24;
    buffer[pos++] = reader->next_expected_seq.low >> 16;
    buffer[pos++] = reader->next_expected_seq.low >> 8;
    buffer[pos++] = reader->next_expected_seq.low;
    
    /* 构建位图 */
    uint32_t num_bits = writer->missing_changes.num_bits;
    buffer[pos++] = num_bits >> 24;
    buffer[pos++] = num_bits >> 16;
    buffer[pos++] = num_bits >> 8;
    buffer[pos++] = num_bits;
    
    /* 位图数据 */
    uint32_t num_words = (num_bits + 31) / 32;
    for (uint32_t i = 0; i < num_words && i < 8; i++) {
        buffer[pos++] = writer->missing_changes.bitmap[i] >> 24;
        buffer[pos++] = writer->missing_changes.bitmap[i] >> 16;
        buffer[pos++] = writer->missing_changes.bitmap[i] >> 8;
        buffer[pos++] = writer->missing_changes.bitmap[i];
    }
    
    /* ACKNACK计数 */
    buffer[pos++] = reader->acknack_count >> 24;
    buffer[pos++] = reader->acknack_count >> 16;
    buffer[pos++] = reader->acknack_count >> 8;
    buffer[pos++] = reader->acknack_count;
    
    *actual_len = pos;
    reader->must_send_acknack = false;
    
    return ETH_OK;
}

/* ============================================================================
 * 网络状态管理实现
 * ============================================================================ */

eth_status_t rtps_network_init(rtps_network_context_t *net_ctx)
{
    if (net_ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(net_ctx, 0, sizeof(rtps_network_context_t));
    net_ctx->state = RTPS_NETWORK_UP;
    
    return ETH_OK;
}

void rtps_network_deinit(rtps_network_context_t *net_ctx)
{
    if (net_ctx == NULL) {
        return;
    }
    
    net_ctx->state = RTPS_NETWORK_DOWN;
    net_ctx->on_network_suspend = NULL;
    net_ctx->on_network_resume = NULL;
}

eth_status_t rtps_network_suspend(rtps_network_context_t *net_ctx,
                                   uint64_t current_time_ms)
{
    if (net_ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (net_ctx->state != RTPS_NETWORK_UP && 
        net_ctx->state != RTPS_NETWORK_RECOVERING) {
        return ETH_OK; /* 已经挂起 */
    }
    
    net_ctx->state = RTPS_NETWORK_SUSPENDED;
    net_ctx->suspend_start_time = current_time_ms;
    net_ctx->suspend_count++;
    
    /* 调用挂起回调 */
    if (net_ctx->on_network_suspend != NULL) {
        net_ctx->on_network_suspend();
    }
    
    return ETH_OK;
}

eth_status_t rtps_network_resume(rtps_network_context_t *net_ctx,
                                  uint64_t current_time_ms)
{
    if (net_ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (net_ctx->state != RTPS_NETWORK_SUSPENDED) {
        return ETH_OK; /* 未挂起 */
    }
    
    /* 计算挂起时间 */
    if (net_ctx->suspend_start_time > 0) {
        uint64_t suspend_time = current_time_ms - net_ctx->suspend_start_time;
        net_ctx->total_suspend_time_ms += suspend_time;
    }
    
    net_ctx->state = RTPS_NETWORK_RECOVERING;
    net_ctx->recovery_start_time = current_time_ms;
    
    /* 调用恢复回调 */
    if (net_ctx->on_network_resume != NULL) {
        net_ctx->on_network_resume();
    }
    
    return ETH_OK;
}

rtps_network_state_t rtps_network_get_state(rtps_network_context_t *net_ctx)
{
    if (net_ctx == NULL) {
        return RTPS_NETWORK_DOWN;
    }
    
    return net_ctx->state;
}

bool rtps_network_is_available(rtps_network_context_t *net_ctx)
{
    if (net_ctx == NULL) {
        return false;
    }
    
    return (net_ctx->state == RTPS_NETWORK_UP) || 
           (net_ctx->state == RTPS_NETWORK_RECOVERING);
}

void rtps_network_set_callbacks(rtps_network_context_t *net_ctx,
                                 void (*on_suspend)(void),
                                 void (*on_resume)(void))
{
    if (net_ctx == NULL) {
        return;
    }
    
    net_ctx->on_network_suspend = on_suspend;
    net_ctx->on_network_resume = on_resume;
}
