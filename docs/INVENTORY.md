# YuleASR 文档清单

> 本文件列出所有已整理的文档，便于检索和管理。

---

## 1. 架构文档 (architecture/)

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `architecture.md` | 系统架构概览 | docs/architecture.md |
| `architecture-overview.md` | 架构设计概览 | docs/design/architecture-overview.md |
| `modules.md` | 模块总览 | docs/modules.md |
| `OS_README.md` | 操作系统文档 | docs/OS_README.md |

---

## 2. API参考 (api/)

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `api-reference.md` | API参考总览 | docs/api-reference.md |
| `crypto_api_reference.md` | Crypto API参考 | docs/api/crypto_api_reference.md |
| `crypto_quick_start.md` | Crypto快速入门 | docs/api/crypto_quick_start.md |
| `com_api_reference.md` | COM API参考 | docs/com/API_REFERENCE.md |
| `dcm_transfer_services.md` | DCM传输服务 | docs/dcm_transfer_services.md |

---

## 3. 模块文档 (modules/)

### 基础软件模块 (27个)

| 模块 | 文件名 | 说明 |
|:------|:-------|:------|
| BSWM | BSWM.md | 基础软件模式管理器 |
| CANM | CANM.md | CAN管理 |
| CANS | CANSM.md | CAN状态管理 |
| CANTSYN | CANTSYN.md | CAN时间同步 |
| COM | COM.md | 通信模块 |
| COMM | COMM.md | 通信管理 |
| CRYIF | CRYIF.md | 加密接口 |
| CSM | CSM.md | 加密服务管理 |
| DCM | DCM.md | 诊断通信管理 |
| DEM | DEM.md | 诊断事件管理 |
| DLT | DLT.md | 诊断日志与跟踪 |
| DOIP | DOIP.md | 诊断Over IP |
| E2E | E2E.md | End-to-End保护 |
| ECUM | ECUM.md | ECU管理 |
| FIM | FIM.md | 功能禁用管理 |
| J1939TP | J1939TP.md | J1939传输协议 |
| KEYM | KEYM.md | 密钥管理 |
| MEM | MEM.md | 内存管理 |
| NM | NM.md | 网络管理 |
| NVM | NVM.md | 非易失性内存管理 |
| PDUR | PDUR.md | PDU路由器 |
| SCHM | SCHM.md | BSW调度器 |
| SECOC | SECOC.md | 安全Onboard通信 |
| SOAD | SOAD.md | Socket适配器 |
| SOMEIP | SOMEIP.md | SOME/IP协议 |
| SOMEIPTP | SOMEIPTP.md | SOME/IP传输协议 |
| SOMEIPXF | SOMEIPXF.md | SOME/IP变换器 |
| STBM | STBM.md | 同步时间基础模块 |
| WDGM | WDGM.md | 看门狗管理 |
| XCP | XCP.md | 通用测量和校准协议 |

---

## 4. 使用指南 (guides/)

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `development-guide.md` | 开发指南 | docs/development-guide.md |
| `s32k312_hsm_guide.md` | S32K312 HSM使用指南 | docs/guides/s32k312_hsm_guide.md |
| `com_config_guide.md` | COM配置指南 | docs/com/CONFIG_GUIDE.md |
| `com_guide.md` | COM模块概述 | docs/com/README.md |
| `com_troubleshooting.md` | COM故障排除 | docs/com/TROUBLESHOOTING.md |
| `com_user_manual.md` | COM用户手册 | docs/com/USER_MANUAL.md |
| `CONFIGURATOR.md` | 配置器使用说明 | docs/CONFIGURATOR.md |
| `misra_compliance_report.md` | MISRA合规报告 | docs/misra_compliance_report.md |
| `misra_deviations.md` | MISRA偏差说明 | docs/misra_deviations.md |
| `hara_analysis.md` | 危害风险分析 | docs/safety/HARA_ANALYSIS.md |
| `safety_manual.md` | 安全手册 | docs/safety/SAFETY_MANUAL.md |
| `safety_verification_report.md` | 安全验证报告 | docs/safety/VERIFICATION_REPORT.md |
| `dem_design.md` | DEM设计文档 | docs/design/modules/services/dem-design.md |

---

## 5. 设计文档 (docs/design/)

### 系统设计文档

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `architecture-overview.md` | 架构概览 | docs/design/architecture-overview.md |
| `brainstorming-bsw-optimization.md` | BSW优化脑暴 | docs/design/brainstorming-bsw-optimization.md |
| `brainstorming-next-steps.md` | 后续步骤脑暴 | docs/design/brainstorming-next-steps.md |
| `configuration-system.md` | 配置系统设计 | docs/design/configuration-system.md |
| `data-flow.md` | 数据流设计 | docs/design/data-flow.md |
| `error-handling.md` | 错误处理策略 | docs/design/error-handling.md |
| `memory-management.md` | 内存管理设计 | design/memory-management.md |
| `module-interactions.md` | 模块交互设计 | design/module-interactions.md |
| `testing-strategy.md` | 测试策略 | design/testing-strategy.md |

### 模块设计文档 (docs/design/modules/)
| `modules/mcal/adc-design.md` | Adc 设计文档 | docs/design/modules/mcal/adc-design.md |
| `modules/mcal/can-design.md` | Can 设计文档 | docs/design/modules/mcal/can-design.md |
| `modules/mcal/crypto-design.md` | Crypto 设计文档 | docs/design/modules/mcal/crypto-design.md |
| `modules/mcal/dio-design.md` | Dio 设计文档 | docs/design/modules/mcal/dio-design.md |
| `modules/mcal/eep-design.md` | Eep 设计文档 | docs/design/modules/mcal/eep-design.md |
| `modules/mcal/eth-design.md` | Eth 设计文档 | docs/design/modules/mcal/eth-design.md |
| `modules/mcal/fee-design.md` | Fee 设计文档 | docs/design/modules/mcal/fee-design.md |
| `modules/mcal/flash-design.md` | Flash 设计文档 | docs/design/modules/mcal/flash-design.md |
| `modules/mcal/fls-design.md` | Fls 设计文档 | docs/design/modules/mcal/fls-design.md |
| `modules/mcal/gpt-design.md` | Gpt 设计文档 | docs/design/modules/mcal/gpt-design.md |
| `modules/mcal/i2c-design.md` | I2c 设计文档 | docs/design/modules/mcal/i2c-design.md |
| `modules/mcal/icu-design.md` | Icu 设计文档 | docs/design/modules/mcal/icu-design.md |
| `modules/mcal/lin-design.md` | Lin 设计文档 | docs/design/modules/mcal/lin-design.md |
| `modules/mcal/mcu-design.md` | Mcu 设计文档 | docs/design/modules/mcal/mcu-design.md |
| `modules/mcal/ocu-design.md` | Ocu 设计文档 | docs/design/modules/mcal/ocu-design.md |
| `modules/mcal/port-design.md` | Port 设计文档 | docs/design/modules/mcal/port-design.md |
| `modules/mcal/pwm-design.md` | Pwm 设计文档 | docs/design/modules/mcal/pwm-design.md |
| `modules/mcal/ramtst-design.md` | RamTst 设计文档 | docs/design/modules/mcal/ramtst-design.md |
| `modules/mcal/spi-design.md` | Spi 设计文档 | docs/design/modules/mcal/spi-design.md |
| `modules/mcal/uart-design.md` | Uart 设计文档 | docs/design/modules/mcal/uart-design.md |
| `modules/mcal/wdg-design.md` | Wdg 设计文档 | docs/design/modules/mcal/wdg-design.md |

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `modules/README.md` | 模块设计文档总览 | docs/design/modules/README.md |
| `modules/services/com-design.md` | Com 设计文档 | docs/design/modules/services/com-design.md |
| `modules/services/dem-design.md` | Dem 设计文档 | docs/design/modules/services/dem-design.md |
| `modules/ecual/canif-design.md` | CanIf 设计文档 | docs/design/modules/ecual/canif-design.md |
| `modules/services/pdur-design.md` | PduR 设计文档 | docs/design/modules/services/pdur-design.md |
| `modules/ecual/cantp-design.md` | CanTp 设计文档 | docs/design/modules/ecual/cantp-design.md |
| `modules/services/dcm-design.md` | Dcm 设计文档 | docs/design/modules/services/dcm-design.md |
| `modules/services/nvm-design.md` | NvM 设计文档 | docs/design/modules/services/nvm-design.md |
| `modules/services/ecum-design.md` | EcuM 设计文档 | docs/design/modules/services/ecum-design.md |
| `modules/services/bswm-design.md` | BswM 设计文档 | docs/design/modules/services/bswm-design.md |
| `modules/services/secoc-design.md` | SecOC 设计文档 | docs/design/modules/services/secoc-design.md |
| `modules/services/csm-design.md` | Csm 设计文档 | docs/design/modules/services/csm-design.md |
| `modules/services/wdgm-design.md` | WdgM 设计文档 | docs/design/modules/services/wdgm-design.md |

---

## 6. 规范文档 (docs/specs/)

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md` | 改进建议 | docs/AUTOSAR_IMPROVEMENT_RECOMMENDATIONS.md |
| `BSW_Module_Coverage_Report.md` | BSW覆盖率报告 | docs/BSW_Module_Coverage_Report.md |
| `COMPLETE_HSM_INTEGRATION_REPORT.md` | HSM集成报告 | docs/COMPLETE_HSM_INTEGRATION_REPORT.md |
| `completion_report_v1.0.0.md` | v1.0.0完成报告 | docs/completion_report_v1.0.0.md |
| `hsm_integration_report.md` | HSM集成报告 | docs/hsm_integration_report.md |
| `changelog.md` | 变更日志 | docs/changelog.md |
| `2025-04-23-bsw-implementation-phase1.md` | BSW实施计划Phase1 | docs/plans/2025-04-23-bsw-implementation-phase1.md |
| `2025-04-23-education-community-phase.md` | 教育社区阶段计划 | docs/plans/2025-04-23-education-community-phase.md |

---

## 7. 项目报告 (reports/)

### Markdown报告 (30+)

| 文件名 | 说明 |
|:-------|:------|
| `AGENTS.md` | Agent导航文档 |
| `AUTOSAR_COMPLIANCE_REPORT.md` | AutoSAR合规性报告 |
| `CLASSIC_AUTOSAR_GAP_ANALYSIS.md` | 典型AutoSAR差距分析 |
| `CHANGELOG.md` | 项目变更日志 |
| `CMake_MODULES_UPDATE.md` | CMake模块更新 |
| `CODE_REVIEW_REPORT.md` | 代码审查报告 |
| `CODE_REVIEW_SUMMARY.md` | 代码审查总结 |
| `COMPLETION_REPORT.md` | 完成报告 |
| `CONTINUOUS_IMPROVEMENT_FINAL_REPORT.md` | 持续改进报告 |
| `FINAL_PROJECT_STATUS.md` | 最终项目状态 |
| `FINAL_REPORT.md` | 最终报告 |
| `MCAL_ORGANIZATION_REPORT.md` | MCAL组织报告 |
| `MISRA_C2012_Compliance_Report.md` | MISRA C2012合规报告 |
| `OSH_AUTOSAR_IMPLEMENTATION_ROADMAP.md` | OSH实施路线图 |
| `OSH_IMPLEMENTATION_ROADMAP.md` | OSH实施路线图 |
| `PROGRESS.md` | 项目进度 |
| `README.md` | 项目说明 |
| `docs/TODO.md` | 待办事项 |
| `TODO_REMAINING.md` | 剩余待办 |
| `100_PERCENT_TEST_COVERAGE.md` | 100%测试覆盖 |
| `DOCUMENTATION_TEST_COVERAGE_REPORT.md` | 文档测试覆盖报告 |
| `GATE1_Storage_Link_Verification_Report.md` | Gate1存储链验证 |
| `Phase2_Security_Modules_Gate1_Report.md` | Phase2安全模块Gate1 |
| `PROJECT_COMPLETENESS_REPORT.md` | 项目完整性报告 |
| `PHASE1_VERIFICATION_REPORT.md` | Phase1验证报告 |
| `PHASE2_VERIFICATION_REPORT.md` | Phase2验证报告 |
| `asw_verification.md` | ASW验证 |
| `bsw_integration_verification.md` | BSW集成验证 |
| `Com_verification.md` | COM验证 |
| `Dcm_verification.md` | DCM验证 |
| `Dem_verification.md` | DEM验证 |
| `nvm_verification.md` | NVM验证 |
| `os_verification.md` | OS验证 |
| `pdur_verification.md` | PDUR验证 |
| `rte_verification.md` | RTE验证 |
| `service_layer_verification.md` | 服务层验证 |

### JSON报告 (25+)

| 文件名 | 说明 |
|:-------|:------|
| `AUTOSAR_COMPLETENESS_UPDATE_REPORT.json` | 完整性更新报告 |
| `AUTOSAR_FINAL_GAP_ANALYSIS.json` | 最终差距分析 |
| `AUTOSAR_MODULE_COMPLETENESS_REPORT.json` | 模块完整性报告 |
| `AUTOSAR_MODULE_COMPLETENESS_V2.json` | 模块完整性V2 |
| `CLASSIC_AUTOSAR_COVERAGE_ANALYSIS.json` | 覆盖率分析 |
| `CLASSIC_AUTOSAR_MODULE_GAP_ANALYSIS.json` | 模块差距分析 |
| `CONTINUOUS_IMPROVEMENT_ANALYSIS.json` | 改进分析 |
| `CONTINUOUS_IMPROVEMENT_PLAN.json` | 改进计划 |
| `ECUAL_PARALLEL_DEV_FINAL_REPORT.json` | ECUAL并行开发报告 |
| `ECUAL_PARALLEL_DEV_REPORT.json` | ECUAL并行开发 |
| `FINAL_COMPLETENESS_REPORT.json` | 最终完整性报告 |
| `improvements_report.json` | 改进报告 |
| `MISRA_Analysis_Details.json` | MISRA分析详情 |
| `MISSING_MODULES_STATUS.json` | 缺失模块状态 |
| `OSH_IMPLEMENTATION_PLAN.json` | OSH实施计划 |
| `PARALLEL_DEV_SUMMARY.json` | 并行开发总结 |
| `PROJECT_VERIFICATION_REPORT.json` | 项目验证报告 |
| `ROUND1_IMPROVEMENT_REPORT.json` | 第1轮改进 |
| `ROUND2_IMPROVEMENT_REPORT.json` | 第2轮改进 |
| `ROUND3_BRANCH_ANALYSIS.json` | 第3轮分支分析 |
| `ROUND3_IMPROVEMENT_REPORT.json` | 第3轮改进 |
| `ROUND4_IMPROVEMENT_REPORT.json` | 第4轮改进 |
| `static_analysis_report.json` | 静态分析 |
| `static_analysis_report_final.json` | 静态分析最终 |
| `static_analysis_report_v2.json` | 静态分析V2 |

---

## 8. 外部资料 (external/)

| 文件名 | 说明 | 来源 |
|:-------|:------|:------|
| `autosar_articles.md` | 爬取的AutoSAR文章汇编 | docs/autosar_blog_crawled/autosar_articles.md |
| `README.md` | 爬取资料说明 | docs/autosar_blog_crawled/README.md |
| `summary.json` | 爬取文章摘要 | docs/autosar_blog_crawled/summary.json |
| `s32k312-learning/` | S32K312学习资料夹 | docs/s32k312-learning/ |

---

## 文档统计汇总

| 分类 | Markdown | JSON | 合计 |
|:-----|:---------|:-----|:------|
| architecture | 4 | 0 | 4 |
| api | 5 | 0 | 5 |
| modules | 27 | 0 | 27 |
| guides | 15 | 0 | 15 |
| design | 9 | 0 | 9 |
| specs | 8 | 0 | 8 |
| reports | 37 | 25 | 62 |
| external | 2 | 1 | 3+ |
| **总计** | **107** | **26** | **133+** |

---

*最后更新: 2025-05-12*
*维护人: yuleASR Team*
