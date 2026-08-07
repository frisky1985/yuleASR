/**
 * @file dds_domain.c
 * @brief DDS 域参与者标准 API 实现
 * @version 1.0
 * @date 2026-08-04
 *
 * 实现 dds_core.h 声明的 DDS 标准 API:
 * - dds_create_participant / dds_delete (参与者)
 * - dds_create_publisher / dds_create_subscriber (发布者/订阅者)
 * - dds_create_topic (主题)
 * - dds_create_writer / dds_create_reader (写入者/读取者)
 * - dds_write / dds_take (数据操作)
 * - dds_get_qos / dds_set_qos / dds_get_sample_rejected_status
 *
 * 参与者管理委托给 runtime 层 (dds_runtime_create_participant 等)，
 * 本层维护标准句柄 → 内部实体的映射。
 */
#include "dds_domain.h"
#include "dds_entity.h"
#include "dds_qos.h"
#include "../runtime/dds_runtime.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 静态实体池 (ISO 26262 / AUTOSAR R21-11 BSW 禁止动态内存)
 * 编译期固定上限: 与 dds_runtime 默认配置一致 (可容纳完整域拓扑)
 * ============================================================================ */

#define DDS_POOL_PUBLISHERS   (DDS_DEFAULT_MAX_PUBLISHERS)
#define DDS_POOL_SUBSCRIBERS  (DDS_DEFAULT_MAX_SUBSCRIBERS)
#define DDS_POOL_TOPICS       (DDS_DEFAULT_MAX_TOPICS)
#define DDS_POOL_WRITERS      (DDS_DEFAULT_MAX_ENDPOINTS)
#define DDS_POOL_READERS      (DDS_DEFAULT_MAX_ENDPOINTS)

static dds_publisher_t  s_pub_pool[DDS_POOL_PUBLISHERS];
static dds_subscriber_t s_sub_pool[DDS_POOL_SUBSCRIBERS];
static dds_topic_t      s_topic_pool[DDS_POOL_TOPICS];
static dds_data_writer_t s_writer_pool[DDS_POOL_WRITERS];
static dds_data_reader_t s_reader_pool[DDS_POOL_READERS];

/* 接收缓冲池: 每个 reader 一个固定 4KB 缓冲 (与旧 malloc(4096) 一致) */
static uint8_t s_recv_buf_pool[DDS_POOL_READERS][4096];

/* 池空闲位图 */
static uint32_t s_pub_free[(DDS_POOL_PUBLISHERS + 31U) / 32U];
static uint32_t s_sub_free[(DDS_POOL_SUBSCRIBERS + 31U) / 32U];
static uint32_t s_topic_free[(DDS_POOL_TOPICS + 31U) / 32U];
static uint32_t s_writer_free[(DDS_POOL_WRITERS + 31U) / 32U];
static uint32_t s_reader_free[(DDS_POOL_READERS + 31U) / 32U];

/**
 * @brief 从静态池分配一个实体槽 (返回 NULL 表示池满)
 */
static void* dds_pool_alloc(void *pool, uint32_t *free_map, uint32_t count, uint32_t elem_size)
{
    for (uint32_t i = 0; i < count; i++) {
        uint32_t word = i / 32U;
        uint32_t bit = i % 32U;
        if ((free_map[word] & (1U << bit)) == 0U) {
            free_map[word] |= (1U << bit);
            return (void *)((uint8_t *)pool + ((size_t)i * elem_size));
        }
    }
    return NULL;
}

/**
 * @brief 释放实体槽回静态池 (越界检查: 指针必须属于对应池)
 */
static void dds_pool_free(void *pool, uint32_t *free_map, uint32_t count, uint32_t elem_size, void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    uintptr_t base = (uintptr_t)pool;
    uintptr_t p = (uintptr_t)ptr;
    if ((p < base) || (p >= (base + (uintptr_t)count * elem_size))) {
        return; /* 不属于本池 */
    }
    uint32_t idx = (uint32_t)((p - base) / (uintptr_t)elem_size);
    if (idx < count) {
        free_map[idx / 32U] &= ~(1U << (idx % 32U));
    }
}

/* dds_entity.c 依赖的池回收接口 (按地址范围判定, 非本池指针则无操作) */
void dds_entity_pool_free_publisher(void *ptr)
{
    dds_pool_free(s_pub_pool, s_pub_free, DDS_POOL_PUBLISHERS, sizeof(dds_publisher_t), ptr);
}

void dds_entity_pool_free_subscriber(void *ptr)
{
    dds_pool_free(s_sub_pool, s_sub_free, DDS_POOL_SUBSCRIBERS, sizeof(dds_subscriber_t), ptr);
}

void dds_entity_pool_free_topic(void *ptr)
{
    dds_pool_free(s_topic_pool, s_topic_free, DDS_POOL_TOPICS, sizeof(dds_topic_t), ptr);
}

void dds_entity_pool_free_writer(void *ptr)
{
    dds_pool_free(s_writer_pool, s_writer_free, DDS_POOL_WRITERS, sizeof(dds_data_writer_t), ptr);
}

void dds_entity_pool_free_reader(void *ptr)
{
    dds_pool_free(s_reader_pool, s_reader_free, DDS_POOL_READERS, sizeof(dds_data_reader_t), ptr);
}
/* ============================================================================
 * 内部辅助: 句柄 → 实体 转换
 * ============================================================================ */

static dds_domain_participant_t* handle_to_participant(dds_DomainParticipantHandleType h)
{
    return (dds_domain_participant_t*)(uintptr_t)h;
}

static dds_publisher_t* handle_to_publisher(dds_PublisherHandleType h)
{
    return (dds_publisher_t*)(uintptr_t)h;
}

static dds_subscriber_t* handle_to_subscriber(dds_SubscriberHandleType h)
{
    return (dds_subscriber_t*)(uintptr_t)h;
}

static dds_topic_t* handle_to_topic(dds_TopicHandleType h)
{
    return (dds_topic_t*)(uintptr_t)h;
}

static dds_data_writer_t* handle_to_writer(dds_DataWriterHandleType h)
{
    return (dds_data_writer_t*)(uintptr_t)h;
}

static dds_data_reader_t* handle_to_reader(dds_DataReaderHandleType h)
{
    return (dds_data_reader_t*)(uintptr_t)h;
}

/* ============================================================================
 * 参与者创建/删除
 * ============================================================================ */

dds_DomainParticipantHandleType dds_create_participant(
    dds_domain_id_t domain_id,
    const dds_DomainParticipantQosType *qos,
    void *listener)
{
    (void)listener;
    dds_domain_participant_t *participant =
        dds_runtime_create_participant(domain_id, NULL);
    if (participant == NULL) {
        return DDS_ENTITY_INVALID;
    }

    /* 应用 QoS (若提供) */
    if (qos != NULL) {
        participant->qos = qos->base;
    }

    return (dds_DomainParticipantHandleType)(uintptr_t)participant;
}

dds_PublisherHandleType dds_create_publisher(
    dds_DomainParticipantHandleType participant_handle,
    const dds_PublisherQosType *qos,
    void *listener)
{
    (void)listener;
    dds_domain_participant_t *participant = handle_to_participant(participant_handle);
    if ((participant == NULL) || !dds_runtime_is_participant(participant)) {
        return DDS_ENTITY_INVALID;
    }

    dds_publisher_t *pub = (dds_publisher_t *)dds_pool_alloc(s_pub_pool, s_pub_free, DDS_POOL_PUBLISHERS, sizeof(dds_publisher_t));
    if (pub == NULL) {
        return DDS_ENTITY_INVALID; /* 池满 (编译期固定上限) */
    }
    memset(pub, 0, sizeof(*pub));

    pub->guid = participant->guid;
    pub->participant = participant;
    pub->qos = (qos != NULL) ? qos->base : participant->qos;
    pub->active = true;

    /* 挂到参与者发布者链表 */
    pub->next = participant->publishers;
    participant->publishers = pub;
    participant->publisher_count++;

    return (dds_PublisherHandleType)(uintptr_t)pub;
}

dds_SubscriberHandleType dds_create_subscriber(
    dds_DomainParticipantHandleType participant_handle,
    const dds_SubscriberQosType *qos,
    void *listener)
{
    (void)listener;
    dds_domain_participant_t *participant = handle_to_participant(participant_handle);
    if ((participant == NULL) || !dds_runtime_is_participant(participant)) {
        return DDS_ENTITY_INVALID;
    }

    dds_subscriber_t *sub = (dds_subscriber_t *)dds_pool_alloc(s_sub_pool, s_sub_free, DDS_POOL_SUBSCRIBERS, sizeof(dds_subscriber_t));
    if (sub == NULL) {
        return DDS_ENTITY_INVALID; /* 池满 */
    }
    memset(sub, 0, sizeof(*sub));

    sub->guid = participant->guid;
    sub->participant = participant;
    sub->qos = (qos != NULL) ? qos->base : participant->qos;
    sub->active = true;

    sub->next = participant->subscribers;
    participant->subscribers = sub;
    participant->subscriber_count++;

    return (dds_SubscriberHandleType)(uintptr_t)sub;
}

dds_TopicHandleType dds_create_topic(
    dds_DomainParticipantHandleType participant_handle,
    const char *topic_name,
    const char *type_name,
    const dds_TopicQosType *qos,
    void *listener)
{
    (void)listener;
    dds_domain_participant_t *participant = handle_to_participant(participant_handle);
    if ((participant == NULL) || !dds_runtime_is_participant(participant) ||
        (topic_name == NULL) || (type_name == NULL)) {
        return DDS_ENTITY_INVALID;
    }

    dds_topic_t *topic = (dds_topic_t *)dds_pool_alloc(s_topic_pool, s_topic_free, DDS_POOL_TOPICS, sizeof(dds_topic_t));
    if (topic == NULL) {
        return DDS_ENTITY_INVALID; /* 池满 */
    }
    memset(topic, 0, sizeof(*topic));

    topic->guid = participant->guid;
    topic->participant = participant;
    topic->qos = (qos != NULL) ? qos->base : participant->qos;
    strncpy(topic->name, topic_name, sizeof(topic->name) - 1U);
    strncpy(topic->type_name, type_name, sizeof(topic->type_name) - 1U);
    topic->active = true;

    topic->next = participant->topics;
    participant->topics = topic;
    participant->topic_count++;

    return (dds_TopicHandleType)(uintptr_t)topic;
}

/* ============================================================================
 * Writer / Reader 创建
 * ============================================================================ */

dds_DataWriterHandleType dds_create_writer(
    dds_PublisherHandleType publisher_handle,
    dds_TopicHandleType topic_handle,
    const dds_DataWriterQosType *qos,
    void *listener)
{
    (void)listener;
    dds_publisher_t *publisher = handle_to_publisher(publisher_handle);
    dds_topic_t *topic = handle_to_topic(topic_handle);
    if ((publisher == NULL) || !publisher->active ||
        (topic == NULL) || !topic->active) {
        return DDS_ENTITY_INVALID;
    }

    dds_data_writer_t *writer = (dds_data_writer_t *)dds_pool_alloc(s_writer_pool, s_writer_free, DDS_POOL_WRITERS, sizeof(dds_data_writer_t));
    if (writer == NULL) {
        return DDS_ENTITY_INVALID; /* 池满 */
    }
    memset(writer, 0, sizeof(*writer));

    writer->guid = topic->guid;
    writer->publisher = publisher;
    writer->topic = topic;
    writer->qos = (qos != NULL) ? qos->base : publisher->qos;
    writer->active = true;

    /* 初始化 RTPS 状态机 (rtps_state.h 提供) */
    rtps_writer_sm_init(&writer->state_machine, &writer->guid,
                        (writer->qos.history_depth > 0U) ? writer->qos.history_depth : 1U);

    writer->next = publisher->writers;
    publisher->writers = writer;
    publisher->writer_count++;
    topic->writer_ref_count++;

    return (dds_DataWriterHandleType)(uintptr_t)writer;
}

dds_DataReaderHandleType dds_create_reader(
    dds_SubscriberHandleType subscriber_handle,
    dds_TopicHandleType topic_handle,
    const dds_DataReaderQosType *qos,
    void *listener)
{
    (void)listener;
    dds_subscriber_t *subscriber = handle_to_subscriber(subscriber_handle);
    dds_topic_t *topic = handle_to_topic(topic_handle);
    if ((subscriber == NULL) || !subscriber->active ||
        (topic == NULL) || !topic->active) {
        return DDS_ENTITY_INVALID;
    }

    dds_data_reader_t *reader = (dds_data_reader_t *)dds_pool_alloc(s_reader_pool, s_reader_free, DDS_POOL_READERS, sizeof(dds_data_reader_t));
    if (reader == NULL) {
        return DDS_ENTITY_INVALID; /* 池满 */
    }
    memset(reader, 0, sizeof(*reader));

    reader->guid = topic->guid;
    reader->subscriber = subscriber;
    reader->topic = topic;
    reader->qos = (qos != NULL) ? qos->base : subscriber->qos;
    reader->active = true;

    /* 初始化 RTPS 状态机 */
    rtps_reader_sm_init(&reader->state_machine, &reader->guid,
                        (reader->qos.history_depth > 0U) ? reader->qos.history_depth : 1U);

    /* 默认接收缓冲: 静态池 (每个 reader 槽位一个固定 4KB 缓冲) */
    reader->receive_buffer_size = 4096;
    {
        uintptr_t base = (uintptr_t)s_recv_buf_pool;
        uintptr_t p = (uintptr_t)reader;
        uint32_t slot = (uint32_t)((p - (uintptr_t)s_reader_pool) / sizeof(dds_data_reader_t));
        if (slot >= DDS_POOL_READERS) {
            dds_pool_free(s_reader_pool, s_reader_free, DDS_POOL_READERS, sizeof(dds_data_reader_t), reader);
            return DDS_ENTITY_INVALID;
        }
        reader->receive_buffer = &s_recv_buf_pool[slot][0];
    }

    reader->next = subscriber->readers;
    subscriber->readers = reader;
    subscriber->reader_count++;
    topic->reader_ref_count++;

    return (dds_DataReaderHandleType)(uintptr_t)reader;
}

/* ============================================================================
 * 数据操作
 * ============================================================================ */

dds_ReturnCode_t dds_write(dds_DataWriterHandleType writer_handle, const void *data)
{
    dds_data_writer_t *writer = handle_to_writer(writer_handle);
    if ((writer == NULL) || !writer->active || (data == NULL)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    if (writer->sample_size == 0U) {
        /* 样本大小未设置: 调用方需先通过 dds_set_writer_sample_size 设置 */
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 写入 RTPS 状态机 (数据已按 sample_size 序列化) */
    rtps_sequence_number_t seq = {0, 0};
    eth_status_t ret = rtps_writer_sm_write(&writer->state_machine,
                                            (const uint8_t*)data,
                                            writer->sample_size,
                                            &seq);
    if (ret != ETH_OK) {
        return DDS_RETCODE_ERROR;
    }

    writer->samples_written++;
    writer->last_write_time = dds_get_time();

    /* 触发写回调 */
    if (writer->write_callback != NULL) {
        writer->write_callback(writer->write_callback_user_data);
    }

    return DDS_RETCODE_OK;
}

dds_ReturnCode_t dds_take(
    dds_DataReaderHandleType reader_handle,
    void *data,
    dds_SampleInfoType *sample_info,
    uint32_t max_samples,
    uint32_t sample_states,
    uint32_t view_states,
    uint32_t instance_states)
{
    (void)sample_states;
    (void)view_states;
    (void)instance_states;

    dds_data_reader_t *reader = handle_to_reader(reader_handle);
    if ((reader == NULL) || !reader->active || (data == NULL) || (max_samples == 0U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 从 reader 接收缓冲取样本 (RTPS 数据经 dds_runtime_handle_rtps_data 写入) */
    if (reader->samples_received == 0U) {
        return DDS_RETCODE_NO_DATA;
    }

    /* 简化: 单样本取出 (完整实现见 pubsub/dds_reader.c) */
    memcpy(data, reader->receive_buffer, (reader->receive_buffer_size > 0U) ? reader->receive_buffer_size : 0U);

    if (sample_info != NULL) {
        memset(sample_info, 0, sizeof(*sample_info));
        sample_info->valid_data = true;
        sample_info->source_timestamp = reader->last_read_time;
        sample_info->sample_state = 0x01; /* DDS_READ_SAMPLE_STATE */
        sample_info->view_state = 0x01;
        sample_info->instance_state = 0x01;
    }

    reader->samples_received = 0;
    reader->last_read_time = dds_get_time();

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 实体删除
 * ============================================================================ */

dds_ReturnCode_t dds_delete(dds_EntityHandleType entity)
{
    if (entity == DDS_ENTITY_INVALID) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 区分实体类型: 仅当指针确实属于已注册参与者时才走 participant 分支
     * (不能用结构体字段偏移猜测 — publisher/subscriber 等首字段布局不同) */
    dds_domain_participant_t *participant = handle_to_participant(entity);
    if ((participant != NULL) && dds_runtime_is_participant(participant)) {
        /* 释放子实体 */
        dds_publisher_t *pub = participant->publishers;
        while (pub != NULL) {
            dds_publisher_t *next = pub->next;
            dds_data_writer_t *w = pub->writers;
            while (w != NULL) {
                dds_data_writer_t *wn = w->next;
                dds_pool_free(s_writer_pool, s_writer_free, DDS_POOL_WRITERS, sizeof(dds_data_writer_t), w);
                w = wn;
            }
            dds_pool_free(s_pub_pool, s_pub_free, DDS_POOL_PUBLISHERS, sizeof(dds_publisher_t), pub);
            pub = next;
        }
        dds_subscriber_t *sub = participant->subscribers;
        while (sub != NULL) {
            dds_subscriber_t *next = sub->next;
            dds_data_reader_t *r = sub->readers;
            while (r != NULL) {
                dds_data_reader_t *rn = r->next;
                /* 接收缓冲为静态池, 无需释放 */
                dds_pool_free(s_reader_pool, s_reader_free, DDS_POOL_READERS, sizeof(dds_data_reader_t), r);
                r = rn;
            }
            dds_pool_free(s_sub_pool, s_sub_free, DDS_POOL_SUBSCRIBERS, sizeof(dds_subscriber_t), sub);
            sub = next;
        }
        dds_topic_t *topic = participant->topics;
        while (topic != NULL) {
            dds_topic_t *next = topic->next;
            dds_pool_free(s_topic_pool, s_topic_free, DDS_POOL_TOPICS, sizeof(dds_topic_t), topic);
            topic = next;
        }

        return (dds_runtime_delete_participant(participant) == ETH_OK)
            ? DDS_RETCODE_OK : DDS_RETCODE_ERROR;
    }

    /* 其它实体类型由 pubsub 层处理 (dds_entity.c) */
    return dds_entity_delete(entity);
}

/* ============================================================================
 * QoS 访问
 * ============================================================================ */

dds_ReturnCode_t dds_get_qos(dds_EntityHandleType entity, void *qos)
{
    if ((entity == DDS_ENTITY_INVALID) || (qos == NULL)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    dds_domain_participant_t *participant = handle_to_participant(entity);
    if ((participant != NULL) && dds_runtime_is_participant(participant)) {
        dds_DomainParticipantQosType *dpq = (dds_DomainParticipantQosType*)qos;
        dpq->base = participant->qos;
        return DDS_RETCODE_OK;
    }

    dds_publisher_t *pub = handle_to_publisher(entity);
    if ((pub != NULL) && pub->active) {
        dds_PublisherQosType *pq = (dds_PublisherQosType*)qos;
        pq->base = pub->qos;
        return DDS_RETCODE_OK;
    }

    dds_subscriber_t *sub = handle_to_subscriber(entity);
    if ((sub != NULL) && sub->active) {
        dds_SubscriberQosType *sq = (dds_SubscriberQosType*)qos;
        sq->base = sub->qos;
        return DDS_RETCODE_OK;
    }

    return DDS_RETCODE_BAD_PARAMETER;
}

dds_ReturnCode_t dds_set_qos(dds_EntityHandleType entity, const void *qos)
{
    if ((entity == DDS_ENTITY_INVALID) || (qos == NULL)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    dds_domain_participant_t *participant = handle_to_participant(entity);
    if ((participant != NULL) && dds_runtime_is_participant(participant)) {
        const dds_DomainParticipantQosType *dpq = (const dds_DomainParticipantQosType*)qos;
        participant->qos = dpq->base;
        return DDS_RETCODE_OK;
    }

    dds_publisher_t *pub = handle_to_publisher(entity);
    if ((pub != NULL) && pub->active) {
        const dds_PublisherQosType *pq = (const dds_PublisherQosType*)qos;
        pub->qos = pq->base;
        return DDS_RETCODE_OK;
    }

    dds_subscriber_t *sub = handle_to_subscriber(entity);
    if ((sub != NULL) && sub->active) {
        const dds_SubscriberQosType *sq = (const dds_SubscriberQosType*)qos;
        sub->qos = sq->base;
        return DDS_RETCODE_OK;
    }

    return DDS_RETCODE_BAD_PARAMETER;
}

dds_ReturnCode_t dds_get_sample_rejected_status(
    dds_DataReaderHandleType reader_handle,
    void *status)
{
    (void)status;
    dds_data_reader_t *reader = handle_to_reader(reader_handle);
    if ((reader == NULL) || !reader->active) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    /* 简化: 返回 OK, 详细状态统计由 pubsub 层维护 */
    return DDS_RETCODE_OK;
}

dds_ReturnCode_t dds_set_writer_sample_size(
    dds_DataWriterHandleType writer_handle,
    uint32_t sample_size)
{
    dds_data_writer_t *writer = handle_to_writer(writer_handle);
    if ((writer == NULL) || !writer->active || (sample_size == 0U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    writer->sample_size = sample_size;
    return DDS_RETCODE_OK;
}
