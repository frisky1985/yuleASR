/**
 * @file dds_qos.c
 * @brief DDS QoS 默认值实现
 * @version 1.0
 * @date 2026-08-04
 *
 * 实现 dds_core.h 声明的 6 个 QoS 默认值初始化函数:
 * dds_domain_participant_qos_init / dds_publisher_qos_init /
 * dds_subscriber_qos_init / dds_topic_qos_init /
 * dds_datawriter_qos_init / dds_datareader_qos_init
 */
#include "dds_qos.h"
#include <string.h>

/* ============================================================================
 * DomainParticipant QoS 默认值
 * ============================================================================ */

void dds_domain_participant_qos_init(dds_DomainParticipantQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->user_data_max_size = 1024;
    qos->entity_factory_autoenable = true;
}

/* ============================================================================
 * Publisher QoS 默认值
 * ============================================================================ */

void dds_publisher_qos_init(dds_PublisherQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->partition_count = 0;
    qos->entity_factory_autoenable = true;
}

/* ============================================================================
 * Subscriber QoS 默认值
 * ============================================================================ */

void dds_subscriber_qos_init(dds_SubscriberQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->partition_count = 0;
    qos->entity_factory_autoenable = true;
}

/* ============================================================================
 * Topic QoS 默认值
 * ============================================================================ */

void dds_topic_qos_init(dds_TopicQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->max_samples_per_instance = 1;
}

/* ============================================================================
 * DataWriter QoS 默认值
 * ============================================================================ */

void dds_datawriter_qos_init(dds_DataWriterQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->autodispose_unregistered_instances = true;
    qos->max_samples = 100;
    qos->max_instances = 10;
    qos->max_samples_per_instance = 10;
}

/* ============================================================================
 * DataReader QoS 默认值
 * ============================================================================ */

void dds_datareader_qos_init(dds_DataReaderQosType *qos)
{
    if (qos == NULL) {
        return;
    }
    memset(qos, 0, sizeof(*qos));
    qos->base.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos->base.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos->base.deadline_ms = 0;
    qos->base.latency_budget_ms = 0;
    qos->base.history_depth = 1;
    qos->max_samples = 100;
    qos->max_instances = 10;
    qos->max_samples_per_instance = 10;
}
