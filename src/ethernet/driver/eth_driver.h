/**
 * @file eth_driver.h
 * @brief 以太网MAC驱动核心接口
 * @version 1.0
 * @date 2026-04-24
 */

#ifndef ETH_DRIVER_H
#define ETH_DRIVER_H

#include "../../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 以太网驱动API接口
 * ============================================================================ */

/**
 * @brief 初始化以太网驱动
 * @param config 驱动配置参数
 * @return ETH_OK成功，其他失败
 */
eth_status_t eth_driver_init(const eth_config_t *config);

/**
 * @brief 反初始化以太网驱动
 */
void eth_driver_deinit(void);

/**
 * @brief 发送数据帧
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功，ETH_TIMEOUT超时，其他失败
 */
eth_status_t eth_driver_send(const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 接收数据帧 (轮询方式)
 * @param buffer 接收缓冲区
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功，ETH_TIMEOUT超时，其他失败
 */
eth_status_t eth_driver_receive(eth_buffer_t *buffer, uint32_t timeout_ms);

/**
 * @brief 注册接收回调函数 (中断方式)
 * @param callback 回调函数指针
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_driver_register_rx_callback(eth_rx_callback_t callback, void *user_data);

/**
 * @brief 注册连接状态变化回调
 * @param callback 回调函数指针
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_driver_register_link_callback(eth_link_callback_t callback, void *user_data);

/**
 * @brief 获取当前连接状态
 * @return 连接状态
 */
eth_link_status_t eth_driver_get_link_status(void);

/**
 * @brief 获取MAC地址
 * @param mac_addr 输出MAC地址缓冲区
 * @return ETH_OK成功
 */
eth_status_t eth_driver_get_mac_addr(eth_mac_addr_t mac_addr);

/**
 * @brief 设置MAC地址
 * @param mac_addr 新的MAC地址
 * @return ETH_OK成功
 */
eth_status_t eth_driver_set_mac_addr(const eth_mac_addr_t mac_addr);

/**
 * @brief 启用以太网控制器
 * @return ETH_OK成功
 */
eth_status_t eth_driver_start(void);

/**
 * @brief 停用以太网控制器
 * @return ETH_OK成功
 */
eth_status_t eth_driver_stop(void);

/**
 * @brief 获取驱动统计信息
 * @param tx_packets 发送包数量输出
 * @param rx_packets 接收包数量输出
 * @param tx_errors 发送错误数输出
 * @param rx_errors 接收错误数输出
 * @return ETH_OK成功
 */
eth_status_t eth_driver_get_stats(uint64_t *tx_packets, uint64_t *rx_packets,
                                   uint64_t *tx_errors, uint64_t *rx_errors);

/**
 * @brief 清除驱动统计信息
 */
void eth_driver_clear_stats(void);

/* ============================================================================
 * PHY管理API
 * ============================================================================ */

/**
 * @brief 初始化PHY芯片
 * @param phy_addr PHY MDIO地址
 * @return ETH_OK成功
 */
eth_status_t eth_phy_init(uint8_t phy_addr);

/**
 * @brief 读取PHY寄存器
 * @param phy_addr PHY地址
 * @param reg_addr 寄存器地址
 * @param value 输出值
 * @return ETH_OK成功
 */
eth_status_t eth_phy_read_reg(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value);

/**
 * @brief 写入PHY寄存器
 * @param phy_addr PHY地址
 * @param reg_addr 寄存器地址
 * @param value 写入值
 * @return ETH_OK成功
 */
eth_status_t eth_phy_write_reg(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);

/**
 * @brief 启用PHY自动协商
 * @return ETH_OK成功
 */
eth_status_t eth_phy_auto_negotiation(void);

/**
 * @brief 获取PHY连接状态
 * @return 连接状态
 */
eth_link_status_t eth_phy_get_link_status(void);

/**
 * @brief 获取PHY当前速率模式
 * @param mode 输出速率模式
 * @return ETH_OK成功
 */
eth_status_t eth_phy_get_speed_mode(eth_mode_t *mode);

#ifdef __cplusplus
}
#endif

#endif /* ETH_DRIVER_H */
