/*******************************************************************************
 * @file health_check.c
 * @brief DDS健康检查实现 - 车载场景
 ******************************************************************************/

#include "health_check.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

/*==============================================================================
 * 常量和宏
 *============================================================================*/
#define DDS_HEALTH_MAGIC        0x4845414C  /* "HEAL" */
#define MAX_REGISTERED_NODES    256
#define MAX_REGISTERED_TOPICS   512
#define MAX_CONNECTIONS         1024
#define MAX_CALLBACKS           32

/*==============================================================================
 * 内部数据结构
 *============================================================================*/

typedef struct {
    dds_node_health_t       info;
    bool                    registered;
} node_entry_t;

typedef struct {
    dds_topic_match_t       info;
    bool                    registered;
} topic_entry_t;

typedef struct {
    dds_connection_quality_t    info;
    bool                        registered;
} connection_entry_t;

typedef struct {
    struct {
        dds_health_callback_t   callback;
        uint32_t                interval_ms;
        uint64_t                last_call;
        void*                   user_data;
    } status[MAX_CALLBACKS];
    
    struct {
        dds_alarm_callback_t    callback;
        void*                   user_data;
    } alarm[MAX_CALLBACKS];
    
    struct {
        dds_node_callback_t     callback;
        void*                   user_data;
    } node[MAX_CALLBACKS];
} callback_registry_t;

typedef struct {
    bool                    initialized;
    dds_health_config_t     config;
    uint64_t                start_time;
    
    /* 注册表 */
    node_entry_t            nodes[MAX_REGISTERED_NODES];
    uint32_t                node_count;
    
    topic_entry_t           topics[MAX_REGISTERED_TOPICS];
    uint32_t                topic_count;
    
    connection_entry_t      connections[MAX_CONNECTIONS];
    uint32_t                connection_count;
    
    /* 告警 */
    dds_health_alarm_t      alarms[DDS_HEALTH_MAX_ALARMS];
    uint32_t                alarm_count;
    uint32_t                next_alarm_id;
    
    /* 回调 */
    callback_registry_t     callbacks;
    
    /* 同步 */
    pthread_mutex_t         mutex;
    pthread_rwlock_t        rwlock;
    
    /* 工作线程 */
    pthread_t               worker_thread;
    volatile bool           worker_running;
} health_global_state_t;

static health_global_state_t g_health_state = {0};

/*==============================================================================
 * 内部函数前向声明
 *============================================================================*/
static void* health_worker_thread(void* arg);
static uint64_t get_timestamp_ms(void);
static void check_node_timeouts(void);
static void check_connection_health(void);
static void check_resource_limits(void);
static void update_overall_health(void);
static void trigger_alarm(dds_alarm_type_t type, dds_alarm_severity_t severity,
                          const char* description, const char* entity);
static dds_health_status_t calculate_health_score(const node_entry_t* node);

/*==============================================================================
 * 工具函数
 *============================================================================*/

static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static const char* health_status_to_string(dds_health_status_t status) {
    switch (status) {
        case DDS_HEALTH_HEALTHY:    return "HEALTHY";
        case DDS_HEALTH_DEGRADED:   return "DEGRADED";
        case DDS_HEALTH_UNHEALTHY:  return "UNHEALTHY";
        case DDS_HEALTH_CRITICAL:   return "CRITICAL";
        case DDS_HEALTH_OFFLINE:    return "OFFLINE";
        default:                    return "UNKNOWN";
    }
}

static const char* alarm_type_to_string(dds_alarm_type_t type) {
    switch (type) {
        case DDS_ALARM_TYPE_NODE_TIMEOUT:       return "NODE_TIMEOUT";
        case DDS_ALARM_TYPE_TOPIC_MISMATCH:     return "TOPIC_MISMATCH";
        case DDS_ALARM_TYPE_CONNECTION_LOST:    return "CONNECTION_LOST";
        case DDS_ALARM_TYPE_QOS_VIOLATION:      return "QOS_VIOLATION";
        case DDS_ALARM_TYPE_SECURITY_BREACH:    return "SECURITY_BREACH";
        case DDS_ALARM_TYPE_RESOURCE_EXHAUSTED: return "RESOURCE_EXHAUSTED";
        case DDS_ALARM_TYPE_LATENCY_SPIKE:      return "LATENCY_SPIKE";
        case DDS_ALARM_TYPE_PACKET_LOSS_HIGH:   return "PACKET_LOSS_HIGH";
        case DDS_ALARM_TYPE_MEMORY_LEAK:        return "MEMORY_LEAK";
        case DDS_ALARM_TYPE_CPU_OVERLOAD:       return "CPU_OVERLOAD";
        default:                                return "UNKNOWN";
    }
}

/*==============================================================================
 * 初始化和生命周期
 *============================================================================*/

int dds_health_init(const dds_health_config_t* config) {
    if (g_health_state.initialized) {
        return 0;
    }
    
    memset(&g_health_state, 0, sizeof(g_health_state));
    
    /* 复制配置 */
    if (config) {
        memcpy(&g_health_state.config, config, sizeof(dds_health_config_t));
    } else {
        /* 默认配置 */
        g_health_state.config.check_interval_ms = DDS_HEALTH_CHECK_INTERVAL_MS;
        g_health_state.config.node_timeout_ms = 5000;
        g_health_state.config.check_mask = DDS_CHECK_ALL;
        g_health_state.config.latency_warning_ms = 10;
        g_health_state.config.latency_critical_ms = 50;
        g_health_state.config.memory_warning_percent = 80;
        g_health_state.config.memory_critical_percent = 95;
        g_health_state.config.cpu_warning_percent = 70;
        g_health_state.config.cpu_critical_percent = 90;
        g_health_state.config.enable_auto_recovery = false;
    }
    
    g_health_state.start_time = get_timestamp_ms();
    
    pthread_mutex_init(&g_health_state.mutex, NULL);
    pthread_rwlock_init(&g_health_state.rwlock, NULL);
    
    /* 启动工作线程 */
    g_health_state.worker_running = true;
    pthread_create(&g_health_state.worker_thread, NULL, health_worker_thread, NULL);
    
    g_health_state.initialized = true;
    return 0;
}

void dds_health_deinit(void) {
    if (!g_health_state.initialized) {
        return;
    }
    
    g_health_state.worker_running = false;
    pthread_join(g_health_state.worker_thread, NULL);
    
    pthread_mutex_destroy(&g_health_state.mutex);
    pthread_rwlock_destroy(&g_health_state.rwlock);
    
    g_health_state.initialized = false;
}

struct dds_health_checker {
    char name[64];
    uint32_t check_count;
    uint64_t last_check_time;
};

dds_health_checker_t* dds_health_checker_create(const char* name) {
    dds_health_checker_t* checker = malloc(sizeof(dds_health_checker_t));
    if (checker) {
        memset(checker, 0, sizeof(dds_health_checker_t));
        if (name) {
            strncpy(checker->name, name, sizeof(checker->name) - 1);
        }
    }
    return checker;
}

void dds_health_checker_destroy(dds_health_checker_t* checker) {
    free(checker);
}

/*==============================================================================
 * 节点管理
 *============================================================================*/

int dds_health_register_node(const dds_node_health_t* node_info) {
    if (!node_info || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (g_health_state.nodes[i].registered &&
            strcmp(g_health_state.nodes[i].info.guid, node_info->guid) == 0) {
            /* 更新现有节点 */
            memcpy(&g_health_state.nodes[i].info, node_info, sizeof(dds_node_health_t));
            g_health_state.nodes[i].info.registration_time = get_timestamp_ms();
            g_health_state.nodes[i].info.last_seen_time = get_timestamp_ms();
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    /* 添加新节点 */
    if (g_health_state.node_count >= MAX_REGISTERED_NODES) {
        pthread_mutex_unlock(&g_health_state.mutex);
        return -1;
    }
    
    memcpy(&g_health_state.nodes[g_health_state.node_count].info, 
           node_info, sizeof(dds_node_health_t));
    g_health_state.nodes[g_health_state.node_count].info.registration_time = get_timestamp_ms();
    g_health_state.nodes[g_health_state.node_count].info.last_seen_time = get_timestamp_ms();
    g_health_state.nodes[g_health_state.node_count].registered = true;
    g_health_state.node_count++;
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return 0;
}

int dds_health_update_heartbeat(const char* node_guid) {
    if (!node_guid || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (g_health_state.nodes[i].registered &&
            strcmp(g_health_state.nodes[i].info.guid, node_guid) == 0) {
            g_health_state.nodes[i].info.last_heartbeat_time = get_timestamp_ms();
            g_health_state.nodes[i].info.last_seen_time = get_timestamp_ms();
            g_health_state.nodes[i].info.missed_heartbeats = 0;
            
            /* 如果之前是离线状态，更新为健康 */
            if (g_health_state.nodes[i].info.status == DDS_HEALTH_OFFLINE ||
                g_health_state.nodes[i].info.status == DDS_HEALTH_CRITICAL) {
                dds_health_status_t old_status = g_health_state.nodes[i].info.status;
                g_health_state.nodes[i].info.status = DDS_HEALTH_HEALTHY;
                
                /* 触发回调 */
                for (int j = 0; j < MAX_CALLBACKS; j++) {
                    if (g_health_state.callbacks.node[j].callback) {
                        g_health_state.callbacks.node[j].callback(
                            &g_health_state.nodes[i].info,
                            old_status,
                            DDS_HEALTH_HEALTHY,
                            g_health_state.callbacks.node[j].user_data
                        );
                    }
                }
            }
            
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return -1;
}

void dds_health_unregister_node(const char* node_guid) {
    if (!node_guid || !g_health_state.initialized) return;
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (g_health_state.nodes[i].registered &&
            strcmp(g_health_state.nodes[i].info.guid, node_guid) == 0) {
            g_health_state.nodes[i].registered = false;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
}

int dds_health_get_node_status(const char* node_guid, dds_node_health_t* status) {
    if (!node_guid || !status || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (g_health_state.nodes[i].registered &&
            strcmp(g_health_state.nodes[i].info.guid, node_guid) == 0) {
            memcpy(status, &g_health_state.nodes[i].info, sizeof(dds_node_health_t));
            pthread_rwlock_unlock(&g_health_state.rwlock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return -1;
}

int dds_health_get_all_nodes(dds_node_health_t* nodes, uint32_t max_count, 
                              uint32_t* actual_count) {
    if (!nodes || !actual_count || max_count == 0 || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    *actual_count = 0;
    for (uint32_t i = 0; i < g_health_state.node_count && *actual_count < max_count; i++) {
        if (g_health_state.nodes[i].registered) {
            memcpy(&nodes[*actual_count], &g_health_state.nodes[i].info, 
                   sizeof(dds_node_health_t));
            (*actual_count)++;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return 0;
}

/*==============================================================================
 * 主题管理
 *============================================================================*/

int dds_health_register_topic(const char* topic_name, const char* type_name) {
    if (!topic_name || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < g_health_state.topic_count; i++) {
        if (g_health_state.topics[i].registered &&
            strcmp(g_health_state.topics[i].info.topic_name, topic_name) == 0) {
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    /* 添加新主题 */
    if (g_health_state.topic_count >= MAX_REGISTERED_TOPICS) {
        pthread_mutex_unlock(&g_health_state.mutex);
        return -1;
    }
    
    strncpy(g_health_state.topics[g_health_state.topic_count].info.topic_name,
            topic_name, sizeof(g_health_state.topics[0].info.topic_name) - 1);
    if (type_name) {
        strncpy(g_health_state.topics[g_health_state.topic_count].info.type_name,
                type_name, sizeof(g_health_state.topics[0].info.type_name) - 1);
    }
    g_health_state.topics[g_health_state.topic_count].registered = true;
    g_health_state.topic_count++;
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return 0;
}

int dds_health_update_topic_match(const char* topic_name, bool matched) {
    if (!topic_name || !g_health_state.initialized) return -1;
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.topic_count; i++) {
        if (g_health_state.topics[i].registered &&
            strcmp(g_health_state.topics[i].info.topic_name, topic_name) == 0) {
            g_health_state.topics[i].info.has_matched = matched;
            if (matched) {
                g_health_state.topics[i].info.match_status = DDS_HEALTH_HEALTHY;
                g_health_state.topics[i].info.last_match_time = get_timestamp_ms();
            } else {
                g_health_state.topics[i].info.match_status = DDS_HEALTH_DEGRADED;
                g_health_state.topics[i].info.last_unmatch_time = get_timestamp_ms();
            }
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return -1;
}

int dds_health_get_topic_match(const char* topic_name, dds_topic_match_t* match) {
    if (!topic_name || !match || !g_health_state.initialized) return -1;
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    for (uint32_t i = 0; i < g_health_state.topic_count; i++) {
        if (g_health_state.topics[i].registered &&
            strcmp(g_health_state.topics[i].info.topic_name, topic_name) == 0) {
            memcpy(match, &g_health_state.topics[i].info, sizeof(dds_topic_match_t));
            pthread_rwlock_unlock(&g_health_state.rwlock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return -1;
}

int dds_health_get_all_topics(dds_topic_match_t* topics, uint32_t max_count, 
                               uint32_t* actual_count) {
    if (!topics || !actual_count || max_count == 0 || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    *actual_count = 0;
    for (uint32_t i = 0; i < g_health_state.topic_count && *actual_count < max_count; i++) {
        if (g_health_state.topics[i].registered) {
            memcpy(&topics[*actual_count], &g_health_state.topics[i].info, 
                   sizeof(dds_topic_match_t));
            (*actual_count)++;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return 0;
}

/*==============================================================================
 * 连接质量
 *============================================================================*/

int dds_health_register_connection(const char* local_guid, const char* remote_guid,
                                   const char* topic_name) {
    if (!local_guid || !remote_guid || !g_health_state.initialized) return -1;
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < g_health_state.connection_count; i++) {
        if (g_health_state.connections[i].registered &&
            strcmp(g_health_state.connections[i].info.local_guid, local_guid) == 0 &&
            strcmp(g_health_state.connections[i].info.remote_guid, remote_guid) == 0) {
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    /* 添加新连接 */
    if (g_health_state.connection_count >= MAX_CONNECTIONS) {
        pthread_mutex_unlock(&g_health_state.mutex);
        return -1;
    }
    
    strncpy(g_health_state.connections[g_health_state.connection_count].info.local_guid,
            local_guid, sizeof(g_health_state.connections[0].info.local_guid) - 1);
    strncpy(g_health_state.connections[g_health_state.connection_count].info.remote_guid,
            remote_guid, sizeof(g_health_state.connections[0].info.remote_guid) - 1);
    if (topic_name) {
        strncpy(g_health_state.connections[g_health_state.connection_count].info.topic_name,
                topic_name, sizeof(g_health_state.connections[0].info.topic_name) - 1);
    }
    g_health_state.connections[g_health_state.connection_count].registered = true;
    g_health_state.connection_count++;
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return 0;
}

int dds_health_update_connection_quality(const char* local_guid, const char* remote_guid,
                                          const dds_connection_quality_t* quality) {
    if (!local_guid || !remote_guid || !quality || !g_health_state.initialized) return -1;
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.connection_count; i++) {
        if (g_health_state.connections[i].registered &&
            strcmp(g_health_state.connections[i].info.local_guid, local_guid) == 0 &&
            strcmp(g_health_state.connections[i].info.remote_guid, remote_guid) == 0) {
            memcpy(&g_health_state.connections[i].info, quality, sizeof(dds_connection_quality_t));
            pthread_mutex_unlock(&g_health_state.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
    return -1;
}

int dds_health_get_connection_quality(const char* local_guid, const char* remote_guid,
                                       dds_connection_quality_t* quality) {
    if (!local_guid || !remote_guid || !quality || !g_health_state.initialized) return -1;
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    for (uint32_t i = 0; i < g_health_state.connection_count; i++) {
        if (g_health_state.connections[i].registered &&
            strcmp(g_health_state.connections[i].info.local_guid, local_guid) == 0 &&
            strcmp(g_health_state.connections[i].info.remote_guid, remote_guid) == 0) {
            memcpy(quality, &g_health_state.connections[i].info, sizeof(dds_connection_quality_t));
            pthread_rwlock_unlock(&g_health_state.rwlock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return -1;
}

/*==============================================================================
 * 健康检查工作线程
 *============================================================================*/

static void* health_worker_thread(void* arg) {
    (void)arg;
    
    while (g_health_state.worker_running) {
        /* 检查节点超时 */
        if (g_health_state.config.check_mask & DDS_CHECK_NODE_STATUS) {
            check_node_timeouts();
        }
        
        /* 检查连接健康 */
        if (g_health_state.config.check_mask & DDS_CHECK_CONNECTION_QUALITY) {
            check_connection_health();
        }
        
        /* 检查资源限制 */
        if (g_health_state.config.check_mask & DDS_CHECK_RESOURCE_USAGE) {
            check_resource_limits();
        }
        
        /* 更新整体状态 */
        update_overall_health();
        
        /* 检查间隔 */
        usleep(g_health_state.config.check_interval_ms * 1000);
    }
    
    return NULL;
}

static void check_node_timeouts(void) {
    uint64_t now = get_timestamp_ms();
    
    pthread_rwlock_wrlock(&g_health_state.rwlock);
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (!g_health_state.nodes[i].registered) continue;
        
        node_entry_t* node = &g_health_state.nodes[i];
        uint64_t elapsed = now - node->info.last_heartbeat_time;
        
        /* ASIL等级对应的超时检查 */
        uint32_t timeout = node->info.timeout_ms;
        if (timeout == 0) {
            switch (node->info.asil_level) {
                case 4: timeout = DDS_HEALTH_ASIL_D_TIMEOUT_MS; break;
                case 2: timeout = DDS_HEALTH_ASIL_B_TIMEOUT_MS; break;
                default: timeout = DDS_HEALTH_QM_TIMEOUT_MS; break;
            }
        }
        
        if (elapsed > timeout) {
            node->info.missed_heartbeats++;
            
            dds_health_status_t old_status = node->info.status;
            
            if (node->info.missed_heartbeats >= node->info.max_missed_heartbeats) {
                node->info.status = DDS_HEALTH_OFFLINE;
            } else if (elapsed > timeout * 2) {
                node->info.status = DDS_HEALTH_CRITICAL;
            } else if (elapsed > timeout * 1.5) {
                node->info.status = DDS_HEALTH_UNHEALTHY;
            } else {
                node->info.status = DDS_HEALTH_DEGRADED;
            }
            
            /* 发出告警 */
            if (node->info.status != old_status) {
                char desc[256];
                snprintf(desc, sizeof(desc), "Node %s heartbeat timeout (%lu ms)",
                         node->info.node_name, (unsigned long)elapsed);
                trigger_alarm(DDS_ALARM_TYPE_NODE_TIMEOUT, 
                              (node->info.status == DDS_HEALTH_CRITICAL) ? DDS_ALARM_CRITICAL : DDS_ALARM_WARNING,
                              desc, node->info.guid);
                
                /* 节点状态变化回调 */
                for (int j = 0; j < MAX_CALLBACKS; j++) {
                    if (g_health_state.callbacks.node[j].callback) {
                        g_health_state.callbacks.node[j].callback(
                            &node->info, old_status, node->info.status,
                            g_health_state.callbacks.node[j].user_data
                        );
                    }
                }
            }
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
}

static void check_connection_health(void) {
    pthread_rwlock_wrlock(&g_health_state.rwlock);
    
    for (uint32_t i = 0; i < g_health_state.connection_count; i++) {
        if (!g_health_state.connections[i].registered) continue;
        
        dds_connection_quality_t* conn = &g_health_state.connections[i].info;
        
        /* 计算健康评分 */
        uint32_t score = 100;
        
        if (conn->latency_us > g_health_state.config.latency_warning_ms * 1000) {
            score -= 20;
        }
        if (conn->latency_us > g_health_state.config.latency_critical_ms * 1000) {
            score -= 40;
        }
        if (conn->loss_rate_ppm > g_health_state.config.loss_warning_ppm) {
            score -= 20;
        }
        if (conn->loss_rate_ppm > g_health_state.config.loss_critical_ppm) {
            score -= 40;
        }
        
        conn->health_score = score;
        
        /* 更新状态 */
        if (score >= 80) {
            conn->quality_status = DDS_HEALTH_HEALTHY;
        } else if (score >= 60) {
            conn->quality_status = DDS_HEALTH_DEGRADED;
        } else if (score >= 40) {
            conn->quality_status = DDS_HEALTH_UNHEALTHY;
        } else {
            conn->quality_status = DDS_HEALTH_CRITICAL;
        }
        
        /* 检测告警 */
        if (conn->loss_rate_ppm > g_health_state.config.loss_critical_ppm) {
            char desc[256];
            snprintf(desc, sizeof(desc), "High packet loss on connection %s-%s (%u ppm)",
                     conn->local_guid, conn->remote_guid, conn->loss_rate_ppm);
            trigger_alarm(DDS_ALARM_TYPE_PACKET_LOSS_HIGH, DDS_ALARM_ERROR, desc, conn->topic_name);
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
}

static void check_resource_limits(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return;
    
    /* 简化版本：实际应用需要更复杂的资源监控 */
    uint32_t memory_percent = 0;
    uint32_t cpu_percent = 0;
    
    if (memory_percent > g_health_state.config.memory_critical_percent) {
        trigger_alarm(DDS_ALARM_TYPE_RESOURCE_EXHAUSTED, DDS_ALARM_CRITICAL,
                      "Memory usage exceeded critical threshold", "system");
    } else if (memory_percent > g_health_state.config.memory_warning_percent) {
        trigger_alarm(DDS_ALARM_TYPE_RESOURCE_EXHAUSTED, DDS_ALARM_WARNING,
                      "Memory usage exceeded warning threshold", "system");
    }
    
    if (cpu_percent > g_health_state.config.cpu_critical_percent) {
        trigger_alarm(DDS_ALARM_TYPE_CPU_OVERLOAD, DDS_ALARM_CRITICAL,
                      "CPU usage exceeded critical threshold", "system");
    }
}

static void update_overall_health(void) {
    pthread_rwlock_wrlock(&g_health_state.rwlock);
    
    /* 计算整体健康状态 */
    uint32_t healthy_nodes = 0;
    uint32_t degraded_nodes = 0;
    uint32_t unhealthy_nodes = 0;
    uint32_t offline_nodes = 0;
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (!g_health_state.nodes[i].registered) continue;
        
        switch (g_health_state.nodes[i].info.status) {
            case DDS_HEALTH_HEALTHY:    healthy_nodes++; break;
            case DDS_HEALTH_DEGRADED:   degraded_nodes++; break;
            case DDS_HEALTH_UNHEALTHY:  unhealthy_nodes++; break;
            case DDS_HEALTH_OFFLINE:    offline_nodes++; break;
            default: break;
        }
    }
    
    /* 保存统计 */
    /* 实际应用可以将这些统计保存到某处供g_health_state使用 */
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
}

static void trigger_alarm(dds_alarm_type_t type, dds_alarm_severity_t severity,
                          const char* description, const char* entity) {
    pthread_mutex_lock(&g_health_state.mutex);
    
    /* 检查是否已存在相同的活动告警 */
    for (uint32_t i = 0; i < g_health_state.alarm_count; i++) {
        if (g_health_state.alarms[i].is_active &&
            g_health_state.alarms[i].type == type &&
            strcmp(g_health_state.alarms[i].affected_entity, entity) == 0) {
            /* 更新现有告警 */
            g_health_state.alarms[i].occurrence_count++;
            g_health_state.alarms[i].last_occurrence = get_timestamp_ms();
            pthread_mutex_unlock(&g_health_state.mutex);
            return;
        }
    }
    
    /* 创建新告警 */
    if (g_health_state.alarm_count < DDS_HEALTH_MAX_ALARMS) {
        dds_health_alarm_t* alarm = &g_health_state.alarms[g_health_state.alarm_count];
        alarm->alarm_id = ++g_health_state.next_alarm_id;
        alarm->type = type;
        alarm->severity = severity;
        alarm->timestamp = get_timestamp_ms();
        alarm->is_active = true;
        alarm->occurrence_count = 1;
        alarm->last_occurrence = alarm->timestamp;
        strncpy(alarm->description, description, sizeof(alarm->description) - 1);
        strncpy(alarm->affected_entity, entity, sizeof(alarm->affected_entity) - 1);
        
        g_health_state.alarm_count++;
        
        /* 触发回调 */
        for (int i = 0; i < MAX_CALLBACKS; i++) {
            if (g_health_state.callbacks.alarm[i].callback) {
                g_health_state.callbacks.alarm[i].callback(
                    alarm, g_health_state.callbacks.alarm[i].user_data
                );
            }
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
}

/*==============================================================================
 * 健康检查
 *============================================================================*/

int dds_health_perform_check(dds_health_checker_t* checker) {
    if (!checker) return -1;
    
    checker->check_count++;
    checker->last_check_time = get_timestamp_ms();
    
    /* 执行所有启用的检查 */
    if (g_health_state.config.check_mask & DDS_CHECK_NODE_STATUS) {
        check_node_timeouts();
    }
    if (g_health_state.config.check_mask & DDS_CHECK_CONNECTION_QUALITY) {
        check_connection_health();
    }
    if (g_health_state.config.check_mask & DDS_CHECK_RESOURCE_USAGE) {
        check_resource_limits();
    }
    
    update_overall_health();
    
    return 0;
}

void dds_health_get_report(dds_health_report_t* report) {
    if (!report || !g_health_state.initialized) return;
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    memset(report, 0, sizeof(dds_health_report_t));
    report->report_time = get_timestamp_ms();
    report->uptime_seconds = (report->report_time - g_health_state.start_time) / 1000;
    
    /* 统计节点 */
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (!g_health_state.nodes[i].registered) continue;
        
        switch (g_health_state.nodes[i].info.status) {
            case DDS_HEALTH_HEALTHY:    report->healthy_nodes++; break;
            case DDS_HEALTH_DEGRADED:   report->degraded_nodes++; break;
            case DDS_HEALTH_UNHEALTHY:  report->unhealthy_nodes++; break;
            case DDS_HEALTH_OFFLINE:    report->offline_nodes++; break;
            default: break;
        }
    }
    
    /* 统计主题 */
    for (uint32_t i = 0; i < g_health_state.topic_count; i++) {
        if (!g_health_state.topics[i].registered) continue;
        
        if (g_health_state.topics[i].info.has_matched) {
            report->matched_topics++;
        } else {
            report->unmatched_topics++;
        }
    }
    
    /* 统计连接 */
    for (uint32_t i = 0; i < g_health_state.connection_count; i++) {
        if (!g_health_state.connections[i].registered) continue;
        
        if (g_health_state.connections[i].info.quality_status == DDS_HEALTH_HEALTHY) {
            report->healthy_connections++;
        } else {
            report->degraded_connections++;
        }
    }
    
    /* 统计告警 */
    for (uint32_t i = 0; i < g_health_state.alarm_count; i++) {
        if (g_health_state.alarms[i].is_active) {
            report->active_alarms++;
            switch (g_health_state.alarms[i].severity) {
                case DDS_ALARM_WARNING: report->warning_count++; break;
                case DDS_ALARM_ERROR:   report->error_count++; break;
                case DDS_ALARM_CRITICAL: report->critical_count++; break;
                default: break;
            }
        }
    }
    
    /* 计算整体状态 */
    uint32_t total_nodes = report->healthy_nodes + report->degraded_nodes + 
                           report->unhealthy_nodes + report->offline_nodes;
    if (total_nodes > 0) {
        uint32_t score = (report->healthy_nodes * 100 + 
                         report->degraded_nodes * 70 +
                         report->unhealthy_nodes * 40 +
                         report->offline_nodes * 0) / total_nodes;
        report->overall_score = score;
        
        if (score >= 90) {
            report->overall_status = DDS_HEALTH_HEALTHY;
        } else if (score >= 70) {
            report->overall_status = DDS_HEALTH_DEGRADED;
        } else if (score >= 50) {
            report->overall_status = DDS_HEALTH_UNHEALTHY;
        } else {
            report->overall_status = DDS_HEALTH_CRITICAL;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
}

void dds_health_get_resource_usage(dds_resource_usage_t* usage) {
    if (!usage || !g_health_state.initialized) return;
    
    memset(usage, 0, sizeof(dds_resource_usage_t));
    
    struct rusage rusage;
    if (getrusage(RUSAGE_SELF, &rusage) == 0) {
        usage->memory_usage_kb = rusage.ru_maxrss;
    }
    
    /* 获取系统信息需要更复杂的实现 */
}

/*==============================================================================
 * 告警管理
 *============================================================================*/

int dds_health_get_active_alarms(dds_health_alarm_t* alarms, uint32_t max_count, 
                                  uint32_t* actual_count) {
    if (!alarms || !actual_count || max_count == 0 || !g_health_state.initialized) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&g_health_state.rwlock);
    
    *actual_count = 0;
    for (uint32_t i = 0; i < g_health_state.alarm_count && *actual_count < max_count; i++) {
        if (g_health_state.alarms[i].is_active) {
            memcpy(&alarms[*actual_count], &g_health_state.alarms[i], sizeof(dds_health_alarm_t));
            (*actual_count)++;
        }
    }
    
    pthread_rwlock_unlock(&g_health_state.rwlock);
    return 0;
}

void dds_health_acknowledge_alarm(uint32_t alarm_id) {
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.alarm_count; i++) {
        if (g_health_state.alarms[i].alarm_id == alarm_id && 
            g_health_state.alarms[i].is_active) {
            /* 确认告警 - 实际应用可能需要记录确认人 */
            break;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
}

void dds_health_clear_alarm(uint32_t alarm_id) {
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.alarm_count; i++) {
        if (g_health_state.alarms[i].alarm_id == alarm_id) {
            g_health_state.alarms[i].is_active = false;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
}

int dds_health_register_alarm_callback(dds_alarm_callback_t callback, void* user_data) {
    if (!callback || !g_health_state.initialized) return -1;
    
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.alarm[i].callback == NULL) {
            g_health_state.callbacks.alarm[i].callback = callback;
            g_health_state.callbacks.alarm[i].user_data = user_data;
            return 0;
        }
    }
    return -1;
}

void dds_health_unregister_alarm_callback(dds_alarm_callback_t callback) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.alarm[i].callback == callback) {
            g_health_state.callbacks.alarm[i].callback = NULL;
            break;
        }
    }
}

/*==============================================================================
 * 回调注册
 *============================================================================*/

int dds_health_register_status_callback(dds_health_callback_t callback, 
                                         uint32_t interval_ms, void* user_data) {
    if (!callback || !g_health_state.initialized) return -1;
    
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.status[i].callback == NULL) {
            g_health_state.callbacks.status[i].callback = callback;
            g_health_state.callbacks.status[i].interval_ms = interval_ms;
            g_health_state.callbacks.status[i].last_call = 0;
            g_health_state.callbacks.status[i].user_data = user_data;
            return 0;
        }
    }
    return -1;
}

void dds_health_unregister_status_callback(dds_health_callback_t callback) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.status[i].callback == callback) {
            g_health_state.callbacks.status[i].callback = NULL;
            break;
        }
    }
}

int dds_health_register_node_callback(dds_node_callback_t callback, void* user_data) {
    if (!callback || !g_health_state.initialized) return -1;
    
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.node[i].callback == NULL) {
            g_health_state.callbacks.node[i].callback = callback;
            g_health_state.callbacks.node[i].user_data = user_data;
            return 0;
        }
    }
    return -1;
}

void dds_health_unregister_node_callback(dds_node_callback_t callback) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (g_health_state.callbacks.node[i].callback == callback) {
            g_health_state.callbacks.node[i].callback = NULL;
            break;
        }
    }
}

/*==============================================================================
 * 报告生成
 *============================================================================*/

int dds_health_generate_text_report(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0 || !g_health_state.initialized) {
        return -1;
    }
    
    dds_health_report_t report;
    dds_health_get_report(&report);
    
    int written = 0;
    
    written += snprintf(buffer + written, buffer_size - written,
                        "===================================\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "DDS Health Report\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "===================================\n\n");
    
    written += snprintf(buffer + written, buffer_size - written,
                        "Overall Status: %s\n", 
                        health_status_to_string(report.overall_status));
    written += snprintf(buffer + written, buffer_size - written,
                        "Overall Score: %u/100\n", report.overall_score);
    written += snprintf(buffer + written, buffer_size - written,
                        "Uptime: %lu seconds\n\n", (unsigned long)report.uptime_seconds);
    
    written += snprintf(buffer + written, buffer_size - written,
                        "Nodes:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Healthy:   %u\n", report.healthy_nodes);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Degraded:  %u\n", report.degraded_nodes);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Unhealthy: %u\n", report.unhealthy_nodes);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Offline:   %u\n\n", report.offline_nodes);
    
    written += snprintf(buffer + written, buffer_size - written,
                        "Topics:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Matched:   %u\n", report.matched_topics);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Unmatched: %u\n\n", report.unmatched_topics);
    
    written += snprintf(buffer + written, buffer_size - written,
                        "Connections:\n");
    written += snprintf(buffer + written, buffer_size - written,
                        "  Healthy:   %u\n", report.healthy_connections);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Degraded:  %u\n\n", report.degraded_connections);
    
    written += snprintf(buffer + written, buffer_size - written,
                        "Active Alarms: %u\n", report.active_alarms);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Critical: %u\n", report.critical_count);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Error:    %u\n", report.error_count);
    written += snprintf(buffer + written, buffer_size - written,
                        "  Warning:  %u\n", report.warning_count);
    
    written += snprintf(buffer + written, buffer_size - written,
                        "===================================\n");
    
    return written;
}

int dds_health_generate_json_report(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0 || !g_health_state.initialized) {
        return -1;
    }
    
    dds_health_report_t report;
    dds_health_get_report(&report);
    
    int written = snprintf(buffer, buffer_size,
        "{\n"
        "  \"report_time\": %lu,\n"
        "  \"uptime_seconds\": %lu,\n"
        "  \"overall_status\": \"%s\",\n"
        "  \"overall_score\": %u,\n"
        "  \"nodes\": {\n"
        "    \"healthy\": %u,\n"
        "    \"degraded\": %u,\n"
        "    \"unhealthy\": %u,\n"
        "    \"offline\": %u\n"
        "  },\n"
        "  \"topics\": {\n"
        "    \"matched\": %u,\n"
        "    \"unmatched\": %u\n"
        "  },\n"
        "  \"connections\": {\n"
        "    \"healthy\": %u,\n"
        "    \"degraded\": %u\n"
        "  },\n"
        "  \"alarms\": {\n"
        "    \"active\": %u,\n"
        "    \"critical\": %u,\n"
        "    \"error\": %u,\n"
        "    \"warning\": %u\n"
        "  }\n"
        "}\n",
        (unsigned long)report.report_time,
        (unsigned long)report.uptime_seconds,
        health_status_to_string(report.overall_status),
        report.overall_score,
        report.healthy_nodes, report.degraded_nodes,
        report.unhealthy_nodes, report.offline_nodes,
        report.matched_topics, report.unmatched_topics,
        report.healthy_connections, report.degraded_connections,
        report.active_alarms, report.critical_count,
        report.error_count, report.warning_count
    );
    
    return written;
}

/*==============================================================================
 * 高级功能（简化版本）
 *============================================================================*/

void dds_health_enable_check(dds_health_check_type_t check_type, bool enable) {
    if (!g_health_state.initialized) return;
    
    if (enable) {
        g_health_state.config.check_mask |= check_type;
    } else {
        g_health_state.config.check_mask &= ~check_type;
    }
}

void dds_health_set_node_timeout(const char* node_guid, uint32_t timeout_ms) {
    if (!node_guid || !g_health_state.initialized) return;
    
    pthread_mutex_lock(&g_health_state.mutex);
    
    for (uint32_t i = 0; i < g_health_state.node_count; i++) {
        if (g_health_state.nodes[i].registered &&
            strcmp(g_health_state.nodes[i].info.guid, node_guid) == 0) {
            g_health_state.nodes[i].info.timeout_ms = timeout_ms;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_health_state.mutex);
}

int dds_health_export_uds_format(uint8_t* buffer, uint32_t* size) {
    (void)buffer;
    (void)size;
    return 0;
}

int dds_health_export_ota_format(uint8_t* buffer, uint32_t max_size, uint32_t* actual_size) {
    (void)buffer;
    (void)max_size;
    (void)actual_size;
    return 0;
}

int dds_health_troubleshoot(const char* entity_guid, char* diagnosis, size_t diagnosis_size) {
    (void)entity_guid;
    (void)diagnosis;
    (void)diagnosis_size;
    return 0;
}

int dds_health_get_remediation(const char* entity_guid, char** actions, uint32_t max_actions) {
    (void)entity_guid;
    (void)actions;
    (void)max_actions;
    return 0;
}

void dds_health_reset_check(dds_health_check_type_t check_type) {
    (void)check_type;
}
