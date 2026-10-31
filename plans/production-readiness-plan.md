# yuleASR 量产准备任务拆解（v1）

> 原则：**所有结论必须可由命令复现**。文档/注释/自述不作为证据；无法复现的项进入“待验证清单”，不得写成结论。
> 目标：把当前“骨架完整、证据未闭环”的状态，逐层收敛到可量产（QM 基线，ASIL-B 轨按决策启用）。

---

## 1. 事实基线（已实测）

| # | 指标 | 实测值 | 证据来源 |
|:--|:--|:--|:--|
| F1 | 模块实现 | 103 个实现目录 / 97 个唯一名（大小写不敏感 94）；6 组同名重复 + 3 组大小写撞名（doIP/doip、ethSm/ethsm、linSM/linsm） | `python3 tools/generate_module_matrix.py` |
| F2 | 配置模板 | 92 个；仅 Boot/Cdd 无模板。**30 个错层已修正为“模板跟随实现层”**（21 个已移动，9 个因同名双实现/撞名待 PR-1.3 定 canonical） | `reports/baseline/module-matrix.csv` |
| F3 | 行/分支覆盖率 | **63.5% / 51.0%**（295 文件） | `.yuleosh/reports/c-coverage.json` |
| F4 | 覆盖率分布 | **24 个模块 0%**，21 个 0<x<80%，55 个 ≥80% | 按模块聚合 c-coverage.json |
| F5 | MISRA | total 1546，**required 58**，deviated 1488，402 文件 | `.yuleosh/tools/misra_full_scan.py --gate` |
| F6 | MISRA gate 阈值 | `max_required=58`、`max_total=1547`（**按现状棘轮，而非目标值**） | [.yuleosh/ci-config.yaml](file:///workspace/.yuleosh/ci-config.yaml) `misra.gate` |
| F7 | 覆盖率 gate | `coverage.enabled=False`；阈值 line 60 / branch 50 / module 35 | 同上 `coverage` 段 |
| F8 | 单元测试 | 最近全量 ctest 有 **3 项失败** | `ctest --test-dir build-native --output-on-failure` |
| F9 | 追溯 | 127 条需求，126 有代码，127 有测试，**评审全部为自动化证据评审** | [traceability-report.json](file:///workspace/.yuleosh/evidence-bundle/traceability/traceability-report.json) |
| F10 | 安全案例一致性 | 宣称语句 91.57%/分支 79.79%/函数 95.7%，实测 63.5%/51.0%；同文档又自称“分支 0.0%、仅 1 文件测量” | [safety-case.md](file:///workspace/docs/safety/safety-case.md#L453) vs F3 |
| F11 | 配置工具可运行性 | CLI 模块缺失；pytest 12 个 collection error（缺 fastapi，属可选 extra） | [pyproject.toml](file:///workspace/tools/yuleasr-configurator/pyproject.toml#L14) |
| F12 | OS/平台 | FreeRTOS V10.6/V11 封装；MCAL 仅 S32K312 | [Os.c](file:///workspace/src/bsw/os/src/Os.c#L5) |
| F13 | 平台缺陷 | Fls 写/擦 `BSY` 忙等被注释；`Fls_Read` 同步阻塞；Fee GC 占位；`FEE_ENTER/EXIT_CRITICAL_SECTION` 空宏；Cdd FvM 默认 RAM 后端 | [Fls.c](file:///workspace/src/bsw/mcal/fls/src/Fls.c)、[Fee.c](file:///workspace/src/bsw/ecual/fee/src/Fee.c) |

### 待验证清单（当前无证据，禁止下结论）

- MCAL 在目标板（S32K312 EVB/HIL）上的实际行为与电气时序
- ARXML 导入/导出往返一致性
- RTE 生成代码在真实 SWC 集成下可用性
- NvM→MemIf→Fee→Fls 端到端（掉电、坏块、GC 期间读）
- OS 时序保护/MPU/栈监控是否存在及其覆盖
- 各模块 API 与 AUTOSAR R22-11 SWS 逐条差异（尚未做 SWS 对照）
- E2E/SecOC/CryIf 等安全机制的实际有效性与密钥生命周期
- 测试断言有效性（是否存在“空断言/恒真断言”）

---

## 2. 分层任务（L0→L7，顺序执行）

### L0 可信度量与证据链（一切的前提）

| ID | 任务 | 验收（命令级） |
|:--|:--|:--|
| PR-0.1 | 新增 `tools/capture_baseline.sh`：一次输出 sha、工具版本、build、ctest、coverage、misra、trace 七件套到 `reports/baseline/`，任一失败即非零退出 | 连续两次运行产物除时间戳外一致 |
| PR-0.2 | 生成**模块关联矩阵** `reports/module-matrix.csv`：module/layer/canonical/模板/测试文件/覆盖率/MISRA required/需求 ID/状态；全部字段来自机器源，禁止手工填 | 94 个唯一模块全覆盖，无空行 |
| PR-0.3 | gate 接 CI 并 fail-closed：`coverage.enabled=True`；`check_coverage_gate.py`、`misra_full_scan.py --gate`、`check_traceability_gate.py --require-review` 进流水线 | 空报告/缺字段/证据过期 → 退出非零 |
| PR-0.4 | 证据字段规范化：`reviewer_type: human|automated`、`evidence_sha`、`tool_version`；gate 输出 human 评审比例 | 报告可区分两类评审 |
| PR-0.5 | 阈值双轨：保留 ratchet（现状）同时登记 target（line 80 / branch 60 / required 0 / 模块≥80），未达标标红不阻塞，逐月收紧 ratchet | target 值写入 ci-config 且可查询 |

**G0 出口**：任一指标都能从一条命令重建，且与提交 sha 绑定；矩阵覆盖全部模块。

### L1 架构收敛（去重 + 分层正确）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-1.1 | 建立 `module-manifest.yaml`：94 模块的 layer、canonical 路径、配置来源、负责人 | 每个模块恰好一条 canonical |
| PR-1.2 | SWS 对照表：对照 AUTOSAR R22-11 BSW 模块清单，人工确认层归属与模块集合（脚本只做比对，判定需复核） | 差异清单签字确认 |
| PR-1.3 | 消解 6 组重复：`memif`（AUTOSAR 属 ECUAL，需从 services 迁移 canonical）、`xcp`、`ipdum`、`fim`、`j1939tp`、`ramtst`；非 canonical 侧改 comment-only stub 或删除 | 同名目录=0；CMake 无双目标 |
| PR-1.4 | 分层依赖检查器：解析 `#include` 与 CMake target 依赖，禁止跨层直连（services→mcal、ecual→services） | 违规=0，且发现现有违规清单 |

**G1 出口**：0 重复、0 分层违规、manifest 与 CMake 一一对应。

### L2 配置工具闭环（配置→代码）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-2.1 | 工具环境固化：依赖锁定；`python3 -m yuleasr.cli --help` 在干净环境可用；pytest 全绿 | 干净 venv 一次安装可跑 |
| PR-2.2 | 补齐 30 个缺失模板，优先级：NvM、PduR、BswM、EcuM、MemIf、Dem、Dcm、Com、CanNm、ComM、Xcp、TcpIp；模板与 manifest 一一映射 | 缺失=0（按 manifest 判定） |
| PR-2.3 | ARXML→配置→生成代码往返用例；生成代码纳入编译与单测回归 | 往返 diff=0；生成物编译通过 |
| PR-2.4 | 生成物边界规则（Cfg/Lcfg/MemMap/SchM/手写区），防止覆盖手写代码 | 规则文档 + 生成前后 diff 检查 |

**G2 出口**：对量产子集模块，一条命令完成“配置生成→编译→测试”。

### L3 静态质量（MISRA required 清零）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-3.1 | 导出 58 条 required 明细到 `reports/misra-required.csv`（文件/规则/行号）并按层分桶 | 58 条全部有归属，可复现 |
| PR-3.2 | 分批清除 required：MCAL → ECUAL → Services → OS/RTE；每批后把 `max_required` 按实际值收紧 | `max_required=0` 时 gate PASS |
| PR-3.3 | 1488 条 deviation 审计：每条需 pattern + 理由 + 责任人 + 到期（≤180 天），过期/失效即 fail | 无过期、无空理由条目 |
| PR-3.4 | 收紧 `max_total`，advisory 分类治理；复杂度/重复代码/死代码纳入扫描 | 阈值逐批下降且可追溯 |

**G3 出口**：required=0、偏离清单全部有效且到期可控。

### L4 动态质量（测试与覆盖率）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-4.1 | 复现并修复 3 个失败测试（`Safety_Stack_UnitTest`、`test_dds1_tbf`、`Safety_Manager_UnitTest`） | ctest 全绿 |
| PR-4.2 | 24 个 0% 模块接入覆盖率（每模块至少：API 契约、主路径、错误路径） | 0% 模块=0 |
| PR-4.3 | 覆盖率达到 line 80 / branch 60，模块下限 35→80 渐进收紧 | gate 以 target 阈值通过 |
| PR-4.4 | 测试有效性审计：断言数统计 + 抽查恒真断言；关键模块引入变异测试（可选） | 抽查报告，无“空断言”模块 |
| PR-4.5 | ASIL 轨（需 D1 决策）：安全相关模块 MC/DC ≥90%（工具支持时） | 工具报告 |

**G4 出口**：全绿 + target 覆盖率达标（QM 轨）/ MC/DC 达标（ASIL 轨）。

### L5 平台与硬件（MCAL/OS/Fls/Fee）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-5.1 | Fls 异步化：恢复 `BSY` 正确处理（中断或状态轮询）、超时与错误上报；去除任何忙等 | 无 `while(BSY)` 忙等；超时用例通过 |
| PR-5.2 | Fee GC 实现（当前占位）+ 掉电一致性 + 临界区真实实现（当前空宏） | GC/掉电/并发用例通过 |
| PR-5.3 | Cdd FvM Flash 后端启用并测试（当前默认 RAM 后端） | Flash 后端用例通过 |
| PR-5.4 | OS 路线决策（需 D2）：FreeRTOS 封装 vs 切 AUTOSAR OS；补齐时序保护/MPU/栈监控或明确限定使用场景 | 决策记录 + 对应实现/限制清单 |
| PR-5.5 | MCAL 目标板 HIL/SIL 用例集与 WCET/最坏时延预算 | 目标板证据 + 时序报告 |

**G5 出口**：目标板证据可复现；存储栈无忙等、GC 可用；OS 限制明示。

### L6 集成链路（端到端）

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-6.1 | NvM→MemIf→Fee→Fls：写/读/掉电/坏块/GC 期间读 | 端到端用例通过 |
| PR-6.2 | Com/PduR/CanIf/CanTp：超时、网关、缓冲溢出 | 用例通过 |
| PR-6.3 | Dcm/Dem：UDS 服务、DTC 冻结帧、事件恢复 | 用例通过 |
| PR-6.4 | EcuM/BswM：启动、休眠、唤醒、模式切换 | 用例通过 |
| PR-6.5 | 故障注入（掉电、总线断开、Flash 写保护） | 注入矩阵通过 |

**G6 出口**：关键链路端到端 + 故障注入证据齐全。

### L7 过程与安全证据

| ID | 任务 | 验收 |
|:--|:--|:--|
| PR-7.1 | 127 条需求人工评审签署（自动化结果仅作辅助），gate `--require-review` 以 human 为准 | 127/127 human 签署 |
| PR-7.2 | 安全案例所有数字改为直接引用 CI artifact（消除 91.57% vs 63.5% 冲突） | 每个数字可溯源到报告文件+sha |
| PR-7.3 | 过程证据：需求—设计—代码—测试—评审闭环；可重复构建 | 构建两次产物一致（可复现） |
| PR-7.4 | 发布包：版本号、已知限制、支持芯片/配置矩阵 | 发布清单完整 |

**G7 出口**：安全案例与 CI 数据一致且人工签署；可复现发布包。

---

## 3. 优先级与执行顺序

| 优先级 | 范围 | 理由 |
|:--|:--|:--|
| **P0** | PR-0.1~0.3、PR-1.3、PR-3.1~3.2、PR-4.1、PR-5.1~5.2 | 先让“度量可信”，再清 required 与存储栈硬缺陷 |
| **P1** | PR-1.1~1.2/1.4、PR-2.1~2.2、PR-4.2~4.3、PR-5.3 | 架构/工具闭环与覆盖率主体 |
| **P2** | PR-2.3~2.4、PR-4.4、PR-5.4~5.5、PR-6.*、PR-7.* | 端到端、HIL、过程证据 |

并行建议：L3（MISRA）与 L4（测试补强）可双轨；L2 工具链独立推进；L5 需硬件资源。

---

## 4. 决策点（阻塞对应任务，需业务确认）

| ID | 决策 | 状态 | 阻塞 |
|:--|:--|:--|:--|
| D1 | 目标安全等级：QM / ASIL-B / ASIL-D；目标 ECU 与 MCU | **已定：ASIL-D**（MCU 待定） | L4.5、L5.4~5.5、L7 |
| D2 | OS 路线：FreeRTOS 封装 或 认证 AUTOSAR OS | **已定：FreeRTOS** | L5.4 |
| D3 | 模块范围：全量维护 或 裁剪到项目子集 | 待定 | L1、L2、L4 范围 |

### 决策冲突登记（不得静默处理）

- **RISK-01（阻断 ASIL-D 认证）**：原生 FreeRTOS 无 ISO 26262 认证、无 AUTOSAR OS 的时序保护/内存保护/栈监控语义；仅靠应用层封装无法满足 ASIL-D 的 OS 独立性要求。FreeRTOS 路线下 ASIL-D 只能通过“第三方认证版 FreeRTOS + 安全手册 + 使用假设限制”成立，或最终降级目标等级。该风险进入安全案例的 Known Limitations，不得以“已封装”表述掩盖。
- **RISK-02**：MCAL 当前仅 S32K312，且 Fls 写/擦忙等被注释、Fee GC 占位；ASIL-D 需要目标板 HIL 证据，不能用 host 测试替代。

---

## 5. 防幻觉规则（写进 gate，不靠自觉）

1. 只以 CI artifact 为准；注释、README、安全案例自述均不算证据。
2. 任何“已完成”必须附：命令 + 输出摘要 + 提交 sha + 工具版本。
3. 禁止脚本把自动化评审写成 human；两个字段独立。
4. 数字冲突先查口径（文件范围、编译配置、测试集），不得取“好看的那个”。
5. 无法复现的内容写入“待验证清单”，不得出现在对外结论。
6. gate 必须 fail-closed：空输入、缺字段、证据过期一律失败。
7. 每次阈值收紧都要有对应提交与报告，禁止“为过 gate 而调阈值”。

---

## 6. 立即可执行的第一步

```bash
bash tools/capture_baseline.sh   # 需先由 PR-0.1 创建
```

没有该脚本前，人工基线命令：

```bash
cmake -B build-native -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-native --parallel
ctest --test-dir build-native --output-on-failure
python3 .yuleosh/tools/misra_full_scan.py --gate
python3 tools/check_coverage_gate.py
python3 tools/check_traceability_gate.py --require-review
```
