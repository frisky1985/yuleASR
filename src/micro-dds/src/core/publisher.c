/** @file publisher.c
 * @brief DDS发布者实现
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
    DDS_DomainParticipant participant;
    DDS_PublisherQos qos;
    bool is_valid;
} Publisher_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static Publisher_State g_publishers[MICRODDS_MAX_PUBLISHERS];

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_Publisher find_free_publisher_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_PUBLISHERS; i++) {
        if (!g_publishers[i].is_valid) {
            return (DDS_Publisher)&g_publishers[i];
        }
    }
    return NULL;
}

static void init_default_publisher_qos(DDS_PublisherQos* qos) {
    if (qos != NULL) {
        qos->presentation.access_scope = DDS_INSTANCE_PRESENTATION_QOS;
        qos->presentation.coherent_access = false;
        qos->presentation.ordered_access = false;
        qos->partition.name._maximum = 0U;
        qos->partition.name._length = 0U;
        qos->partition.name._buffer = NULL;
        qos->partition.name._release = false;
        qos->group_data.value._maximum = 0U;
        qos->group_data.value._length = 0U;
        qos->group_data.value._buffer = NULL;
        qos->group_data.value._release = false;
        qos->entity_factory.autoenable_created_entities = true;
    }
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_Publisher DDS_Publisher_create(
    DDS_DomainParticipant participant,
    const DDS_PublisherQos* qos) {
    
    if (participant == NULL) {
        return NULL;
    }

    DDS_Publisher publisher = find_free_publisher_slot();
    if (publisher == NULL) {
        return NULL;
    }

    Publisher_State* state = (Publisher_State*)publisher;

    state->participant = participant;

    if (qos != NULL) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_PublisherQos));
    } else {
        init_default_publisher_qos(&state->qos);
    }

    state->is_valid = true;

    return publisher;
}

DDS_ReturnCode_t DDS_Publisher_delete(DDS_Publisher publisher) {
    if (publisher == NULL) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    Publisher_State* state = (Publisher_State*)publisher;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->is_valid = false;

    return DDS_RETCODE_OK;
}
