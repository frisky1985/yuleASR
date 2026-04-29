# RAM安全检查模块 - 设计文档

## 架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                    RamSafety 模块架构              │
├─────────────────────────────────────────────────────────────────┤
│                                                            │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │              RamSafety.h (公共API)              │  │
│  │  - RamSafety_Init()                            │  │
│  │  - RamSafety_RunStartupTest()                  │  │
│  │  - RamSafety_MainFunction()                    │  │
│  │  - RamSafety_VerifyRegion()                    │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                           │                              │
│  ┌───────────────────────┐  │  ┌───────────────────────┐  │
│  │  Startup Test    │  │  │  Runtime Test    │  │
│  │  (启动检查)       │  │  │  (运行检查)      │  │
│  │                  │  │  │                 │  │
│  │  - March C-      │  │  │  - 抽样检查     │  │
│  │  - Walk Pattern   │  │  │  - CRC验证    │  │
│  │  - Addr/Data Line│  │  │  - ECC检查     │  │
│  └───────────────────────┘  │  └───────────────────────┘  │
│                           │                              │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │              RamSafety.c (核心实现)             │  │
│  │  - 检查算法实现                                │  │
│  │  - 错误处理逻辑                                │  │
│  │  - 统计收集                                    │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                           │                              │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │         Platform_RamSafety.c (平台层)         │  │
│  │  - 硬件抽象层 (寄存器操作)                    │  │
│  │  - ECC状态读取                                  │  │
│  │  - 硬件BIST触发                                │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                            │
└─────────────────────────────────────────────────────────────────┘
```

## 检查算法详细设计

### 1. March C- 算法 (启动时)

March C- 是嵌入式系统最常用的RAM测试算法。

```
操作序列:
1. ↑ (w0)        - 从低到高地址写0
2. ↑ (r0,w1,r1)  - 从低到高: 读0,写1,读1
3. ↑ (r1,w0,r0)  - 从低到高: 读1,写0,读0
4. ↓ (r0,w1,r1)  - 从高到低: 读0,写1,读1
5. ↓ (r1,w0,r0)  - 从高到低: 读1,写0,读0
6. ↓ (r0)        - 从高到低读0

覆盖故障:
- SA0, SA1 (固定故障)
- TF (转换故障)
- AF (地址故障)
- CFin, CFid (耦合故障)
```

### 2. Walking Pattern (行走模式)

用于检测数据线和地址线故障。

```c
// 数据线检查
patterns[] = {0x00000000, 0xFFFFFFFF, 0x55555555, 0xAAAAAAAA,
              0x01010101, 0x80808080, 0xFEFEFEFE, 0x7F7F7F7F};

// 地址线检查 (每个地址位独立检查)
for (i = 0; i < addr_bits; i++) {
    write_pattern_to_address_with_bit_i_set();
    verify_no_aliasing();
}
```

### 3. 运行时抽样检查

在运行时定期检查关键内存区域。

```
策略:
1. 将RAM划分为多个区域
2. 每个周期检查一个区域
3. 使用CRC或校验和验证
4. 发现错误立即报告
```

## 状态机

```
                    ┌─────────┐
            ───────────────→│ UNINIT │
            │              └─────────┘
            │                   │ Init()
            │                   ↓
            │              ┌─────────┐
            │      ─────────────→│ INIT   │
            │      │         └─────────┘
            │      │              │
            │      │              ↓
            │      │         ┌─────────┐
            │      │         │ STARTUP│← RunStartupTest()
            │      │         └─────────┘
            │      │              │
            │      │              ↓ Test OK
            │      │         ┌─────────┐
            │      │    Test│ ACTIVE │
            │      │   Fail └─────────┘
            │      │              │
            │      │    ┌──────────┐
            │      └───────→│  ERROR   │
            └──────────────────────┘
```

## API设计

### 核心API

```c
/* 初始化 */
Std_ReturnType RamSafety_Init(const RamSafety_ConfigType* config);

/* 启动时完整检查 */
Std_ReturnType RamSafety_RunStartupTest(void);

/* 运行时主函数 (定期调用) */
void RamSafety_MainFunction(void);

/* 验证指定区域 */
Std_ReturnType RamSafety_VerifyRegion(uint8 regionId);

/* 手动触发检查 */
Std_ReturnType RamSafety_TriggerTest(RamSafety_TestType testType);

/* 获取统计信息 */
Std_ReturnType RamSafety_GetStatistics(RamSafety_StatisticsType* stats);
```

### 配置结构体

```c
typedef struct {
    uint32 startAddress;
    uint32 size;
    uint8 priority;           /* 检查优先级 */
    boolean startupTest;      /* 启动时检查 */
    boolean runtimeTest;      /* 运行时检查 */
    uint32 crcSeed;          /* CRC种子 */
} RamSafety_RegionType;

typedef struct {
    const RamSafety_RegionType* regions;
    uint8 numRegions;
    uint16 runtimePeriodMs;   /* 运行时检查周期 */
    boolean enableEccCheck;   /* 硬件ECC使能 */
} RamSafety_ConfigType;
```

## 与其他模块交互

### 与Dem模块

```c
/* RAM检查失败事件 */
Dem_ReportErrorStatus(
    RAMSAFETY_E_STARTUP_TEST_FAILED,  /* 启动检查失败 */
    DEM_EVENT_STATUS_FAILED
);

Dem_ReportErrorStatus(
    RAMSAFETY_E_RUNTIME_TEST_FAILED,  /* 运行检查失败 */
    DEM_EVENT_STATUS_FAILED
);

Dem_ReportErrorStatus(
    RAMSAFETY_E_ECC_ERROR,            /* ECC错误 */
    DEM_EVENT_STATUS_FAILED
);
```

### 与Lockstep模块

```c
/* 锁步失效时检查RAM完整性 */
void Lockstep_EventCallback(Lockstep_EventType event, ...) {
    if (event == LOCKSTEP_EVENT_MISMATCH) {
        /* 验证RAM是否受影响 */
        RamSafety_TriggerTest(RAMSAFETY_TEST_FULL);
    }
}
```

## 安全考虑

### ASIL-D要求

1. **多重检查**
   - 启动时: March C- + Walking Pattern
   - 运行时: 抽样 + CRC
   - 硬件: ECC

2. **错误检测**
   - 所有检查算法返回明确状态
   - 超时检测
   - 非预期数据模式检测

3. **安全响应**
   - 启动失败: 系统停止
   - 运行失败: 进入安全状态
   - 记录诊断码

## 性能估算

| 检查类型 | 时间 | 内存占用 |
|---------|------|---------|
| March C- (1MB) | ~50ms | 4KB临时缓冲 |
| Walking Pattern | ~10ms | 256B |
| 运行时抽样 (64KB) | ~2ms | 1KB |
| CRC验证 | ~5ms | 512B |

## 测试策略

1. **单元测试**
   - 模拟RAM故障
   - 验证检查算法
   - 测试状态机

2. **集成测试**
   - 与Dem联动
   - 与Lockstep联动
   - S32K312实物测试

3. **性能测试**
   - 启动时间测量
   - 运行时CPU占用
   - 内存消耗
