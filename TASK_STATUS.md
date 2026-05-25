# 项目状态与待办

> 最后更新: 2026-05-26

## ✅ 已完成

### 文档 (Batch 1+2)
- 84篇AUTOSAR模块文档迁移到Docusaurus站 (MCAL 21 + ECUAL 29 + Services 34)
- 44篇安全/平台/API/指南/设计文档迁移
- Docusaurus 3.8.0 → 3.10.1 升级
- 侧边栏支持128篇文档导航

### CI/CD
- 6个workflow全部就绪
- ci.yml CMake路径修复
- deploy-docs.yml切换到website/部署

### RTE P0修复
- Rte_CSOperations.c: 20个Read/Write/Call函数 (700行)
- Rte_AswScheduler.h/c: ASW调度集成 (475行)
- 25个单元测试, 100%通过

### P1修复
- NvM测试: 5/6启用
- ComM: 6状态机 + 唤醒处理
- Csm: KeyGenerate/Derive/Exchange
- Dem: 全局tick + 5处TODO替换

### README/CHANGELOG
- CI徽章、模块表格、文档站链接
- CHANGELOG v1.2.0

## ⏳ 待启动

### CCC Digital Key R2.0 示例项目
- 规划文档: `/home/admin/yuleDKCS/CCC_DIGITAL_KEY_PLAN.md`
- 定位: yuleASR平台的旗舰示例, 基于NXP KW47 + SE050
- Phase 1: 项目重构 (2周)
- Phase 2: 嵌入式核心 (3周)
- Phase 3: 三端联调 (2周)
- Phase 4: 文档与发布 (1周)
