/**
 * @file eth_sm.h
 * @brief EthSM (Ethernet State Manager) 核心API
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR EthSM模块规范
 * @note MISRA C:2012 compliant
 * @note 状态机: UNINIT -> INIT -> WAIT_REQ -> READY
 */

#ifndef ETH_SM_H
#define ETH_SM_H

#include "eth_sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * EthSM生命周期API
 * ============================================================================ */

/**
 * @brief 初始化EthSM模块
 * @note 状态机转换: UNINIT -> INIT
 * @param config 模块配置 (可为NULL使用默认配置)
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_init(const eth_sm_config_t *config);

/**
 * @brief 反初始化EthSM模块
 * @note 状态机转换: 任意状态 -> UNINIT
 */
void eth_sm_deinit(void);

/**
 * @brief 获取EthSM版本信息
 * @param major 主版本输出
 * @param minor 次版本输出
 * @param patch 补丁版本输出
 */
void eth_sm_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

/* ============================================================================
 * 网络管理API
 * ============================================================================ */

/**
 * @brief 创建网络实例
 * @param config 网络配置
 * @param network_idx 网络索引输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_create_network(const eth_sm_network_config_t *config,
                                      uint8_t *network_idx);

/**
 * @brief 删除网络实例
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_delete_network(uint8_t network_idx);

/**
 * @brief 获取网络配置
 * @param network_idx 网络索引
 * @param config 配置输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_network_config(uint8_t network_idx,
                                          eth_sm_network_config_t *config);

/* ============================================================================
 * 状态机控制API
 * ============================================================================ */

/**
 * @brief 启动网络通讯
 * @note 状态机转换: WAIT_REQ -> READY (ComM请求)
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_request_com_mode(uint8_t network_idx);

/**
 * @brief 停止网络通讯
 * @note 状态机转换: READY -> WAIT_REQ (ComM释放)
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_release_com_mode(uint8_t network_idx);

/**
 * @brief 设置网络模式
 * @param network_idx 网络索引
 * @param mode 目标模式
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_set_network_mode(uint8_t network_idx, 
                                        eth_sm_mode_t mode);

/**
 * @brief 获取网络模式
 * @param network_idx 网络索引
 * @param mode 模式输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_network_mode(uint8_t network_idx,
                                        eth_sm_mode_t *mode);

/**
 * @brief 获取状态机状态
 * @param network_idx 网络索引
 * @param state 状态输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_state(uint8_t network_idx, 
                                 eth_sm_state_t *state);

/**
 * @brief 检查网络是否就绪
 * @param network_idx 网络索引
 * @param ready 就绪状态输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_is_network_ready(uint8_t network_idx, bool *ready);

/* ============================================================================
 * 链路状态API
 * ============================================================================ */

/**
 * @brief 通知链路状态变化
 * @note 由以太网驱动调用
 * @param network_idx 网络索引
 * @param link_state 新链路状态
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_link_state_change(uint8_t network_idx,
                                         eth_sm_link_state_t link_state);

/**
 * @brief 获取链路状态
 * @param network_idx 网络索引
 * @param link_state 链路状态输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_link_state(uint8_t network_idx,
                                      eth_sm_link_state_t *link_state);

/**
 * @brief 等待链路连接
 * @param network_idx 网络索引
 * @param timeout_ms 超时(毫秒)
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_wait_for_link(uint8_t network_idx, uint32_t timeout_ms);

/* ============================================================================
 * 主循环处理API
 * ============================================================================ */

/**
 * @brief EthSM主循环函数
 * @note 应定期调用以处理状态转换
 * @param elapsed_ms 经过的毫秒数
 */
void eth_sm_main_function(uint32_t elapsed_ms);

/**
 * @brief 处理模式请求
 * @note 在主循环中调用
 * @param network_idx 网络索引
 */
void eth_sm_process_mode_request(uint8_t network_idx);

/**
 * @brief 检查状态转换条件
 * @param network_idx 网络索引
 * @return ETHSM_OK可以转换
 */
eth_sm_error_t eth_sm_check_transitions(uint8_t network_idx);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

/**
 * @brief 注册模式变化回调
 * @param network_idx 网络索引 (0xFF = 所有网络)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_register_mode_indication(uint8_t network_idx,
                                                eth_sm_mode_indication_cb_t callback,
                                                void *user_data);

/**
 * @brief 注销模式变化回调
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_unregister_mode_indication(uint8_t network_idx);

/**
 * @brief 注册状态变化回调
 * @param network_idx 网络索引 (0xFF = 所有网络)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_register_state_change(uint8_t network_idx,
                                             eth_sm_state_change_cb_t callback,
                                             void *user_data);

/**
 * @brief 注销状态变化回调
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_unregister_state_change(uint8_t network_idx);

/**
 * @brief 注册链路变化回调
 * @param network_idx 网络索引 (0xFF = 所有网络)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_register_link_change(uint8_t network_idx,
                                            eth_sm_link_change_cb_t callback,
                                            void *user_data);

/**
 * @brief 注销链路变化回调
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_unregister_link_change(uint8_t network_idx);

/* ============================================================================
 * BswM集成API
 * ============================================================================ */

/**
 * @brief BswM请求模式改变
 * @note BswM_ModeSwitchPort接口
 * @param network_idx 网络索引
 * @param mode 请求的模式
 * @return ETHSM_OK接受请求
 */
eth_sm_error_t eth_sm_bswm_request_mode(uint8_t network_idx,
                                         eth_sm_mode_t mode);

/**
 * @brief BswM查询当前模式
 * @note BswM_ModePort接口
 * @param network_idx 网络索引
 * @param mode 当前模式输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_bswm_get_current_mode(uint8_t network_idx,
                                             eth_sm_mode_t *mode);

/**
 * @brief 注册BswM请求回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_register_bswm_request(eth_sm_bswm_request_cb_t callback,
                                             void *user_data);

/* ============================================================================
 * EcuM集成API
 * ============================================================================ */

/**
 * @brief 通知EcuM通讯状态变化
 * @note 通讯建立/释放时调用
 * @param network_idx 网络索引
 * @param has_comm 是否有通讯
 */
void eth_sm_ecum_notify(uint8_t network_idx, bool has_comm);

/**
 * @brief 设置唤醒回调
 * @note EcuM_Wakeup接口
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_set_wakeup(uint8_t network_idx);

/**
 * @brief 清除唤醒状态
 * @param network_idx 网络索引
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_clear_wakeup(uint8_t network_idx);

/**
 * @brief 检查是否有唤醒事件
 * @param network_idx 网络索引
 * @param has_wakeup 唤醒状态输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_has_wakeup(uint8_t network_idx, bool *has_wakeup);

/* ============================================================================
 * 诊断API
 * ============================================================================ */

/**
 * @brief 获取网络状态信息
 * @param network_idx 网络索引
 * @param info 状态信息输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_network_info(uint8_t network_idx,
                                        eth_sm_network_info_t *info);

/**
 * @brief 获取最后状态转换信息
 * @param network_idx 网络索引
 * @param trans 转换信息输出
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_get_last_transition(uint8_t network_idx,
                                           eth_sm_transition_info_t *trans);

/**
 * @brief 获取所有网络数量
 * @return 网络数量
 */
uint8_t eth_sm_get_network_count(void);

/**
 * @brief 获取活动网络数量
 * @return 活动网络数量
 */
uint8_t eth_sm_get_active_network_count(void);

/**
 * @brief 获取就绪网络数量
 * @return 就绪网络数量
 */
uint8_t eth_sm_get_ready_network_count(void);

/* ============================================================================
 * 调试与诊断API
 * ============================================================================ */

/**
 * @brief 输出调试信息
 * @param network_idx 网络索引
 */
void eth_sm_print_debug_info(uint8_t network_idx);

/**
 * @brief 验证配置
 * @param config 配置指针
 * @return ETHSM_OK配置有效
 */
eth_sm_error_t eth_sm_validate_config(const eth_sm_config_t *config);

/**
 * @brief 检查是否已初始化
 * @return true已初始化
 */
bool eth_sm_is_initialized(void);

/**
 * @brief 强制状态转换 (调试用)
 * @param network_idx 网络索引
 * @param state 目标状态
 * @return ETHSM_OK成功
 */
eth_sm_error_t eth_sm_force_state(uint8_t network_idx, eth_sm_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* ETH_SM_H */
