/**
 * @file dds_eth_transport.c
 * @brief DDS RTPS over ETH传输层实现 - DDS-RTPS 2.5 compliant
 * @version 1.0
 * @date 2026-04-26
 */

#include <string.h>
#include <stdio.h>
#include "dds_eth_transport.h"
#include "../../../tcpip/tcpip_socket.h"
#include "../../../tcpip/tcpip_udp.h"
#include "../../../ethernet/eth_manager.h"

/* ============================================================================
 * 私有定义
 * ============================================================================ */

#define DDS_ETH_MAGIC_NUMBER         0x44445345U  /* "DDSE" */
#define DDS_ETH_VERSION_MAJOR        1
#define DDS_ETH_VERSION_MINOR        0

/** 编译工具宏 */
#if defined(__GNUC__) || defined(__clang__)
    #define DDS_PACKED __attribute__((packed))
#else
    #define DDS_PACKED
#endif

/* ============================================================================
 * 静态数据
 * ============================================================================ */

typedef struct {
    bool initialized;
    bool running;
    uint32_t magic;
    dds_eth_transport_config_t config;
    
    /* Socket管理 */
    tcpip_socket_id_t unicast_socket;
    tcpip_socket_id_t multicast_socket;
    
    /* Endpoint表 */
    dds_endpoint_entry_t endpoints[DDS_ETH_MAX_ENDPOINTS];
    uint32_t endpoint_count;
    
    /* Participant表 */
    dds_participant_entry_t participants[DDS_ETH_MAX_PARTICIPANTS];
    uint32_t participant_count;
    
    /* 回调函数 */
    dds_eth_data_rx_cb_t data_rx_callback;
    void *data_rx_user_data;
    dds_eth_discovery_cb_t discovery_callback;
    void *discovery_user_data;
    
    /* 统计 */
    dds_eth_transport_stats_t stats;
    
    /* 计时器 */
    uint32_t last_heartbeat_time;
    uint32_t last_discovery_time;
    
    /* GUID前缀 */
    uint8_t local_guid_prefix[12];
    
    /* 缓冲区 */
    uint8_t tx_buffer[DDS_ETH_TX_BUFFER_SIZE];
    uint8_t rx_buffer[DDS_ETH_RX_BUFFER_SIZE];
} dds_eth_transport_ctx_t;

static dds_eth_transport_ctx_t g_eth_transport = {0};

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static eth_status_t dds_eth_init_sockets(void);
static void dds_eth_deinit_sockets(void);
static eth_status_t dds_eth_process_unicast(void);
static eth_status_t dds_eth_process_multicast(void);
static eth_status_t dds_eth_handle_rtps_packet(const uint8_t *data, uint32_t len,
                                                const tcpip_sockaddr_t *src_addr);
static int32_t dds_eth_find_free_endpoint_slot(void);
static int32_t dds_eth_find_free_participant_slot(void);
static int32_t dds_eth_find_participant_by_guid(const dds_guid_t *guid);
static bool dds_eth_guid_equal(const dds_guid_t *a, const dds_guid_t *b);
static void dds_eth_generate_guid_prefix(uint8_t prefix[12], uint16_t domain_id);
static eth_status_t dds_eth_sendto_socket(tcpip_socket_id_t socket,
                                           const uint8_t *data, uint32_t len,
                                           const tcpip_sockaddr_t *dest);

/* ============================================================================
 * 初始化和反初始化API实现
 * ============================================================================ */

eth_status_t dds_eth_transport_init(const dds_eth_transport_config_t *config)
{
    eth_status_t status = ETH_OK;
    
    if (config == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (g_eth_transport.initialized) {
        status = ETH_BUSY;
    }
    else {
        /* 复制配置 */
        (void)memcpy(&g_eth_transport.config, config, sizeof(dds_eth_transport_config_t));
        
        /* 初始化状态 */
        g_eth_transport.initialized = false;
        g_eth_transport.running = false;
        g_eth_transport.magic = DDS_ETH_MAGIC_NUMBER;
        
        /* 清零Endpoint表 */
        (void)memset(g_eth_transport.endpoints, 0, sizeof(g_eth_transport.endpoints));
        g_eth_transport.endpoint_count = 0;
        
        /* 清零Participant表 */
        (void)memset(g_eth_transport.participants, 0, sizeof(g_eth_transport.participants));
        g_eth_transport.participant_count = 0;
        
        /* 清零回调 */
        g_eth_transport.data_rx_callback = NULL;
        g_eth_transport.data_rx_user_data = NULL;
        g_eth_transport.discovery_callback = NULL;
        g_eth_transport.discovery_user_data = NULL;
        
        /* 清零统计 */
        (void)memset(&g_eth_transport.stats, 0, sizeof(g_eth_transport.stats));
        
        /* 生成本地GUID前缀 */
        dds_eth_generate_guid_prefix(g_eth_transport.local_guid_prefix, 
                                      config->domain_id);
        
        /* 初始化套接字 */
        status = dds_eth_init_sockets();
        
        if (status == ETH_OK) {
            g_eth_transport.initialized = true;
        }
    }
    
    return status;
}

void dds_eth_transport_deinit(void)
{
    if (g_eth_transport.initialized) {
        /* 停止运行 */
        (void)dds_eth_transport_stop();
        
        /* 关闭套接字 */
        dds_eth_deinit_sockets();
        
        /* 清零数据 */
        (void)memset(&g_eth_transport, 0, sizeof(g_eth_transport));
    }
}

eth_status_t dds_eth_transport_start(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (g_eth_transport.running) {
        status = ETH_BUSY;
    }
    else {
        /* 绑定单播套接字 */
        tcpip_sockaddr_t local_addr;
        tcpip_error_t err;
        
        (void)memset(&local_addr, 0, sizeof(local_addr));
        local_addr.addr = g_eth_transport.config.local_ip;
        local_addr.port = g_eth_transport.config.base_port + 
                         g_eth_transport.config.port_offset;
        
        err = tcpip_socket_bind(g_eth_transport.unicast_socket, &local_addr);
        if (err != TCPIP_OK) {
            status = ETH_ERROR;
        }
        
        /* 加入多播组 */
        if ((status == ETH_OK) && g_eth_transport.config.enable_multicast) {
            err = tcpip_udp_join_multicast(g_eth_transport.multicast_socket,
                                           g_eth_transport.config.multicast_addr);
            if (err != TCPIP_OK) {
                status = ETH_ERROR;
            }
            
            /* 绑定多播端口 */
            if (status == ETH_OK) {
                tcpip_sockaddr_t mc_addr;
                (void)memset(&mc_addr, 0, sizeof(mc_addr));
                mc_addr.addr = g_eth_transport.config.multicast_addr;
                mc_addr.port = g_eth_transport.config.base_port + 
                              DDS_ETH_METATRAFFIC_MULTICAST_PORT_OFFSET;
                
                err = tcpip_socket_bind(g_eth_transport.multicast_socket, &mc_addr);
                if (err != TCPIP_OK) {
                    status = ETH_ERROR;
                }
            }
        }
        
        if (status == ETH_OK) {
            g_eth_transport.running = true;
        }
    }
    
    return status;
}

eth_status_t dds_eth_transport_stop(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (!g_eth_transport.running) {
        /* 已停止，直接返回成功 */
        status = ETH_OK;
    }
    else {
        /* 离开多播组 */
        if (g_eth_transport.config.enable_multicast) {
            (void)tcpip_udp_leave_multicast(g_eth_transport.multicast_socket,
                                            g_eth_transport.config.multicast_addr);
        }
        
        /* 关闭套接字 */
        (void)tcpip_socket_close(g_eth_transport.unicast_socket);
        (void)tcpip_socket_close(g_eth_transport.multicast_socket);
        
        g_eth_transport.running = false;
    }
    
    return status;
}

/* ============================================================================
 * 回调注册API实现
 * ============================================================================ */

eth_status_t dds_eth_register_data_callback(dds_eth_data_rx_cb_t callback, 
                                             void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_eth_transport.data_rx_callback = callback;
        g_eth_transport.data_rx_user_data = user_data;
    }
    
    return status;
}

eth_status_t dds_eth_register_discovery_callback(dds_eth_discovery_cb_t callback,
                                                  void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_eth_transport.discovery_callback = callback;
        g_eth_transport.discovery_user_data = user_data;
    }
    
    return status;
}

/* ============================================================================
 * 数据发送API实现
 * ============================================================================ */

eth_status_t dds_eth_send_rtps(const uint8_t *data, uint32_t len,
                                const dds_endpoint_locator_t *locator,
                                bool reliable)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    tcpip_sockaddr_t dest_addr;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((data == NULL) || (locator == NULL) || (len == 0U)) {
        status = ETH_INVALID_PARAM;
    }
    else if (len > DDS_ETH_MAX_RTPS_PACKET_SIZE) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 构建目标地址 */
        (void)memset(&dest_addr, 0, sizeof(dest_addr));
        
        if (locator->is_multicast) {
            dest_addr.addr = locator->multicast_addr;
        }
        else {
            dest_addr.addr = locator->ip_addr;
        }
        dest_addr.port = locator->port;
        
        /* 发送数据 */
        if (reliable) {
            /* 可靠传输 - 使用TCP */
            /* 注: 实际实现需要TCP socket */
            err = TCPIP_ERROR; /* 未实现 */
        }
        else {
            /* 最佳努力传输 - 使用UDP */
            err = tcpip_udp_sendto(g_eth_transport.unicast_socket, 
                                   &dest_addr, data, (uint16_t)len);
        }
        
        if (err == TCPIP_OK) {
            g_eth_transport.stats.tx_packets++;
            g_eth_transport.stats.tx_bytes += len;
        }
        else {
            g_eth_transport.stats.tx_errors++;
            status = ETH_ERROR;
        }
    }
    
    return status;
}

/* ============================================================================
 * 数据处理API实现
 * ============================================================================ */

eth_status_t dds_eth_transport_process(uint32_t timeout_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (!g_eth_transport.running) {
        status = ETH_ERROR;
    }
    else {
        /* 处理单播数据 */
        (void)dds_eth_process_unicast();
        
        /* 处理多播数据 */
        if (g_eth_transport.config.enable_multicast) {
            (void)dds_eth_process_multicast();
        }
        
        /* 检查Participant超时 */
        uint32_t current_time = 0; /* 实际实现需要获取当前时间 */
        for (uint32_t i = 0; i < DDS_ETH_MAX_PARTICIPANTS; i++) {
            if (g_eth_transport.participants[i].used) {
                if ((current_time - g_eth_transport.participants[i].info.last_seen_ms) >
                    g_eth_transport.participants[i].info.lease_duration_ms) {
                    /* Participant超时，移除 */
                    g_eth_transport.participants[i].used = false;
                    g_eth_transport.participant_count--;
                    
                    if (g_eth_transport.discovery_callback != NULL) {
                        g_eth_transport.discovery_callback(
                            &g_eth_transport.participants[i].info,
                            false, /* left */
                            g_eth_transport.discovery_user_data);
                    }
                }
            }
        }
        
        (void)timeout_ms; /* 参数暂未使用 */
    }
    
    return status;
}

/* ============================================================================
 * 统计API实现
 * ============================================================================ */

eth_status_t dds_eth_get_stats(dds_eth_transport_stats_t *stats)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stats == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)memcpy(stats, &g_eth_transport.stats, sizeof(dds_eth_transport_stats_t));
        stats->active_endpoints = g_eth_transport.endpoint_count;
        stats->active_participants = g_eth_transport.participant_count;
    }
    
    return status;
}

eth_status_t dds_eth_clear_stats(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        (void)memset(&g_eth_transport.stats, 0, sizeof(g_eth_transport.stats));
    }
    
    return status;
}

/* ============================================================================
 * Endpoint管理API实现
 * ============================================================================ */

eth_status_t dds_eth_register_endpoint(const dds_endpoint_config_t *config,
                                        uint32_t *endpoint_id)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((config == NULL) || (endpoint_id == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_eth_find_free_endpoint_slot();
        if (idx < 0) {
            status = ETH_NO_MEMORY;
        }
        else {
            (void)memcpy(&g_eth_transport.endpoints[idx].config, 
                        config, sizeof(dds_endpoint_config_t));
            g_eth_transport.endpoints[idx].used = true;
            g_eth_transport.endpoints[idx].last_activity_ms = 0;
            g_eth_transport.endpoints[idx].packet_count = 0;
            
            *endpoint_id = (uint32_t)idx;
            g_eth_transport.endpoint_count++;
        }
    }
    
    return status;
}

eth_status_t dds_eth_unregister_endpoint(uint32_t endpoint_id)
{
    eth_status_t status = ETH_OK;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (endpoint_id >= DDS_ETH_MAX_ENDPOINTS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_eth_transport.endpoints[endpoint_id].used) {
        status = ETH_INVALID_PARAM;
    }
    else {
        g_eth_transport.endpoints[endpoint_id].used = false;
        g_eth_transport.endpoint_count--;
    }
    
    return status;
}

eth_status_t dds_eth_find_endpoint(const dds_guid_t *guid,
                                    dds_endpoint_config_t *endpoint)
{
    eth_status_t status = ETH_ERROR;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((guid == NULL) || (endpoint == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        for (uint32_t i = 0; i < DDS_ETH_MAX_ENDPOINTS; i++) {
            if (g_eth_transport.endpoints[i].used) {
                if (dds_eth_guid_equal(&g_eth_transport.endpoints[i].config.guid, guid)) {
                    (void)memcpy(endpoint, &g_eth_transport.endpoints[i].config,
                                sizeof(dds_endpoint_config_t));
                    status = ETH_OK;
                    break;
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_eth_get_active_endpoints(dds_endpoint_config_t *endpoints,
                                           uint32_t max_count,
                                           uint32_t *actual_count)
{
    eth_status_t status = ETH_OK;
    uint32_t count = 0;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((endpoints == NULL) || (actual_count == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        for (uint32_t i = 0; (i < DDS_ETH_MAX_ENDPOINTS) && (count < max_count); i++) {
            if (g_eth_transport.endpoints[i].used) {
                (void)memcpy(&endpoints[count], 
                            &g_eth_transport.endpoints[i].config,
                            sizeof(dds_endpoint_config_t));
                count++;
            }
        }
        *actual_count = count;
    }
    
    return status;
}

/* ============================================================================
 * Participant管理API实现
 * ============================================================================ */

eth_status_t dds_eth_add_participant(const dds_participant_info_t *info)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (info == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 检查是否已存在 */
        idx = dds_eth_find_participant_by_guid(&info->guid);
        
        if (idx >= 0) {
            /* 更新现有Participant */
            (void)memcpy(&g_eth_transport.participants[idx].info, info,
                        sizeof(dds_participant_info_t));
            g_eth_transport.participants[idx].info.last_seen_ms = 0; /* 当前时间 */
        }
        else {
            /* 添加新Participant */
            idx = dds_eth_find_free_participant_slot();
            if (idx < 0) {
                status = ETH_NO_MEMORY;
            }
            else {
                (void)memcpy(&g_eth_transport.participants[idx].info, info,
                            sizeof(dds_participant_info_t));
                g_eth_transport.participants[idx].used = true;
                g_eth_transport.participants[idx].info.last_seen_ms = 0;
                g_eth_transport.participant_count++;
                
                if (g_eth_transport.discovery_callback != NULL) {
                    g_eth_transport.discovery_callback(
                        &g_eth_transport.participants[idx].info,
                        true, /* joined */
                        g_eth_transport.discovery_user_data);
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_eth_remove_participant(const dds_guid_t *guid)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (guid == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_eth_find_participant_by_guid(guid);
        if (idx < 0) {
            status = ETH_ERROR;
        }
        else {
            if (g_eth_transport.discovery_callback != NULL) {
                g_eth_transport.discovery_callback(
                    &g_eth_transport.participants[idx].info,
                    false, /* left */
                    g_eth_transport.discovery_user_data);
            }
            
            g_eth_transport.participants[idx].used = false;
            g_eth_transport.participant_count--;
        }
    }
    
    return status;
}

eth_status_t dds_eth_get_participant(const dds_guid_t *guid,
                                      dds_participant_info_t *info)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((guid == NULL) || (info == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_eth_find_participant_by_guid(guid);
        if (idx < 0) {
            status = ETH_ERROR;
        }
        else {
            (void)memcpy(info, &g_eth_transport.participants[idx].info,
                        sizeof(dds_participant_info_t));
        }
    }
    
    return status;
}

eth_status_t dds_eth_get_active_participants(dds_participant_info_t *participants,
                                              uint32_t max_count,
                                              uint32_t *actual_count)
{
    eth_status_t status = ETH_OK;
    uint32_t count = 0;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((participants == NULL) || (actual_count == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        for (uint32_t i = 0; (i < DDS_ETH_MAX_PARTICIPANTS) && (count < max_count); i++) {
            if (g_eth_transport.participants[i].used) {
                (void)memcpy(&participants[count],
                            &g_eth_transport.participants[i].info,
                            sizeof(dds_participant_info_t));
                count++;
            }
        }
        *actual_count = count;
    }
    
    return status;
}

eth_status_t dds_eth_refresh_participant(const dds_guid_t *guid)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (guid == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_eth_find_participant_by_guid(guid);
        if (idx < 0) {
            status = ETH_ERROR;
        }
        else {
            g_eth_transport.participants[idx].info.last_seen_ms = 0; /* 当前时间 */
        }
    }
    
    return status;
}

/* ============================================================================
 * 多播组管理API实现
 * ============================================================================ */

eth_status_t dds_eth_join_multicast_group(eth_ip_addr_t multicast_addr,
                                           const eth_mac_addr_t multicast_mac)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        err = tcpip_udp_join_multicast(g_eth_transport.multicast_socket,
                                       multicast_addr);
        if (err != TCPIP_OK) {
            status = ETH_ERROR;
        }
    }
    
    (void)multicast_mac; /* 参数暂未使用 */
    return status;
}

eth_status_t dds_eth_leave_multicast_group(eth_ip_addr_t multicast_addr)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    
    if (!g_eth_transport.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        err = tcpip_udp_leave_multicast(g_eth_transport.multicast_socket,
                                        multicast_addr);
        if (err != TCPIP_OK) {
            status = ETH_ERROR;
        }
    }
    
    return status;
}

eth_status_t dds_eth_calc_multicast_addr(uint32_t domain_id, uint32_t participant_id,
                                          eth_ip_addr_t *multicast_addr)
{
    eth_status_t status = ETH_OK;
    
    if (multicast_addr == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* DDS规范: 多播地址计算公式
         * 239.255.0.1 + (domain_id % 256) */
        uint32_t last_octet = (domain_id + participant_id) % 256U;
        *multicast_addr = ETH_IP_ADDR(239, 255, 0, (uint8_t)last_octet);
    }
    
    return status;
}

/* ============================================================================
 * RTPS消息封装/解封API实现
 * ============================================================================ */

eth_status_t dds_eth_create_rtps_header(dds_rtps_header_t *header,
                                         const uint8_t guid_prefix[12])
{
    eth_status_t status = ETH_OK;
    
    if ((header == NULL) || (guid_prefix == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        header->protocol_id = DDS_RTPS_PROTOCOL_ID;
        header->protocol_version = DDS_RTPS_PROTOCOL_VERSION_2_5;
        header->vendor_id = 0x0001; /* Vendor ID for this implementation */
        (void)memcpy(header->guid_prefix, guid_prefix, 12);
    }
    
    return status;
}

eth_status_t dds_eth_parse_rtps_header(const uint8_t *data, uint32_t len,
                                        dds_rtps_header_t *header,
                                        uint32_t *header_len)
{
    eth_status_t status = ETH_OK;
    
    if ((data == NULL) || (header == NULL) || (header_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (len < sizeof(dds_rtps_header_t)) {
        status = ETH_ERROR;
    }
    else {
        /* 解析协议ID */
        header->protocol_id = ((uint32_t)data[0] << 24) |
                              ((uint32_t)data[1] << 16) |
                              ((uint32_t)data[2] << 8) |
                              (uint32_t)data[3];
        
        if (header->protocol_id != DDS_RTPS_PROTOCOL_ID) {
            status = ETH_ERROR;
        }
        else {
            header->protocol_version = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
            header->vendor_id = ((uint16_t)data[6] << 8) | (uint16_t)data[7];
            (void)memcpy(header->guid_prefix, &data[8], 12);
            *header_len = sizeof(dds_rtps_header_t);
        }
    }
    
    return status;
}

eth_status_t dds_eth_encapsulate_data(uint8_t *buffer, uint32_t buf_size,
                                       const uint8_t writer_id[4],
                                       const uint8_t reader_id[4],
                                       uint64_t seq_num,
                                       const uint8_t *payload,
                                       uint32_t payload_len,
                                       uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((buffer == NULL) || (writer_id == NULL) || (reader_id == NULL) ||
        (payload == NULL) || (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 计算需要的总长度 */
        uint32_t total_len = sizeof(dds_rtps_data_submsg_t) + payload_len;
        if (total_len > buf_size) {
            status = ETH_NO_MEMORY;
        }
        else {
            dds_rtps_data_submsg_t *data_submsg = (dds_rtps_data_submsg_t *)buffer;
            
            /* 填充子消息头 */
            data_submsg->header.submessage_id = DDS_RTPS_SUBMSG_DATA;
            data_submsg->header.flags = DDS_RTPS_SUBMSG_FLAG_ENDIANNESS | 
                                       DDS_RTPS_SUBMSG_FLAG_DATA_PRESENT;
            data_submsg->header.submessage_length = (uint16_t)(sizeof(dds_rtps_data_submsg_t) - 
                                                               sizeof(dds_rtps_submsg_header_t) +
                                                               payload_len - 4);
            
            data_submsg->extra_flags = 0;
            data_submsg->octets_to_inline_qos = 16; /* 到inline QoS的偏移 */
            (void)memcpy(data_submsg->reader_id, reader_id, 4);
            (void)memcpy(data_submsg->writer_id, writer_id, 4);
            data_submsg->writer_seq_num_high = (uint32_t)(seq_num >> 32);
            data_submsg->writer_seq_num_low = (uint32_t)(seq_num & 0xFFFFFFFFU);
            
            offset = sizeof(dds_rtps_data_submsg_t);
            
            /* 复制负载 */
            (void)memcpy(&buffer[offset], payload, payload_len);
            offset += payload_len;
            
            *actual_len = offset;
        }
    }
    
    return status;
}

eth_status_t dds_eth_decapsulate_data(const uint8_t *data, uint32_t len,
                                       uint8_t writer_id[4],
                                       uint8_t reader_id[4],
                                       uint64_t *seq_num,
                                       const uint8_t **payload,
                                       uint32_t *payload_len)
{
    eth_status_t status = ETH_OK;
    
    if ((data == NULL) || (writer_id == NULL) || (reader_id == NULL) ||
        (seq_num == NULL) || (payload == NULL) || (payload_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (len < sizeof(dds_rtps_data_submsg_t)) {
        status = ETH_ERROR;
    }
    else {
        const dds_rtps_data_submsg_t *data_submsg = (const dds_rtps_data_submsg_t *)data;
        
        if (data_submsg->header.submessage_id != DDS_RTPS_SUBMSG_DATA) {
            status = ETH_ERROR;
        }
        else {
            (void)memcpy(reader_id, data_submsg->reader_id, 4);
            (void)memcpy(writer_id, data_submsg->writer_id, 4);
            *seq_num = ((uint64_t)data_submsg->writer_seq_num_high << 32) |
                       (uint64_t)data_submsg->writer_seq_num_low;
            
            /* 计算负载起始位置 */
            uint32_t header_size = sizeof(dds_rtps_data_submsg_t);
            if (len > header_size) {
                *payload = &data[header_size];
                *payload_len = len - header_size;
            }
            else {
                *payload = NULL;
                *payload_len = 0;
            }
        }
    }
    
    return status;
}

eth_status_t dds_eth_create_heartbeat(uint8_t *buffer, uint32_t buf_size,
                                       const uint8_t writer_id[4],
                                       uint64_t first_seq,
                                       uint64_t last_seq,
                                       uint32_t count,
                                       uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    
    if ((buffer == NULL) || (writer_id == NULL) || (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (buf_size < sizeof(dds_rtps_heartbeat_submsg_t)) {
        status = ETH_NO_MEMORY;
    }
    else {
        dds_rtps_heartbeat_submsg_t *hb = (dds_rtps_heartbeat_submsg_t *)buffer;
        
        hb->header.submessage_id = DDS_RTPS_SUBMSG_HEARTBEAT;
        hb->header.flags = DDS_RTPS_SUBMSG_FLAG_ENDIANNESS;
        hb->header.submessage_length = sizeof(dds_rtps_heartbeat_submsg_t) - 
                                       sizeof(dds_rtps_submsg_header_t);
        
        (void)memcpy(hb->writer_id, writer_id, 4);
        hb->reader_id[0] = 0; hb->reader_id[1] = 0;
        hb->reader_id[2] = 0; hb->reader_id[3] = 0; /* EntityId unknown */
        
        hb->first_seq_num_high = (uint32_t)(first_seq >> 32);
        hb->first_seq_num_low = (uint32_t)(first_seq & 0xFFFFFFFFU);
        hb->last_seq_num_high = (uint32_t)(last_seq >> 32);
        hb->last_seq_num_low = (uint32_t)(last_seq & 0xFFFFFFFFU);
        hb->count = count;
        
        *actual_len = sizeof(dds_rtps_heartbeat_submsg_t);
        
        g_eth_transport.stats.heartbeat_sent++;
    }
    
    return status;
}

eth_status_t dds_eth_create_acknack(uint8_t *buffer, uint32_t buf_size,
                                     const uint8_t reader_id[4],
                                     const uint8_t writer_id[4],
                                     uint32_t bitmap,
                                     uint32_t count,
                                     uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    
    if ((buffer == NULL) || (reader_id == NULL) || (writer_id == NULL) || 
        (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (buf_size < sizeof(dds_rtps_acknack_submsg_t)) {
        status = ETH_NO_MEMORY;
    }
    else {
        dds_rtps_acknack_submsg_t *ack = (dds_rtps_acknack_submsg_t *)buffer;
        
        ack->header.submessage_id = DDS_RTPS_SUBMSG_ACKNACK;
        ack->header.flags = DDS_RTPS_SUBMSG_FLAG_ENDIANNESS;
        ack->header.submessage_length = sizeof(dds_rtps_acknack_submsg_t) - 
                                        sizeof(dds_rtps_submsg_header_t);
        
        (void)memcpy(ack->reader_id, reader_id, 4);
        (void)memcpy(ack->writer_id, writer_id, 4);
        ack->reader_sn_state = bitmap;
        ack->count = count;
        
        *actual_len = sizeof(dds_rtps_acknack_submsg_t);
        
        g_eth_transport.stats.acknack_sent++;
    }
    
    return status;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static eth_status_t dds_eth_init_sockets(void)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    
    /* 创建UDP单播套接字 */
    err = tcpip_socket_create(TCPIP_SOCK_DGRAM, &g_eth_transport.unicast_socket);
    if (err != TCPIP_OK) {
        status = ETH_ERROR;
    }
    else {
        /* 创建多播套接字 */
        err = tcpip_socket_create(TCPIP_SOCK_DGRAM, &g_eth_transport.multicast_socket);
        if (err != TCPIP_OK) {
            (void)tcpip_socket_close(g_eth_transport.unicast_socket);
            status = ETH_ERROR;
        }
    }
    
    return status;
}

static void dds_eth_deinit_sockets(void)
{
    (void)tcpip_socket_close(g_eth_transport.unicast_socket);
    (void)tcpip_socket_close(g_eth_transport.multicast_socket);
}

static eth_status_t dds_eth_process_unicast(void)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    tcpip_sockaddr_t src_addr;
    uint16_t recv_len = sizeof(g_eth_transport.rx_buffer);
    
    err = tcpip_udp_recvfrom(g_eth_transport.unicast_socket,
                             &src_addr,
                             g_eth_transport.rx_buffer,
                             &recv_len,
                             0); /* 非阻塞 */
    
    if (err == TCPIP_OK) {
        g_eth_transport.stats.rx_packets++;
        g_eth_transport.stats.rx_bytes += recv_len;
        
        (void)dds_eth_handle_rtps_packet(g_eth_transport.rx_buffer, 
                                          recv_len, &src_addr);
    }
    else if (err == TCPIP_ERROR_TIMEOUT) {
        /* 没有数据，正常 */
        status = ETH_OK;
    }
    else {
        g_eth_transport.stats.rx_errors++;
        status = ETH_ERROR;
    }
    
    return status;
}

static eth_status_t dds_eth_process_multicast(void)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    tcpip_sockaddr_t src_addr;
    uint16_t recv_len = sizeof(g_eth_transport.rx_buffer);
    
    err = tcpip_udp_recvfrom(g_eth_transport.multicast_socket,
                             &src_addr,
                             g_eth_transport.rx_buffer,
                             &recv_len,
                             0); /* 非阻塞 */
    
    if (err == TCPIP_OK) {
        g_eth_transport.stats.rx_packets++;
        g_eth_transport.stats.rx_bytes += recv_len;
        
        (void)dds_eth_handle_rtps_packet(g_eth_transport.rx_buffer,
                                          recv_len, &src_addr);
    }
    else if (err == TCPIP_ERROR_TIMEOUT) {
        status = ETH_OK;
    }
    else {
        g_eth_transport.stats.rx_errors++;
    }
    
    return status;
}

static eth_status_t dds_eth_handle_rtps_packet(const uint8_t *data, uint32_t len,
                                                const tcpip_sockaddr_t *src_addr)
{
    eth_status_t status = ETH_OK;
    dds_rtps_header_t header;
    uint32_t header_len;
    
    /* 解析RTPS头 */
    status = dds_eth_parse_rtps_header(data, len, &header, &header_len);
    
    if (status == ETH_OK) {
        /* 检查是否来自己己 */
        if (memcmp(header.guid_prefix, g_eth_transport.local_guid_prefix, 12) != 0) {
            /* 处理子消息 */
            uint32_t offset = header_len;
            
            while (offset < len) {
                if ((len - offset) < sizeof(dds_rtps_submsg_header_t)) {
                    break;
                }
                
                const dds_rtps_submsg_header_t *submsg_hdr = 
                    (const dds_rtps_submsg_header_t *)&data[offset];
                
                switch (submsg_hdr->submessage_id) {
                    case DDS_RTPS_SUBMSG_DATA:
                        /* 处理数据消息 */
                        if (g_eth_transport.data_rx_callback != NULL) {
                            dds_endpoint_locator_t locator;
                            locator.ip_addr = src_addr->addr;
                            locator.port = src_addr->port;
                            locator.is_multicast = false;
                            
                            g_eth_transport.data_rx_callback(data + offset,
                                                             len - offset,
                                                             &locator,
                                                             g_eth_transport.data_rx_user_data);
                        }
                        break;
                        
                    case DDS_RTPS_SUBMSG_HEARTBEAT:
                        g_eth_transport.stats.heartbeat_received++;
                        break;
                        
                    case DDS_RTPS_SUBMSG_ACKNACK:
                        g_eth_transport.stats.acknack_received++;
                        break;
                        
                    default:
                        /* 其他子消息类型 */
                        break;
                }
                
                /* 移到下一个子消息 */
                offset += sizeof(dds_rtps_submsg_header_t) + 
                         submsg_hdr->submessage_length;
                
                /* 对齐到4字节边界 */
                if ((offset % 4) != 0) {
                    offset += 4 - (offset % 4);
                }
            }
        }
    }
    
    return status;
}

static int32_t dds_eth_find_free_endpoint_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_ETH_MAX_ENDPOINTS; i++) {
        if (!g_eth_transport.endpoints[i].used) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_eth_find_free_participant_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_ETH_MAX_PARTICIPANTS; i++) {
        if (!g_eth_transport.participants[i].used) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_eth_find_participant_by_guid(const dds_guid_t *guid)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_ETH_MAX_PARTICIPANTS; i++) {
        if (g_eth_transport.participants[i].used) {
            if (dds_eth_guid_equal(&g_eth_transport.participants[i].info.guid, guid)) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static bool dds_eth_guid_equal(const dds_guid_t *a, const dds_guid_t *b)
{
    return (memcmp(a->prefix, b->prefix, 12) == 0) &&
           (memcmp(a->entity_id, b->entity_id, 4) == 0);
}

static void dds_eth_generate_guid_prefix(uint8_t prefix[12], uint16_t domain_id)
{
    /* 生成GUID前缀:
     * 前4字节: Vendor ID + 协议版本
     * 中两4字节: Domain ID + Participant ID
     * 后4字节: 随机值(用MAC地址和时间生成) */
    
    prefix[0] = 0x00; /* Vendor ID high */
    prefix[1] = 0x01; /* Vendor ID low */
    prefix[2] = 0x02; /* Protocol major */
    prefix[3] = 0x05; /* Protocol minor */
    
    prefix[4] = (uint8_t)((domain_id >> 8) & 0xFF);
    prefix[5] = (uint8_t)(domain_id & 0xFF);
    prefix[6] = 0x00; /* Participant ID high */
    prefix[7] = 0x01; /* Participant ID low */
    
    /* 使用MAC地址后4字节 */
    prefix[8] = g_eth_transport.config.local_mac[2];
    prefix[9] = g_eth_transport.config.local_mac[3];
    prefix[10] = g_eth_transport.config.local_mac[4];
    prefix[11] = g_eth_transport.config.local_mac[5];
}

static eth_status_t dds_eth_sendto_socket(tcpip_socket_id_t socket,
                                           const uint8_t *data, uint32_t len,
                                           const tcpip_sockaddr_t *dest)
{
    eth_status_t status = ETH_OK;
    tcpip_error_t err;
    uint16_t sent_len = (uint16_t)len;
    
    err = tcpip_udp_sendto(socket, dest, data, sent_len);
    if (err != TCPIP_OK) {
        status = ETH_ERROR;
    }
    
    return status;
}
