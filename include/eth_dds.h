/**
 * @file eth_dds.h
 * @brief ETH-DDS Integration Library - Unified API Header
 *
 * 该文件提供ETH-DDS集成库的统一API入口。
 * 通过包含此单个头文件，应用程序可以访问所有功能模块。
 *
 * @copyright Copyright (c) 2026 ETH-DDS Project
 * @license Apache-2.0
 *
 * @author ETH-DDS Development Team
 * @version 2.0.0
 */

#ifndef ETH_DDS_H
#define ETH_DDS_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本信息
 *============================================================================*/

/** 主版本号 */
#ifndef ETH_DDS_VERSION_MAJOR
#define ETH_DDS_VERSION_MAJOR 2
#endif

/** 次版本号 */
#ifndef ETH_DDS_VERSION_MINOR
#define ETH_DDS_VERSION_MINOR 0
#endif

/** 修订版本号 */
#ifndef ETH_DDS_VERSION_PATCH
#define ETH_DDS_VERSION_PATCH 0
#endif

/** 完整版本字符串 */
#define ETH_DDS_VERSION_STRING "2.0.0"

/*==============================================================================
 * 平台检测与类型定义
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*==============================================================================
 * 核心类型定义
 *============================================================================*/

/**
 * @brief 返回码定义
 */
typedef enum {
    ETH_DDS_OK = 0,           /**< 成功 */
    ETH_DDS_ERROR = -1,       /**< 通用错误 */
    ETH_DDS_EINVAL = -2,      /**< 无效参数 */
    ETH_DDS_ENOMEM = -3,      /**< 内存不足 */
    ETH_DDS_EAGAIN = -4,      /**< 资源不可用 */
    ETH_DDS_ETIMEDOUT = -5,   /**< 超时 */
    ETH_DDS_EBUSY = -6,       /**< 设备忙 */
    ETH_DDS_ENOTSUP = -7,     /**< 不支持的操作 */
    ETH_DDS_EPERM = -8,       /**< 权限不足 */
    ETH_DDS_EIO = -9,         /**< I/O错误 */
} eth_dds_ret_t;

/**
 * @brief QoS策略级别
 */
typedef enum {
    ETH_QOS_BEST_EFFORT = 0,  /**< 最佳努力 */
    ETH_QOS_RELIABLE = 1,     /**< 可靠传输 */
    ETH_QOS_REALTIME = 2,     /**< 实时传输 */
    ETH_QOS_DETERMINISTIC = 3, /**< 确定性传输 (TSN) */
} eth_qos_level_t;

/*==============================================================================
 * 模块功能宏定义
 *============================================================================*/

/* 功能模块开关 */
#ifdef ENABLE_ETHERNET
    #define ETH_DDS_HAS_ETHERNET 1
#else
    #define ETH_DDS_HAS_ETHERNET 0
#endif

#ifdef ENABLE_DDS
    #define ETH_DDS_HAS_DDS 1
#else
    #define ETH_DDS_HAS_DDS 0
#endif

#ifdef ENABLE_TSN
    #define ETH_DDS_HAS_TSN 1
#else
    #define ETH_DDS_HAS_TSN 0
#endif

#ifdef ENABLE_AUTOSAR_CLASSIC
    #define ETH_DDS_HAS_AUTOSAR_CLASSIC 1
#else
    #define ETH_DDS_HAS_AUTOSAR_CLASSIC 0
#endif

#ifdef ENABLE_AUTOSAR_ADAPTIVE
    #define ETH_DDS_HAS_AUTOSAR_ADAPTIVE 1
#else
    #define ETH_DDS_HAS_AUTOSAR_ADAPTIVE 0
#endif

#ifdef ENABLE_DDS_SECURITY
    #define ETH_DDS_HAS_DDS_SECURITY 1
#else
    #define ETH_DDS_HAS_DDS_SECURITY 0
#endif

#ifdef ENABLE_DDS_ADVANCED
    #define ETH_DDS_HAS_DDS_ADVANCED 1
#else
    #define ETH_DDS_HAS_DDS_ADVANCED 0
#endif

/*==============================================================================
 * Common模块 - 基础组件
 *============================================================================*/
#include "common/types/eth_types.h"
#include "common/utils/eth_utils.h"
#include "common/log/dds_log.h"

/*==============================================================================
 * Ethernet模块 - 以太网驱动
 *============================================================================*/
#if ETH_DDS_HAS_ETHERNET
#include "ethernet/driver/eth_driver.h"
#include "ethernet/driver/eth_mac_driver.h"
#include "ethernet/driver/eth_dma.h"
#include "ethernet/driver/eth_automotive.h"
#include "ethernet/eth_manager.h"
#endif

/*==============================================================================
 * Transport模块 - 传输层
 *============================================================================*/
#include "transport/transport_interface.h"
#include "transport/transport_manager.h"
#include "transport/udp_transport.h"
#include "transport/shm_transport.h"

/*==============================================================================
 * DDS模块 - 数据分发服务
 *============================================================================*/
#if ETH_DDS_HAS_DDS
#include "dds/core/dds_core.h"
#include "dds/rtps/rtps_message.h"
#include "dds/rtps/rtps_discovery.h"
#include "dds/rtps/rtps_state.h"

    /* DDS高级功能 */
    #if ETH_DDS_HAS_DDS_ADVANCED
    #include "dds/pubsub/content_filtered_topic.h"
    #include "dds/pubsub/time_filter.h"
    #include "dds/pubsub/ownership.h"
    #include "dds/pubsub/persistence.h"
    #include "dds/pubsub/dds_advanced_pubsub.h"
    #endif

    /* DDS安全扩展 */
    #if ETH_DDS_HAS_DDS_SECURITY
    #include "dds/security/dds_security_types.h"
    #include "dds/security/dds_security_manager.h"
    #include "dds/security/dds_auth.h"
    #include "dds/security/dds_crypto.h"
    #include "dds/security/dds_access.h"
    #endif

    /* DDS运行时 */
    #ifdef ENABLE_DDS_RUNTIME
    #include "dds/runtime/dds_runtime.h"
    #endif
#endif

/*==============================================================================
 * TSN模块 - 时间敏感网络
 *============================================================================*/
#if ETH_DDS_HAS_TSN
#include "tsn/tsn_stack.h"
#include "tsn/gptp/gptp.h"
#include "tsn/tas/tas.h"
#include "tsn/cbs/cbs.h"
#include "tsn/fp/frame_preemption.h"
#include "tsn/srp/stream_reservation.h"
#endif

/*==============================================================================
 * AUTOSAR模块 - 汽车软件架构
 *============================================================================*/
#if ETH_DDS_HAS_AUTOSAR_CLASSIC
#include "autosar/classic/rte_dds.h"
#include "autosar/e2e/e2e_protection.h"
#include "autosar/arxml/arxml_parser.h"
#endif

#if ETH_DDS_HAS_AUTOSAR_ADAPTIVE
#include "autosar/adaptive/ara_com_dds.h"
#endif

/*==============================================================================
 * 统一API接口
 *============================================================================*/

/**
 * @brief 库初始化配置参数
 */
typedef struct {
    uint32_t flags;           /**< 初始化标志 */
    uint16_t domain_id;       /**< DDS域ID */
    uint8_t priority;         /**< 默认优先级 */
    const char* config_file;  /**< 配置文件路径 (可为NULL) */
} eth_dds_config_t;

/**
 * @brief 初始化配置默认值
 */
#define ETH_DDS_CONFIG_DEFAULT { \
    .flags = 0, \
    .domain_id = 0, \
    .priority = 0, \
    .config_file = NULL \
}

/**
 * @brief 初始化标志位
 */
#define ETH_DDS_INIT_ETHERNET       0x0001  /**< 初始化Ethernet模块 */
#define ETH_DDS_INIT_DDS            0x0002  /**< 初始化DDS模块 */
#define ETH_DDS_INIT_TSN            0x0004  /**< 初始化TSN模块 */
#define ETH_DDS_INIT_AUTOSAR        0x0008  /**< 初始化AUTOSAR模块 */
#define ETH_DDS_INIT_SECURITY       0x0010  /**< 初始化安全模块 */
#define ETH_DDS_INIT_LOG            0x0020  /**< 初始化日志模块 */
#define ETH_DDS_INIT_ALL            0xFFFF  /**< 初始化所有模块 */

/*==============================================================================
 * 核心API函数声明
 *============================================================================*/

/**
 * @brief 获取库版本信息
 *
 * @param[out] major 主版本号 (可为NULL)
 * @param[out] minor 次版本号 (可为NULL)
 * @param[out] patch 修订版本号 (可为NULL)
 * @return 版本字符串 (静态存储，不需释放)
 */
const char* eth_dds_get_version(int* major, int* minor, int* patch);

/**
 * @brief 初始化ETH-DDS库
 *
 * 初始化库的所有模块和子系统。必须在使用其他API之前调用。
 *
 * @param[in] config 初始化配置参数 (可为NULL使用默认配置)
 * @return ETH_DDS_OK 成功，否则返回错误码
 */
eth_dds_ret_t eth_dds_init(const eth_dds_config_t* config);

/**
 * @brief 反初始化ETH-DDS库
 *
 * 释放库占用的所有资源，关闭所有模块。
 */
void eth_dds_deinit(void);

/**
 * @brief 检查模块是否可用
 *
 * @param[in] module 模块标识符 ('E'=Ethernet, 'D'=DDS, 'T'=TSN, 'C'=AUTOSAR Classic, 'A'=AUTOSAR Adaptive, 'S'=Security)
 * @return true 模块已启用且可用，false 模块未启用或不可用
 */
bool eth_dds_module_available(char module);

/**
 * @brief 获取最后错误信息
 *
 * @return 错误描述字符串 (静态存储)
 */
const char* eth_dds_get_last_error(void);

/**
 * @brief 设置日志级别
 *
 * @param[in] level 日志级别 (0=禁用, 1=错误, 2=警告, 3=信息, 4=调试)
 */
void eth_dds_set_log_level(int level);

/*==============================================================================
 * 常用API宏定义
 *============================================================================*/

/**
 * @brief 检查返回码是否表示成功
 */
#define ETH_DDS_IS_OK(ret) ((ret) == ETH_DDS_OK)

/**
 * @brief 检查返回码是否表示失败
 */
#define ETH_DDS_IS_ERROR(ret) ((ret) != ETH_DDS_OK)

/**
 * @brief 安全释放指针并置NULL
 */
#define ETH_DDS_FREE(ptr) do { if (ptr) { free(ptr); (ptr) = NULL; } } while(0)

/**
 * @brief 数组大小计算
 */
#define ETH_DDS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief 字节对齐
 */
#define ETH_DDS_ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ETH_DDS_ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

/*==============================================================================
 * 内联函数定义
 *============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* ETH_DDS_H */
