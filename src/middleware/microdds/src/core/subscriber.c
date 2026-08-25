/** @file subscriber.c
 * @brief DDS订阅者实现
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
    DDS_DomainParticipant participant;
    DDS_SubscriberQos qos;
    bool is_valid;
} Subscriber_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static Subscriber_State g_subscribers[MICRODDS_MAX_SUBSCRIBERS];

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_Subscriber find_free_subscriber_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_SUBSCRIBERS; i++) {
        if (!g_subscribers[i].is_valid) {
            return (DDS_Subscriber)&g_subscribers[i];
        }
    }
    return NULL_PTR;
}

static void init_default_subscriber_qos(DDS_SubscriberQos* qos) {
    if (qos != NULL_PTR) {
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
    }
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_Subscriber DDS_Subscriber_create(
    DDS_DomainParticipant participant,
    const DDS_SubscriberQos* qos) {
    
    if (participant == NULL_PTR) {
        return NULL_PTR;
    }

    DDS_Subscriber subscriber = find_free_subscriber_slot();
    if (subscriber == NULL_PTR) {
        return NULL_PTR;
    }

    Subscriber_State* state = (Subscriber_State*)subscriber;

    state->participant = participant;

    if (qos != NULL_PTR) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_SubscriberQos));
    } else {
        init_default_subscriber_qos(&state->qos);
    }

    state->is_valid = true;

    return subscriber;
}

DDS_ReturnCode_t DDS_Subscriber_delete(DDS_Subscriber subscriber) {
    if (subscriber == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    Subscriber_State* state = (Subscriber_State*)subscriber;

    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->is_valid = false;

    return DDS_RETCODE_OK;
}
