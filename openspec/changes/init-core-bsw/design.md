# Design: 核心 BSW 模块架构设计

> **变更ID**: init-core-bsw  
> **版本**: v1.0  
> **日期**: 2026-04-13

## 架构概述

```
┌─────────────────────────────────────────────────────────────┐
│                    Yule BSW Core v1.0                       │
├─────────────────────────────────────────────────────────────┤
│  Configuration Layer (ARXML + JSON)                         │
├─────────────────────────────────────────────────────────────┤
│  Code Generator (Python + Jinja2)                           │
├─────────────────────────────────────────────────────────────┤
│  MCAL Layer                                                 │
│  ┌─────────────┬─────────────┬─────────────────────────┐   │
│  │  MCU Driver │  GPIO Driver│      CAN Driver         │   │
│  │  - Clock    │  - Digital  │      - Controller       │   │
│  │  - Reset    │    I/O      │      - Send/Recv        │   │
│  │  - Power    │  - Interrupt│      - Interrupt        │   │
│  └─────────────┴─────────────┴─────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Hardware Abstraction Layer (HAL)                           │
├─────────────────────────────────────────────────────────────┤
│  Target: NXP S32K344 (Cortex-M7)                            │
└─────────────────────────────────────────────────────────────┘
```

## 目录结构

```
src/bsw/
├── common/                     # 通用定义
│   ├── Std_Types.h            # 标准类型定义
│   ├── Std_ReturnType.h       # 返回类型定义
│   └── Compiler.h             # 编译器抽象
│
├── mcal/                       # MCAL 层
│   ├── mcu/                   # MCU Driver
│   │   ├── include/
│   │   │   ├── Mcu.h
│   │   │   └── Mcu_Cfg.h
│   │   └── src/
│   │       ├── Mcu.c
│   │       └── Mcu_Lcfg.c
│   │
│   ├── port/                  # GPIO Driver
│   │   ├── include/
│   │   │   ├── Port.h
│   │   │   └── Port_Cfg.h
│   │   └── src/
│   │       ├── Port.c
│   │       └── Port_Lcfg.c
│   │
│   └── can/                   # CAN Driver
│       ├── include/
│       │   ├── Can.h
│       │   └── Can_Cfg.h
│       └── src/
│           ├── Can.c
│           └── Can_Lcfg.c
│
└── hal/                        # 硬件抽象层
    └── s32k3/                 # S32K3 平台实现
        ├── Mcu_Hw.h
        ├── Port_Hw.h
        └── Can_Hw.h
```

## 模块详细设计

### 1. MCU Driver

#### 1.1 架构设计

```c
/* Mcu.h - 公共接口 */
#ifndef MCU_H
#define MCU_H

#include "Std_Types.h"

typedef uint32 Mcu_ClockType;
typedef uint32 Mcu_RawResetType;
typedef uint8 Mcu_ModeType;

typedef struct {
    Mcu_ClockType ClockSetting;
    uint32 ClockFrequency;
} Mcu_ConfigType;

/* API 声明 */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr);
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);
void Mcu_SetMode(Mcu_ModeType McuMode);
Mcu_RawResetType Mcu_GetResetReason(void);

#endif
```

#### 1.2 状态机

```
                    ┌─────────────┐
                    │   MCU_UNINIT │
                    └──────┬──────┘
                           │ Mcu_Init()
                           ▼
                    ┌─────────────┐
         ┌─────────│  MCU_CLOCK_UNINIT │
         │         └──────┬──────┘
         │                │ Mcu_InitClock()
         │                ▼
         │         ┌─────────────┐
         │         │  MCU_CLOCK_INITIALIZED │
         │         └──────┬──────┘
         │                │ Mcu_SetMode()
         │                ▼
         └────────►┌─────────────┐
                   │  MCU_MODE_xxx │
                   └─────────────┘
```

### 2. GPIO (Port) Driver

#### 2.1 架构设计

```c
/* Port.h - 公共接口 */
#ifndef PORT_H
#define PORT_H

#include "Std_Types.h"

typedef uint8 Port_PinType;
typedef uint8 Port_PinModeType;
typedef uint8 Port_PinDirectionType;
typedef uint8 Port_PinLevelType;

/* 引脚方向 */
#define PORT_PIN_IN    0x00
#define PORT_PIN_OUT   0x01

/* 引脚电平 */
#define PORT_PIN_LEVEL_LOW    0x00
#define PORT_PIN_LEVEL_HIGH   0x01

typedef struct {
    Port_PinType Pin;
    Port_PinDirectionType Direction;
    Port_PinModeType Mode;
    Port_PinLevelType InitialLevel;
} Port_PinConfigType;

typedef struct {
    uint8 NumPins;
    const Port_PinConfigType* PinConfig;
} Port_ConfigType;

/* API 声明 */
void Port_Init(const Port_ConfigType* ConfigPtr);
void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction);
void Port_RefreshPortDirection(void);

/* Dio 接口 */
void Dio_WriteChannel(Port_PinType ChannelId, Port_PinLevelType Level);
Port_PinLevelType Dio_ReadChannel(Port_PinType ChannelId);

#endif
```

#### 2.2 引脚映射 (S32K344)

```
PTA[0-31]  - Port A (32 pins)
PTB[0-31]  - Port B (32 pins)
PTC[0-31]  - Port C (32 pins)
PTD[0-31]  - Port D (32 pins)
PTE[0-31]  - Port E (32 pins)
```

### 3. CAN Driver

#### 3.1 架构设计

```c
/* Can.h - 公共接口 */
#ifndef CAN_H
#define CAN_H

#include "Std_Types.h"

typedef uint8 Can_HwHandleType;
typedef uint16 Can_IdType;
typedef uint8 Can_PduLengthType;

typedef struct {
    Can_IdType id;
    Can_PduLengthType length;
    uint8* sdu;
} Can_PduType;

typedef struct {
    uint32 Baudrate;
    uint8 ControllerId;
    /* ... */
} Can_ControllerConfigType;

typedef struct {
    uint8 NumControllers;
    const Can_ControllerConfigType* ControllerConfig;
} Can_ConfigType;

/* API 声明 */
void Can_Init(const Can_ConfigType* Config);
Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);

/* 回调函数 (由应用实现) */
void CanIf_RxIndication(uint8 Hrh, Can_IdType CanId, 
                        uint8 CanDlc, const uint8* CanSduPtr);
void CanIf_TxConfirmation(uint8 Hth);

#endif
```

#### 3.2 报文格式

```
标准帧 (11-bit ID):
┌────────┬────────┬────────┬────────┐
│  SOF   │  ID    │  RTR   │  DLC   │
│  1bit  │ 11bits │  1bit  │ 4bits  │
└────────┴────────┴────────┴────────┘

扩展帧 (29-bit ID):
┌────────┬────────┬────────┬────────┬────────┐
│  SOF   │  ID    │  SRR   │  IDE   │  ID    │
│  1bit  │ 11bits │  1bit  │  1bit  │ 18bits │
└────────┴────────┴────────┴────────┴────────┘
```

## 代码生成器设计

### 架构

```python
# generator/core.py

class BswCodeGenerator:
    """BSW 代码生成器核心"""
    
    def __init__(self, template_dir: str):
        self.jinja_env = Environment(
            loader=FileSystemLoader(template_dir),
            trim_blocks=True,
            lstrip_blocks=True
        )
    
    def generate_from_arxml(self, arxml_path: str, output_dir: str):
        """从 ARXML 生成代码"""
        # 1. 解析 ARXML
        model = self._parse_arxml(arxml_path)
        
        # 2. 验证模型
        self._validate_model(model)
        
        # 3. 生成各模块代码
        for module in model.modules:
            self._generate_module(module, output_dir)
    
    def _generate_module(self, module: Module, output_dir: str):
        """生成单个模块代码"""
        # 生成头文件
        header_template = self.jinja_env.get_template(f"{module.name}_Cfg.h.j2")
        header_code = header_template.render(module=module)
        self._write_file(output_dir, f"{module.name}_Cfg.h", header_code)
        
        # 生成源文件
        source_template = self.jinja_env.get_template(f"{module.name}_Cfg.c.j2")
        source_code = source_template.render(module=module)
        self._write_file(output_dir, f"{module.name}_Cfg.c", source_code)
```

### 模板示例

```jinja2
{# Mcu_Cfg.h.j2 #}
#ifndef MCU_CFG_H
#define MCU_CFG_H

#include "Mcu.h"

/* Version Info */
#define MCU_CFG_MAJOR_VERSION    1
#define MCU_CFG_MINOR_VERSION    0
#define MCU_CFG_PATCH_VERSION    0

/* Clock Configurations */
{% for clock in module.clocks %}
#define MCU_CLOCK_{{ clock.name | upper }}    {{ loop.index0 }}U
{% endfor %}

/* External Configuration */
extern const Mcu_ConfigType Mcu_Config;

#endif
```

## 配置工具设计

### 前端架构

```
┌─────────────────────────────────────────────┐
│  Vue 3 + TypeScript + Element Plus         │
├─────────────────────────────────────────────┤
│  Views                                      │
│  - ModuleConfigView (模块配置)              │
│  - CodeGenerationView (代码生成)            │
│  - ValidationView (校验结果)                │
├─────────────────────────────────────────────┤
│  Components                                 │
│  - ModuleTree (模块树组件)                  │
│  - ConfigForm (配置表单)                    │
│  - ParameterEditor (参数编辑器)             │
├─────────────────────────────────────────────┤
│  Stores (Pinia)                             │
│  - useConfigStore (配置状态)                │
│  - useModuleStore (模块状态)                │
│  - useValidationStore (校验状态)            │
├─────────────────────────────────────────────┤
│  API (Axios)                                │
│  - ConfigAPI                                │
│  - GeneratorAPI                             │
└─────────────────────────────────────────────┘
```

### API 设计

```typescript
// api/config.ts
export interface ConfigAPI {
  createProject(name: string): Promise<Project>;
  getModules(): Promise<Module[]>;
  saveConfig(projectId: string, config: Config): Promise<void>;
  validateConfig(projectId: string): Promise<ValidationResult>;
  generateCode(projectId: string): Promise<GenerationResult>;
}

// api/generator.ts
export interface GeneratorAPI {
  generate(projectId: string, options: GenerateOptions): Promise<Blob>;
  getTemplates(): Promise<Template[]>;
  previewGeneration(projectId: string, filePath: string): Promise<string>;
}
```

## 测试策略

### 单元测试

```c
/* test_mcu.c - MCU 单元测试 */
#include "unity.h"
#include "Mcu.h"

void setUp(void) {
    /* 测试前初始化 */
}

void tearDown(void) {
    /* 测试后清理 */
}

void test_Mcu_Init_shouldInitializeClock(void) {
    /* Given */
    Mcu_ConfigType config = {
        .ClockSetting = 0,
        .ClockFrequency = 160000000  /* 160MHz */
    };
    
    /* When */
    Mcu_Init(&config);
    
    /* Then */
    TEST_ASSERT_EQUAL(160000000, Mcu_GetClockFrequency());
}

void test_Mcu_GetResetReason_shouldReturnValidReason(void) {
    /* Given/When */
    Mcu_RawResetType reason = Mcu_GetResetReason();
    
    /* Then */
    TEST_ASSERT_NOT_EQUAL(0, reason);
}

/* 测试主函数 */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Mcu_Init_shouldInitializeClock);
    RUN_TEST(test_Mcu_GetResetReason_shouldReturnValidReason);
    return UNITY_END();
}
```

### 测试覆盖率目标

| 模块 | 语句覆盖 | 分支覆盖 | MC/DC |
|:-----|:---------|:---------|:------|
| MCU  | > 80%    | > 75%    | -     |
| GPIO | > 85%    | > 80%    | -     |
| CAN  | > 75%    | > 70%    | -     |

## 构建系统

### Makefile

```makefile
# Makefile for Yule BSW Core

CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m7 -mthumb -O2 -g \
         -Wall -Wextra -Werror \
         -I./include -I./src/bsw/common \
         -I./src/bsw/mcal/mcu/include \
         -I./src/bsw/mcal/port/include \
         -I./src/bsw/mcal/can/include

LDFLAGS = -T./linker/S32K344.ld

SOURCES = \
    src/bsw/common/Std_Types.c \
    src/bsw/mcal/mcu/src/Mcu.c \
    src/bsw/mcal/port/src/Port.c \
    src/bsw/mcal/can/src/Can.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = yule_bsw.elf

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	$(MAKE) -C tests/unit

clean:
	rm -f $(OBJECTS) $(TARGET)
```

## 版本历史

| 版本 | 日期 | 变更 |
|:-----|:-----|:-----|
| v1.0 | 2026-04-13 | 初始设计，定义核心架构 |
