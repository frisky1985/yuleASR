# YuleASR 文档总索引

> **版本**: v2.0  
> **更新日期**: 2026-01-09  
> **文档总数**: 114+ 文件  
> **覆盖模块**: 93个AUTOSAR模块

---

## 📚 文档结构概览

```
docs/
├── README.md                     # 文档入口
├── DOCUMENTATION_INDEX.md        # 本文档 - 总索引
├── ARCHITECTURE.md               # 架构总览
├── MODULE_INDEX.md               # 模块文档索引
├── API_INDEX.md                  # API文档索引
├── GUIDES_INDEX.md               # 开发指南索引
├── REPORTS_INDEX.md              # 报告汇总索引
│
├── modules/                      # 模块文档 (114个)
│   ├── MCAL层 (20个)
│   ├── ECUAL层 (29个)
│   └── Services层 (44个)
│
├── api/                          # API参考文档
├── guides/                       # 开发指南
├── reports/                      # 项目报告
├── architecture/                 # 架构文档
├── design/                       # 设计文档
├── specs/                        # 规范文档
└── safety/                       # 安全文档
```

---

## 🎯 快速导航

| 需求 | 文档位置 |
|:-----|:---------|
| 了解项目架构 | [architecture/architecture-overview.md](architecture/architecture-overview.md) |
| 查看模块API | [api/api-reference.md](api/api-reference.md) |
| 开发指导 | [guides/development-guide.md](guides/development-guide.md) |
| 配置工具 | [guides/CONFIGURATOR.md](guides/CONFIGURATOR.md) |
| 项目进度 | [reports/PROGRESS.md](reports/PROGRESS.md) |
| 测试覆盖 | [reports/COVERAGE_OPTIMIZATION_COMPLETE.md](reports/COVERAGE_OPTIMIZATION_COMPLETE.md) |

---

## 📦 模块文档分类

### MCAL层 (微控制器驱动)

| 模块 | 文档 | 说明 | 测试覆盖 |
|:-----|:-----|:-----|:---------|
| ADC | [modules/adc.md](modules/adc.md) | 模数转换驱动 | ✅ 100% |
| CAN | [modules/can.md](modules/can.md) | CAN控制器驱动 | ✅ 100% |
| CRYPTO | [modules/crypto.md](modules/crypto.md) | 硬件加密驱动 | ✅ 100% |
| DIO | [modules/dio.md](modules/dio.md) | 数字IO驱动 | ✅ 100% |
| EEPROM | [modules/eep.md](modules/eep.md) | EEPROM驱动 | ✅ 100% |
| ETH | [modules/eth.md](modules/eth.md) | 以太网MAC驱动 | ✅ 100% |
| FLASH | [modules/flash.md](modules/flash.md) | Flash存储驱动 | ✅ 100% |
| FLS | [modules/fls.md](modules/fls.md) | Flash接口驱动 | ✅ 100% |
| GPT | [modules/gpt.md](modules/gpt.md) | 通用定时器 | ✅ 100% |
| I2C | [modules/i2c.md](modules/i2c.md) | I2C总线驱动 | ✅ 100% |
| ICU | [modules/icu.md](modules/icu.md) | 输入捕获单元 | ✅ 100% |
| LIN | [modules/lin.md](modules/lin.md) | LIN总线驱动 | ✅ 100% |
| MCU | [modules/mcu.md](modules/mcu.md) | 微控制器单元 | ✅ 100% |
| OCU | [modules/ocu.md](modules/ocu.md) | 输出比较单元 | ✅ 100% |
| PORT | [modules/port.md](modules/port.md) | 端口驱动 | ✅ 100% |
| PWM | [modules/pwm.md](modules/pwm.md) | PWM生成器 | ✅ 100% |
| RAMTST | [modules/ramtst.md](modules/ramtst.md) | RAM测试 | ✅ 100% |
| SPI | [modules/spi.md](modules/spi.md) | SPI总线驱动 | ✅ 100% |
| UART | [modules/uart.md](modules/uart.md) | 串口驱动 | ✅ 100% |
| WDG | [modules/wdg.md](modules/wdg.md) | 看门狗驱动 | ✅ 100% |

### ECUAL层 (ECU抽象层)

| 模块 | 文档 | 说明 | 测试覆盖 |
|:-----|:-----|:-----|:---------|
| CanIf | [modules/canif.md](modules/canif.md) | CAN接口 | ✅ 100% |
| CanNm | [modules/cannm.md](modules/cannm.md) | CAN网络管理 | ✅ 100% |
| CanSm | [modules/cansm.md](modules/cansm.md) | CAN状态管理 | ✅ 100% |
| CanTp | [modules/cantp.md](modules/cantp.md) | CAN传输协议 | ✅ 100% |
| CanTrcv | [modules/cantrcv.md](modules/cantrcv.md) | CAN收发器 | ✅ 100% |
| DLT | [modules/dlt.md](modules/dlt.md) | 诊断日志跟踪 | ✅ 100% |
| DoIP | [modules/doip.md](modules/doip.md) | 诊断 over IP | ✅ 100% |
| EA | [modules/ea.md](modules/ea.md) | EEPROM抽象 | ✅ 100% |
| EthIf | [modules/ethif.md](modules/ethif.md) | 以太网接口 | ✅ 100% |
| EthSm | [modules/ethsm.md](modules/ethsm.md) | 以太网状态管理 | ✅ 100% |
| EthTrcv | [modules/ethtrcv.md](modules/ethtrcv.md) | 以太网收发器 | ✅ 100% |
| Fee | [modules/fee.md](modules/fee.md) | Flash EEPROM仿真 | ✅ 100% |
| FiM | [modules/fim.md](modules/fim.md) | 功能禁止管理 | ✅ 100% |
| FrIf | [modules/frif.md](modules/frif.md) | FlexRay接口 | ✅ 100% |
| FrTp | [modules/frtp.md](modules/frtp.md) | FlexRay传输协议 | ✅ 100% |
| IoHwAb | [modules/iohwab.md](modules/iohwab.md) | IO硬件抽象 | ✅ 100% |
| IpduM | [modules/ipdum.md](modules/ipdum.md) | IPDU复用 | ✅ 100% |
| J1939Tp | [modules/j1939tp.md](modules/j1939tp.md) | J1939传输协议 | ✅ 100% |
| LinIf | [modules/linif.md](modules/linif.md) | LIN接口 | ✅ 100% |
| LinNm | [modules/linnm.md](modules/linnm.md) | LIN网络管理 | ✅ 100% |
| LinSm | [modules/linsm.md](modules/linsm.md) | LIN状态管理 | ✅ 100% |
| LinTp | [modules/lintp.md](modules/lintp.md) | LIN传输协议 | ✅ 100% |
| LinTrcv | [modules/lintrcv.md](modules/lintrcv.md) | LIN收发器 | ✅ 100% |
| MemIf | [modules/memif.md](modules/memif.md) | 存储器接口 | ✅ 100% |
| SoAdIf | [modules/someipif.md](modules/someipif.md) | SoAd接口 | ✅ 100% |
| SoAdSd | [modules/someipsd.md](modules/someipsd.md) | SoAd服务发现 | ✅ 100% |
| SRP | [modules/srp.md](modules/srp.md) | 流预留协议 | ✅ 100% |
| WdgIf | [modules/wdgif.md](modules/wdgif.md) | 看门狗接口 | ✅ 100% |
| XCP | [modules/xcp.md](modules/xcp.md) | 通用测量标定 | ✅ 100% |

### Services层 (服务层)

| 模块 | 文档 | 说明 | 测试覆盖 |
|:-----|:-----|:-----|:---------|
| BswM | [modules/bswm.md](modules/bswm.md) | BSW模式管理 | ✅ 100% |
| CanM | [modules/canm.md](modules/canm.md) | CAN管理 | ✅ 100% |
| CanSm | [modules/cansm.md](modules/cansm.md) | CAN状态管理 | ✅ 100% |
| CanTSyn | [modules/cantsyn.md](modules/cantsyn.md) | CAN时间同步 | ✅ 100% |
| Com | [modules/com.md](modules/com.md) | 通信服务 | ✅ 100% |
| ComM | [modules/comm.md](modules/comm.md) | 通信管理 | ✅ 100% |
| CRC | [modules/crc.md](modules/crc.md) | CRC计算 | ✅ 100% |
| CryIf | [modules/cryif.md](modules/cryif.md) | 加密接口 | ✅ 100% |
| CSM | [modules/csm.md](modules/csm.md) | 加密服务管理 | ✅ 100% |
| DCM | [modules/dcm.md](modules/dcm.md) | 诊断通信管理 | ✅ 100% |
| DEM | [modules/dem.md](modules/dem.md) | 诊断事件管理 | ✅ 100% |
| DET | [modules/det.md](modules/det.md) | 默认错误跟踪 | ✅ 100% |
| DLT | [modules/dlt.md](modules/dlt.md) | 诊断日志 | ✅ 100% |
| DoCan | [modules/docan.md](modules/docan.md) | CAN诊断 | ✅ 100% |
| DoIP | [modules/doip.md](modules/doip.md) | IP诊断 | ✅ 100% |
| E2E | [modules/e2e.md](modules/e2e.md) | 端到端保护 | ✅ 100% |
| EcuC | [modules/ecuc.md](modules/ecuc.md) | ECU配置 | ✅ 100% |
| EcuM | [modules/ecum.md](modules/ecum.md) | ECU状态管理 | ✅ 100% |
| EthSM | [modules/ethsm.md](modules/ethsm.md) | 以太网状态管理 | ✅ 100% |
| FiM | [modules/fim.md](modules/fim.md) | 功能禁止管理 | ✅ 100% |
| IpduM | [modules/ipdum.md](modules/ipdum.md) | IPDU复用 | ✅ 100% |
| J1939Nm | [modules/j1939nm.md](modules/j1939nm.md) | J1939网络管理 | ✅ 100% |
| KeyM | [modules/keym.md](modules/keym.md) | 密钥管理 | ✅ 100% |
| LinM | [modules/linm.md](modules/linm.md) | LIN管理 | ✅ 100% |
| LinSM | [modules/linsm.md](modules/linsm.md) | LIN状态管理 | ✅ 100% |
| LnTM | [modules/lntm.md](modules/lntm.md) | LIN时间调度 | ✅ 100% |
| Mem | [modules/mem.md](modules/mem.md) | 存储服务 | ✅ 100% |
| MemIf | [modules/memif.md](modules/memif.md) | 存储接口 | ✅ 100% |
| MQTT | [modules/mqtt.md](modules/mqtt.md) | MQTT协议 | ✅ 100% |
| NM | [modules/nm.md](modules/nm.md) | 网络管理 | ✅ 100% |
| NvM | [modules/nvm.md](modules/nvm.md) | 非易失存储 | ✅ 100% |
| PduR | [modules/pdur.md](modules/pdur.md) | PDU路由 | ✅ 100% |
| RamSafety | [modules/ramsafety.md](modules/ramsafety.md) | RAM安全 | ✅ 100% |
| SchM | [modules/schm.md](modules/schm.md) | BSW调度 | ✅ 100% |
| SecOC | [modules/secoc.md](modules/secoc.md) | 安全通信 | ✅ 100% |
| SoAd | [modules/soad.md](modules/soad.md) | Socket适配 | ✅ 100% |
| SomeIP | [modules/someip.md](modules/someip.md) | SOME/IP协议 | ✅ 100% |
| SomeIPTp | [modules/someiptp.md](modules/someiptp.md) | SOME/IP传输 | ✅ 100% |
| SomeIPXF | [modules/someipxf.md](modules/someipxf.md) | SOME/IP转换 | ✅ 100% |
| StbM | [modules/stbm.md](modules/stbm.md) | 同步时基 | ✅ 100% |
| SWC | [modules/swc.md](modules/swc.md) | 软件组件 | ✅ 100% |
| UdpNm | [modules/udpnm.md](modules/udpnm.md) | UDP网络管理 | ✅ 100% |
| WdgM | [modules/wdgm.md](modules/wdgm.md) | 看门狗管理 | ✅ 100% |
| XCP | [modules/xcp.md](modules/xcp.md) | XCP协议 | ✅ 100% |

---

## 🔧 API参考文档

### 核心API文档

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| API总览 | 全模块API索引 | [api/api-reference.md](api/api-reference.md) |
| COM API | 通信服务API详述 | [api/com_api_reference.md](api/com_api_reference.md) |
| 加密API | 密码学服务API | [api/crypto_api_reference.md](api/crypto_api_reference.md) |
| 加密快速入门 | 加密服务使用指南 | [api/crypto_quick_start.md](api/crypto_quick_start.md) |
| DCM传输服务 | 诊断传输服务 | [api/dcm_transfer_services.md](api/dcm_transfer_services.md) |

---

## 📖 开发指南

### 核心指南

| 指南 | 说明 | 路径 |
|:-----|:-----|:-----|
| 开发指南 | 完整开发手册 | [guides/development-guide.md](guides/development-guide.md) |
| 配置工具 | BSW配置器使用 | [guides/CONFIGURATOR.md](guides/CONFIGURATOR.md) |
| COM配置 | COM模块配置 | [guides/com_config_guide.md](guides/com_config_guide.md) |
| COM手册 | COM使用手册 | [guides/com_user_manual.md](guides/com_user_manual.md) |
| COM故障排查 | COM问题诊断 | [guides/com_troubleshooting.md](guides/com_troubleshooting.md) |
| DEM设计 | DEM模块设计 | [guides/dem_design.md](guides/dem_design.md) |
| HSM集成 | 硬件安全模块 | [guides/s32k312_hsm_guide.md](guides/s32k312_hsm_guide.md) |
| S32K312学习 | MCU学习资料 | [external/s32k312-learning/README.md](external/s32k312-learning/README.md) |

### 规范与合规

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| MISRA合规 | MISRA C 2012合规报告 | [guides/misra_compliance_report.md](guides/misra_compliance_report.md) |
| MISRA偏差 | 允许的偏差说明 | [guides/misra_deviations.md](guides/misra_deviations.md) |
| 安全手册 | 功能安全指南 | [safety/SAFETY_MANUAL.md](safety/SAFETY_MANUAL.md) |
| HARA分析 | 危害分析与风险评估 | [safety/HARA_ANALYSIS.md](safety/HARA_ANALYSIS.md) |

---

## 📊 项目报告汇总

### 进度与状态报告

| 报告 | 说明 | 路径 |
|:-----|:-----|:-----|
| 覆盖优化完成 | 测试与文档覆盖优化 | [reports/COVERAGE_OPTIMIZATION_COMPLETE.md](reports/COVERAGE_OPTIMIZATION_COMPLETE.md) |
| 项目进度 | 总体进度追踪 | [reports/PROGRESS.md](reports/PROGRESS.md) |
| 最终报告 | 项目完成报告 | [reports/FINAL_REPORT.md](reports/FINAL_REPORT.md) |
| 完成度报告 | 模块完成度统计 | [reports/PROJECT_COMPLETENESS_REPORT.md](reports/PROJECT_COMPLETENESS_REPORT.md) |
| 变更日志 | 版本变更记录 | [reports/CHANGELOG.md](reports/CHANGELOG.md) |

### 验证报告

| 报告 | 说明 | 路径 |
|:-----|:-----|:-----|
| COM验证 | COM模块验证 | [reports/Com_verification.md](reports/Com_verification.md) |
| DCM验证 | DCM模块验证 | [reports/Dcm_verification.md](reports/Dcm_verification.md) |
| DEM验证 | DEM模块验证 | [reports/Dem_verification.md](reports/Dem_verification.md) |
| NVM验证 | NVM模块验证 | [reports/nvm_verification.md](reports/nvm_verification.md) |
| OS验证 | OS模块验证 | [reports/os_verification.md](reports/os_verification.md) |
| PduR验证 | PduR模块验证 | [reports/pdur_verification.md](reports/pdur_verification.md) |
| RTE验证 | RTE模块验证 | [reports/rte_verification.md](reports/rte_verification.md) |
| ASW验证 | 应用层验证 | [reports/asw_verification.md](reports/asw_verification.md) |

### 架构与设计报告

| 报告 | 说明 | 路径 |
|:-----|:-----|:-----|
| 架构总览 | 系统架构说明 | [architecture/architecture-overview.md](architecture/architecture-overview.md) |
| 模块架构 | 模块架构详述 | [architecture/modules.md](architecture/modules.md) |
| BSW优化 | BSW优化建议 | [specs/AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md](specs/AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md) |
| HSM集成 | 硬件安全集成 | [specs/COMPLETE_HSM_INTEGRATION_REPORT.md](specs/COMPLETE_HSM_INTEGRATION_REPORT.md) |
| 覆盖率报告 | BSW模块覆盖 | [specs/BSW_Module_Coverage_Report.md](specs/BSW_Module_Coverage_Report.md) |

---

## 🏗️ 架构文档

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| 架构概览 | 系统架构说明 | [architecture/architecture-overview.md](architecture/architecture-overview.md) |
| 详细架构 | 架构设计详述 | [architecture/architecture.md](architecture/architecture.md) |
| 模块架构 | 模块架构图 | [architecture/modules.md](architecture/modules.md) |
| OS架构 | 操作系统架构 | [architecture/OS_README.md](architecture/OS_README.md) |

---

## 📝 设计文档

### 系统设计文档

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| 架构概览 | 设计架构概览 | [design/architecture-overview.md](design/architecture-overview.md) |
| BSW优化 | BSW优化讨论 | [design/brainstorming-bsw-optimization.md](design/brainstorming-bsw-optimization.md) |
| 配置系统 | 配置系统设计 | [design/configuration-system.md](design/configuration-system.md) |
| 数据流 | 数据流设计 | [design/data-flow.md](design/data-flow.md) |
| 错误处理 | 错误处理设计 | [design/error-handling.md](design/error-handling.md) |
| 内存管理 | 内存管理设计 | [design/memory-management.md](design/memory-management.md) |
| 模块交互 | 模块交互设计 | [design/module-interactions.md](design/module-interactions.md) |
| 测试策略 | 测试设计策略 | [design/testing-strategy.md](design/testing-strategy.md) |

### 模块设计文档

| 模块 | 层级 | 文档 | 状态 |
|:-----|:-----|:-----|:-----|
| Com | Services | [design/modules/services/com-design.md](design/modules/services/com-design.md) | 已完成 |
| ADC | MCAL | [design/modules/mcal/adc-design.md](design/modules/mcal/adc-design.md) | 已完成 |
| CAN | MCAL | [design/modules/mcal/can-design.md](design/modules/mcal/can-design.md) | 已完成 |
| CRYPTO | MCAL | [design/modules/mcal/crypto-design.md](design/modules/mcal/crypto-design.md) | 已完成 |
| DIO | MCAL | [design/modules/mcal/dio-design.md](design/modules/mcal/dio-design.md) | 已完成 |
| EEP | MCAL | [design/modules/mcal/eep-design.md](design/modules/mcal/eep-design.md) | 已完成 |
| ETH | MCAL | [design/modules/mcal/eth-design.md](design/modules/mcal/eth-design.md) | 已完成 |
| FEE | MCAL | [design/modules/mcal/fee-design.md](design/modules/mcal/fee-design.md) | 已完成 |
| FLASH | MCAL | [design/modules/mcal/flash-design.md](design/modules/mcal/flash-design.md) | 已完成 |
| FLS | MCAL | [design/modules/mcal/fls-design.md](design/modules/mcal/fls-design.md) | 已完成 |
| GPT | MCAL | [design/modules/mcal/gpt-design.md](design/modules/mcal/gpt-design.md) | 已完成 |
| I2C | MCAL | [design/modules/mcal/i2c-design.md](design/modules/mcal/i2c-design.md) | 已完成 |
| ICU | MCAL | [design/modules/mcal/icu-design.md](design/modules/mcal/icu-design.md) | 已完成 |
| LIN | MCAL | [design/modules/mcal/lin-design.md](design/modules/mcal/lin-design.md) | 已完成 |
| MCU | MCAL | [design/modules/mcal/mcu-design.md](design/modules/mcal/mcu-design.md) | 已完成 |
| OCU | MCAL | [design/modules/mcal/ocu-design.md](design/modules/mcal/ocu-design.md) | 已完成 |
| PORT | MCAL | [design/modules/mcal/port-design.md](design/modules/mcal/port-design.md) | 已完成 |
| PWM | MCAL | [design/modules/mcal/pwm-design.md](design/modules/mcal/pwm-design.md) | 已完成 |
| RAMTST | MCAL | [design/modules/mcal/ramtst-design.md](design/modules/mcal/ramtst-design.md) | 已完成 |
| SPI | MCAL | [design/modules/mcal/spi-design.md](design/modules/mcal/spi-design.md) | 已完成 |
| UART | MCAL | [design/modules/mcal/uart-design.md](design/modules/mcal/uart-design.md) | 已完成 |
| WDG | MCAL | [design/modules/mcal/wdg-design.md](design/modules/mcal/wdg-design.md) | 已完成 |
| Dem | Services | [design/modules/services/dem-design.md](design/modules/services/dem-design.md) | 已完成 |
| CanIf | ECUAL | [design/modules/ecual/canif-design.md](design/modules/ecual/canif-design.md) | 已完成 |
| PduR | Services | [design/modules/services/pdur-design.md](design/modules/services/pdur-design.md) | 已完成 |
| CanTp | ECUAL | [design/modules/ecual/cantp-design.md](design/modules/ecual/cantp-design.md) | 已完成 |
| Dcm | Services | [design/modules/services/dcm-design.md](design/modules/services/dcm-design.md) | 已完成 |
| NvM | Services | [design/modules/services/nvm-design.md](design/modules/services/nvm-design.md) | 已完成 |
| EcuM | Services | [design/modules/services/ecum-design.md](design/modules/services/ecum-design.md) | 已完成 |
| BswM | Services | [design/modules/services/bswm-design.md](design/modules/services/bswm-design.md) | 已完成 |
| SecOC | Services | [design/modules/services/secoc-design.md](design/modules/services/secoc-design.md) | 已完成 |
| Csm | Services | [design/modules/services/csm-design.md](design/modules/services/csm-design.md) | 已完成 |
| WdgM | Services | [design/modules/services/wdgm-design.md](design/modules/services/wdgm-design.md) | 已完成 |

完整清单见 [design/modules/README.md](design/modules/README.md)。

---

## 🔒 安全文档

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| 安全手册 | 功能安全指南 | [safety/SAFETY_MANUAL.md](safety/SAFETY_MANUAL.md) |
| HARA分析 | 危害分析 | [safety/HARA_ANALYSIS.md](safety/HARA_ANALYSIS.md) |
| 验证报告 | 安全验证报告 | [safety/VERIFICATION_REPORT.md](safety/VERIFICATION_REPORT.md) |

---

## 📋 规范与标准

| 文档 | 说明 | 路径 |
|:-----|:-----|:-----|
| 实施计划 | BSW实施计划 | [specs/2025-04-23-bsw-implementation-phase1.md](specs/2025-04-23-bsw-implementation-phase1.md) |
| 教育社区 | 教育社区计划 | [specs/2025-04-23-education-community-phase.md](specs/2025-04-23-education-community-phase.md) |
| 优化建议 | AUTOSAR优化 | [specs/AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md](specs/AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md) |
| 模块覆盖 | 模块覆盖报告 | [specs/BSW_Module_Coverage_Report.md](specs/BSW_Module_Coverage_Report.md) |
| HSM集成 | HSM集成报告 | [specs/COMPLETE_HSM_INTEGRATION_REPORT.md](specs/COMPLETE_HSM_INTEGRATION_REPORT.md) |
| 完成报告 | v1.0.0完成 | [specs/completion_report_v1.0.0.md](specs/completion_report_v1.0.0.md) |

---

## 📈 统计信息

### 文档覆盖统计

| 类别 | 数量 | 覆盖率 |
|:-----|:-----|:-------|
| MCAL模块文档 | 20 | 100% |
| ECUAL模块文档 | 29 | 100% |
| Services模块文档 | 44 | 100% |
| API参考文档 | 4 | - |
| 开发指南 | 10+ | - |
| 项目报告 | 30+ | - |
| **总计** | **140+** | **100%** |

### 测试覆盖统计

| 层级 | 模块数 | 测试文件 | 覆盖率 |
|:-----|:-------|:---------|:-------|
| MCAL | 20 | 22 | 100% |
| ECUAL | 29 | 32 | 100% |
| Services | 44 | 53 | 100% |
| **总计** | **93** | **107+** | **100%** |

---

## 🔍 搜索索引

### 按关键词搜索

| 关键词 | 相关文档 |
|:-----|:---------|
| CAN | CAN, CanIf, CanNm, CanSm, CanTp, CanTrcv, CanM |
| LIN | LIN, LinIf, LinNm, LinSm, LinTp, LinTrcv, LinM |
| ETH | ETH, EthIf, EthSm, EthTrcv |
| 诊断 | DCM, DEM, DET, DLT, DoIP, DoCan |
| 存储 | NvM, Mem, Fee, Ea, EEPROM, Flash |
| 安全 | CRYPTO, CSM, CryIf, SecOC, KeyM, HSM |
| 网络 | NM, CanNm, LinNm, UdpNm, J1939Nm |
| 时间 | GPT, ICU, OCU, PWM, StbM, CanTSyn |

---

## 📞 维护信息

- **文档维护**: YuleTech团队
- **最后更新**: 2026-01-09
- **文档版本**: v2.0
- **项目仓库**: https://github.com/frisky1985/yuleASR

---

*本文档由自动化工具生成并维护*  
*如有问题请联系开发团队*
