/** @file topic.c
 * @brief DDS主题管理实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "microdds/microdds.h"
#include <string.h>

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    char name[MICRODDS_TOPIC_NAME_MAX];
    char type_name[MICRODDS_TYPE_NAME_MAX];
    DDS_TopicQos qos;
    DDS_DomainParticipant participant;
    bool is_valid;
} Topic_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static Topic_State g_topics[MICRODDS_MAX_TOPICS];

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_Topic find_free_topic_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_TOPICS; i++) {
        if (!g_topics[i].is_valid) {
            return (DDS_Topic)&g_topics[i];
        }
    }
    return NULL;
}

static DDS_ReturnCode_t init_default_topic_qos(DDS_TopicQos* qos) {
    if (qos == NULL) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 初始化为默认值 */
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
    qos->resource_limits.max_samples = -1;  /* 无限制 */
    qos->resource_limits.max_instances = -1;
    qos->resource_limits.max_samples_per_instance = -1;
    qos->transport_priority.value = 0;
    qos->lifespan.duration = DDS_DURATION_INFINITE;
    qos->ownership.kind = DDS_SHARED_OWNERSHIP_QOS;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_Topic DDS_Topic_create(
    DDS_DomainParticipant participant,
    const char* name,
    const char* type_name,
    const DDS_TopicQos* qos) {
    
    if ((participant == NULL) || (name == NULL)) {
        return NULL;
    }

    DDS_Topic topic = find_free_topic_slot();
    if (topic == NULL) {
        return NULL;
    }

    Topic_State* state = (Topic_State*)topic;

    /* 复制主题名称 */
    (void)strncpy(state->name, name, MICRODDS_TOPIC_NAME_MAX - 1U);
    state->name[MICRODDS_TOPIC_NAME_MAX - 1U] = '\0';

    /* 复制类型名称 */
    if (type_name != NULL) {
        (void)strncpy(state->type_name, type_name, MICRODDS_TYPE_NAME_MAX - 1U);
        state->type_name[MICRODDS_TYPE_NAME_MAX - 1U] = '\0';
    } else {
        state->type_name[0] = '\0';
    }

    /* 设置QoS */
    if (qos != NULL) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_TopicQos));
    } else {
        (void)init_default_topic_qos(&state->qos);
    }

    state->participant = participant;
    state->is_valid = true;

    return topic;
}

DDS_ReturnCode_t DDS_Topic_delete(DDS_Topic topic) {
    if (topic == NULL) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    Topic_State* state = (Topic_State*)topic;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->is_valid = false;

    return DDS_RETCODE_OK;
}

const char* DDS_Topic_get_name(DDS_Topic topic) {
    if (topic == NULL) {
        return NULL;
    }

    Topic_State* state = (Topic_State*)topic;

    if (!state->is_valid) {
        return NULL;
    }

    return state->name;
}

const char* DDS_Topic_get_type_name(DDS_Topic topic) {
    if (topic == NULL) {
        return NULL;
    }

    Topic_State* state = (Topic_State*)topic;

    if (!state->is_valid) {
        return NULL;
    }

    return state->type_name;
}
