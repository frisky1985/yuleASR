/**
 * @file dds_core.h
 * @brief DDS 核心类型定义与标准 API 契约
 * @version 1.0
 * @date 2026-08-04
 *
 * 本文件是 DDS 中间件的核心头文件，定义:
 * - DDS 标准句柄类型 (dds_DomainParticipantHandleType 等)
 * - 内部实体结构 typedef (dds_domain_participant_t 等)
 * - 返回码、样本状态等常量
 * - DDS 标准 API (create/write/take/delete/qos)
 *
 * 契约来源: rte_dds.c / ara_com_dds.c / dds_runtime.h / dds_advanced_pubsub.h
 * 类型基础: eth_types.h (dds_qos_t, dds_domain_id_t, dds_data_callback_t)
 */

#ifndef DDS_CORE_H
#define DDS_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../common/types/eth_types.h"
#include "../rtps/rtps_message.h"
#include "../rtps/rtps_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DDS 返回码
 * ============================================================================ */

typedef enum {
    DDS_RETCODE_OK = 0,
    DDS_RETCODE_ERROR = 1,
    DDS_RETCODE_UNSUPPORTED = 2,
    DDS_RETCODE_BAD_PARAMETER = 3,
    DDS_RETCODE_PRECONDITION_NOT_MET = 4,
    DDS_RETCODE_OUT_OF_RESOURCES = 5,
    DDS_RETCODE_NOT_ENABLED = 6,
    DDS_RETCODE_IMMUTABLE_POLICY = 7,
    DDS_RETCODE_INCONSISTENT_POLICY = 8,
    DDS_RETCODE_ALREADY_DELETED = 9,
    DDS_RETCODE_TIMEOUT = 10,
    DDS_RETCODE_NO_DATA = 11,
    DDS_RETCODE_ILLEGAL_OPERATION = 12,
} dds_ReturnCode_t;

/* ============================================================================
 * DDS 标准常量
 * ============================================================================ */

/** 无效实体句柄 */
#define DDS_ENTITY_INVALID            ((dds_EntityHandleType)0)

/** 样本状态掩码 (取任意) */
#define DDS_ANY_SAMPLE_STATE          0xFFFF
#define DDS_ANY_VIEW_STATE            0xFFFF
#define DDS_ANY_INSTANCE_STATE        0xFFFF

/** SOME/IP 桥接专用域 ID */
#define DDS_DOMAIN_ID_SOMEIP_BRIDGE   0x80000001U

/** 句柄类型 (统一用 uintptr_t, 0 = 无效) */
typedef uintptr_t dds_EntityHandleType;
typedef dds_EntityHandleType dds_DomainParticipantHandleType;
typedef dds_EntityHandleType dds_PublisherHandleType;
typedef dds_EntityHandleType dds_SubscriberHandleType;
typedef dds_EntityHandleType dds_TopicHandleType;
typedef dds_EntityHandleType dds_DataWriterHandleType;
typedef dds_EntityHandleType dds_DataReaderHandleType;

/** 样本句柄 (ara::com 适配用) */
typedef dds_EntityHandleType dds_SampleHandleType;

/* ============================================================================
 * 内部实体结构 (与 dds_runtime.h 中的 struct 定义一致)
 * ============================================================================ */

struct dds_domain_participant;
struct dds_publisher;
struct dds_subscriber;
struct dds_topic;
struct dds_data_writer;
struct dds_data_reader;

typedef struct dds_domain_participant dds_domain_participant_t;
typedef struct dds_publisher dds_publisher_t;
typedef struct dds_subscriber dds_subscriber_t;
typedef struct dds_topic dds_topic_t;
typedef struct dds_data_writer dds_data_writer_t;
typedef struct dds_data_reader dds_data_reader_t;

/* ---- 实体完整定义 (core 层) ---- */

/** 参与者内部结构 */
struct dds_domain_participant {
    rtps_guid_t guid;                       /* 参与者GUID */
    dds_domain_id_t domain_id;              /* 域ID */
    dds_qos_t qos;                          /* QoS配置 */

    /* 资源管理 */
    struct dds_publisher *publishers;       /* 发布者列表 */
    struct dds_subscriber *subscribers;     /* 订阅者列表 */
    struct dds_topic *topics;               /* 主题列表 */
    uint32_t publisher_count;
    uint32_t subscriber_count;
    uint32_t topic_count;

    /* 状态 */
    bool active;
    uint64_t create_time;

    /* 链表指针 */
    struct dds_domain_participant *next;
};

/** 发布者内部结构 */
struct dds_publisher {
    rtps_guid_t guid;                       /* 发布者GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */

    /* 资源管理 */
    struct dds_data_writer *writers;        /* 写入者列表 */
    uint32_t writer_count;

    /* 状态 */
    bool active;

    /* 链表指针 */
    struct dds_publisher *next;
};

/** 订阅者内部结构 */
struct dds_subscriber {
    rtps_guid_t guid;                       /* 订阅者GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */

    /* 资源管理 */
    struct dds_data_reader *readers;        /* 读取者列表 */
    uint32_t reader_count;

    /* 状态 */
    bool active;

    /* 链表指针 */
    struct dds_subscriber *next;
};

/** 主题内部结构 */
struct dds_topic {
    rtps_guid_t guid;                       /* 主题GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */

    char name[64];                          /* 主题名称 */
    char type_name[64];                     /* 类型名称 */

    /* 端点引用 */
    uint32_t writer_ref_count;
    uint32_t reader_ref_count;

    /* 状态 */
    bool active;

    /* 链表指针 */
    struct dds_topic *next;
};

/** 数据写入者内部结构 */
struct dds_data_writer {
    rtps_guid_t guid;                       /* Writer GUID */
    dds_publisher_t *publisher;             /* 所属发布者 */
    dds_topic_t *topic;                     /* 关联主题 */
    dds_qos_t qos;                          /* QoS配置 */

    /* RTPS状态机 */
    rtps_writer_state_machine_t state_machine;

    /* 回调 */
    void (*write_callback)(void *user_data);
    void *write_callback_user_data;

    /* 样本大小 (由应用设置, dds_write 序列化长度) */
    uint32_t sample_size;

    /* 统计 */
    uint32_t samples_written;
    uint64_t last_write_time;

    /* 状态 */
    bool active;

    /* 链表指针 */
    struct dds_data_writer *next;
};

/** 数据读取者内部结构 */
struct dds_data_reader {
    rtps_guid_t guid;                       /* Reader GUID */
    dds_subscriber_t *subscriber;           /* 所属订阅者 */
    dds_topic_t *topic;                     /* 关联主题 */
    dds_qos_t qos;                          /* QoS配置 */

    /* RTPS状态机 */
    rtps_reader_state_machine_t state_machine;

    /* 回调 */
    dds_data_callback_t data_callback;
    void *data_callback_user_data;

    /* 接收缓冲 */
    uint8_t *receive_buffer;
    uint32_t receive_buffer_size;

    /* 统计 */
    uint32_t samples_received;
    uint64_t last_read_time;

    /* 状态 */
    bool active;

    /* 链表指针 */
    struct dds_data_reader *next;
};

/** 条件变量类型 (ara::com 适配用) */
typedef struct dds_condition_variable {
    bool signaled;
    void *user_data;
} dds_ConditionVariableType;

/* ============================================================================
 * QoS 类型 (DDS 标准 QoS)
 * ============================================================================ */

/** DomainParticipant QoS */
typedef struct {
    dds_qos_t base;
    uint32_t user_data_max_size;
    bool entity_factory_autoenable;
} dds_DomainParticipantQosType;

/** Publisher QoS */
typedef struct {
    dds_qos_t base;
    uint32_t partition_count;
    char partitions[8][64];
    bool entity_factory_autoenable;
} dds_PublisherQosType;

/** Subscriber QoS */
typedef struct {
    dds_qos_t base;
    uint32_t partition_count;
    char partitions[8][64];
    bool entity_factory_autoenable;
} dds_SubscriberQosType;

/** Topic QoS */
typedef struct {
    dds_qos_t base;
    uint32_t max_samples_per_instance;
} dds_TopicQosType;

/** DataWriter QoS */
typedef struct {
    dds_qos_t base;
    bool autodispose_unregistered_instances;
    uint32_t max_samples;
    uint32_t max_instances;
    uint32_t max_samples_per_instance;
} dds_DataWriterQosType;

/** DataReader QoS */
typedef struct {
    dds_qos_t base;
    uint32_t max_samples;
    uint32_t max_instances;
    uint32_t max_samples_per_instance;
} dds_DataReaderQosType;

/* ============================================================================
 * 样本信息结构 (dds_take 返回)
 * ============================================================================ */

typedef struct {
    uint8_t sample_state;
    uint8_t view_state;
    uint8_t instance_state;
    bool valid_data;
    uint64_t source_timestamp;
    uint32_t disposed_generation_count;
    uint32_t no_writers_generation_count;
    uint32_t instance_handle;
} dds_SampleInfoType;

/* ============================================================================
 * DDS 标准 API
 * ============================================================================ */

/* 注: dds_runtime_init/deinit 由 dds_runtime.h 声明 (eth_status_t 返回),
 * 本文件不重复声明. dds_get_time/dds_process_events 由 runtime 层提供. */

/* ---- 时间与事件处理 (由 dds_runtime.c 实现) ---- */

/** 获取当前时间 (微秒) */
uint64_t dds_get_time(void);

/** 事件处理 (AUTOSAR 轮询模式, 处理发现/心跳/数据分发) */
eth_status_t dds_process_events(void);

/* ---- 实体创建 ---- */

dds_DomainParticipantHandleType dds_create_participant(
    dds_domain_id_t domain_id,
    const dds_DomainParticipantQosType *qos,
    void *listener);

dds_PublisherHandleType dds_create_publisher(
    dds_DomainParticipantHandleType participant,
    const dds_PublisherQosType *qos,
    void *listener);

dds_SubscriberHandleType dds_create_subscriber(
    dds_DomainParticipantHandleType participant,
    const dds_SubscriberQosType *qos,
    void *listener);

dds_TopicHandleType dds_create_topic(
    dds_DomainParticipantHandleType participant,
    const char *topic_name,
    const char *type_name,
    const dds_TopicQosType *qos,
    void *listener);

dds_DataWriterHandleType dds_create_writer(
    dds_PublisherHandleType publisher,
    dds_TopicHandleType topic,
    const dds_DataWriterQosType *qos,
    void *listener);

dds_DataReaderHandleType dds_create_reader(
    dds_SubscriberHandleType subscriber,
    dds_TopicHandleType topic,
    const dds_DataReaderQosType *qos,
    void *listener);

/* ---- 数据操作 ---- */

dds_ReturnCode_t dds_write(
    dds_DataWriterHandleType writer,
    const void *data);

dds_ReturnCode_t dds_take(
    dds_DataReaderHandleType reader,
    void *data,
    dds_SampleInfoType *sample_info,
    uint32_t max_samples,
    uint32_t sample_states,
    uint32_t view_states,
    uint32_t instance_states);

dds_ReturnCode_t dds_delete(dds_EntityHandleType entity);

/* ---- QoS 访问 ---- */

dds_ReturnCode_t dds_get_qos(
    dds_EntityHandleType entity,
    void *qos);

dds_ReturnCode_t dds_set_qos(
    dds_EntityHandleType entity,
    const void *qos);

dds_ReturnCode_t dds_get_sample_rejected_status(
    dds_DataReaderHandleType reader,
    void *status);

/** 设置 writer 样本大小 (dds_write 序列化长度) */
dds_ReturnCode_t dds_set_writer_sample_size(
    dds_DataWriterHandleType writer,
    uint32_t sample_size);

/* ---- QoS 默认值初始化 ---- */

void dds_domain_participant_qos_init(dds_DomainParticipantQosType *qos);
void dds_publisher_qos_init(dds_PublisherQosType *qos);
void dds_subscriber_qos_init(dds_SubscriberQosType *qos);
void dds_topic_qos_init(dds_TopicQosType *qos);
void dds_datawriter_qos_init(dds_DataWriterQosType *qos);
void dds_datareader_qos_init(dds_DataReaderQosType *qos);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CORE_H */
