/** @file microdds.h
 * @brief Micro-DDS 主API头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 轻量级DDS实现 - 专为资源受限MCU设计
 * 内存占用目标: ROM < 50KB, RAM < 16KB
 */

#ifndef MICRODDS_H
#define MICRODDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microdds/types.h"
#include "microdds/qos.h"

/* ============================================================================
 * 域参与者API
 * ============================================================================ */

/** @brief DDS域参与者句柄 */
typedef struct DDS_DomainParticipantImpl* DDS_DomainParticipant;

/** @brief 创建域参与者
 * @param domain_id 域ID
 * @param qos QoS配置，可为NULL使用默认值
 * @return 域参与者句柄，失败返回NULL
 */
DDS_DomainParticipant DDS_DomainParticipant_create(
    DDS_DomainId_t domain_id,
    const DDS_DomainParticipantQos* qos);

/** @brief 删除域参与者
 * @param participant 域参与者句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DomainParticipant_delete(
    DDS_DomainParticipant participant);

/** @brief 获取域参与者QoS
 * @param participant 域参与者句柄
 * @param qos 输出QoS配置
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DomainParticipant_get_qos(
    DDS_DomainParticipant participant,
    DDS_DomainParticipantQos* qos);

/** @brief 设置域参与者QoS
 * @param participant 域参与者句柄
 * @param qos QoS配置
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DomainParticipant_set_qos(
    DDS_DomainParticipant participant,
    const DDS_DomainParticipantQos* qos);

/* ============================================================================
 * 主题API
 * ============================================================================ */

/** @brief DDS主题句柄 */
typedef struct DDS_TopicImpl* DDS_Topic;

/** @brief 创建主题
 * @param participant 域参与者句柄
 * @param name 主题名称
 * @param type_name 类型名称
 * @param qos QoS配置，可为NULL使用默认值
 * @return 主题句柄，失败返回NULL
 */
DDS_Topic DDS_Topic_create(
    DDS_DomainParticipant participant,
    const char* name,
    const char* type_name,
    const DDS_TopicQos* qos);

/** @brief 删除主题
 * @param topic 主题句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_Topic_delete(DDS_Topic topic);

/** @brief 获取主题名称
 * @param topic 主题句柄
 * @return 主题名称
 */
const char* DDS_Topic_get_name(DDS_Topic topic);

/** @brief 获取类型名称
 * @param topic 主题句柄
 * @return 类型名称
 */
const char* DDS_Topic_get_type_name(DDS_Topic topic);

/* ============================================================================
 * 发布者API
 * ============================================================================ */

/** @brief DDS发布者句柄 */
typedef struct DDS_PublisherImpl* DDS_Publisher;

/** @brief 创建发布者
 * @param participant 域参与者句柄
 * @param qos QoS配置，可为NULL使用默认值
 * @return 发布者句柄，失败返回NULL
 */
DDS_Publisher DDS_Publisher_create(
    DDS_DomainParticipant participant,
    const DDS_PublisherQos* qos);

/** @brief 删除发布者
 * @param publisher 发布者句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_Publisher_delete(DDS_Publisher publisher);

/* ============================================================================
 * 订阅者API
 * ============================================================================ */

/** @brief DDS订阅者句柄 */
typedef struct DDS_SubscriberImpl* DDS_Subscriber;

/** @brief 创建订阅者
 * @param participant 域参与者句柄
 * @param qos QoS配置，可为NULL使用默认值
 * @return 订阅者句柄，失败返回NULL
 */
DDS_Subscriber DDS_Subscriber_create(
    DDS_DomainParticipant participant,
    const DDS_SubscriberQos* qos);

/** @brief 删除订阅者
 * @param subscriber 订阅者句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_Subscriber_delete(DDS_Subscriber subscriber);

/* ============================================================================
 * 数据写入器API
 * ============================================================================ */

/** @brief DDS数据写入器句柄 */
typedef struct DDS_DataWriterImpl* DDS_DataWriter;

/** @brief 创建数据写入器
 * @param publisher 发布者句柄
 * @param topic 主题句柄
 * @param qos QoS配置，可为NULL使用默认值
 * @return 数据写入器句柄，失败返回NULL
 */
DDS_DataWriter DDS_DataWriter_create(
    DDS_Publisher publisher,
    DDS_Topic topic,
    const DDS_DataWriterQos* qos);

/** @brief 删除数据写入器
 * @param writer 数据写入器句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataWriter_delete(DDS_DataWriter writer);

/** @brief 写入数据
 * @param writer 数据写入器句柄
 * @param data 数据指针
 * @param handle 实例句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataWriter_write(
    DDS_DataWriter writer,
    const void* data,
    DDS_InstanceHandle_t handle);

/* ============================================================================
 * 数据读取器API
 * ============================================================================ */

/** @brief DDS数据读取器句柄 */
typedef struct DDS_DataReaderImpl* DDS_DataReader;

/** @brief 创建数据读取器
 * @param subscriber 订阅者句柄
 * @param topic 主题句柄
 * @param qos QoS配置，可为NULL使用默认值
 * @return 数据读取器句柄，失败返回NULL
 */
DDS_DataReader DDS_DataReader_create(
    DDS_Subscriber subscriber,
    DDS_Topic topic,
    const DDS_DataReaderQos* qos);

/** @brief 删除数据读取器
 * @param reader 数据读取器句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataReader_delete(DDS_DataReader reader);

/** @brief 读取数据
 * @param reader 数据读取器句柄
 * @param data_samples 数据样本数组
 * @param sample_infos 样本信息数组
 * @param max_samples 最大样本数
 * @return 实际读取的样本数
 */
int32_t DDS_DataReader_read(
    DDS_DataReader reader,
    void** data_samples,
    DDS_SampleInfo* sample_infos,
    int32_t max_samples);

/** @brief 获取数据
 * @param reader 数据读取器句柄
 * @param data_samples 数据样本数组
 * @param sample_infos 样本信息数组
 * @param max_samples 最大样本数
 * @return 实际获取的样本数
 */
int32_t DDS_DataReader_take(
    DDS_DataReader reader,
    void** data_samples,
    DDS_SampleInfo* sample_infos,
    int32_t max_samples);

/** @brief 返回已读取的样本
 * @param reader 数据读取器句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataReader_return_loan(
    DDS_DataReader reader);

/* ============================================================================
 * 等待集API (简化版)
 * ============================================================================ */

/** @brief 条件句柄 */
typedef struct DDS_ConditionImpl* DDS_Condition;

/** @brief 等待集句柄 */
typedef struct DDS_WaitSetImpl* DDS_WaitSet;

/** @brief 创建等待集
 * @return 等待集句柄
 */
DDS_WaitSet DDS_WaitSet_create(void);

/** @brief 删除等待集
 * @param waitset 等待集句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_WaitSet_delete(DDS_WaitSet waitset);

/** @brief 附加条件
 * @param waitset 等待集句柄
 * @param condition 条件句柄
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_WaitSet_attach_condition(
    DDS_WaitSet waitset,
    DDS_Condition condition);

/** @brief 等待条件触发
 * @param waitset 等待集句柄
 * @param active_conditions 活动条件数组
 * @param max_conditions 最大条件数
 * @param timeout 超时时间
 * @return 触发的条件数
 */
int32_t DDS_WaitSet_wait(
    DDS_WaitSet waitset,
    DDS_Condition* active_conditions,
    int32_t max_conditions,
    DDS_Duration_t timeout);

/* ============================================================================
 * 监听器回调类型定义
 * ============================================================================ */

/** @brief 数据可用回调 */
typedef void (*DDS_DataAvailableCallback)(
    DDS_DataReader reader);

/** @brief 设置数据可用回调
 * @param reader 数据读取器句柄
 * @param callback 回调函数
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t DDS_DataReader_set_data_available_callback(
    DDS_DataReader reader,
    DDS_DataAvailableCallback callback);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/** @brief 初始化Micro-DDS库
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t MicroDDS_init(void);

/** @brief 关闭Micro-DDS库
 * @return DDS_RETCODE_OK 成功
 */
DDS_ReturnCode_t MicroDDS_shutdown(void);

/** @brief 获取版本字符串
 * @return 版本字符串
 */
const char* MicroDDS_get_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_H */
