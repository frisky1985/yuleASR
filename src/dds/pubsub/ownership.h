/**
 * @file ownership.h
 * @brief DDS所有权策略 - Ownership QoS实现
 * @version 1.0
 * @date 2026-04-24
 * 
 * 车载要求：
 * - 支持EXCLUSIVE和SHARED模式
 * - 所有权强度协商
 * - 无感所有权转移
 * - ASIL-D安全等级支持
 */

#ifndef OWNERSHIP_H
#define OWNERSHIP_H

#include "../../common/types/eth_types.h"
#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/** 最大所有权主数量 */
#define OWN_MAX_OWNERS         16
/** 最大竞争者数量 */
#define OWN_MAX_CONTENDERS     8
/** 所有权协商超时(ms) */
#define OWN_NEGOTIATION_TIMEOUT_MS  100
/** 默认所有权强度 */
#define OWN_DEFAULT_STRENGTH   0
/** 最大所有权强度 */
#define OWN_MAX_STRENGTH       65535
/** 所有权确认超时(ms) */
#define OWN_ACK_TIMEOUT_MS     50

/* ============================================================================
 * 类型定义
 * ============================================================================ */

/** 所有权类型 */
typedef enum {
    OWN_TYPE_SHARED = 0,        /**< 共享模式 - 多个发布者同时提供数据 */
    OWN_TYPE_EXCLUSIVE,         /**< 排他模式 - 只有最高强度的发布者提供数据 */
} own_type_t;

/** 所有权状态 */
typedef enum {
    OWN_STATE_IDLE = 0,         /**< 空闲 */
    OWN_STATE_NEGOTIATING,      /**< 正在协商 */
    OWN_STATE_OWNER,            /**< 当前所有权主 */
    OWN_STATE_CONTENDER,        /**< 竞争者 */
    OWN_STATE_STANDBY,          /**< 待机状态 */
} own_state_t;

/** 所有权事件类型 */
typedef enum {
    OWN_EVENT_NONE = 0,
    OWN_EVENT_OWNERSHIP_GAINED,     /**< 获得所有权 */
    OWN_EVENT_OWNERSHIP_LOST,       /**< 丧失所有权 */
    OWN_EVENT_STRENGTH_CHANGED,     /**< 强度变化 */
    OWN_EVENT_CONTENDER_APPEARED,   /**< 新竞争者出现 */
    OWN_EVENT_OWNER_DISCONNECTED,   /**< 所有权断开 */
    OWN_EVENT_TRANSFER_REQUESTED,   /**< 转移请求 */
} own_event_t;

/** 所有权转移策略 */
typedef enum {
    OWN_TRANSFER_IMMEDIATE = 0,     /**< 立即转移 */
    OWN_TRANSFER_GRACEFUL,          /**< 优雅转移(完成当前消息) */
    OWN_TRANSFER_DELAYED,           /**< 延迟转移(等待窗口期) */
} own_transfer_policy_t;

/** 所有权主信息 */
typedef struct {
    dds_guid_t guid;                    /**< 所有权主GUID */
    uint32_t strength;                  /**< 当前强度 */
    own_state_t state;                  /**< 状态 */
    uint64_t last_heartbeat;            /**< 最后心跳时间 */
    bool active;                        /**< 是否活跃 */
    uint8_t priority;                   /**< 优先级(强度相同时使用) */
    uint64_t ownership_time;            /**< 获得所有权的时间 */
} own_owner_info_t;

/** 所有权配置 */
typedef struct {
    own_type_t type;                    /**< 所有权类型 */
    uint32_t initial_strength;          /**< 初始强度 */
    own_transfer_policy_t transfer_policy; /**< 转移策略 */
    uint32_t negotiation_timeout_ms;    /**< 协商超时 */
    bool auto_strength_adjust;          /**< 自动调整强度 */
    uint32_t min_strength;              /**< 最小强度 */
    uint32_t max_strength;              /**< 最大强度 */
} own_config_t;

/** 所有权事件回调 */
typedef void (*own_event_callback_t)(own_event_t event, const void *event_data, void *user_data);

/** 所有权转移请求回调 */
typedef bool (*own_transfer_callback_t)(dds_guid_t *current_owner, 
                                        dds_guid_t *new_owner,
                                        uint32_t new_strength,
                                        void *user_data);

/** 所有权统计 */
typedef struct {
    uint64_t ownership_gained_count;    /**< 获得所有权次数 */
    uint64_t ownership_lost_count;      /**< 丧失所有权次数 */
    uint64_t negotiation_count;         /**< 协商次数 */
    uint64_t transfer_count;            /**< 转移次数 */
    uint64_t ownership_duration_ms;     /**< 总所有权时长(ms) */
    uint32_t current_strength;          /**< 当前强度 */
    uint32_t avg_strength;              /**< 平均强度 */
} own_stats_t;

/** 所有权管理器句柄(不透明) */
typedef struct own_handle own_handle_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化所有权模块
 * @return ETH_OK成功
 */
eth_status_t own_init(void);

/**
 * @brief 反初始化所有权模块
 */
void own_deinit(void);

/**
 * @brief 创建所有权管理器
 * @param config 所有权配置
 * @param local_guid 本地GUID
 * @return 所有权管理器句柄
 */
own_handle_t* own_create(const own_config_t *config, const dds_guid_t *local_guid);

/**
 * @brief 删除所有权管理器
 * @param own 所有权管理器句柄
 * @return ETH_OK成功
 */
eth_status_t own_delete(own_handle_t *own);

/**
 * @brief 注册事件回调
 * @param own 所有权管理器句柄
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t own_register_event_callback(own_handle_t *own, own_event_callback_t callback, void *user_data);

/**
 * @brief 注册转移请求回调
 * @param own 所有权管理器句柄
 * @param callback 转移回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t own_register_transfer_callback(own_handle_t *own, own_transfer_callback_t callback, void *user_data);

/**
 * @brief 设置所有权强度
 * @param own 所有权管理器句柄
 * @param strength 新的强度值
 * @return ETH_OK成功
 */
eth_status_t own_set_strength(own_handle_t *own, uint32_t strength);

/**
 * @brief 获取当前所有权强度
 * @param own 所有权管理器句柄
 * @param strength 输出: 当前强度
 * @return ETH_OK成功
 */
eth_status_t own_get_strength(own_handle_t *own, uint32_t *strength);

/**
 * @brief 提高所有权强度(用于主动获取所有权)
 * @param own 所有权管理器句柄
 * @param increment 增量
 * @return ETH_OK成功
 */
eth_status_t own_increase_strength(own_handle_t *own, uint32_t increment);

/**
 * @brief 降低所有权强度(用于主动放弃所有权)
 * @param own 所有权管理器句柄
 * @param decrement 减量
 * @return ETH_OK成功
 */
eth_status_t own_decrease_strength(own_handle_t *own, uint32_t decrement);

/**
 * @brief 处理来自其他发布者的强度宣告
 * @param own 所有权管理器句柄
 * @param remote_guid 远程发布者GUID
 * @param remote_strength 远程强度
 * @return ETH_OK成功
 */
eth_status_t own_process_strength_announcement(own_handle_t *own, 
                                                const dds_guid_t *remote_guid,
                                                uint32_t remote_strength);

/**
 * @brief 检查是否是当前所有权主
 * @param own 所有权管理器句柄
 * @param is_owner 输出: 是否是所有权主
 * @return ETH_OK成功
 */
eth_status_t own_is_owner(own_handle_t *own, bool *is_owner);

/**
 * @brief 获取当前所有权主信息
 * @param own 所有权管理器句柄
 * @param owner_info 输出: 所有权主信息
 * @return ETH_OK成功，ETH_ERROR无当前所有权主
 */
eth_status_t own_get_current_owner(own_handle_t *own, own_owner_info_t *owner_info);

/**
 * @brief 执行所有权协商
 * @param own 所有权管理器句柄
 * @return ETH_OK成功，ETH_TIMEOUT协商超时
 */
eth_status_t own_negotiate(own_handle_t *own);

/**
 * @brief 请求所有权转移
 * @param own 所有权管理器句柄
 * @param target_guid 目标接收者GUID
 * @return ETH_OK成功
 */
eth_status_t own_request_transfer(own_handle_t *own, const dds_guid_t *target_guid);

/**
 * @brief 接受所有权转移
 * @param own 所有权管理器句柄
 * @param new_strength 新强度(可为0保持当前)
 * @return ETH_OK成功
 */
eth_status_t own_accept_transfer(own_handle_t *own, uint32_t new_strength);

/**
 * @brief 拒绝所有权转移
 * @param own 所有权管理器句柄
 * @param reason 拒绝原因
 * @return ETH_OK成功
 */
eth_status_t own_reject_transfer(own_handle_t *own, const char *reason);

/**
 * @brief 更新所有权主心跳
 * @param own 所有权管理器句柄
 * @param owner_guid 所有权主GUID
 * @return ETH_OK成功
 */
eth_status_t own_update_heartbeat(own_handle_t *own, const dds_guid_t *owner_guid);

/**
 * @brief 检查所有权主是否超时
 * @param own 所有权管理器句柄
 * @param current_time_ms 当前时间(ms)
 * @param timed_out 输出: 是否超时
 * @return ETH_OK成功
 */
eth_status_t own_check_timeout(own_handle_t *own, uint64_t current_time_ms, bool *timed_out);

/**
 * @brief 执行所有权主故障转移
 * @param own 所有权管理器句柄
 * @return ETH_OK成功
 */
eth_status_t own_handle_owner_failure(own_handle_t *own);

/**
 * @brief 获取所有权统计信息
 * @param own 所有权管理器句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t own_get_stats(own_handle_t *own, own_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param own 所有权管理器句柄
 * @return ETH_OK成功
 */
eth_status_t own_reset_stats(own_handle_t *own);

/**
 * @brief 设置ASIL安全模式
 * @param own 所有权管理器句柄
 * @param asil_level ASIL等级(0-4)
 * @return ETH_OK成功
 */
eth_status_t own_enable_asil_mode(own_handle_t *own, uint8_t asil_level);

/**
 * @brief 获取所有权主列表(用于评估)
 * @param own 所有权管理器句柄
 * @param owners 输出: 所有权主数组
 * @param max_count 最大数量
 * @param actual_count 实际数量输出
 * @return ETH_OK成功
 */
eth_status_t own_get_owner_list(own_handle_t *own, 
                                 own_owner_info_t *owners,
                                 uint8_t max_count,
                                 uint8_t *actual_count);

/**
 * @brief 强制获取所有权(管理员命令)
 * @param own 所有权管理器句柄
 * @param new_strength 新强度
 * @return ETH_OK成功
 */
eth_status_t own_force_ownership(own_handle_t *own, uint32_t new_strength);

#ifdef __cplusplus
}
#endif

#endif /* OWNERSHIP_H */
