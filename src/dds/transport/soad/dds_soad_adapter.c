/**
 * @file dds_soad_adapter.c
 * @brief DDS到SoAd适配层实现
 * @version 1.0
 * @date 2026-04-26
 */

#include <string.h>
#include <stdio.h>
#include "dds_soad_adapter.h"

/* ============================================================================
 * 私有定义
 * ============================================================================ */

typedef struct {
    bool used;
    dds_topic_name_t topic_name;
    dds_type_name_t type_name;
    dds_qos_t qos;
    SoAd_SoConIdType tx_socket_id;
    SoAd_SoConIdType rx_socket_id;
    dds_data_callback_t rx_callback;
    void *rx_user_data;
} dds_soad_topic_entry_t;

typedef struct {
    bool initialized;
    dds_soad_adapter_config_t config;
    dds_soad_topic_entry_t topic_mappings[DDS_SOAD_MAX_MAPPINGS];
    uint16_t next_port;
    uint8_t default_qos_map_count;
} dds_soad_adapter_ctx_t;

static dds_soad_adapter_ctx_t g_adapter_ctx = {0};

/* 默认QoS映射表 */
static const dds_soad_qos_mapping_t g_default_qos_maps[] = {
    /* 可靠传输 - TCP */
    {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .transport = DDS_SOAD_TRANSPORT_TCP,
        .port = DDS_SOAD_DEFAULT_TCP_PORT,
        .buffer_size = 4096,
        .timeout_ms = 5000,
        .enable_keepalive = true,
        .keepalive_interval = 60,
        .enable_nagle = false,
        .priority = 5
    },
    /* 最佳努力传输 - UDP */
    {
        .reliability = DDS_QOS_RELIABILITY_BEST_EFFORT,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .transport = DDS_SOAD_TRANSPORT_UDP,
        .port = DDS_SOAD_DEFAULT_BESTEFFORT_PORT,
        .buffer_size = 2048,
        .timeout_ms = 1000,
        .enable_keepalive = false,
        .keepalive_interval = 0,
        .enable_nagle = false,
        .priority = 3
    },
    /* 可靠多播 - UDP多播 */
    {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
        .transport = DDS_SOAD_TRANSPORT_UDP_MULTICAST,
        .port = DDS_SOAD_DEFAULT_RELIABLE_PORT,
        .buffer_size = 4096,
        .timeout_ms = 3000,
        .enable_keepalive = false,
        .keepalive_interval = 0,
        .enable_nagle = false,
        .priority = 4
    }
};

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int32_t dds_soad_find_free_mapping_slot(void);
static int32_t dds_soad_find_mapping_by_topic(const char *topic_name);
static eth_status_t dds_soad_setup_socket_for_qos(const dds_qos_t *qos,
                                                   SoAd_SoConIdType *socket_id,
                                                   bool is_tx);
static void dds_soad_rx_indication(uint16_t pduId, const uint8_t *data, uint16_t length);
static void dds_soad_tx_confirmation(uint16_t pduId);
static void dds_soad_mode_change(SoAd_SoConIdType soConId, SoAd_SocketStateType newMode);

/* ============================================================================
 * 初始化和反初始化API实现
 * ============================================================================ */

eth_status_t dds_soad_adapter_init(const dds_soad_adapter_config_t *config)
{
    eth_status_t status = ETH_OK;
    SoAd_ReturnType soad_result;
    
    if (config == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (g_adapter_ctx.initialized) {
        status = ETH_BUSY;
    }
    else {
        /* 复制配置 */
        (void)memcpy(&g_adapter_ctx.config, config, sizeof(dds_soad_adapter_config_t));
        
        /* 初始化状态 */
        g_adapter_ctx.initialized = false;
        g_adapter_ctx.next_port = DDS_SOAD_DEFAULT_RELIABLE_PORT + 100;
        
        /* 清零Topic映射表 */
        (void)memset(g_adapter_ctx.topic_mappings, 0, sizeof(g_adapter_ctx.topic_mappings));
        
        /* 初始化SoAd */
        soad_result = SoAd_Init(config->soad_config);
        if (soad_result != SOAD_E_NO_ERROR) {
            status = ETH_ERROR;
        }
        else {
            /* 注册SoAd回调 */
            SoAd_RegisterRxIndication(dds_soad_rx_indication);
            SoAd_RegisterTxConfirmation(dds_soad_tx_confirmation);
            SoAd_RegisterSoConModeChg(dds_soad_mode_change);
            
            /* 初始化默认QoS映射 */
            if (config->num_qos_mappings == 0) {
                g_adapter_ctx.default_qos_map_count = sizeof(g_default_qos_maps) / 
                                                       sizeof(g_default_qos_maps[0]);
            }
            
            g_adapter_ctx.initialized = true;
        }
    }
    
    return status;
}

void dds_soad_adapter_deinit(void)
{
    if (g_adapter_ctx.initialized) {
        /* 关闭所有Topic Socket */
        for (uint16_t i = 0; i < DDS_SOAD_MAX_MAPPINGS; i++) {
            if (g_adapter_ctx.topic_mappings[i].used) {
                (void)dds_soad_delete_topic_socket(i);
            }
        }
        
        /* 反初始化SoAd */
        SoAd_DeInit();
        
        /* 清零上下文 */
        (void)memset(&g_adapter_ctx, 0, sizeof(g_adapter_ctx));
    }
}

eth_status_t dds_soad_adapter_start(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 启动所有已配置的Topic Socket */
        for (uint16_t i = 0; i < DDS_SOAD_MAX_MAPPINGS; i++) {
            if (g_adapter_ctx.topic_mappings[i].used) {
                SoAd_ReturnType result = SoAd_OpenSoCon(
                    g_adapter_ctx.topic_mappings[i].tx_socket_id);
                if (result != SOAD_E_NO_ERROR) {
                    status = ETH_ERROR;
                    break;
                }
                
                result = SoAd_OpenSoCon(g_adapter_ctx.topic_mappings[i].rx_socket_id);
                if (result != SOAD_E_NO_ERROR) {
                    status = ETH_ERROR;
                    break;
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_soad_adapter_stop(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 关闭所有Topic Socket */
        for (uint16_t i = 0; i < DDS_SOAD_MAX_MAPPINGS; i++) {
            if (g_adapter_ctx.topic_mappings[i].used) {
                (void)SoAd_CloseSoCon(g_adapter_ctx.topic_mappings[i].tx_socket_id, false);
                (void)SoAd_CloseSoCon(g_adapter_ctx.topic_mappings[i].rx_socket_id, false);
            }
        }
    }
    
    return status;
}

eth_status_t dds_soad_adapter_main_function(uint32_t elapsed_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 调用SoAd主函数 */
        SoAd_MainFunction();
        
        /* 处理超时等逻辑 (如果需要) */
        (void)elapsed_ms;
    }
    
    return status;
}

/* ============================================================================
 * QoS映射API实现
 * ============================================================================ */

eth_status_t dds_soad_get_socket_config_for_qos(const dds_qos_t *qos,
                                                 dds_soad_qos_mapping_t *mapping)
{
    eth_status_t status = ETH_OK;
    
    if ((qos == NULL) || (mapping == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 查找最匹配的默认映射 */
        const dds_soad_qos_mapping_t *best_match = NULL;
        
        for (uint8_t i = 0; i < g_adapter_ctx.default_qos_map_count; i++) {
            if (g_default_qos_maps[i].reliability == qos->reliability) {
                best_match = &g_default_qos_maps[i];
                break;
            }
        }
        
        if (best_match == NULL) {
            /* 使用缺省映射 (最佳努力) */
            best_match = &g_default_qos_maps[1];
        }
        
        (void)memcpy(mapping, best_match, sizeof(dds_soad_qos_mapping_t));
        
        /* 根据具体的预期性调整 */
        if (qos->deadline_ms > 0) {
            mapping->timeout_ms = qos->deadline_ms;
        }
    }
    
    return status;
}

eth_status_t dds_soad_register_qos_mapping(const dds_soad_qos_mapping_t *mapping)
{
    eth_status_t status = ETH_OK;
    
    if (mapping == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (g_adapter_ctx.config.num_qos_mappings >= DDS_SOAD_MAX_QOS_PROFILES) {
        status = ETH_NO_MEMORY;
    }
    else {
        uint8_t idx = g_adapter_ctx.config.num_qos_mappings;
        (void)memcpy(&g_adapter_ctx.config.qos_mappings[idx], mapping,
                    sizeof(dds_soad_qos_mapping_t));
        g_adapter_ctx.config.num_qos_mappings++;
    }
    
    return status;
}

eth_status_t dds_soad_unregister_qos_mapping(dds_qos_reliability_t reliability,
                                              dds_qos_durability_t durability)
{
    eth_status_t status = ETH_ERROR;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 查找并删除匹配的映射 */
        for (uint8_t i = 0; i < g_adapter_ctx.config.num_qos_mappings; i++) {
            if (g_adapter_ctx.config.qos_mappings[i].reliability == reliability) {
                /* 移除该映射 (简化实现: 将后面的映射向前移动) */
                for (uint8_t j = i; j < g_adapter_ctx.config.num_qos_mappings - 1; j++) {
                    (void)memcpy(&g_adapter_ctx.config.qos_mappings[j],
                                &g_adapter_ctx.config.qos_mappings[j + 1],
                                sizeof(dds_soad_qos_mapping_t));
                }
                g_adapter_ctx.config.num_qos_mappings--;
                status = ETH_OK;
                break;
            }
        }
    }
    
    (void)durability;
    return status;
}

eth_status_t dds_soad_create_topic_socket(const char *topic_name,
                                           const char *type_name,
                                           const dds_qos_t *qos,
                                           uint16_t *mapping_id)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((topic_name == NULL) || (qos == NULL) || (mapping_id == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (strlen(topic_name) >= sizeof(dds_topic_name_t)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 查找是否已存在 */
        idx = dds_soad_find_mapping_by_topic(topic_name);
        if (idx >= 0) {
            *mapping_id = (uint16_t)idx;
            status = ETH_OK; /* 已存在，返回现有映射 */
        }
        else {
            /* 创建新映射 */
            idx = dds_soad_find_free_mapping_slot();
            if (idx < 0) {
                status = ETH_NO_MEMORY;
            }
            else {
                dds_soad_topic_entry_t *entry = &g_adapter_ctx.topic_mappings[idx];
                
                entry->used = true;
                (void)strncpy(entry->topic_name, topic_name, sizeof(entry->topic_name) - 1);
                entry->topic_name[sizeof(entry->topic_name) - 1] = '\0';
                
                if (type_name != NULL) {
                    (void)strncpy(entry->type_name, type_name, sizeof(entry->type_name) - 1);
                    entry->type_name[sizeof(entry->type_name) - 1] = '\0';
                }
                else {
                    entry->type_name[0] = '\0';
                }
                
                (void)memcpy(&entry->qos, qos, sizeof(dds_qos_t));
                entry->rx_callback = NULL;
                entry->rx_user_data = NULL;
                
                /* 创建Socket */
                status = dds_soad_setup_socket_for_qos(qos, &entry->tx_socket_id, true);
                if (status == ETH_OK) {
                    status = dds_soad_setup_socket_for_qos(qos, &entry->rx_socket_id, false);
                }
                
                if (status == ETH_OK) {
                    *mapping_id = (uint16_t)idx;
                }
                else {
                    /* 清理 */
                    entry->used = false;
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_soad_delete_topic_socket(uint16_t mapping_id)
{
    eth_status_t status = ETH_OK;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (mapping_id >= DDS_SOAD_MAX_MAPPINGS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_adapter_ctx.topic_mappings[mapping_id].used) {
        status = ETH_ERROR;
    }
    else {
        dds_soad_topic_entry_t *entry = &g_adapter_ctx.topic_mappings[mapping_id];
        
        /* 关闭Socket */
        (void)SoAd_CloseSoCon(entry->tx_socket_id, true);
        (void)SoAd_CloseSoCon(entry->rx_socket_id, true);
        
        /* 清理条目 */
        entry->used = false;
    }
    
    return status;
}

/* ============================================================================
 * 数据传输API实现
 * ============================================================================ */

eth_status_t dds_soad_send_data(const char *topic_name,
                                 const uint8_t *data,
                                 uint32_t len,
                                 uint32_t timeout_ms)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((topic_name == NULL) || (data == NULL) || (len == 0)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_soad_find_mapping_by_topic(topic_name);
        
        if (idx < 0) {
            status = ETH_ERROR; /* Topic未配置 */
        }
        else {
            dds_soad_topic_entry_t *entry = &g_adapter_ctx.topic_mappings[idx];
            SoAd_PduInfoType pdu_info;
            
            pdu_info.data = (uint8_t *)data;
            pdu_info.length = (uint16_t)len;
            pdu_info.pdu_id = (uint16_t)idx;
            pdu_info.route_id = entry->tx_socket_id;
            
            SoAd_ReturnType result = SoAd_IfTransmit((uint16_t)idx, &pdu_info);
            
            if (result != SOAD_E_NO_ERROR) {
                status = ETH_ERROR;
            }
        }
    }
    
    (void)timeout_ms;
    return status;
}

eth_status_t dds_soad_receive_data(const char *topic_name,
                                    uint8_t *buffer,
                                    uint32_t buf_size,
                                    uint32_t *received_len,
                                    uint32_t timeout_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 接收由回调处理，这里只返回状态 */
        (void)topic_name;
        (void)buffer;
        (void)buf_size;
        (void)received_len;
        (void)timeout_ms;
    }
    
    return status;
}

eth_status_t dds_soad_register_rx_callback(const char *topic_name,
                                            dds_data_callback_t callback,
                                            void *user_data)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_adapter_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((topic_name == NULL) || (callback == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_soad_find_mapping_by_topic(topic_name);
        
        if (idx < 0) {
            status = ETH_ERROR;
        }
        else {
            g_adapter_ctx.topic_mappings[idx].rx_callback = callback;
            g_adapter_ctx.topic_mappings[idx].rx_user_data = user_data;
        }
    }
    
    return status;
}

/* ============================================================================
 * 内部映射API实现
 * ============================================================================ */

eth_status_t dds_soad_convert_locator(const dds_endpoint_locator_t *dds_locator,
                                       SoAd_SocketAddrType *soad_addr)
{
    eth_status_t status = ETH_OK;
    
    if ((dds_locator == NULL) || (soad_addr == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        soad_addr->addr = dds_locator->ip_addr;
        soad_addr->port = dds_locator->port;
        soad_addr->is_multicast = dds_locator->is_multicast;
    }
    
    return status;
}

eth_status_t dds_soad_convert_to_locator(const SoAd_SocketAddrType *soad_addr,
                                          dds_endpoint_locator_t *dds_locator)
{
    eth_status_t status = ETH_OK;
    
    if ((soad_addr == NULL) || (dds_locator == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        dds_locator->ip_addr = soad_addr->addr;
        dds_locator->port = soad_addr->port;
        dds_locator->is_multicast = soad_addr->is_multicast;
        if (soad_addr->is_multicast) {
            dds_locator->multicast_addr = soad_addr->addr;
        }
    }
    
    return status;
}

eth_status_t dds_soad_create_qos_socket(const dds_qos_t *qos,
                                         bool is_publisher,
                                         SoAd_SoConIdType *socket_id)
{
    eth_status_t status = ETH_OK;
    dds_soad_qos_mapping_t mapping;
    SoAd_SocketConnectionCfgType socket_cfg;
    
    if ((qos == NULL) || (socket_id == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 获取QoS映射 */
        status = dds_soad_get_socket_config_for_qos(qos, &mapping);
        
        if (status == ETH_OK) {
            /* 配置Socket */
            (void)memset(&socket_cfg, 0, sizeof(socket_cfg));
            
            if (mapping.transport == DDS_SOAD_TRANSPORT_TCP) {
                socket_cfg.socket_type = SOAD_SOCKET_TYPE_STREAM;
                socket_cfg.protocol = SOAD_SOCKET_PROTOCOL_TCP;
            }
            else {
                socket_cfg.socket_type = SOAD_SOCKET_TYPE_DGRAM;
                socket_cfg.protocol = SOAD_SOCKET_PROTOCOL_UDP;
            }
            
            socket_cfg.local_port = g_adapter_ctx.next_port++;
            socket_cfg.local_ip = g_adapter_ctx.config.local_ip;
            socket_cfg.auto_connect = is_publisher;
            socket_cfg.enable_keepalive = mapping.enable_keepalive;
            socket_cfg.keepalive_time = mapping.keepalive_interval;
            socket_cfg.enable_nagle = mapping.enable_nagle;
            socket_cfg.rx_buffer_size = mapping.buffer_size;
            socket_cfg.tx_buffer_size = mapping.buffer_size;
            socket_cfg.tx_timeout_ms = (uint16_t)mapping.timeout_ms;
            
            /* 创建Socket连接 */
            SoAd_ReturnType result = SoAd_OpenSoCon(*socket_id);
            if (result != SOAD_E_NO_ERROR) {
                status = ETH_ERROR;
            }
        }
    }
    
    (void)is_publisher;
    return status;
}

eth_status_t dds_soad_close_qos_socket(SoAd_SoConIdType socket_id)
{
    eth_status_t status = ETH_OK;
    
    SoAd_ReturnType result = SoAd_CloseSoCon(socket_id, false);
    if (result != SOAD_E_NO_ERROR) {
        status = ETH_ERROR;
    }
    
    return status;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static int32_t dds_soad_find_free_mapping_slot(void)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < DDS_SOAD_MAX_MAPPINGS; i++) {
        if (!g_adapter_ctx.topic_mappings[i].used) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_soad_find_mapping_by_topic(const char *topic_name)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < DDS_SOAD_MAX_MAPPINGS; i++) {
        if (g_adapter_ctx.topic_mappings[i].used) {
            if (strcmp(g_adapter_ctx.topic_mappings[i].topic_name, topic_name) == 0) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static eth_status_t dds_soad_setup_socket_for_qos(const dds_qos_t *qos,
                                                   SoAd_SoConIdType *socket_id,
                                                   bool is_tx)
{
    eth_status_t status = ETH_OK;
    dds_soad_qos_mapping_t mapping;
    
    (void)is_tx;
    
    /* 获取QoS映射 */
    status = dds_soad_get_socket_config_for_qos(qos, &mapping);
    
    if (status == ETH_OK) {
        /* 分配新的Socket ID */
        *socket_id = g_adapter_ctx.next_port++; /* 简化实现 */
    }
    
    return status;
}

static void dds_soad_rx_indication(uint16_t pduId, const uint8_t *data, uint16_t length)
{
    if (pduId < DDS_SOAD_MAX_MAPPINGS) {
        if (g_adapter_ctx.topic_mappings[pduId].used &&
            g_adapter_ctx.topic_mappings[pduId].rx_callback != NULL) {
            g_adapter_ctx.topic_mappings[pduId].rx_callback(
                data, length,
                g_adapter_ctx.topic_mappings[pduId].rx_user_data);
        }
    }
}

static void dds_soad_tx_confirmation(uint16_t pduId)
{
    /* 发送确认处理 */
    (void)pduId;
}

static void dds_soad_mode_change(SoAd_SoConIdType soConId, SoAd_SocketStateType newMode)
{
    /* Socket状态变化处理 */
    (void)soConId;
    (void)newMode;
}
