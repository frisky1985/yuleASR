# yuleASR 项目结构说明

本文档说明 yuleASR 项目的目录结构和组织方式。

## 目录结构

```
yuleASR/
├── src/                      # 源代码
│   ├── autosar/             # AUTOSAR BSW 静态代码
│   │   ├── mcal/           # 微控制器驱动层 (Microcontroller Drivers)
│   │   ├── ecual/          # ECU抽象层 (ECU Abstraction)
│   │   ├── services/       # 服务层 (Services)
│   │   ├── crypto/         # 加密服务
│   │   ├── common/         # 通用头文件
│   │   └── integration/    # 集成代码
│   ├── application/         # 应用层 (ASW)
│   │   └── swc/            # 软件组件
│   ├── middleware/          # 中间件
│   │   ├── dds/            # DDS协议栈
│   │   ├── microdds/       # Micro-DDS实现
│   │   └── rte/            # RTE实现
│   ├── platform/            # 平台相关代码
│   │   └── s32k312/        # NXP S32K312
│   └── diagnostics/         # 诊断模块 (DCM/DEM)
├── config/                 # 配置代码
│   ├── mcal/              # MCAL配置
│   ├── ecual/             # ECUAL配置
│   ├── services/          # Services配置
│   ├── generated/         # 工具生成的配置
│   ├── templates/         # 配置模板
│   └── arxml/             # ARXML配置文件
├── tests/                  # 测试代码
│   ├── unit/              # 单元测试
│   │   ├── autosar/       # AUTOSAR模块测试
│   │   ├── middleware/    # 中间件测试
│   │   ├── platform/      # 平台测试
│   │   ├── diagnostics/   # 诊断模块测试
│   │   └── framework/     # 测试框架
│   ├── integration/       # 集成测试
│   ├── system/            # 系统测试
│   └── resources/         # 测试资源
├── tools/                  # 工具链
│   ├── arxml/             # ARXML处理工具
│   ├── can_config/        # CAN配置工具
│   ├── dtc_config/        # DTC配置工具
│   ├── code_generators/   # 代码生成器
│   ├── analysis/          # 静态分析工具
│   └── build/             # 构建脚本
├── third_party/            # 第三方代码
│   ├── crypto/            # 加密库
│   ├── test_frameworks/   # 测试框架
│   ├── network/           # 网络协议栈
│   └── rtos/              # 实时操作系统
├── docs/                   # 文档
│   ├── architecture/      # 架构文档
│   ├── api/               # API参考
│   ├── modules/           # 模块文档
│   ├── guides/            # 使用指南
│   ├── design/            # 设计文档
│   ├── specs/             # 规范文档
│   ├── reports/           # 项目报告
│   └── external/          # 外部资料
├── scripts/                # 脚本
│   ├── build/             # 构建脚本
│   ├── test/              # 测试脚本
│   ├── deploy/            # 部署脚本
│   └── analysis/          # 分析脚本
├── output/                 # 输出目录
│   ├── build/             # 构建输出
│   ├── reports/           # 报告输出
│   └── generated/         # 生成的文件
├── cmake/                  # CMake配置
├── build/                  # 构建目录
├── examples/               # 示例代码
├── platform/               # 平台配置
└── openspec/               # OpenSpec文档
```

## 各层详细说明

### 1. src/autosar/ - AUTOSAR静态代码

遵循AUTOSAR标准的基础软件模块，按层级组织：

- **mcal/**: 微控制器驱动层 (ADC, CAN, Crypto, DIO, ETH, FLS, GPT, I2C, ICU, LIN, MCU, PORT, PWM, SPI, UART, WDG等)
- **ecual/**: ECU抽象层 (CanIf, CanTp, EthIf, LinIf, FrIf, MemIf, WdgIf等)
- **services/**: 服务层 (COM, DCM, DEM, NVM, PDUR, CSM, Crypto Services等)
- **crypto/**: 加密服务特定实现
- **common/**: 通用类型和头文件 (Std_Types.h, Compiler.h等)
- **integration/**: 集成模块 (BswM, EcuM)

每个模块结构：
```
<module>/
├── include/
│   ├── <Module>.h       # API头文件
│   ├── <Module>_Cfg.h   # 配置头文件
│   └── <Module>_MemMap.h # 内存映射
└── src/
    ├── <Module>.c       # 主实现
    ├── <Module>_Irq.c   # 中断处理
    └── <Module>_Lcfg.c  # 链接配置
```

### 2. src/application/ - 应用层 (ASW)

AUTOSAR应用软件组件 (SWC)：
- **swc/**: 软件组件实现
  - communication_manager: 通信管理
  - diagnostic_manager: 诊断管理
  - engine_control: 发动机控制
  - io_control: IO控制
  - mode_manager: 模式管理
  - storage_manager: 存储管理
  - vehicle_dynamics: 车辆动力学
  - watchdog_manager: 看门狗管理

### 3. src/middleware/ - 中间件

- **dds/**: OMG DDS协议栈实现
- **microdds/**: 轻量级DDS实现
- **rte/**: 运行时环境 (RTE)

### 4. src/platform/ - 平台相关

微控制器特定的平台实现：
- **s32k312/**: NXP S32K312平台支持

### 5. config/ - 配置代码

按层级组织的配置文件：
- **mcal/**: MCAL模块配置
- **ecual/**: ECUAL模块配置
- **services/**: Services模块配置
- **generated/**: 工具生成的配置
- **templates/**: 配置模板
- **arxml/**: ARXML配置文件

### 6. tests/ - 测试代码

- **unit/**: 单元测试
  - autosar/: AUTOSAR模块测试
  - middleware/: 中间件测试
  - platform/: 平台测试
  - diagnostics/: 诊断模块测试
  - framework/: 测试框架
- **integration/**: 集成测试
- **system/**: 系统测试
- **resources/**: 测试资源 (ARXML, DBC, CSV)

### 7. tools/ - 工具链

- **arxml/**: ARXML处理工具
  - parser/: 解析器
  - generator/: 代码生成器
  - checker/: 完整性检查
- **can_config/**: CAN配置工具 (支持DBC/CSV)
- **dtc_config/**: DTC配置工具
- **code_generators/**: 代码生成器
- **analysis/**: 静态分析工具
- **build/**: 构建脚本

### 8. third_party/ - 第三方代码

- **crypto/**: 加密库
  - mbedtls/: mbedTLS (Git Submodule)
  - aes_modes/: AES加密模式
  - blake2/: Blake2哈希
  - hash/: SHA系列
- **test_frameworks/**: 测试框架
  - unity/: Unity单元测试框架
  - gtest/: Google Test
- **network/**: 网络协议栈
  - lwip/: lwIP (预留)
  - tls/: TLS/SSL (预留)
- **rtos/**: 实时操作系统
  - freertos/: FreeRTOS (预留)

### 9. docs/ - 文档

- **architecture/**: 架构文档
- **api/**: API参考手册
- **modules/**: 模块文档
- **guides/**: 使用指南
- **design/**: 设计文档
- **specs/**: 规范文档
- **reports/**: 项目报告
- **external/**: 外部资料

## 移植指南

### 应用移植步骤

1. **导入BSW模块**
   - 从 `src/autosar/` 复制需要的模块
   - 将配置从 `config/` 复制到目标项目

2. **导入中间件**
   - 从 `src/middleware/` 复制DDS或RTE
   - 配置中间件参数

3. **使用工具链**
   - ARXML工具: `tools/arxml/arxml_tool.py`
   - CAN配置: `tools/can_config/can-config-tool.py`

4. **运行测试**
   - 使用 `tests/` 下的测试用例
   - 导入Unity测试框架

### 关键文件

| 类型 | 位置 | 说明 |
|------|------|------|
| 通用头文件 | `src/autosar/common/include/` | Std_Types.h, Compiler.h |
| 配置模板 | `config/templates/` | 配置文件模板 |
| 构建系统 | `CMakeLists.txt`, `cmake/` | CMake配置 |
| 项目说明 | `README.md`, `PROJECT_STRUCTURE.md` | 本文档 |

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024-04 | 初始结构 |
| 2.0 | 2025-05 | 重构整理，建立清晰的层次结构 |
