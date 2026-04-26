# Phase 5 AUTOSAR核心模块补充设计

**日期**: 2026-04-26  
**目标**: 补充AUTOSAR核心模块 (Adaptive运行时 + Classic模式管理 + 完整存储栈)  
**规范**: AUTOSAR Classic R22-11 + Adaptive R22-11

---

## 背景

当前项目已完成DDS中间件、安全堆栈、诊断栈、OTA等核心功能，但缺少AUTOSAR规范中的基础运行时和模式管理模块。本Phase将补充这些基础设施，使项目达到生产级AUTOSAR堆栈完整性。

---

## 设计方案

### 方案A: Adaptive平台优先
集中实现ara::exec和ara::sm，使项目成为完整的Adaptive AUTOSAR平台

**Pros**:
- 适应自动驾驶和高性能计算趋势
- 支持容器化部署
- 与DDS现代架构完美匹配

**Cons**:
- 与现有Classic RTE可能冲突
- 需要更多资源

### 方案B: Classic平台完善
优先实现EcuM和BswM，增强Classic BSW堆栈

**Pros**:
- 与现有DCM/DEM等模块无缝集成
- 成熟稳定的架构
- 适合传统汽车电子

**Cons**:
- 不支持容器化部署
- 难以扩展到云原生应用

### 方案C: 双平台并行 (推荐)
**同时补充Adaptive和Classic核心模块，通过适配层实现互操作**

**Pros**:
- 最大化覆盖范围
- 支持混合架构 (Classic ECU + Adaptive ECU)
- 保留未来扩展性

**Cons**:
- 开发工作量较大
- 需要统一的适配层

**Decision**: 采用方案C，并行开发3个方向

---

## 详细设计

### 1. ara::exec + ara::sm (Adaptive运行时)

**ara::exec** (执行管理):
- **进程生命周期管理**: 启动、停止、重启、杀死
- **资源管理**: CPU/内存/时间限制
- **沙箱机制**: 进程隔离和权限控制
- **健康监控**: 进程健康检查点 (PHM集成)

**ara::sm** (状态管理):
- **机器状态机**: MachineState定义和转换
- **功能组 (Function Group)**: 模式分组管理
- **请求协议**: smRequest/确认/拒绝机制
- **ara::com集成**: DDS节点启停与状态联动

**架构设计**:
```
MachineState (Driving, Parking, Startup, Shutdown)
    ↓
FunctionGroup (PowerMode, Communication, Diagnostics)
    ↓
Process (DDS节点, UDS服务, OTA客户端)
    ↓
Resource (CPU, Memory, File)
```

### 2. EcuM + BswM (Classic模式管理)

**EcuM** (ECU状态管理):
- **状态机**: STARTUP, RUN, SLEEP, WAKE_SLEEP, SHUTDOWN
- **唤醒源管理**: 硬件唤醒、计时器唤醒、通信唤醒
- **监睡过渡**: 监睡准备和模式选择
- **Shutdown管理**: 顺序关闭和数据保存

**BswM** (基础软件模式管理):
- **规则引擎**: 条件-动作规则配置
- **模式切换**: 模式请求和确认机制
- **行动列表**: 模式切换时的有序执行
- **与Dcm集成**: 诊断会话影响模式切换

**架构设计**:
```
EcuM State Machine
    STARTUP → RUN → [SLEEP | SHUTDOWN]
         ↑___________┗━━━━━━━━━━━┛

BswM Rule Engine
    Condition (EcuM_State, ComM_State, Dcm_Session)
        ↓
    ActionList (Start/Stop COM, Enable/Disable DTC)
```

### 3. Fee + Fls (完整存储栈)

**Fls** (Flash驱动):
- **页操作**: 读取、写入、擦除、比较
- **异步操作**: 事件驱动的非阻塞API
- **安全检查**: 写保护、边界检查
- **硬件抽象**: Aurix/S32G Flash控制器统一接口

**Fee** (Flash EEPROM Emulation):
- **逻辑块管理**: 虚拟EEPROM块映射
- **垃圾回收**: 自动数据迁移和空间回收
- **写均衡**: 延长Flash寿命
- **与NvM集成**: 为NvM提供持久化存储

**架构设计**:
```
NvM
    ↓ (Native/Redundant)
Fee (Virtual EEPROM Blocks)
    ↓ (Write Balancing)
Fls (Flash Driver)
    ↓ (Page Operations)
Hardware Flash Controller
```

---

## 与现有模块的集成

### ara::sm 与 ara::com (DDS)
```c
// 当MachineState切换到Driving时启动DDS节点
void Sm_StateChanged(MachineStateType state) {
    if (state == MACHINE_STATE_DRIVING) {
        ara::com::Runtime::GetInstance().Start();
        // 启动DDS Participants/Publishers/Subscribers
    }
}
```

### EcuM 与 Dcm
```c
// Dcm会话影响ECU状态
void Dcm_SessionChanged(SessionType newSession) {
    if (newSession == SESSION_PROGRAMMING) {
        EcuM_RequestState(ECUM_STATE_SHUTDOWN); // 禁止监睡
    }
}
```

### Fee 与 NvM
```c
// NvM通过Fee持久化存储
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType blockId, void* data) {
    return Fee_Write(blockId, data);
}
```

---

## 文件结构

```
src/autosar/adaptive/exec/       # ara::exec
  - exec_manager.h/c
  - process_manager.h/c
  - resource_manager.h/c
  - sandbox.h/c

src/autosar/adaptive/sm/         # ara::sm
  - state_machine.h/c
  - function_group.h/c
  - sm_interface.h/c

src/autosar/classic/ecum/        # EcuM
  - ecum.h/c
  - ecum_types.h
  - ecum_callbacks.h

src/autosar/classic/bswm/        # BswM
  - bswm.h/c
  - bswm_rules.h/c
  - bswm_actions.h

src/mcal/fls/                    # Flash Driver
  - fls.h/c
  - fls_types.h
  - fls_cfg.h

src/mcal/fee/                    # Fee
  - fee.h/c
  - fee_types.h
  - fee_cfg.h
  - fee_gc.h/c    # Garbage Collection
```

---

## 开发计划

### Agent 1: Adaptive运行时 (ara::exec + ara::sm)
**时间**: 6天  
**输出**: 
- ara::exec: 进程管理 + 资源限制 + 健康监控
- ara::sm: 状态机 + FunctionGroup + 请求协议
- 与DDS集成示例

### Agent 2: Classic模式管理 (EcuM + BswM)
**时间**: 5天  
**输出**:
- EcuM: 完整状态机 + 唤醒源管理
- BswM: 规则引擎 + 模式切换
- 与DCM/DEM集成

### Agent 3: 存储栈 (Fee + Fls)
**时间**: 5天  
**输出**:
- Fls: Flash驱动 + 异步API
- Fee: EEPROM模拟 + 垃圾回收 + 写均衡
- 与NvM完全集成

---

## 风险与应对

| 风险 | 影响 | 应对措施 |
|------|------|----------|
| ara::exec与FreeRTOS冲突 | 中 | 在FreeRTOS上封装Process为Task |
| Fee垃圾回收复杂度高 | 中 | 分阶段实现，先实现基础功能 |
| EcuM状态转换时序严格 | 高 | 详细设计状态转换图，添加断言 |
| 多Agent并行集成问题 | 中 | 明确界面接口，先定义头文件 |

---

*设计版本: v1.0*  
*最后更新: 2026-04-26*
