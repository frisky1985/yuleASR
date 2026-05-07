# LinSlave v2.1 更新日志

## 版本信息
- **版本**: 2.1.0
- **日期**: 2024
- **更新内容**: 支持多个Unconditional Frame配置

## 主要改进

### 1. 配置表结构优化

#### v2.0 限制
- 最多16个PID条目
- 所有类型的报文混合在一个数组中
- 不支持Event Triggered Frame和Sporadic Frame的特定配置

#### v2.1 改进
- 最多20个Unconditional Frame
- 最多4个Event Triggered Frame
- 最多4个Sporadic Frame
- 独立的诊断帧配置
- 每种类型有独立的配置结构

### 2. 新增配置结构

```c
/* Unconditional Frame配置 */
typedef struct {
    uint8 Pid;
    uint8 Length;
    LinSlave_DirectionType Direction;
    LinSlave_ChecksumType ChecksumType;
    LinSlave_UnconditionalRxCallbackType RxCallback;
    LinSlave_UnconditionalTxCallbackType TxCallback;
    LinSlave_FrameErrorCallbackFuncType ErrorCallback;
    void* UserData;
    LinSlave_FrameStatusType Status;    /* 运行时状态 */
    uint8 LastData[8];                  /* 数据缓存 */
    uint8 UpdateFlag;                   /* 更新标志 */
} LinSlave_UnconditionalFrameConfigType;

/* Event Triggered Frame配置 */
typedef struct {
    uint8 Pid;
    uint8 AssociatedFrameCount;
    const uint8* AssociatedPids;
} LinSlave_EventFrameConfigType;

/* Sporadic Frame配置 */
typedef struct {
    uint8 Pid;
    uint8 AssociatedFrameCount;
    const uint8* AssociatedPids;
} LinSlave_SporadicFrameConfigType;
```

### 3. 新增API函数

#### 配置表管理
```c
LinSlave_StatusType LinSlave_CfgTableV2_Init(const LinSlave_ConfigTableV2Type* ConfigTable);
const LinSlave_UnconditionalFrameConfigType* LinSlave_CfgTableV2_FindUnconditionalByPid(uint8 Pid);
const LinSlave_EventFrameConfigType* LinSlave_CfgTableV2_FindEventFrame(uint8 Pid);
const LinSlave_SporadicFrameConfigType* LinSlave_CfgTableV2_FindSporadicFrame(uint8 Pid);
```

#### 帧状态管理
```c
LinSlave_FrameStatusType LinSlave_CfgTableV2_GetFrameStatus(uint8 FrameIndex);
void LinSlave_CfgTableV2_SetFrameStatus(uint8 FrameIndex, LinSlave_FrameStatusType Status);
uint8 LinSlave_CfgTableV2_IsFrameUpdated(uint8 FrameIndex);
void LinSlave_CfgTableV2_ClearUpdateFlag(uint8 FrameIndex);
```

#### 数据缓存访问
```c
const uint8* LinSlave_CfgTableV2_GetFrameData(uint8 FrameIndex);
void LinSlave_CfgTableV2_SetFrameData(uint8 FrameIndex, const uint8* DataPtr, uint8 Length);
```

#### 工具函数
```c
uint8 LinSlave_CfgTableV2_GetPidByIndex(uint8 Index);
uint8 LinSlave_CfgTableV2_GetIndexByPid(uint8 Pid);
uint8 LinSlave_CfgTableV2_GetAllUnconditionalPids(uint8* PidList, uint8 MaxCount);
boolean LinSlave_CfgTableV2_IsUnconditionalFrame(uint8 Pid);
boolean LinSlave_CfgTableV2_IsEventFrame(uint8 Pid);
boolean LinSlave_CfgTableV2_IsSporadicFrame(uint8 Pid);
boolean LinSlave_CfgTableV2_IsDiagnosticFrame(uint8 Pid);
```

### 4. 初始化API更新

```c
/* v2.1 初始化函数 */
LinSlave_StatusType LinSlave_InitWithConfigTableV2(const LinSlave_ConfigTableV2Type* ConfigTable);
```

## 配置示例

```c
/* 配置8个Unconditional Frame */
static const LinSlave_UnconditionalFrameConfigType* UnconditionalFrames[] = {
    &Frame_0, &Frame_1, &Frame_2, &Frame_3,
    &Frame_4, &Frame_5, &Frame_6, &Frame_7
};

const LinSlave_ConfigTableV2Type LinSlave_ConfigTableV2 = {
    2, 1, 0,                        /* 版本 */
    0x05,                           /* 节点ID */
    19200,                          /* 波特率 */
    8,                              /* Unconditional Frame数量 */
    (const LinSlave_UnconditionalFrameConfigType*)UnconditionalFrames,
    1,                              /* Event Frame数量 */
    EventFrames,
    1,                              /* Sporadic Frame数量 */
    SporadicFrames,
    &DiagnosticConfig,              /* 诊断配置 */
    TRUE,                           /* 使用诊断 */
    GlobalErrorCallback             /* 全局错误回调 */
};

/* 初始化 */
LinSlave_InitWithConfigTableV2(&LinSlave_ConfigTableV2);
```

## 文件变更

### 修改的文件
- `include/LinSlave_CfgTable.h` - 增加v2.1配置结构
- `src/LinSlave_CfgTable.c` - 实现v2.1配置表管理
- `include/LinSlave.h` - 更新初始化API
- `src/LinSlave.c` - 更新状态机支持v2.1

### 新增文件
- `example/LinSlave_MultiFrameCfg.c` - 多帧配置示例

## 向后兼容性

- v2.0 API保持可用 (`LinSlave_CfgTable_Init`)
- 建议新项目使用v2.1 API (`LinSlave_InitWithConfigTableV2`)
- v2.1配置表结构与v2.0不兼容，需要迁移

## 性能影响

- 代码大小: 增加约1KB (v2.1新功能)
- RAM使用: 每个Unconditional Frame增加10字节(状态+缓存)
- 查找性能: O(n)线性查找，最多20个帧可接受

## 使用建议

1. **新项目**: 直接使用v2.1配置表结构
2. **迁移项目**: 按照示例重构配置表
3. **帧管理**: 利用更新标志检查数据变化
4. **错误处理**: 使用全局错误回调统一处理

## 已知问题

1. Event Triggered Frame完整支持需要进一步实现
2. Sporadic Frame调度算法需要应用层实现
