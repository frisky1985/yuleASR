# 本地main与远程master分支合并策略

## 现状分析

| 维度 | main分支 (本地) | master分支 (远程) |
|------|----------------|------------------|
| **核心定位** | 功能开发主干 | 工程基础设施重构 |
| **代码量** | ~500K行 (实际功能代码) | ~220K行 (精简架构) |
| **主要优势** | 完整BSW/DDS/安全/诊断实现 | Harness工程体系/CI/文档 |
| **FreeRTOS** | 完整内核源码 | 已移除 |
| **AUTOSAR模块** | EthIf/SoAd/PduR/UDS/SecOC/OTA/DLT完整实现 | 精简版本 |
| **工具链** | DDS配置工具/GUI/分析器 | RTE生成器/配置GUI |

## 推荐方案：选择性融合 (Cherry-Pick Strategy)

### 理由
1. **main分支不可替代** - 包含您8个阶段开发的所有功能代码
2. **master分支不可直接合并** - 删除了大量实际需要的代码(FreeRTOS、TSN、Safety等)
3. **选择性吸收** - 只取master的工程改进，保留main的功能代码

### 实施步骤

#### Phase 1: 引入Harness工程体系 (高优先级)
```bash
# 从master分支复制关键文件
git checkout origin/master -- .harness/
git checkout origin/master -- .github/workflows/ci.yml
git checkout origin/master -- AGENTS.md
git checkout origin/master -- docs/development-guide.md
```

**收益**:
- 获得完整的Agent开发工作流
- 质量门禁体系 (quality-gates.yml)
- 架构规则检查

#### Phase 2: 整合测试框架改进 (中优先级)
```bash
# 保留main的功能测试，添加master的测试工具
git checkout origin/master -- tools/analysis/
git checkout origin/master -- tests/run_tests.sh
git checkout origin/master -- tests/run_tests.ps1
```

**收益**:
- 静态分析工具改进
- 跨平台测试脚本

#### Phase 3: 文档系统升级 (低优先级)
```bash
# 选择性合并文档改进
git checkout origin/master -- docs/architecture/
git checkout origin/master -- docs/api-reference.md
```

**收益**:
- 更完善的架构文档
- API参考手册

#### Phase 4: 工具链对比选择 (按需)
| 工具 | main版本 | master版本 | 建议 |
|------|---------|-----------|------|
| DDS配置工具 | dds_config/ (完整) | dds_config/ (简化) | **保留main** |
| RTE生成器 | 无 | rte_generator/ | **引入master** |
| 代码生成器 | codegen/ | generator/ | 对比后选择 |
| 配置GUI | config_tool/ | config/gui/ | 对比后选择 |

### 不采纳的master变更

以下变更**不应**合并到main:

1. **删除FreeRTOS** - main需要FreeRTOS内核
2. **删除Safety模块** - ECC/SafeRAM是功能安全必需的
3. **删除TSN模块** - 时间敏感网络是项目核心
4. **删除完整DDS实现** - 保留main的完整DDS中间件
5. **删除SoAd/PduR实现** - BSW以太网栈必须保留

### 具体操作命令

```bash
# 1. 确保在main分支
git checkout main

# 2. 创建特性分支进行整合
git checkout -b integrate-master-improvements

# 3. 选择性检出master的改进文件
git checkout origin/master -- .harness/
git checkout origin/master -- .github/workflows/ci.yml
git checkout origin/master -- .github/workflows/deploy-docs.yml
git checkout origin/master -- AGENTS.md
git checkout origin/master -- PROGRESS.md
git checkout origin/master -- TODO.md

# 4. 移除与main冲突的文件（保留main版本）
git reset HEAD tools/config_tool/  # 保留main的完整版本
git reset HEAD tools/dds_config/   # 保留main的完整版本

# 5. 提交整合结果
git add .
git commit -m "chore: 整合master分支的Harness工程体系和CI改进

- 添加.harness/工程质量体系
- 更新CI/CD工作流配置
- 保留main分支所有功能代码
- 保留FreeRTOS完整内核
- 保留BSW/DDS/安全/诊断完整实现"

# 6. 推送并创建PR
git push origin integrate-master-improvements
```

### 合并后的预期结构

```
yuleASR/
├── .harness/               # 新增: Harness工程体系
├── .github/workflows/      # 更新: 精简CI配置
├── docs/                   # 补充: 文档改进
├── src/
│   ├── autosar/           # 保留: 完整BSW实现
│   ├── dds/               # 保留: 完整DDS中间件
│   ├── diagnostics/       # 保留: UDS诊断栈
│   ├── crypto_stack/      # 保留: SecOC安全栈
│   ├── ota/               # 保留: OTA更新模块
│   ├── dlt/               # 保留: DLT完整实现
│   ├── safety/            # 保留: 功能安全模块
│   ├── tsn/               # 保留: TSN时间敏感网络
│   └── platform/
│       └── freertos/
│           └── kernel/     # 保留: 完整FreeRTOS内核
├── tools/
│   ├── dds_config/        # 保留: 完整DDS配置工具
│   ├── config_tool/       # 保留: GUI配置工具
│   └── analysis/          # 新增: 静态分析工具
└── tests/                 # 补充: 测试脚本改进
```

### 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| 文件冲突 | 中 | 中 | 选择性检出，避免自动合并 |
| 功能丢失 | 低 | 高 | 明确保留main的所有功能目录 |
| 构建中断 | 中 | 中 | 合并后立即运行完整构建测试 |
| 文档不一致 | 高 | 低 | 逐步更新文档，保持功能优先 |

### 建议执行顺序

1. **今天**: 执行Phase 1 (Harness体系)
2. **本周**: 执行Phase 2 (测试框架)
3. **下周**: 评估Phase 3 (文档)和Phase 4 (工具链)
4. **持续**: main分支继续功能开发，定期同步master的基础设施改进

### 最终建议

**不要**执行 `git merge master`，这会导致：
- FreeRTOS被删除
- TSN模块被删除
- Safety模块被删除
- 大量功能代码丢失

**应该**采用上述选择性融合策略，保留main的功能代码，只吸收master的工程改进。

---

**下一步操作**: 请确认此策略，我将立即执行Phase 1的整合。
