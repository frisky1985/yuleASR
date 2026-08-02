# Sprint Contract: S1 测试体系重建（能验证、能回归）+ MCAL 桩审计

> 基于 S0（能链接、能启动）验收结果与 Out of Scope 清单，2026-08-03 制定。
> 目标：把 S0 的"构建证据"推进到"运行证据"，建立 Linux CI 回归基线，并为 MCAL 桩补齐铺路。
> 分支: v1.3.0 | 仓库: git@github.com:frisky1985/yuleASR.git

## 背景与缺口（调研结论）

1. **S0 验收链缺运行环**：S0 的 C7（native 运行验证）在 macOS 无法执行（FreeRTOS Posix port 官方仅支持 Linux），`tests/s0_smoke_test.c` 已提交但**未被任何 CMake/CI 引用**（全仓库 grep 零命中），S0"能启动"结论只有链接证据、无运行证据。
2. **CI 不覆盖当前分支**：`.github/workflows/ci.yml` 触发分支为 `master/develop`，`v1.3.0` push 不会触发任何 CI。
3. **MCAL 5 模块不是空桩**：ocu(690 行)/eth(880)/fee(1210)/eep(570)/lin(7913) 有实质代码且可编译（`gcc -fsyntax-only` 通过），但属浅实现/演示级（如 Eep 依赖 Fls、不访问寄存器），补齐前必须先摸清 API 面与依赖。
4. **Services 桩依赖底层**：cansm/secoc/ethsm/udpNm 等 E_NOT_OK 密集，补齐依赖 MCAL/ECUAL 先落地。

## Scope

### In Scope
1. **P0 · 测试体系重建 — native 运行验证**（S1-1）：
   - `tests/s0_smoke_test.c` 接入 CMake：`BUILD_TESTING=ON` 时生成 `s0_smoke_test` 可执行（链接 Os 库，含 FreeRTOS V11.1.0 Posix port + Rte 库）
   - 注册 ctest 用例，使 `ctest -R s0_smoke` 可发现
2. **P0 · Linux CI 运行验证**（S1-2）：
   - 新建 `.github/workflows/smoke-linux.yml`（独立于 ci.yml，避免改动 master/develop 既有门禁），触发分支含 `v1.3.0`
   - ubuntu-latest 上构建并运行 `s0_smoke_test`，断言退出码 0 + 关键输出
   - 同一 workflow 内补 ARM 交叉编译回归（复用 `cmake/toolchain-arm-none-eabi.cmake`）
3. **P1 · MCAL 桩审计**（S1-3，只读调研交付）：
   - 产出 `docs/audit/mcal-stub-audit.md`：对 ocu/eth/fee/eep/lin 五个模块逐一给出 API 面完整性、依赖缺失清单、实现深度分级（空桩/浅实现/演示级/完整）、符号统计（nm 证据）
   - 报告末尾给出下一轮 MCAL 补齐的优先级排序与理由

### Out of Scope（后续 sprint）
- MCAL 桩**完整补齐**（下一轮，按 S1-3 审计排序执行）
- ECUAL 浅实现补齐（ethif/frif/iohwab 调 MCAL）
- Services 桩补齐（cansm/canm/secoc/cantsyn/ethsm/ethtsyn/udpNm 等 7+ 个）
- 功能安全文档造假问题（独立治理流程，非代码 sprint）
- 全量单元测试体系（UT 覆盖率门禁、mock 体系等；本轮只建 smoke/运行基线）

## Architecture Decision
- **architect-lead**: Hermes（测试体系先行：先建运行基线，再补模块；避免"补了没法验"的返工）
- **运行验证环境 = Linux CI（ubuntu-latest）**：FreeRTOS Posix port 官方仅支持 Linux；macOS 可编译链接（S0 已证：build-s0 的 libOs.a 为 Posix port 构建且含 `pxPortInitialiseStack`/`prvWaitForEvent` 符号）但运行不可靠 —— macOS 只承担"编译链接"验证，运行验证一律走 CI
- **独立 workflow 而非改 ci.yml**：ci.yml 触发 `master/develop`，当前开发在 `v1.3.0`；新建 `smoke-linux.yml` 触发 `v1.3.0` + `master`，不动既有门禁
- **s0_smoke_test 挂载位置**：`tests/` 层（`tests/CMakeLists.txt` 增加子目录/目标），仅 `BUILD_TESTING=ON` 时构建，链接 `Os`（FreeRTOS 内核 + Posix port + pthread）+ `Rte` 库；测试内 `Rte_SchedulerGetTickCount` 经 extern 声明访问（沿用现有约定）
- **审计先行、实现后置**：MCAL 5 模块本轮**只读**（仅新增 docs 文件，禁止修改 `src/bsw/mcal/*`），审计证据一律用可复现命令（nm/grep/wc）
- **验收证据以命令输出为准**：所有 Pass/Fail 条件给出可执行命令与预期输出，杜绝主观判断

## Testable Behaviors

### 构建（macOS 可验）
- [ ] B1.1: `cmake -B build-s1 -DBUILD_TESTING=ON` 配置成功 | Owner: Generator
- [ ] B1.2: `cmake --build build-s1 --target s0_smoke_test` 生成可执行文件（链接通过） | Owner: Generator
- [ ] B1.3: `cmake -B build-s1-base -DBUILD_TESTING=OFF && cmake --build build-s1-base` 全绿（S0 基线不回退） | Owner: Evaluator

### 运行（仅 Linux CI 可验，macOS 不验）
- [ ] B2.1: `./s0_smoke_test` 退出码 0 | Owner: Evaluator
- [ ] B2.2: stdout 含 `RESULT: ALL CHECKS PASSED` | Owner: Evaluator
- [ ] B2.3: stdout 含 `Rte_SchedulerGetTickCount = N` 且 N > 0（调度器真实派发过周期任务） | Owner: Evaluator

### CI（Linux CI 可验）
- [ ] B3.1: push v1.3.0 触发 `smoke-linux` workflow 且全绿 | Owner: Evaluator
- [ ] B3.2: 同 workflow 内 ARM 交叉编译 job 全绿 | Owner: Evaluator

### 审计（macOS 可验）
- [ ] B4.1: `docs/audit/mcal-stub-audit.md` 存在，5 模块全覆盖 | Owner: Evaluator
- [ ] B4.2: 报告每模块含 nm/grep 命令证据摘录 | Owner: Evaluator
- [ ] B4.3: `git status --porcelain src/` 为空（未修改源代码） | Owner: Evaluator

## Acceptance Criteria

| ID | Criterion | Pass Condition | Fail Condition | Priority | Owner | 验证环境 |
|----|-----------|----------------|----------------|----------|-------|----------|
| C1 | s0_smoke_test 接入 CMake | `cmake --build build-s1 --target s0_smoke_test` 成功，产物为可执行文件（`file` 显示 Mach-O/ELF） | target 缺失或链接失败（undefined ref） | P0 | Generator | macOS ✅ / CI ✅ |
| C2 | ctest 注册 | `ctest --test-dir build-s1 -N` 输出含 `s0_smoke` 用例 | 未注册 | P0 | Generator | macOS ✅ / CI ✅ |
| C3 | Linux 运行验证（核心） | ubuntu-latest 上 `./s0_smoke_test` 退出码 0，且输出 grep 命中 `RESULT: ALL CHECKS PASSED` 与 `Rte_SchedulerGetTickCount = [1-9]` | 退出码非 0 / 输出缺失 / 断言失败 | P0 | Generator(实现) + Evaluator(确认) | CI ✅（macOS 不验） |
| C4 | CI 触发覆盖 v1.3.0 | push v1.3.0 产生 `smoke-linux` workflow run 且绿 | push 后无 run 或 run 红 | P0 | Generator | CI ✅ |
| C5 | ARM 交叉编译回归 | workflow 内 `cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake && cmake --build build-arm` 全绿 | ARM 编译/链接失败 | P0 | Generator | CI ✅（macOS 本地有 arm-none-eabi-gcc 可手动复验） |
| C6 | macOS 基线不回退 | B1.3 全绿（S0 的 native 全量构建保持通过） | 编译失败 | P0 | Evaluator | macOS ✅ |
| C7 | 审计报告产出 | `docs/audit/mcal-stub-audit.md` 存在，ocu/eth/fee/eep/lin 各有独立章节 | 报告缺失或某模块无条目 | P1 | Generator | macOS ✅ |
| C8 | 审计证据客观 | 每模块含 `nm build-s1/lib/bsw/mcal/libmcal_*.a` 符号计数、`wc -l` 行数、依赖 grep 证据（命令+输出摘录） | 无命令证据或数据不可复现 | P1 | Generator | macOS ✅ |
| C9 | 补齐排序建议 | 报告末尾给出 5 模块补齐优先级排序（含理由，如依赖关系/ASW 耦合度） | 无排序或理由缺失 | P1 | Generator | macOS ✅ |
| C10 | 审计不引入代码变更 | `git status --porcelain src/` 为空；本轮提交仅含 docs/ 与 workflow/CMake 变更 | 修改了 src/bsw/mcal/* 等源码 | P1 | Evaluator | macOS ✅ |

## Responsibility Matrix

| Criterion | Responsible | Fallback |
|-----------|-------------|----------|
| C1-C2 | Generator (Hermes) | — |
| C3-C5 | Generator (Hermes) 实现 workflow；Evaluator (Hermes) 触发并确认 CI 绿 | Generator 修复迭代 |
| C6 | Evaluator (Hermes) | Generator 修复迭代 |
| C7-C9 | Generator (Hermes) 产出审计报告 | Evaluator 复核证据 |
| C10 | Evaluator (Hermes) 门禁 | Generator 回退越界改动 |

## Negotiation Log

| Round | Party | Action | Notes |
|-------|-------|--------|-------|
| 1 | Generator | 提案 | 从 S0 Out of Scope 5 项中选定"测试体系重建"为 P0（理由：S0 验收链唯一缺失的运行环；无运行基线则后续 MCAL/ECUAL/Services 补齐均无法验收），"MCAL 桩审计"为 P1 只读调研交付（理由：5 模块已有数千行实质代码，先摸清再补，防范围蔓延） |
| 1 | architect-lead | APPROVE | "先建基线、再补模块"排序合理；独立 smoke-linux.yml 不动 master/develop 门禁，风险可控 |
| 1 | Evaluator | 条件 APPROVE | 要求：① 所有验收标准给出可执行命令与预期输出；② C3 运行断言必须含退出码 + grep 双重证据；③ macOS 只承担编译验证，运行一律以 CI 为准，避免"本地跑不了"争议 |

## 风险与备注

| # | 风险/备注 | 影响 | 处理 |
|---|-----------|------|------|
| R1 | s0_smoke_test 首次在 Linux 运行可能暴露 S0 未覆盖的缺陷（Posix 信号/时钟语义差异） | 中 | 属预期收益：发现即修复，修复迭代纳入本 sprint C3，不视为范围蔓延 |
| R2 | 现有 ci.yml 的 test job（ubuntu, BUILD_TESTING=ON）也会 ctest 到新注册用例 | 低（正收益） | master 分支 CI 自动获得双保险；smoke-linux.yml 保证 v1.3.0 覆盖 |
| R3 | macOS 本地 `ctest -R s0_smoke` 运行可能挂（Posix port 不支持 macOS 运行） | 低 | 契约明确：macOS 只验编译链接（C1/C2 前半），运行断言仅 CI 执行；本地运行失败不作为验收 Fail |
| R4 | 审计范围越界（顺手"修"源码） | 中 | C10 门禁：`git status --porcelain src/` 必须为空 |
| R5 | CI workflow 触发条件写错导致 v1.3.0 push 不触发 | 中 | C4 验收硬性要求一次真实 push 触发的 run 记录 |

## 下一轮候选（本 sprint Out of Scope 的后续去向）

1. MCAL 桩补齐（按 S1-3 审计排序执行，目标：行为契约 + 运行可验）
2. ECUAL 浅实现补齐（iohwab 与 ASW 耦合最高，建议优先）
3. Services 桩补齐（依赖底层，最后）
4. 功能安全文档造假问题（独立治理流程）
