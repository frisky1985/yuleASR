# WdgIf Design Document

> **Module ID**: 0x2B (43)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Watchdog Interface  
> **Source Path**: `src/bsw/ecual/wdgif/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

WdgIf (Watchdog Interface) 为 WdgM (Watchdog Manager) 提供统一的看门狗硬件访问接口。WdgIf 抽象不同的看门狗硬件（内部 WDG + 外部 Window Watchdog），提供触发条件设置、模式切换和状态查询的标准 API。WdgIf 是安全链路中监督机制的关键中间层。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Watchdog Interface | 4.4.0 | WdgIf 规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | WdgM | 看门狗管理器 |
| 下层 | Wdg (MCAL) | 看门狗硬件驱动 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│            WdgM                     │
├─────────────────────────────────────┤
│         WdgIf (ECUAL)               │
├─────────────────────────────────────┤
│         Wdg (MCAL)                  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **HW Abstraction**: 内部/外部看门狗统一接口
- **Mode Manager**: FAST/SLOW/OFF 模式管理
- **Trigger Interface**: 喂狗触发接口

---

## 4. 状态机

```
           WdgIf_Init()
  OFF ──────────────────► FAST
                            │
              WdgM SetMode()│
                            ▼
                         SLOW / OFF
```

---

## 5. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `Std_ReturnType WdgIf_Init(const WdgIf_ConfigType* Config)` | 初始化 | SWS_WdgIf_00001 |
| `Std_ReturnType WdgIf_SetMode(uint8 Instance, WdgIf_ModeType Mode)` | 设置模式 | SWS_WdgIf_00004 |
| `Std_ReturnType WdgIf_Trigger(uint8 Instance)` | 触发喂狗 | SWS_WdgIf_00005 |
| `Std_ReturnType WdgIf_SetTriggerCondition(uint8 Instance, uint16 Ticks)` | 设置触发条件 | SWS_WdgIf_00006 |
| `Std_ReturnType WdgIf_GetMode(uint8 Instance, WdgIf_ModeType* Mode)` | 获取当前模式 |  |

---

## 6. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `WDGIF_NUM_INSTANCES` | 2U | 看门狗实例数（内部+外部） |
| `WDGIF_FAST_MODE_PERIOD` | 10U | FAST 模式触发周期 (ms) |
| `WDGIF_SLOW_MODE_PERIOD` | 50U | SLOW 模式触发周期 (ms) |

---

## 7. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `WDGIF_E_UNINIT` | 初始化前调用 |
| `WDGIF_E_INV_INSTANCE` | 实例索引越界 |
| `WDGIF_E_INV_MODE` | 无效模式 |

---

## 8. 内存与性能

- **RAM**: ~16 字节/实例
- **ROM**: ~1.5 KB 代码
- **性能**: Trigger ~1 µs

---

## 9. 集成指南

- WdgM 通过 WdgIf 控制看门狗硬件
- 内部看门狗（IWDG）和外部看门狗（窗口看门狗）分别配置
- 触发周期需与 WdgM 监督周期匹配

---

## 10. 测试策略

- 模式切换测试
- 触发喂狗时序测试
- 超时复位测试
- 无效实例错误测试

---

## 11. 参考文献

- AUTOSAR_SWS_WatchdogInterface.pdf (R4.4.0)
- yuleASR WdgIf 源码: `src/bsw/ecual/wdgif/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_WdgIf_00002 | `WdgIf_DeInit` | 测试 test_WdgIf_DeInit_ValidCall_ShouldSucceed 覆盖: WdgIf_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_WdgIf_00003 | `WdgIf_GetVersionInfo` | 测试 test_WdgIf_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: WdgIf_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_WdgIf_00007 | `WdgIf_CheckReset` | 测试 test_WdgIf_CheckReset_ValidCall_ShouldSucceed 覆盖: WdgIf_CheckReset_ValidCall_ShouldSucceed 场景 |
| SWS_WdgIf_00008 | `WdgIf_SetTriggerCondition` | 测试 test_WdgIf_SetTriggerCondition_ValidCall_ShouldSucceed 覆盖: WdgIf_SetTriggerCondition_ValidCall_ShouldSucceed 场景 |
