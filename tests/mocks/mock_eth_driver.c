/**
 * @file mock_eth_driver.c
 * @brief 以太网驱动模拟实现 - 用于单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "mock_eth_driver.h"
#include "../../src/common/types/eth_types.h"

/*==============================================================================
 * 内部状态和数据结构
 *==============================================================================*/

typedef struct mock_packet {
    uint8_t data[MOCK_ETH_MAX_PACKET_SIZE];
    uint32_t length;
    eth_mac_addr_t src_addr;
    eth_mac_addr_t dst_addr;
    uint16_t vlan_tag;
    bool has_vlan;
    struct mock_packet* next;
} mock_packet_t;

typedef struct mock_eth_port {
    uint32_t port_id;
    bool initialized;
    bool link_up;
    eth_mode_t mode;
    eth_mac_addr_t mac_addr;
    eth_config_t config;
    eth_rx_callback_t rx_callback;
    void* rx_user_data;
    eth_link_callback_t link_callback;
    void* link_user_data;
    
    /* TX/RX队列模拟 */
    mock_packet_t* tx_queue_head;
    mock_packet_t* tx_queue_tail;
    uint32_t tx_queue_count;
    mock_packet_t* rx_queue_head;
    mock_packet_t* rx_queue_tail;
    uint32_t rx_queue_count;
    
    /* 统计信息 */
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t tx_errors;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t rx_errors;
    uint64_t rx_dropped;
    
    pthread_mutex_t mutex;
    pthread_t rx_thread;
    bool rx_thread_running;
    
    struct mock_eth_port* next;
} mock_eth_port_t;

static struct {
    mock_eth_port_t* ports;
    uint32_t port_count;
    pthread_mutex_t global_mutex;
    bool initialized;
    uint32_t next_port_id;
    
    /* 模拟网络环境 */
    uint32_t latency_min_us;
    uint32_t latency_max_us;
    uint32_t packet_loss_rate;  /* 0-10000 (0.01% 步进) */
    uint32_t error_rate;        /* 0-10000 (0.01% 步进) */
    bool enable_vlan_support;
} mock_eth_state = {
    .latency_min_us = 10,
    .latency_max_us = 100,
    .packet_loss_rate = 0,
    .error_rate = 0,
    .enable_vlan_support = true
};

/*==============================================================================
 * 内部辅助函数
 *==============================================================================*/

static void mock_eth_init(void) {
    if (!mock_eth_state.initialized) {
        pthread_mutex_init(&mock_eth_state.global_mutex, NULL);
        mock_eth_state.ports = NULL;
        mock_eth_state.port_count = 0;
        mock_eth_state.next_port_id = 1;
        mock_eth_state.initialized = true;
    }
}

static mock_eth_port_t* find_port(uint32_t port_id) {
    mock_eth_port_t* port = mock_eth_state.ports;
    while (port != NULL) {
        if (port->port_id == port_id) {
            return port;
        }
        port = port->next;
    }
    return NULL;
}

static void enqueue_packet(mock_packet_t** head, mock_packet_t** tail, 
                          uint32_t* count, mock_packet_t* packet) {
    packet->next = NULL;
    if (*tail == NULL) {
        *head = *tail = packet;
    } else {
        (*tail)->next = packet;
        *tail = packet;
    }
    (*count)++;
}

static mock_packet_t* dequeue_packet(mock_packet_t** head, mock_packet_t** tail,
                                     uint32_t* count) {
    if (*head == NULL) return NULL;
    mock_packet_t* packet = *head;
    *head = packet->next;
    if (*head == NULL) {
        *tail = NULL;
    }
    (*count)--;
    return packet;
}

static void simulate_latency(void) {
    uint32_t latency = mock_eth_state.latency_min_us + 
                       (rand() % (mock_eth_state.latency_max_us - 
                                  mock_eth_state.latency_min_us + 1));
    usleep(latency);
}

static bool should_drop_packet(void) {
    return (rand() % 10000) < mock_eth_state.packet_loss_rate;
}

static bool should_inject_error(void) {
    return (rand() % 10000) < mock_eth_state.error_rate;
}

static void* rx_thread_func(void* arg) {
    mock_eth_port_t* port = (mock_eth_port_t*)arg;
    
    while (port->rx_thread_running) {
        pthread_mutex_lock(&port->mutex);
        
        mock_packet_t* packet = dequeue_packet(&port->rx_queue_head,
                                               &port->rx_queue_tail,
                                               &port->rx_queue_count);
        
        if (packet != NULL && port->rx_callback != NULL) {
            pthread_mutex_unlock(&port->mutex);
            
            simulate_latency();
            
            eth_buffer_t buffer;
            buffer.data = packet->data;
            buffer.len = packet->length;
            buffer.max_len = packet->length;
            
            port->rx_callback(&buffer, port->rx_user_data);
            
            pthread_mutex_lock(&port->mutex);
            port->rx_packets++;
            port->rx_bytes += packet->length;
            pthread_mutex_unlock(&port->mutex);
            
            free(packet);
        } else {
            pthread_mutex_unlock(&port->mutex);
            usleep(100);  /* 100us休眠 */
        }
    }
    
    return NULL;
}

/*==============================================================================
 * 驱动API实现
 *==============================================================================*/

mock_eth_handle_t mock_eth_driver_init(const eth_config_t* config) {
    mock_eth_init();
    
    pthread_mutex_lock(&mock_eth_state.global_mutex);
    
    mock_eth_port_t* port = (mock_eth_port_t*)malloc(sizeof(mock_eth_port_t));
    if (port == NULL) {
        pthread_mutex_unlock(&mock_eth_state.global_mutex);
        return NULL;
    }
    
    memset(port, 0, sizeof(mock_eth_port_t));
    port->port_id = mock_eth_state.next_port_id++;
    memcpy(&port->config, config, sizeof(eth_config_t));
    memcpy(port->mac_addr, config->mac_addr, sizeof(eth_mac_addr_t));
    port->mode = config->mode;
    port->initialized = true;
    port->link_up = false;
    pthread_mutex_init(&port->mutex, NULL);
    
    /* 加入全局列表 */
    port->next = mock_eth_state.ports;
    mock_eth_state.ports = port;
    mock_eth_state.port_count++;
    
    pthread_mutex_unlock(&mock_eth_state.global_mutex);
    
    printf("[MockEth] Port %u initialized with MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
           port->port_id,
           port->mac_addr[0], port->mac_addr[1], port->mac_addr[2],
           port->mac_addr[3], port->mac_addr[4], port->mac_addr[5]);
    
    return port;
}

eth_status_t mock_eth_driver_deinit(mock_eth_handle_t handle) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    mock_eth_driver_stop(handle);
    
    pthread_mutex_lock(&mock_eth_state.global_mutex);
    
    /* 从列表中移除 */
    mock_eth_port_t** current = &mock_eth_state.ports;
    while (*current != NULL) {
        if (*current == port) {
            *current = port->next;
            break;
        }
        current = &(*current)->next;
    }
    mock_eth_state.port_count--;
    
    pthread_mutex_unlock(&mock_eth_state.global_mutex);
    
    pthread_mutex_destroy(&port->mutex);
    free(port);
    
    return ETH_OK;
}

eth_status_t mock_eth_driver_start(mock_eth_handle_t handle) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    
    if (!port->rx_thread_running) {
        port->rx_thread_running = true;
        pthread_create(&port->rx_thread, NULL, rx_thread_func, port);
    }
    
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

eth_status_t mock_eth_driver_stop(mock_eth_handle_t handle) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    
    if (port->rx_thread_running) {
        port->rx_thread_running = false;
        pthread_mutex_unlock(&port->mutex);
        pthread_join(port->rx_thread, NULL);
    } else {
        pthread_mutex_unlock(&port->mutex);
    }
    
    return ETH_OK;
}

eth_status_t mock_eth_send_packet(mock_eth_handle_t handle,
                                   const eth_mac_addr_t dst_addr,
                                   const uint8_t* data,
                                   uint32_t length) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    if (length > MOCK_ETH_MAX_PACKET_SIZE) {
        return ETH_ERROR;
    }
    if (!port->link_up) {
        return ETH_ERROR;
    }
    
    /* 检查是否丢包 */
    if (should_drop_packet()) {
        pthread_mutex_lock(&port->mutex);
        port->tx_errors++;
        pthread_mutex_unlock(&port->mutex);
        return ETH_ERROR;
    }
    
    simulate_latency();
    
    mock_packet_t* packet = (mock_packet_t*)malloc(sizeof(mock_packet_t));
    if (packet == NULL) {
        return ETH_NO_MEMORY;
    }
    
    memcpy(packet->data, data, length);
    packet->length = length;
    memcpy(packet->src_addr, port->mac_addr, sizeof(eth_mac_addr_t));
    memcpy(packet->dst_addr, dst_addr, sizeof(eth_mac_addr_t));
    packet->has_vlan = false;
    
    pthread_mutex_lock(&port->mutex);
    enqueue_packet(&port->tx_queue_head, &port->tx_queue_tail,
                   &port->tx_queue_count, packet);
    port->tx_packets++;
    port->tx_bytes += length;
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

eth_status_t mock_eth_send_vlan_packet(mock_eth_handle_t handle,
                                        const eth_mac_addr_t dst_addr,
                                        uint16_t vlan_id,
                                        uint8_t priority,
                                        const uint8_t* data,
                                        uint32_t length) {
    if (!mock_eth_state.enable_vlan_support) {
        return ETH_ERROR;
    }
    
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    if (length + 4 > MOCK_ETH_MAX_PACKET_SIZE) {
        return ETH_ERROR;
    }
    
    /* 构建VLAN标签数据 */
    uint8_t vlan_data[MOCK_ETH_MAX_PACKET_SIZE];
    vlan_data[0] = 0x81;
    vlan_data[1] = 0x00;
    vlan_data[2] = (priority << 5) | ((vlan_id >> 8) & 0x0F);
    vlan_data[3] = vlan_id & 0xFF;
    memcpy(&vlan_data[4], data, length);
    
    return mock_eth_send_packet(handle, dst_addr, vlan_data, length + 4);
}

eth_status_t mock_eth_receive_packet(mock_eth_handle_t handle,
                                      uint8_t* buffer,
                                      uint32_t buffer_size,
                                      uint32_t* received_length,
                                      eth_mac_addr_t src_addr) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    
    mock_packet_t* packet = dequeue_packet(&port->rx_queue_head,
                                           &port->rx_queue_tail,
                                           &port->rx_queue_count);
    
    if (packet == NULL) {
        pthread_mutex_unlock(&port->mutex);
        return ETH_TIMEOUT;
    }
    
    if (packet->length > buffer_size) {
        /* 数据被截断 */
        memcpy(buffer, packet->data, buffer_size);
        *received_length = buffer_size;
    } else {
        memcpy(buffer, packet->data, packet->length);
        *received_length = packet->length;
    }
    
    if (src_addr != NULL) {
        memcpy(src_addr, packet->src_addr, sizeof(eth_mac_addr_t));
    }
    
    pthread_mutex_unlock(&port->mutex);
    
    free(packet);
    return ETH_OK;
}

eth_status_t mock_eth_set_link_status(mock_eth_handle_t handle,
                                       eth_link_status_t status) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    bool new_link_up = (status == ETH_LINK_UP);
    if (port->link_up != new_link_up) {
        port->link_up = new_link_up;
        
        if (port->link_callback != NULL) {
            port->link_callback(status, port->link_user_data);
        }
    }
    
    return ETH_OK;
}

eth_status_t mock_eth_get_link_status(mock_eth_handle_t handle,
                                       eth_link_status_t* status) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized || status == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *status = port->link_up ? ETH_LINK_UP : ETH_LINK_DOWN;
    return ETH_OK;
}

eth_status_t mock_eth_register_rx_callback(mock_eth_handle_t handle,
                                            eth_rx_callback_t callback,
                                            void* user_data) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    port->rx_callback = callback;
    port->rx_user_data = user_data;
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

eth_status_t mock_eth_register_link_callback(mock_eth_handle_t handle,
                                              eth_link_callback_t callback,
                                              void* user_data) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    port->link_callback = callback;
    port->link_user_data = user_data;
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

/*==============================================================================
 * 模拟控制API
 *==============================================================================*/

void mock_eth_set_latency(uint32_t min_us, uint32_t max_us) {
    mock_eth_state.latency_min_us = min_us;
    mock_eth_state.latency_max_us = max_us;
}

void mock_eth_set_packet_loss(uint32_t rate_x10000) {
    if (rate_x10000 > 10000) rate_x10000 = 10000;
    mock_eth_state.packet_loss_rate = rate_x10000;
}

void mock_eth_set_error_rate(uint32_t rate_x10000) {
    if (rate_x10000 > 10000) rate_x10000 = 10000;
    mock_eth_state.error_rate = rate_x10000;
}

void mock_eth_enable_vlan(bool enable) {
    mock_eth_state.enable_vlan_support = enable;
}

eth_status_t mock_eth_inject_rx_packet(mock_eth_handle_t handle,
                                        const eth_mac_addr_t src_addr,
                                        const uint8_t* data,
                                        uint32_t length) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return ETH_INVALID_PARAM;
    }
    if (length > MOCK_ETH_MAX_PACKET_SIZE) {
        return ETH_ERROR;
    }
    
    mock_packet_t* packet = (mock_packet_t*)malloc(sizeof(mock_packet_t));
    if (packet == NULL) {
        return ETH_NO_MEMORY;
    }
    
    memcpy(packet->data, data, length);
    packet->length = length;
    memcpy(packet->src_addr, src_addr, sizeof(eth_mac_addr_t));
    memcpy(packet->dst_addr, port->mac_addr, sizeof(eth_mac_addr_t));
    packet->has_vlan = false;
    
    pthread_mutex_lock(&port->mutex);
    enqueue_packet(&port->rx_queue_head, &port->rx_queue_tail,
                   &port->rx_queue_count, packet);
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

/*==============================================================================
 * 统计API
 *==============================================================================*/

eth_status_t mock_eth_get_stats(mock_eth_handle_t handle,
                                 mock_eth_stats_t* stats) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized || stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&port->mutex);
    stats->tx_packets = port->tx_packets;
    stats->tx_bytes = port->tx_bytes;
    stats->tx_errors = port->tx_errors;
    stats->rx_packets = port->rx_packets;
    stats->rx_bytes = port->rx_bytes;
    stats->rx_errors = port->rx_errors;
    stats->rx_dropped = port->rx_dropped;
    stats->tx_queue_count = port->tx_queue_count;
    stats->rx_queue_count = port->rx_queue_count;
    pthread_mutex_unlock(&port->mutex);
    
    return ETH_OK;
}

void mock_eth_reset_stats(mock_eth_handle_t handle) {
    mock_eth_port_t* port = (mock_eth_port_t*)handle;
    if (port == NULL || !port->initialized) {
        return;
    }
    
    pthread_mutex_lock(&port->mutex);
    port->tx_packets = 0;
    port->tx_bytes = 0;
    port->tx_errors = 0;
    port->rx_packets = 0;
    port->rx_bytes = 0;
    port->rx_errors = 0;
    port->rx_dropped = 0;
    pthread_mutex_unlock(&port->mutex);
}

/*==============================================================================
 * 模拟网络API (端口之间转发)
 *==============================================================================*/

eth_status_t mock_eth_forward_between_ports(uint32_t src_port_id,
                                             uint32_t dst_port_id,
                                             const uint8_t* data,
                                             uint32_t length) {
    pthread_mutex_lock(&mock_eth_state.global_mutex);
    
    mock_eth_port_t* src_port = find_port(src_port_id);
    mock_eth_port_t* dst_port = find_port(dst_port_id);
    
    if (src_port == NULL || dst_port == NULL) {
        pthread_mutex_unlock(&mock_eth_state.global_mutex);
        return ETH_INVALID_PARAM;
    }
    
    pthread_mutex_unlock(&mock_eth_state.global_mutex);
    
    if (should_drop_packet()) {
        return ETH_ERROR;
    }
    
    simulate_latency();
    
    mock_packet_t* packet = (mock_packet_t*)malloc(sizeof(mock_packet_t));
    if (packet == NULL) {
        return ETH_NO_MEMORY;
    }
    
    memcpy(packet->data, data, length);
    packet->length = length;
    memcpy(packet->src_addr, src_port->mac_addr, sizeof(eth_mac_addr_t));
    memcpy(packet->dst_addr, dst_port->mac_addr, sizeof(eth_mac_addr_t));
    packet->has_vlan = false;
    
    pthread_mutex_lock(&dst_port->mutex);
    enqueue_packet(&dst_port->rx_queue_head, &dst_port->rx_queue_tail,
                   &dst_port->rx_queue_count, packet);
    pthread_mutex_unlock(&dst_port->mutex);
    
    return ETH_OK;
}

void mock_eth_reset_all(void) {
    pthread_mutex_lock(&mock_eth_state.global_mutex);
    
    mock_eth_port_t* port = mock_eth_state.ports;
    while (port != NULL) {
        mock_eth_port_t* next = port->next;
        mock_eth_driver_deinit(port);
        port = next;
    }
    
    mock_eth_state.ports = NULL;
    mock_eth_state.port_count = 0;
    mock_eth_state.next_port_id = 1;
    
    pthread_mutex_unlock(&mock_eth_state.global_mutex);
}

uint32_t mock_eth_get_port_count(void) {
    return mock_eth_state.port_count;
}
