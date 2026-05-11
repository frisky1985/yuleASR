# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-04-29

### Added
- **MCAL 层新驱动 (3个)**
  - Eth (以太网驱动, Module ID: 0x53) - 支持 10/100/1000 Mbps 以太网 MAC 操作
  - Icu (输入捕获驱动, Module ID: 0x10) - 支持边沿检测、时间戳、信号测量和边沿计数
  - Ocu (输出比较驱动, Module ID: 0x7A) - 支持绝对/相对阈值设置和引脚动作控制
- **ECUAL 层新模块 (1个)**
  - FrTp (FlexRay 传输协议, Module ID: 0x2D) - 支持 ISO TP 分段传输协议
- **完善的车载网络支持**
  - CAN (Can, CanIf, CanTp) - 已完成
  - FlexRay (FrIf, FrTp) - 已完成
  - Ethernet (Eth, EthIf) - 已完成
  - LIN (LinIf) - 已完成
- **MISRA C:2012 合规性验证**
  - 所有新模块通过 MISRA C:2012 规范检查
  - 高达 98% 以上的代码覆盖率

### Technical Details

#### Eth (以太网驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 10/100/1000 Mbps 操作速率
- 提供 MII/RMII 接口支持
- 支持 MAC 地址过滤器配置
- 支持发送/接收中断处理

#### Icu (输入捕获驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 4 种测量模式:
  - 信号边沿检测 (Signal Edge Detection)
  - 信号测量 (Signal Measurement)
  - 时间戳 (Timestamp)
  - 边沿计数 (Edge Counter)
- 支持唤醒功能 (Wakeup)

#### Ocu (输出比较驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 4 种引脚动作:
  - SET_HIGH - 比较匹配时置高
  - SET_LOW - 比较匹配时置低
  - TOGGLE - 比较匹配时翻转
  - HOLD - 保持当前状态
- 支持绝对和相对阈值设置
- ASIL-D 安全等级兼容

#### FrTp (FlexRay 传输协议)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 ISO TP 分段传输:
  - 单帧 (Single Frame, SF)
  - 首帧 (First Frame, FF)
  - 连续帧 (Consecutive Frame, CF)
  - 流量控制 (Flow Control, FC)
- 支持多连接管理
- 可配置的超时管理 (N_As, N_Bs, N_Cs, N_Ar, N_Br, N_Cr)

### Changed
- 更新项目统计: 模块总数从 32 个增加到 36 个
- MCAL 层驱动从 9 个增加到 12 个
- ECUAL 层模块从 9 个增加到 10 个

### Project Statistics
| 层级 | 模块数 | 状态 |
|:-----|:-------|:-----|
| MCAL | 12 | ✅ 完成 |
| ECUAL | 10 | ✅ 完成 |
| Service | 5 | ✅ 完成 |
| RTE | 1 | ✅ 完成 |
| ASW | 8 | ✅ 完成 |
| **总计** | **36** | **✅ 完成** |

## [1.0.0] - 2026-04-23

### Added
- 完整MCAL层驱动 (ADC, CAN, DIO, GPT, MCU, PORT, PWM, SPI, WDG)
- ECUAL层框架 (CanIf, CanTp, Ea, Fee)
- Services层框架 (COM, DCM, DEM, NVM)
- 130+单元测试用例
- 5层Mock系统
- Python构建系统
- Docusaurus文档站
- GitHub Actions CI/CD
- 初始发布
- Basic BSW structure
- Core MCAL drivers
- Test framework foundation
