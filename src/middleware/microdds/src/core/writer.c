/** @file writer.c
 * @brief DDS数据写入器实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */
/* @req SHALL_MICRODDS */


#include "microdds/microdds.h"
#include <string.h>

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    DDS_Publisher publisher;
    DDS_Topic topic;
    DDS_DataWriterQos qos;
    bool is_valid;
    uint32_t sequence_number;
} DataWriter_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static DataWriter_State g_writers[MICRODDS_MAX_DATA_WRITERS];

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_DataWriter find_free_writer_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_DATA_WRITERS; i++) {
        if (!g_writers[i].is_valid) {
            return (DDS_DataWriter)&g_writers[i];
        }
    }
    return NULL_PTR;
}

static DDS_ReturnCode_t init_default_writer_qos(DDS_DataWriterQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 使用主题默认值作为基础 */
    qos->durability.kind = DDS_VOLATILE_DURABILITY_QOS;
    qos->deadline.period = DDS_DURATION_INFINITE;
    qos->latency_budget.duration = DDS_DURATION_ZERO;
    qos->liveliness.kind = DDS_AUTOMATIC_LIVELINESS_QOS;
    qos->liveliness.lease_duration = DDS_DURATION_INFINITE;
    qos->reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    qos->reliability.max_blocking_time = DDS_DURATION_ZERO;
    qos->destination_order.kind = DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
    qos->history.depth = 1;
    qos->resource_limits.max_samples = -1;
    qos->resource_limits.max_instances = -1;
    qos->resource_limits.max_samples_per_instance = -1;
    qos->transport_priority.value = 0;
    qos->lifespan.duration = DDS_DURATION_INFINITE;
    qos->ownership.kind = DDS_SHARED_OWNERSHIP_QOS;
    qos->ownership_strength.value = 0;
    qos->writer_data_lifecycle.autodispose_unregistered_instances = true;
    qos->user_data.value._maximum = 0U;
    qos->user_data.value._length = 0U;
    qos->user_data.value._buffer = NULL_PTR;
    qos->user_data.value._release = false;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_DataWriter DDS_DataWriter_create(
    DDS_Publisher publisher,
    DDS_Topic topic,
    const DDS_DataWriterQos* qos) {
    
    if ((publisher == NULL_PTR) || (topic == NULL_PTR)) {
        return NULL_PTR;
    }

    DDS_DataWriter writer = find_free_writer_slot();
    if (writer == NULL_PTR) {
        return NULL_PTR;
    }

    DataWriter_State* state = (DataWriter_State*)writer;

    state->publisher = publisher;
    state->topic = topic;

    if (qos != NULL_PTR) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_DataWriterQos));
    } else {
        (void)init_default_writer_qos(&state->qos);
    }

    state->sequence_number = 0U;
    state->is_valid = true;

    return writer;
}

DDS_ReturnCode_t DDS_DataWriter_delete(DDS_DataWriter writer) {
    if (writer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DataWriter_State* state = (DataWriter_State*)writer;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->is_valid = false;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataWriter_write(
    DDS_DataWriter writer,
    const void* data,
    DDS_InstanceHandle_t handle) {
    
    (void)handle;  /* 暂时不使用 */

    if ((writer == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DataWriter_State* state = (DataWriter_State*)writer;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    /* 递增序列号 */
    state->sequence_number++;

    /* 实际数据发送将在运输层实现 */
    /* 此处仅记录写入操作 */

    return DDS_RETCODE_OK;
}
