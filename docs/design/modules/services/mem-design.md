# Mem Design Document

> **Module ID**: 0x58 (88)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Memory Abstraction Interface  
> **Source Path**: `src/bsw/services/mem/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

Mem (Memory Abstraction) 提供统一的内存读写接口，抽象底层不同的存储设备（NvM、Fee、EA、Fls 等）。上层模块通过 Mem 的标准 API 访问非易失性存储，无需关心具体存储后端。Mem 负责路由请求到正确的存储设备，管理作业队列和异步回调。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Memory Abstraction | 4.4.0 | Mem 接口规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Dcm, Dem, NvM | 诊断/事件/数据持久化 |
| 下层 | MemIf | 内存接口抽象 |
| 下层 | Fee, EA, Fls | 具体存储后端 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│    Dcm / Dem / NvM                  │
├─────────────────────────────────────┤
│         Mem (Services)              │
├─────────────────────────────────────┤
│    MemIf → Fee / EA / Fls           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Job Queue**: 异步读写作业队列
- **Device Router**: 根据 Block ID 路由到正确存储设备
- **Callback Manager**: 管理作业完成回调

### 3.3 文件结构

```
src/bsw/services/mem/
├── include/
│   ├── Mem.h          # 公共 API
│   ├── Mem_Cfg.h      # 设备配置
│   └── SchM_Mem.h     # 调度器接口
└── src/
    └── Mem.c           # 核心实现
```

---

## 4. 状态机

```
          Mem_Init()
  UNINIT ──────────────► IDLE
                           │
              Mem_Read/Write()
                           │
                           ▼
                        BUSY
                           │
              Job Complete Callback
                           │
                           ▼
                         IDLE
```

---

## 5. 数据结构

```c
typedef enum {
    MEM_JOB_READ = 0,
    MEM_JOB_WRITE,
    MEM_JOB_ERASE
} Mem_JobType;

typedef struct {
    uint16  BlockNumber;
    uint8*  DataPtr;
    uint16  Length;
    Mem_JobType JobType;
    void (*Callback)(uint16, Std_ReturnType);
} Mem_JobType_Entry;
```

---

## 6. API 规范

| API | 说明 |
|-----|------|
| `void Mem_Init(const Mem_ConfigType* Config)` | 初始化 |
| `void Mem_DeInit(void)` | 反初始化 |
| `Std_ReturnType Mem_Read(uint16 BlockNumber, uint8* DataPtr, uint16 Length)` | 异步读取 |
| `Std_ReturnType Mem_Write(uint16 BlockNumber, const uint8* DataPtr, uint16 Length)` | 异步写入 |
| `void Mem_MainFunction(void)` | 周期主函数 |
| `void Mem_GetVersionInfo(Std_VersionInfoType* VersionInfo)` | 版本信息 |

---

## 7. 处理流程

### 7.1 异步写入流程

1. 上层调用 `Mem_Write(BlockNumber, Data, Length)`
2. Mem 创建 Job 条目加入队列
3. MainFunction 取出 Job → 路由到对应设备（Fee/EA）
4. 调用 MemIf_Write → 等待硬件完成
5. 完成后调用上层 Callback

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `MEM_JOB_QUEUE_SIZE` | 8U | 作业队列深度 |
| `MEM_MAIN_FUNCTION_PERIOD` | 5U | 主函数周期 (ms) |
| `MEM_NUM_DEVICES` | 2U | 存储设备数量 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `MEM_E_UNINIT` | 初始化前调用 |
| `MEM_E_QUEUE_FULL` | 作业队列已满 |
| `MEM_E_INV_BLOCK` | 无效 Block Number |

---

## 10. 内存与性能

- **RAM**: 作业队列 = QueueSize × ~20B ≈ 160 字节
- **ROM**: ~2 KB 代码
- **性能**: Job 入队 O(1)，MainFunction 处理 ~5 µs/job

---

## 11. 集成指南

- Dcm 通过 Mem 读写诊断数据
- Dem 通过 Mem 存储冻结帧数据
- 存储设备映射在 Mem_Cfg.h 中配置

---

## 12. 测试策略

- 读写往返正确性测试
- 队列满时拒绝测试
- 异步回调时序测试
- 设备路由正确性测试

---

## 13. 实现说明

- 作业队列使用环形缓冲实现
- 支持编译时裁剪不需要的存储设备
- 通过 SchM_Mem.h 与调度器集成

---

## 14. 参考文献

- AUTOSAR_SWS_MemoryAbstractionInterface.pdf (R4.4.0)
- yuleASR Mem 源码: `src/bsw/services/mem/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Mem_00001 | `Mem` | 测试 test_Mem_Init_DoubleInit_ShouldSucceed 覆盖: Mem_Init_DoubleInit_ShouldSucceed 场景 |
| SWS_Mem_00002 | `Mem_DeInit` | 测试 test_Mem_DeInit_ValidCall_ShouldSucceed 覆盖: Mem_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_Mem_00003 | `Mem_GetVersionInfo` | 测试 test_Mem_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: Mem_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_Mem_00004 | `Mem_MainFunction` | 测试 test_Mem_MainFunction_ValidCall_ShouldSucceed 覆盖: Mem_MainFunction_ValidCall_ShouldSucceed 场景 |
| SWS_Mem_00005 | `Mem_Read` | 测试 test_Mem_Read_ValidCall_ShouldSucceed 覆盖: Mem_Read_ValidCall_ShouldSucceed 场景 |
| SWS_Mem_00006 | `Mem_Write` | 测试 test_Mem_Write_ValidCall_ShouldSucceed 覆盖: Mem_Write_ValidCall_ShouldSucceed 场景 |
| SWS_Mem_00007 | `Mem_Erase` | 测试 test_Mem_Erase_ValidCall_ShouldSucceed 覆盖: Mem_Erase_ValidCall_ShouldSucceed 场景 |
| SWS_Mem_00008 | `Mem_GetStatus` | 测试 test_Mem_GetStatus_ValidCall_ShouldReturnStatus 覆盖: Mem_GetStatus_ValidCall_ShouldReturnStatus 场景 |
