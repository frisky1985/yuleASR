#include "tcpip/tcpip_socket.h"

/**
 * @file soad.c
 * @brief AUTOSAR Socket Adapter (SoAd) 核心实现
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR 4.x规范
 * @note 支挂TCP/UDP Socket管理和PDU路由
 */

#include <string.h>
#include <stdio.h>
#include "soad.h"
#include "../tcpip/tcpip_socket.h"

/* ============================================================================
 * 私有定义
 * ============================================================================ */

typedef struct {
    bool used;
    SoAd_SocketConnectionCfgType config;
    SoAd_SoConStateType state;
    tcpip_socket_id_t socket_id;
    uint8_t rx_buffer[SOAD_SOCKET_RX_BUFFER_SIZE];
    uint8_t tx_buffer[SOAD_SOCKET_TX_BUFFER_SIZE];
    uint16_t rx_offset;
    uint16_t tx_offset;
    bool tx_in_progress;
} SoAd_SocketConnectionType;

typedef struct {
    bool used;
    SoAd_PduRouteCfgType config;
    bool enabled;
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
} SoAd_PduRouteType;

typedef struct {
    bool initialized;
    const SoAd_ConfigType *config;
    SoAd_SocketConnectionType sockets[SOAD_MAX_SOCKET_CONNECTIONS];
    SoAd_PduRouteType pdu_routes[SOAD_MAX_PDU_ROUTES];
    
    /* 回调函数 */
    SoAd_IfTxConfirmation_FuncType tx_confirmation_cb;
    SoAd_IfRxIndication_FuncType rx_indication_cb;
    SoAd_SoConModeChg_FuncType mode_chg_cb;
} SoAd_ContextType;

static SoAd_ContextType g_soad_context = {0};

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static SoAd_ReturnType soad_map_tcpip_error(tcpip_error_t err);
static int32_t soad_find_free_socket_slot(void);
static int32_t soad_find_socket_by_id(SoAd_SoConIdType soConId);
static int32_t soad_find_pdu_route_by_id(uint16_t pduId);
static int32_t soad_find_pdu_route_by_socket(SoAd_SoConIdType soConId);
static SoAd_ReturnType soad_init_socket(SoAd_SocketConnectionType *socket);
static SoAd_ReturnType soad_connect_tcp(SoAd_SocketConnectionType *socket);
static SoAd_ReturnType soad_listen_tcp(SoAd_SocketConnectionType *socket);
static SoAd_ReturnType soad_process_rx(SoAd_SocketConnectionType *socket);
static SoAd_ReturnType soad_process_tx(SoAd_SocketConnectionType *socket);
static SoAd_ReturnType soad_route_pdu_to_upper(uint16_t pduId, const uint8_t *data, uint16_t length);
static void soad_handle_socket_state_change(SoAd_SocketConnectionType *socket, 
                                             SoAd_SocketStateType new_state);

/* ============================================================================
 * 初始化和反初始化API实现
 * ============================================================================ */

SoAd_ReturnType SoAd_Init(const SoAd_ConfigType *config)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (config == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else if (g_soad_context.initialized) {
        result = SOAD_E_ALREADY_INITIALIZED;
    }
    else {
        g_soad_context.config = config;
        g_soad_context.initialized = false;
        
        /* 清零Socket连接表 */
        (void)memset(g_soad_context.sockets, 0, sizeof(g_soad_context.sockets));
        
        /* 清零PDU路由表 */
        (void)memset(g_soad_context.pdu_routes, 0, sizeof(g_soad_context.pdu_routes));
        
        /* 清零回调 */
        g_soad_context.tx_confirmation_cb = NULL;
        g_soad_context.rx_indication_cb = NULL;
        g_soad_context.mode_chg_cb = NULL;
        
        /* 初始化Socket模块 */
        tcpip_error_t err = tcpip_socket_init();
        if (err != TCPIP_OK) {
            result = soad_map_tcpip_error(err);
        }
        else {
            g_soad_context.initialized = true;
        }
    }
    
    return result;
}

void SoAd_DeInit(void)
{
    if (g_soad_context.initialized) {
        /* 关闭所有Socket连接 */
        for (uint16_t i = 0; i < SOAD_MAX_SOCKET_CONNECTIONS; i++) {
            if (g_soad_context.sockets[i].used) {
                (void)SoAd_CloseSoCon((SoAd_SoConIdType)i, true);
            }
        }
        
        /* 反初始化Socket模块 */
        tcpip_socket_deinit();
        
        /* 清零上下文 */
        (void)memset(&g_soad_context, 0, sizeof(g_soad_context));
    }
}

void SoAd_GetVersionInfo(Std_VersionInfoType *versioninfo)
{
    if (versioninfo != NULL) {
        versioninfo->vendorID = SOAD_VENDOR_ID;
        versioninfo->moduleID = SOAD_MODULE_ID;
        versioninfo->sw_major_version = SOAD_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = SOAD_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = SOAD_SW_PATCH_VERSION;
    }
}

void SoAd_MainFunction(void)
{
    if (g_soad_context.initialized) {
        /* 处理所有Socket */
        for (uint16_t i = 0; i < SOAD_MAX_SOCKET_CONNECTIONS; i++) {
            if (g_soad_context.sockets[i].used) {
                /* 处理接收 */
                (void)soad_process_rx(&g_soad_context.sockets[i]);
                
                /* 处理发送 */
                if (g_soad_context.sockets[i].tx_in_progress) {
                    (void)soad_process_tx(&g_soad_context.sockets[i]);
                }
                
                /* 更新Socket状态 */
                tcpip_socket_state_t tcpip_state;
                tcpip_error_t err = tcpip_socket_get_state(
                    g_soad_context.sockets[i].socket_id, &tcpip_state);
                
                if (err == TCPIP_OK) {
                    SoAd_SocketStateType new_state = g_soad_context.sockets[i].state.state;
                    
                    switch (tcpip_state) {
                        case TCPIP_SOCKET_STATE_CONNECTED:
                            new_state = SOAD_SOCKET_STATE_CONNECTED;
                            break;
                        case TCPIP_SOCKET_STATE_LISTENING:
                            new_state = SOAD_SOCKET_STATE_LISTENING;
                            break;
                        case TCPIP_SOCKET_STATE_CLOSED:
                            new_state = SOAD_SOCKET_STATE_CLOSED;
                            break;
                        default:
                            break;
                    }
                    
                    if (new_state != g_soad_context.sockets[i].state.state) {
                        soad_handle_socket_state_change(&g_soad_context.sockets[i], new_state);
                    }
                }
            }
        }
    }
}

/* ============================================================================
 * Socket连接管理API实现
 * ============================================================================ */

SoAd_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType soConId)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        idx = soad_find_socket_by_id(soConId);
        
        if (idx < 0) {
            /* 创建新Socket连接 */
            idx = soad_find_free_socket_slot();
            if (idx < 0) {
                result = SOAD_E_NO_SOCKET;
            }
            else {
                /* 从配置中获取Socket配置 */
                if (g_soad_context.config != NULL &&
                    soConId < g_soad_context.config->num_socket_conns) {
                    (void)memcpy(&g_soad_context.sockets[idx].config,
                                &g_soad_context.config->socket_conns[soConId],
                                sizeof(SoAd_SocketConnectionCfgType));
                    g_soad_context.sockets[idx].used = true;
                    
                    result = soad_init_socket(&g_soad_context.sockets[idx]);
                    
                    if (result == SOAD_E_NO_ERROR) {
                        /* 尝试连接 */
                        if (g_soad_context.sockets[idx].config.socket_type == 
                            SOAD_SOCKET_TYPE_STREAM) {
                            if (g_soad_context.sockets[idx].config.local_port != 0) {
                                /* 服务器模式 - 监听 */
                                result = soad_listen_tcp(&g_soad_context.sockets[idx]);
                            }
                            else {
                                /* 客户端模式 - 连接 */
                                result = soad_connect_tcp(&g_soad_context.sockets[idx]);
                            }
                        }
                        else {
                            /* UDP直接设为已连接 */
                            g_soad_context.sockets[idx].state.state = SOAD_SOCKET_STATE_CONNECTED;
                            soad_handle_socket_state_change(&g_soad_context.sockets[idx],
                                                           SOAD_SOCKET_STATE_CONNECTED);
                        }
                    }
                }
                else {
                    result = SOAD_E_INVALID_PARAMETER;
                }
            }
        }
        else {
            result = SOAD_E_SOCKET_IN_USE;
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType soConId, bool abort)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        idx = soad_find_socket_by_id(soConId);
        
        if (idx < 0) {
            result = SOAD_E_INVALID_PARAMETER;
        }
        else {
            /* 关闭Socket */
            tcpip_error_t err;
            
            if (abort) {
                err = tcpip_socket_abort(g_soad_context.sockets[idx].socket_id);
            }
            else {
                err = tcpip_socket_close(g_soad_context.sockets[idx].socket_id);
            }
            
            if (err == TCPIP_OK) {
                g_soad_context.sockets[idx].used = false;
                g_soad_context.sockets[idx].state.state = SOAD_SOCKET_STATE_CLOSED;
                soad_handle_socket_state_change(&g_soad_context.sockets[idx],
                                               SOAD_SOCKET_STATE_CLOSED);
            }
            else {
                result = soad_map_tcpip_error(err);
            }
        }
    }
    
    (void)abort;
    return result;
}

SoAd_ReturnType SoAd_GetSoConState(SoAd_SoConIdType soConId, SoAd_SoConStateType *state)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if (state == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        idx = soad_find_socket_by_id(soConId);
        
        if (idx < 0) {
            result = SOAD_E_INVALID_PARAMETER;
        }
        else {
            (void)memcpy(state, &g_soad_context.sockets[idx].state, 
                        sizeof(SoAd_SoConStateType));
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_SetSoConMode(SoAd_SoConIdType soConId, SoAd_SoConModeType mode)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        idx = soad_find_socket_by_id(soConId);
        
        if (idx < 0) {
            result = SOAD_E_INVALID_PARAMETER;
        }
        else {
            /* 设置Socket连接模式 */
            g_soad_context.sockets[idx].config.mode = mode;
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_RequestIpAddrAssignment(SoAd_SoConIdType soConId, bool *assigned)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if (assigned == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        /* 检查IP地址是否已分配 */
        *assigned = true; /* 简化实现，假设IP已分配 */
    }
    
    (void)soConId;
    return result;
}

SoAd_ReturnType SoAd_ReleaseIpAddrAssignment(SoAd_SoConIdType soConId)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    
    (void)soConId;
    return result;
}

SoAd_ReturnType SoAd_GetLocalAddr(SoAd_SoConIdType soConId, SoAd_SocketAddrType *localAddr)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if (localAddr == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        idx = soad_find_socket_by_id(soConId);
        
        if (idx < 0) {
            result = SOAD_E_INVALID_PARAMETER;
        }
        else {
            localAddr->addr = g_soad_context.sockets[idx].config.local_ip;
            localAddr->port = g_soad_context.sockets[idx].config.local_port;
            localAddr->is_multicast = false;
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_GetPhysAddr(SoAd_SoConIdType soConId, uint8_t physAddr[6])
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if (physAddr == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        /* 获取MAC地址 */
        /* 注: 实际实现需要从以太网驱动获取 */
        (void)memset(physAddr, 0, 6);
    }
    
    (void)soConId;
    return result;
}

/* ============================================================================
 * PDU传输API实现
 * ============================================================================ */

SoAd_ReturnType SoAd_IfTransmit(uint16_t pduId, const SoAd_PduInfoType *info)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t route_idx;
    int32_t socket_idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if (info == NULL) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        /* 查找PDU路由 */
        route_idx = soad_find_pdu_route_by_id(pduId);
        
        if (route_idx < 0) {
            result = SOAD_E_PDU_NOT_FOUND;
        }
        else if (!g_soad_context.pdu_routes[route_idx].enabled) {
            result = SOAD_E_ROUTING_ERROR;
        }
        else {
            /* 获取关联Socket */
            SoAd_SoConIdType socket_id = g_soad_context.pdu_routes[route_idx].config.socket_id;
            socket_idx = soad_find_socket_by_id(socket_id);
            
            if (socket_idx < 0) {
                result = SOAD_E_SOCKET_NOT_CONNECTED;
            }
            else if (g_soad_context.sockets[socket_idx].state.state != 
                     SOAD_SOCKET_STATE_CONNECTED) {
                result = SOAD_E_SOCKET_NOT_CONNECTED;
            }
            else if (g_soad_context.sockets[socket_idx].tx_in_progress) {
                result = SOAD_E_BUSY;
            }
            else {
                /* 复制数据到发送缓冲区 */
                uint16_t copy_len = info->length;
                if (copy_len > SOAD_SOCKET_TX_BUFFER_SIZE) {
                    copy_len = SOAD_SOCKET_TX_BUFFER_SIZE;
                }
                
                (void)memcpy(g_soad_context.sockets[socket_idx].tx_buffer,
                            info->data, copy_len);
                g_soad_context.sockets[socket_idx].tx_offset = 0;
                g_soad_context.sockets[socket_idx].tx_in_progress = true;
                
                /* 尝试立即发送 */
                result = soad_process_tx(&g_soad_context.sockets[socket_idx]);
                
                /* 更新统计 */
                if (result == SOAD_E_NO_ERROR) {
                    g_soad_context.pdu_routes[route_idx].tx_count++;
                }
                else {
                    g_soad_context.pdu_routes[route_idx].error_count++;
                }
            }
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_TpTransmit(uint16_t pduId, const SoAd_PduInfoType *info,
                                 uint8_t retry, uint16_t tpDataLength)
{
    /* TP传输与IF传输类似，但支持分段传输 */
    /* 简化实现，直接调用IF传输 */
    (void)retry;
    (void)tpDataLength;
    return SoAd_IfTransmit(pduId, info);
}

SoAd_ReturnType SoAd_TpProvideRxBuffer(uint16_t pduId, uint8_t **buffer, uint16_t length)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t socket_idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else if ((buffer == NULL) || (length == 0)) {
        result = SOAD_E_INVALID_PARAMETER;
    }
    else {
        /* 查找PDU路由对应的Socket */
        int32_t route_idx = soad_find_pdu_route_by_id(pduId);
        if (route_idx < 0) {
            result = SOAD_E_PDU_NOT_FOUND;
        }
        else {
            socket_idx = soad_find_socket_by_id(
                g_soad_context.pdu_routes[route_idx].config.socket_id);
            
            if (socket_idx < 0) {
                result = SOAD_E_SOCKET_NOT_CONNECTED;
            }
            else if (length > SOAD_SOCKET_RX_BUFFER_SIZE) {
                result = SOAD_E_PDU_TOO_LARGE;
            }
            else {
                *buffer = g_soad_context.sockets[socket_idx].rx_buffer;
            }
        }
    }
    
    return result;
}

SoAd_ReturnType SoAd_TpRxIndication(uint16_t pduId, uint16_t length)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        /* 触发上层接收指示 */
        result = soad_route_pdu_to_upper(pduId, NULL, length);
    }
    
    return result;
}

/* ============================================================================
 * 路由组API实现
 * ============================================================================ */

SoAd_ReturnType SoAd_OpenRoutingGroup(uint16_t routingGroup)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        /* 打开指定路由组中的所有Socket */
        for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
            if (g_soad_context.pdu_routes[i].used) {
                /* 注: 实际实现需要检查路由组ID */
                (void)SoAd_OpenSoCon(g_soad_context.pdu_routes[i].config.socket_id);
            }
        }
    }
    
    (void)routingGroup;
    return result;
}

SoAd_ReturnType SoAd_CloseRoutingGroup(uint16_t routingGroup)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        /* 关闭指定路由组中的所有Socket */
        for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
            if (g_soad_context.pdu_routes[i].used) {
                (void)SoAd_CloseSoCon(g_soad_context.pdu_routes[i].config.socket_id, false);
            }
        }
    }
    
    (void)routingGroup;
    return result;
}

SoAd_ReturnType SoAd_EnableRouting(uint16_t routingGroup)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        /* 使能指定路由组中的所有PDU路由 */
        for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
            if (g_soad_context.pdu_routes[i].used) {
                g_soad_context.pdu_routes[i].enabled = true;
            }
        }
    }
    
    (void)routingGroup;
    return result;
}

SoAd_ReturnType SoAd_DisableRouting(uint16_t routingGroup)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        /* 禁用指定路由组中的所有PDU路由 */
        for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
            if (g_soad_context.pdu_routes[i].used) {
                g_soad_context.pdu_routes[i].enabled = false;
            }
        }
    }
    
    (void)routingGroup;
    return result;
}

SoAd_ReturnType SoAd_SetPduRoutingStatus(uint16_t pduId, bool status)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    int32_t idx;
    
    if (!g_soad_context.initialized) {
        result = SOAD_E_NOT_INITIALIZED;
    }
    else {
        idx = soad_find_pdu_route_by_id(pduId);
        
        if (idx < 0) {
            result = SOAD_E_PDU_NOT_FOUND;
        }
        else {
            g_soad_context.pdu_routes[idx].enabled = status;
        }
    }
    
    return result;
}

/* ============================================================================
 * 标准API映射实现
 * ============================================================================ */

void SoAd_IfTxConfirmation(uint16_t pduId)
{
    if (g_soad_context.tx_confirmation_cb != NULL) {
        g_soad_context.tx_confirmation_cb(pduId);
    }
}

void SoAd_IfRxIndication(uint16_t pduId, const uint8_t *data, uint16_t length)
{
    if (g_soad_context.rx_indication_cb != NULL) {
        g_soad_context.rx_indication_cb(pduId, data, length);
    }
}

void SoAd_TpTxConfirmation(uint16_t pduId, SoAd_ReturnType result)
{
    (void)pduId;
    (void)result;
}

Std_ReturnType SoAd_TpRxIndication(uint16_t pduId, const uint8_t *data, uint16_t length)
{
    SoAd_IfRxIndication(pduId, data, length);
    return E_OK;
}

/* ============================================================================
 * 回调注册API实现
 * ============================================================================ */

void SoAd_RegisterTxConfirmation(SoAd_IfTxConfirmation_FuncType callback)
{
    g_soad_context.tx_confirmation_cb = callback;
}

void SoAd_RegisterRxIndication(SoAd_IfRxIndication_FuncType callback)
{
    g_soad_context.rx_indication_cb = callback;
}

void SoAd_RegisterSoConModeChg(SoAd_SoConModeChg_FuncType callback)
{
    g_soad_context.mode_chg_cb = callback;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static SoAd_ReturnType soad_map_tcpip_error(tcpip_error_t err)
{
    SoAd_ReturnType result;
    
    switch (err) {
        case TCPIP_OK:
            result = SOAD_E_NO_ERROR;
            break;
        case TCPIP_ERROR_PARAM:
            result = SOAD_E_INVALID_PARAMETER;
            break;
        case TCPIP_ERROR_NO_MEMORY:
            result = SOAD_E_MEMORY_ERROR;
            break;
        case TCPIP_ERROR_TIMEOUT:
            result = SOAD_E_TIMEOUT;
            break;
        default:
            result = SOAD_E_SOCKET_ERROR;
            break;
    }
    
    return result;
}

static int32_t soad_find_free_socket_slot(void)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < SOAD_MAX_SOCKET_CONNECTIONS; i++) {
        if (!g_soad_context.sockets[i].used) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t soad_find_socket_by_id(SoAd_SoConIdType soConId)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < SOAD_MAX_SOCKET_CONNECTIONS; i++) {
        if (g_soad_context.sockets[i].used) {
            /* 注: 实际实现需要存储并比较soConId */
            idx = (int32_t)i;
            break;
        }
    }
    
    (void)soConId;
    return idx;
}

static int32_t soad_find_pdu_route_by_id(uint16_t pduId)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
        if (g_soad_context.pdu_routes[i].used &&
            g_soad_context.pdu_routes[i].config.source_pdu_id == pduId) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t soad_find_pdu_route_by_socket(SoAd_SoConIdType soConId)
{
    int32_t idx = -1;
    
    for (uint16_t i = 0; i < SOAD_MAX_PDU_ROUTES; i++) {
        if (g_soad_context.pdu_routes[i].used &&
            g_soad_context.pdu_routes[i].config.socket_id == soConId) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static SoAd_ReturnType soad_init_socket(SoAd_SocketConnectionType *socket)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    tcpip_error_t err;
    tcpip_socket_type_t type;
    
    /* 确定Socket类型 */
    if (socket->config.socket_type == SOAD_SOCKET_TYPE_STREAM) {
        type = TCPIP_SOCK_STREAM;
    }
    else {
        type = TCPIP_SOCK_DGRAM;
    }
    
    /* 创建Socket */
    err = tcpip_socket_create(type, &socket->socket_id);
    if (err != TCPIP_OK) {
        result = soad_map_tcpip_error(err);
    }
    else {
        /* 初始化状态 */
        socket->state.state = SOAD_SOCKET_STATE_INITIALIZED;
        socket->rx_offset = 0;
        socket->tx_offset = 0;
        socket->tx_in_progress = false;
        
        /* 设置Socket选项 */
        /* 注: Keep-alive、Nagle等选项需要在tcpip_socket层实现 */
    }
    
    return result;
}

static SoAd_ReturnType soad_connect_tcp(SoAd_SocketConnectionType *socket)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    tcpip_error_t err;
    tcpip_sockaddr_t remote_addr;
    
    /* 设置远程地址 */
    remote_addr.addr = socket->config.remote_ip;
    remote_addr.port = socket->config.remote_port;
    
    /* 尝试连接 */
    err = tcpip_tcp_connect(socket->socket_id, &remote_addr);
    
    if (err == TCPIP_OK) {
        socket->state.state = SOAD_SOCKET_STATE_CONNECTING;
        socket->state.is_server = false;
    }
    else if (err == TCPIP_ERROR_IN_PROGRESS) {
        socket->state.state = SOAD_SOCKET_STATE_CONNECTING;
        result = SOAD_E_NO_ERROR; /* 连接进行中 */
    }
    else {
        result = soad_map_tcpip_error(err);
    }
    
    return result;
}

static SoAd_ReturnType soad_listen_tcp(SoAd_SocketConnectionType *socket)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    tcpip_error_t err;
    tcpip_sockaddr_t local_addr;
    
    /* 绑定本地地址 */
    local_addr.addr = socket->config.local_ip;
    local_addr.port = socket->config.local_port;
    
    err = tcpip_socket_bind(socket->socket_id, &local_addr);
    if (err != TCPIP_OK) {
        result = soad_map_tcpip_error(err);
    }
    else {
        /* 开始监听 */
        err = tcpip_tcp_listen(socket->socket_id, 5); /* backlog = 5 */
        if (err != TCPIP_OK) {
            result = soad_map_tcpip_error(err);
        }
        else {
            socket->state.state = SOAD_SOCKET_STATE_LISTENING;
            socket->state.is_server = true;
        }
    }
    
    return result;
}

static SoAd_ReturnType soad_process_rx(SoAd_SocketConnectionType *socket)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    tcpip_error_t err;
    tcpip_sockaddr_t src_addr;
    uint16_t recv_len = SOAD_SOCKET_RX_BUFFER_SIZE;
    
    if (socket->config.socket_type == SOAD_SOCKET_TYPE_DGRAM) {
        /* UDP接收 */
        err = tcpip_udp_recvfrom(socket->socket_id, &src_addr,
                                  socket->rx_buffer, &recv_len, 0);
    }
    else {
        /* TCP接收 */
        err = tcpip_tcp_receive(socket->socket_id, socket->rx_buffer, 
                                &recv_len, 0);
    }
    
    if (err == TCPIP_OK && recv_len > 0) {
        socket->state.bytes_received += recv_len;
        socket->state.last_activity = 0; /* 更新活动时间 */
        
        /* 路由到上层 */
        int32_t route_idx = soad_find_pdu_route_by_socket(
            (SoAd_SoConIdType)(socket - g_soad_context.sockets));
        
        if (route_idx >= 0) {
            uint16_t pdu_id = g_soad_context.pdu_routes[route_idx].config.dest_pdu_id;
            result = soad_route_pdu_to_upper(pdu_id, socket->rx_buffer, recv_len);
            
            if (result == SOAD_E_NO_ERROR) {
                g_soad_context.pdu_routes[route_idx].rx_count++;
            }
        }
    }
    
    return result;
}

static SoAd_ReturnType soad_process_tx(SoAd_SocketConnectionType *socket)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    tcpip_error_t err;
    uint16_t remaining = SOAD_SOCKET_TX_BUFFER_SIZE - socket->tx_offset;
    uint16_t sent_len = remaining;
    
    if (socket->config.socket_type == SOAD_SOCKET_TYPE_DGRAM) {
        /* UDP发送 */
        tcpip_sockaddr_t dest_addr;
        dest_addr.addr = socket->config.remote_ip;
        dest_addr.port = socket->config.remote_port;
        
        err = tcpip_udp_sendto(socket->socket_id, &dest_addr,
                               &socket->tx_buffer[socket->tx_offset], sent_len);
    }
    else {
        /* TCP发送 */
        err = tcpip_tcp_send(socket->socket_id, 
                             &socket->tx_buffer[socket->tx_offset], &sent_len);
    }
    
    if (err == TCPIP_OK) {
        socket->tx_offset += sent_len;
        socket->state.bytes_sent += sent_len;
        
        if (socket->tx_offset >= SOAD_SOCKET_TX_BUFFER_SIZE) {
            /* 发送完成 */
            socket->tx_in_progress = false;
            socket->tx_offset = 0;
            
            /* 触发发送确认 */
            int32_t route_idx = soad_find_pdu_route_by_socket(
                (SoAd_SoConIdType)(socket - g_soad_context.sockets));
            if (route_idx >= 0) {
                uint16_t pdu_id = g_soad_context.pdu_routes[route_idx].config.source_pdu_id;
                SoAd_IfTxConfirmation(pdu_id);
            }
        }
    }
    else {
        result = soad_map_tcpip_error(err);
    }
    
    return result;
}

static SoAd_ReturnType soad_route_pdu_to_upper(uint16_t pduId, const uint8_t *data, 
                                                uint16_t length)
{
    SoAd_ReturnType result = SOAD_E_NO_ERROR;
    
    if (g_soad_context.rx_indication_cb != NULL) {
        g_soad_context.rx_indication_cb(pduId, data, length);
    }
    
    return result;
}

static void soad_handle_socket_state_change(SoAd_SocketConnectionType *socket,
                                             SoAd_SocketStateType new_state)
{
    socket->state.state = new_state;
    
    if (g_soad_context.mode_chg_cb != NULL) {
        SoAd_SoConIdType soConId = (SoAd_SoConIdType)(socket - g_soad_context.sockets);
        g_soad_context.mode_chg_cb(soConId, new_state);
    }
}
