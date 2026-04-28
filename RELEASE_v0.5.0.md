# v0.5.0 发布说明

**发布日期:** 2025年4月28日  
**版本号:** v0.5.0  
**代码名称:** 分支整合与功能增强 (Branch Integration & Feature Enhancement)

---

## 概述

v0.5.0是ETH-DDS Integration项目的重要里程碑版本。本版本成功将master分支的核心优势整合到main分支，形成了兼具两分支特色的完整解决方案。

### 主要亮点

- ✅ 完善的OpenSpec文档规范系统（48个文件）
- ✅ 8个完整的ASW应用层组件
- ✅ Python版RTE代码生成器
- ✅ 完善的验证报告模板
- ✅ React文档网站
- ✅ 双分支并行开发计划

---

## 新增功能

### 1. OpenSpec文档规范系统

**目录:** `openspec/`

从master分支完整移植的规范文档系统，包含：

- **changes/** - 原始规范变更文档（26个文件）
- **modules/** - 按模块整理的规范（20个文件）

**支持的模块:**
| 模块 | 说明 |
|------|------|
| Com | 通信模块 |
| Dcm | 诊断通信管理 |
| Dem | 诊断事件管理 |
| DoCan | CAN诊断协议 |
| DoIp | IP诊断协议 |
| NvM | 非易失性存储 |
| PduR | 路由模块 |
| RTE | 运行时环境生成器 |

### 2. ASW应用层软件组件

**位置:** `src/autosar/asw/`

8个完整实现的SWC组件：

| 组件名 | 文件 | 功能描述 |
|--------|------|----------|
| CommunicationManager | `Swc_CommunicationManager.c/h` | 通信管理 |
| DiagnosticManager | `Swc_DiagnosticManager.c/h` | 诊断管理 |
| EngineControl | `Swc_EngineControl.c/h` | 发动机控制 |
| IOControl | `Swc_IOControl.c/h` | IO控制 |
| ModeManager | `Swc_ModeManager.c/h` | 模式管理 |
| StorageManager | `Swc_StorageManager.c/h` | 存储管理 |
| VehicleDynamics | `Swc_VehicleDynamics.c/h` | 车辆动力学 |
| WatchdogManager | `Swc_WatchdogManager.c/h` | 看门狗管理 |

**接口定义:** `asw_interfaces.h` - ASW层统一接口定义

### 3. RTE代码生成器

**位置:** `tools/rte_generator/`

- **rte_generator.py** - Python主程序（27KB）
- **example_config.json** - 配置示例
- **README.md** - 使用文档

**CMake支持:** `cmake/modules/rte_generator.cmake`

### 4. 验证报告模板

**位置:** `verification/`

完整的验证报告模板（7份）：

| 报告 | 说明 |
|------|------|
| asw_verification.md | ASW应用层验证 |
| bsw_integration_verification.md | BSW集成验证 |
| nvm_verification.md | NvM模块验证 |
| os_verification.md | OS操作系统验证 |
| pdur_verification.md | PduR路由验证 |
| rte_verification.md | RTE运行时环境验证 |
| service_layer_verification.md | 服务层验证 |

### 5. 文档网站

**技术栈:** React + Vite + TypeScript + Tailwind CSS

**位置:** `docs-site/`

**功能特性:**
- 模块文档和API参考
- 客户端搜索功能
- 响应式设计
- 多页面导航

---

## 文档更新

### 新增文档

1. **docs/BRANCH_INTEGRATION_REPORT.md**
   - 完整的分支整合报告
   - 移植内容清单
   - 统计数据

2. **docs/PARALLEL_DEVELOPMENT_PLAN.md**
   - 双分支并行开发策略
   - 每周同步会合机制
   - 开发路线图

### 更新文档

- **CHANGELOG.md** - 添加v0.5.0版本说明

---

## 双分支并行开发策略

从本版本开始，项目采用双分支并行开发模式：

### main分支
**定位:** ETH-DDS集成中间件  
**重点:** ETH协议栈、DDS配置工具、TSN、汽车级功能安全

### master分支
**定位:** Classic AUTOSAR标准平台  
**重点:** 标准BSW模块、RTE生成器、ASW组件库

### 同步机制
- **每周会合:** 进度汇报、技术分享、同步计划
- **每月发布:** 合并到release分支，打标签

---

## 统计数据

### 代码统计

| 指标 | 数值 |
|------|------|
| 总文件数 | ~1000+ |
| 源代码文件 | ~700+ |
| 头文件 | ~100+ |
| 文档文件 | ~150+ |

### 版本历史

```
v0.1.0 → v0.2.0 → v0.3.0 → v0.4.0 → v0.5.0 (当前)
```

---

## 安装与使用

### 快速开始

```bash
# 克隆仓库
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR

# 检出 v0.5.0 版本
git checkout v0.5.0

# 构建项目
mkdir build && cd build
cmake ..
make -j

# 运行测试
ctest
```

### 文档网站

```bash
cd docs-site
npm install
npm run dev
```

---

## 致谢

感谢所有为本版本做出贡献的开发者！

特别感谢：
- 多Agent并行移植系统
- 所有贡献者的代码提交

---

## 参考链接

- [项目主页](https://github.com/frisky1985/yuleASR)
- [CHANGELOG](CHANGELOG.md)
- [分支整合报告](docs/BRANCH_INTEGRATION_REPORT.md)
- [并行开发计划](docs/PARALLEL_DEVELOPMENT_PLAN.md)

---

*发布于 2025年4月28日*  
*版本 v0.5.0*
