/**
 * @file transport_interface.h
 * @brief 传输层抽象接口定义
 * @version 1.0
 * @date 2026-04-24
 */

#ifndef TRANSPORT_INTERFACE_H
#define TRANSPORT_INTERFACE_H

#include "../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 传输层类型定义
 * ============================================================================ */

/** 传输协议类型 */
typedef enum {
    TRANSPORT_UDP = 0,      /* UDP传输 */
    TRANSPORT_TCP,          /* TCP传输 */
    TRANSPORT_SHM,          /* 共享内存传输 */
} transport_type_t;

/** 传输层配置 */
typedef struct {
    transport_type_t type;
    eth_ip_addr_t local_addr;
    uint16_t local_port;
    eth_ip_addr_t remote_addr;
    uint16_t remote_port;
    uint32_t tx_buffer_size;
    uint32_t rx_buffer_size;
    uint32_t timeout_ms;
} transport_config_t;

/** 传输层操作函数表前向声明 */
typedef struct transport_ops transport_ops_t;

/** 传输层句柄结构体 */
typedef struct transport_handle {
    const transport_ops_t *ops;
    void *private_data;
} transport_handle_t;

/** 传输层接口函数表 */
struct transport_ops {
    const char *name;
    eth_status_t (*init)(transport_handle_t *handle, const transport_config_t *config);
    eth_status_t (*deinit)(transport_handle_t *handle);
    eth_status_t (*send)(transport_handle_t *handle, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
    eth_status_t (*receive)(transport_handle_t *handle, uint8_t *buffer, uint32_t max_len, uint32_t *received_len, uint32_t timeout_ms);
    eth_status_t (*register_callback)(transport_handle_t *handle, eth_rx_callback_t callback, void *user_data);
    eth_status_t (*get_status)(transport_handle_t *handle, uint32_t *status);
};

/* ============================================================================
 * 传输层通用API
 * ============================================================================ */

/**
 * @brief 创建传输层实例
 * @param type 传输类型
 * @param config 配置参数
 * @return 传输层句柄，NULL表示失败
 */
transport_handle_t* transport_create(transport_type_t type, const transport_config_t *config);

/**
 * @brief 销毁传输层实例
 * @param handle 传输层句柄
 * @return ETH_OK成功
 */
eth_status_t transport_destroy(transport_handle_t *handle);

/**
 * @brief 发送数据
 * @param handle 传输层句柄
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t transport_send(transport_handle_t *handle, const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 接收数据
 * @param handle 传输层句柄
 * @param buffer 接收缓冲区
 * @param max_len 缓冲区大小
 * @param received_len 实际接收长度输出
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t transport_receive(transport_handle_t *handle, uint8_t *buffer, uint32_t max_len,
                                uint32_t *received_len, uint32_t timeout_ms);

/**
 * @brief 注册接收回调
 * @param handle 传输层句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t transport_register_callback(transport_handle_t *handle, eth_rx_callback_t callback, void *user_data);

/**
 * @brief 获取传输层状态
 * @param handle 传输层句柄
 * @param status 状态值输出
 * @return ETH_OK成功
 */
eth_status_t transport_get_status(transport_handle_t *handle, uint32_t *status);

/* ============================================================================
 * 具体传输实现初始化
 * ============================================================================ */

/**
 * @brief 初始化UDP传输实现
 * @return ETH_OK成功
 */
eth_status_t udp_transport_init(void);

/**
 * @brief 初始化TCP传输实现
 * @return ETH_OK成功
 */
eth_status_t tcp_transport_init(void);

/**
 * @brief 初始化共享内存传输实现
 * @return ETH_OK成功
 */
eth_status_t shm_transport_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_INTERFACE_H */
