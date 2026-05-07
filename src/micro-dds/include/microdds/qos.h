/** @file qos.h
 * @brief Micro-DDS QoS策略定义
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 遵循MISRA C:2012规范
 */

#ifndef MICRODDS_QOS_H
#define MICRODDS_QOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 包含文件
 * ============================================================================ */
#include "types.h"

/* ============================================================================
 * QoS策略ID定义
 * ============================================================================ */
/** @brief QoS策略标识符 */
typedef enum {
    DDS_USERDATA_QOS_POLICY_ID = 0,          /**< 用户数据 */
    DDS_DURABILITY_QOS_POLICY_ID = 1,        /**< 持久性 */
    DDS_PRESENTATION_QOS_POLICY_ID = 2,      /**< 表示 */
    DDS_DEADLINE_QOS_POLICY_ID = 3,          /**< 截止期限 */
    DDS_LATENCYBUDGET_QOS_POLICY_ID = 4,     /**< 延迟预算 */
    DDS_OWNERSHIP_QOS_POLICY_ID = 5,         /**< 所有权 */
    DDS_OWNERSHIPSTRENGTH_QOS_POLICY_ID = 6, /**< 所有权强度 */
    DDS_LIVELINESS_QOS_POLICY_ID = 7,        /**< 活跃性 */
    DDS_TIMEBASEDFILTER_QOS_POLICY_ID = 8,   /**< 基于时间的过滤 */
    DDS_PARTITION_QOS_POLICY_ID = 9,         /**< 分区 */
    DDS_RELIABILITY_QOS_POLICY_ID = 10,      /**< 可靠性 */
    DDS_DESTINATIONORDER_QOS_POLICY_ID = 11, /**< 目的排序 */
    DDS_HISTORY_QOS_POLICY_ID = 12,          /**< 历史 */
    DDS_RESOURCELIMITS_QOS_POLICY_ID = 13,   /**< 资源限制 */
    DDS_ENTITYFACTORY_QOS_POLICY_ID = 14,    /**< 实体工厂 */
    DDS_WRITERDATALIFECYCLE_QOS_POLICY_ID = 15, /**< 写入器数据生命周期 */
    DDS_READERDATALIFECYCLE_QOS_POLICY_ID = 16, /**< 读取器数据生命周期 */
    DDS_TOPICDATA_QOS_POLICY_ID = 17,        /**< 主题数据 */
    DDS_GROUPDATA_QOS_POLICY_ID = 18,        /**< 组数据 */
    DDS_TRANSPORTPRIORITY_QOS_POLICY_ID = 19, /**< 传输优先级 */
    DDS_LIFESPAN_QOS_POLICY_ID = 20,         /**< 生命周期 */
    DDS_DURABILITYSERVICE_QOS_POLICY_ID = 21, /**< 持久性服务 */
    DDS_INVALID_QOS_POLICY_ID = 22           /**< 无效策略 */
} DDS_QosPolicyId_t;

/* ============================================================================
 * 历史策略
 * ============================================================================ */
/** @brief 历史QoS策略类型 */
typedef enum {
    DDS_KEEP_LAST_HISTORY_QOS = 0, /**< 保留最后N个样本 */
    DDS_KEEP_ALL_HISTORY_QOS = 1   /**< 保留所有样本 */
} DDS_HistoryQosPolicyKind;

/** @brief 历史QoS策略 */
typedef struct {
    DDS_HistoryQosPolicyKind kind; /**< 历史类型 */
    int32_t depth;                 /**< 历史深度 */
} DDS_HistoryQosPolicy;

/* ============================================================================
 * 持久性策略
 * ============================================================================ */
/** @brief 持久性QoS策略类型 */
typedef enum {
    DDS_VOLATILE_DURABILITY_QOS = 0,      /**< 易失性 */
    DDS_TRANSIENT_LOCAL_DURABILITY_QOS = 1, /**< 本地临时 */
    DDS_TRANSIENT_DURABILITY_QOS = 2,     /**< 临时性 */
    DDS_PERSISTENT_DURABILITY_QOS = 3     /**< 持久性 */
} DDS_DurabilityQosPolicyKind;

/** @brief 持久性QoS策略 */
typedef struct {
    DDS_DurabilityQosPolicyKind kind; /**< 持久性类型 */
} DDS_DurabilityQosPolicy;

/* ============================================================================
 * 可靠性策略
 * ============================================================================ */
/** @brief 可靠性QoS策略类型 */
typedef enum {
    DDS_BEST_EFFORT_RELIABILITY_QOS = 0, /**< 尽力而为 */
    DDS_RELIABLE_RELIABILITY_QOS = 1     /**< 可靠传输 */
} DDS_ReliabilityQosPolicyKind;

/** @brief 可靠性QoS策略 */
typedef struct {
    DDS_ReliabilityQosPolicyKind kind; /**< 可靠性类型 */
    DDS_Duration_t max_blocking_time;  /**< 最大阻塞时间 */
} DDS_ReliabilityQosPolicy;

/* ============================================================================
 * 截止期限策略
 * ============================================================================ */
/** @brief 截止期限QoS策略 */
typedef struct {
    DDS_Duration_t period; /**< 截止期限周期 */
} DDS_DeadlineQosPolicy;

/* ============================================================================
 * 延迟预算策略
 * ============================================================================ */
/** @brief 延迟预算QoS策略 */
typedef struct {
    DDS_Duration_t duration; /**< 延迟预算持续时间 */
} DDS_LatencyBudgetQosPolicy;

/* ============================================================================
 * 活跃性策略
 * ============================================================================ */
/** @brief 活跃性QoS策略类型 */
typedef enum {
    DDS_AUTOMATIC_LIVELINESS_QOS = 0,      /**< 自动活跃性 */
    DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS = 1, /**< 手动按参与者 */
    DDS_MANUAL_BY_TOPIC_LIVELINESS_QOS = 2  /**< 手动按主题 */
} DDS_LivelinessQosPolicyKind;

/** @brief 活跃性QoS策略 */
typedef struct {
    DDS_LivelinessQosPolicyKind kind; /**< 活跃性类型 */
    DDS_Duration_t lease_duration;    /**< 租约持续时间 */
} DDS_LivelinessQosPolicy;

/* ============================================================================
 * 资源限制策略
 * ============================================================================ */
/** @brief 资源限制QoS策略 */
typedef struct {
    int32_t max_samples;           /**< 最大样本数 */
    int32_t max_instances;         /**< 最大实例数 */
    int32_t max_samples_per_instance; /**< 每实例最大样本数 */
} DDS_ResourceLimitsQosPolicy;

/* ============================================================================
 * 生命周期策略
 * ============================================================================ */
/** @brief 生命周期QoS策略 */
typedef struct {
    DDS_Duration_t autopurge_nowriter_samples_delay; /**< 自动清理无写入器样本延迟 */
    DDS_Duration_t autopurge_disposed_samples_delay; /**< 自动清理已处置样本延迟 */
} DDS_ReaderDataLifecycleQosPolicy;

/** @brief 写入器生命周期QoS策略 */
typedef struct {
    bool autodispose_unregistered_instances; /**< 自动处置未注册实例 */
} DDS_WriterDataLifecycleQosPolicy;

/* ============================================================================
 * 所有权策略
 * ============================================================================ */
/** @brief 所有权QoS策略类型 */
typedef enum {
    DDS_SHARED_OWNERSHIP_QOS = 0, /**< 共享所有权 */
    DDS_EXCLUSIVE_OWNERSHIP_QOS = 1 /**< 独占所有权 */
} DDS_OwnershipQosPolicyKind;

/** @brief 所有权QoS策略 */
typedef struct {
    DDS_OwnershipQosPolicyKind kind; /**< 所有权类型 */
} DDS_OwnershipQosPolicy;

/** @brief 所有权强度QoS策略 */
typedef struct {
    int32_t value; /**< 所有权强度值 */
} DDS_OwnershipStrengthQosPolicy;

/* ============================================================================
 * 目的地顺序策略
 * ============================================================================ */
/** @brief 目的地顺序QoS策略类型 */
typedef enum {
    DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS = 0, /**< 按接收时间戳 */
    DDS_BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS = 1     /**< 按源时间戳 */
} DDS_DestinationOrderQosPolicyKind;

/** @brief 目的地顺序QoS策略 */
typedef struct {
    DDS_DestinationOrderQosPolicyKind kind; /**< 顺序类型 */
} DDS_DestinationOrderQosPolicy;

/* ============================================================================
 * 分区策略
 * ============================================================================ */
/** @brief 分区QoS策略 */
typedef struct {
    DDS_StringSeq name; /**< 分区名称序列 */
} DDS_PartitionQosPolicy;

/* ============================================================================
 * 传输优先级策略
 * ============================================================================ */
/** @brief 传输优先级QoS策略 */
typedef struct {
    int32_t value; /**< 优先级值 */
} DDS_TransportPriorityQosPolicy;

/* ============================================================================
 * 表示策略
 * ============================================================================ */
/** @brief 访问范围类型 */
typedef enum {
    DDS_INSTANCE_PRESENTATION_QOS = 0, /**< 实例级访问 */
    DDS_TOPIC_PRESENTATION_QOS = 1,    /**< 主题级访问 */
    DDS_GROUP_PRESENTATION_QOS = 2     /**< 组级访问 */
} DDS_PresentationQosPolicyAccessScopeKind;

/** @brief 表示QoS策略 */
typedef struct {
    DDS_PresentationQosPolicyAccessScopeKind access_scope; /**< 访问范围 */
    bool coherent_access;    /**< 一致访问 */
    bool ordered_access;     /**< 有序访问 */
} DDS_PresentationQosPolicy;

/* ============================================================================
 * 用户数据策略
 * ============================================================================ */
/** @brief 用户数据QoS策略 */
typedef struct {
    DDS_Sequence value; /**< 用户数据值 */
} DDS_UserDataQosPolicy;

/** @brief 主题数据QoS策略 */
typedef struct DDS_UserDataQosPolicy DDS_TopicDataQosPolicy;
/** @brief 组数据QoS策略 */
typedef struct DDS_UserDataQosPolicy DDS_GroupDataQosPolicy;

/* ============================================================================
 * 生命周期策略
 * ============================================================================ */
/** @brief 生命周期QoS策略 */
typedef struct {
    DDS_Duration_t duration; /**< 生命周期持续时间 */
} DDS_LifespanQosPolicy;

/* ============================================================================
 * 实体工厂策略
 * ============================================================================ */
/** @brief 实体工厂QoS策略 */
typedef struct {
    bool autoenable_created_entities; /**< 自动启用创建的实体 */
} DDS_EntityFactoryQosPolicy;

/* ============================================================================
 * 完整QoS结构定义
 * ============================================================================ */
/** @brief 主题QoS */
typedef struct {
    DDS_TopicDataQosPolicy topic_data;
    DDS_DurabilityQosPolicy durability;
    DDS_DeadlineQosPolicy deadline;
    DDS_LatencyBudgetQosPolicy latency_budget;
    DDS_LivelinessQosPolicy liveliness;
    DDS_ReliabilityQosPolicy reliability;
    DDS_DestinationOrderQosPolicy destination_order;
    DDS_HistoryQosPolicy history;
    DDS_ResourceLimitsQosPolicy resource_limits;
    DDS_TransportPriorityQosPolicy transport_priority;
    DDS_LifespanQosPolicy lifespan;
    DDS_OwnershipQosPolicy ownership;
} DDS_TopicQos;

/** @brief 数据写入器QoS */
typedef struct {
    DDS_DurabilityQosPolicy durability;
    DDS_DeadlineQosPolicy deadline;
    DDS_LatencyBudgetQosPolicy latency_budget;
    DDS_LivelinessQosPolicy liveliness;
    DDS_ReliabilityQosPolicy reliability;
    DDS_DestinationOrderQosPolicy destination_order;
    DDS_HistoryQosPolicy history;
    DDS_ResourceLimitsQosPolicy resource_limits;
    DDS_TransportPriorityQosPolicy transport_priority;
    DDS_LifespanQosPolicy lifespan;
    DDS_UserDataQosPolicy user_data;
    DDS_OwnershipQosPolicy ownership;
    DDS_OwnershipStrengthQosPolicy ownership_strength;
    DDS_WriterDataLifecycleQosPolicy writer_data_lifecycle;
} DDS_DataWriterQos;

/** @brief 数据读取器QoS */
typedef struct {
    DDS_DurabilityQosPolicy durability;
    DDS_DeadlineQosPolicy deadline;
    DDS_LatencyBudgetQosPolicy latency_budget;
    DDS_LivelinessQosPolicy liveliness;
    DDS_ReliabilityQosPolicy reliability;
    DDS_DestinationOrderQosPolicy destination_order;
    DDS_HistoryQosPolicy history;
    DDS_ResourceLimitsQosPolicy resource_limits;
    DDS_UserDataQosPolicy user_data;
    DDS_TimeBasedFilterQosPolicy time_based_filter;
    DDS_ReaderDataLifecycleQosPolicy reader_data_lifecycle;
    DDS_OwnershipQosPolicy ownership;
} DDS_DataReaderQos;

/** @brief 基于时间的过滤策略（简化） */
typedef struct {
    DDS_Duration_t minimum_separation; /**< 最小分离时间 */
} DDS_TimeBasedFilterQosPolicy;

/** @brief 发布者QoS */
typedef struct {
    DDS_PresentationQosPolicy presentation;
    DDS_PartitionQosPolicy partition;
    DDS_GroupDataQosPolicy group_data;
    DDS_EntityFactoryQosPolicy entity_factory;
} DDS_PublisherQos;

/** @brief 订阅者QoS */
typedef struct {
    DDS_PresentationQosPolicy presentation;
    DDS_PartitionQosPolicy partition;
    DDS_GroupDataQosPolicy group_data;
    DDS_EntityFactoryQosPolicy entity_factory;
} DDS_SubscriberQos;

/** @brief 域参与者QoS */
typedef struct {
    DDS_UserDataQosPolicy user_data;
    DDS_EntityFactoryQosPolicy entity_factory;
} DDS_DomainParticipantQos;

/* ============================================================================
 * QoS策略默认值初始化函数
 * ============================================================================ */
/**
 * @brief 初始化主题QoS为默认值
 * @param qos 主题QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_TopicQos_init_default(DDS_TopicQos *qos);

/**
 * @brief 初始化数据写入器QoS为默认值
 * @param qos 数据写入器QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataWriterQos_init_default(DDS_DataWriterQos *qos);

/**
 * @brief 初始化数据读取器QoS为默认值
 * @param qos 数据读取器QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataReaderQos_init_default(DDS_DataReaderQos *qos);

/**
 * @brief 初始化发布者QoS为默认值
 * @param qos 发布者QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_PublisherQos_init_default(DDS_PublisherQos *qos);

/**
 * @brief 初始化订阅者QoS为默认值
 * @param qos 订阅者QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_SubscriberQos_init_default(DDS_SubscriberQos *qos);

/**
 * @brief 初始化域参与者QoS为默认值
 * @param qos 域参与者QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DomainParticipantQos_init_default(DDS_DomainParticipantQos *qos);

/**
 * @brief 复制主题QoS
 * @param dst 目标QoS结构指针
 * @param src 源QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_TopicQos_copy(DDS_TopicQos *dst, const DDS_TopicQos *src);

/**
 * @brief 复制数据写入器QoS
 * @param dst 目标QoS结构指针
 * @param src 源QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataWriterQos_copy(DDS_DataWriterQos *dst, const DDS_DataWriterQos *src);

/**
 * @brief 复制数据读取器QoS
 * @param dst 目标QoS结构指针
 * @param src 源QoS结构指针
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataReaderQos_copy(DDS_DataReaderQos *dst, const DDS_DataReaderQos *src);

/**
 * @brief 检查两个主题QoS是否兼容
 * @param offered 提供的QoS
 * @param requested 请求的QoS
 * @return true 兼容
 */
bool DDS_TopicQos_is_compatible(const DDS_TopicQos *offered, const DDS_TopicQos *requested);

/**
 * @brief 检查两个数据写入器/读取器QoS是否兼容
 * @param writer_qos 写入器QoS
 * @param reader_qos 读取器QoS
 * @return true 兼容
 */
bool DDS_DataWriterReaderQos_is_compatible(const DDS_DataWriterQos *writer_qos,
                                           const DDS_DataReaderQos *reader_qos);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_QOS_H */
