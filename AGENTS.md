# 予乐科技 (YuleTech) - 基础软件平台 Agent 导航

> **基于 AutoSAR 标准的开源基础软件、工具链及社区平台**

## 快速入口

| 命令 | 用途 | 阶段 |
|:-----|:-----|:-----|
| `/triple-init` | 初始化工程环境 | Stage 0 |
| `/triple-explore` | 需求探索分析 | Stage 1 |
| `/triple-new "描述"` | 创建新变更 | Stage 2 |
| `/triple-dev` | 开发执行 | Stage 3 |
| `/triple-verify` | 验证审查 | Stage 4 |
| `/triple-archive` | 归档合并 | Stage 5 |
| `/triple-health` | 健康检查 | Stage 6 |

## 项目愿景

**成为工程师的合作伙伴**，通过提供基于 AutoSAR 标准的开源基础软件、便捷的开发工具链和活跃的技术社区，降低汽车基础软件开发门槛，为中国汽车软件产业赋能。

## 目录结构

```
yuletech-openspec/
├── AGENTS.md                 # 本文件 - Agent 导航入口
├── README.md                 # 项目说明
│
├── openspec/                 # OpenSpec: 规范真相源
│   ├── specs/                # 当前系统行为规范
│   │   ├── bsw/              # 基础软件规范
│   │   ├── toolchain/        # 工具链规范
│   │   ├── platform/         # 平台服务规范
│   │   └── community/        # 社区生态规范
│   └── changes/              # 待处理变更
│       └── [change-name]/    # 每个变更一个文件夹
│
├── design/                   # Superpowers: 设计文档
│   ├── active/               # 当前活跃设计
│   └── archived/             # 已归档设计
│
├── plans/                    # Superpowers: 实施计划
│   ├── current-tasks.md      # 当前任务
│   └── roadmap.md            # 路线图
│
├── src/                      # 源代码
│   ├── bsw/                  # 基础软件源码
│   ├── toolchain/            # 工具链源码
│   ├── platform/             # 平台服务源码
│   └── community/            # 社区平台源码
│
├── tests/                    # 测试
│   ├── unit/                 # 单元测试
│   ├── integration/          # 集成测试
│   └── scenarios/            # OpenSpec 场景测试
│
└── .harness/                 # Harness: 约束配置
    ├── lint-rules/           # 自定义规则
    ├── architecture-rules.md # 架构规则
    └── quality-gates.yml     # 质量门禁
```

## 核心产品体系

| 产品体系 | 核心产品 | 技术栈 |
|:---------|:---------|:-------|
| **Yule BSW** | 基础软件包 (MCAL/ECU/服务层) | C/AutoSAR |
| **Yule Toolchain** | 配置工具/代码生成器/编译工具 | Python/Vue.js |
| **Yule Platform** | 开源社区/学习平台/插件市场 | Spring Boot/Vue.js |

## 工作流

### 标准开发流程

```
1. 创建变更: /triple-new "添加诊断服务模块"
   ↓
2. 规范定义: openspec/changes/[change]/specs/
   ↓
3. 设计文档: openspec/changes/[change]/design.md
   ↓
4. 任务拆解: openspec/changes/[change]/tasks.md
   ↓
5. TDD 开发: /triple-dev
   ↓
6. 验证审查: /triple-verify
   ↓
7. 归档合并: /triple-archive
```

### 质量门禁

#### Stage 2 (规范定义) 门禁
- [ ] proposal 清晰完整
- [ ] specs 覆盖所有 scenarios
- [ ] design 符合架构约束
- [ ] tasks 可执行、粒度合适

#### Stage 3 (开发) 门禁
- [ ] TDD: 红-绿-重构
- [ ] 所有 scenarios 通过
- [ ] linter 通过
- [ ] 复杂度达标 (圈复杂度 < 10)

#### Stage 4 (验证) 门禁
- [ ] OpenSpec: specs 与实现一致
- [ ] Superpowers: 设计符合度 100%
- [ ] Harness: 质量扫描通过
- [ ] 测试覆盖率 > 80%

#### Stage 5 (归档) 门禁
- [ ] 人类审查通过
- [ ] 架构方向一致
- [ ] 无未解决问题

## 架构约束

### 分层架构 (Harness)

```
┌─────────────────────────────────────┐
│  UI Layer (Vue.js/React)            │
├─────────────────────────────────────┤
│  Service Layer (业务逻辑)            │
├─────────────────────────────────────┤
│  Repository Layer (数据访问)         │
├─────────────────────────────────────┤
│  Config Layer (配置管理)             │
├─────────────────────────────────────┤
│  Types Layer (类型定义)              │
└─────────────────────────────────────┘
```

### 依赖规则
- 上层可依赖下层
- 禁止循环依赖
- 禁止跨层调用

### 代码规范
- 所有代码必须通过 ESLint/Prettier
- 函数圈复杂度 < 10
- 单元测试覆盖率 > 80%
- 所有公共函数必须有 JSDoc 注释

## 当前活跃变更

查看 `openspec/changes/` 目录获取当前活跃变更列表。

## 项目进度

### 已完成模块
- **MCAL 层**: 9个驱动全部完成 ✓
  - Mcu, Port, Dio, Can, Spi, Gpt, Pwm, Adc, Wdg
- **ECUAL 层**: 9个模块全部完成 ✓
  - CanIf, IoHwAb, CanTp, EthIf, MemIf, Fee, Ea, FrIf, LinIf
- **Service 层**: 5个模块全部完成 ✓
  - Com, PduR, NvM, Dcm, Dem
- **RTE 层**: 全部完成 ✓
  - Rte (核心), Rte_ComInterface, Rte_NvMInterface, Rte_Scheduler
- **ASW 层**: 8个组件全部完成 ✓
  - EngineControl, VehicleDynamics, DiagnosticManager, CommunicationManager
  - StorageManager, IOControl, ModeManager, WatchdogManager
- **OS 层**: 全部完成 ✓
  - Os (基于 FreeRTOS V10.6.x / V11.x)
  - 任务管理、事件管理、资源管理、报警管理、中断管理
  - Os_Cfg.c 任务配置表、弱引用钩子函数

- **Integration 层**: 已完成 ✓
  - BswM (基础软件模式管理器)
  - EcuM (ECU状态管理器)

- **ConfigGenerator 集成**: 已完成 ✓
  - MCAL: Mcu, Port, Can, Spi, Gpt 配置模板
  - BSW: Com, PduR, NvM, CanIf, IoHwAb 配置模板

### 验证状态
- PduR: ✅ 验证通过 (TxConfirmation修复, TriggerTransmit新增)
- NvM: ✅ 验证通过 (CRC验证, 写计数器, 缺失API补齐)
- Com: ✅ 实现完成 (MainFunctionRx/Tx完善, 网关支持)
- Dcm: ✅ 实现完成 (DTC子功能增强, Dem集成)
- Dem: ✅ 实现完成 (冻结帧存储, DTCSetting控制)
- RTE: ✅ 验证通过 (组件初始化, 事件处理完善)
- ASW: ✅ 验证通过 (8/8 组件)
- BswM: ✅ 新增完成
- EcuM: ✅ 新增完成

### 项目里程碑
- ✅ MCAL 层完成 (9/9)
- ✅ ECUAL 层完成 (9/9)
- ✅ Service 层完成 (5/5)
- ✅ RTE 层完成 (1/1)
- ✅ ASW 层完成 (8/8)
- ✅ OS 层完善 (弱引用钩子 + 配置表)
- ✅ Integration 层完成 (2/2)
- 🎉 **AutoSAR 全栈开发完成 (34个模块/组件)**

### 项目统计
| 层级 | 模块/组件数 | 状态 |
|:-----|:-----------|:-----|
| MCAL | 9 | ✅ 完成 |
| ECUAL | 9 | ✅ 完成 |
| Service | 5 | ✅ 完成 |
| RTE | 1 | ✅ 完成 |
| ASW | 8 | ✅ 完成 |
| OS | 1 | ✅ 完成 |
| Integration | 2 | ✅ 完成 |
| **总计** | **35** | **✅ 完成** |

### 下一步工作
- 系统集成测试
- 性能优化
- 文档完善
- 发布准备

## 技术栈

| 层级 | 技术选型 |
|:-----|:---------|
| 前端 | Vue 3 + TypeScript + Element Plus |
| 后端 | Spring Boot 3 + Java 17 |
| 数据库 | MySQL 8.0 + Redis 7 |
| 代码生成 | Python + Jinja2 |
| 容器化 | Docker + Kubernetes |

## 联系方式

- 公司: 上海予乐电子科技有限公司
- 项目: YuleTech AutoSAR BSW Platform
- 文档版本: v1.0
