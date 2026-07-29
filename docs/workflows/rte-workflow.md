# RTE 工作流: ARXML 配置驱动代码生成

> **文档版本**: v1.0  
> **更新日期**: 2026-07-26  
> **关联项目**: yuleASR Phase 1 — P1-1

---

## 1. 概述

本工作流描述了从 ARXML 配置到 RTE (Runtime Environment) C 代码生成的完整流程。采用 **配置驱动代码生成** 方法，替代传统手写 BSW/SWC 胶合代码，确保与 AUTOSAR 规范一致。

### 核心原则

1. **配置即代码** — ARXML 是唯一的配置来源
2. **可重复** — 相同输入产生相同输出（确定性生成）
3. **增量更新** — 只新增，不修改现有 tools/ 代码
4. **可追溯** — 生成的代码包含源 ARXML 引用和生成时间戳

---

## 2. 架构

```
yuleASR-Configurator (Web UI, React 19 + TypeScript)
        │
        ▼  Export .arxml
┌─────────────────────────────┐
│  ARXML 文件                  │
│  (config/input/arxml/*.arxml) │
└──────────┬──────────────────┘
           │
           ▼  Parse
┌─────────────────────────────┐
│  ARXML Parser                │
│  (tools/arxml/parser/)       │
│  • SWC 描述                  │
│  • Port/Interface            │
│  • Runnable/Event            │
│  • Data Type                 │
└──────────┬──────────────────┘
           │
           ▼  Internal IR
┌─────────────────────────────┐
│  RTE Code Generator          │
│  (tools/code_generators/rte/)│
│  • IR Builder                │
│  • C Code Renderer           │
│  • MISRA Compliant Output    │
└──────────┬──────────────────┘
           │
           ▼  .h / .c
┌─────────────────────────────┐
│  Generated RTE Code          │
│  (src/rte/generated/)        │
│  • Rte.h                     │
│  • Rte_Type.h                │
│  • Rte.c                     │
│  • Rte_SWC.h / Rte_SWC.c     │
└──────────┬──────────────────┘
           │
           ▼  Compile + Check
┌─────────────────────────────┐
│  yuleOSH CI Pipeline         │
│  • CMake Compile             │
│  • MISRA Check               │
│  • Coverage Report           │
└─────────────────────────────┘
```

---

## 3. 快速开始

### 3.1 前提条件

- Python 3.10+
- yuleASR ARXML Parser (tools/arxml/parser/)
- 现有 .arxml 文件

### 3.2 基本用法

```bash
# 从 ARXML 生成所有 SWC 的 RTE 代码
cd /path/to/yuleASR
python3 tools/code_generators/rte/rte_generator.py \
    -i config/input/arxml/bcm_demo.arxml \
    -o src/rte/generated/

# 仅生成指定 SWC
python3 tools/code_generators/rte/rte_generator.py \
    -i config/input/arxml/bcm_demo.arxml \
    -o src/rte/generated/ \
    --swc BCM_Door --swc BCM_Light

# 查看可选参数
python3 tools/code_generators/rte/rte_generator.py --help
```

### 3.3 使用 Shell 脚本

```bash
# 完整生成
./scripts/rte_generation.sh

# 自定义输入
./scripts/rte_generation.sh --arxml my_config.arxml

# 仅验证（不生成）
./scripts/rte_generation.sh --check

# 生成 + MISRA 检查
./scripts/rte_generation.sh --misra
```

### 3.4 使用 yuleOSH Pipeline

```bash
# 运行完整的 CI pipeline（包含 RTE 生成阶段）
yuleosh ci run

# 仅运行 RTE 生成阶段
python3 tools/code_generators/rte/stage/rte_generation.py
```

---

## 4. 生成的文件

| 文件 | 描述 | 对应 SWC |
|------|------|----------|
| `Rte.h` | 全局 RTE 头文件（类型定义、全局缓冲、Read/Write 声明、宏） | — |
| `Rte_Type.h` | 共享类型定义（Std_Types, Compiler 兼容） | — |
| `Rte.c` | 全局 RTE 实现（初始化、主循环调度） | — |
| `Rte_<SwcName>.h` | 特定 SWC 的 RTE 接口（Port API、Runnable 声明） | 每个 SWC |
| `Rte_<SwcName>.c` | 特定 SWC 的 RTE 实现（缓冲管理、Read/Write 实现、Runnable 框架） | 每个 SWC |

### 4.1 命名约定

```
Rte_Read_<SWC>_<Port>_<DataElement>(<type>* data)
Rte_Write_<SWC>_<Port>_<DataElement>(const <type>* data)
Rte_Call_<SWC>_<Port>_<Operation>(args...)
Rte_Server_<SWC>_<Port>_<Operation>(args...)
```

### 4.2 宏速记

```c
// Read (R_PORT) → 宏定义 RTE_READ_<SWC>_<PORT>_<DE>
Rte_Read_BCM_Door_DoorStatus_R_DoorStatus(&status);

// Write (P_PORT) → 宏定义 RTE_WRITE_<SWC>_<PORT>_<DE>
Rte_Write_BCM_Door_DoorLock_P_LockCommand(&cmd);
```

---

## 5. BCM Demo SWCs

BCM (Body Control Module) Demo 包含 4 个 SWC，验证完整的 ARXML→RTE 流程：

| SWC | 端口 | Runnable | 描述 |
|-----|------|----------|------|
| BCM_Door | DoorStatus_R (Read), DoorLock_P (Write), LightSwitch_R (Read) | DoorMonitor_Runnable, DoorLock_Runnable | 门控 |
| BCM_Light | LightSwitch_R (Read), LightOutput_P (Write) | LightControl_Runnable | 灯光 |
| BCM_Wiper | WiperSpeed_R (Read), WiperCtrl_P (Write) | WiperControl_Runnable | 雨刮 |
| BCM_Power | PowerMode_P (Write) | PowerManager_Runnable | 电源 |

### 5.1 ARXML 文件位置

```
tools/code_generators/rte/examples/bcm_demo.arxml
```

### 5.2 BCM 模板实现

```
tools/code_generators/rte/templates/bcm/
├── Bcm_Door.c
├── Bcm_Light.c
└── Bcm_Wiper.c
```

---

## 6. Pipeline 集成

### 6.1 CI 配置

在 `.yuleosh/ci-config.yaml` 中配置 RTE 生成阶段：

```yaml
stages:
  - name: rte_generation
    label: "ARXML → RTE C Code Generation"
    layer: 1
    timeout: 60
    entry: tools/code_generators/rte/stage/rte_generation.py
    args:
      --project-root: "."
      --output: "src/rte/generated"
```

### 6.2 构建集成

在 `src/rte/CMakeLists.txt` 中已支持从 `generated/` 目录加载生成的源文件：

```cmake
file(GLOB RTE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c
)
```

### 6.3 测试集成

```bash
# 运行 RTE 生成器单元测试
cd /path/to/yuleASR
python3 -m pytest tools/code_generators/rte/tests/ -v

# 验证生成输出
python3 tools/code_generators/rte/stage/rte_generation.py --validate-only
```

---

## 7. MISRA 合规

生成的 RTE 代码遵循 AUTOSAR MISRA 编码规范：

| MISRA 规则 | 状态 | 说明 |
|-----------|------|------|
| Dir 4.1 (必要的 include guard) | 通过 | 每个 .h 有 `#ifndef`/`#define`/`#endif` |
| Rule 8.4 (外部链接) | 通过 | 正确的 `extern` 声明 |
| Rule 11.9 (NULL 指针) | 通过 | 使用 `NULL_PTR` 宏 |
| Rule 17.7 (函数返回值) | 通过 | 生成代码正确处理返回值 |

### 偏差管理

生成代码不引入新的 MISRA 偏差。所有偏差来自现有 BSW 代码。

---

## 8. 扩展指南

### 8.1 添加新 SWC

1. 在 ARXML 文件中定义新的 `APPLICATION-SW-COMPONENT-TYPE`
2. 添加端口（P-PORT/R-PORT）和接口引用
3. 添加内部行为（Runnable 实体）
4. 运行生成器 → 自动生成 `Rte_<NewSwc>.h`/`.c`

### 8.2 更新现有 SWC

1. 修改 ARXML（添加端口、数据元素等）
2. 重新运行生成器 → 更新对应文件
3. 生成器是确定性的，不修改已重写的业务逻辑

### 8.3 自定义类型映射

在 `rte_generator.py` 中扩展 `AUTOSAR_TO_C_TYPE` 字典：

```python
AUTOSAR_TO_C_TYPE = {
    "myCustomType": "uint32",
    # ...
}
```

---

## 9. 验收标准

| # | 标准 | 验证方法 | 状态 |
|:-:|------|----------|:----:|
| 1 | BCM Demo: ARXML→Rte.h/Rte.c→编译通过 | pytest + CMake | ✅ |
| 2 | MISRA check 通过 | cppcheck | ✅ |
| 3 | yuleosh ci run 包含 RTE stage | CI 配置 | ✅ |
| 4 | 生成效率: 配置到编译 < 1分钟 | 实际测试 ~5s | ✅ |

---

## 10. 文件清单

```
tools/code_generators/rte/
├── __init__.py                        # 包入口
├── rte_generator.py                   # 主生成器 (IR Builder + Code Renderer)
├── stage/
│   └── rte_generation.py             # yuleOSH pipeline stage
├── examples/
│   └── bcm_demo.arxml                # BCM Demo ARXML
├── templates/
│   └── bcm/
│       ├── Bcm_Door.c                # BCM Door Runnable 模板
│       ├── Bcm_Light.c               # BCM Light Runnable 模板
│       └── Bcm_Wiper.c               # BCM Wiper Runnable 模板
└── tests/
    └── test_rte_generator.py         # 22 个单元/集成测试

scripts/
└── rte_generation.sh                  # Shell wrapper

docs/workflows/
└── rte-workflow.md                    # 本文档
```
