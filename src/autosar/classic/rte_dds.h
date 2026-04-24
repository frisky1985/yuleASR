/******************************************************************************
 * @file    rte_dds.h
 * @brief   AUTOSAR Classic RTE DDS集成头文件
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * RTE (Runtime Environment) to DDS Mapping Layer
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef RTE_DDS_H
#define RTE_DDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"
#include "autosar_errors.h"
#include "dds_types.h"

/* RTE版本 */
#define RTE_DDS_MAJOR_VERSION           22
#define RTE_DDS_MINOR_VERSION           11
#define RTE_DDS_PATCH_VERSION           0

/* RTE配置 */
#define RTE_MAX_SW_COMPONENTS           64
#define RTE_MAX_PORTS_PER_COMPONENT     32
#define RTE_MAX_CONNECTIONS             256
#define RTE_MAX_DATA_ELEMENTS           512
#define RTE_MAX_BUFFER_SIZE             4096

/* 端口类型 */
typedef enum {
    RTE_PORT_SENDER_RECEIVER = 0,
    RTE_PORT_CLIENT_SERVER,
    RTE_PORT_MODE_SWITCH,
    RTE_PORT_NONVOLATILE,
    RTE_PORT_TRIGGER,
    RTE_PORT_PARAMETER
} rte_PortTypeType;

/* 端口方向 */
typedef enum {
    RTE_PORT_PROVIDED = 0,
    RTE_PORT_REQUIRED
} rte_PortDirectionType;

/* 数据发送类型 */
typedef enum {
    RTE_SEND_TYPE_EXPLICIT = 0,
    RTE_SEND_TYPE_PERIODIC,
    RTE_SEND_TYPE_ON_WRITE,
    RTE_SEND_TYPE_ON_CHANGE
} rte_SendTypeType;

/* 服务调用类型 */
typedef enum {
    RTE_CALL_SYNC = 0,
    RTE_CALL_ASYNC,
    RTE_CALL_QUEUED
} rte_CallTypeType;

/* 组件状态 */
typedef enum {
    RTE_CMP_STOPPED = 0,
    RTE_CMP_STARTED,
    RTE_CMP_SUSPENDED,
    RTE_CMP_TERMINATED
} rte_ComponentStateType;

/* 调用结果 */
typedef enum {
    RTE_E_OK = 0,
    RTE_E_NOK,
    RTE_E_INVALID,
    RTE_E_COMMS_ERROR,
    RTE_E_TIMEOUT,
    RTE_E_LIMIT,
    RTE_E_NO_DATA,
    RTE_E_UNCONNECTED,
    RTE_E_E2E_ERROR
} rte_ResultType;

/* 数据过滤器类型 */
typedef enum {
    RTE_FILTER_ALWAYS = 0,
    RTE_FILTER_NEVER,
    RTE_FILTER_MASKED_NEW_DIFFERS_MASKED_OLD,
    RTE_FILTER_MASKED_NEW_EQUALS_X,
    RTE_FILTER_NEW_IS_WITHIN,
    RTE_FILTER_NEW_IS_OUTSIDE,
    RTE_FILTER_ONE_EVERY_N
} rte_DataFilterType;

/* 信号质量类型 */
typedef enum {
    RTE_SIGNAL_GOOD = 0,
    RTE_SIGNAL_OUT_OF_RANGE,
    RTE_SIGNAL_INVALID,
    RTE_SIGNAL_NOT_AVAILABLE
} rte_SignalQualityType;

/******************************************************************************
 * Data Types
 ******************************************************************************/

/* SW Component Handle */
typedef uint16_t rte_ComponentHandleType;
#define RTE_INVALID_COMPONENT           ((rte_ComponentHandleType)0xFFFF)

/* Port Handle */
typedef uint16_t rte_PortHandleType;
#define RTE_INVALID_PORT                ((rte_PortHandleType)0xFFFF)

/* Data Element Handle */
typedef uint16_t rte_DataElementHandleType;
#define RTE_INVALID_DATA_ELEMENT        ((rte_DataElementHandleType)0xFFFF)

/* Connection Handle */
typedef uint16_t rte_ConnectionHandleType;
#define RTE_INVALID_CONNECTION          ((rte_ConnectionHandleType)0xFFFF)

/* Runnable Handle */
typedef uint16_t rte_RunnableHandleType;
#define RTE_INVALID_RUNNABLE            ((rte_RunnableHandleType)0xFFFF)

/* Buffer Handle */
typedef uint16_t rte_BufferHandleType;
#define RTE_INVALID_BUFFER              ((rte_BufferHandleType)0xFFFF)

/* COM Signal Handle */
typedef uint16_t rte_ComSignalHandleType;
#define RTE_INVALID_COM_SIGNAL          ((rte_ComSignalHandleType)0xFFFF)

/* PDU Handle */
typedef uint16_t rte_PduHandleType;
#define RTE_INVALID_PDU                 ((rte_PduHandleType)0xFFFF)

/* Mode Declaration Group */
typedef uint8_t rte_ModeType;

/* Timestamp type */
typedef uint32_t rte_TimestampType;

/* 数据元素配置 */
typedef struct {
    rte_DataElementHandleType handle;
    uint16_t dataTypeId;
    uint32_t dataSize;
    uint32_t bufferSize;
    rte_SendTypeType sendType;
    uint32_t sendPeriodMs;
    rte_DataFilterType filterType;
    uint32_t filterParam;
    bool e2eEnabled;
    uint8_t e2eProfile;
    bool isQueued;
    uint32_t queueLength;
} rte_DataElementConfigType;

/* Sender/Receiver Port配置 */
typedef struct {
    rte_PortHandleType portHandle;
    char portName[64];
    rte_PortDirectionType direction;
    uint32_t numDataElements;
    rte_DataElementConfigType* dataElements;
    dds_TopicHandleType ddsTopic;
    dds_PublisherHandleType ddsPublisher;
    dds_SubscriberHandleType ddsSubscriber;
} rte_SRPortConfigType;

/* Client/Server Operation配置 */
typedef struct {
    uint16_t operationId;
    uint32_t maxRequestSize;
    uint32_t maxResponseSize;
    uint32_t timeoutMs;
    rte_CallTypeType callType;
    bool e2eEnabled;
    uint8_t e2eProfile;
} rte_OperationConfigType;

/* Client/Server Port配置 */
typedef struct {
    rte_PortHandleType portHandle;
    char portName[64];
    rte_PortDirectionType direction;
    uint32_t numOperations;
    rte_OperationConfigType* operations;
    dds_TopicHandleType ddsRequestTopic;
    dds_TopicHandleType ddsResponseTopic;
} rte_CSPortConfigType;

/* Runnable配置 */
typedef struct {
    rte_RunnableHandleType runnableHandle;
    char runnableName[64];
    uint32_t priority;
    uint32_t periodMs;
    bool supportsMultipleActivation;
    uint32_t minStartIntervalMs;
    uint32_t numReadPorts;
    rte_PortHandleType* readPorts;
    uint32_t numWritePorts;
    rte_PortHandleType* writePorts;
    uint32_t numModeAccess;
    rte_ModeType* accessedModes;
    uint32_t numModeSwitch;
    rte_ModeType* switchedModes;
} rte_RunnableConfigType;

/* SW Component配置 */
typedef struct {
    rte_ComponentHandleType componentHandle;
    char componentName[64];
    uint32_t numSRPorts;
    rte_SRPortConfigType* srPorts;
    uint32_t numCSPorts;
    rte_CSPortConfigType* csPorts;
    uint32_t numRunnables;
    rte_RunnableConfigType* runnables;
    rte_ComponentStateType state;
} rte_ComponentConfigType;

/* COM信号配置 */
typedef struct {
    rte_ComSignalHandleType signalHandle;
    char signalName[64];
    uint32_t bitPosition;
    uint32_t bitSize;
    rte_DataElementHandleType mappedDataElement;
    bool e2eEnabled;
    uint8_t e2eProfile;
} rte_ComSignalConfigType;

/* PDU配置 */
typedef struct {
    rte_PduHandleType pduHandle;
    char pduName[64];
    uint32_t pduLength;
    uint32_t numSignals;
    rte_ComSignalConfigType* signals;
    dds_TopicHandleType ddsTopic;
} rte_PduConfigType;

/* 数据发送信息 */
typedef struct {
    rte_SignalQualityType quality;
    rte_TimestampType timestamp;
    uint32_t sequenceNumber;
    uint16_t e2eStatus;
} rte_DataInfoType;

/* 调用请求信息 */
typedef struct {
    uint32_t requestId;
    rte_TimestampType requestTime;
    uint32_t timeoutMs;
    rte_CallTypeType callType;
    void* callback;
} rte_CallInfoType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

Std_ReturnType rte_Init(void);
Std_ReturnType rte_Deinit(void);
bool rte_IsInitialized(void);

/******************************************************************************
 * Function Prototypes - Component Management
 ******************************************************************************/

Std_ReturnType rte_CreateComponent(
    const rte_ComponentConfigType* config,
    rte_ComponentHandleType* handle);

Std_ReturnType rte_DestroyComponent(rte_ComponentHandleType handle);
Std_ReturnType rte_StartComponent(rte_ComponentHandleType handle);
Std_ReturnType rte_StopComponent(rte_ComponentHandleType handle);

Std_ReturnType rte_GetComponentState(
    rte_ComponentHandleType handle,
    rte_ComponentStateType* state);

/******************************************************************************
 * Function Prototypes - Sender/Receiver Communication
 ******************************************************************************/

/* 显式发送/接收 */
rte_ResultType rte_Write(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    const void* data);

rte_ResultType rte_Read(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    void* data);

rte_ResultType rte_ReadWithInfo(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    void* data,
    rte_DataInfoType* info);

/* 发送/接收组 */
rte_ResultType rte_Send(
    rte_PortHandleType port,
    const void* data,
    uint32_t length);

rte_ResultType rte_Receive(
    rte_PortHandleType port,
    void* data,
    uint32_t* length);

/* 缓冲区访问 */
rte_ResultType rte_WriteBuffer(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    const void* data,
    uint32_t length);

rte_ResultType rte_ReadBuffer(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    void* buffer,
    uint32_t bufferSize,
    uint32_t* readLength);

/* 定期发送 */
rte_ResultType rte_TriggerSend(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement);

/******************************************************************************
 * Function Prototypes - Client/Server Communication
 ******************************************************************************/

/* 同步调用 */
rte_ResultType rte_Call(
    rte_PortHandleType port,
    uint16_t operationId,
    const void* requestData,
    void* responseData,
    uint32_t* responseLength);

/* 异步调用 */
rte_ResultType rte_CallAsync(
    rte_PortHandleType port,
    uint16_t operationId,
    const void* requestData,
    uint32_t requestLength,
    void (*callback)(rte_ResultType result, void* response, uint32_t length));

/* 结果获取 */
rte_ResultType rte_Result(
    rte_PortHandleType port,
    uint16_t operationId,
    void* responseData,
    uint32_t* responseLength);

/* 请求取消 */
rte_ResultType rte_Cancel(
    rte_PortHandleType port,
    uint16_t operationId);

/* 服务提供方处理 */
rte_ResultType rte_ServerRegister(
    rte_PortHandleType port,
    uint16_t operationId,
    rte_ResultType (*handler)(const void* request, uint32_t reqLength,
                               void* response, uint32_t* respLength));

/******************************************************************************
 * Function Prototypes - Mode Management
 ******************************************************************************/

rte_ResultType rte_SwitchMode(
    rte_PortHandleType port,
    rte_ModeType newMode);

rte_ResultType rte_ReadMode(
    rte_PortHandleType port,
    rte_ModeType* currentMode);

rte_ResultType rte_ModeNotification(
    rte_PortHandleType port,
    void (*callback)(rte_ModeType oldMode, rte_ModeType newMode));

/******************************************************************************
 * Function Prototypes - Runnable Management
 ******************************************************************************/

rte_ResultType rte_RunnableTrigger(
    rte_ComponentHandleType component,
    rte_RunnableHandleType runnable);

rte_ResultType rte_RunnableEnable(
    rte_ComponentHandleType component,
    rte_RunnableHandleType runnable);

rte_ResultType rte_RunnableDisable(
    rte_ComponentHandleType component,
    rte_RunnableHandleType runnable);

rte_ResultType rte_RunnableSetPriority(
    rte_ComponentHandleType component,
    rte_RunnableHandleType runnable,
    uint32_t priority);

/******************************************************************************
 * Function Prototypes - COM Stack Integration
 ******************************************************************************/

/* COM信号映射 */
Std_ReturnType rte_ComMapSignal(
    rte_ComSignalHandleType comSignal,
    rte_DataElementHandleType dataElement);

Std_ReturnType rte_ComUnmapSignal(rte_ComSignalHandleType comSignal);

/* PDU映射 */
Std_ReturnType rte_ComMapPdu(
    rte_PduHandleType pdu,
    rte_PortHandleType port);

Std_ReturnType rte_ComUnmapPdu(rte_PduHandleType pdu);

/* 数据发送/接收 */
Std_ReturnType rte_ComSendSignal(
    rte_ComSignalHandleType signal,
    const void* data);

Std_ReturnType rte_ComReceiveSignal(
    rte_ComSignalHandleType signal,
    void* data);

Std_ReturnType rte_ComSendPdu(
    rte_PduHandleType pdu,
    const void* data,
    uint32_t length);

Std_ReturnType rte_ComReceivePdu(
    rte_PduHandleType pdu,
    void* data,
    uint32_t* length);

/* 信号质量 */
Std_ReturnType rte_ComGetSignalQuality(
    rte_ComSignalHandleType signal,
    rte_SignalQualityType* quality);

/* 初始化/反初始化 */
Std_ReturnType rte_ComInit(void);
Std_ReturnType rte_ComDeinit(void);

/******************************************************************************
 * Function Prototypes - E2E Protection
 ******************************************************************************/

Std_ReturnType rte_E2EProtect(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    void* data,
    uint32_t* length);

Std_ReturnType rte_E2ECheck(
    rte_PortHandleType port,
    rte_DataElementHandleType dataElement,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Utility
 ******************************************************************************/

const char* rte_GetVersion(void);
void rte_Process(void);
uint32_t rte_GetDroppedMessages(rte_PortHandleType port);
uint32_t rte_GetLatencyUs(rte_PortHandleType port);

#ifdef __cplusplus
}
#endif

#endif /* RTE_DDS_H */