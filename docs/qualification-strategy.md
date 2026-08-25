# Qualification Test Strategy — yuleASR BSW

> 生成: 2026-08-25 (yuleOSH 审计闭环 — SWE.6.BP1)
> 适用项目: /Users/ingeek/workspace/AUTOSAR
> 状态: DRAFT — 定义合格性测试范围、环境与验收标准

## 1. 目的

证明集成后的 yuleASR BSW 软件栈满足软件需求（Spec → Code → Tests 追溯链已接通，
56 需求 ID / 287 SHALL 语句），为量产评审提供合格性证据。

## 2. 合格性测试范围

| 范围 | 内容 | 证据 |
|:-----|:-----|:-----|
| 需求覆盖 | 全部 56 需求 ID 的 SHALL 语句可追溯 | traceability-matrix |
| 单元级 | 35 模块单元测试 (54 ctest 用例) | ctest passing-run |
| 集成级 | 层间接口集成测试 | integration tests |
| 系统级 | QEMU/SIL 运行 + 启动序列 | sil-reports/ |
| 目标级 | S32K312 交叉编译 + 链接 | build-arm 产物 |

## 3. 测试环境

- **Host**: macOS native (ctest, 54 用例)
- **SIL**: QEMU M33 (`tests/qemu_m33/`)，预编译 .elf 放 `tests/fixtures/prebuilt/`
- **Target**: S32K312 (arm-none-eabi-gcc 交叉编译)，配置见 `.yuleosh.yaml` cross_compile
- **覆盖率门槛**: line ≥ 70%, branch ≥ 60%（.yuleosh.yaml coverage）

## 4. 执行与证据归档

1. 每层 CI 执行记录: `.osh/ci/layer{1,2,3}-*.json`
2. 测试报告: ctest 输出 + JUnit XML
3. 覆盖率报告: `.osh/evidence/code-coverage-report.md`
4. 全部归档: `yuleosh audit evidence` → `.yuleosh/audit/`

## 5. 每条需求的验收标准

- 需求可追溯到至少一个测试用例（test → req 双向）
- 测试用例通过且覆盖率达标
- 测试在目标环境或等效环境（QEMU SIL）执行

## 6. 完成定义 (Definition of Done)

- [ ] 全部合格性测试通过（passing-run evidence 归档）
- [ ] 覆盖率 line ≥ 70%
- [ ] 需求 → 测试 100% 双向追溯
- [ ] SIL/HIL 执行记录归档
