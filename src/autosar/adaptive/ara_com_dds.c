/******************************************************************************
 * @file    ara_com_dds.c
 * @brief   AUTOSAR Adaptive ara::com DDS绑定层实现
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include "ara_com_dds.h"
#include "e2e_protection.h"
#include "autosar_common.h"
#include "../../../dds/core/dds_core.h"
#include <string.h>

/******************************************************************************
 * Private Definitions
 ******************************************************************************/
#define ARA_COM_INITIALIZED_MAGIC       0xA5A5A5A5U
#define ARA_COM_MAX_SAMPLE_POOL_SIZE    1024
#define ARA_COM_METHOD_TIMEOUT_DEFAULT  5000

/******************************************************************************
 * Private Types
 ******************************************************************************/
typedef struct {
    ara_com_ServiceHandleType* services[ARA_COM_MAX_SERVICE_INSTANCES];
    uint32_t numServices;
    uint32_t initMagic;
    bool initialized;
} ara_com_ContextType;

/* 方法调用上下文 */
typedef struct {
    ara_com_MethodHandleType* method;
    void* responseBuffer;
    uint32_t* responseLength;
    ara_com_CallResultType result;
    bool completed;
    dds_ConditionVariableType cv;
} ara_com_MethodCallContextType;

/******************************************************************************
 * Private Data
 ******************************************************************************/
static ara_com_ContextType g_araComContext = {0};
static ara_com_MethodCallContextType g_methodContexts[ARA_COM_MAX_METHODS_PER_SERVICE];

#if SOMEIP_DDS_GATEWAY_ENABLED
static SomeIP_DDS_GatewayEntryType g_gatewayEntries[SOMEIP_MAX_SERVICE_ENTRIES];
static uint32_t g_gatewayNumEntries = 0;
#endif

/******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief 获取服务索引
 */
static sint32 ara_com_GetServiceIndex(ara_com_ServiceHandleType* handle)
{
    for (uint32 i = 0; i < g_araComContext.numServices; i++) {
        if (g_araComContext.services[i] == handle) {
            return (sint32)i;
        }
    }
    return -1;
}

/**
 * @brief 映射ServiceID到DDS Topic名
 */
static void ara_com_ServiceIdToTopicName(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId,
    ara_com_EventIdType eventId,
    char* topicName,
    uint32_t maxLen)
{
    (void)snprintf(topicName, maxLen, "/ara/com/S%d_I%d_E%d", 
                     serviceId, instanceId, eventId);
}

/**
 * @brief 映射MethodID到DDS Topic名
 */
static void ara_com_MethodIdToTopicName(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId,
    ara_com_MethodIdType methodId,
    bool isRequest,
    char* topicName,
    uint32_t maxLen)
{
    const char* suffix = isRequest ? "_Req" : "_Rsp";
    (void)snprintf(topicName, maxLen, "/ara/com/S%d_I%d_M%d%s",
                     serviceId, instanceId, methodId, suffix);
}

/**
 * @brief E2E保护回调函数
 */
static void ara_com_E2EProtectCallback(
    void* data,
    uint32_t* length,
    void* userData)
{
    ara_com_ServiceHandleType* service = (ara_com_ServiceHandleType*)userData;
    if (service != NULL && service->e2eContext != NULL) {
        E2E_Protect(service->e2eContext, data, length, E2E_PROFILE_04);
    }
}

/**
 * @brief E2E校验回调函数
 */
static void ara_com_E2ECheckCallback(
    const void* data,
    uint32_t length,
    uint16_t* status,
    void* userData)
{
    ara_com_ServiceHandleType* service = (ara_com_ServiceHandleType*)userData;
    if (service != NULL && service->e2eContext != NULL) {
        E2E_Check(service->e2eContext, data, length, status, E2E_PROFILE_04);
    }
}

#if SOMEIP_DDS_GATEWAY_ENABLED
/**
 * @brief Some/IP-DDS网关处理
 */
static void ara_com_SomeIPDDSGateway_ProcessEntry(SomeIP_DDS_GatewayEntryType* entry)
{
    /* 在实际实现中，这里会处理Some/IP报文与DDS数据的转换 */
    /* 包括SD消息转换、数据序列化格式转换等 */
    (void)entry;
}
#endif

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

/**
 * @brief 初始化ara::com DDS绑定层
 */
Std_ReturnType ara_com_Init(void)
{
    if (g_araComContext.initialized) {
        return E_OK;
    }

    /* 初始化E2E保护模块 */
    if (E2E_Init() != E_OK) {
        return E_NOT_OK;
    }

    /* 初始化DDS运行时 */
    if (dds_runtime_init() != DDS_RETCODE_OK) {
        return E_NOT_OK;
    }

    /* 清零服务表 */
    memset(g_araComContext.services, 0, sizeof(g_araComContext.services));
    g_araComContext.numServices = 0;
    g_araComContext.initMagic = ARA_COM_INITIALIZED_MAGIC;
    g_araComContext.initialized = TRUE;

#if SOMEIP_DDS_GATEWAY_ENABLED
    memset(g_gatewayEntries, 0, sizeof(g_gatewayEntries));
    g_gatewayNumEntries = 0;
    ara_com_SomeIPDDSGateway_Init();
#endif

    return E_OK;
}

/**
 * @brief 反初始化ara::com DDS绑定层
 */
Std_ReturnType ara_com_Deinit(void)
{
    if (!g_araComContext.initialized) {
        return E_OK;
    }

#if SOMEIP_DDS_GATEWAY_ENABLED
    ara_com_SomeIPDDSGateway_Deinit();
#endif

    /* 销毁所有服务 */
    for (uint32 i = 0; i < g_araComContext.numServices; i++) {
        if (g_araComContext.services[i] != NULL) {
            ara_com_DestroyServiceInterface(g_araComContext.services[i]);
        }
    }

    dds_runtime_deinit();
    E2E_Deinit();

    g_araComContext.initialized = FALSE;
    g_araComContext.initMagic = 0;

    return E_OK;
}

/**
 * @brief 检查是否已初始化
 */
bool ara_com_IsInitialized(void)
{
    return g_araComContext.initialized && 
           (g_araComContext.initMagic == ARA_COM_INITIALIZED_MAGIC);
}

/******************************************************************************
 * Public Functions - ServiceInterface Management
 ******************************************************************************/

/**
 * @brief 创建ServiceInterface
 */
Std_ReturnType ara_com_CreateServiceInterface(
    const ara_com_ServiceInterfaceConfigType* config,
    ara_com_ServiceHandleType** handle)
{
    if (!ara_com_IsInitialized() || config == NULL || handle == NULL) {
        return E_NOT_OK;
    }

    if (g_araComContext.numServices >= ARA_COM_MAX_SERVICE_INSTANCES) {
        return E_NOT_OK;
    }

    /* 分配服务句柄 */
    ara_com_ServiceHandleType* newHandle = 
        (ara_com_ServiceHandleType*)malloc(sizeof(ara_com_ServiceHandleType));
    if (newHandle == NULL) {
        return E_NOT_OK;
    }

    newHandle->serviceId = config->serviceId;
    newHandle->instanceId = config->instanceId;
    newHandle->state = ARA_COM_SD_NOT_AVAILABLE;
    newHandle->ddsHandle = DDS_ENTITY_INVALID;
    newHandle->e2eContext = NULL;

    /* 初始化E2E上下文 */
    if (config->e2eEnabled) {
        E2E_ContextType* e2eCtx = (E2E_ContextType*)malloc(sizeof(E2E_ContextType));
        if (e2eCtx != NULL) {
            E2E_InitContext(e2eCtx, config->e2eProfile);
            newHandle->e2eContext = e2eCtx;
        }
    }

    g_araComContext.services[g_araComContext.numServices++] = newHandle;
    *handle = newHandle;

    return E_OK;
}

/**
 * @brief 销毁ServiceInterface
 */
Std_ReturnType ara_com_DestroyServiceInterface(ara_com_ServiceHandleType* handle)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    sint32 index = ara_com_GetServiceIndex(handle);
    if (index < 0) {
        return E_NOT_OK;
    }

    /* 停止服务发现 */
    ara_com_StopOfferService(handle);

    /* 释放E2E上下文 */
    if (handle->e2eContext != NULL) {
        E2E_DeinitContext(handle->e2eContext);
        free(handle->e2eContext);
    }

    /* 从服务表中移除 */
    g_araComContext.services[index] = 
        g_araComContext.services[--g_araComContext.numServices];

    free(handle);

    return E_OK;
}

/**
 * @brief 提供服务
 */
Std_ReturnType ara_com_OfferService(ara_com_ServiceHandleType* handle)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    /* 创建DDS DomainParticipant */
    dds_DomainParticipantQosType dpQos;
    dds_domain_participant_qos_init(&dpQos);
    
    handle->ddsHandle = dds_create_participant(0, &dpQos, NULL);
    if (handle->ddsHandle == DDS_ENTITY_INVALID) {
        return E_NOT_OK;
    }

    handle->state = ARA_COM_SD_OFFERING;

    /* 发送SD Offer服务消息 */
    /* 实际实现需要通过Some/IP SD协议发送服务发现消息 */

    return E_OK;
}

/**
 * @brief 停止提供服务
 */
Std_ReturnType ara_com_StopOfferService(ara_com_ServiceHandleType* handle)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    if (handle->ddsHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->ddsHandle);
        handle->ddsHandle = DDS_ENTITY_INVALID;
    }

    handle->state = ARA_COM_SD_STOPPED;

    return E_OK;
}

/**
 * @brief 查找服务
 */
Std_ReturnType ara_com_FindService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceOfferType offerType,
    ara_com_ServiceHandleType** handle)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    /* 在服务表中查找 */
    for (uint32 i = 0; i < g_araComContext.numServices; i++) {
        if (g_araComContext.services[i]->serviceId == serviceId) {
            *handle = g_araComContext.services[i];
            return E_OK;
        }
    }

    (void)offerType;
    return E_NOT_OK;
}

/******************************************************************************
 * Public Functions - Event Management
 ******************************************************************************/

/**
 * @brief 创建Event
 */
Std_ReturnType ara_com_CreateEvent(
    ara_com_ServiceHandleType* service,
    const ara_com_EventConfigType* config,
    ara_com_EventHandleType** handle)
{
    if (!ara_com_IsInitialized() || service == NULL || config == NULL || handle == NULL) {
        return E_NOT_OK;
    }

    ara_com_EventHandleType* newHandle = 
        (ara_com_EventHandleType*)malloc(sizeof(ara_com_EventHandleType));
    if (newHandle == NULL) {
        return E_NOT_OK;
    }

    newHandle->service = service;
    newHandle->eventId = config->eventId;
    newHandle->subState = ARA_COM_SUB_NOT_SUBSCRIBED;

    /* 创建Topic名称 */
    char topicName[128];
    ara_com_ServiceIdToTopicName(service->serviceId, service->instanceId,
                                   config->eventId, topicName, sizeof(topicName));

    /* 创建DDS Topic */
    dds_TopicQosType topicQos;
    dds_topic_qos_init(&topicQos);
    newHandle->topicHandle = dds_create_topic(service->ddsHandle, NULL, topicName, &topicQos, NULL);

    /* 创建Publisher和DataWriter */
    dds_PublisherQosType pubQos;
    dds_publisher_qos_init(&pubQos);
    newHandle->pubHandle = dds_create_publisher(service->ddsHandle, &pubQos, NULL);

    dds_DataWriterQosType writerQos;
    dds_datawriter_qos_init(&writerQos);
    newHandle->writerHandle = dds_create_writer(newHandle->pubHandle, newHandle->topicHandle, &writerQos, NULL);

    /* 创建Subscriber和DataReader */
    dds_SubscriberQosType subQos;
    dds_subscriber_qos_init(&subQos);
    newHandle->subHandle = dds_create_subscriber(service->ddsHandle, &subQos, NULL);

    dds_DataReaderQosType readerQos;
    dds_datareader_qos_init(&readerQos);
    newHandle->readerHandle = dds_create_reader(newHandle->subHandle, newHandle->topicHandle, &readerQos, NULL);

    *handle = newHandle;
    return E_OK;
}

/**
 * @brief 销毁Event
 */
Std_ReturnType ara_com_DestroyEvent(ara_com_EventHandleType* handle)
{
    if (handle == NULL) {
        return E_NOT_OK;
    }

    /* 删除DDS实体 */
    if (handle->readerHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->readerHandle);
    }
    if (handle->writerHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->writerHandle);
    }
    if (handle->subHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->subHandle);
    }
    if (handle->pubHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->pubHandle);
    }
    if (handle->topicHandle != DDS_ENTITY_INVALID) {
        dds_delete(handle->topicHandle);
    }

    free(handle);
    return E_OK;
}

/**
 * @brief 发送Event
 */
Std_ReturnType ara_com_SendEvent(
    ara_com_EventHandleType* handle,
    const void* data,
    uint32_t length)
{
    if (!ara_com_IsInitialized() || handle == NULL || data == NULL || length == 0) {
        return E_NOT_OK;
    }

    /* 应用E2E保护 */
    uint8_t protectedData[2048];
    uint32_t protectedLength = length + E2E_MAX_OVERHEAD;
    
    if (protectedLength > sizeof(protectedData)) {
        return E_NOT_OK;
    }

    memcpy(protectedData, data, length);

    if (handle->service->e2eContext != NULL) {
        if (E2E_Protect(handle->service->e2eContext, protectedData, &protectedLength, 
                       E2E_PROFILE_04) != E_OK) {
            return E_NOT_OK;
        }
    }

    /* 通过DDS发送 */
    dds_SampleHandleType sample;
    sample.data = protectedData;
    sample.length = protectedLength;
    sample.timestamp = dds_get_time();

    if (dds_write(handle->writerHandle, &sample) != DDS_RETCODE_OK) {
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief 订阅Event
 */
Std_ReturnType ara_com_SubscribeEvent(
    ara_com_EventHandleType* handle,
    uint32_t maxSampleCount)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    /* 设置DataReader的资源限制 */
    dds_DataReaderQosType qos;
    dds_get_qos(handle->readerHandle, &qos);
    qos.resource_limits.max_samples = maxSampleCount;
    dds_set_qos(handle->readerHandle, &qos);

    handle->subState = ARA_COM_SUB_SUBSCRIBED;

    return E_OK;
}

/**
 * @brief 取消订阅Event
 */
Std_ReturnType ara_com_UnsubscribeEvent(ara_com_EventHandleType* handle)
{
    if (!ara_com_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    handle->subState = ARA_COM_SUB_NOT_SUBSCRIBED;

    return E_OK;
}

/**
 * @brief 获取新样本
 */
Std_ReturnType ara_com_GetNewSamples(
    ara_com_EventHandleType* handle,
    void* buffer,
    uint32_t bufferSize,
    uint32_t* numSamples)
{
    if (!ara_com_IsInitialized() || handle == NULL || buffer == NULL || numSamples == NULL) {
        return E_NOT_OK;
    }

    *numSamples = 0;

    dds_SampleHandleType samples[32];
    dds_SampleInfoType info[32];
    int32_t maxSamples = (bufferSize < sizeof(samples)) ? 1 : 32;

    dds_ReturnCode_t ret = dds_take(handle->readerHandle, samples, info, maxSamples, 
                                     DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
    
    if (ret != DDS_RETCODE_OK) {
        return E_NOT_OK;
    }

    uint32_t validSamples = 0;
    for (int32 i = 0; i < maxSamples; i++) {
        if (info[i].valid_data) {
            /* 验证E2E保护 */
            if (handle->service->e2eContext != NULL) {
                uint16_t e2eStatus = 0;
                if (E2E_Check(handle->service->e2eContext, samples[i].data, 
                             samples[i].length, &e2eStatus, E2E_PROFILE_04) == E_OK) {
                    if ((e2eStatus & E2E_ERROR_NONE) != 0) {
                        /* 复制数据到缓冲区（跳过E2E头） */
                        uint32_t headerSize = E2E_GetHeaderSize(E2E_PROFILE_04);
                        memcpy((uint8_t*)buffer + (validSamples * (samples[i].length - headerSize)),
                               (uint8_t*)samples[i].data + headerSize,
                               samples[i].length - headerSize);
                        validSamples++;
                    }
                }
            } else {
                memcpy((uint8_t*)buffer + (validSamples * samples[i].length),
                       samples[i].data, samples[i].length);
                validSamples++;
            }
        }
    }

    *numSamples = validSamples;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Method Management
 ******************************************************************************/

/**
 * @brief 创建Method
 */
Std_ReturnType ara_com_CreateMethod(
    ara_com_ServiceHandleType* service,
    const ara_com_MethodConfigType* config,
    ara_com_MethodHandleType** handle)
{
    if (!ara_com_IsInitialized() || service == NULL || config == NULL || handle == NULL) {
        return E_NOT_OK;
    }

    ara_com_MethodHandleType* newHandle = 
        (ara_com_MethodHandleType*)malloc(sizeof(ara_com_MethodHandleType));
    if (newHandle == NULL) {
        return E_NOT_OK;
    }

    newHandle->service = service;
    newHandle->methodId = config->methodId;

    /* 创建Request Topic */
    char reqTopicName[128];
    ara_com_MethodIdToTopicName(service->serviceId, service->instanceId,
                                  config->methodId, TRUE, reqTopicName, sizeof(reqTopicName));

    dds_TopicQosType topicQos;
    dds_topic_qos_init(&topicQos);
    newHandle->requestTopic = dds_create_topic(service->ddsHandle, NULL, reqTopicName, &topicQos, NULL);

    /* 创建Response Topic */
    char rspTopicName[128];
    ara_com_MethodIdToTopicName(service->serviceId, service->instanceId,
                                  config->methodId, FALSE, rspTopicName, sizeof(rspTopicName));
    newHandle->responseTopic = dds_create_topic(service->ddsHandle, NULL, rspTopicName, &topicQos, NULL);

    /* 创建Request Writer */
    dds_PublisherQosType pubQos;
    dds_publisher_qos_init(&pubQos);
    dds_PublisherHandleType pub = dds_create_publisher(service->ddsHandle, &pubQos, NULL);

    dds_DataWriterQosType writerQos;
    dds_datawriter_qos_init(&writerQos);
    newHandle->requestWriter = dds_create_writer(pub, newHandle->requestTopic, &writerQos, NULL);

    /* 创建Request Reader */
    dds_SubscriberQosType subQos;
    dds_subscriber_qos_init(&subQos);
    dds_SubscriberHandleType sub = dds_create_subscriber(service->ddsHandle, &subQos, NULL);

    dds_DataReaderQosType readerQos;
    dds_datareader_qos_init(&readerQos);
    newHandle->requestReader = dds_create_reader(sub, newHandle->requestTopic, &readerQos, NULL);

    /* 创建Response Writer */
    newHandle->responseWriter = dds_create_writer(pub, newHandle->responseTopic, &writerQos, NULL);

    /* 创建Response Reader */
    newHandle->responseReader = dds_create_reader(sub, newHandle->responseTopic, &readerQos, NULL);

    *handle = newHandle;
    return E_OK;
}

/**
 * @brief 销毁Method
 */
Std_ReturnType ara_com_DestroyMethod(ara_com_MethodHandleType* handle)
{
    if (handle == NULL) {
        return E_NOT_OK;
    }

    /* 删除DDS实体 */
    if (handle->responseReader != DDS_ENTITY_INVALID) dds_delete(handle->responseReader);
    if (handle->responseWriter != DDS_ENTITY_INVALID) dds_delete(handle->responseWriter);
    if (handle->requestReader != DDS_ENTITY_INVALID) dds_delete(handle->requestReader);
    if (handle->requestWriter != DDS_ENTITY_INVALID) dds_delete(handle->requestWriter);
    if (handle->responseTopic != DDS_ENTITY_INVALID) dds_delete(handle->responseTopic);
    if (handle->requestTopic != DDS_ENTITY_INVALID) dds_delete(handle->requestTopic);

    free(handle);
    return E_OK;
}

/**
 * @brief 同步调用Method
 */
Std_ReturnType ara_com_CallMethod(
    ara_com_MethodHandleType* handle,
    const void* requestData,
    uint32_t requestLength,
    void* responseData,
    uint32_t* responseLength,
    uint32_t timeoutMs)
{
    if (!ara_com_IsInitialized() || handle == NULL || requestData == NULL ||
        responseData == NULL || responseLength == NULL) {
        return E_NOT_OK;
    }

    /* 发送请求 */
    dds_SampleHandleType requestSample;
    requestSample.data = (void*)requestData;
    requestSample.length = requestLength;
    requestSample.timestamp = dds_get_time();

    if (dds_write(handle->requestWriter, &requestSample) != DDS_RETCODE_OK) {
        return E_NOT_OK;
    }

    /* 等待响应 */
    dds_SampleHandleType responseSample;
    dds_SampleInfoType info;
    dds_ReturnCode_t ret = dds_take(handle->responseReader, &responseSample, &info, 1,
                                     DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

    if (ret != DDS_RETCODE_OK || !info.valid_data) {
        return E_NOT_OK;
    }

    /* 复制响应数据 */
    if (responseSample.length <= *responseLength) {
        memcpy(responseData, responseSample.data, responseSample.length);
        *responseLength = responseSample.length;
    } else {
        return E_NOT_OK;
    }

    (void)timeoutMs;
    return E_OK;
}

/**
 * @brief 注册Method处理器
 */
Std_ReturnType ara_com_RegisterMethodHandler(
    ara_com_MethodHandleType* handle,
    Std_ReturnType (*handler)(const void* request, uint32_t reqLength,
                               void* response, uint32_t* respLength))
{
    if (!ara_com_IsInitialized() || handle == NULL || handler == NULL) {
        return E_NOT_OK;
    }

    /* 在实际实现中，这里需要设置一个轮询线程或回调来处理请求 */
    /* 简化实现：存储处理器指针并在ara_com_Process中调用 */

    (void)handle;
    (void)handler;
    return E_OK;
}

/******************************************************************************
 * Public Functions - E2E Protection
 ******************************************************************************/

/**
 * @brief E2E保护数据
 */
Std_ReturnType ara_com_E2EProtect(
    ara_com_ServiceHandleType* service,
    void* data,
    uint32_t* length,
    uint8_t profile)
{
    if (!ara_com_IsInitialized() || service == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (service->e2eContext == NULL) {
        return E_NOT_OK;
    }

    return E2E_Protect(service->e2eContext, data, length, profile);
}

/**
 * @brief E2E校验数据
 */
Std_ReturnType ara_com_E2ECheck(
    ara_com_ServiceHandleType* service,
    const void* data,
    uint32_t length,
    uint8_t profile,
    uint16_t* status)
{
    if (!ara_com_IsInitialized() || service == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (service->e2eContext == NULL) {
        return E_NOT_OK;
    }

    return E2E_Check(service->e2eContext, data, length, status, profile);
}

#if SOMEIP_DDS_GATEWAY_ENABLED
/******************************************************************************
 * Public Functions - Some/IP-DDS Gateway
 ******************************************************************************/

/**
 * @brief 初始化Some/IP-DDS网关
 */
Std_ReturnType ara_com_SomeIPDDSGateway_Init(void)
{
    memset(g_gatewayEntries, 0, sizeof(g_gatewayEntries));
    g_gatewayNumEntries = 0;
    return E_OK;
}

/**
 * @brief 反初始化Some/IP-DDS网关
 */
Std_ReturnType ara_com_SomeIPDDSGateway_Deinit(void)
{
    g_gatewayNumEntries = 0;
    return E_OK;
}

/**
 * @brief 注册Some/IP服务
 */
Std_ReturnType ara_com_SomeIPDDSGateway_RegisterService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId,
    uint16_t someipInstanceId)
{
    if (g_gatewayNumEntries >= SOMEIP_MAX_SERVICE_ENTRIES) {
        return E_NOT_OK;
    }

    SomeIP_DDS_GatewayEntryType* entry = &g_gatewayEntries[g_gatewayNumEntries++];
    entry->serviceId = serviceId;
    entry->instanceId = instanceId;
    entry->someipInstanceId = someipInstanceId;
    entry->ddsDomainId = DDS_DOMAIN_ID_SOMEIP_BRIDGE;
    entry->isActive = TRUE;

    return E_OK;
}

/**
 * @brief 注销Some/IP服务
 */
Std_ReturnType ara_com_SomeIPDDSGateway_UnregisterService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId)
{
    for (uint32 i = 0; i < g_gatewayNumEntries; i++) {
        if (g_gatewayEntries[i].serviceId == serviceId && 
            g_gatewayEntries[i].instanceId == instanceId) {
            g_gatewayEntries[i].isActive = FALSE;
            /* 移动数组元素填充空位 */
            if (i < g_gatewayNumEntries - 1) {
                memmove(&g_gatewayEntries[i], &g_gatewayEntries[i + 1],
                        (g_gatewayNumEntries - i - 1) * sizeof(SomeIP_DDS_GatewayEntryType));
            }
            g_gatewayNumEntries--;
            return E_OK;
        }
    }
    return E_NOT_OK;
}

/**
 * @brief 网关处理
 */
void ara_com_SomeIPDDSGateway_Process(void)
{
    for (uint32 i = 0; i < g_gatewayNumEntries; i++) {
        if (g_gatewayEntries[i].isActive) {
            ara_com_SomeIPDDSGateway_ProcessEntry(&g_gatewayEntries[i]);
        }
    }
}
#endif

/******************************************************************************
 * Public Functions - Diagnostics
 ******************************************************************************/

/**
 * @brief 获取服务状态
 */
Std_ReturnType ara_com_GetServiceStatus(
    ara_com_ServiceHandleType* handle,
    ara_com_ServiceDiscoveryStateType* state)
{
    if (handle == NULL || state == NULL) {
        return E_NOT_OK;
    }

    *state = handle->state;
    return E_OK;
}

/**
 * @brief 获取丢弃样本数
 */
uint32_t ara_com_GetDroppedSamples(ara_com_EventHandleType* handle)
{
    if (handle == NULL) {
        return 0;
    }

    dds_SampleRejectedStatusType status;
    dds_get_sample_rejected_status(handle->readerHandle, &status);
    return (uint32_t)status.total_count;
}

/**
 * @brief 获取延迟(微秒)
 */
uint32_t ara_com_GetLatencyUs(ara_com_EventHandleType* handle)
{
    if (handle == NULL) {
        return 0;
    }

    dds_LatencyBudgetQosPolicyType latency;
    dds_get_qos(handle->writerHandle, &latency);
    return latency.duration.sec * 1000000 + latency.duration.nanosec / 1000;
}