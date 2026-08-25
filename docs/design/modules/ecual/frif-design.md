# FrIf Design Document

> **Module ID**: 0x3F (63)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS FlexRay Interface  
> **Source Path**: `src/bsw/ecual/frif/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

FrIf (FlexRay Interface) 是 FlexRay 总线的 ECUAL 层接口模块，为上层（FrTp、PduR）提供统一的 FlexRay 帧收发接口。FrIf 管理 FlexRay 集群的通信周期（Communication Cycle），处理静态段/动态段帧调度、符号窗口、NIT 间隙和 PDU 到 FlexRay Frame 的映射。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS FlexRay Interface | 4.4.0 | FrIf 规范 |
| FlexRay Protocol | 3.0 | FlexRay 总线协议 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | FrTp, PduR | 传输协议 / PDU 路由 |
| 下层 | Fr (MCAL) | FlexRay 硬件驱动 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       FrTp / PduR                   │
├─────────────────────────────────────┤
│       FrIf (ECUAL)                  │
├─────────────────────────────────────┤
│       Fr (MCAL)                     │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **PDU Mapper**: PDU 到 FlexRay Frame Slot 的映射
- **Cycle Manager**: 管理通信周期（奇/偶周期、宏 ticks）
- **Slot Handler**: 静态段/动态段帧调度
- **Wakeup Controller**: 集群唤醒管理

### 3.3 文件结构

```
src/bsw/ecual/frif/
├── include/
│   ├── FrIf.h       # 公共 API
│   └── FrIf_Cfg.h   # 集群/帧配置
└── src/
    └── FrIf.c        # 核心实现
```

---

## 4. 状态机

```
           FrIf_Init()
  DEFAULT ──────────────► CONFIG
                            │
              FrIf_Transmit()│ Cluster Start
                            ▼
                        NORMAL_ACTIVE
                       (通信周期运行中)
```

---

## 5. 数据结构

```c
typedef struct {
    uint16 FrameId;
    uint8  SlotId;
    uint8  CycleFilter;    /* 奇/偶/所有 */
    uint8  PayloadLength;
    uint8* DataPtr;
    void (*RxIndication)(uint16, const uint8*);
} FrIf_FrameConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void FrIf_Init(const FrIf_ConfigType* Config)` | 初始化 | SWS_FrIf_00001 |
| `void FrIf_DeInit(void)` | 反初始化 |  |
| `Std_ReturnType FrIf_Transmit(uint16 PduId, const PduInfoType* PduInfo)` | 发送 PDU | SWS_FrIf_00009 |
| `void FrIf_RxIndication(uint16 FrameId, const uint8* DataPtr, uint8 Length)` | 帧接收回调 |  |
| `void FrIf_TxConfirmation(uint16 FrameId, Std_ReturnType Result)` | 发送确认 |  |
| `void FrIf_MainFunction(void)` | 周期主函数 | SWS_FrIf_00003 |

---

## 7. 处理流程

### 7.1 PDU 发送流程

1. FrTp 调用 `FrIf_Transmit(PduId, PduInfo)`
2. FrIf 查找 PduId 对应的 Frame 配置
3. 将 PDU 数据拷贝到 FlexRay Frame 缓冲区
4. 在下一个匹配的 Slot 中发送
5. 发送完成后回调 `FrIf_TxConfirmation`

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `FRIF_NUM_FRAMES` | 32U | FlexRay 帧数量 |
| `FRIF_NUM_SLOTS` | 64U | 静态段 Slot 数 |
| `FRIF_CYCLE_LENGTH` | 5000U | 通信周期长度 (宏 ticks) |
| `FRIF_MAIN_FUNCTION_PERIOD` | 1U | 主函数周期 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `FRIF_E_UNINIT` | 初始化前调用 |
| `FRIF_E_INV_PDU` | 无效 PDU ID |
| `FRIF_E_INV_FRAME` | 无效帧 ID |

---

## 10. 内存与性能

- **RAM**: 帧配置表 ~32 × 20B = 640 字节
- **ROM**: ~4 KB 代码
- **性能**: 帧映射查找 O(1)（直接索引）

---

## 11. 集成指南

- FrTp 通过 FrIf 发送/接收 FlexRay TP 帧
- FlexRay 帧配置（Slot 分配、周期过滤）在 Lcfg 中静态定义
- 需与 Fr MCAL 的 CC 配置（集群参数）一致

---

## 12. 测试策略

- PDU 发送/接收往返测试
- Slot 调度正确性测试
- 奇/偶周期过滤测试
- 帧超时处理测试

---

## 13. 实现说明

- PDU 到 Frame 的映射使用静态配置表
- 支持 FlexRay 双通道（A/B）冗余
- Wakeup 通过 Fr MCAL 的 WUP 模式实现

---

## 14. 参考文献

- AUTOSAR_SWS_FlexRayInterface.pdf (R4.4.0)
- FlexRay Communications System Protocol Specification v3.0
- yuleASR FrIf 源码: `src/bsw/ecual/frif/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_FrIf_00002 | `FrIf_DeInit` | 测试 test_FrIf_DeInit_ValidCall_ShouldSucceed 覆盖: FrIf_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_FrIf_00004 | `FrIf_Transmit` | 测试 test_FrIf_Transmit_ValidCall_ShouldSucceed 覆盖: FrIf_Transmit_ValidCall_ShouldSucceed 场景 |
| SWS_FrIf_00005 | `FrIf_Cancel` | 测试 test_FrIf_Cancel_ValidCall_ShouldSucceed 覆盖: FrIf_Cancel_ValidCall_ShouldSucceed 场景 |
| SWS_FrIf_00006 | `FrIf_GetCtrlIdx` | 测试 test_FrIf_GetCtrlIdx_ValidCall_ShouldSucceed 覆盖: FrIf_GetCtrlIdx_ValidCall_ShouldSucceed 场景 |
| SWS_FrIf_00007 | `FrIf_GetCtrlMode` | 测试 test_FrIf_GetCtrlMode_ValidCall_ShouldReturnMode 覆盖: FrIf_GetCtrlMode_ValidCall_ShouldReturnMode 场景 |
| SWS_FrIf_00008 | `FrIf_SetCtrlMode` | 测试 test_FrIf_SetCtrlMode_ValidCall_ShouldSucceed 覆盖: FrIf_SetCtrlMode_ValidCall_ShouldSucceed 场景 |
