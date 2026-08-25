/** @file reader.c
 * @brief DDS数据读取器实现
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
    DDS_Subscriber subscriber;
    DDS_Topic topic;
    DDS_DataReaderQos qos;
    bool is_valid;
    DDS_DataAvailableCallback callback;
} DataReader_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static DataReader_State g_readers[MICRODDS_MAX_DATA_READERS];

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_DataReader find_free_reader_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_DATA_READERS; i++) {
        if (!g_readers[i].is_valid) {
            return (DDS_DataReader)&g_readers[i];
        }
    }
    return NULL_PTR;
}

static DDS_ReturnCode_t init_default_reader_qos(DDS_DataReaderQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

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
    qos->ownership.kind = DDS_SHARED_OWNERSHIP_QOS;
    qos->user_data.value._maximum = 0U;
    qos->user_data.value._length = 0U;
    qos->user_data.value._buffer = NULL_PTR;
    qos->user_data.value._release = false;
    qos->time_based_filter.minimum_separation = DDS_DURATION_ZERO;
    qos->reader_data_lifecycle.autopurge_nowriter_samples_delay = DDS_DURATION_INFINITE;
    qos->reader_data_lifecycle.autopurge_disposed_samples_delay = DDS_DURATION_INFINITE;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_DataReader DDS_DataReader_create(
    DDS_Subscriber subscriber,
    DDS_Topic topic,
    const DDS_DataReaderQos* qos) {
    
    if ((subscriber == NULL_PTR) || (topic == NULL_PTR)) {
        return NULL_PTR;
    }

    DDS_DataReader reader = find_free_reader_slot();
    if (reader == NULL_PTR) {
        return NULL_PTR;
    }

    DataReader_State* state = (DataReader_State*)reader;

    state->subscriber = subscriber;
    state->topic = topic;

    if (qos != NULL_PTR) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_DataReaderQos));
    } else {
        (void)init_default_reader_qos(&state->qos);
    }

    state->callback = NULL_PTR;
    state->is_valid = true;

    return reader;
}

DDS_ReturnCode_t DDS_DataReader_delete(DDS_DataReader reader) {
    if (reader == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DataReader_State* state = (DataReader_State*)reader;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->is_valid = false;

    return DDS_RETCODE_OK;
}

int32_t DDS_DataReader_read(
    DDS_DataReader reader,
    void** data_samples,
    DDS_SampleInfo* sample_infos,
    int32_t max_samples) {
    
    if ((reader == NULL_PTR) || (data_samples == NULL_PTR) || (sample_infos == NULL_PTR)) {
        return 0;
    }

    const DataReader_State* state = (DataReader_State*)reader;

    if (!state->is_valid) {
        return 0;
    }

    (void)max_samples;  /* 暂时不使用 */

    /* 实际读取操作将在运输层实现 */
    /* 此处仅返回空结果 */

    return 0;
}

int32_t DDS_DataReader_take(
    DDS_DataReader reader,
    void** data_samples,
    DDS_SampleInfo* sample_infos,
    int32_t max_samples) {
    
    /* 实现类似于read，但会移除样本 */
    return DDS_DataReader_read(reader, data_samples, sample_infos, max_samples);
}

DDS_ReturnCode_t DDS_DataReader_return_loan(DDS_DataReader reader) {
    (void)reader;  /* 暂时不使用 */
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataReader_set_data_available_callback(
    DDS_DataReader reader,
    DDS_DataAvailableCallback callback) {
    
    if (reader == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DataReader_State* state = (DataReader_State*)reader;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->callback = callback;

    return DDS_RETCODE_OK;
}
