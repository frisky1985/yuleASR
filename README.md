# yuleASR - YuleTech AutoSAR BSW Platform

<p align="center">
  <strong>基于 AutoSAR Classic Platform 标准的开源汽车基础软件平台</strong>
</p>

<p align="center">
  <a href="https://github.com/frisky1985/yuleASR/stargazers"><img src="https://img.shields.io/github/stars/frisky1985/yuleASR?style=flat-square&logo=github&color=yellow" alt="Stars"></a>
  <a href="https://github.com/frisky1985/yuleASR/network/members"><img src="https://img.shields.io/github/forks/frisky1985/yuleASR?style=flat-square&logo=github&color=blue" alt="Forks"></a>
  <a href="https://github.com/frisky1985/yuleASR/issues"><img src="https://img.shields.io/github/issues/frisky1985/yuleASR?style=flat-square&logo=github&color=red" alt="Issues"></a>
  <a href="https://github.com/frisky1985/yuleASR/blob/master/LICENSE"><img src="https://img.shields.io/github/license/frisky1985/yuleASR?style=flat-square&color=green" alt="License"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/AutoSAR-4.x%20%7C%2020-11-blue?style=flat-square&logo=automotive" alt="AutoSAR">
  <img src="https://img.shields.io/badge/C-99-blue?style=flat-square&logo=c" alt="C99">
  <img src="https://img.shields.io/badge/Python-3.8+-blue?style=flat-square&logo=python" alt="Python">
  <img src="https://img.shields.io/badge/Platform-NXP%20S32K312-orange?style=flat-square&logo=nxp" alt="NXP">
</p>

<p align="center">
  <a href="#overview">概览</a> •
  <a href="#structure">项目结构</a> •
  <a href="#features">功能特性</a> •
  <a href="#quick-start">快速开始</a> •
  <a href="#tools">开发工具</a> •
  <a href="#docs">文档</a>
</p>

---

## <a name="overview"></a> 概览

yuleASR 是 **上海予乐电子科技有限公司** 开发的开源汽车基础软件平台，提供完整的 AutoSAR Classic Platform 基础软件栈实现。

### 项目愿景

成为工程师的合作伙伴，通过提供基于 AutoSAR 标准的开源基础软件、便捷的开发工具链和活跃的技术社区，降低汽车基础软件开发门槛，为中国汽车软件产业赋能。

### 支持的硬件平台

- **NXP S32K312** (主要目标平台) - ARM Cortex-M7 处理器
- **NXP i.MX8M Mini** - ARM Cortex-A53 四核处理器
- 支持 CAN、CAN FD、Ethernet、LIN 等车载网络

---

## <a name="structure"></a> 项目结构

```
yuleASR/
├── src/                      # 源代码
│   ├── autosar/             # AUTOSAR BSW 静态代码
│   │   ├── mcal/           # 微控制器驱动层 (21模块)
│   │   ├── ecual/          # ECU抽象层 (29模块)
│   │   ├── services/       # 服务层 (44模块)
│   │   └── common/         # 通用头文件
│   ├── application/         # 应用层 (ASW)
│   ├── middleware/          # 中间件 (DDS, RTE)
│   ├── platform/            # 平台相关 (S32K312)
│   └── diagnostics/         # 诊断模块 (DCM/DEM)
├── config/                 # 配置代码 (117个配置文件)
├── tests/                  # 测试代码
├── tools/                  # 工具链
│   ├── arxml/             # ARXML处理工具
│   ├── can_config/        # CAN配置工具
│   ├── dtc_config/        # DTC配置工具
│   └── code_generators/   # 代码生成器
├── third_party/            # 第三方代码
├── docs/                   # 文档 (150+文档)
└── scripts/                # 构建和测试脚本
```

详细结构请参阅 [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)

---

## <a name="features"></a> 功能特性

### 完整的 BSW 分层架构

```
├── RTE (Runtime Environment)
│   ├── 组件间通信接口
│   ├── 数据类型定义
│   └── 调度器
├── Service Layer (44模块)
│   ├── Com, PduR, NvM, MemIf
│   ├── Dcm, Dem, Det, Dlt
│   ├── Csm, CryIf, KeyM, SecOC
│   ├── DoIP, SoAd, SomeIP
│   └── BswM, EcuM, SchM
├── ECUAL Layer (29模块)
│   ├── CanIf, CanTp, CanNm, CanSm
│   ├── EthIf, EthSm, EthTrcv
│   ├── LinIf, LinNm, LinSM, LinTp
│   ├── FrIf, FrTp, J1939Tp
│   └── IoHwAb, MemIf, Fee, Ea
├── MCAL Layer (21模块)
│   ├── Mcu, Port, Dio, Gpt, Pwm
│   ├── Adc, Spi, I2C, Uart, Lin
│   ├── Can, Eth, Wdg, Icu, Ocu
│   └── Flash, Fee, Crypto, Crc
└── Hardware (NXP S32K312)
```

### 核心功能

| 功能 | 说明 | 状态 |
|------|------|------|
| **完整的 MCAL** | 21个微控制器驱动 | ✅ 已实现 |
| **完整的 ECUAL** | 29个ECU抽象模块 | ✅ 已实现 |
| **完整的 Services** | 44个服务模块 | ✅ 已实现 |
| **诊断协议栈** | DCM/DEM完整实现 | ✅ 已实现 |
| **DDS中间件** | OMG DDS v1.4协议 | ✅ 已实现 |
| **ARXML工具链** | 解析/生成/检查 | ✅ 已实现 |
| **符合 AutoSAR** | 4.x / R20-11 标准 | ✅ 验证通过 |
| **完整测试** | 单元测试 + 集成测试 | ✅ 已覆盖 |

### 项目统计

<p align="center">
  <img src="https://img.shields.io/badge/BSW Modules-94-blue?style=flat-square" alt="94 Modules">
  <img src="https://img.shields.io/badge/Lines of Code-50K+-blue?style=flat-square" alt="50K+ LOC">
  <img src="https://img.shields.io/badge/Documentation-150+ Docs-success?style=flat-square" alt="150+ Docs">
  <img src="https://img.shields.io/badge/Tools-6 Categories-success?style=flat-square" alt="6 Tools">
</p>

---

## <a name="quick-start"></a> 快速开始

### 环境要求

- **操作系统**: Linux (Ubuntu 20.04+), Windows 10/11 (WSL2), macOS (12+)
- **编译器**: GCC ARM 10.3+, Clang 12+
- **Python**: 3.8+ (pip, venv)
- **CMake**: 3.20+
- **Git**: 2.30+

### 安装依赖

```bash
# 安装系统依赖 (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 python3-pip

# 安装 Python 依赖
pip3 install -r tools/requirements.txt
```

### 获取代码

```bash
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
git submodule update --init --recursive
```

### 构建项目

```bash
# 创建构建目录
mkdir -p build && cd build

# 配置项目
cmake .. -DTARGET_PLATFORM=S32K312

# 构建项目
make -j$(nproc)

# 运行测试
make test
```

### 使用工具链

```bash
# ARXML处理工具
./tools/arxml/arxml_tool.py --help
./tools/arxml/arxml_tool.py parse examples/system.arxml
./tools/arxml/arxml_tool.py generate config.json -o output/
./tools/arxml/arxml_tool.py analyze system.arxml -o report.md

# CAN配置工具
./tools/can_config/can-config-tool.py --dbc examples/example.dbc --output config/

# DTC配置工具
./tools/dtc_config/dtc-tool.sh --input dtc_config.csv
```

---

## <a name="tools"></a> 开发工具

### ARXML 工具链

完整的 ARXML 处理解决方案：

| 工具 | 功能 | 状态 |
|------|------|------|
| **arxml_parser.py** | ARXML 解析器 (R20-11) | ✅ 完成 |
| **config_generator.py** | C代码生成器 | ✅ 完成 |
| **integrity_checker.py** | 完整性检查 | ✅ 完成 |
| **arxml_tool.py** | 统一CLI入口 | ✅ 完成 |

### 配置工具

| 工具 | 功能 | 支持格式 |
|------|------|----------|
| **CAN Config Tool** | CAN报文配置 | DBC, CSV, ARXML |
| **DTC Configurator** | 诊断故障码配置 | JSON, CSV |
| **Code Generator** | 代码生成 | C/H, ARXML |

---

## <a name="docs"></a> 文档

### 快速链接

- [项目结构说明](PROJECT_STRUCTURE.md) - 完整的目录结构文档
- [API 参考手册](docs/api/) - 完整的API文档
- [BSW 模块文档](docs/modules/) - 各模块详细说明
- [开发指南](docs/guides/) - 开发人员手册
- [API参考](docs/api-reference.md) - API快速参考

### 文档分类

```
docs/
├── architecture/      # 架构文档
├── api/              # API参考 (5个文档)
├── modules/          # 模块文档 (30个文档)
├── guides/           # 使用指南 (13个文档)
├── design/           # 设计文档 (9个文档)
├── specs/            # 规范文档 (8个文档)
├── reports/          # 项目报告 (61个文档)
└── external/         # 外部资料
```

---

## 贡献

欢迎提交 Issue 和 Pull Request！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 了解详情。

### 代码规范

- 使用 C99 标准
- 严格遵循 MISRA C:2012 规范
- 所有代码需要通过静态分析和单元测试

---

## 版本历史

| 版本 | 日期 | 主要变更 |
|------|------|---------|
| v2.0 | 2025-05 | 项目结构重构，添加DDS和ARXML工具 |
| v1.0 | 2024-04 | 初始版本，完整BSW实现 |

---

## 许可证

本项目采用 MIT 许可证 - 详情请参阅 [LICENSE](LICENSE)

---

<p align="center">
  <strong>上海予乐电子科技有限公司</strong><br>
  让每个工程师都能构建可靠的汽车软件
</p>
