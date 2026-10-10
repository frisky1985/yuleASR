# 系统级安全概念 (System-Level Safety Concept)

> **文档编号**: YS-SC-001  
> **版本**: 1.0.0  
> **日期**: 2026-10-25  
> **适用标准**: ISO 26262-2018 Part 3 (Concept Phase), Part 6 (Software Level)  
> **目标 ASIL**: ASIL B (存储/诊断/通信)

---

## 1. 文档目的与范围

本文档定义 YuleTech AutoSAR BSW 平台的系统级安全概念，覆盖从 HARA（危害分析与风险评估）到安全需求分解的完整概念阶段 (Concept Phase) 工作产物。

### 1.1 适用范围

| 层级 | 模块 | 安全相关性 |
|:-----|:-----|:----------|
| Service 层 | NvM (NVRAM Manager) | ASIL B — 安全参数持久化 |
| Service 层 | Dcm (Diagnostic Communication Manager) | ASIL B — 安全诊断通道 |
| Service 层 | Dem (Diagnostic Event Manager) | ASIL B — 故障事件管理 |
| Service 层 | Com / PduR | QM — 通信路由 (非安全关键) |
| Integration 层 | BswM / EcuM | ASIL B — 状态管理 |

### 1.2 排除范围

- MCAL 层驱动 (由 MCU 厂商安全手册覆盖)
- ASW 层应用组件 (由 OEM 系统集成覆盖)
- RTE 层 (作为透明传输层，无独立安全需求)

---

## 2. Item 定义 (Item Definition)

### 2.1 系统描述

YuleTech AutoSAR BSW 是一个符合 AUTOSAR Release 4.x 规范的基础软件平台，运行于 NXP i.MX8M Mini (ARM Cortex-A53) 处理器上，为上层应用组件提供标准化的系统服务。

### 2.2 Item 边界

```
┌─────────────────────────────────────────────────────────┐
│  ASW 层 (EngineControl, VehicleDynamics, ...)          │ ← 上层消费者
├─────────────────────────────────────────────────────────┤
│  RTE 层                                                 │ ← 接口层
├─────────────────────────────────────────────────────────┤
│  Service 层  ◄── 本文档范围                              │
│  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐         │
│  │ NvM │  │ Dcm │  │ Dem │  │ Com │  │PduR │         │
│  └──┬──┘  └──┬──┘  └──┬──┘  └─────┘  └─────┘         │
├─────┼────────┼────────┼────────────────────────────────┤
│  ECUAL 层 (MemIf, Fee, Ea, CanIf, ...)                │
├─────────────────────────────────────────────────────────┤
│  MCAL 层 (Mcu, Port, Can, Spi, ...)                   │ ← 硬件抽象
├─────────────────────────────────────────────────────────┤
│  Hardware: NXP i.MX8M Mini + External EEPROM/Flash    │
└─────────────────────────────────────────────────────────┘
```

### 2.3 Item 接口

| 接口 | 方向 | 描述 | 安全相关性 |
|:-----|:-----|:-----|:----------|
| NvM ↔ MemIf | 双向 | NV 存储读写请求 | 数据完整性 |
| Dcm ↔ PduR | 双向 | 诊断消息收发 | 通信完整性 |
| Dcm ↔ Dem | 单向 | DTC 信息查询 | 诊断数据一致性 |
| Dem ↔ NvM | 双向 | DTC/冻结帧持久化 | 故障数据可靠性 |
| Dem ↔ BswM | 事件 | 故障事件通知 | 状态切换时效 |
| EcuM ↔ 各模块 | 初始化/关机 | 生命周期管理 | 启动/关机顺序 |

### 2.4 运行场景 (Operational Situations)

| ID | 场景 | 描述 |
|:---|:-----|:-----|
| OS-01 | 正常运行 | ECU 处于 Run 状态，BSW 各模块周期性执行 MainFunction |
| OS-02 | NV 数据恢复 | 上电后 NvM_ReadAll 恢复安全相关参数 |
| OS-03 | 故障诊断 | 外部诊断仪通过 Dcm 读取/清除 DTC |
| OS-04 | 安全参数写入 | 标定参数通过 Dcm_WriteDataByIdentifier 写入并经 NvM 持久化 |
| OS-05 | 关机数据刷新 | EcuM 关机流程中 NvM_WriteAll 刷新脏数据 |
| OS-06 | NV 存储故障 | EEPROM/Flash 损坏导致数据不可用 |

---

## 3. 危害分析与风险评估 (HARA)

### 3.1 HARA 摘要表

| ID | 危害事件 | 严重度 (S) | 暴露度 (E) | 可控性 (C) | ASIL | 安全目标 (SG) |
|:---|:---------|:----------|:----------|:----------|:-----|:-------------|
| H-01 | 安全参数丢失导致执行器失控 | S3 | E3 | C2 | **B** | SG-01: 安全参数必须在 NV 存储故障时恢复为已知默认值 |
| H-02 | 诊断通道被未授权访问导致参数篡改 | S2 | E2 | C2 | **B** | SG-02: 安全相关 DID 写入必须经过安全访问验证 |
| H-03 | 故障事件丢失导致维修遗漏 | S2 | E3 | C1 | **B** | SG-03: DTC 及冻结帧必须在存储故障时有冗余保护 |
| H-04 | 诊断响应超时导致安全状态不确定 | S1 | E2 | C2 | **QM→B** | SG-04: 诊断会话超时后必须自动回退到默认会话并锁定安全等级 |
| H-05 | NV 写入中断导致数据损坏 | S3 | E2 | C2 | **B** | SG-05: 写入中断后必须能通过冗余副本或 CRC 检测恢复数据 |

### 3.2 风险评估依据

**H-01 (安全参数丢失)**:
- S3: 可能导致车辆失控 (如发动机标定参数丢失导致过浓/过稀混合气)
- E3: 每次驾驶循环都会读取 NV 参数
- C2: 驾驶员可能通过异常驾驶感受察觉，但不一定能控制

**H-02 (未授权参数篡改)**:
- S2: 可能导致排放超标或性能下降，但通常不会立即导致危险
- E2: 仅在诊断连接时可访问
- C2: 有安全访问保护，但绕过可能导致不可控

**H-03 (故障事件丢失)**:
- S2: 不直接影响车辆运行，但影响维修质量
- E3: 每次故障发生都需要记录
- C1: 驾驶员无法直接控制故障记录

---

## 4. 安全需求分解 (Safety Requirements Decomposition)

### 4.1 SG-01: 安全参数 NV 存储可靠性

| 需求 ID | 安全需求 | 分配模块 | ASIL |
|:--------|:---------|:---------|:-----|
| SR-01.1 | NvM 必须支持冗余块 (Redundant Block) 管理，主副本失败时自动切换到镜像副本 | NvM | B |
| SR-01.2 | NvM 读取时必须进行 CRC 校验 (CRC16 或 CRC32)，CRC 不匹配时触发冗余副本读取 | NvM | B |
| SR-01.3 | 当所有 NV 副本均不可用时，NvM 必须从 ROM 默认值恢复数据，并报告 `NVM_REQ_RESTORED_FROM_ROM` | NvM | B |
| SR-01.4 | NvM 必须提供 `NvM_RepairDamagedBlocks()` 接口，用于运行时检测和修复损坏的冗余块 | NvM | B |
| SR-01.5 | NvM 写入失败后必须支持可配置的重试次数 (`MaxNumberOfWriteRetries`) | NvM | B |

### 4.2 SG-02: 诊断通道安全访问控制

| 需求 ID | 安全需求 | 分配模块 | ASIL |
|:--------|:---------|:---------|:-----|
| SR-02.1 | Dcm 必须实现安全访问服务 (Security Access, SID 0x27)，支持 Seed-Key 机制 | Dcm | B |
| SR-02.2 | 安全访问失败次数超过 `DCM_MAX_SECURITY_ATTEMPTS` 时必须触发延迟锁定 (`SecurityDelayTimer`) | Dcm | B |
| SR-02.3 | 安全相关 DID 的写入必须检查当前安全等级 (`SecurityLevel`) 和会话类型 (`SessionType`) | Dcm | B |
| SR-02.4 | S3 会话超时后必须自动回退到默认会话 (`DCM_DEFAULT_SESSION`) 并锁定安全等级 (`DCM_SEC_LEV_LOCKED`) | Dcm | B |

### 4.3 SG-03: 故障事件存储可靠性

| 需求 ID | 安全需求 | 分配模块 | ASIL |
|:--------|:---------|:---------|:-----|
| SR-03.1 | Dem 必须在 DTC 确认时自动捕获冻结帧数据 (`Dem_IntStoreFreezeFrame`) | Dem | B |
| SR-03.2 | Dem 必须支持 DTC 状态持久化到 NvM，通过 `NvM_WriteBlock` 写入 | Dem → NvM | B |
| SR-03.3 | Dem 必须实现老化计数器 (`AgingCounter`)，达到阈值 (`AgingThreshold`) 后标记 DTC 为已老化 | Dem | B |
| SR-03.4 | Dem 必须支持操作周期管理 (`OperationCycle`)，跟踪 `TestFailedThisOperationCycle` 和 `TestCompletedThisOperationCycle` | Dem | B |

### 4.4 SG-04: 诊断会话超时保护

| 需求 ID | 安全需求 | 分配模块 | ASIL |
|:--------|:---------|:---------|:-----|
| SR-04.1 | Dcm 必须维护 S3 服务器定时器 (`S3Timer`)，超时后回退到默认会话 | Dcm | B |
| SR-04.2 | Dcm 必须维护 P2 服务器定时器 (`P2Timer`)，超时后发送 NRC 响应 | Dcm | B |
| SR-04.3 | TesterPresent 服务 (SID 0x3E) 必须重置 S3 定时器 | Dcm | B |

### 4.5 SG-05: NV 写入中断保护

| 需求 ID | 安全需求 | 分配模块 | ASIL |
|:--------|:---------|:---------|:-----|
| SR-05.1 | 冗余块写入时必须先写主副本，成功后再写镜像副本，保证至少一个副本有效 | NvM | B |
| SR-05.2 | 写入完成后必须递增 `WriteCounter` 用于写入次数追踪 | NvM | B |
| SR-05.3 | 支持 WriteOnce 保护，防止安全相关参数被意外覆盖 | NvM | B |

---

## 5. 故障检测与反应概念 (Fault Detection and Reaction Concept)

### 5.1 故障检测机制

| 故障类型 | 检测方法 | 检测时间 | 响应动作 |
|:---------|:---------|:---------|:---------|
| NV 数据损坏 | CRC 校验 (`NvM_CalculateCrc` vs 存储 CRC) | 每次 Read 操作 | 切换冗余副本 → ROM 恢复 |
| NV 写入失败 | MemIf 返回 `MEMIF_JOB_NOT_OK` | 写入操作期间 | 重试 → 报告 `NVM_REQ_NOT_OK` |
| 诊断会话超时 | S3Timer 递减到 0 | 周期性 (MainFunction) | 回退默认会话 + 锁定安全 |
| 安全访问暴力破解 | SecurityAttempts 计数器 | 每次 Security Access | 触发延迟锁定 |
| DTC 存储失败 | NvM 返回 `NVM_REQ_NOT_OK` | 写入操作期间 | 重试 → Dem 报告存储错误 |
| 诊断缓冲区溢出 | `DCM_RX_BUFFER_SIZE` / `DCM_TX_BUFFER_SIZE` 边界检查 | 每次 Rx/Tx | 截断 + NRC 响应 |

### 5.2 故障反应时间线

```
t=0        故障发生
  │
  ├─ NV 读取 CRC 不匹配
  │    t=0+Δt1   NvM_MainFunction 检测 MEMIF_IDLE + CRC mismatch
  │    t=0+Δt2   切换到冗余副本 (NvM_ReadRedundantBlock, CopyIndex=1)
  │    t=0+Δt3   冗余副本也失败 → NvM_CopyRomDataToRam → ROM 恢复
  │    总时间: < 3 × NvM_MainFunction 周期 (典型 < 30ms)
  │
  ├─ 诊断会话超时
  │    t=0       S3Timer 递减到 0 (Dcm_MainFunction)
  │    t=0+Δt    CurrentSession = DCM_DEFAULT_SESSION, SecurityLevel = LOCKED
  │    总时间: 1 个 MainFunction 周期 (典型 < 10ms)
  │
  └─ DTC 确认
       t=0       Dem_SetEventStatus 报告 FAILED
       t=0+Δt1   Debounce 算法通过 (counter/time/monitor)
       t=0+Δt2   Dem_IntUpdateDTCStatusFromDebounce 设置 TestFailed
       t=0+Δt3   Dem_IntStoreFreezeFrame 捕获冻结帧
       t=0+Δt4   NvM_WriteBlock 持久化 DTC 状态
       总时间: < 5 个 MainFunction 周期 (典型 < 50ms)
```

### 5.3 安全状态定义

| 模块 | 安全状态 | 进入条件 | 退出条件 |
|:-----|:---------|:---------|:---------|
| NvM | `NVM_SAFE_STATE_ROM_DEFAULT` | 所有 NV 副本不可用 | 下次成功写入 NV |
| NvM | `NVM_SAFE_STATE_REDUNDANT` | 主副本 CRC 失败，使用镜像副本 | 下次成功写入修复主副本 |
| Dcm | `DCM_SAFE_STATE_DEFAULT_SESSION` | S3 超时或 EcuM 重置 | 诊断仪请求扩展会话 |
| Dcm | `DCM_SAFE_STATE_LOCKED` | 安全访问失败超限 | 延迟时间到期后重新尝试 |
| Dem | `DEM_SAFE_STATE_DTC_DISABLED` | DTC 记录更新被禁用 | Dem_EnableDTCRecordUpdate 调用 |

---

## 6. 使用假设 (Assumptions of Use)

### 6.1 集成级安全假设

| ID | 假设 | 责任方 |
|:---|:-----|:-------|
| AU-01 | MCU 硬件看门狗在软件失控时触发复位 | MCU 厂商 / 系统集成商 |
| AU-02 | 外部 EEPROM 的写入耐久性满足产品生命周期要求 (> 100k cycles) | 硬件设计 |
| AU-03 | EcuM 保证 NvM_Init 在所有 ReadAll 之前完成 | 系统集成商 |
| AU-04 | Dcm 的安全访问算法 (Seed-Key) 由 OEM 在集成时替换为密码学安全实现 | OEM / 系统集成商 |
| AU-05 | 诊断通信链路 (CAN/DoIP) 的物理层安全由网络层保护 | 网络架构师 |
| AU-06 | 编译器符合 MISRA C:2012 且开启所有安全关键编译选项 | 工具链供应商 |

### 6.2 集成安全要求

- **内存保护**: NvM 的 CRC 查找表 (`NvM_Crc8Table[256]`, `NvM_Crc16Table[256]`, `NvM_Crc32Table[256]`) 必须放置在受保护的 Flash 区域，防止运行时篡改
- **栈保护**: Dcm 的 `RxBuffer[DCM_RX_BUFFER_SIZE]` 和 `TxBuffer[DCM_TX_BUFFER_SIZE]` 必须有栈溢出保护
- **初始化顺序**: EcuM 必须按以下顺序初始化: NvM_Init → Dem_Init → Dcm_Init → BswM_Init
- **关机顺序**: EcuM 必须确保 NvM_WriteAll 在断电前完成

---

## 7. ISO 26262 条款追溯

| 本文档章节 | ISO 26262 条款 | 要求 |
|:-----------|:--------------|:-----|
| 第 2 章 Item 定义 | Part 3, Clause 7 | Item definition at the vehicle level |
| 第 3 章 HARA | Part 3, Clause 8 | Hazard analysis and risk assessment |
| 第 4 章 安全需求分解 | Part 3, Clause 9 | Safety goals → Technical safety requirements |
| 第 5 章 故障检测与反应 | Part 6, Clause 7 | Software safety requirements |
| 第 5.3 节 安全状态 | Part 6, Clause 7.4 | Safe state definition |
| 第 6 章 使用假设 | Part 3, Clause 10 | Assumptions of use |

---

## 8. 审批记录

| 角色 | 姓名 | 日期 | 签名 |
|:-----|:-----|:-----|:-----|
| 功能安全经理 | | | |
| 系统架构师 | | | |
| 软件架构师 | | | |
