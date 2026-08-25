# EthIf Design Document

> **Module ID**: 0x70 (112)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Ethernet Interface  
> **Source Path**: `src/bsw/ecual/ethif/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

EthIf (Ethernet Interface) 是 Eth MCAL 驱动的上层抽象，为上层模块（EthSM、TcpIp、DoIP 等）提供统一的以太网访问接口。EthIf 管理多个以太网控制器实例，处理帧类型分发、PHY 链路状态管理、MAC 地址过滤和 VLAN 标签处理。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Ethernet Interface | 4.4.0 | EthIf 规范 |
| IEEE 802.3 | — | 以太网标准 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | EthSM, TcpIp, DoIP | 网络管理 / 协议栈 |
| 下层 | Eth (MCAL) | 硬件驱动 |
| 下层 | EthTrcv | PHY 收发器 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   EthSM / TcpIp / DoIP / SoAd       │
├─────────────────────────────────────┤
│         EthIf (ECUAL)               │
├─────────────────────────────────────┤
│    Eth (MCAL) / EthTrcv             │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Controller Manager**: 管理多个以太网控制器实例
- **Frame Dispatcher**: 按 EtherType 分发接收帧到上层协议
- **Link State Monitor**: 监控 PHY 链路状态变化
- **MAC Filter**: 管理 MAC 地址过滤表

### 3.3 文件结构

```
src/bsw/ecual/ethif/
├── include/
│   ├── EthIf.h       # 公共 API
│   └── EthIf_Cfg.h   # 控制器配置
└── src/
    ├── EthIf.c        # 核心实现
    └── EthIf_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
          EthIf_Init()
  DOWN ──────────────────► ACTIVE
                             │
              Link Down Event │
                             ▼
                           DOWN
```

---

## 5. 数据结构

```c
typedef enum {
    ETHIF_STATE_DOWN = 0,
    ETHIF_STATE_ACTIVE
} EthIf_StateType;

typedef struct {
    uint8  CtrlIdx;
    uint8  MacAddr[6];
    uint16 MtuSize;
    boolean VlanSupport;
    uint16 VlanId;
} EthIf_ControllerConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void EthIf_Init(const EthIf_ConfigType* Config)` | 初始化 | SWS_EthIf_00001 |
| `void EthIf_DeInit(void)` | 反初始化 | SWS_EthIf_00002 |
| `Std_ReturnType EthIf_Transmit(uint8, uint32, const EthIf_PduType*)` | 发送以太网帧 | SWS_EthIf_00003 |
| `Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, EthIf_ControllerMode Mode)` | 设置控制器模式 | SWS_EthIf_00004 |
| `EthIf_ControllerMode EthIf_GetControllerMode(uint8 CtrlIdx)` | 获取控制器模式 | SWS_EthIf_00005 |
| `void EthIf_RxIndication(uint8 CtrlIdx, const EthIf_PduType* PduInfoPtr)` | 帧接收回调（来自 Eth） | SWS_EthIf_00006 |
| `void EthIf_TxConfirmation(uint8 CtrlIdx, uint32 BufferHandle)` | 发送确认 | SWS_EthIf_00007 |
| `void EthIf_MainFunction(void)` | 周期主函数 | SWS_EthIf_00008 |
| `void EthIf_GetVersionInfo(Std_VersionInfoType*)` | 获取版本信息 | SWS_EthIf_00009 |

---

## 7. 处理流程

### 7.1 帧接收分发流程

1. Eth MCAL 调用 `EthIf_RxIndication` 传递接收帧
2. EthIf 解析 EtherType 字段
3. 根据 EtherType 分发到对应上层：
   - 0x0800 → IPv4 → TcpIp
   - 0x8100 → VLAN 标签剥离后分发
   - 0x88F7 → gPTP
4. 调用对应上层的 RxIndication 回调

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `ETHIF_NUM_CONTROLLERS` | 1U | 以太网控制器数 |
| `ETHIF_VLAN_SUPPORT` | STD_OFF | VLAN 标签支持 |
| `ETHIF_MAX_FRAME_SIZE` | 1522U | 最大帧大小 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `ETHIF_E_UNINIT` | 初始化前调用 |
| `ETHIF_E_INV_CTRL` | 控制器索引越界 |
| `ETHIF_E_LINK_DOWN` | 链路断开时发送 |

---

## 10. 内存与性能

- **RAM**: 每控制器 ~64 字节
- **ROM**: ~3 KB 代码
- **性能**: 帧分发 ~2 µs/帧

---

## 11. 集成指南

- EthSM 通过 `EthIf_SetControllerMode` 控制链路状态
- TcpIp 注册 EtherType 回调接收 IP 帧
- DoIP 注册诊断帧回调

---

## 12. 测试策略

- 控制器模式切换测试
- 帧类型分发正确性测试
- 链路状态变化测试
- VLAN 标签处理测试

---

## 13. 实现说明

- 帧分发使用静态 EtherType 映射表
- PHY 链路状态通过 EthTrcv 回调获取
- 支持多播/广播 MAC 过滤

---

## 14. 参考文献

- AUTOSAR_SWS_EthernetInterface.pdf (R4.4.0)
- yuleASR EthIf 源码: `src/bsw/ecual/ethif/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_EthIf_00010 | `EthIf_GetPhyState` | 测试 test_EthIf_GetPhyState_ValidCall_ShouldSucceed 覆盖: EthIf_GetPhyState_ValidCall_ShouldSucceed 场景 |
| SWS_EthIf_00011 | `EthIf_SetForwardingMode` | 测试 test_EthIf_SetForwardingMode_ValidCall_ShouldSucceed 覆盖: EthIf_SetForwardingMode_ValidCall_ShouldSucceed 场景 |
