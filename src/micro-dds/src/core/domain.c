/** @file domain.c
 * @brief DDS域参与者管理实现
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
    DDS_DomainId_t domain_id;
    DDS_DomainParticipantQos qos;
    bool is_valid;
    uint32_t ref_count;
} DomainParticipant_State;

/* ============================================================================
 * 静态内存分配
 * ============================================================================ */

static DomainParticipant_State g_participants[MICRODDS_MAX_PARTICIPANTS];
static bool g_initialized = false;

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static DDS_DomainParticipant find_free_participant_slot(void) {
    for (uint32_t i = 0U; i < MICRODDS_MAX_PARTICIPANTS; i++) {
        if (!g_participants[i].is_valid) {
            return (DDS_DomainParticipant)&g_participants[i];
        }
    }
    return NULL;
}

static void init_default_participant_qos(DDS_DomainParticipantQos* qos) {
    if (qos != NULL) {
        qos->user_data.value._maximum = 0U;
        qos->user_data.value._length = 0U;
        qos->user_data.value._buffer = NULL;
        qos->user_data.value._release = false;
        qos->entity_factory.autoenable_created_entities = true;
    }
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

DDS_ReturnCode_t MicroDDS_init(void) {
    if (g_initialized) {
        return DDS_RETCODE_OK;
    }

    for (uint32_t i = 0U; i < MICRODDS_MAX_PARTICIPANTS; i++) {
        g_participants[i].is_valid = false;
        g_participants[i].ref_count = 0U;
    }

    g_initialized = true;
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t MicroDDS_shutdown(void) {
    if (!g_initialized) {
        return DDS_RETCODE_OK;
    }

    /* 清理所有活动的域参与者 */
    for (uint32_t i = 0U; i < MICRODDS_MAX_PARTICIPANTS; i++) {
        if (g_participants[i].is_valid) {
            g_participants[i].is_valid = false;
            g_participants[i].ref_count = 0U;
        }
    }

    g_initialized = false;
    return DDS_RETCODE_OK;
}

const char* MicroDDS_get_version_string(void) {
    return "Micro-DDS v0.1.0";
}

DDS_DomainParticipant DDS_DomainParticipant_create(
    DDS_DomainId_t domain_id,
    const DDS_DomainParticipantQos* qos) {
    
    if (!g_initialized) {
        (void)MicroDDS_init();
    }

    DDS_DomainParticipant participant = find_free_participant_slot();
    if (participant == NULL) {
        return NULL;
    }

    DomainParticipant_State* state = (DomainParticipant_State*)participant;
    
    state->domain_id = domain_id;
    
    if (qos != NULL) {
        (void)memcpy(&state->qos, qos, sizeof(DDS_DomainParticipantQos));
    } else {
        init_default_participant_qos(&state->qos);
    }
    
    state->is_valid = true;
    state->ref_count = 1U;

    return participant;
}

DDS_ReturnCode_t DDS_DomainParticipant_delete(DDS_DomainParticipant participant) {
    if (participant == NULL) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DomainParticipant_State* state = (DomainParticipant_State*)participant;
    
    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    state->ref_count--;
    
    if (state->ref_count == 0U) {
        state->is_valid = false;
    }

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DomainParticipant_get_qos(
    DDS_DomainParticipant participant,
    DDS_DomainParticipantQos* qos) {
    
    if ((participant == NULL) || (qos == NULL)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    const DomainParticipant_State* state = (DomainParticipant_State*)participant;
    
    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    (void)memcpy(qos, &state->qos, sizeof(DDS_DomainParticipantQos));
    
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t DDS_DomainParticipant_set_qos(
    DDS_DomainParticipant participant,
    const DDS_DomainParticipantQos* qos) {
    
    if ((participant == NULL) || (qos == NULL)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    DomainParticipant_State* state = (DomainParticipant_State*)participant;
    
    if (!state->is_valid) {
        return DDS_RETCODE_ALREADY_DELETED;
    }

    (void)memcpy(&state->qos, qos, sizeof(DDS_DomainParticipantQos));
    
    return DDS_RETCODE_OK;
}
