/**
 * @file transport_interface.c
 * @brief 传输层通用API实现
 * @version 1.0
 * @date 2026-04-24
 */

#include "transport_interface.h"
#include <stdlib.h>

/* ============================================================================
 * 传输层通用API实现
 * ============================================================================ */

transport_handle_t* transport_create(transport_type_t type, const transport_config_t *config)
{
    /* 这个函数由具体传输实现提供，
     * 这里提供一个统一的入口点 */
    (void)type;
    (void)config;
    return NULL;
}

eth_status_t transport_destroy(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (handle->ops != NULL && handle->ops->deinit != NULL) {
        return handle->ops->deinit(handle);
    }
    
    return ETH_ERROR;
}

eth_status_t transport_send(transport_handle_t *handle, const uint8_t *data, 
                            uint32_t len, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    if (handle->ops != NULL && handle->ops->send != NULL) {
        return handle->ops->send(handle, data, len, timeout_ms);
    }
    
    return ETH_ERROR;
}

eth_status_t transport_receive(transport_handle_t *handle, uint8_t *buffer, 
                               uint32_t max_len, uint32_t *received_len, 
                               uint32_t timeout_ms)
{
    if (handle == NULL || buffer == NULL || received_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (handle->ops != NULL && handle->ops->receive != NULL) {
        return handle->ops->receive(handle, buffer, max_len, received_len, timeout_ms);
    }
    
    return ETH_ERROR;
}

eth_status_t transport_register_callback(transport_handle_t *handle, 
                                         eth_rx_callback_t callback, 
                                         void *user_data)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (handle->ops != NULL && handle->ops->register_callback != NULL) {
        return handle->ops->register_callback(handle, callback, user_data);
    }
    
    return ETH_ERROR;
}

eth_status_t transport_get_status(transport_handle_t *handle, uint32_t *status)
{
    if (handle == NULL || status == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (handle->ops != NULL && handle->ops->get_status != NULL) {
        return handle->ops->get_status(handle, status);
    }
    
    return ETH_ERROR;
}

/* ============================================================================
 * 具体传输实现初始化函数声明
 * ============================================================================ */

/* 这些函数在各自的传输层源文件中实现 */
extern eth_status_t udp_transport_init(void);
extern eth_status_t tcp_transport_init(void);
extern eth_status_t shm_transport_init(void);
