/**
 * @file tcpip_core.h
 * @brief TCP/IP核心 - IP层与路由管理
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 */

#ifndef TCPIP_CORE_H
#define TCPIP_CORE_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 初始化与管理API
 * ============================================================================ */

/**
 * @brief 初始化TCP/IP协议栈
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_init(void);

/**
 * @brief 反初始化TCP/IP协议栈
 */
void tcpip_deinit(void);

/**
 * @brief 主循环处理
 * @param elapsed_ms 经过的毫秒数
 */
void tcpip_main_function(uint32_t elapsed_ms);

/**
 * @brief 获取TCP/IP版本信息
 * @param major 主版本输出
 * @param minor 次版本输出
 * @param patch 补丁版本输出
 */
void tcpip_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

/* ============================================================================
 * 网络接口管理API
 * ============================================================================ */

/**
 * @brief 添加网络接口
 * @param config 接口配置
 * @param iface_id 接口ID输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_add_interface(const tcpip_iface_config_t *config, 
                                   uint8_t *iface_id);

/**
 * @brief 移除网络接口
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_remove_interface(uint8_t iface_id);

/**
 * @brief 启用网络接口
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_iface_up(uint8_t iface_id);

/**
 * @brief 禁用网络接口
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_iface_down(uint8_t iface_id);

/**
 * @brief 获取接口信息
 * @param iface_id 接口ID
 * @param info 接口信息输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_get_iface_info(uint8_t iface_id, tcpip_iface_info_t *info);

/**
 * @brief 注册接口状态变化回调
 * @param iface_id 接口ID
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_register_state_callback(uint8_t iface_id,
                                             tcpip_state_callback_t callback,
                                             void *user_data);

/**
 * @brief 更新接口MAC地址
 * @param iface_id 接口ID
 * @param mac_addr 新MAC地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_set_mac_addr(uint8_t iface_id, const uint8_t *mac_addr);

/**
 * @brief 获取接口MAC地址
 * @param iface_id 接口ID
 * @param mac_addr MAC地址输出缓冲区
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_get_mac_addr(uint8_t iface_id, uint8_t *mac_addr);

/* ============================================================================
 * IP地址管理API
 * ============================================================================ */

/**
 * @brief 配置静态IP地址
 * @param iface_id 接口ID
 * @param ip_config IP配置
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_set_ip_config(uint8_t iface_id, 
                                   const tcpip_ip_config_t *ip_config);

/**
 * @brief 获取IP地址配置
 * @param iface_id 接口ID
 * @param ip_config IP配置输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_get_ip_config(uint8_t iface_id, tcpip_ip_config_t *ip_config);

/**
 * @brief 解析IP地址字符串
 * @param ip_str IP地址字符串 (e.g., "192.168.1.1")
 * @param ip_addr IP地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_ip_from_string(const char *ip_str, tcpip_ip_addr_t *ip_addr);

/**
 * @brief 将IP地址转换为字符串
 * @param ip_addr IP地址
 * @param ip_str 输出缓冲区 (至少16字节)
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_ip_to_string(tcpip_ip_addr_t ip_addr, char *ip_str);

/**
 * @brief 计算子网网络地址
 * @param ip_addr IP地址
 * @param netmask 子网掩码
 * @return 网络地址
 */
tcpip_ip_addr_t tcpip_get_network_addr(tcpip_ip_addr_t ip_addr, 
                                        tcpip_ip_addr_t netmask);

/**
 * @brief 计算广播地址
 * @param ip_addr IP地址
 * @param netmask 子网掩码
 * @return 广播地址
 */
tcpip_ip_addr_t tcpip_get_broadcast_addr(tcpip_ip_addr_t ip_addr, 
                                          tcpip_ip_addr_t netmask);

/* ============================================================================
 * 路由表管理API
 * ============================================================================ */

/**
 * @brief 添加路由表项
 * @param route 路由表项
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_add_route(const tcpip_route_entry_t *route);

/**
 * @brief 删除路由表项
 * @param dest_addr 目的网络地址
 * @param netmask 子网掩码
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_del_route(tcpip_ip_addr_t dest_addr, 
                               tcpip_ip_addr_t netmask);

/**
 * @brief 查找路由
 * @param dest_addr 目的IP地址
 * @param route 路由表项输出
 * @return TCPIP_OK找到路由
 */
tcpip_error_t tcpip_find_route(tcpip_ip_addr_t dest_addr, 
                                tcpip_route_entry_t *route);

/**
 * @brief 清除所有路由表项
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_clear_routes(void);

/**
 * @brief 获取路由表大小
 * @return 活动路由数
 */
uint8_t tcpip_get_route_count(void);

/* ============================================================================
 * IP包收发API
 * ============================================================================ */

/**
 * @brief 发送IP数据包
 * @param iface_id 出口接口ID
 * @param dest_addr 目的IP地址
 * @param protocol 协议类型
 * @param data 数据指针
 * @param len 数据长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_send_ip_packet(uint8_t iface_id,
                                    tcpip_ip_addr_t dest_addr,
                                    tcpip_ip_protocol_t protocol,
                                    const uint8_t *data,
                                    uint16_t len);

/**
 * @brief 处理接收的IP数据包
 * @param iface_id 入口接口ID
 * @param packet 数据包指针
 * @param len 数据长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_process_ip_packet(uint8_t iface_id,
                                       const uint8_t *packet,
                                       uint16_t len);

/**
 * @brief 注册原始IP包接收回调 (用于特殊处理)
 * @param protocol 协议类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_register_raw_handler(tcpip_ip_protocol_t protocol,
                                          tcpip_rx_callback_t callback,
                                          void *user_data);

/**
 * @brief 注销原始IP包处理
 * @param protocol 协议类型
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_unregister_raw_handler(tcpip_ip_protocol_t protocol);

/* ============================================================================
 * 校验和辅助函数
 * ============================================================================ */

/**
 * @brief 计算IP校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return 校验和
 */
uint16_t tcpip_calc_checksum(const uint8_t *data, uint16_t len);

/**
 * @brief 计算IP伪头部校验和
 * @param ip_header IP头部指针
 * @return 校验和
 */
uint16_t tcpip_calc_ip_header_checksum(const tcpip_ip_header_t *ip_header);

/**
 * @brief 验证IP头部校验和
 * @param ip_header IP头部指针
 * @return true有效，false无效
 */
bool tcpip_verify_ip_checksum(const tcpip_ip_header_t *ip_header);

/**
 * @brief 计算TCP/UDP伪头部校验和
 * @param src_addr 源IP地址
 * @param dst_addr 目的IP地址
 * @param protocol 协议
 * @param payload 载荷数据
 * @param len 载荷长度
 * @return 校验和
 */
uint16_t tcpip_calc_transport_checksum(tcpip_ip_addr_t src_addr,
                                        tcpip_ip_addr_t dst_addr,
                                        tcpip_ip_protocol_t protocol,
                                        const uint8_t *payload,
                                        uint16_t len);

/**
 * @brief 获取数据包源MAC地址 (用于ARP)
 * @param iface_id 接口ID
 * @param src_mac MAC地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_get_src_mac(uint8_t iface_id, uint8_t *src_mac);

/**
 * @brief 通过IP查找接口
 * @param ip_addr IP地址
 * @param iface_id 接口ID输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_find_iface_by_ip(tcpip_ip_addr_t ip_addr, uint8_t *iface_id);

/**
 * @brief 检查IP地址是否是本地地址
 * @param ip_addr IP地址
 * @param iface_id 接口ID
 * @return true是本地地址
 */
bool tcpip_is_local_addr(tcpip_ip_addr_t ip_addr, uint8_t iface_id);

/**
 * @brief 获取适合发送到目标地址的接口
 * @param dest_addr 目标IP地址
 * @param iface_id 接口ID输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_get_outgoing_iface(tcpip_ip_addr_t dest_addr, 
                                        uint8_t *iface_id);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_CORE_H */
