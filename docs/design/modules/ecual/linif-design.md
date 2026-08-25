# LinIf Design Document

> **Module ID**: 0x27 (39)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS LIN Interface  
> **Source Path**: `src/bsw/ecual/linif/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

LinIf (LIN Interface) 是 LIN 总线的 ECUAL 层模块，为上层（LinSM、Com）提供统一的 LIN 帧调度接口。LinIf 管理 LIN 调度表（Schedule Table），控制 Master 帧头的发送和 Slave 响应的接收，支持无条件帧、事件触发帧、偶发帧和诊断帧等多种帧类型。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LIN Interface | 4.4.0 | LIN 接口规范 |
| LIN Protocol Specification | 2.1/2.2 | LIN 总线协议 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | LinSM, Com | 状态管理 / 信号收发 |
| 下层 | Lin (MCAL) | 硬件驱动 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      LinSM / Com                    │
├─────────────────────────────────────┤
│       LinIf (ECUAL)                 │
├─────────────────────────────────────┤
│       Lin (MCAL)                    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Schedule Table Manager**: 管理多个调度表的切换和执行
- **Frame Handler**: 处理 Master/Slave 帧的发送和接收
- **Channel Controller**: 管理 LIN 通道的状态和配置

### 3.3 文件结构

```
src/bsw/ecual/linif/
├── include/
│   ├── LinIf.h       # 公共 API
│   └── LinIf_Cfg.h   # 通道/调度表配置
└── src/
    ├── LinIf.c        # 核心实现
    └── LinIf_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
          LinIf_Init()
  UNINIT ──────────────► IDLE
                           │
         LinIf_SwitchScheduleTable()
                           │
                           ▼
                     EXECUTING
                     (调度表运行中)
```

---

## 5. 数据结构

```c
typedef struct {
    uint8  FrameId;            /* LIN 帧 ID (Protected ID) */
    uint8  Length;             /* 帧数据长度 (1/2/4/8) */
    uint8  Direction;          /* MASTER_TX / SLAVE_RX / SLAVE_TX */
    uint8* DataPtr;            /* 数据缓冲区指针 */
    void (*RxIndication)(uint8, const uint8*);  /* 接收回调 */
} LinIf_FrameConfigType;

typedef struct {
    uint8  TableId;
    const LinIf_FrameConfigType* Frames;
    uint8  NumFrames;
    uint32 RunMode;            /* CONTINUOUS / ONCE */
} LinIf_ScheduleTableType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void LinIf_Init(const LinIf_ConfigType* Config)` | 初始化 | SWS_LinIf_00001 |
| `void LinIf_DeInit(void)` | 反初始化 | SWS_LinIf_00002 |
| `Std_ReturnType LinIf_SwitchScheduleTable(uint8 Channel, uint8 TableId)` | 切换调度表 | SWS_LinIf_00003 |
| `Std_ReturnType LinIf_GotoSleep(uint8 Channel)` | 请求进入睡眠 | SWS_LinIf_00004 |
| `void LinIf_RxIndication(uint8 Channel, uint8* DataPtr, uint8 Length)` | 帧接收回调 | SWS_LinIf_00005 |
| `void LinIf_TxConfirmation(uint8 Channel, Std_ReturnType Result)` | 发送确认 | SWS_LinIf_00006 |
| `void LinIf_MainFunction(void)` | 周期主函数 | SWS_LinIf_00007 |

---

## 7. 处理流程

### 7.1 MainFunction 调度流程

1. 遍历每个 LIN 通道
2. 检查当前调度表是否处于 EXECUTING 状态
3. 按调度表顺序取出下一帧配置
4. 根据帧方向调用 Lin 驱动发送/接收
5. 更新调度表索引（循环或单次）

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `LINIF_NUM_CHANNELS` | 2U | LIN 通道数 |
| `LINIF_MAX_SCHEDULE_TABLES` | 4U | 每通道最大调度表数 |
| `LINIF_MAIN_FUNCTION_PERIOD` | 5U | 主函数周期 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `LINIF_E_UNINIT` | 初始化前调用 |
| `LINIF_E_INV_CHANNEL` | 通道号越界 |
| `LINIF_E_INV_TABLE` | 调度表 ID 越界 |

---

## 10. 内存与性能

- **RAM**: 每通道 ~48 字节 + 调度表帧配置
- **ROM**: ~3 KB 代码
- **性能**: 每帧调度 ~10 µs

---

## 11. 集成指南

- LinSM 通过 `LinIf_SwitchScheduleTable` 控制调度表
- Com 通过 LinIf 的 RxIndication 回调接收 LIN 信号
- 调度表配置在 Lcfg 中静态定义

---

## 12. 测试策略

- 调度表切换测试
- Master/Slave 帧收发测试
- 调度表循环/单次执行测试
- 睡眠/唤醒转换测试

---

## 13. 实现说明

- 调度表使用静态配置（预编译），运行时仅切换索引
- Protected ID 由 FrameId + Parity 计算
- 支持诊断帧（Master Request / Slave Response）

---

## 14. 参考文献

- AUTOSAR_SWS_LINInterface.pdf (R4.4.0)
- yuleASR LinIf 源码: `src/bsw/ecual/linif/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_LinIf_00008 | `LinIf_GetFrameStatus` | 测试 test_LinIf_GetFrameStatus_ValidCall_ShouldReturnStatus 覆盖: LinIf_GetFrameStatus_ValidCall_ShouldReturnStatus 场景 |
