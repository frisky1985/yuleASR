/**
 * @file dds_soad_adapter.h
 * @brief DDS到SoAd适配层 - DDS与AUTOSAR SoAd映射
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 实现DDS QoS到Socket参数的映射
 * @note 支持可靠传输(TCP)和实时传输(UDP)
 */

#ifndef DDS_SOAD_ADAPTER_H
#define DDS_SOAD_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../common/types/eth_types.h"
#include "../../../soad/soad.h"
#include "../eth/dds_eth_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 适配层配置常量
 * ============================================================================ */

#define DDS_SOAD_MAX_MAPPINGS           16U
#define DDS_SOAD_MAX_SOCKETS            8U
#define DDS_SOAD_MAX_QOS_PROFILES       8U
#define DDS_SOAD_ADAPTER_VERSION        0x0100U

/* 缺省QoS配置 */
#define DDS_SOAD_DEFAULT_RELIABLE_PORT  7400U
#define DDS_SOAD_DEFAULT_BESTEFFORT_PORT 7401U
#define DDS_SOAD_DEFAULT_TCP_PORT       7402U

/* ============================================================================
 * QoS到Socket参数映射结构
 * ============================================================================ */

/** QoS传输类型 */
typedef enum {
    DDS_SOAD_TRANSPORT_UDP = 0,        /* UDP - 最佳努力 */
    DDS_SOAD_TRANSPORT_TCP,            /* TCP - 可靠传输 */
    DDS_SOAD_TRANSPORT_UDP_MULTICAST,  /* UDP多播 */
} dds_soad_transport_type_t;

/** QoS到Socket映射配置 */
typedef struct {
    dds_qos_reliability_t reliability;     /* DDS可靠性配置 */
    dds_qos_durability_t durability;       /* DDS持久性配置 */
    dds_soad_transport_type_t transport;   /* 传输层类型 */
    uint16_t port;                         /* 端口号 */
    uint16_t buffer_size;                  /* 缓冲区大小 */
    uint32_t timeout_ms;                   /* 超时时间 */
    bool enable_keepalive;                 /* 启用Keep-alive */
    uint16_t keepalive_interval;           /* Keep-alive间隔 */
    bool enable_nagle;                     /* 启用Nagle算法(TCP) */
    uint8_t priority;                      /* Socket优先级 */
} dds_soad_qos_mapping_t;

/** DDS-Topic到Socket映射 */
typedef struct {
    dds_topic_name_t topic_name;           /* DDS主题名 */
    dds_type_name_t type_name;             /* 数据类型名 */
    dds_qos_t qos;                         /* QoS配置 */
    SoAd_SoConIdType soad_socket_id;       /* 关联的SoAd Socket ID */
    dds_soad_transport_type_t transport;   /* 传输类型 */
    bool is_active;                        /* 映射是否活跃 */
} dds_soad_topic_mapping_t;

/** Endpoint Socket信息 */
typedef struct {
    SoAd_SoConIdType socket_id;
    dds_soad_transport_type_t transport;
    dds_qos_t qos;
    SoAd_SocketStateType state;
    eth_ip_addr_t remote_ip;
    uint16_t remote_port;
} dds_soad_endpoint_socket_t;

/* ============================================================================
 * 适配层配置
 * ============================================================================ */

typedef struct {
    eth_ip_addr_t local_ip;                /* 本地IP地址 */
    eth_mac_addr_t local_mac;              /* 本地MAC地址 */
    uint32_t domain_id;                    /* DDS域ID */
    
    /* SoAd配置引用 */
    const SoAd_ConfigType *soad_config;
    
    /* QoS映射表 */
    dds_soad_qos_mapping_t qos_mappings[DDS_SOAD_MAX_QOS_PROFILES];
    uint8_t num_qos_mappings;
    
    /* 缓冲区配置 */
    uint16_t rx_buffer_size;
    uint16_t tx_buffer_size;
    
    /* 超时配置 */
    uint32_t connect_timeout_ms;
    uint32_t tx_timeout_ms;
    uint32_t rx_timeout_ms;
} dds_soad_adapter_config_t;

/* ============================================================================
 * 适配层API
 * ============================================================================ */

/**
 * @brief 初始化DDS-SoAd适配层
 * @param config 适配层配置
 * @return ETH_OK成功
 */
eth_status_t dds_soad_adapter_init(const dds_soad_adapter_config_t *config);

/**
 * @brief 反初始化DDS-SoAd适配层
 */
void dds_soad_adapter_deinit(void);

/**
 * @brief 启动适配层
 * @return ETH_OK成功
 */
eth_status_t dds_soad_adapter_start(void);

/**
 * @brief 停止适配层
 * @return ETH_OK成功
 */
eth_status_t dds_soad_adapter_stop(void);

/**
 * @brief 适配层主函数 (需要定期调用)
 * @param elapsed_ms 经过的毫秒数
 * @return ETH_OK成功
 */
eth_status_t dds_soad_adapter_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * QoS映射API
 * ============================================================================ */

/**
 * @brief 根据QoS获取Socket配置
 * @param qos DDS QoS配置
 * @param mapping 输出映射配置
 * @return ETH_OK成功
 */
eth_status_t dds_soad_get_socket_config_for_qos(const dds_qos_t *qos,
                                                 dds_soad_qos_mapping_t *mapping);

/**
 * @brief 注册QoS映射
 * @param mapping QoS映射配置
 * @return ETH_OK成功
 */
eth_status_t dds_soad_register_qos_mapping(const dds_soad_qos_mapping_t *mapping);

/**
 * @brief 删除QoS映射
 * @param reliability 可靠性类型
 * @param durability 持久性类型
 * @return ETH_OK成功
 */
eth_status_t dds_soad_unregister_qos_mapping(dds_qos_reliability_t reliability,
                                              dds_qos_durability_t durability);

/**
 * @brief 创建Topic Socket映射
 * @param topic_name 主题名称
 * @param type_name 类型名称
 * @param qos QoS配置
 * @param mapping_id 输出映射ID
 * @return ETH_OK成功
 */
eth_status_t dds_soad_create_topic_socket(const char *topic_name,
                                           const char *type_name,
                                           const dds_qos_t *qos,
                                           uint16_t *mapping_id);

/**
 * @brief 删除Topic Socket映射
 * @param mapping_id 映射ID
 * @return ETH_OK成功
 */
eth_status_t dds_soad_delete_topic_socket(uint16_t mapping_id);

/* ============================================================================
 * 数据传输API
 * ============================================================================ */

/**
 * @brief 通过SoAd发送DDS数据
 * @param topic_name 主题名称
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t dds_soad_send_data(const char *topic_name,
                                 const uint8_t *data,
                                 uint32_t len,
                                 uint32_t timeout_ms);

/**
 * @brief 通过SoAd接收DDS数据
 * @param topic_name 主题名称
 * @param buffer 接收缓冲区
 * @param buf_size 缓冲区大小
 * @param received_len 实际接收长度输出
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t dds_soad_receive_data(const char *topic_name,
                                    uint8_t *buffer,
                                    uint32_t buf_size,
                                    uint32_t *received_len,
                                    uint32_t timeout_ms);

/**
 * @brief 注册数据接收回调
 * @param topic_name 主题名称
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_soad_register_rx_callback(const char *topic_name,
                                            dds_data_callback_t callback,
                                            void *user_data);

/* ============================================================================
 * 内部映射API
 * ============================================================================ */

/**
 * @brief 将DDS定位器转换为SoAd地址
 * @param dds_locator DDS定位器
 * @param soad_addr SoAd地址输出
 * @return ETH_OK成功
 */
eth_status_t dds_soad_convert_locator(const dds_endpoint_locator_t *dds_locator,
                                       SoAd_SocketAddrType *soad_addr);

/**
 * @brief 将SoAd地址转换为DDS定位器
 * @param soad_addr SoAd地址
 * @param dds_locator DDS定位器输出
 * @return ETH_OK成功
 */
eth_status_t dds_soad_convert_to_locator(const SoAd_SocketAddrType *soad_addr,
                                          dds_endpoint_locator_t *dds_locator);

/**
 * @brief 创建基于QoS的Socket连接
 * @param qos QoS配置
 * @param is_publisher 是否为发布者
 * @param socket_id 输出Socket ID
 * @return ETH_OK成功
 */
eth_status_t dds_soad_create_qos_socket(const dds_qos_t *qos,
                                         bool is_publisher,
                                         SoAd_SoConIdType *socket_id);

/**
 * @brief 关闭基于QoS的Socket连接
 * @param socket_id Socket ID
 * @return ETH_OK成功
 */
eth_status_t dds_soad_close_qos_socket(SoAd_SoConIdType socket_id);

#ifdef __cplusplus
}
#endif

#endif /* DDS_SOAD_ADAPTER_H */
