/**
 * @file mock_eth_driver.h
 * @brief 以太网驱动模拟头文件 - 用于单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#ifndef MOCK_ETH_DRIVER_H
#define MOCK_ETH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 常量定义
 *==============================================================================*/

#define MOCK_ETH_MAX_PACKET_SIZE    1522
#define MOCK_ETH_MIN_PACKET_SIZE    64
#define MOCK_ETH_MTU                1500

/*==============================================================================
 * 类型定义
 *==============================================================================*/

typedef void* mock_eth_handle_t;

typedef struct {
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t tx_errors;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t rx_errors;
    uint64_t rx_dropped;
    uint32_t tx_queue_count;
    uint32_t rx_queue_count;
} mock_eth_stats_t;

/*==============================================================================
 * 驱动API声明
 *==============================================================================*/

mock_eth_handle_t mock_eth_driver_init(const eth_config_t* config);
eth_status_t mock_eth_driver_deinit(mock_eth_handle_t handle);
eth_status_t mock_eth_driver_start(mock_eth_handle_t handle);
eth_status_t mock_eth_driver_stop(mock_eth_handle_t handle);

eth_status_t mock_eth_send_packet(mock_eth_handle_t handle,
                                   const eth_mac_addr_t dst_addr,
                                   const uint8_t* data,
                                   uint32_t length);
eth_status_t mock_eth_send_vlan_packet(mock_eth_handle_t handle,
                                        const eth_mac_addr_t dst_addr,
                                        uint16_t vlan_id,
                                        uint8_t priority,
                                        const uint8_t* data,
                                        uint32_t length);
eth_status_t mock_eth_receive_packet(mock_eth_handle_t handle,
                                      uint8_t* buffer,
                                      uint32_t buffer_size,
                                      uint32_t* received_length,
                                      eth_mac_addr_t src_addr);

eth_status_t mock_eth_set_link_status(mock_eth_handle_t handle,
                                       eth_link_status_t status);
eth_status_t mock_eth_get_link_status(mock_eth_handle_t handle,
                                       eth_link_status_t* status);

eth_status_t mock_eth_register_rx_callback(mock_eth_handle_t handle,
                                            eth_rx_callback_t callback,
                                            void* user_data);
eth_status_t mock_eth_register_link_callback(mock_eth_handle_t handle,
                                              eth_link_callback_t callback,
                                              void* user_data);

/*==============================================================================
 * 模拟控制API
 *==============================================================================*/

void mock_eth_set_latency(uint32_t min_us, uint32_t max_us);
void mock_eth_set_packet_loss(uint32_t rate_x10000);
void mock_eth_set_error_rate(uint32_t rate_x10000);
void mock_eth_enable_vlan(bool enable);

eth_status_t mock_eth_inject_rx_packet(mock_eth_handle_t handle,
                                        const eth_mac_addr_t src_addr,
                                        const uint8_t* data,
                                        uint32_t length);

/*==============================================================================
 * 统计API
 *==============================================================================*/

eth_status_t mock_eth_get_stats(mock_eth_handle_t handle,
                                 mock_eth_stats_t* stats);
void mock_eth_reset_stats(mock_eth_handle_t handle);

/*==============================================================================
 * 模拟网络API
 *==============================================================================*/

eth_status_t mock_eth_forward_between_ports(uint32_t src_port_id,
                                             uint32_t dst_port_id,
                                             const uint8_t* data,
                                             uint32_t length);
void mock_eth_reset_all(void);
uint32_t mock_eth_get_port_count(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_ETH_DRIVER_H */
