/**
 * @file dds_eth_discovery.h
 * @brief DDS发现协议 - SPDP & SEDP (DDS-RTPS 2.5)
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 实现DDS Simple Participant Discovery Protocol (SPDP)
 * @note 实现DDS Simple Endpoint Discovery Protocol (SEDP)
 */

#ifndef DDS_ETH_DISCOVERY_H
#define DDS_ETH_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include "dds_eth_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 发现协议常量
 * ============================================================================ */

/** SPDP内建主题名称 */
#define DDS_SPDP_BUILTIN_TOPIC_NAME      "DCPSParticipant"
#define DDS_SEDP_BUILTIN_TOPIC_SUBSCRIPTIONS "DCPSSubscription"
#define DDS_SEDP_BUILTIN_TOPIC_PUBLICATIONS  "DCPSPublication"

/** RTPS实体ID */
#define DDS_ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER  0x000100C2U
#define DDS_ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER  0x000100C7U
#define DDS_ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER 0x000200C2U
#define DDS_ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER 0x000200C7U
#define DDS_ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER 0x000700C2U
#define DDS_ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER 0x000700C7U

/** 发现参数 */
#define DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN    256
#define DDS_DISCOVERY_DEFAULT_PERIOD_MS     3000    /* 默认发现周期 */
#define DDS_DISCOVERY_MIN_PERIOD_MS         100     /* 最小发现周期 */
#define DDS_DISCOVERY_MAX_PERIOD_MS         60000   /* 最大发现周期 */
#define DDS_DISCOVERY_LEASE_DURATION_MS     10000   /* 默认租约持续时间 */
#define DDS_DISCOVERY_PARTICIPANT_CLEANUP_MS 20000  /* Participant清理时间 */

/* ============================================================================
 * 发现数据类型
 * ============================================================================ */

/** 实体种类 */
typedef enum {
    DDS_ENTITY_KIND_USER_UNKNOWN = 0x00,
    DDS_ENTITY_KIND_BUILTIN_UNKNOWN = 0xC0,
    DDS_ENTITY_KIND_PARTICIPANT = 0xC1,
    DDS_ENTITY_KIND_WRITER_WITH_KEY = 0xC2,
    DDS_ENTITY_KIND_WRITER_NO_KEY = 0xC3,
    DDS_ENTITY_KIND_READER_WITH_KEY = 0xC7,
    DDS_ENTITY_KIND_READER_NO_KEY = 0xC4,
} dds_entity_kind_t;

/** 实体ID */
typedef struct {
    uint8_t entity_key[3];      /* 实体键 */
    uint8_t entity_kind;        /* 实体种类 */
} dds_entity_id_t;

/** 实体GUID */
typedef struct {
    uint8_t guid_prefix[12];    /* GUID前缀 */
    dds_entity_id_t entity_id;  /* 实体ID */
} dds_entity_guid_t;

/** 定位器种类 */
typedef enum {
    DDS_LOCATOR_KIND_INVALID = -1,
    DDS_LOCATOR_KIND_RESERVED = 0,
    DDS_LOCATOR_KIND_UDPv4 = 1,
    DDS_LOCATOR_KIND_UDPv6 = 2,
} dds_locator_kind_t;

/** 定位器 */
typedef struct {
    int32_t kind;               /* 定位器种类 */
    uint32_t port;              /* 端口 */
    uint8_t address[16];        /* 地址(IPv4或IPv6) */
} dds_locator_t;

/** 发现参数 (ParticipantProxy) */
typedef struct {
    dds_entity_guid_t participant_guid;      /* Participant GUID */
    dds_entity_guid_t builtin_endpoint_qos;  /* 内建Endpoint QoS */
    dds_locator_t metatraffic_unicast_locator;    /* 元数据单播定位器 */
    dds_locator_t metatraffic_multicast_locator;  /* 元数据多播定位器 */
    dds_locator_t default_unicast_locator;        /* 默认单播定位器 */
    dds_locator_t default_multicast_locator;      /* 默认多播定位器 */
    uint32_t available_builtin_endpoints;    /* 可用内建Endpoints */
    uint32_t manual_liveliness_count;        /* 手动活性计数 */
    uint32_t builtin_endpoint_qos_mask;      /* QoS掩码 */
    char domain_tag[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN]; /* 域标签 */
} dds_participant_proxy_t;

/** 发布信息数据 (PublicationBuiltinTopicData) */
typedef struct {
    dds_entity_guid_t participant_guid;      /* Participant GUID */
    dds_entity_guid_t publication_guid;      /* Publication GUID */
    char topic_name[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN];      /* 主题名 */
    char type_name[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN];       /* 类型名 */
    dds_qos_t qos;                           /* QoS政策 */
    dds_locator_t unicast_locator;           /* 单播定位器 */
    dds_locator_t multicast_locator;         /* 多播定位器 */
    uint32_t strength;                       /* Ownership strength */
} dds_publication_builtin_topic_data_t;

/** 订阅信息数据 (SubscriptionBuiltinTopicData) */
typedef struct {
    dds_entity_guid_t participant_guid;      /* Participant GUID */
    dds_entity_guid_t subscription_guid;     /* Subscription GUID */
    char topic_name[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN];      /* 主题名 */
    char type_name[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN];       /* 类型名 */
    dds_qos_t qos;                           /* QoS政策 */
    dds_locator_t unicast_locator;           /* 单播定位器 */
    dds_locator_t multicast_locator;         /* 多播定位器 */
} dds_subscription_builtin_topic_data_t;

/** 发现数据缓存 */
typedef struct {
    bool valid;
    dds_participant_proxy_t participant;
    uint32_t discovery_time_ms;
    uint32_t last_update_ms;
} dds_discovered_participant_t;

typedef struct {
    bool valid;
    dds_publication_builtin_topic_data_t publication;
    uint32_t discovery_time_ms;
} dds_discovered_publication_t;

typedef struct {
    bool valid;
    dds_subscription_builtin_topic_data_t subscription;
    uint32_t discovery_time_ms;
} dds_discovered_subscription_t;

/* ============================================================================
 * 发现配置
 * ============================================================================ */

typedef struct {
    /* SPDP配置 */
    bool enable_spdp;                       /* 启用SPDP */
    uint32_t spdp_period_ms;                /* SPDP广播周期 */
    uint32_t spdp_lease_duration_ms;        /* 租约持续时间 */
    
    /* SEDP配置 */
    bool enable_sedp;                       /* 启用SEDP */
    uint32_t sedp_resend_period_ms;         /* SEDP重发周期 */
    
    /* 域配置 */
    uint32_t domain_id;                     /* DDS域ID */
    char domain_tag[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN]; /* 域标签 */
    
    /* Participant配置 */
    dds_entity_guid_t participant_guid;     /* 本地Participant GUID */
    
    /* 定位器配置 */
    dds_locator_t default_unicast_locator;
    dds_locator_t default_multicast_locator;
    dds_locator_t metatraffic_unicast_locator;
    dds_locator_t metatraffic_multicast_locator;
} dds_discovery_config_t;

/** 发现回调函数 */
typedef void (*dds_spdp_participant_discovered_cb_t)(
    const dds_participant_proxy_t *participant, 
    bool discovered, void *user_data);

typedef void (*dds_sedp_publication_discovered_cb_t)(
    const dds_publication_builtin_topic_data_t *publication,
    bool discovered, void *user_data);

typedef void (*dds_sedp_subscription_discovered_cb_t)(
    const dds_subscription_builtin_topic_data_t *subscription,
    bool discovered, void *user_data);

/* ============================================================================
 * 发现API
 * ============================================================================ */

/**
 * @brief 初始化DDS发现模块
 * @param config 发现配置
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_init(const dds_discovery_config_t *config);

/**
 * @brief 反初始化DDS发现模块
 */
void dds_discovery_deinit(void);

/**
 * @brief 启动发现协议
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_start(void);

/**
 * @brief 停止发现协议
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_stop(void);

/**
 * @brief 发现模块主函数 (需要定期调用)
 * @param elapsed_ms 经过的毫秒数
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * SPDP API
 * ============================================================================ */

/**
 * @brief 广播Participant数据 (SPDP)
 * @return ETH_OK成功
 */
eth_status_t dds_spdp_announce_participant(void);

/**
 * @brief 处理收到的SPDP数据
 * @param data SPDP数据
 * @param len 数据长度
 * @param locator 来源定位器
 * @return ETH_OK成功
 */
eth_status_t dds_spdp_handle_data(const uint8_t *data, uint32_t len,
                                   const dds_locator_t *locator);

/**
 * @brief 获取本地Participant代理
 * @param proxy 输出Participant代理
 * @return ETH_OK成功
 */
eth_status_t dds_spdp_get_local_participant(dds_participant_proxy_t *proxy);

/**
 * @brief 注册SPDP Participant发现回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_spdp_register_callback(dds_spdp_participant_discovered_cb_t callback,
                                         void *user_data);

/* ============================================================================
 * SEDP API
 * ============================================================================ */

/**
 * @brief 广播Publication数据 (SEDP)
 * @param pub_data 发布数据
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_announce_publication(
    const dds_publication_builtin_topic_data_t *pub_data);

/**
 * @brief 广播Subscription数据 (SEDP)
 * @param sub_data 订阅数据
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_announce_subscription(
    const dds_subscription_builtin_topic_data_t *sub_data);

/**
 * @brief 处理收到的SEDP Publication数据
 * @param data 数据指针
 * @param len 数据长度
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_handle_publication(const uint8_t *data, uint32_t len);

/**
 * @brief 处理收到的SEDP Subscription数据
 * @param data 数据指针
 * @param len 数据长度
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_handle_subscription(const uint8_t *data, uint32_t len);

/**
 * @brief 注册SEDP Publication发现回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_register_publication_callback(
    dds_sedp_publication_discovered_cb_t callback, void *user_data);

/**
 * @brief 注册SEDP Subscription发现回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_sedp_register_subscription_callback(
    dds_sedp_subscription_discovered_cb_t callback, void *user_data);

/* ============================================================================
 * 发现数据管理API
 * ============================================================================ */

/**
 * @brief 获取发现的Participant
 * @param guid Participant GUID
 * @param participant 输出Participant数据
 * @return ETH_OK找到
 */
eth_status_t dds_discovery_get_participant(const dds_entity_guid_t *guid,
                                            dds_participant_proxy_t *participant);

/**
 * @brief 获取发现的Publication
 * @param guid Publication GUID
 * @param publication 输出Publication数据
 * @return ETH_OK找到
 */
eth_status_t dds_discovery_get_publication(const dds_entity_guid_t *guid,
                                            dds_publication_builtin_topic_data_t *publication);

/**
 * @brief 获取发现的Subscription
 * @param guid Subscription GUID
 * @param subscription 输出Subscription数据
 * @return ETH_OK找到
 */
eth_status_t dds_discovery_get_subscription(const dds_entity_guid_t *guid,
                                             dds_subscription_builtin_topic_data_t *subscription);

/**
 * @brief 获取所有发现的Participant列表
 * @param participants 输出数组
 * @param max_count 最大数量
 * @param actual_count 实际数量输出
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_get_all_participants(dds_participant_proxy_t *participants,
                                                 uint32_t max_count,
                                                 uint32_t *actual_count);

/**
 * @brief 清理超时的Participant
 * @param current_time_ms 当前时间(毫秒)
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_cleanup_stale_participants(uint32_t current_time_ms);

/* ============================================================================
 * 发现数据序列化API
 * ============================================================================ */

/**
 * @brief 序列化ParticipantProxy
 * @param proxy Participant代理
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_serialize_participant(
    const dds_participant_proxy_t *proxy,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len);

/**
 * @brief 反序列化ParticipantProxy
 * @param data 输入数据
 * @param len 数据长度
 * @param proxy 输出Participant代理
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_deserialize_participant(
    const uint8_t *data, uint32_t len,
    dds_participant_proxy_t *proxy);

/**
 * @brief 序列化Publication数据
 * @param pub 发布数据
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_serialize_publication(
    const dds_publication_builtin_topic_data_t *pub,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len);

/**
 * @brief 反序列化Publication数据
 * @param data 输入数据
 * @param len 数据长度
 * @param pub 输出发布数据
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_deserialize_publication(
    const uint8_t *data, uint32_t len,
    dds_publication_builtin_topic_data_t *pub);

/**
 * @brief 序列化Subscription数据
 * @param sub 订阅数据
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_serialize_subscription(
    const dds_subscription_builtin_topic_data_t *sub,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len);

/**
 * @brief 反序列化Subscription数据
 * @param data 输入数据
 * @param len 数据长度
 * @param sub 输出订阅数据
 * @return ETH_OK成功
 */
eth_status_t dds_discovery_deserialize_subscription(
    const uint8_t *data, uint32_t len,
    dds_subscription_builtin_topic_data_t *sub);

#ifdef __cplusplus
}
#endif

#endif /* DDS_ETH_DISCOVERY_H */
