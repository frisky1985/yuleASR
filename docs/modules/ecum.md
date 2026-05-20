# ECU State Manager (EcuM)

## 1. 模块概述

### 1.1 功能描述

EcuM (ECU State Manager) 是AutoSAR基础软件服务层的核心模块，负责管理ECU的状态和状态转换。它控制ECU的启动、运行、睡眠和关机等生命周期阶段，协调基础软件(BSW)和应用软件(ASW)的初始化和反初始化。

### 1.2 主要特性

- **多阶段启动**: 支持Startup One、Startup Two、Startup Three三个启动阶段
- **状态管理**: 管理ECU的主状态和子状态
- **睡眠管理**: 支持Sleep、Halt、Poll三种低功耗模式
- **唤醒管理**: 支持多种唤醒源的配置、验证和处理
- **关机管理**: 支持OFF、RESET、SLEEP三种关机目标
- **RUN请求管理**: 多用户RUN请求跟踪和管理
- **与BswM集成**: 状态变化时通知BswM进行模式仲裁

### 1.3 AutoSAR版本

- **AUTOSAR Classic Platform**: R4.0.3
- **SWS版本**: ECU State Manager SWS 4.0.3

### 1.4 模块标识

| 属性 | 值 |
|------|-----|
| 模块ID | 0x0A |
| 实例ID | 0x00 |
| 软件主版本 | 2 |
| 软件次版本 | 0 |
| 软件补丁版本 | 0 |

---

## 2. 架构设计

### 2.1 状态机架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           ECUM STATE MACHINE                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   OFF                                                                    │
│    │                                                                     │
│    │ EcuM_Init()                                                         │
│    ▼                                                                     │
│   STARTUP ──────────────────────────────────────────────────────┐       │
│    │                                                             │       │
│    ├─[SUBSTATE_STARTUP_ONE]   → DriverInitOne()                 │       │
│    │                              Mcu/Port/Dio/Gpt Init          │       │
│    │                              StartOS()                      │       │
│    ▼                                                             │       │
│    ├─[SUBSTATE_STARTUP_TWO]   → SchM_Init()                     │       │
│    │                              BswM_Init()                    │       │
│    │                              ComM_Init()                    │       │
│    │                              NvM_ReadAll()                  │       │
│    ▼                                                             │       │
│   RUN ◄─────────────────────────────────────────────────────────┘       │
│    │
│    ├─[SUBSTATE_RUN]           → Normal Operation
│    │
│    │ All RUN Requests Released
│    ▼
│   POST_RUN
│    │
│    ├─[SUBSTATE_POST_RUN]      → Cleanup Activities
│    │
│    │ Shutdown Target = SLEEP
│    ├─────────────────────────► SLEEP
│    │                             ├─[SUBSTATE_GO_SLEEP]
│    │                             ├─[SUBSTATE_SLEEP]
│    │                             ├─[SUBSTATE_WAKEUP_ONE]
│    │                             └─[SUBSTATE_WAKEUP_TWO]
│    │ Shutdown Target = OFF/RESET
│    └─────────────────────────► SHUTDOWN
│                                  ├─[SUBSTATE_GO_OFF_ONE]
│                                  └─[SUBSTATE_GO_OFF_TWO]
│                                        │
│                                        ▼
│                                       OFF
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 主状态定义

| 状态 | 值 | 描述 |
|------|-----|------|
| ECUM_STATE_OFF | 0x00 | ECU关闭状态 |
| ECUM_STATE_STARTUP | 0x01 | 启动状态 |
| ECUM_STATE_RUN | 0x10 | 正常运行状态 |
| ECUM_STATE_POST_RUN | 0x20 | 后运行状态(清理) |
| ECUM_STATE_SLEEP | 0x30 | 睡眠状态 |
| ECUM_STATE_WAKE_SLEEP | 0x31 | 睡眠唤醒状态 |
| ECUM_STATE_SHUTDOWN | 0x40 | 关机状态 |

### 2.3 子状态定义

| 子状态 | 值 | 父状态 | 描述 |
|--------|-----|--------|------|
| ECUM_SUBSTATE_STARTUP_ONE | 0x11 | STARTUP | 第一阶段启动(MCU/OS前) |
| ECUM_SUBSTATE_STARTUP_TWO | 0x12 | STARTUP | 第二阶段启动(BSW初始化) |
| ECUM_SUBSTATE_STARTUP_THREE | 0x13 | STARTUP | 第三阶段启动(SWC初始化) |
| ECUM_SUBSTATE_RUN | 0x21 | RUN | 正常运行 |
| ECUM_SUBSTATE_POST_RUN | 0x22 | POST_RUN | 后运行 |
| ECUM_SUBSTATE_GO_SLEEP | 0x31 | SLEEP | 准备进入睡眠 |
| ECUM_SUBSTATE_SLEEP | 0x32 | SLEEP | 睡眠中 |
| ECUM_SUBSTATE_WAKEUP_ONE | 0x33 | WAKE_SLEEP | 唤醒阶段一 |
| ECUM_SUBSTATE_WAKEUP_TWO | 0x34 | WAKE_SLEEP | 唤醒阶段二 |
| ECUM_SUBSTATE_GO_OFF_ONE | 0x41 | SHUTDOWN | 关机阶段一 |
| ECUM_SUBSTATE_GO_OFF_TWO | 0x42 | SHUTDOWN | 关机阶段二 |
| ECUM_SUBSTATE_RESET | 0x43 | SHUTDOWN | 复位状态 |
| ECUM_SUBSTATE_HALT | 0x50 | - | Halt模式 |
| ECUM_SUBSTATE_POLL | 0x51 | - | Poll模式 |

---

## 3. API接口

### 3.1 初始化函数

#### EcuM_Init
```c
void EcuM_Init(void)
```
**描述**: 初始化EcuM模块，启动多阶段启动序列

**参数**: 无

**返回值**: 无

**调用时机**: ECU启动时的第一个BSW调用

**示例**:
```c
void main(void)
{
    EcuM_Init();
    /* 不会返回，EcuM_StartupTwo会启动OS */
}
```

#### EcuM_StartupOne
```c
void EcuM_StartupOne(void)
```
**描述**: 启动阶段一，初始化无需OS的驱动

**初始化内容**:
- 调用EcuM_DriverInitOne()
- 初始化Mcu、Port、Dio、Gpt
- 启动OS
- 自动过渡到StartupTwo

#### EcuM_StartupTwo
```c
void EcuM_StartupTwo(void)
```
**描述**: 启动阶段二，初始化需要OS的BSW模块

**初始化内容**:
- 初始化SchM
- 初始化BswM
- 初始化ComM
- 初始化NvM并读取数据
- 初始化COM协议栈
- 初始化RTE
- 调用EcuM_DriverInitThree()
- 过渡到RUN状态

---

### 3.2 状态查询函数

#### EcuM_GetState
```c
Std_ReturnType EcuM_GetState(EcuM_StateType* state)
```
**描述**: 获取当前ECU主状态

**参数**:
| 参数 | 类型 | 方向 | 描述 |
|------|------|------|------|
| state | EcuM_StateType* | OUT | 当前状态指针 |

**返回值**:
- E_OK: 成功
- E_NOT_OK: 失败(模块未初始化或NULL指针)

#### EcuM_GetSubState
```c
Std_ReturnType EcuM_GetSubState(EcuM_SubStateType* subState)
```
**描述**: 获取当前ECU子状态

**参数**:
| 参数 | 类型 | 方向 | 描述 |
|------|------|------|------|
| subState | EcuM_SubStateType* | OUT | 当前子状态指针 |

**返回值**:
- E_OK: 成功
- E_NOT_OK: 失败

---

### 3.3 RUN请求管理

#### EcuM_RequestRUN
```c
Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
```
**描述**: 用户请求RUN模式，防止ECU进入低功耗状态

**参数**:
| 参数 | 类型 | 描述 |
|------|------|------|
| user | EcuM_UserType | 用户ID (0 ~ ECUM_MAX_USERS-1) |

**返回值**:
- E_OK: 请求成功
- E_NOT_OK: 请求失败(无效用户ID或未初始化)

**示例**:
```c
#define USER_APP 0
#define USER_COM 1

void App_Init(void)
{
    /* 应用初始化时请求RUN */
    EcuM_RequestRUN(USER_APP);
}
```

#### EcuM_ReleaseRUN
```c
Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)
```
**描述**: 用户释放RUN模式请求

**参数**:
| 参数 | 类型 | 描述 |
|------|------|------|
| user | EcuM_UserType | 用户ID |

**返回值**:
- E_OK: 释放成功
- E_NOT_OK: 释放失败

**注意**: 当所有RUN请求都释放后，ECU将过渡到POST_RUN状态

#### EcuM_KillAllRUNRequests
```c
Std_ReturnType EcuM_KillAllRUNRequests(void)
```
**描述**: 强制释放所有RUN请求

**使用场景**: 紧急情况或诊断会话结束

**返回值**:
- E_OK: 操作成功
- E_NOT_OK: 操作失败

---

### 3.4 关机管理

#### EcuM_SelectShutdownTarget
```c
Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)
```
**描述**: 选择关机目标

**参数**:
| 参数 | 类型 | 描述 |
|------|------|------|
| target | EcuM_ShutdownTargetType | 关机目标(OFF/RESET/SLEEP) |
| mode | uint8 | 模式(睡眠模式或复位模式) |

**关机目标类型**:
| 目标 | 值 | 描述 |
|------|-----|------|
| ECUM_SHUTDOWN_TARGET_OFF | 0x00 | 关机断电 |
| ECUM_SHUTDOWN_TARGET_RESET | 0x01 | 复位重启 |
| ECUM_SHUTDOWN_TARGET_SLEEP | 0x02 | 进入睡眠 |

**返回值**:
- E_OK: 选择成功
- E_NOT_OK: 选择失败(无效目标)

#### EcuM_GetShutdownTarget
```c
Std_ReturnType EcuM_GetShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode)
```
**描述**: 获取当前关机目标

#### EcuM_GetLastShutdownTarget
```c
Std_ReturnType EcuM_GetLastShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode)
```
**描述**: 获取上次关机目标(从NvM读取)

#### EcuM_SelectShutdownCause
```c
Std_ReturnType EcuM_SelectShutdownCause(EcuM_ShutdownCauseType cause)
```
**描述**: 选择关机原因

**关机原因类型**:
| 原因 | 值 | 描述 |
|------|-----|------|
| ECUM_CAUSE_ECU_STATE | 0x00 | ECU状态变化 |
| ECUM_CAUSE_WATCHDOG | 0x01 | 看门狗复位 |
| ECUM_CAUSE_HARDWARE | 0x02 | 硬件原因 |
| ECUM_CAUSE_SOFTWARE | 0x03 | 软件原因 |
| ECUM_CAUSE_FATAL_ERROR | 0x04 | 致命错误 |
| ECUM_CAUSE_DCM | 0x05 | 诊断请求 |

#### EcuM_Shutdown
```c
void EcuM_Shutdown(void)
```
**描述**: 执行关机序列

**流程**:
1. 通知BswM进入关机
2. 停止RTE
3. 反初始化ComM
4. 写入NvM数据
5. 执行GoOffOne
6. 执行GoOffTwo
7. 断电或复位(不返回)

---

### 3.5 睡眠管理

#### EcuM_GoSleep
```c
void EcuM_GoSleep(void)
```
**描述**: 进入睡眠模式

**流程**:
1. 通知BswM准备睡眠
2. 停止RTE
3. 释放ComM通道
4. 停止看门狗
5. 使能唤醒源
6. 进入睡眠

#### EcuM_GoHalt
```c
void EcuM_GoHalt(void)
```
**描述**: 进入Halt模式(CPU时钟停止)

**条件**: ECUM_HALT_MODE_SUPPORTED == STD_ON

#### EcuM_GoPoll
```c
void EcuM_GoPoll(void)
```
**描述**: 进入Poll模式(主动轮询)

**条件**: ECUM_POLL_MODE_SUPPORTED == STD_ON

#### EcuM_WakeupRestart
```c
void EcuM_WakeupRestart(void)
```
**描述**: 从睡眠中唤醒并重新启动

**流程**:
1. 执行WakeupOne(重新初始化驱动)
2. 验证唤醒源
3. 执行WakeupTwo(重新初始化OS和BSW)
4. 回到RUN状态

---

### 3.6 唤醒源管理

#### EcuM_SetWakeupEvent
```c
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
```
**描述**: 设置唤醒事件

**唤醒源类型**:
| 唤醒源 | 值 | 描述 |
|--------|-----|------|
| ECUM_WKSOURCE_POWER | 0x00000001 | 电源唤醒 |
| ECUM_WKSOURCE_RESET | 0x00000002 | 复位唤醒 |
| ECUM_WKSOURCE_TIMER | 0x00000020 | 定时器唤醒 |
| ECUM_WKSOURCE_CAN | 0x00000040 | CAN唤醒 |
| ECUM_WKSOURCE_LIN | 0x00000800 | LIN唤醒 |
| ECUM_WKSOURCE_ETH | 0x00008000 | 以太网唤醒 |
| ECUM_WKSOURCE_GPIO | 0x00200000 | GPIO唤醒 |

#### EcuM_ClearWakeupEvent
```c
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources)
```
**描述**: 清除唤醒事件

#### EcuM_EnableWakeupSources
```c
Std_ReturnType EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources)
```
**描述**: 使能唤醒源

#### EcuM_DisableWakeupSources
```c
Std_ReturnType EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources)
```
**描述**: 禁用唤醒源

#### EcuM_GetStatusOfWakeupSource
```c
EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources)
```
**描述**: 获取唤醒源状态

**唤醒状态**:
| 状态 | 值 | 描述 |
|------|-----|------|
| ECUM_WKSTATUS_NONE | 0x00 | 无唤醒 |
| ECUM_WKSTATUS_PENDING | 0x01 | 唤醒待验证 |
| ECUM_WKSTATUS_VALIDATED | 0x02 | 唤醒已验证 |
| ECUM_WKSTATUS_EXPIRED | 0x03 | 唤醒已过期 |
| ECUM_WKSTATUS_DISABLED | 0x04 | 唤醒已禁用 |

#### EcuM_CheckValidation
```c
Std_ReturnType EcuM_CheckValidation(EcuM_WakeupSourceType source)
```
**描述**: 检查唤醒源是否已验证

---

### 3.7 其他函数

#### EcuM_MainFunction
```c
void EcuM_MainFunction(void)
```
**描述**: EcuM主函数，周期性处理状态机

**调用周期**: 由EcuM_MainFunctionPeriod配置决定(默认10ms)

#### EcuM_GetVersionInfo
```c
void EcuM_GetVersionInfo(Std_VersionInfoType* versionInfo)
```
**描述**: 获取模块版本信息

---

## 4. 配置参数

### 4.1 通用配置 (EcuM_Cfg.h)

| 参数 | 默认值 | 描述 |
|------|--------|------|
| ECUM_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| ECUM_VERSION_INFO_API | STD_ON | 版本信息API使能 |
| ECUM_MAIN_FUNCTION_PERIOD | 10u | 主函数周期(ms) |
| ECUM_MAX_USERS | 32u | 最大用户数量 |
| ECUM_MAX_WAKEUP_SOURCES | 32u | 最大唤醒源数量 |

### 4.2 功能支持配置

| 参数 | 默认值 | 描述 |
|------|--------|------|
| ECUM_HALT_MODE_SUPPORTED | STD_ON | Halt模式支持 |
| ECUM_POLL_MODE_SUPPORTED | STD_ON | Poll模式支持 |
| ECUM_MULTI_CORE_SUPPORT | STD_OFF | 多核支持 |
| ECUM_FAST_STARTUP_MODE | STD_OFF | 快速启动模式 |

### 4.3 模块集成配置

| 参数 | 默认值 | 描述 |
|------|--------|------|
| ECUM_NVM_ENABLED | STD_ON | NvM集成使能 |
| ECUM_WDGM_ENABLED | STD_ON | WdgM集成使能 |
| ECUM_COMM_ENABLED | STD_ON | ComM集成使能 |
| ECUM_BSWM_ENABLED | STD_ON | BswM集成使能 |
| ECUM_RTE_ENABLED | STD_ON | RTE集成使能 |

### 4.4 超时配置

| 参数 | 默认值 | 描述 |
|------|--------|------|
| ECUM_WAKEUP_VALIDATION_TIMEOUT | 100u | 唤醒验证超时(ms) |
| ECUM_NVM_READALL_TIMEOUT | 10000u | NvM读取超时(ms) |
| ECUM_NVM_WRITEALL_TIMEOUT | 10000u | NvM写入超时(ms) |
| ECUM_STARTUP_TIMEOUT | 5000u | 启动超时(ms) |
| ECUM_SLEEP_TRANSITION_TIMEOUT | 1000u | 睡眠过渡超时(ms) |

---

## 5. 集成指南

### 5.1 模块依赖

```
EcuM
├── Det (可选) - 错误报告
├── BswM (必需) - 模式仲裁
├── SchM (必需) - 调度管理
├── NvM (可选) - 非易失性存储
├── WdgM (可选) - 看门狗管理
├── ComM (可选) - 通信管理
└── RTE (可选) - 运行时环境
```

### 5.2 集成步骤

1. **配置EcuM_Cfg.h**
   - 使能需要的功能
   - 配置超时参数
   - 定义唤醒源

2. **实现Callout函数**
   ```c
   void EcuM_DriverInitOne(const EcuM_ConfigType* config)
   {
       /* 初始化Mcu、Port、Dio、Gpt */
       Mcu_Init(&Mcu_Config);
       Port_Init(&Port_Config);
       Dio_Init(&Dio_Config);
       Gpt_Init(&Gpt_Config);
   }
   
   void EcuM_DriverInitTwo(const EcuM_ConfigType* config)
   {
       /* 初始化CAN、SPI等 */
       Can_Init(&Can_Config);
       Spi_Init(&Spi_Config);
   }
   
   void EcuM_AL_SwitchOff(void)
   {
       /* 执行断电操作 */
       Mcu_SetMode(MCU_MODE_POWER_OFF);
   }
   ```

3. **启动代码修改**
   ```c
   void main(void)
   {
       /* EcuM_Init会完成所有启动工作 */
       EcuM_Init();
       
       /* 不会执行到这里 */
       while(1);
   }
   ```

4. **集成SchM_MainFunction**
   ```c
   void SchM_MainFunction(void)
   {
       EcuM_MainFunction();
       /* 其他周期函数 */
   }
   ```

---

## 6. 使用示例

### 6.1 基本启动流程

```c
/* main.c */
#include "EcuM.h"

int main(void)
{
    /* EcuM_Init会自动执行StartupOne和StartupTwo */
    EcuM_Init();
    
    /* 不会返回 */
    return 0;
}
```

### 6.2 RUN请求管理

```c
/* App.c */
#include "EcuM.h"

#define USER_ID_APPLICATION 0
#define USER_ID_COMMUNICATION 1

void Application_Start(void)
{
    /* 应用启动时请求RUN */
    EcuM_RequestRUN(USER_ID_APPLICATION);
    
    /* 初始化应用 */
    App_Init();
}

void Application_Stop(void)
{
    /* 应用停止时释放RUN */
    EcuM_ReleaseRUN(USER_ID_APPLICATION);
}

void Communication_Start(void)
{
    /* 通信启动时请求RUN */
    EcuM_RequestRUN(USER_ID_COMMUNICATION);
    
    /* 启动通信 */
    ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
}
```

### 6.3 关机流程

```c
void Shutdown_Requested(void)
{
    /* 设置关机目标为OFF */
    EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_OFF, 0);
    
    /* 设置关机原因 */
    EcuM_SelectShutdownCause(ECUM_CAUSE_ECU_STATE);
    
    /* 释放所有RUN请求 */
    EcuM_ReleaseRUN(USER_ID_APPLICATION);
    EcuM_ReleaseRUN(USER_ID_COMMUNICATION);
    
    /* EcuM_MainFunction会检测无RUN请求并执行关机 */
}
```

### 6.4 唤醒处理

```c
/* 中断服务程序 */
void CAN_Wakeup_ISR(void)
{
    /* 设置唤醒事件 */
    EcuM_SetWakeupEvent(ECUM_WKSOURCE_CAN);
}

void Wakeup_Validation(void)
{
    /* 检查唤醒源状态 */
    EcuM_WakeupStatusType status = 
        EcuM_GetStatusOfWakeupSource(ECUM_WKSOURCE_CAN);
    
    if (status == ECUM_WKSTATUS_VALIDATED)
    {
        /* 唤醒已验证，可以处理 */
        Process_Wakeup();
    }
    else if (status == ECUM_WKSTATUS_EXPIRED)
    {
        /* 唤醒过期，忽略 */
        EcuM_ClearWakeupEvent(ECUM_WKSOURCE_CAN);
    }
}
```

---

## 7. 测试覆盖

### 7.1 单元测试

测试文件: `tests/unit/autosar/services/test_ecum.c`

| 测试类别 | 测试用例数 | 覆盖率 |
|----------|-----------|--------|
| 初始化测试 | 4 | 100% |
| 状态管理 | 6 | 95% |
| RUN请求管理 | 7 | 100% |
| 关机管理 | 9 | 90% |
| 睡眠管理 | 4 | 85% |
| 唤醒源管理 | 12 | 90% |
| 引导目标管理 | 4 | 100% |
| 应用模式管理 | 3 | 100% |
| 通信模式 | 2 | 100% |
| BSW模式 | 2 | 100% |
| 主函数 | 2 | 80% |
| 版本信息 | 2 | 100% |
| 集成测试 | 5 | 85% |
| **总计** | **62** | **>80%** |

### 7.2 测试执行

```bash
# 编译测试
cd /home/admin/yuleASR
cmake -B build -S . -DENABLE_TESTS=ON
cmake --build build --target test_ecum

# 运行测试
./build/tests/unit/autosar/services/test_ecum
```

---

## 8. 故障排除

### 8.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| EcuM_Init后无法进入RUN | StartupTwo失败 | 检查DriverInitTwo实现 |
| 无法进入睡眠 | RUN请求未释放 | 检查所有EcuM_RequestRUN是否有对应Release |
| 唤醒无效 | 唤醒源未使能 | 调用EcuM_EnableWakeupSources |
| 关机失败 | NvM_WriteAll阻塞 | 检查NvM配置和回调 |
| 状态机卡住 | 中断禁用 | 检查EcuM_DisableInterrupts实现 |

### 8.2 调试信息

启用DET报告获取详细错误信息:
```c
#define ECUM_DEV_ERROR_DETECT STD_ON
```

常见错误码:
- ECUM_E_NOT_INITIALIZED (0x10): 模块未初始化
- ECUM_E_INVALID_PAR (0x11): 无效参数
- ECUM_E_NULL_POINTER (0x12): NULL指针
- ECUM_E_STATE_CHANGE_FAILED (0x13): 状态转换失败
- ECUM_E_WRONG_API_ORDER (0x16): API调用顺序错误

---

## 9. 源代码路径

- **头文件**: `src/bsw/services/ecum/include/EcuM.h`
- **配置文件**: `src/bsw/services/ecum/include/EcuM_Cfg.h`
- **源文件**: `src/bsw/services/ecum/src/EcuM.c`

---

## 10. 版本历史

| 版本 | 日期 | 变更描述 |
|------|------|----------|
| 2.0.0 | 2026-05-15 | 多阶段启动/关机/睡眠完整实现 |
| 1.0.0 | 2024-04-23 | 初始版本 |

---

## 11. 参考文档

- AUTOSAR Classic Platform SWS ECU State Manager R4.0.3
- AUTOSAR Classic Platform SWS BSW Mode Manager R4.0.3
- AUTOSAR Classic Platform SWS Communication Manager R4.0.3
