/******************************************************************************
 * @file    ara_com_dds.h
 * @brief   AUTOSAR Adaptive ara::com DDS绑定层
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ARA_COM_DDS_H
#define ARA_COM_DDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "autosar_types.h"
#include "autosar_errors.h"
#include "dds_types.h"

/* AUTOSAR Adaptive ara::com版本 */
#define ARA_COM_DDS_MAJOR_VERSION       22
#define ARA_COM_DDS_MINOR_VERSION       11
#define ARA_COM_DDS_PATCH_VERSION       0

/* ServiceInterface标识符 */
#define ARA_COM_MAX_SERVICE_INSTANCES   64
#define ARA_COM_MAX_EVENTS_PER_SERVICE  32
#define ARA_COM_MAX_METHODS_PER_SERVICE 32
#define ARA_COM_MAX_FIELDS_PER_SERVICE  16

/* Some/IP-DDS网关配置 */
#define SOMEIP_DDS_GATEWAY_ENABLED      1
#define SOMEIP_MAX_SERVICE_ENTRIES      128
#define DDS_DOMAIN_ID_SOMEIP_BRIDGE     100

/* E2E保护配置 */
#define ARA_COM_E2E_MAX_PROFILES        8
#define ARA_COM_E2E_PROFILE_01          1   /* CRC8 */
#define ARA_COM_E2E_PROFILE_02          2   /* CRC8+Counter */
#define ARA_COM_E2E_PROFILE_04          4   /* CRC32 */
#define ARA_COM_E2E_PROFILE_05          5   /* CRC16 */
#define ARA_COM_E2E_PROFILE_06          6   /* CRC16+Counter */
#define ARA_COM_E2E_PROFILE_07          7   /* CRC32+Counter */
#define ARA_COM_E2E_PROFILE_11          11  /* Dynamic - CRC8+Seq */

/******************************************************************************
 * Data Types
 ******************************************************************************/

/* ServiceInstanceID类型 */
typedef uint16_t ara_com_ServiceInstanceIdType;

/* ServiceID类型 */
typedef uint16_t ara_com_ServiceIdType;

/* EventID类型 */
typedef uint16_t ara_com_EventIdType;

/* MethodID类型 */
typedef uint16_t ara_com_MethodIdType;

/* FieldID类型 */
typedef uint16_t ara_com_FieldIdType;

/* ClientID类型 */
typedef uint32_t ara_com_ClientIdType;

/* SessionID类型 */
typedef uint16_t ara_com_SessionIdType;

/* 服务发现状态 */
typedef enum {
    ARA_COM_SD_NOT_AVAILABLE = 0,
    ARA_COM_SD_AVAILABLE,
    ARA_COM_SD_FINDING,
    ARA_COM_SD_OFFERING,
    ARA_COM_SD_STOPPED
} ara_com_ServiceDiscoveryStateType;

/* 服务提供方式 */
typedef enum {
    ARA_COM_SD_OFFER = 0,
    ARA_COM_SD_REQUIRE
} ara_com_ServiceOfferType;

/* Event订阅状态 */
typedef enum {
    ARA_COM_SUB_NOT_SUBSCRIBED = 0,
    ARA_COM_SUB_SUBSCRIBED,
    ARA_COM_SUB_PENDING
} ara_com_SubscriptionStateType;

/* 调用结果状态 */
typedef enum {
    ARA_COM_RESULT_OK = 0,
    ARA_COM_RESULT_PENDING,
    ARA_COM_RESULT_TIMEOUT,
    ARA_COM_RESULT_COMM_ERROR,
    ARA_COM_RESULT_E2E_ERROR
} ara_com_CallResultType;

/* E2E保护级别 */
typedef enum {
    ARA_COM_E2E_NONE = 0,
    ARA_COM_E2E_PROTECTION,
    ARA_COM_E2E_FULL
} ara_com_E2EProtectionLevelType;

/* ServiceInterface配置 */
typedef struct {
    ara_com_ServiceIdType serviceId;
    ara_com_ServiceInstanceIdType instanceId;
    ara_com_ServiceOfferType offerType;
    uint16_t majorVersion;
    uint32_t minorVersion;
    uint32_t ttl;           /* 生存时间(ms) */
    bool e2eEnabled;
    uint8_t e2eProfile;
} ara_com_ServiceInterfaceConfigType;

/* Event配置 */
typedef struct {
    ara_com_EventIdType eventId;
    uint16_t dataTypeId;
    uint32_t maxDataLength;
    uint32_t sampleBufferSize;
    ara_com_E2EProtectionLevelType e2eLevel;
} ara_com_EventConfigType;

/* Method配置 */
typedef struct {
    ara_com_MethodIdType methodId;
    uint16_t requestTypeId;
    uint16_t responseTypeId;
    uint32_t maxRequestLength;
    uint32_t maxResponseLength;
    uint32_t timeoutMs;
    ara_com_E2EProtectionLevelType e2eLevel;
} ara_com_MethodConfigType;

/* Field配置 */
typedef struct {
    ara_com_FieldIdType fieldId;
    uint16_t dataTypeId;
    uint32_t maxLength;
    bool hasGetter;
    bool hasSetter;
    bool hasNotifier;
    ara_com_E2EProtectionLevelType e2eLevel;
} ara_com_FieldConfigType;

/* 服务句柄 */
typedef struct {
    ara_com_ServiceIdType serviceId;
    ara_com_ServiceInstanceIdType instanceId;
    dds_EntityHandleType ddsHandle;
    ara_com_ServiceDiscoveryStateType state;
    void* e2eContext;
} ara_com_ServiceHandleType;

/* Event句柄 */
typedef struct {
    ara_com_ServiceHandleType* service;
    ara_com_EventIdType eventId;
    dds_TopicHandleType topicHandle;
    dds_PublisherHandleType pubHandle;
    dds_SubscriberHandleType subHandle;
    dds_DataWriterHandleType writerHandle;
    dds_DataReaderHandleType readerHandle;
    ara_com_SubscriptionStateType subState;
} ara_com_EventHandleType;

/* Method句柄 */
typedef struct {
    ara_com_ServiceHandleType* service;
    ara_com_MethodIdType methodId;
    dds_TopicHandleType requestTopic;
    dds_TopicHandleType responseTopic;
    dds_DataWriterHandleType requestWriter;
    dds_DataReaderHandleType requestReader;
    dds_DataWriterHandleType responseWriter;
    dds_DataReaderHandleType responseReader;
} ara_com_MethodHandleType;

/* Field句柄 */
typedef struct {
    ara_com_ServiceHandleType* service;
    ara_com_FieldIdType fieldId;
    ara_com_EventHandleType* notifier;
    ara_com_MethodHandleType* getter;
    ara_com_MethodHandleType* setter;
} ara_com_FieldHandleType;

/* 样本数据头 (用于E2E保护) */
typedef struct {
    uint32_t timestamp;
    uint32_t sequenceNumber;
    uint16_t e2eStatus;
    uint16_t dataLength;
} ara_com_SampleHeaderType;

/* Some/IP-DDS网关条目 */
typedef struct {
    ara_com_ServiceIdType serviceId;
    ara_com_ServiceInstanceIdType instanceId;
    uint16_t someipInstanceId;
    uint32_t ddsDomainId;
    bool isActive;
} SomeIP_DDS_GatewayEntryType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* 初始化与反初始化 */
Std_ReturnType ara_com_Init(void);
Std_ReturnType ara_com_Deinit(void);
bool ara_com_IsInitialized(void);

/* ServiceInterface管理 */
Std_ReturnType ara_com_CreateServiceInterface(
    const ara_com_ServiceInterfaceConfigType* config,
    ara_com_ServiceHandleType** handle);

Std_ReturnType ara_com_DestroyServiceInterface(ara_com_ServiceHandleType* handle);

Std_ReturnType ara_com_OfferService(ara_com_ServiceHandleType* handle);
Std_ReturnType ara_com_StopOfferService(ara_com_ServiceHandleType* handle);
Std_ReturnType ara_com_FindService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceOfferType offerType,
    ara_com_ServiceHandleType** handle);

Std_ReturnType ara_com_StartFindService(
    ara_com_ServiceIdType serviceId,
    void (*callback)(ara_com_ServiceHandleType*, ara_com_ServiceDiscoveryStateType));

Std_ReturnType ara_com_StopFindService(ara_com_ServiceIdType serviceId);

/* Event管理 */
Std_ReturnType ara_com_CreateEvent(
    ara_com_ServiceHandleType* service,
    const ara_com_EventConfigType* config,
    ara_com_EventHandleType** handle);

Std_ReturnType ara_com_DestroyEvent(ara_com_EventHandleType* handle);

Std_ReturnType ara_com_SendEvent(
    ara_com_EventHandleType* handle,
    const void* data,
    uint32_t length);

Std_ReturnType ara_com_SubscribeEvent(
    ara_com_EventHandleType* handle,
    uint32_t maxSampleCount);

Std_ReturnType ara_com_UnsubscribeEvent(ara_com_EventHandleType* handle);

Std_ReturnType ara_com_GetNewSamples(
    ara_com_EventHandleType* handle,
    void* buffer,
    uint32_t bufferSize,
    uint32_t* numSamples);

/* Method管理 */
Std_ReturnType ara_com_CreateMethod(
    ara_com_ServiceHandleType* service,
    const ara_com_MethodConfigType* config,
    ara_com_MethodHandleType** handle);

Std_ReturnType ara_com_DestroyMethod(ara_com_MethodHandleType* handle);

Std_ReturnType ara_com_CallMethod(
    ara_com_MethodHandleType* handle,
    const void* requestData,
    uint32_t requestLength,
    void* responseData,
    uint32_t* responseLength,
    uint32_t timeoutMs);

Std_ReturnType ara_com_CallMethodAsync(
    ara_com_MethodHandleType* handle,
    const void* requestData,
    uint32_t requestLength,
    void (*callback)(ara_com_CallResultType, void*, uint32_t));

Std_ReturnType ara_com_RegisterMethodHandler(
    ara_com_MethodHandleType* handle,
    Std_ReturnType (*handler)(
        const void* request,
        uint32_t reqLength,
        void* response,
        uint32_t* respLength));

/* Field管理 */
Std_ReturnType ara_com_CreateField(
    ara_com_ServiceHandleType* service,
    const ara_com_FieldConfigType* config,
    ara_com_FieldHandleType** handle);

Std_ReturnType ara_com_DestroyField(ara_com_FieldHandleType* handle);

Std_ReturnType ara_com_FieldGet(
    ara_com_FieldHandleType* handle,
    void* data,
    uint32_t* length,
    uint32_t timeoutMs);

Std_ReturnType ara_com_FieldSet(
    ara_com_FieldHandleType* handle,
    const void* data,
    uint32_t length,
    uint32_t timeoutMs);

Std_ReturnType ara_com_UpdateField(
    ara_com_FieldHandleType* handle,
    const void* data,
    uint32_t length);

/* E2E保护接口 */
Std_ReturnType ara_com_E2EProtect(
    ara_com_ServiceHandleType* service,
    void* data,
    uint32_t* length,
    uint8_t profile);

Std_ReturnType ara_com_E2ECheck(
    ara_com_ServiceHandleType* service,
    const void* data,
    uint32_t length,
    uint8_t profile,
    uint16_t* status);

/* Some/IP-DDS网关 */
#if SOMEIP_DDS_GATEWAY_ENABLED
Std_ReturnType ara_com_SomeIPDDSGateway_Init(void);
Std_ReturnType ara_com_SomeIPDDSGateway_Deinit(void);

Std_ReturnType ara_com_SomeIPDDSGateway_RegisterService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId,
    uint16_t someipInstanceId);

Std_ReturnType ara_com_SomeIPDDSGateway_UnregisterService(
    ara_com_ServiceIdType serviceId,
    ara_com_ServiceInstanceIdType instanceId);

void ara_com_SomeIPDDSGateway_Process(void);
#endif

/* 诊断与监控 */
Std_ReturnType ara_com_GetServiceStatus(
    ara_com_ServiceHandleType* handle,
    ara_com_ServiceDiscoveryStateType* state);

uint32_t ara_com_GetDroppedSamples(ara_com_EventHandleType* handle);
uint32_t ara_com_GetLatencyUs(ara_com_EventHandleType* handle);

#ifdef __cplusplus
}
#endif

#endif /* ARA_COM_DDS_H */