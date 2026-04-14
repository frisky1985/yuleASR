# API 参考文档

## 概述

本文档提供 YuleTech AutoSAR BSW Platform 的完整 API 接口参考。

## 目录

- [通用 API](#通用-api)
- [MCAL 层 API](#mcal-层-api)
- [ECUAL 层 API](#ecual-层-api)
- [Service 层 API](#service-层-api)
- [RTE 层 API](#rte-层-api)

---

## 通用 API

### 标准类型定义

```c
#include "Std_Types.h"

/* 标准返回类型 */
typedef uint8 Std_ReturnType;
#define E_OK      (0x00U)
#define E_NOT_OK  (0x01U)

/* 标准版本信息类型 */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;
```

### 通用模块接口

所有模块实现以下标准接口：

```c
/* 初始化 */
void <Module>_Init(const <Module>_ConfigType* ConfigPtr);

/* 反初始化 */
void <Module>_DeInit(void);

/* 主函数 */
void <Module>_MainFunction(void);

/* 版本信息 */
void <Module>_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

---

## MCAL 层 API

### Mcu (微控制器驱动)

```c
#include "Mcu.h"

/* 初始化 */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr);

/* 初始化 RAM */
void Mcu_InitRamSection(const Mcu_RamSectionType RamSection);

/* 分发 PLL 时钟 */
Std_ReturnType Mcu_DistributePllClock(void);

/* 获取 PLL 状态 */
Mcu_PllStatusType Mcu_GetPllStatus(void);

/* 设置模式 */
void Mcu_SetMode(const Mcu_ModeType McuMode);

/* 获取复位原因 */
Mcu_ResetType Mcu_GetResetReason(void);

/* 获取复位原始值 */
Mcu_RawResetType Mcu_GetResetRawValue(void);

/* 触发复位 */
void Mcu_PerformReset(void);

/* 设置时钟 */
Std_ReturnType Mcu_InitClock(const Mcu_ClockType ClockSetting);
```

### Can (CAN 驱动)

```c
#include "Can.h"

/* 初始化 */
void Can_Init(const Can_ConfigType* Config);

/* 获取版本信息 */
void Can_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* 设置控制器模式 */
Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);

/* 禁用中断 */
void Can_DisableControllerInterrupts(uint8 Controller);

/* 使能中断 */
void Can_EnableControllerInterrupts(uint8 Controller);

/* 写入数据 */
Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);

/* 主函数 - 写入 */
void Can_MainFunction_Write(void);

/* 主函数 - 读取 */
void Can_MainFunction_Read(void);

/* 主函数 - 总线关闭处理 */
void Can_MainFunction_BusOff(void);

/* 主函数 - 模式处理 */
void Can_MainFunction_Mode(void);

/* 主函数 - 唤醒处理 */
void Can_MainFunction_Wakeup(void);
```

---

## ECUAL 层 API

### CanIf (CAN 接口)

```c
#include "CanIf.h"

/* 初始化 */
void CanIf_Init(const CanIf_ConfigType* ConfigPtr);

/* 反初始化 */
void CanIf_DeInit(void);

/* 获取版本信息 */
void CanIf_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/* 传输数据 */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/* 读取接收数据 */
Std_ReturnType CanIf_ReadRxPduData(PduIdType CanIfRxSduId, PduInfoType* PduInfoPtr);

/* 设置 PDU 模式 */
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest);

/* 获取 PDU 模式 */
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType* PduModePtr);

/* 设置控制器模式 */
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/* 获取控制器模式 */
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType* ControllerModePtr);

/* 发送确认回调 */
void CanIf_TxConfirmation(PduIdType CanTxPduId);

/* 接收指示回调 */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr);

/* 控制器模式指示回调 */
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/* 总线关闭回调 */
void CanIf_ControllerBusOff(uint8 ControllerId);
```

---

## Service 层 API

### PduR (PDU 路由器)

```c
#include "PduR.h"

/* 初始化 */
void PduR_Init(const PduR_ConfigType* ConfigPtr);

/* 反初始化 */
void PduR_DeInit(void);

/* 获取版本信息 */
void PduR_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* 传输数据 */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/* 接收指示回调 */
void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* 发送确认回调 */
void PduR_TxConfirmation(PduIdType TxPduId);

/* 触发发送回调 */
Std_ReturnType PduR_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/* 主函数 */
void PduR_MainFunction(void);
```

### NvM (NVRAM 管理器)

```c
#include "NvM.h"

/* 初始化 */
void NvM_Init(const NvM_ConfigType* ConfigPtr);

/* 读取块 */
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr);

/* 写入块 */
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);

/* 恢复块默认值 */
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr);

/* 获取错误状态 */
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr);

/* 设置数据索引 */
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex);

/* 获取数据索引 */
Std_ReturnType NvM_GetDataIndex(NvM_BlockIdType BlockId, uint8* DataIndexPtr);

/* 设置块保护 */
Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled);

/* 使能 CRC 比较 */
void NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked);

/* 主函数 */
void NvM_MainFunction(void);

/* 任务完成回调 */
void NvM_JobEndNotification(void);

/* 任务错误回调 */
void NvM_JobErrorNotification(void);
```

### Dcm (诊断通信管理器)

```c
#include "Dcm.h"

/* 初始化 */
void Dcm_Init(const Dcm_ConfigType* ConfigPtr);

/* 反初始化 */
void Dcm_DeInit(void);

/* 获取版本信息 */
void Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* 接收指示回调 */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* 发送确认回调 */
void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* 触发发送回调 */
Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/* 主函数 */
void Dcm_MainFunction(void);

/* 获取安全级别 */
Std_ReturnType Dcm_GetSecurityLevel(uint8* SecLevel);

/* 获取会话控制类型 */
Std_ReturnType Dcm_GetSesCtrlType(uint8* SesCtrlType);

/* 重置为默认会话 */
Std_ReturnType Dcm_ResetToDefaultSession(void);
```

### Com (通信服务)

```c
#include "Com.h"

/* 初始化 */
void Com_Init(const Com_ConfigType* config);

/* 反初始化 */
void Com_DeInit(void);

/* 获取版本信息 */
void Com_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* 发送信号 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/* 接收信号 */
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);

/* 发送信号组 */
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);

/* 接收信号组 */
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);

/* 更新影子信号 */
uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/* 接收影子信号 */
uint8 Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr);

/* 触发 IPDU 发送 */
Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId);

/* 发送确认回调 */
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* 接收指示回调 */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* 触发发送回调 */
Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/* 主函数 - 接收 */
void Com_MainFunctionRx(void);

/* 主函数 - 发送 */
void Com_MainFunctionTx(void);

/* 主函数 - 路由信号 */
void Com_MainFunctionRouteSignals(void);
```

---

## RTE 层 API

### RTE 核心 API

```c
#include "Rte.h"

/* 初始化 */
Rte_StatusType Rte_Init(void);

/* 启动 */
Rte_StatusType Rte_Start(void);

/* 停止 */
Rte_StatusType Rte_Stop(void);

/* 获取版本信息 */
void Rte_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* 读取数据 */
Std_ReturnType Rte_Read(Rte_PortHandleType portHandle, void* data);

/* 写入数据 */
Std_ReturnType Rte_Write(Rte_PortHandleType portHandle, const void* data);

/* 发送数据 */
Std_ReturnType Rte_Send(Rte_PortHandleType portHandle, const void* data);

/* 接收数据 */
Std_ReturnType Rte_Receive(Rte_PortHandleType portHandle, void* data);

/* 读取 IRV */
Rte_StatusType Rte_IrvRead(Rte_IrvHandleType irvHandle, void* data);

/* 写入 IRV */
Rte_StatusType Rte_IrvWrite(Rte_IrvHandleType irvHandle, const void* data);

/* 调用操作 */
Std_ReturnType Rte_Call(Rte_PortHandleType portHandle, uint8 operationId, void* params);

/* 获取结果 */
Std_ReturnType Rte_Result(Rte_PortHandleType portHandle, uint8 operationId, void* result);

/* 切换模式 */
Rte_StatusType Rte_Switch(Rte_ModeHandleType modeGroup, uint8 mode);

/* 获取模式 */
Rte_StatusType Rte_Mode(Rte_ModeHandleType modeGroup, uint8* mode);

/* 触发事件 */
Std_ReturnType Rte_Trigger(Rte_PortHandleType triggerHandle);

/* 进入独占区 */
void Rte_EnterExclusiveArea(Rte_ExclusiveAreaHandleType exclusiveArea);

/* 退出独占区 */
void Rte_ExitExclusiveArea(Rte_ExclusiveAreaHandleType exclusiveArea);

/* 主函数 */
void Rte_MainFunction(void);
```

### RTE COM 接口

```c
/* 初始化 */
void Rte_ComInterface_Init(void);

/* 发送信号 */
Std_ReturnType Rte_ComSendSignal(uint16 comSignalId, const void* signalData);

/* 接收信号 */
Std_ReturnType Rte_ComReceiveSignal(uint16 comSignalId, void* signalData);

/* COM 接收回调 */
void Rte_ComCallbackRx(uint16 signalId, const void* data);

/* COM 发送回调 */
void Rte_ComCallbackTx(uint16 signalId);

/* 触发 IPDU 发送 */
Std_ReturnType Rte_ComTriggerIPDUSend(PduIdType pduId);

/* 切换 IPDU 传输模式 */
void Rte_ComSwitchIpduTxMode(PduIdType pduId, boolean mode);

/* 主函数 */
void Rte_ComInterface_MainFunction(void);
```

### RTE NVM 接口

```c
/* 初始化 */
void Rte_NvmInterface_Init(void);

/* 读取块 */
Std_ReturnType Rte_NvmReadBlock(NvM_BlockIdType blockId, void* dataPtr);

/* 写入块 */
Std_ReturnType Rte_NvmWriteBlock(NvM_BlockIdType blockId, const void* dataPtr);

/* 恢复块默认值 */
Std_ReturnType Rte_NvmRestoreBlockDefaults(NvM_BlockIdType blockId, void* dataPtr);

/* 获取块状态 */
Std_ReturnType Rte_NvmGetBlockStatus(NvM_BlockIdType blockId, NvM_RequestResultType* requestResult);

/* NVM 回调 */
void Rte_NvmCallback(uint16 blockId, Std_ReturnType jobResult);

/* 写入所有块 */
Std_ReturnType Rte_NvmWriteAll(void);

/* 读取所有块 */
Std_ReturnType Rte_NvmReadAll(void);

/* 主函数 */
void Rte_NvmInterface_MainFunction(void);
```

### RTE 调度器

```c
/* 初始化 */
void Rte_Scheduler_Init(void);

/* 启动 */
void Rte_Scheduler_Start(void);

/* 停止 */
void Rte_Scheduler_Stop(void);

/* 创建任务 */
Std_ReturnType Rte_SchedulerCreateTask(uint8 taskId, uint8 priority, uint32 periodMs,
                                       void (*entryPoint)(void));

/* 激活任务 */
Std_ReturnType Rte_SchedulerActivateTask(uint8 taskId);

/* 终止任务 */
void Rte_SchedulerTerminateTask(void);

/* 等待事件 */
Rte_StatusType Rte_WaitForEvent(Rte_InstanceHandleType instance, Rte_EventType eventMask, uint32 timeout);

/* 设置事件 */
Rte_StatusType Rte_SetEvent(Rte_InstanceHandleType instance, Rte_EventType event);

/* 清除事件 */
Rte_StatusType Rte_ClearEvent(Rte_InstanceHandleType instance, Rte_EventType event);

/* 调度器 Tick */
void Rte_SchedulerTick(void);

/* 主函数 */
void Rte_Scheduler_MainFunction(void);

/* 获取运行状态 */
boolean Rte_SchedulerIsRunning(void);

/* 获取当前任务 */
uint8 Rte_SchedulerGetCurrentTask(void);

/* 获取 Tick 计数 */
uint32 Rte_SchedulerGetTickCount(void);
```

---

## 错误代码

### 通用错误代码

| 错误代码 | 值 | 描述 |
|:---------|:---|:-----|
| E_OK | 0x00 | 成功 |
| E_NOT_OK | 0x01 | 失败 |

### RTE 错误代码

| 错误代码 | 值 | 描述 |
|:---------|:---|:-----|
| RTE_E_OK | 0x00 | 成功 |
| RTE_E_INVALID | 0x01 | 无效参数 |
| RTE_E_UNCONNECTED | 0x02 | 未连接 |
| RTE_E_TIMEOUT | 0x08 | 超时 |
| RTE_E_LIMIT | 0x04 | 限制超出 |
| RTE_E_NO_DATA | 0x05 | 无数据 |

### NvM 错误代码

| 错误代码 | 值 | 描述 |
|:---------|:---|:-----|
| NVM_REQ_OK | 0x00 | 请求成功 |
| NVM_REQ_NOT_OK | 0x01 | 请求失败 |
| NVM_REQ_PENDING | 0x02 | 请求挂起 |
| NVM_REQ_INTEGRITY_FAILED | 0x03 | 完整性检查失败 |

---

## 数据类型

### 标准数据类型

```c
/* 布尔类型 */
typedef unsigned char boolean;
#define FALSE  (0U)
#define TRUE   (1U)

/* 整数类型 */
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef signed char sint8;
typedef signed short sint16;
typedef signed long sint32;

/* 浮点类型 */
typedef float float32;
typedef double float64;
```

### RTE 数据类型

```c
/* 引擎速度 (0-15000 RPM) */
typedef uint16 Rte_EngineSpeedType;

/* 引擎温度 (-40 to +215 degC) */
typedef sint8 Rte_EngineTempType;

/* 车速 (0-300 km/h) */
typedef uint16 Rte_VehicleSpeedType;

/* 电池电压 (0-65V) */
typedef uint16 Rte_BatteryVoltageType;

/* 档位位置 */
typedef uint8 Rte_GearPositionType;

/* DTC 类型 */
typedef uint32 Rte_DTCType;
```

---

## 版本历史

| 版本 | 日期 | 变更描述 |
|:-----|:-----|:---------|
| 1.0.0 | 2026-04-15 | 初始版本，完整 API 文档 |

---

**文档版本**: 1.0.0
**最后更新**: 2026-04-15
**作者**: YuleTech AutoSAR Team
