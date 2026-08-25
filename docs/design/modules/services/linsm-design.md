# LinSM Design Document

> **Module ID**: 0x8F (143)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS LIN State Manager  
> **Source Path**: `src/bsw/services/linsm/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

LinSM (LIN State Manager) 管理 LIN 总线的网络状态机，控制 LIN 集群从休眠到唤醒、从初始化到正常运行之间的状态转换。LinSM 作为 LinIf 的上层，协调 Master/Slave 节点的调度表切换和总线睡眠管理。LinSM 与 UDS 睡眠/唤醒请求（DTC 31 服务）紧密集成。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LIN State Manager | 4.4.0 | LIN 状态管理 |
| LIN Protocol | 2.1/2.2 | LIN 总线协议 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ComM, Dcm | 通信管理 / 诊断睡眠请求 |
| 下层 | LinIf | LIN 调度接口 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      ComM / Dcm                     │
├─────────────────────────────────────┤
│       LinSM (Services)              │
├─────────────────────────────────────┤
│       LinIf / Lin                   │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **LIN 状态机**: INIT → READY → SLEEP 三态控制
- **调度表管理**: 根据状态切换调度表
- **唤醒管理**: 处理本地/远程唤醒事件

### 3.3 文件结构

```
src/bsw/services/linsm/
├── include/
│   ├── LinSM.h       # 公共 API
│   └── LinSM_Cfg.h   # 通道配置
└── src/
    ├── LinSM.c        # 核心实现
    └── LinSM_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
              LinSM_Init()
   INIT ────────────────────► READY
                                │
                   LinSM_RequestSleep()
                                │
                                ▼
         LinSM_ConfirmPduConfirmation()
                                │
                                ▼
                             SLEEP
                                │
                   Wakeup Event / LinSM_RequestWakeup()
                                │
                                ▼
                             READY
```

---

## 5. 数据结构

```c
typedef enum {
    LINSM_STATE_INIT = 0,
    LINSM_STATE_READY,
    LINSM_STATE_SLEEP,
    LINSM_STATE_GOTO_SLEEP
} LinSM_StateType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void LinSM_Init(const LinSM_ConfigType* Config)` | 初始化 | SWS_LinSM_00001 |
| `void LinSM_DeInit(void)` | 反初始化 | SWS_LinSM_00002 |
| `Std_ReturnType LinSM_RequestSleep(uint8 Channel)` | 请求进入睡眠 | SWS_LinSM_00003 |
| `Std_ReturnType LinSM_GotoSleep(uint8 Channel)` | 执行睡眠转换 | SWS_LinSM_00004 |
| `void LinSM_SleepConfirmation(uint8 Channel, LinSM_StateType State)` | 睡眠确认回调 | SWS_LinSM_00005 |
| `LinSM_StateType LinSM_GetState(uint8 Channel)` | 获取当前状态 | SWS_LinSM_00006 |
| `void LinSM_MainFunction(void)` | 周期主函数 | SWS_LinSM_00007 |

---

## 7. 处理流程

### 7.1 睡眠转换流程

1. ComM 或 Dcm 请求睡眠
2. LinSM 通知 LinIf 停止当前调度表
3. 等待所有进行中的 LIN 帧完成
4. LinIf 确认空闲 → LinSM 切换到 SLEEP 状态
5. 通知 ComM 睡眠确认

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `LINSM_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |
| `LINSM_SLEEP_TIMEOUT` | 1000U | 睡眠转换超时 (ms) |
| `LINSM_NUM_CHANNELS` | 2U | LIN 通道数 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `LINSM_E_UNINIT` | 初始化前调用 API |
| `LINSM_E_INV_CHANNEL` | 通道号越界 |
| `LINSM_E_TRANS_TIMEOUT` | 状态转换超时 |

---

## 10. 内存与性能

- **RAM**: 每通道 ~16 字节
- **ROM**: ~3 KB 代码
- **性能**: MainFunction ~2 µs/通道

---

## 11. 集成指南

- ComM 通过 LinSM 控制 LIN 网络睡眠
- Dcm 31 服务通过 LinSM 请求总线睡眠
- LinIf 提供调度表切换和帧完成确认

---

## 12. 测试策略

- INIT → READY → SLEEP 状态转换测试
- 睡眠转换超时测试
- 唤醒恢复测试
- 多通道独立控制测试

---

## 13. 实现说明

- 状态转换使用定时器超时保护
- 与 ComM 通过 `ComM_BusSMModeIndication` 交互

---

## 14. 参考文献

- AUTOSAR_SWS_LINStateManager.pdf (R4.4.0)
- yuleASR LinSM 源码: `src/bsw/services/linsm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_LinSM | — | LINSM 模块级需求归属 |
| SWS_LinSM_00008 | `LinSM_MainFunction` | 测试 test_LinSM_MainFunction_AfterInit_ShouldNotCrash 覆盖: LinSM_MainFunction_AfterInit_ShouldNotCrash 场景 |
| SWS_LinSM_00009 | `LinSM_ScheduleConfirmation` | 测试 test_LinSM_ScheduleConfirmation_ShouldNotCrash 覆盖: LinSM_ScheduleConfirmation_ShouldNotCrash 场景 |
| SWS_LinSM_00010 | `LinSM_WakeUpConfirmation` | 测试 test_LinSM_WakeUpConfirmation_ShouldNotCrash 覆盖: LinSM_WakeUpConfirmation_ShouldNotCrash 场景 |
| SWS_LinSM_00011 | `LinSM_GotoSleepConfirmation` | 测试 test_LinSM_GotoSleepConfirmation_ShouldNotCrash 覆盖: LinSM_GotoSleepConfirmation_ShouldNotCrash 场景 |
