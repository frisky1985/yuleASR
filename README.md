# yuleASR — AUTOSAR Classic BSW Platform

<p align="center">
  <strong>基于 AUTOSAR Classic Platform 标准的开源汽车基础软件平台</strong><br>
  <em>由上海予乐电子科技开发，社区驱动</em>
</p>

<p align="center">
  <a href="https://github.com/frisky1985/yuleASR/stargazers"><img src="https://img.shields.io/github/stars/frisky1985/yuleASR?style=flat-square&logo=github&color=yellow" alt="Stars"></a>
  <a href="https://github.com/frisky1985/yuleASR/network/members"><img src="https://img.shields.io/github/forks/frisky1985/yuleASR?style=flat-square&logo=github&color=blue" alt="Forks"></a>
  <a href="https://github.com/frisky1985/yuleASR/issues"><img src="https://img.shields.io/github/issues/frisky1985/yuleASR?style=flat-square&logo=github&color=red" alt="Issues"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="MIT License"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/AutoSAR-4.x%20%7C%2020-11-blue?style=flat-square&logo=automotive" alt="AutoSAR">
  <img src="https://img.shields.io/badge/C-99-blue?style=flat-square&logo=c" alt="C99">
  <img src="https://img.shields.io/badge/Python-3.8+-blue?style=flat-square&logo=python" alt="Python">
  <img src="https://img.shields.io/badge/Platform-NXP%20S32K312-orange?style=flat-square&logo=nxp" alt="NXP">
</p>

<p align="center">
  <a href="https://github.com/frisky1985/yuleASR/actions/workflows/ci.yml"><img src="https://github.com/frisky1985/yuleASR/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/frisky1985/yuleASR/actions/workflows/deploy-docs.yml"><img src="https://github.com/frisky1985/yuleASR/actions/workflows/deploy-docs.yml/badge.svg" alt="Docs"></a>
  <a href="https://github.com/frisky1985/yuleASR/actions/workflows/integration-tests.yml"><img src="https://github.com/frisky1985/yuleASR/actions/workflows/integration-tests.yml/badge.svg" alt="Integration"></a>
  <a href="https://github.com/frisky1985/yuleASR/actions/workflows/misra-check.yml"><img src="https://github.com/frisky1985/yuleASR/actions/workflows/misra-check.yml/badge.svg" alt="MISRA"></a>
</p>

<p align="center">
  📖 <strong>文档站:</strong> <a href="https://frisky1985.github.io/yuleASR/"><code>https://frisky1985.github.io/yuleASR/</code></a>
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

### 模块完整性

| 层级 | 模块数 | 状态 | 备注 |
|:-----|:------:|:----:|:-----|
| **MCAL** (微控制器抽象层) | 21/21 | ✅ 全部完成 | Adc, Can, Crypto, Dio, Eep, Eth, Fee, Flash, Fls, Gpt, I2c, Icu, Lin, Mcu, Ocu, Port, Pwm, Ramtst, Spi, Uart, Wdg |
| **ECUAL** (ECU抽象层) | 29/29 | ✅ 全部完成 | CanIf, CanNm, CanSm, CanTp, CanTrcv, Dlt, DoIP, Ea, EthIf, EthSm, EthTrcv, Fee, FiM, FrIf, FrTp, IoHwAb, IpduM, J1939Tp, LinIf, LinNm, LinSM, LinTp, LinTrcv, MemIf, SomeIpIf, SomeIpSd, Srp, WdgIf, Xcp |
| **Services** (服务层) | 46/46 | ✅ 全部完成 | BswM, CanM, CanSM, CanTSyn, Com, ComM, Crc, CryIf, Csm, Dcm, Dem, Det, Dlt, DoCan, DoIP, E2E, EcuC, EcuM, EthSm, FiM, IpduM, J1939Nm, J1939Tp, KeyM, LinM, LinSM, LnTm, Mem, MemIf, Mqtt, Nm, NvM, PduR, RamSafety, SchM, SecOC, SoAd, SomeIp, SomeIpTp, SomeIpXf, StbM, Swc, UdpNm, WdgM, Xcp |
| **ASW** (应用层) | 8/8 | ✅ 全部完成 | EngineControl, VehicleDynamics, DiagnosticManager, CommunicationManager, StorageManager, IOControl, ModeManager, WatchdogManager |
| **RTE** (运行时环境) | — | ✅ 全部完成 | 组件间通信、数据类型定义、调度器 |
| **OS** (操作系统) | — | ✅ 全部完成 | 基于 FreeRTOS，支持任务/事件/资源/报警/中断管理 |

<p align="center">
  <img src="https://img.shields.io/badge/BSW Modules-96-blue?style=flat-square" alt="96 Modules">
  <img src="https://img.shields.io/badge/C Code-~214K%20lines-blue?style=flat-square" alt="~214K LOC">
  <img src="https://img.shields.io/badge/单元测试-260%2B-success?style=flat-square" alt="260+ Tests">
  <img src="https://img.shields.io/badge/Documentation-150%2B Docs-success?style=flat-square" alt="150+ Docs">
  <img src="https://img.shields.io/badge/Tools-6 Categories-success?style=flat-square" alt="6 Tools">
</p>

---

## <a name="vs-easyxmen"></a> 与 EasyXMen 对比亮点

yuleASR 作为新一代开源 AUTOSAR BSW 平台，与业界知名的 EasyXMen 相比具有以下优势：

| 维度 | yuleASR | EasyXMen |
|:-----|:--------|:---------|
| **开源许可** | ✅ MIT，无附加限制 | ⚠️ 需商业许可或社区版受限 |
| **模块完整性** | ✅ 96 BSW 模块全覆盖（MCAL 21 + ECUAL 29 + Services 46） | 基础模块需额外购买 |
| **DDS 中间件** | ✅ 内建 OMG DDS v1.4 支持 | ❌ 需第三方集成 |
| **ARXML 工具链** | ✅ 统一 CLI + 完整解析/生成/检查 | 分离式工具链 |
| **配置工具** | ✅ 多工具（CAN/DTC/UDS/DoCAN/Signing）集成 | 部分配置依赖 GUI |
| **目标硬件** | ✅ NXP S32K312 深度适配，持续扩展 | 需定制移植 |
| **社区贡献** | ✅ 开放 PR + 活跃维护 | 厂商主导 |
| **持续集成** | ✅ 4 条 CI 流水线（构建/测试/MISRA/部署） | 需自建 |

> **总结**: yuleASR 在开源透明性、模块完整性、工具链集成度上均优于闭源竞品，特别适合希望自主可控的 Tier-1 和 OEM 团队。

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
sudo apt-get install -y build-essential cmake git python3 python3-pip python3-venv

# 安装 Python 依赖
pip3 install -r tools/arxml/requirements.txt
pip3 install -r tools/arxml-tool/requirements.txt
```

### 获取代码

```bash
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
git submodule update --init --recursive
```

### 构建项目

```bash
# 使用构建脚本（推荐）
./build.sh --platform S32K312

# 或手动构建
mkdir -p build && cd build
cmake .. -DTARGET_PLATFORM=S32K312
make -j$(nproc)

# 运行测试
make test
ctest --output-on-failure
```

### 使用工具链

```bash
# ARXML处理工具（统一CLI）
python3 tools/arxml/arxml_tool.py --help
python3 tools/arxml/arxml_tool.py parse examples/system.arxml
python3 tools/arxml/arxml_tool.py generate config.json -o output/
python3 tools/arxml/arxml_tool.py analyze system.arxml -o report.md

# ARXML工具（独立版）
python3 tools/arxml-tool/arxml-tool.py --help

# CAN配置工具
python3 tools/can_config/can-config-tool.py --dbc examples/example.dbc --output config/

# DTC配置工具
bash tools/dtc_config/dtc-tool.sh --input dtc_config.csv

# RTE代码生成器
python3 tools/rte-generator/rte_generator.py --config config/rte_cfg.json --output src/rte/

# DDS配置工具 (唯一工具链: tools/dds_config, 2026-08-08 P2-2 收敛)
python3 tools/dds_config/dds_config_cli.py --help
# 用法: validate <yaml/json> / generate <yaml/json> -o <outdir> / convert --yaml2arxml <in> <out>
# C 工具链 (根目录 dds-config-tool/): make && make test (工具链详见 dds-config-tool/README)
```

### 项目目录速览

```
yuleASR/
├── src/
│   ├── bsw/                 # AutoSAR BSW 源代码
│   │   ├── mcal/           # 微控制器驱动层 (21模块)
│   │   ├── ecual/          # ECU抽象层 (29模块)
│   │   ├── services/       # 服务层 (46模块)
│   │   └── os/             # 操作系统 (FreeRTOS)
│   ├── asw/                # 应用层 (8组件)
│   ├── rte/                # 运行时环境
│   └── micro-dds/          # DDS中间件
├── config/                 # 配置文件
├── docs/                   # 文档 (150+文档)
├── docs-site/              # 文档站 (Vite + React)
├── tests/                  # 单元/集成测试 (260+)
├── tools/                  # 工具链 (12+工具)
├── examples/               # 示例代码
├── scripts/                # 构建/测试脚本
├── third_party/            # 第三方依赖
├── website/                # 项目官网
├── build.sh                # 一键构建脚本
└── CMakeLists.txt          # CMake构建配置
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

### 在线文档站

<p align="center">
  <a href="https://frisky1985.github.io/yuleASR/">
    <img src="https://img.shields.io/badge/GitHub%20Pages-文档站-2ea44f?style=for-the-badge&logo=githubpages" alt="GitHub Pages">
  </a>
</p>

在线文档站 (GitHub Pages) 托管了完整的项目文档、API参考、模块说明和开发指南：
👉 **<https://frisky1985.github.io/yuleASR/>**

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
