/** @file qos_policy.c
 * @brief QoS策略实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "microdds/qos.h"
#include <string.h>

/* ============================================================================
 * 默认值初始化函数实现
 * ============================================================================ */

DDS_ReturnCode_t DDS_TopicQos_init_default(DDS_TopicQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* TopicData */
    qos->topic_data.value._maximum = 0U;
    qos->topic_data.value._length = 0U;
    qos->topic_data.value._buffer = NULL_PTR;
    qos->topic_data.value._release = false;

    /* Durability */
    qos->durability.kind = DDS_VOLATILE_DURABILITY_QOS;

    /* Deadline */
    qos->deadline.period = DDS_DURATION_INFINITE;

    /* LatencyBudget */
    qos->latency_budget.duration = DDS_DURATION_ZERO;

    /* Liveliness */
    qos->liveliness.kind = DDS_AUTOMATIC_LIVELINESS_QOS;
    qos->liveliness.lease_duration = DDS_DURATION_INFINITE;

    /* Reliability */
    qos->reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    qos->reliability.max_blocking_time = DDS_DURATION_ZERO;

    /* DestinationOrder */
    qos->destination_order.kind = DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

    /* History */
    qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
    qos->history.depth = 1;

    /* ResourceLimits */
    qos->resource_limits.max_samples = -1;
    qos->resource_limits.max_instances = -1;
    qos->resource_limits.max_samples_per_instance = -1;

    /* TransportPriority */
    qos->transport_priority.value = 0;

    /* Lifespan */
    qos->lifespan.duration = DDS_DURATION_INFINITE;

    /* Ownership */
    qos->ownership.kind = DDS_SHARED_OWNERSHIP_QOS;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataWriterQos_init_default(DDS_DataWriterQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 先初始化为与主题相同的值 */
    (void)DDS_TopicQos_init_default((DDS_TopicQos*)qos);

    /* 然后设置写入器特定的字段 */
    qos->ownership_strength.value = 0;
    qos->writer_data_lifecycle.autodispose_unregistered_instances = true;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataReaderQos_init_default(DDS_DataReaderQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 先初始化为与主题相同的值 */
    (void)DDS_TopicQos_init_default((DDS_TopicQos*)qos);

    /* 然后设置读取器特定的字段 */
    qos->time_based_filter.minimum_separation = DDS_DURATION_ZERO;
    qos->reader_data_lifecycle.autopurge_nowriter_samples_delay = DDS_DURATION_INFINITE;
    qos->reader_data_lifecycle.autopurge_disposed_samples_delay = DDS_DURATION_INFINITE;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_PublisherQos_init_default(DDS_PublisherQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    qos->presentation.access_scope = DDS_INSTANCE_PRESENTATION_QOS;
    qos->presentation.coherent_access = false;
    qos->presentation.ordered_access = false;

    qos->partition.name._maximum = 0U;
    qos->partition.name._length = 0U;
    qos->partition.name._buffer = NULL_PTR;
    qos->partition.name._release = false;

    qos->group_data.value._maximum = 0U;
    qos->group_data.value._length = 0U;
    qos->group_data.value._buffer = NULL_PTR;
    qos->group_data.value._release = false;

    qos->entity_factory.autoenable_created_entities = true;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_SubscriberQos_init_default(DDS_SubscriberQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 与PublisherQos相同 */
    return DDS_PublisherQos_init_default((DDS_PublisherQos*)qos);
}

DDS_ReturnCode_t DDS_DomainParticipantQos_init_default(DDS_DomainParticipantQos* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    qos->user_data.value._maximum = 0U;
    qos->user_data.value._length = 0U;
    qos->user_data.value._buffer = NULL_PTR;
    qos->user_data.value._release = false;

    qos->entity_factory.autoenable_created_entities = true;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * QoS复制函数实现
 * ============================================================================ */

DDS_ReturnCode_t DDS_TopicQos_copy(DDS_TopicQos* dst, const DDS_TopicQos* src) {
    if ((dst == NULL_PTR) || (src == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    (void)memcpy(dst, src, sizeof(DDS_TopicQos));
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataWriterQos_copy(DDS_DataWriterQos* dst, const DDS_DataWriterQos* src) {
    if ((dst == NULL_PTR) || (src == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    (void)memcpy(dst, src, sizeof(DDS_DataWriterQos));
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DataReaderQos_copy(DDS_DataReaderQos* dst, const DDS_DataReaderQos* src) {
    if ((dst == NULL_PTR) || (src == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    (void)memcpy(dst, src, sizeof(DDS_DataReaderQos));
    return DDS_RETCODE_OK;
}

/* ============================================================================
 * QoS兼容性检查实现
 * ============================================================================ */

bool DDS_TopicQos_is_compatible(const DDS_TopicQos* offered, const DDS_TopicQos* requested) {
    if ((offered == NULL_PTR) || (requested == NULL_PTR)) {
        return false;
    }

    /* 检查可靠性策略 */
    if (offered->reliability.kind < requested->reliability.kind) {
        return false;
    }

    /* 检查持久性策略 */
    if (offered->durability.kind < requested->durability.kind) {
        return false;
    }

    /* 检查过滤器策略 */
    if (offered->history.kind < requested->history.kind) {
        return false;
    }

    return true;
}

bool DDS_DataWriterReaderQos_is_compatible(const DDS_DataWriterQos* writer_qos,
                                           const DDS_DataReaderQos* reader_qos) {
    if ((writer_qos == NULL_PTR) || (reader_qos == NULL_PTR)) {
        return false;
    }

    /* 检查可靠性 */
    if (writer_qos->reliability.kind < reader_qos->reliability.kind) {
        return false;
    }

    /* 检查持久性 */
    if (writer_qos->durability.kind < reader_qos->durability.kind) {
        return false;
    }

    /* 检查截止日期 */
    if (writer_qos->deadline.period > reader_qos->deadline.period) {
        return false;
    }

    return true;
}
