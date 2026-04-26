# Phase 5 实施计划

**日期**: 2026-04-26  
**目标**: 完成AUTOSAR核心模块 (ara::exec, ara::sm, EcuM, BswM, Fee, Fls)  
**时间**: 6天

---

## 任务分配

### Agent 1: Adaptive运行时 (ara::exec + ara::sm)

**负责人**: Agent-ARA  
**时间**: 6天  
**输出路径**: `src/autosar/adaptive/`

#### Day 1-2: ara::exec 执行管理
1. **进程管理器** (`exec/process_manager.c`)
   - 进程定义结构 (ProcessId, Executable, Arguments, Environment)
   - 进程生命周期控制 (Start/Stop/Restart/Kill)
   - 进程状态追踪 (Idle->Starting->Running->Terminating->Terminated)
   - 进程健康监控 (Health Check)

2. **资源管理器** (`exec/resource_manager.c`)
   - CPU限制 (CGroup封装)
   - 内存限制 (Memory Quota)
   - 文件描述符限制
   - 资源使用监控

3. **沙箱机制** (`exec/sandbox.c`)
   - 文件系统隔离 (Chroot/Namespace)
   - 网络隔离
   - 系统调用过滤 (Seccomp)

#### Day 3-4: ara::sm 状态管理
1. **状态机实现** (`sm/state_machine.c`)
   - MachineState定义 (Startup, Driving, Parking, Shutdown)
   - 状态转换规则
   - 状态转换回调注册

2. **FunctionGroup** (`sm/function_group.c`)
   - FG定义 (PowerMode, Communication, Diagnostics)
   - FG状态 (Off->Starting->On->Stopping)
   - FG成员管理 (进程/服务)

3. **请求协议** (`sm/sm_interface.c`)
   - smRequest()实现
   - 确认/拒绝机制
   - 超时处理

#### Day 5-6: 集成与测试
1. **与DDS集成**
   - 状态变化触发DDS节点启停
   - ara::com与ara::sm联动

2. **测试**
   - 单元测试: 进程生命周期、状态转换
   - 集成测试: 完整应用启动/停止

---

### Agent 2: Classic模式管理 (EcuM + BswM)

**负责人**: Agent-ModeM  
**时间**: 5天  
**输出路径**: `src/autosar/classic/`

#### Day 1-2: EcuM实现
1. **状态机** (`ecum/ecum.c`)
   - 状态定义: STARTUP, RUN, SLEEP, WAKE_SLEEP, SHUTDOWN
   - 状态转换逻辑
   - 回调机制 (EcuM_OnStartup, EcuM_OnShutdown)

2. **唤醒源管理** (`ecum/ecum_wakeup.c`)
   - 唤醒源注册
   - 唤醒验证
   - 唤醒时间戳

3. **监睡管理** (`ecum/ecum_sleep.c`)
   - 监睡准备检查
   - 监睡模式选择
   - 唤醒复位

#### Day 3-4: BswM实现
1. **规则引擎** (`bswm/bswm_rules.c`)
   - 规则表配置
   - 条件计算 (逻辑/按位)
   - 规则执行

2. **行动列表** (`bswm/bswm_actions.c`)
   - 标准行动 (ComM, Dcm, EcuM调用)
   - 列表执行引擎
   - 执行回调

3. **模式切换** (`bswm/bswm_mode.c`)
   - 模式请求接口
   - 模式确认机制
   - 模式通知

#### Day 5: 集成与测试
1. **与现有模块集成**
   - EcuM与Dcm (会话影响状态)
   - BswM与ComM (通信控制)

2. **测试**
   - 状态转换测试
   - 唤醒流程测试
   - 规则引擎测试

---

### Agent 3: 存储栈 (Fee + Fls)

**负责人**: Agent-Memory  
**时间**: 5天  
**输出路径**: `src/mcal/`

#### Day 1-2: Fls Flash驱动
1. **驱动实现** (`mcal/fls/fls.c`)
   - 页操作 (Read/Write/Erase)
   - 异步API (返回Async状态)
   - 安全检查 (Write Protection)
   - 硬件抽象层

2. **硬件适配** (`mcal/fls/fls_hw_*.c`)
   - Aurix TC3xx Flash驱动
   - S32G3 Flash驱动
   - 模拟器 (测试用)

#### Day 3-4: Fee EEPROM模拟
1. **逻辑块管理** (`mcal/fee/fee.c`)
   - 虚拟块定义 (BlockId, Size, Data)
   - 块映射表管理
   - 写操作队列

2. **垃圾回收** (`mcal/fee/fee_gc.c`)
   - GC触发条件
   - 数据迁移算法
   - 空间回收

3. **写均衡** (`mcal/fee/fee_wl.c`)
   - 操作计数
   - 页均衡策略
   - 寿命监测

#### Day 5: NvM集成与测试
1. **NvM适配** (`nvm/nvm_fee_adapter.c`)
   - NvM到Fee的映射
   - Native/Redundant Block支持

2. **测试**
   - Fls硬件操作测试
   - Fee持久化测试
   - 完整写读流程

---

## 界面接口定义

### ara::exec 接口
```cpp
// ara/exec/exec_manager.h
namespace ara {
namespace exec {

class ExecManager {
public:
    static ExecManager& GetInstance();
    
    // 进程管理
    Result<ProcessId> StartProcess(const ExecutionConfig& config);
    Result<void> StopProcess(ProcessId pid, TerminationType type);
    ProcessState GetProcessState(ProcessId pid);
    
    // 资源管理
    Result<void> SetResourceLimit(ProcessId pid, const ResourceLimit& limit);
    ResourceUsage GetResourceUsage(ProcessId pid);
    
    // 健康监控
    void RegisterHealthCheck(ProcessId pid, HealthCheckCallback callback);
};

} // namespace exec
} // namespace ara
```

### ara::sm 接口
```cpp
// ara/sm/state_machine.h
namespace ara {
namespace sm {

class StateMachine {
public:
    // 状态请求
    Result<void> RequestStateTransition(MachineState state, 
                                       FunctionGroupList fgList);
    MachineState GetCurrentState() const;
    
    // 回调注册
    void RegisterStateChangeCallback(StateChangeCallback callback);
    
    // FunctionGroup
    Result<void> SetFunctionGroupState(FunctionGroupName fg, 
                                      FunctionGroupState state);
};

} // namespace sm
} // namespace ara
```

### EcuM 接口
```c
// autosar/classic/ecum/ecum.h
Std_ReturnType EcuM_Init(void);
Std_ReturnType EcuM_Startup(void);
Std_ReturnType EcuM_Shutdown(void);
Std_ReturnType EcuM_GoToSleep(EcuM_SleepModeType sleepMode);
Std_ReturnType EcuM_Wakeup(EcuM_WakeupSourceType source);

void EcuM_SetState(EcuM_StateType state);
EcuM_StateType EcuM_GetState(void);

void EcuM_RequestState(EcuM_StateType state);
```

### BswM 接口
```c
// autosar/classic/bswm/bswm.h
void BswM_Init(const BswM_ConfigType* config);
void BswM_Deinit(void);

void BswM_RequestMode(BswM_UserType user, BswM_ModeType mode);
void BswM_RuleNotification(BswM_RuleIdType ruleId, 
                          BswM_RuleStateType state);

void BswM_MainFunction(void);
```

### Fee 接口
```c
// mcal/fee/fee.h
Std_ReturnType Fee_Init(const Fee_ConfigType* config);
Std_ReturnType Fee_Read(uint16 blockNumber, uint16 blockOffset,
                       uint8* dataBufferPtr, uint16 length);
Std_ReturnType Fee_Write(uint16 blockNumber, const uint8* dataBufferPtr);
Std_ReturnType Fee_EraseImmediateBlock(uint16 blockNumber);

void Fee_MainFunction(void);
void Fee_JobEndNotification(void);
void Fee_JobErrorNotification(void);
```

---

## 测试覆盖要求

| 模块 | 目标覆盖率 | 关键测试场景 |
|------|----------|-------------|
| ara::exec | 85% | 进程启停、资源限制、健康监控失败 |
| ara::sm | 85% | 状态转换、FunctionGroup切换、请求拒绝 |
| EcuM | 90% | 所有状态转换、唤醒源、监睡流程 |
| BswM | 85% | 规则计算、行动执行、模式切换 |
| Fee | 85% | 块写入、垃圾回收、写均衡、异步操作 |
| Fls | 90% | 页操作、异步API、错误处理 |

---

## 依赖关系

```
Agent-ARA (ara::exec/sm)
    ↓ 依赖 ara::com (DDS) 集成示例
    ↓ 依赖 FreeRTOS/POSIX 进程模型

Agent-ModeM (EcuM/BswM)
    ↓ 依赖 Dcm (会话管理)
    ↓ 依赖 ComM (通信管理 - 如果存在)
    ↓ 依赖 BswM Actions (需要被管理的模块)

Agent-Memory (Fee/Fls)
    ↓ 依赖 NvM (需要适配层)
    ↓ 依赖 平台Flash驱动抽象
```

---

## 风险管理

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| ara::exec与FreeRTOS线程模型不匹配 | 中 | 高 | 在FreeRTOS上封装进程为独立Task |
| Fee垃圾回收在写操作时触发 | 低 | 高 | 实现写缓冲 + 异步GC |
| EcuM状态转换时序错误 | 中 | 高 | 详细状态图 + 断言检查 |
| 并行开发接口冲突 | 中 | 中 | 先定义接口头文件，明确边界 |

---

*计划版本: v1.0*  
*最后更新: 2026-04-26*
