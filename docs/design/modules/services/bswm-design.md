# BswM (BSW Mode Manager) Design Document

> **Module ID**: 0x12  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_BSWModeManager  
> **Source Path**: `src/bsw/services/bswm/`  
> **Reference Document**: `docs/modules/BSWM.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

BswM 是 BSW 模式管理器，负责根据来自 EcuM、ComM、DCM、NM、SchM 等模块的模式请求（Mode Request）评估规则（Rule），并执行对应的动作列表（Action List）。它是 BSW 模块间模式切换的中央仲裁器。

主要功能：
- 接收并缓存来自多个请求源的模式请求
- 维护模式请求端口（Mode Request Port）
- 评估规则并决定当前模式
- 在 `BswM_MainFunction()` 中应用新的模式并执行动作列表
- 向其他模块提供当前模式与请求模式查询

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS BSW Mode Manager | 4.4.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层/同层 | EcuM | 接收 ECU 状态与唤醒事件通知 |
| 同层 | ComM, DCM, NM, SchM | 模式请求源 |
| 同层 | 各 BSW 模块 | 动作列表中调用 |
| 公共 | Det | 开发错误检测（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   EcuM   ComM   DCM   NM   SchM    │
├─────────────────────────────────────┤
│            BswM (Services)          │
├─────────────────────────────────────┤
│   被管理模块（Com、PduR、CanIf 等）  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **模式请求端口（Mode Request Port）**：每个请求源对应一个端口，保存请求的模式与活跃状态。
- **规则表（Rule Table）**：定义模式请求端口索引、目标模式、优先级、使能状态。
- **动作列表（Action List）**：每个规则对应一组动作函数指针，模式切换时顺序执行。
- **主函数仲裁器**：在 `BswM_MainFunction()` 中检查模式请求掩码，更新当前模式并执行动作列表。

### 3.3 文件结构

```
src/bsw/services/bswm/
├── include/
│   ├── BswM.h
│   └── BswM_Cfg.h
└── src/
    ├── BswM.c
    └── BswM_Lcfg.c
```

---

## 4. 状态机

BswM 内部状态简单，主要分为：

```
UNINIT -- BswM_Init() --> INIT
INIT -- BswM_DeInit() --> UNINIT
```

模式值包括：

| 模式值 | 名称 | 说明 |
|--------|------|------|
| 0 | OFF | 关闭 |
| 1 | START | 启动 |
| 2 | RUN | 运行 |
| 3 | POST_RUN | 后运行 |
| 4 | SLEEP | 睡眠 |
| 5 | SHUTDOWN | 关闭 |
| 6 | WAKEUP | 唤醒 |
| 7 | STARTUP | 启动中 |

---

## 5. 核心数据结构

### 5.1 模式与回调类型

```c
typedef uint8 BswM_ModeType;
typedef void (*BswM_ActionCallback)(BswM_ModeType Mode);
```

### 5.2 模式请求端口

```c
typedef struct {
    uint8  CompositionId;       /* 软件组件/请求源标识 */
    uint8  RequestSourceId;     /* 请求源 ID */
    BswM_ModeType RequestedMode; /* 请求的模式 */
    boolean IsActive;           /* 是否活跃 */
} BswM_ModeRequestPortType;
```

### 5.3 规则

```c
typedef struct {
    uint8 RuleId;               /* 规则 ID */
    uint8 ModeRequestPortIndex; /* 关联的模式请求端口索引 */
    BswM_ModeType TargetMode;   /* 目标模式 */
    uint8 Priority;             /* 优先级 */
    boolean IsEnabled;          /* 是否使能 */
} BswM_RuleType;
```

### 5.4 动作列表

```c
typedef struct {
    uint8 ActionListId;         /* 动作列表 ID */
    uint8 RuleId;               /* 关联规则 ID */
    uint8 NumActions;           /* 动作数量 */
    BswM_ActionCallback* Actions; /* 动作函数指针数组 */
} BswM_ActionListType;
```

### 5.5 配置结构

```c
typedef struct {
    uint8 NumModeRequestPorts;
    uint8 NumRules;
    uint8 NumActionLists;
    const BswM_ModeRequestPortType* ModeRequestPorts;
    const BswM_RuleType* Rules;
    const BswM_ActionListType* ActionLists;
} BswM_ConfigType;
```

### 5.6 内部状态

```c
typedef enum {
    BSWM_INTERNAL_UNINIT = 0,
    BSWM_INTERNAL_INIT
} BswM_InternalStateType;

typedef struct {
    BswM_InternalStateType internalState;
    BswM_ModeType          currentMode;
    BswM_ModeType          requestedMode;
    uint16                 modeRequestMask;
    const BswM_ConfigType* configPtr;
} BswM_InternalType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | SWS 需求 | 备注 |
|-----|------|------|----------|------|
| `BswM_Init` | `void BswM_Init(const BswM_ConfigType* ConfigPtr)` | 初始化 BswM | SWS_BswM_00001 | 保存配置指针，状态置 INIT |
| `BswM_DeInit` | `void BswM_DeInit(void)` | 反初始化 | SWS_BswM_00002 | 状态置 UNINIT |
| `BswM_RequestMode` | `Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode)` | 接收模式请求 | SWS_BswM_00010 | 设置 requestedMode 并置位 mask |
| `BswM_GetCurrentMode` | `BswM_ModeType BswM_GetCurrentMode(void)` | 获取当前模式 | SWS_BswM_00011 | |
| `BswM_GetRequestedMode` | `BswM_ModeType BswM_GetRequestedMode(void)` | 获取最新请求模式 | SWS_BswM_00012 | |
| `BswM_MainFunction` | `void BswM_MainFunction(void)` | 周期仲裁与执行 | SWS_BswM_00020 | 未初始化直接返回 |
| `BswM_GetVersionInfo` | `void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | SWS_BswM_00030 | |

### 6.2 回调函数

当前实现主要依赖配置中的动作列表函数指针；EcuM 相关的回调在 EcuM 模块中通过 `extern` 声明调用：

| 回调 | 说明 |
|------|------|
| `BswM_EcuM_CurrentState` | EcuM 状态变更通知 |
| `BswM_EcuM_CurrentWakeup` | EcuM 唤醒源状态通知 |

### 6.3 服务 ID 与错误码

| SID | API | SWS 需求 | 主要错误码 |
|-----|-----|----------|------------|
| 0x00 | Init | SWS_BswM_00001 | `BSWM_E_PARAM_POINTER` |
| 0x01 | DeInit | SWS_BswM_00002 | - |
| 0x02 | MainFunction | SWS_BswM_00020 | `BSWM_E_UNINIT` |
| 0x03 | RequestMode | SWS_BswM_00010 | `BSWM_E_UNINIT` |
| 0x04 | GetCurrentMode | SWS_BswM_00011 | - |
| 0x05 | GetRequestedMode | SWS_BswM_00012 | - |
| 0xFF | GetVersionInfo | SWS_BswM_00030 | `BSWM_E_PARAM_POINTER` |

---

## 7. 处理流程

### 7.1 初始化

1. `BswM_Init()` 接收配置指针。
2. 验证非空（DET 开启时）。
3. 初始化内部状态：`currentMode = OFF`，`requestedMode = OFF`，`modeRequestMask = 0`。
4. `internalState = BSWM_INTERNAL_INIT`。

### 7.2 模式请求

1. `BswM_RequestMode()` 检查模块已初始化。
2. 保存 `requestedMode`。
3. `modeRequestMask |= (1U << Mode)`，标记有新请求待处理。
4. 返回 `E_OK`。

### 7.3 主函数仲裁

1. `BswM_MainFunction()` 检查 `internalState` 与 `configPtr`。
2. 若 `modeRequestMask != 0`，将 `currentMode` 更新为 `requestedMode`。
3. 遍历配置中的 `ActionLists`，依次调用动作回调（当前实现仅遍历占位，未真正调用）。
4. 清除 `modeRequestMask`。

---

## 8. 配置设计

### 8.1 预编译配置（`BswM_Cfg.h`）

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `BSWM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `BSWM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `BSWM_MAX_MODE_REQUEST_PORTS` | 32U | 最大模式请求端口数 |
| `BSWM_MAX_RULES` | 64U | 最大规则数 |
| `BSWM_MAX_ACTIONS` | 128U | 最大动作数 |
| `BSWM_MAX_ACTION_LISTS` | 32U | 最大动作列表数 |
| `BSWM_MODE_STARTUP` | 0U | 启动模式值 |
| `BSWM_MODE_RUN` | 1U | 运行模式值 |
| `BSWM_MODE_SHUTDOWN` | 2U | 关闭模式值 |
| `BSWM_MODE_SLEEP` | 3U | 睡眠模式值 |
| `BSWM_MODE_WAKEUP` | 4U | 唤醒模式值 |
| `BSWM_ECUM_STATE_STARTUP` | 16U | EcuM 启动状态映射 |
| `BSWM_ECUM_STATE_RUN` | 32U | EcuM 运行状态映射 |
| `BSWM_ECUM_STATE_SHUTDOWN` | 48U | EcuM 关闭状态映射 |
| `BSWM_ECUM_STATE_SLEEP` | 64U | EcuM 睡眠状态映射 |

### 8.2 链接时配置

`BswM_Lcfg.c` 中定义：

- `BswM_ModeRequestPorts[]`：模式请求端口数组
- `BswM_Rules[]`：规则数组
- `BswM_ActionLists[]`：动作列表数组
- `BswM_Config`：顶层配置结构

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | `BSWM_E_PARAM_POINTER` | `BswM_Init()` 传入空指针 |
| 0x20 | `BSWM_E_UNINIT` | 未初始化时调用 `BswM_RequestMode` / `BswM_MainFunction` |
| 0x30 | `BSWM_E_PARAM_MODE` | 请求非法模式值 |
| 0x40 | `BSWM_E_MODE_REQUEST_REJECT` | 模式请求被拒绝（当前未使用） |

### 9.2 安全机制

- 模式切换通过集中式仲裁，避免多个模块直接修改状态。
- 动作列表可配置优先级，支持按顺序执行安全关键动作。

---

## 10. 内存与性能

### 10.1 MemMap 分区

当前实现未显式使用 MemMap 分区。后续应补充：

| 分区 | 用途 |
|------|------|
| `BSWM_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化全局变量 |
| `BSWM_START_SEC_CODE` | 代码段 |
| `BSWM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 链接配置数据 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~50 B | 内部状态变量 |
| ROM | ~2 KB | 代码与配置表引用 |
| 周期 | 低 | 主函数仅遍历动作列表 |

---

## 11. 集成指南

- **EcuM 集成**：EcuM 在状态变化时调用 `BswM_EcuM_CurrentState()` 与 `BswM_EcuM_CurrentWakeup()`，BswM 需在 EcuM 之前初始化。
- **动作列表**：在 `BswM_Lcfg.c` 中填充具体动作函数，如 `ComM_AllowCom`、`CanIf_SetPduMode` 等。
- **模式映射**：注意 `BswM_ModeType` 与 EcuM/ComM 模式值的映射关系。
- **配置生成**：`BswM_Cfg.h` 由 yuleASR Configurator 自动生成，手动修改需保持一致。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `tests/unit/autosar/services/BswM_Test.c` | 初始化、模式请求、主函数仲裁、版本信息 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| EcuM 状态通知 | EcuM RUN 状态 → BswM 模式切换 → 动作列表执行 |
| 多源模式请求 | ComM/DCM 同时请求不同模式，验证仲裁行为 |
| 反初始化 | BswM_DeInit 后 API 返回错误 |

---

## 13. 实现说明 / TODO

- `BswM_MainFunction()` 当前仅遍历 `ActionLists` 并未真正调用动作函数，需补充动作执行逻辑。
- 规则评估逻辑当前简化为直接采用最近一次请求模式，未实现多端口组合规则与优先级仲裁。
- `BswM_RequestMode()` 的 `SwCompositionId` 参数当前未使用。
- 未实现基于请求端口的复杂规则条件表达式。
- EcuM 回调函数原型在 EcuM.c 中通过 `extern` 声明，需在 BswM 中提供实际定义。

---

## 14. 参考资料

1. AUTOSAR_SWS_BSWModeManager.pdf
2. `docs/modules/BSWM.md`
3. `src/bsw/services/bswm/BswM.h`
4. `src/bsw/services/bswm/BswM.c`
5. `src/bsw/services/bswm/BswM_Cfg.h`
