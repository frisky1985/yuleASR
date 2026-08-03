/**
 * @file ownership.c
 * @brief DDS所有权策略实现
 * @version 1.0
 * @date 2026-04-24
 */

#define _GNU_SOURCE
#include "ownership.h"
#include "../../common/log/dds_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * 私有数据结构定义
 * ============================================================================ */

typedef struct {
    dds_guid_t guid;
    uint32_t strength;
    uint64_t last_seen;
    uint8_t priority;
    bool active;
} own_contender_t;

struct own_handle {
    own_config_t config;
    dds_guid_t local_guid;
    uint32_t current_strength;
    own_state_t state;
    own_owner_info_t current_owner;
    own_contender_t contenders[OWN_MAX_CONTENDERS];
    uint8_t contender_count;
    own_event_callback_t event_callback;
    own_transfer_callback_t transfer_callback;
    void *event_user_data;
    void *transfer_user_data;
    own_stats_t stats;
    uint64_t ownership_start_time;
    bool asil_enabled;
    uint8_t asil_level;
    uint32_t pending_transfer_requests;
    dds_guid_t pending_transfer_target;
};

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

static inline uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000;
}

static inline void notify_event(own_handle_t *own, own_event_t event, const void *data) {
    if (own->event_callback) {
        own->event_callback(event, data, own->event_user_data);
    }
}

static int compare_contenders(const void *a, const void *b) {
    const own_contender_t *ca = (const own_contender_t*)a;
    const own_contender_t *cb = (const own_contender_t*)b;
    
    if (!ca->active && !cb->active) return 0;
    if (!ca->active) return 1;
    if (!cb->active) return -1;
    
    // 首先比较强度(降序)
    if (ca->strength != cb->strength) {
        return (cb->strength > ca->strength) ? 1 : -1;
    }
    // 强度相同时比较优先级
    return (cb->priority > ca->priority) ? 1 : -1;
}

static void sort_contenders(own_handle_t *own) {
    if (own->contender_count > 1) {
        qsort(own->contenders, own->contender_count, sizeof(own_contender_t), compare_contenders);
    }
}

static own_contender_t* find_contender(own_handle_t *own, const dds_guid_t *guid) {
    for (uint8_t i = 0; i < own->contender_count; i++) {
        if (memcmp(&own->contenders[i].guid, guid, sizeof(dds_guid_t)) == 0) {
            return &own->contenders[i];
        }
    }
    return NULL;
}

static own_contender_t* add_contender(own_handle_t *own, const dds_guid_t *guid, uint32_t strength) {
    // 检查是否已存在
    own_contender_t *existing = find_contender(own, guid);
    if (existing) {
        existing->strength = strength;
        existing->last_seen = get_time_ms();
        return existing;
    }
    
    // 添加新竞争者
    if (own->contender_count < OWN_MAX_CONTENDERS) {
        own_contender_t *c = &own->contenders[own->contender_count++];
        memcpy(&c->guid, guid, sizeof(dds_guid_t));
        c->strength = strength;
        c->last_seen = get_time_ms();
        c->priority = 0;
        c->active = true;
        return c;
    }
    
    // 替换最弱的不活跃竞争者
    for (uint8_t i = 0; i < own->contender_count; i++) {
        if (!own->contenders[i].active) {
            memcpy(&own->contenders[i].guid, guid, sizeof(dds_guid_t));
            own->contenders[i].strength = strength;
            own->contenders[i].last_seen = get_time_ms();
            own->contenders[i].active = true;
            return &own->contenders[i];
        }
    }
    
    return NULL;
}

static void remove_contender(own_handle_t *own, const dds_guid_t *guid) {
    own_contender_t *c = find_contender(own, guid);
    if (c) {
        c->active = false;
    }
}

static void determine_owner(own_handle_t *own) {
    sort_contenders(own);
    
    if (own->contender_count == 0 || !own->contenders[0].active) {
        // 没有活跃竞争者，当前节点成为所有权主
        if (own->state != OWN_STATE_OWNER) {
            own->state = OWN_STATE_OWNER;
            own->stats.ownership_gained_count++;
            own->ownership_start_time = get_time_ms();
            notify_event(own, OWN_EVENT_OWNERSHIP_GAINED, &own->local_guid);
        }
        memcpy(&own->current_owner.guid, &own->local_guid, sizeof(dds_guid_t));
        own->current_owner.strength = own->current_strength;
        own->current_owner.state = OWN_STATE_OWNER;
        own->current_owner.active = true;
        return;
    }
    
    own_contender_t *top = &own->contenders[0];
    bool we_are_owner = (memcmp(&top->guid, &own->local_guid, sizeof(dds_guid_t)) == 0);
    
    if (we_are_owner) {
        if (own->state != OWN_STATE_OWNER) {
            own->state = OWN_STATE_OWNER;
            own->stats.ownership_gained_count++;
            own->ownership_start_time = get_time_ms();
            notify_event(own, OWN_EVENT_OWNERSHIP_GAINED, &own->local_guid);
        }
    } else {
        if (own->state == OWN_STATE_OWNER) {
            own->state = OWN_STATE_CONTENDER;
            own->stats.ownership_lost_count++;
            uint64_t ownership_duration = get_time_ms() - own->ownership_start_time;
            own->stats.ownership_duration_ms += ownership_duration;
            notify_event(own, OWN_EVENT_OWNERSHIP_LOST, &top->guid);
        }
        memcpy(&own->current_owner.guid, &top->guid, sizeof(dds_guid_t));
        own->current_owner.strength = top->strength;
        own->current_owner.last_heartbeat = top->last_seen;
    }
    
    own->current_owner.active = top->active;
}

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

eth_status_t own_init(void) {
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership module initialized");
    return ETH_OK;
}

void own_deinit(void) {
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership module deinitialized");
}

own_handle_t* own_create(const own_config_t *config, const dds_guid_t *local_guid) {
    if (!config || !local_guid) return NULL;
    
    own_handle_t *own = (own_handle_t*)calloc(1, sizeof(own_handle_t));
    if (!own) return NULL;
    
    memcpy(&own->config, config, sizeof(own_config_t));
    memcpy(&own->local_guid, local_guid, sizeof(dds_guid_t));
    own->current_strength = config->initial_strength;
    own->state = OWN_STATE_IDLE;
    own->contender_count = 0;
    
    // 添加自己作为竞争者
    own_contender_t *self = add_contender(own, local_guid, own->current_strength);
    if (self) {
        self->priority = 255; // 最高优先级
    }
    
    determine_owner(own);
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Created ownership manager: type=%s, strength=%u",
                config->type == OWN_TYPE_EXCLUSIVE ? "EXCLUSIVE" : "SHARED",
                own->current_strength);
    
    return own;
}

eth_status_t own_delete(own_handle_t *own) {
    if (!own) return ETH_INVALID_PARAM;
    
    if (own->state == OWN_STATE_OWNER) {
        uint64_t ownership_duration = get_time_ms() - own->ownership_start_time;
        own->stats.ownership_duration_ms += ownership_duration;
    }
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Deleted ownership manager");
    free(own);
    return ETH_OK;
}

eth_status_t own_register_event_callback(own_handle_t *own, own_event_callback_t callback, void *user_data) {
    if (!own) return ETH_INVALID_PARAM;
    
    own->event_callback = callback;
    own->event_user_data = user_data;
    return ETH_OK;
}

eth_status_t own_register_transfer_callback(own_handle_t *own, own_transfer_callback_t callback, void *user_data) {
    if (!own) return ETH_INVALID_PARAM;
    
    own->transfer_callback = callback;
    own->transfer_user_data = user_data;
    return ETH_OK;
}

eth_status_t own_set_strength(own_handle_t *own, uint32_t strength) {
    if (!own) return ETH_INVALID_PARAM;
    
    // 限制强度范围
    if (strength > own->config.max_strength) {
        strength = own->config.max_strength;
    }
    if (strength < own->config.min_strength) {
        strength = own->config.min_strength;
    }
    
    uint32_t old_strength = own->current_strength;
    own->current_strength = strength;
    
    // 更新竞争者列表中的自己
    own_contender_t *self = find_contender(own, &own->local_guid);
    if (self) {
        self->strength = strength;
    }
    
    // 重新确定所有权
    determine_owner(own);
    
    if (old_strength != strength) {
        own->stats.current_strength = strength;
        notify_event(own, OWN_EVENT_STRENGTH_CHANGED, &strength);
    }
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Strength changed: %u -> %u", old_strength, strength);
    return ETH_OK;
}

eth_status_t own_get_strength(own_handle_t *own, uint32_t *strength) {
    if (!own || !strength) return ETH_INVALID_PARAM;
    
    *strength = own->current_strength;
    return ETH_OK;
}

eth_status_t own_increase_strength(own_handle_t *own, uint32_t increment) {
    if (!own) return ETH_INVALID_PARAM;
    
    uint32_t new_strength = own->current_strength + increment;
    return own_set_strength(own, new_strength);
}

eth_status_t own_decrease_strength(own_handle_t *own, uint32_t decrement) {
    if (!own) return ETH_INVALID_PARAM;
    
    uint32_t new_strength = (own->current_strength > decrement) ? 
                            own->current_strength - decrement : 0;
    return own_set_strength(own, new_strength);
}

eth_status_t own_process_strength_announcement(own_handle_t *own, 
                                                const dds_guid_t *remote_guid,
                                                uint32_t remote_strength) {
    if (!own || !remote_guid) return ETH_INVALID_PARAM;
    
    // 检查是否是自己
    if (memcmp(remote_guid, &own->local_guid, sizeof(dds_guid_t)) == 0) {
        return ETH_OK;
    }
    
    DDS_LOG_DEBUG(DDS_LOG_MODULE_CORE, "OWN", "Received strength announcement: %u", remote_strength);
    
    own_contender_t *c = add_contender(own, remote_guid, remote_strength);
    if (!c) {
        DDS_LOG_WARN(DDS_LOG_MODULE_CORE, "OWN", "Failed to add/update contender");
        return ETH_ERROR;
    }
    
    notify_event(own, OWN_EVENT_CONTENDER_APPEARED, remote_guid);
    
    // 在EXCLUSIVE模式下重新确定所有权
    if (own->config.type == OWN_TYPE_EXCLUSIVE) {
        determine_owner(own);
    }
    
    own->stats.negotiation_count++;
    return ETH_OK;
}

eth_status_t own_is_owner(own_handle_t *own, bool *is_owner) {
    if (!own || !is_owner) return ETH_INVALID_PARAM;
    
    *is_owner = (own->state == OWN_STATE_OWNER);
    return ETH_OK;
}

eth_status_t own_get_current_owner(own_handle_t *own, own_owner_info_t *owner_info) {
    if (!own || !owner_info) return ETH_INVALID_PARAM;
    
    memcpy(owner_info, &own->current_owner, sizeof(own_owner_info_t));
    return ETH_OK;
}

eth_status_t own_negotiate(own_handle_t *own) {
    if (!own) return ETH_INVALID_PARAM;
    
    own->state = OWN_STATE_NEGOTIATING;
    
    // 广播自己的强度
    // 实际应用中这里会通过DDS发送强度宣告
    
    // 等待响应(简化版本立即结束)
    determine_owner(own);
    
    own->state = (own->state == OWN_STATE_OWNER) ? OWN_STATE_OWNER : OWN_STATE_CONTENDER;
    return ETH_OK;
}

eth_status_t own_request_transfer(own_handle_t *own, const dds_guid_t *target_guid) {
    if (!own || !target_guid) return ETH_INVALID_PARAM;
    
    if (own->state != OWN_STATE_OWNER) {
        return ETH_ERROR;
    }
    
    own->pending_transfer_requests++;
    memcpy(&own->pending_transfer_target, target_guid, sizeof(dds_guid_t));
    
    // 详细实现需要发送转移请求消息
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership transfer requested to target");
    return ETH_OK;
}

eth_status_t own_accept_transfer(own_handle_t *own, uint32_t new_strength) {
    if (!own) return ETH_INVALID_PARAM;
    
    // 接受转移，设置新强度
    if (new_strength > 0) {
        own_set_strength(own, new_strength);
    }
    
    own->state = OWN_STATE_OWNER;
    own->stats.ownership_gained_count++;
    own->ownership_start_time = get_time_ms();
    own->stats.transfer_count++;
    
    notify_event(own, OWN_EVENT_OWNERSHIP_GAINED, &own->local_guid);
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership transfer accepted");
    return ETH_OK;
}

eth_status_t own_reject_transfer(own_handle_t *own, const char *reason) {
    if (!own) return ETH_INVALID_PARAM;
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership transfer rejected: %s", reason ? reason : "no reason");
    return ETH_OK;
}

eth_status_t own_update_heartbeat(own_handle_t *own, const dds_guid_t *owner_guid) {
    if (!own || !owner_guid) return ETH_INVALID_PARAM;
    
    own_contender_t *c = find_contender(own, owner_guid);
    if (c) {
        c->last_seen = get_time_ms();
    }
    
    return ETH_OK;
}

eth_status_t own_check_timeout(own_handle_t *own, uint64_t current_time_ms, bool *timed_out) {
    if (!own || !timed_out) return ETH_INVALID_PARAM;
    
    *timed_out = false;
    
    // 检查当前所有权主是否超时
    if (own->current_owner.active && 
        memcmp(&own->current_owner.guid, &own->local_guid, sizeof(dds_guid_t)) != 0) {
        
        uint64_t timeout_threshold = own->config.negotiation_timeout_ms;
        if (own->asil_enabled && own->asil_level >= 3) {
            timeout_threshold = timeout_threshold / 2; // ASIL-C/D更严格
        }
        
        if (current_time_ms - own->current_owner.last_heartbeat > timeout_threshold) {
            *timed_out = true;
            
            // 移除超时的竞争者
            remove_contender(own, &own->current_owner.guid);
            
            // 发送所有权主断开事件
            notify_event(own, OWN_EVENT_OWNER_DISCONNECTED, &own->current_owner.guid);
            
            // 重新确定所有权
            determine_owner(own);
            
            DDS_LOG_WARN(DDS_LOG_MODULE_CORE, "OWN", "Current owner timed out");
        }
    }
    
    return ETH_OK;
}

eth_status_t own_handle_owner_failure(own_handle_t *own) {
    if (!own) return ETH_INVALID_PARAM;
    
    DDS_LOG_ERROR(DDS_LOG_MODULE_CORE, "OWN", "Handling owner failure");
    
    // 检查是否是自己失败
    if (own->state == OWN_STATE_OWNER) {
        // 降低强度让渡所有权
        own_decrease_strength(own, own->current_strength);
    } else {
        // 移除失败的所有权主
        remove_contender(own, &own->current_owner.guid);
        determine_owner(own);
    }
    
    return ETH_OK;
}

eth_status_t own_get_stats(own_handle_t *own, own_stats_t *stats) {
    if (!own || !stats) return ETH_INVALID_PARAM;
    
    memcpy(stats, &own->stats, sizeof(own_stats_t));
    return ETH_OK;
}

eth_status_t own_reset_stats(own_handle_t *own) {
    if (!own) return ETH_INVALID_PARAM;
    
    memset(&own->stats, 0, sizeof(own_stats_t));
    return ETH_OK;
}

eth_status_t own_enable_asil_mode(own_handle_t *own, uint8_t asil_level) {
    if (!own || asil_level > 4) return ETH_INVALID_PARAM;
    
    own->asil_enabled = true;
    own->asil_level = asil_level;
    
    // ASIL模式下更严格的配置
    own->config.auto_strength_adjust = false; // 禁用自动调整
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "ASIL mode enabled (level=%d)", asil_level);
    return ETH_OK;
}

eth_status_t own_get_owner_list(own_handle_t *own, 
                                 own_owner_info_t *owners,
                                 uint8_t max_count,
                                 uint8_t *actual_count) {
    if (!own || !owners || !actual_count || max_count == 0) return ETH_INVALID_PARAM;
    
    sort_contenders(own);
    
    *actual_count = 0;
    for (uint8_t i = 0; i < own->contender_count && *actual_count < max_count; i++) {
        if (own->contenders[i].active) {
            memcpy(&owners[*actual_count].guid, &own->contenders[i].guid, sizeof(dds_guid_t));
            owners[*actual_count].strength = own->contenders[i].strength;
            owners[*actual_count].last_heartbeat = own->contenders[i].last_seen;
            owners[*actual_count].active = true;
            (*actual_count)++;
        }
    }
    
    return ETH_OK;
}

eth_status_t own_force_ownership(own_handle_t *own, uint32_t new_strength) {
    if (!own) return ETH_INVALID_PARAM;
    
    // 管理员强制获取所有权
    own_set_strength(own, new_strength > 0 ? new_strength : OWN_MAX_STRENGTH);
    
    DDS_LOG_INFO(DDS_LOG_MODULE_CORE, "OWN", "Ownership forced (admin command)");
    return ETH_OK;
}
