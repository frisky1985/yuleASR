# YuleTech AutoSAR BSW Platform

<p align="center">
  <strong>基于 AutoSAR Classic Platform 4.x 标准的开源基础软件平台</strong>
</p>

<p align="center">
  <a href="https://github.com/frisky1985/yuleASR/stargazers"><img src="https://img.shields.io/github/stars/frisky1985/yuleASR?style=flat-square&logo=github&color=yellow" alt="Stars"></a>
  <a href="https://github.com/frisky1985/yuleASR/network/members"><img src="https://img.shields.io/github/forks/frisky1985/yuleASR?style=flat-square&logo=github&color=blue" alt="Forks"></a>
  <a href="https://github.com/frisky1985/yuleASR/issues"><img src="https://img.shields.io/github/issues/frisky1985/yuleASR?style=flat-square&logo=github&color=red" alt="Issues"></a>
  <a href="https://github.com/frisky1985/yuleASR/blob/master/LICENSE"><img src="https://img.shields.io/github/license/frisky1985/yuleASR?style=flat-square&color=green" alt="License"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/AutoSAR-4.x-blue?style=flat-square&logo=automotive" alt="AutoSAR 4.x">
  <img src="https://img.shields.io/badge/C-99-blue?style=flat-square&logo=c" alt="C99">
  <img src="https://img.shields.io/badge/Platform-i.MX8M%20Mini-orange?style=flat-square&logo=nxp" alt="i.MX8M Mini">
  <img src="https://img.shields.io/badge/Status-Production%20Ready-success?style=flat-square" alt="Status">
</p>

<p align="center">
  <a href="#overview">概览</a> •
  <a href="#features">特性</a> •
  <a href="#quick-start">快速开始</a> •
  <a href="#documentation">文档</a> •
  <a href="#contributing">贡献</a>
</p>

---

## 概览

YuleTech AutoSAR BSW Platform 是 **上海予乐电子科技有限公司** 开发的开源汽车基础软件平台，为 NXP i.MX8M Mini 处理器提供完整的 AutoSAR Classic Platform 4.x 基础软件栈实现。

### 项目愿景

成为工程师的合作伙伴，通过提供基于 AutoSAR 标准的开源基础软件、便捷的开发工具链和活跃的技术社区，降低汽车基础软件开发门槛，为中国汽车软件产业赋能。

### 支持的硬件平台

- **NXP i.MX8M Mini** (主要目标平台)
- ARM Cortex-A53 四核处理器
- 支持 CAN、Ethernet、LIN 等车载网络

## 特性

### 完整的 BSW 分层架构

```
┌─────────────────────────────────────────┐
│           RTE (Runtime Environment)      │
│    - 组件间通信接口                      │
│    - 数据类型定义                        │
│    - 调度器                              │
├─────────────────────────────────────────┤
│           Service Layer                  │
│    - Com (通信服务)                      │
│    - PduR (PDU 路由器)                   │
│    - NvM (NVRAM 管理器)                  │
│    - Dcm (诊断通信管理器)                │
│    - Dem (诊断事件管理器)                │
├─────────────────────────────────────────┤
│           ECUAL Layer                    │
│    - CanIf, CanTp, EthIf, FrIf, LinIf   │
│    - IoHwAb, MemIf, Fee, Ea             │
├─────────────────────────────────────────┤
│           MCAL Layer                     │
│    - Mcu, Port, Dio, Can, Spi           │
│    - Gpt, Pwm, Adc, Wdg                 │
├─────────────────────────────────────────┤
│           Hardware (i.MX8M Mini)         │
└─────────────────────────────────────────┘
```

### 核心功能

- **✅ 完整的 MCAL 层** - 9 个驱动全部实现
- **✅ 完整的 ECUAL 层** - 9 个模块全部实现
- **✅ 完整的服务层** - 5 个模块全部实现
- **✅ 完整的 RTE 层** - 运行时环境完整实现
- **✅ 符合 AutoSAR 4.x 标准** - 严格遵循 AutoSAR 规范
- **✅ 完整的错误检测** - DET (Development Error Tracer) 支持
- **✅ 内存分区管理** - MemMap 内存分区支持

### 项目统计

<p align="center">
  <img src="https://img.shields.io/badge/Modules-24-blue?style=flat-square" alt="24 Modules">
  <img src="https://img.shields.io/badge/Lines%20of%20Code-32K+-blue?style=flat-square" alt="32K+ LOC">
  <img src="https://img.shields.io/badge/Verification%20Reports-3-success?style=flat-square" alt="3 Verification Reports">
  <img src="https://img.shields.io/badge/Documentation-5%20Docs-success?style=flat-square" alt="5 Documentation">
</p>

## 快速开始

### 环境要求

- **操作系统**: Windows 10/11, Linux (Ubuntu 20.04+), macOS (12+)
- **编译器**: GCC ARM Embedded 10.3.1+ 或 IAR Embedded Workbench 9.20+
- **构建工具**: CMake 3.20+
- **Python**: 3.9+ (用于代码生成工具)

### 获取代码

```bash
git clone https://github.com/yuletech/autosar-bsw-platform.git
cd autosar-bsw-platform
```

### 构建项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc-toolchain.cmake

# 构建
make -j$(nproc)
```

### 运行测试

```bash
# 运行单元测试
make test

# 生成测试报告
make test-report
```

## 文档

### 项目文档

| 文档 | 描述 |
|:-----|:-----|
| [架构文档](docs/architecture.md) | 系统架构设计说明 |
| [API 参考](docs/api-reference.md) | 完整 API 接口文档 |
| [开发指南](docs/development-guide.md) | 开发规范和指南 |
| [模块清单](docs/modules.md) | 所有模块详细说明 |
| [版本历史](docs/changelog.md) | 版本更新记录 |

### 模块文档

- [MCAL 驱动文档](docs/mcal/README.md)
- [ECUAL 模块文档](docs/ecual/README.md)
- [Service 层文档](docs/services/README.md)
- [RTE 层文档](docs/rte/README.md)

## 项目结构

```
yuletech-openspec/
├── AGENTS.md                 # Agent 导航入口
├── README.md                 # 本文件
├── LICENSE                   # 许可证
│
├── openspec/                 # OpenSpec: 规范真相源
│   ├── specs/                # 系统行为规范
│   │   ├── mcal/            # MCAL 层规范
│   │   ├── ecual/           # ECUAL 层规范
│   │   ├── services/        # Service 层规范
│   │   └── rte/             # RTE 层规范
│   ├── changes/             # 待处理变更
│   └── archived/            # 已归档变更
│
├── design/                   # 设计文档
│   ├── bsw-architecture.md  # BSW 架构设计
│   └── service-layer-implementation.md
│
├── plans/                    # 实施计划
│   └── service-layer-implementation.md
│
├── src/                      # 源代码
│   └── bsw/
│       ├── mcal/            # MCAL 实现 (9 驱动)
│       ├── ecual/           # ECUAL 实现 (9 模块)
│       ├── services/        # Service 层实现 (5 模块)
│       ├── rte/             # RTE 实现
│       └── common/          # 通用头文件
│
├── verification/             # 验证报告
│   ├── pdur_verification.md
│   ├── nvm_verification.md
│   └── rte_verification.md
│
├── .harness/                 # Harness: 约束配置
│   └── autosar-bsw-development.md
│
└── docs/                     # 项目文档
    ├── architecture.md
    ├── api-reference.md
    ├── development-guide.md
    ├── modules.md
    └── changelog.md
```

## 模块清单

### MCAL 层 (9/9)

| 模块 | 描述 | 状态 |
|:-----|:-----|:----:|
| Mcu | 微控制器驱动 | ✅ |
| Port | 端口驱动 | ✅ |
| Dio | 数字 I/O 驱动 | ✅ |
| Can | CAN 驱动 | ✅ |
| Spi | SPI 驱动 | ✅ |
| Gpt | 通用定时器驱动 | ✅ |
| Pwm | PWM 驱动 | ✅ |
| Adc | ADC 驱动 | ✅ |
| Wdg | 看门狗驱动 | ✅ |

### ECUAL 层 (9/9)

| 模块 | 描述 | 状态 |
|:-----|:-----|:----:|
| CanIf | CAN 接口 | ✅ |
| IoHwAb | I/O 硬件抽象 | ✅ |
| CanTp | CAN 传输协议 | ✅ |
| EthIf | 以太网接口 | ✅ |
| MemIf | 存储器接口 | ✅ |
| Fee | Flash EEPROM 仿真 | ✅ |
| Ea | EEPROM 抽象 | ✅ |
| FrIf | FlexRay 接口 | ✅ |
| LinIf | LIN 接口 | ✅ |

### Service 层 (5/5)

| 模块 | 描述 | 状态 |
|:-----|:-----|:----:|
| Com | 通信服务 | ✅ |
| PduR | PDU 路由器 | ✅ |
| NvM | NVRAM 管理器 | ✅ |
| Dcm | 诊断通信管理器 | ✅ |
| Dem | 诊断事件管理器 | ✅ |

### RTE 层 (1/1)

| 模块 | 描述 | 状态 |
|:-----|:-----|:----:|
| Rte | 运行时环境 | ✅ |

## 开发指南

### 开发流程

我们采用 **OpenSpec + Superpowers + Harness Engineering** 开发方法论：

1. **需求探索** (`/triple-explore`) - 理解需求和约束
2. **创建变更** (`/triple-new`) - 创建新的变更提案
3. **开发执行** (`/triple-dev`) - TDD 红-绿-重构
4. **验证审查** (`/triple-verify`) - 验证实现符合规范
5. **归档合并** (`/triple-archive`) - 合并到主分支

### 代码规范

- 遵循 AutoSAR 命名规范
- 使用 MemMap.h 进行内存分区
- 所有模块支持 DET 错误检测
- 函数圈复杂度 < 10
- 代码注释完整

### 贡献指南

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交变更 (`git commit -m 'Add amazing feature'`)
4. 推送分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 许可证

本项目采用 **MIT 许可证** - 详见 [LICENSE](LICENSE) 文件

## 联系方式

- **公司**: 上海予乐电子科技有限公司
- **邮箱**: contact@yuletech.com
- **网站**: https://www.yuletech.com
- **GitHub**: https://github.com/yuletech

## 致谢

感谢所有为本项目做出贡献的开发者和社区成员。

---

<p align="center">
  <sub>Built with ❤️ by Shanghai Yule Electronics Technology Co., Ltd.</sub>
</p>
