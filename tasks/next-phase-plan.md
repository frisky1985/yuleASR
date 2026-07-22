# yuleASR 下一阶段推进计划

> 日期: 2026-07-22
> 当前基线: CI L1/L2/L3 全绿 | MISRA 0R/0A | Coverage CI 就绪 | ASPICE 18/18 BP | 证据链 30 件

## 老板核心要求（已记住）

1. **拆成子任务并行推进** — 小克和小马并行动作
2. **Loop Chaining** — 发现的问题自己修，不要反复问，最终给报告
3. **零打扰模式** — 中间不问，最后出总结报告
4. **Zero P0/P1** — 不允许遗留阻塞问题到下一阶段
5. **每阶段完成后质量审查** — 小马审查通过后才进入下一阶段
6. **量产思维** — 不是 Demo，按量产要求做

---

## 任务拆解

### Track A：覆盖率实战（小克 👨‍💻）
**目标**：让 gcov 行级覆盖率真正跑起来，获取第一份真实基线

- A1: 修复嵌入式头文件查找路径（`Std_Types.h`, `MemMap.h` 等）
- A2: 配置 CMake native 构建通过编译（mock 掉依赖的硬件头文件或提供 stub）
- A3: 运行 `coverage` target 获取真实行级覆盖率
- A4: 根据基线调整 `ci-config.yaml` 覆盖阈值
- A5: 提交全部变更 → git commit + push

### Track B：已变更提交 + 缺陷闭环（小克 👨‍💻）
**目标**：整理所有未提交的 MISRA 豁免/覆盖率工具/证据文件，统一提交

- B1: 审查 git status 脏文件清单，分类（MISRA / coverage / evidence / docs）
- B2: 确保所有变更都有对应的 spec-delta 记录
- B3: 完整 CI 验证（`yuleosh ci run 3`）确保提交前全绿
- B4: 统一提交推送

### Track C：ISO 26262 功能安全基础（小克 👨‍💻）
**目标**：建立功能安全基线文档

- C1: 创建 `docs/safety/` 目录和 SOC（Safety Concept）结构
- C2: HARA 分析 — Item Definition → Hazard Identification → Safety Goals
- C3: FSR（功能安全需求）草稿 — 对应 yuleASR BSW 模块
- C4: ASIL 等级分配 — 每个 BSW 模块标注 ASIL(A/B/C/D)

### Track D：质量审查 — v1.3.0 完整评估（小马 🐴）
**目标**：对目前 v1.3.0 状态做完整质量评估

- D1: 审查 CI 全绿状态验证
- D2: 审查 MISRA 豁免合理性（33条豁免是否有无根据的？）
- D3: 审查 ASPICE 证据完整性
- D4: 审查 SHALL 需求追溯矩阵完整性
- D5: 审查覆盖率基线（至少确认管道就绪）
- D6: 输出质量评估报告（评分 + 阻塞项 + 建议）

### Track E：Spec 与文档审计（小马 🐴）
**目标**：确保所有文档和规范一致，无矛盾/过时

- E1: 审计 specs/ 下 4 个 spec 的版本一致性
- E2: 审计 docs/ 下所有文档的时效性
- E3: 检查 SHALL ID 枚举在 spec 与测试间的一致性
- E4: 输出审计报告

### Track F：量产就绪检查（小马 🐴）
**目标**：按量产标准评估当前 yuleASR 成熟度

- F1: MCU 选型确认（S32K312）
- F2: ASIL 等级策略
- F3: BSW 模块完整度
- F4: 工具链就绪度（CMake/编译器/gcov/lcov/cppcheck/MISRA）
- F5: 文档/测试/构建 三件套评估
- F6: 输出量产就绪度报告

---

## 执行顺序

```
第一波并行:
  └── 小克: Track A (覆盖率实战) — 核心阻断
  └── 小克: Track B (提交+闭环) — 清理积压
  └── 小马: Track D (质量审查) — 基线评估

第二波（依赖第一波结果）:
  └── 小马: Track E (文档审计) — 需要 Track B 提交后最新状态
  └── 小马: Track F (量产检查) — 需要 Track D 审查结果
  └── 小克: Track C (ISO 26262) — 非阻塞，可并行
```
