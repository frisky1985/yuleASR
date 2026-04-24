/**
 * @file transport_manager.c
 * @brief 传输层管理器实现 - 多传输层协调与路由决策
 * @version 1.0
 * @date 2026-04-24
 */

#include "transport_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/syscall.h>

/* ============================================================================
 * 管理器私有数据结构
 * ============================================================================ */

/** 传输管理器实例 */
typedef struct {
    tm_config_t config;
    
    /* 传输层表 */
    tm_transport_entry_t transports[TM_MAX_TRANSPORTS];
    uint32_t transport_count;
    pthread_mutex_t transports_mutex;
    
    /* 端点表 */
    tm_endpoint_entry_t endpoints[TM_MAX_ENDPOINTS];
    uint32_t endpoint_count;
    pthread_mutex_t endpoints_mutex;
    
    /* 路由表 */
    tm_route_entry_t routes[TM_MAX_ROUTES];
    uint32_t route_count;
    pthread_mutex_t routes_mutex;
    
    /* 统计 */
    tm_stats_t stats;
    pthread_mutex_t stats_mutex;
    
    /* 健康检查线程 */
    pthread_t health_thread;
    volatile int health_thread_running;
    
    /* 状态 */
    volatile int initialized;
} transport_manager_t;

/* 全局管理器实例 */
static transport_manager_t g_manager = {0};

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 获取当前时间(微秒)
 */
static inline uint64_t get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/**
 * @brief 计算路由延迟得分(越低越好)
 */
static uint32_t calculate_latency_score(tm_transport_entry_t *transport)
{
    if (transport == NULL || transport->state != TM_STATE_ACTIVE) {
        return 0xFFFFFFFF;
    }
    
    return transport->latency_us;
}

/**
 * @brief 计算路由吞吐量得分(越高越好)
 */
static uint32_t calculate_throughput_score(tm_transport_entry_t *transport)
{
    if (transport == NULL || transport->state != TM_STATE_ACTIVE) {
        return 0;
    }
    
    return transport->throughput_mbps;
}

/**
 * @brief 检查端点是否为本地
 */
static int is_local_endpoint(tm_endpoint_entry_t *endpoint)
{
    if (endpoint == NULL) {
        return 0;
    }
    
    return (endpoint->type == TM_EP_LOCAL_PROCESS || 
            endpoint->type == TM_EP_LOCAL_NODE);
}

/**
 * @brief 查找可用的共享内存传输层
 */
static int find_shm_transport(transport_manager_t *mgr, uint32_t *transport_id)
{
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        if (mgr->transports[i].type == TRANSPORT_SHM &&
            mgr->transports[i].state == TM_STATE_ACTIVE) {
            *transport_id = i;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 查找可用的UDP传输层
 */
static int find_udp_transport(transport_manager_t *mgr, uint32_t *transport_id)
{
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        if (mgr->transports[i].type == TRANSPORT_UDP &&
            mgr->transports[i].state == TM_STATE_ACTIVE) {
            *transport_id = i;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 根据策略选择最佳传输层
 */
static int select_transport_for_route(transport_manager_t *mgr,
                                       tm_endpoint_entry_t *src,
                                       tm_endpoint_entry_t *dst,
                                       tm_routing_policy_t policy,
                                       uint32_t *selected_transport)
{
    switch (policy) {
        case TM_ROUTE_PREFER_SHM:
            /* 优先共享内存 */
            if (is_local_endpoint(src) && is_local_endpoint(dst)) {
                if (find_shm_transport(mgr, selected_transport) == 0) {
                    return 0;
                }
            }
            return find_udp_transport(mgr, selected_transport);
            
        case TM_ROUTE_PREFER_UDP:
            /* 优先UDP */
            return find_udp_transport(mgr, selected_transport);
            
        case TM_ROUTE_LOW_LATENCY:
            /* 选择延迟最低的 */
            {
                uint32_t best_latency = 0xFFFFFFFF;
                int best_idx = -1;
                for (uint32_t i = 0; i < mgr->transport_count; i++) {
                    if (mgr->transports[i].state == TM_STATE_ACTIVE) {
                        if (mgr->transports[i].latency_us < best_latency) {
                            best_latency = mgr->transports[i].latency_us;
                            best_idx = (int)i;
                        }
                    }
                }
                if (best_idx >= 0) {
                    *selected_transport = (uint32_t)best_idx;
                    return 0;
                }
            }
            return -1;
            
        case TM_ROUTE_HIGH_THROUGHPUT:
            /* 选择吞吐量最高的 */
            {
                uint32_t best_throughput = 0;
                int best_idx = -1;
                for (uint32_t i = 0; i < mgr->transport_count; i++) {
                    if (mgr->transports[i].state == TM_STATE_ACTIVE) {
                        if (mgr->transports[i].throughput_mbps > best_throughput) {
                            best_throughput = mgr->transports[i].throughput_mbps;
                            best_idx = (int)i;
                        }
                    }
                }
                if (best_idx >= 0) {
                    *selected_transport = (uint32_t)best_idx;
                    return 0;
                }
            }
            return -1;
            
        case TM_ROUTE_DETERMINISTIC:
            /* 确定性路由 - 优先共享内存 */
            if (is_local_endpoint(src) && is_local_endpoint(dst)) {
                if (find_shm_transport(mgr, selected_transport) == 0) {
                    return 0;
                }
            }
            return find_udp_transport(mgr, selected_transport);
            
        case TM_ROUTE_AUTO:
        default:
            /* 自动选择 */
            if (is_local_endpoint(src) && is_local_endpoint(dst)) {
                /* 同节点使用共享内存 */
                if (find_shm_transport(mgr, selected_transport) == 0) {
                    return 0;
                }
            }
            /* 跨节点使用UDP */
            return find_udp_transport(mgr, selected_transport);
    }
    
    return -1;
}

/**
 * @brief 健康检查线程
 */
static void* health_check_thread(void *arg)
{
    transport_manager_t *mgr = (transport_manager_t *)arg;
    
    while (mgr->health_thread_running) {
        uint32_t interval_ms = mgr->config.health_check_interval_ms;
        if (interval_ms == 0) {
            interval_ms = 1000; /* 默认1秒 */
        }
        
        usleep(interval_ms * 1000);
        
        pthread_mutex_lock(&mgr->transports_mutex);
        
        uint64_t now = get_time_us();
        
        for (uint32_t i = 0; i < mgr->transport_count; i++) {
            tm_transport_entry_t *transport = &mgr->transports[i];
            
            if (transport->state == TM_STATE_ACTIVE) {
                /* 检查超时 */
                uint64_t idle_time = now - transport->last_used;
                if (idle_time > (uint64_t)mgr->config.heartbeat_timeout_ms * 1000) {
                    transport->state = TM_STATE_DEGRADED;
                }
                
                /* 检查错误率 */
                if (transport->error_count > mgr->config.failover_threshold) {
                    transport->state = TM_STATE_FAILED;
                }
            }
        }
        
        pthread_mutex_unlock(&mgr->transports_mutex);
    }
    
    return NULL;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

eth_status_t tm_init(const tm_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (g_manager.initialized) {
        return ETH_ERROR; /* 已初始化 */
    }
    
    memset(&g_manager, 0, sizeof(g_manager));
    memcpy(&g_manager.config, config, sizeof(tm_config_t));
    
    /* 初始化互斥锁 */
    pthread_mutex_init(&g_manager.transports_mutex, NULL);
    pthread_mutex_init(&g_manager.endpoints_mutex, NULL);
    pthread_mutex_init(&g_manager.routes_mutex, NULL);
    pthread_mutex_init(&g_manager.stats_mutex, NULL);
    
    /* 启动健康检查线程 */
    if (config->health_check_interval_ms > 0) {
        g_manager.health_thread_running = 1;
        pthread_create(&g_manager.health_thread, NULL, health_check_thread, &g_manager);
    }
    
    g_manager.initialized = 1;
    
    return ETH_OK;
}

eth_status_t tm_deinit(void)
{
    if (!g_manager.initialized) {
        return ETH_OK;
    }
    
    /* 停止健康检查线程 */
    g_manager.health_thread_running = 0;
    if (g_manager.health_thread) {
        pthread_join(g_manager.health_thread, NULL);
    }
    
    /* 清理所有传输层 */
    pthread_mutex_lock(&g_manager.transports_mutex);
    for (uint32_t i = 0; i < g_manager.transport_count; i++) {
        if (g_manager.transports[i].handle != NULL) {
            transport_destroy(g_manager.transports[i].handle);
        }
    }
    g_manager.transport_count = 0;
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    /* 销毁互斥锁 */
    pthread_mutex_destroy(&g_manager.transports_mutex);
    pthread_mutex_destroy(&g_manager.endpoints_mutex);
    pthread_mutex_destroy(&g_manager.routes_mutex);
    pthread_mutex_destroy(&g_manager.stats_mutex);
    
    g_manager.initialized = 0;
    
    return ETH_OK;
}

eth_status_t tm_register_transport(transport_type_t type,
                                    const void *config,
                                    uint32_t *transport_id)
{
    if (!g_manager.initialized || config == NULL || transport_id == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (g_manager.transport_count >= TM_MAX_TRANSPORTS) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_ERROR;
    }
    
    uint32_t id = g_manager.transport_count;
    tm_transport_entry_t *entry = &g_manager.transports[id];
    
    entry->id = id;
    entry->type = type;
    entry->state = TM_STATE_INIT;
    
    /* 创建传输层实例 */
    transport_handle_t *handle = NULL;
    
    switch (type) {
        case TRANSPORT_UDP: {
            const udp_transport_config_t *udp_config = (const udp_transport_config_t *)config;
            handle = udp_transport_create(udp_config);
            entry->config = malloc(sizeof(udp_transport_config_t));
            if (entry->config) {
                memcpy(entry->config, udp_config, sizeof(udp_transport_config_t));
            }
            break;
        }
        case TRANSPORT_SHM: {
            const shm_transport_config_t *shm_config = (const shm_transport_config_t *)config;
            handle = shm_transport_create(shm_config);
            entry->config = malloc(sizeof(shm_transport_config_t));
            if (entry->config) {
                memcpy(entry->config, shm_config, sizeof(shm_transport_config_t));
            }
            break;
        }
        default:
            pthread_mutex_unlock(&g_manager.transports_mutex);
            return ETH_INVALID_PARAM;
    }
    
    if (handle == NULL) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_ERROR;
    }
    
    entry->handle = handle;
    entry->state = TM_STATE_ACTIVE;
    entry->priority = 0;
    entry->latency_us = 0;
    entry->throughput_mbps = 0;
    entry->last_used = get_time_us();
    entry->error_count = 0;
    
    g_manager.transport_count++;
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    *transport_id = id;
    
    /* 更新统计 */
    pthread_mutex_lock(&g_manager.stats_mutex);
    g_manager.stats.active_transports = g_manager.transport_count;
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    return ETH_OK;
}

eth_status_t tm_unregister_transport(uint32_t transport_id)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (transport_id >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_INVALID_PARAM;
    }
    
    tm_transport_entry_t *entry = &g_manager.transports[transport_id];
    
    entry->state = TM_STATE_SHUTDOWN;
    
    if (entry->handle != NULL) {
        if (entry->type == TRANSPORT_UDP) {
            udp_transport_destroy(entry->handle);
        } else if (entry->type == TRANSPORT_SHM) {
            shm_transport_destroy(entry->handle);
        }
        entry->handle = NULL;
    }
    
    if (entry->config != NULL) {
        free(entry->config);
        entry->config = NULL;
    }
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    return ETH_OK;
}

eth_status_t tm_register_endpoint(const tm_endpoint_entry_t *endpoint,
                                   uint32_t *endpoint_id)
{
    if (!g_manager.initialized || endpoint == NULL || endpoint_id == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.endpoints_mutex);
    
    if (g_manager.endpoint_count >= TM_MAX_ENDPOINTS) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        return ETH_ERROR;
    }
    
    uint32_t id = g_manager.endpoint_count;
    memcpy(&g_manager.endpoints[id], endpoint, sizeof(tm_endpoint_entry_t));
    g_manager.endpoints[id].endpoint_id = id;
    g_manager.endpoints[id].last_seen = get_time_us();
    g_manager.endpoint_count++;
    
    pthread_mutex_unlock(&g_manager.endpoints_mutex);
    
    *endpoint_id = id;
    
    pthread_mutex_lock(&g_manager.stats_mutex);
    g_manager.stats.active_endpoints = g_manager.endpoint_count;
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    return ETH_OK;
}

eth_status_t tm_unregister_endpoint(uint32_t endpoint_id)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.endpoints_mutex);
    
    if (endpoint_id >= g_manager.endpoint_count) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        return ETH_INVALID_PARAM;
    }
    
    g_manager.endpoints[endpoint_id].type = TM_EP_UNKNOWN;
    
    pthread_mutex_unlock(&g_manager.endpoints_mutex);
    
    return ETH_OK;
}

eth_status_t tm_create_route(uint32_t src_endpoint,
                              uint32_t dst_endpoint,
                              tm_routing_policy_t policy,
                              uint32_t *route_id)
{
    if (!g_manager.initialized || route_id == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.routes_mutex);
    pthread_mutex_lock(&g_manager.endpoints_mutex);
    
    if (g_manager.route_count >= TM_MAX_ROUTES) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_ERROR;
    }
    
    if (src_endpoint >= g_manager.endpoint_count ||
        dst_endpoint >= g_manager.endpoint_count) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_INVALID_PARAM;
    }
    
    tm_endpoint_entry_t *src = &g_manager.endpoints[src_endpoint];
    tm_endpoint_entry_t *dst = &g_manager.endpoints[dst_endpoint];
    
    uint32_t id = g_manager.route_count;
    tm_route_entry_t *route = &g_manager.routes[id];
    
    route->route_id = id;
    route->src_endpoint = src_endpoint;
    route->dst_endpoint = dst_endpoint;
    route->preferred_type = (policy == TM_ROUTE_PREFER_SHM) ? TRANSPORT_SHM : TRANSPORT_UDP;
    route->enable_failover = (policy == TM_ROUTE_REDUNDANT);
    
    /* 选择传输层 */
    uint32_t transport_id;
    if (select_transport_for_route(&g_manager, src, dst, policy, &transport_id) == 0) {
        route->primary_transport = transport_id;
        route->backup_transport = 0xFFFFFFFF;
    } else {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_ERROR;
    }
    
    g_manager.route_count++;
    
    pthread_mutex_unlock(&g_manager.endpoints_mutex);
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    *route_id = id;
    
    pthread_mutex_lock(&g_manager.stats_mutex);
    g_manager.stats.active_routes = g_manager.route_count;
    g_manager.stats.routing_decisions++;
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    return ETH_OK;
}

eth_status_t tm_delete_route(uint32_t route_id)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.routes_mutex);
    
    if (route_id >= g_manager.route_count) {
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_INVALID_PARAM;
    }
    
    g_manager.routes[route_id].src_endpoint = 0xFFFFFFFF;
    g_manager.routes[route_id].dst_endpoint = 0xFFFFFFFF;
    
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    return ETH_OK;
}

eth_status_t tm_send(uint32_t src_endpoint,
                      uint32_t dst_endpoint,
                      const uint8_t *data,
                      uint32_t len,
                      uint32_t timeout_ms)
{
    if (!g_manager.initialized || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    /* 查找路由 */
    pthread_mutex_lock(&g_manager.routes_mutex);
    
    tm_route_entry_t *route = NULL;
    for (uint32_t i = 0; i < g_manager.route_count; i++) {
        if (g_manager.routes[i].src_endpoint == src_endpoint &&
            g_manager.routes[i].dst_endpoint == dst_endpoint) {
            route = &g_manager.routes[i];
            break;
        }
    }
    
    if (route == NULL) {
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_ERROR;
    }
    
    uint32_t transport_id = route->primary_transport;
    
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    /* 获取传输层 */
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (transport_id >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_ERROR;
    }
    
    tm_transport_entry_t *transport = &g_manager.transports[transport_id];
    transport_handle_t *handle = transport->handle;
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    if (handle == NULL) {
        return ETH_ERROR;
    }
    
    /* 发送数据 */
    eth_status_t status;
    
    switch (transport->type) {
        case TRANSPORT_UDP:
            status = udp_transport_send_priority(handle, data, len, 0, timeout_ms);
            break;
        case TRANSPORT_SHM:
            status = shm_transport_send_zero_copy(handle, (void *)data, len, 
                                                   SHM_PRIO_MEDIUM, timeout_ms);
            break;
        default:
            status = ETH_ERROR;
    }
    
    /* 更新统计 */
    if (status == ETH_OK) {
        pthread_mutex_lock(&g_manager.stats_mutex);
        g_manager.stats.total_tx_messages++;
        g_manager.stats.total_tx_bytes += len;
        pthread_mutex_unlock(&g_manager.stats_mutex);
        
        pthread_mutex_lock(&g_manager.transports_mutex);
        transport->last_used = get_time_us();
        pthread_mutex_unlock(&g_manager.transports_mutex);
    } else {
        pthread_mutex_lock(&g_manager.transports_mutex);
        transport->error_count++;
        pthread_mutex_unlock(&g_manager.transports_mutex);
    }
    
    return status;
}

eth_status_t tm_receive(uint32_t endpoint_id,
                         uint8_t *buffer,
                         uint32_t max_len,
                         uint32_t *received_len,
                         uint32_t timeout_ms)
{
    if (!g_manager.initialized || buffer == NULL || received_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 查找关联的传输层 */
    pthread_mutex_lock(&g_manager.routes_mutex);
    
    tm_route_entry_t *route = NULL;
    for (uint32_t i = 0; i < g_manager.route_count; i++) {
        if (g_manager.routes[i].dst_endpoint == endpoint_id) {
            route = &g_manager.routes[i];
            break;
        }
    }
    
    if (route == NULL) {
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_ERROR;
    }
    
    uint32_t transport_id = route->primary_transport;
    
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    /* 获取传输层 */
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (transport_id >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_ERROR;
    }
    
    tm_transport_entry_t *transport = &g_manager.transports[transport_id];
    transport_handle_t *handle = transport->handle;
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    if (handle == NULL) {
        return ETH_ERROR;
    }
    
    /* 接收数据 */
    eth_status_t status = transport_receive(handle, buffer, max_len, received_len, timeout_ms);
    
    if (status == ETH_OK) {
        pthread_mutex_lock(&g_manager.stats_mutex);
        g_manager.stats.total_rx_messages++;
        g_manager.stats.total_rx_bytes += *received_len;
        pthread_mutex_unlock(&g_manager.stats_mutex);
    }
    
    return status;
}

eth_status_t tm_get_recommended_transport(uint32_t src_endpoint,
                                           uint32_t dst_endpoint,
                                           transport_type_t *recommended_type)
{
    if (!g_manager.initialized || recommended_type == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.endpoints_mutex);
    
    if (src_endpoint >= g_manager.endpoint_count ||
        dst_endpoint >= g_manager.endpoint_count) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        return ETH_INVALID_PARAM;
    }
    
    tm_endpoint_entry_t *src = &g_manager.endpoints[src_endpoint];
    tm_endpoint_entry_t *dst = &g_manager.endpoints[dst_endpoint];
    
    if (is_local_endpoint(src) && is_local_endpoint(dst)) {
        *recommended_type = TRANSPORT_SHM;
    } else {
        *recommended_type = TRANSPORT_UDP;
    }
    
    pthread_mutex_unlock(&g_manager.endpoints_mutex);
    
    return ETH_OK;
}

eth_status_t tm_set_routing_policy(tm_routing_policy_t policy)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    g_manager.config.routing_policy = policy;
    
    return ETH_OK;
}

eth_status_t tm_get_transport_status(uint32_t transport_id,
                                      tm_transport_state_t *state)
{
    if (!g_manager.initialized || state == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (transport_id >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        return ETH_INVALID_PARAM;
    }
    
    *state = g_manager.transports[transport_id].state;
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    return ETH_OK;
}

eth_status_t tm_health_check(void)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    uint64_t now = get_time_us();
    
    for (uint32_t i = 0; i < g_manager.transport_count; i++) {
        tm_transport_entry_t *transport = &g_manager.transports[i];
        
        if (transport->state == TM_STATE_ACTIVE) {
            uint32_t status;
            if (transport_get_status(transport->handle, &status) != ETH_OK) {
                transport->state = TM_STATE_DEGRADED;
            }
        }
    }
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    
    return ETH_OK;
}

eth_status_t tm_force_transport_switch(uint32_t route_id,
                                        uint32_t new_transport_id)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.routes_mutex);
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (route_id >= g_manager.route_count ||
        new_transport_id >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_INVALID_PARAM;
    }
    
    tm_route_entry_t *route = &g_manager.routes[route_id];
    route->backup_transport = route->primary_transport;
    route->primary_transport = new_transport_id;
    
    pthread_mutex_lock(&g_manager.stats_mutex);
    g_manager.stats.route_failovers++;
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    return ETH_OK;
}

eth_status_t tm_get_stats(tm_stats_t *stats)
{
    if (!g_manager.initialized || stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.stats_mutex);
    memcpy(stats, &g_manager.stats, sizeof(tm_stats_t));
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    return ETH_OK;
}

eth_status_t tm_clear_stats(void)
{
    if (!g_manager.initialized) {
        return ETH_NOT_INIT;
    }
    
    pthread_mutex_lock(&g_manager.stats_mutex);
    memset(&g_manager.stats, 0, sizeof(tm_stats_t));
    pthread_mutex_unlock(&g_manager.stats_mutex);
    
    return ETH_OK;
}

eth_status_t tm_set_node_info(const tm_node_info_t *node_info)
{
    if (!g_manager.initialized || node_info == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_manager.config.node_info, node_info, sizeof(tm_node_info_t));
    
    return ETH_OK;
}

eth_status_t tm_update_endpoint(uint32_t endpoint_id,
                                 const tm_endpoint_entry_t *endpoint)
{
    if (!g_manager.initialized || endpoint == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.endpoints_mutex);
    
    if (endpoint_id >= g_manager.endpoint_count) {
        pthread_mutex_unlock(&g_manager.endpoints_mutex);
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_manager.endpoints[endpoint_id], endpoint, sizeof(tm_endpoint_entry_t));
    g_manager.endpoints[endpoint_id].endpoint_id = endpoint_id;
    g_manager.endpoints[endpoint_id].last_seen = get_time_us();
    
    pthread_mutex_unlock(&g_manager.endpoints_mutex);
    
    return ETH_OK;
}

eth_status_t tm_get_route_latency(uint32_t route_id,
                                   uint32_t *avg_latency_us,
                                   uint32_t *max_latency_us)
{
    if (!g_manager.initialized || avg_latency_us == NULL || max_latency_us == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_manager.routes_mutex);
    pthread_mutex_lock(&g_manager.transports_mutex);
    
    if (route_id >= g_manager.route_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_INVALID_PARAM;
    }
    
    tm_route_entry_t *route = &g_manager.routes[route_id];
    
    if (route->primary_transport >= g_manager.transport_count) {
        pthread_mutex_unlock(&g_manager.transports_mutex);
        pthread_mutex_unlock(&g_manager.routes_mutex);
        return ETH_ERROR;
    }
    
    tm_transport_entry_t *transport = &g_manager.transports[route->primary_transport];
    *avg_latency_us = transport->latency_us;
    *max_latency_us = transport->latency_us * 2; /* 估计值 */
    
    pthread_mutex_unlock(&g_manager.transports_mutex);
    pthread_mutex_unlock(&g_manager.routes_mutex);
    
    return ETH_OK;
}
