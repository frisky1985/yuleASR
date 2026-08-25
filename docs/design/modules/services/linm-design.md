# LinM Design Document

> **Module ID**: 0x34 (52)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_LINMaster  
> **Source Path**: `src/bsw/services/linm/`  
> **Reference Document**: `docs/modules/linm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

LinM（LIN Master Management）是 AUTOSAR BSW 服务层的 LIN 主节点管理模块。该模块负责管理 LIN 总线上的帧调度（Schedule Table），控制主节点按照预定义的调度表顺序发送帧头（Header），并管理从节点的唤醒（WakeUp）和休眠（GoToSleep）功能。

LinM 支持多个调度表（Schedule Table），每个调度表包含多个调度条目（Schedule Entry），每个条目定义了帧类型（Unconditional、Event Triggered、Sporadic、Diagnostic、Slave-to-Slave）和延迟时间。模块支持调度的单次执行和循环执行两种模式。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LINMaster | 4.4.0 | LIN 主节点管理模块规范 |
| LIN Protocol Specification | 2.1 | LIN 协议规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | LinSM | LIN 状态管理 |
| 上层 | LinTp | LIN 传输层 |
| 下层 | LinIf | LIN 接口 |
| 下层 | Lin | LIN 驱动 |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   LinSM / LinTp (上层)              │
├─────────────────────────────────────┤
│   LinM (LIN 主节点管理)              │
├─────────────────────────────────────┤
│   LinIf (LIN 接口)                   │
├─────────────────────────────────────┤
│   Lin Driver (LIN 驱动)              │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **调度管理**: 管理调度表的初始化、启动、停止和模式设置
- **调度执行引擎**: 按顺序执行调度条目，处理帧头发送和延迟控制
- **帧类型处理**: 支持 Unconditional、Event Triggered、Sporadic、Diagnostic、Slave-to-Slave 五种帧类型
- **唤醒/休眠管理**: 通过 LIN 总线发送 WakeUp 信号和 GoToSleep 命令
- **从节点响应监控**: 跟踪从节点的响应状态（OK/Error/Timeout/Invalid）

### 3.3 文件结构

```
src/bsw/services/linm/
├── include/
│   ├── LinM.h          # 公共 API 与类型声明
│   └── LinM_Cfg.h      # 预编译配置（自动生成）
└── src/
    ├── LinM.c           # 核心实现
    └── LinM_Lcfg.c      # 链接时配置
```

---

## 4. 状态机

### 4.1 调度状态

```
              StartSchedule()
   IDLE ──────────────────────► RUNNING
     ▲                            │
     │  StopSchedule()            │ Schedule 完成 (ONCE 模式)
     └────────────────────────────┘
```

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `LINM_SCHEDULE_IDLE` | 0 | 空闲 |
| `LINM_SCHEDULE_RUNNING` | 1 | 运行中 |
| `LINM_SCHEDULE_WAITING` | 2 | 等待中 |
| `LINM_SCHEDULE_PAUSED` | 3 | 暂停 |

### 4.2 调度模式

| 模式 | 枚举值 | 说明 |
|------|--------|------|
| `LINM_SCHEDULE_MODE_STOPPED` | 0 | 已停止 |
| `LINM_SCHEDULE_MODE_STARTED` | 1 | 已启动 |
| `LINM_SCHEDULE_MODE_CONTINUE` | 2 | 继续执行 |
| `LINM_SCHEDULE_MODE_ONCE` | 3 | 单次执行 |

---

## 5. 核心数据结构

### 5.1 调度条目配置 `LinM_ScheduleEntryConfigType`

```c
typedef struct {
    uint16 Delay;        // 条目延迟 (ms)
    uint8  FrameIndex;   // 帧索引（0xFF 为空）
    uint8  FrameType;    // 帧类型
} LinM_ScheduleEntryConfigType;
```

### 5.2 通道运行时状态 `LinM_ChannelStateType`

```c
typedef struct {
    LinM_ScheduleStatusType ScheduleStatus;      // 调度状态
    LinM_ScheduleModeType ScheduleMode;          // 调度模式
    LinM_ScheduleType CurrentSchedule;           // 当前调度表
    LinM_ScheduleEntryType CurrentEntry;         // 当前条目索引
    LinM_SlaveResponseStatusType SlaveResponse;  // 从节点响应
    uint16 ScheduleTimer;                        // 调度定时器
    uint16 WakeupTimer;                          // 唤醒定时器
    uint16 SleepTimer;                           // 休眠定时器
    boolean Initialized;                         // 初始化标志
} LinM_ChannelStateType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `LinM_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `LinM_DeInit()` | 0x02 | 反初始化 |
| `LinM_GetVersionInfo(VersionInfo)` | 0x03 | 获取版本信息 |
| `LinM_InitSchedule(Channel, Schedule)` | 0x04 | 初始化调度表 |
| `LinM_StartSchedule(Channel, Schedule)` | 0x05 | 启动调度表 |
| `LinM_StopSchedule(Channel)` | 0x06 | 停止调度表 |
| `LinM_SetScheduleMode(Channel, Mode)` | 0x07 | 设置调度模式 |
| `LinM_GetScheduleStatus(Channel, Status)` | 0x08 | 获取调度状态 |
| `LinM_MainFunction()` | 0x09 | 主函数 |
| `LinM_WakeUp(Channel)` | 0x0A | 发送唤醒信号 |
| `LinM_GotoSleep(Channel)` | 0x0B | 发送休眠命令 |
| `LinM_GetSlaveResponse(Channel, Status)` | 0x0C | 获取从节点响应 |

### 6.2 错误码

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `LINM_E_NOT_INITIALIZED` | 未初始化 |
| 0x02 | `LINM_E_INVALID_PARAMETER` | 无效参数 |
| 0x03 | `LINM_E_INVALID_POINTER` | 无效指针 |
| 0x04 | `LINM_E_INVALID_SCHEDULE` | 无效调度表 |
| 0x05 | `LINM_E_SCHEDULE_NOT_RUNNING` | 调度未运行 |
| 0x06 | `LINM_E_INIT_FAILED` | 初始化失败 |

---

## 7. 处理流程

### 7.1 调度执行流程

`LinM_MainFunction()` 周期调用（默认 5ms）:

1. 检查模块已初始化
2. 遍历所有通道:
   - 检查调度状态为 RUNNING
   - 检查当前调度表有效
   - 递减调度定时器
   - 定时器到期时:
     1. 执行当前条目（根据帧类型发送帧头）
     2. 设置下一条目延迟定时器
     3. 条目索引递增
     4. 到达调度表末尾时:
        - ONCE 模式: 停止调度，状态变为 IDLE
        - 其他模式: 重置条目索引，继续循环

### 7.2 帧类型处理

| 帧类型 | 值 | 处理 |
|--------|----|------|
| Unconditional | 0 | 发送无条件帧头 |
| Event Triggered | 1 | 发送事件触发帧头 |
| Diagnostic | 3 | 发送诊断帧头 |
| Sporadic | 2 | 发送零星帧头 |
| Slave-to-Slave | 4 | 发送从到从帧头 |
| Empty | 255 | 空条目，不操作 |

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `LINM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `LINM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `LINM_NUMBER_OF_CHANNELS` | 1 | 通道数量 |
| `LINM_NUMBER_OF_SCHEDULES` | 4 | 调度表数量 |
| `LINM_NUMBER_OF_ENTRIES` | 16 | 每调度表条目数 |
| `LINM_WAKEUP_TIMEOUT_MS` | 50 | 唤醒超时 (ms) |
| `LINM_SLEEP_TIMEOUT_MS` | 100 | 休眠超时 (ms) |
| `LINM_SLAVE_RESPONSE_TIMEOUT_MS` | 20 | 从节点响应超时 (ms) |
| `LINM_MAIN_FUNCTION_PERIOD_MS` | 5 | 主函数周期 (ms) |
| `LINM_WAKEUP_SUPPORT` | STD_ON | 唤醒支持 |
| `LINM_SLEEP_SUPPORT` | STD_ON | 休眠支持 |

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口进行初始化状态、通道有效性、调度表有效性、指针非空检查。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。

### 9.3 安全机制

- **编译时版本检查**: `#error` 宏确保 AR 版本一致性
- **调度表边界检查**: 验证调度 ID 和条目索引不越界
- **从节点超时检测**: 通过 `SlaveResponse` 状态跟踪从节点响应

---

## 10. 内存与性能

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每通道 RAM | ~20 bytes | ChannelStateType |
| 调度配置 ROM | ~200 bytes | 4 调度表 × 16 条目 × ~3 bytes |
| ROM（代码） | ~4 KB | 调度引擎 + API |

---

## 11. 集成指南

1. **LinSM 集成**: LinSM 通过 `LinM_StartSchedule()` / `LinM_StopSchedule()` 管理调度
2. **LinIf 集成**: LinM 通过 LinIf 发送帧头
3. **LinTp 集成**: 诊断调度表用于 LinTp 消息传输
4. **EcuM 集成**: 在启动阶段调用 `LinM_Init()`
5. **SchM 集成**: 配置 `LinM_MainFunction()` 调用周期（5ms）

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针检测、通道初始化验证 |
| 调度启动/停止 | StartSchedule/StopSchedule 状态转换 |
| 调度执行 | 条目顺序执行、循环执行、单次执行 |
| 帧类型处理 | 各帧类型的帧头发送 |
| 唤醒/休眠 | WakeUp/GotoSleep 定时器管理 |
| 从节点响应 | GetSlaveResponse 返回值验证 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| LinSM-LinM 集成 | 完整的调度管理流程 |
| LinM-LinIf 集成 | 帧头发送的端到端验证 |
| 诊断调度 | LinTp 诊断消息的调度传输 |

---

## 13. 实现说明 / TODO

- `LinM_SendFrameHeader()` 为桩实现（注释 "In real implementation, this would call Lin_SendHeader"）
- `LinM_WakeUp()` 和 `LinM_GotoSleep()` 仅设置定时器，未调用实际 Lin 驱动 API
- 从节点响应状态 `SlaveResponse` 始终为 `LINM_SLAVE_RESPONSE_INVALID`，未从 LinIf 回调更新
- 调度优先级（Priority）配置已定义但未在调度执行中使用
- 事件触发调度的处理逻辑尚未实现

---

## 14. 参考资料

- AUTOSAR SWS LINMaster (AUTOSAR_SWS_LINMaster.pdf)
- LIN Protocol Specification V2.1
- 源码: `src/bsw/services/linm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_LinM_00001 | `LinM` | 测试 test_LinM_Init_NullConfig 覆盖: LinM_Init_NullConfig 场景 |
| SWS_LinM_00002 | `LinM_DeInit` | 测试 test_LinM_DeInit_ValidCall_ShouldSucceed 覆盖: LinM_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_LinM_00003 | `LinM_GetVersionInfo` | 测试 test_LinM_GetVersionInfo_NullPointer 覆盖: LinM_GetVersionInfo_NullPointer 场景 |
| SWS_LinM_00004 | `LinM_MainFunction` | 测试 test_LinM_MainFunction 覆盖: LinM_MainFunction 场景 |
| SWS_LinM_00005 | `LinM_RequestComMode` | 测试 test_LinM_RequestComMode_ValidCall_ShouldSucceed 覆盖: LinM_RequestComMode_ValidCall_ShouldSucceed 场景 |
| SWS_LinM_00006 | `LinM_GetComMode` | 测试 test_LinM_GetComMode_ValidCall_ShouldReturnMode 覆盖: LinM_GetComMode_ValidCall_ShouldReturnMode 场景 |
| SWS_LinM_00007 | `LinM_ScheduleRequest` | 测试 test_LinM_ScheduleRequest_ValidCall_ShouldSucceed 覆盖: LinM_ScheduleRequest_ValidCall_ShouldSucceed 场景 |
| SWS_LinM_00008 | `LinM_SetScheduleMode` | 测试 test_LinM_SetScheduleMode 覆盖: LinM_SetScheduleMode 场景 |
| SWS_LinM_00010 | `LinM_WakeUp` | 测试 test_LinM_WakeUp 覆盖: LinM_WakeUp 场景 |
| SWS_LinM_00011 | `LinM_GotoSleep` | 测试 test_LinM_GotoSleep 覆盖: LinM_GotoSleep 场景 |
| SWS_LinM_00012 | `LinM_GetSlaveResponse` | 测试 test_LinM_GetSlaveResponse 覆盖: LinM_GetSlaveResponse 场景 |
