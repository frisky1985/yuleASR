# Harness Architecture Rules - YuleTech BSW

> **架构约束规则** - 所有代码必须遵守

## 分层架构规则

### 1. 分层定义

```
┌─────────────────────────────────────┐
│  Layer 5: Application (ASW)         │  ← 客户实现
├─────────────────────────────────────┤
│  Layer 4: RTE (Runtime Environment) │  ← 运行时接口
├─────────────────────────────────────┤
│  Layer 3: Service Layer             │  ← 服务层
├─────────────────────────────────────┤
│  Layer 2: ECU Abstraction           │  ← 硬件抽象
├─────────────────────────────────────┤
│  Layer 1: Microcontroller Drivers   │  ← MCAL 驱动
├─────────────────────────────────────┤
│  Layer 0: Hardware                  │  ← 硬件
└─────────────────────────────────────┘
```

### 2. 依赖规则

| 规则 | 描述 | 违规示例 |
|:-----|:-----|:---------|
| **R1** | 上层可依赖下层 | ✅ Service → MCAL |
| **R2** | 禁止下层依赖上层 | ❌ MCAL → Service |
| **R3** | 禁止同层跨模块依赖 | ❌ Can → Port (直接) |
| **R4** | 依赖必须通过标准接口 | ✅ 通过 BSW API |
| **R5** | 禁止循环依赖 | ❌ A → B → C → A |

### 3. 接口规则

```c
/* 正确: 通过标准接口 */
#include "Mcu.h"  /* 标准头文件 */
Mcu_Init(&McuConfig);  /* 标准 API */

/* 错误: 直接访问硬件 */
#include "S32K344.h"  /* 直接包含硬件头 */
SIU->GPDO[0] = 1;  /* 直接寄存器访问 */
```

## 代码质量规则

### 1. 复杂度限制

| 指标 | 限制 | 检查工具 |
|:-----|:-----|:---------|
| 圈复杂度 (Cyclomatic) | < 10 | cppcheck |
| 函数行数 | < 50 行 | custom linter |
| 文件行数 | < 500 行 | custom linter |
| 参数数量 | < 5 个 | cppcheck |
| 嵌套深度 | < 4 层 | cppcheck |

### 2. 命名规范

| 类型 | 规范 | 示例 |
|:-----|:-----|:-----|
| 宏定义 | 全大写 + 下划线 | `MCU_CLOCK_MAIN` |
| 类型定义 | PascalCase + _Type | `Mcu_ConfigType` |
| 函数 | Module_ActionName | `Mcu_Init`, `Can_Write` |
| 变量 | camelCase | `clockSetting`, `canId` |
| 全局变量 | g_ModuleName | `g_McuConfig` |
| 常量 | kConstantName | `kMaxCanChannels` |

### 3. 注释规范

```c
/**
 * @brief 初始化 MCU 模块
 * 
 * @param ConfigPtr 指向 MCU 配置结构的指针
 * @return Std_ReturnType 
 *         - E_OK: 初始化成功
 *         - E_NOT_OK: 初始化失败
 * 
 * @pre 模块处于 MCU_UNINIT 状态
 * @post 模块处于 MCU_CLOCK_UNINIT 状态
 * 
 * @note 必须在其他模块初始化之前调用
 */
Std_ReturnType Mcu_Init(const Mcu_ConfigType* ConfigPtr);
```

## AutoSAR 合规规则

### 1. 标准类型使用

```c
/* 正确: 使用 AutoSAR 标准类型 */
#include "Std_Types.h"

Std_ReturnType result;
uint32 clockFrequency;
sint16 temperature;
boolean isEnabled;

/* 错误: 使用原生类型 */
int result;  /* 应该用 Std_ReturnType */
unsigned long clockFrequency;  /* 应该用 uint32 */
```

### 2. 错误处理

```c
/* 正确: 显式错误处理 */
Std_ReturnType result = Mcu_Init(&McuConfig);
if (result != E_OK) {
    /* 错误处理 */
    Det_ReportError(MCU_MODULE_ID, MCU_INSTANCE_ID, 
                    MCU_API_ID_INIT, MCU_E_INIT_FAILED);
    return E_NOT_OK;
}

/* 错误: 忽略返回值 */
Mcu_Init(&McuConfig);  /* 危险! */
```

### 3. 配置 vs 代码分离

```c
/* Mcu_Cfg.c - 配置文件 */
const Mcu_ConfigType Mcu_Config = {
    .ClockSetting = MCU_CLOCK_MAIN,
    .ClockFrequency = 160000000U
};

/* Mcu.c - 实现文件 */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr) {
    /* 使用 ConfigPtr 中的配置 */
}
```

## 安全规则

### 1. 指针检查

```c
/* 正确: 空指针检查 */
if (ConfigPtr == NULL_PTR) {
    Det_ReportError(MCU_MODULE_ID, MCU_INSTANCE_ID,
                    MCU_API_ID_INIT, MCU_E_PARAM_POINTER);
    return E_NOT_OK;
}

/* 错误: 不检查指针 */
/* ConfigPtr 可能为 NULL! */
```

### 2. 数组边界检查

```c
/* 正确: 边界检查 */
if (ChannelId >= MCU_MAX_CHANNELS) {
    Det_ReportError(MCU_MODULE_ID, MCU_INSTANCE_ID,
                    MCU_API_ID_SETMODE, MCU_E_PARAM_CHANNEL);
    return E_NOT_OK;
}
Mcu_ChannelConfig[ChannelId].mode = newMode;

/* 错误: 不检查边界 */
Mcu_ChannelConfig[ChannelId].mode = newMode;  /* 可能越界! */
```

### 3. 初始化检查

```c
/* 正确: 状态检查 */
if (Mcu_State == MCU_UNINIT) {
    Det_ReportError(MCU_MODULE_ID, MCU_INSTANCE_ID,
                    MCU_API_ID_SETMODE, MCU_E_UNINIT);
    return E_NOT_OK;
}

/* 错误: 不检查状态 */
/* 模块可能未初始化! */
```

## 测试规则

### 1. TDD 要求

- **RED**: 先写测试，测试失败
- **GREEN**: 写最小实现，测试通过
- **REFACTOR**: 重构代码，保持测试通过

### 2. 测试覆盖率

| 模块类型 | 语句覆盖 | 分支覆盖 | MC/DC |
|:---------|:---------|:---------|:------|
| MCAL | > 90% | > 85% | > 100% |
| Service | > 85% | > 80% | > 100% |
| Utility | > 80% | > 75% | - |

### 3. 测试命名

```c
/* 测试函数命名规范 */
void test_<Module>_<Function>_<Scenario>(void);

/* 示例 */
void test_Mcu_Init_shouldInitializeClock(void);
void test_Mcu_Init_shouldReturnErrorWhenNullConfig(void);
void test_Can_Write_shouldSendStandardFrame(void);
```

## 工具链规则

### 1. 代码生成

- 生成代码必须可编译
- 生成代码必须通过 MISRA C:2012 检查
- 生成代码必须包含版本信息
- 生成代码必须包含版权声明

### 2. 配置验证

- 所有配置参数必须验证范围
- 模块依赖必须自动检查
- 冲突配置必须报告错误
- 生成前必须通过所有校验

## 违规处理

| 违规级别 | 处理方式 |
|:---------|:---------|
| **ERROR** | 阻止提交，必须修复 |
| **WARNING** | 允许提交，但需记录 |
| **INFO** | 仅提示，建议改进 |

## 检查清单

### 提交前检查

- [ ] 代码通过编译 (0 errors, 0 warnings)
- [ ] 通过 cppcheck 静态分析
- [ ] 通过 MISRA C:2012 检查
- [ ] 单元测试全部通过
- [ ] 测试覆盖率达标
- [ ] 代码审查通过
- [ ] 文档已更新

### 代码审查检查

- [ ] 符合分层架构
- [ ] 命名规范正确
- [ ] 错误处理完善
- [ ] 注释清晰完整
- [ ] 复杂度达标
- [ ] 无安全漏洞
