/**
 * @file eth_sm_types.h
 * @brief EthSM (Ethernet State Manager) 核心类型定义
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR EthSM模块规范
 * @note MISRA C:2012 compliant
 */

#ifndef ETH_SM_TYPES_H
#define ETH_SM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * EthSM版本信息
 * ============================================================================ */

#define ETHSM_MAJOR_VERSION     1u
#define ETHSM_MINOR_VERSION     0u
#define ETHSM_PATCH_VERSION     0u

#define ETHSM_MAX_NETWORKS      4u      /**< 最大网络数量 */

/* ============================================================================
 * EthSM网络状态定义 (AUTOSAR标准)
 * ============================================================================ */

/**
 * @brief EthSM状态机状态
 * @note 符合AUTOSAR EthSM_StateType定义
 */
typedef enum {
    ETHSM_STATE_UNINIT = 0u,        /**< 未初始化 */
    ETHSM_STATE_INIT,               /**< 已初始化 */
    ETHSM_STATE_WAIT_REQ,           /**< 等待请求 */
    ETHSM_STATE_READY               /**< 就绪 */
} eth_sm_state_t;

/**
 * @brief EthSM网络模式
 * @note 符合AUTOSAR EthSM_NetworkModeStateType定义
 */
typedef enum {
    ETHSM_MODE_ACTIVE = 0u,         /**< 活动模式 */
    ETHSM_MODE_NO_COMM,             /**< 无通讯 */
    ETHSM_MODE_FULL_COMM            /**< 完全通讯 */
} eth_sm_mode_t;

/**
 * @brief 连接状态
 */
typedef enum {
    ETHSM_LINK_OFF = 0u,            /**< 链路断开 */
    ETHSM_LINK_ON                   /**< 链路连接 */
} eth_sm_link_state_t;

/**
 * @brief 通讯请求状态
 */
typedef enum {
    ETHSM_COMM_NONE = 0u,           /**< 无通讯请求 */
    ETHSM_COMM_REQUESTED,           /**< 通讯已请求 */
    ETHSM_COMM_RELEASED             /**< 通讯已释放 */
} eth_sm_comm_state_t;

/* ============================================================================
 * EthSM错误码定义
 * ============================================================================ */

/**
 * @brief EthSM错误码
 */
typedef enum {
    ETHSM_OK = 0u,                  /**< 成功 */
    ETHSM_E_NOT_OK,                 /**< 通用错误 */
    ETHSM_E_PARAM_POINTER,          /**< 空指针参数 */
    ETHSM_E_PARAM_CONFIG,           /**< 配置参数错误 */
    ETHSM_E_INV_NETWORK_IDX,        /**< 无效网络索引 */
    ETHSM_E_INV_MODE,               /**< 无效模式 */
    ETHSM_E_NOT_INITIALIZED,        /**< 未初始化 */
    ETHSM_E_ALREADY_INITIALIZED,    /**< 已初始化 */
    ETHSM_E_NO_RESOURCE             /**< 资源不足 */
} eth_sm_error_t;

/* ============================================================================
 * EthSM网络配置类型
 * ============================================================================ */

/**
 * @brief 触发模式切换的ComM用户
 */
typedef uint8_t eth_sm_com_user_t;

/**
 * @brief 网络配置结构
 */
typedef struct {
    uint8_t network_idx;                /**< 网络索引 */
    uint8_t eth_if_controller;          /**< 以太网控制器ID */
    bool start_all_channels;            /**< 启动时设置所有通道 */
    uint32_t startup_delay_ms;          /**< 启动延迟 */
    uint32_t shutdown_delay_ms;         /**< 关闭延迟 */
    eth_sm_com_user_t com_users[4];     /**< ComM用户列表 */
    uint8_t com_user_count;             /**< ComM用户数量 */
} eth_sm_network_config_t;

/**
 * @brief EthSM模块配置
 */
typedef struct {
    const eth_sm_network_config_t *networks;    /**< 网络配置数组 */
    uint8_t network_count;                      /**< 网络数量 */
    bool development_error_detection;           /**< 开启调试错误检测 */
} eth_sm_config_t;

/* ============================================================================
 * EthSM状态信息类型
 * ============================================================================ */

/**
 * @brief 网络状态信息
 */
typedef struct {
    uint8_t network_idx;                /**< 网络索引 */
    eth_sm_state_t sm_state;            /**< 状态机状态 */
    eth_sm_mode_t current_mode;         /**< 当前模式 */
    eth_sm_link_state_t link_state;     /**< 链路状态 */
    eth_sm_comm_state_t comm_state;     /**< 通讯状态 */
    uint32_t trans_request_count;       /**< 状态转换请求数 */
    uint32_t mode_indication_count;     /**< 模式指示数 */
    uint32_t link_change_count;         /**< 链路变化数 */
} eth_sm_network_info_t;

/**
 * @brief 状态机转换信息
 */
typedef struct {
    uint8_t network_idx;
    eth_sm_state_t from_state;
    eth_sm_state_t to_state;
    uint32_t timestamp_ms;
} eth_sm_transition_info_t;

/* ============================================================================
 * 回调函数类型定义
 * ============================================================================ */

/**
 * @brief 模式变化回调函数类型
 * @param network_idx 网络索引
 * @param mode 新模式
 * @param user_data 用户数据
 */
typedef void (*eth_sm_mode_indication_cb_t)(uint8_t network_idx,
                                             eth_sm_mode_t mode,
                                             void *user_data);

/**
 * @brief 状态变化回调函数类型
 * @param network_idx 网络索引
 * @param old_state 旧状态
 * @param new_state 新状态
 * @param user_data 用户数据
 */
typedef void (*eth_sm_state_change_cb_t)(uint8_t network_idx,
                                          eth_sm_state_t old_state,
                                          eth_sm_state_t new_state,
                                          void *user_data);

/**
 * @brief 链路状态变化回调函数类型
 * @param network_idx 网络索引
 * @param link_state 链路状态
 * @param user_data 用户数据
 */
typedef void (*eth_sm_link_change_cb_t)(uint8_t network_idx,
                                         eth_sm_link_state_t link_state,
                                         void *user_data);

/**
 * @brief BswM请求模式改变回调
 * @param network_idx 网络索引
 * @param mode 请求的模式
 * @return 是否接受请求
 */
typedef bool (*eth_sm_bswm_request_cb_t)(uint8_t network_idx,
                                          eth_sm_mode_t mode);

/**
 * @brief EcuM通讯状态变化回调
 * @param network_idx 网络索引
 * @param has_comm 是否有通讯
 */
typedef void (*eth_sm_ecum_notify_cb_t)(uint8_t network_idx, bool has_comm);

/* ============================================================================
 * 工具宏
 * ============================================================================ */

/** 检查网络索引有效性 */
#define ETHSM_IS_VALID_NETWORK_IDX(idx) \
    ((idx) < ETHSM_MAX_NETWORKS)

/** 状态转换宏 */
#define ETHSM_STATE_CAN_CHANGE(from, to) \
    (((from) == ETHSM_STATE_UNINIT && (to) == ETHSM_STATE_INIT) || \
     ((from) == ETHSM_STATE_INIT && (to) == ETHSM_STATE_WAIT_REQ) || \
     ((from) == ETHSM_STATE_WAIT_REQ && (to) == ETHSM_STATE_READY) || \
     ((from) == ETHSM_STATE_READY && (to) == ETHSM_STATE_WAIT_REQ) || \
     ((from) == ETHSM_STATE_WAIT_REQ && (to) == ETHSM_STATE_INIT))

/** 模式转换宏 */
#define ETHSM_MODE_CAN_CHANGE(from, to) \
    ((from) != (to))

#ifdef __cplusplus
}
#endif

#endif /* ETH_SM_TYPES_H */
