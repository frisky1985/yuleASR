# yuleASR CI 配置指南

> **文档版本**: 1.0 | **日期**: 2026-07-26
> **范围**: `.yuleosh/ci-config.yaml`

---

## 1. 概述

yuleASR 使用 yuleOSH 引擎驱动的 CI 流水线，配置统一在 `.yuleosh/ci-config.yaml` 中管理。

### 1.1 三层架构

| 层 | 阶段 | 内容 | 超时 |
|:--:|:-----|:-----|:----:|
| Layer 1 | 构建 + 测试 | 编译构建、单元测试、MISRA 检查、覆盖率测量 | 180s |
| Layer 2 | 审查 | 架构审查(L2.0)、代码审查(L2.1)、性能分析(L2.2) | — |
| Layer 3 | 证据链 | 追溯矩阵生成、合规审查、证据打包 | — |

### 1.2 执行方式

```bash
# 运行全部三层
yuleosh ci run 1
yuleosh ci run 2
yuleosh ci run 3

# 仅运行特定层
yuleosh ci run 1 --quick
```

---

## 2. 配置块说明

### 2.1 CI 基础配置 (`ci:`)

```yaml
ci:
  layers: [1, 2, 3]
  layer_dependencies:
    1: []        # Layer 1 无依赖
    2: [1]       # Layer 2 依赖 Layer 1
    3: [1, 2]    # Layer 3 依赖 Layer 1 和 2
  layer1_timeout: 180  # Layer 1 超时(秒)
```

### 2.2 覆盖率配置 (`coverage:`)

```yaml
coverage:
  threshold_line: 20.0        # 行覆盖率阈值(%)
  threshold_condition: 10.0   # 条件覆盖率阈值(%)
  c_fail_under: 35            # C 代码最低通过率
  strict: false               # 严格模式(开启则所有阈值硬性要求)
  threshold_branch: 60.0      # 分支覆盖率阈值(%)
  module_thresholds:
    src/ecual: 35.0
    src/mcal: 35.0
    src/services: 35.0
```

**说明**:
- `threshold_line`：全仓库行覆盖率最低要求
- `threshold_branch`：分支覆盖率门禁（ASIL B 相关）
- `module_thresholds`：各模块独立阈值，比全局更精细
- `c_fail_under`：C 代码整体覆盖率的最低容忍值

### 2.3 MISRA 配置 (`misra:`)

`misra` 块包含完整的 MISRA 规则分析配置：

```yaml
misra:
  enabled: true
  addon: "misra"                 # 使用的 MISRA 插件
  active_profile: "safety"       # 当前激活的配置模板
  advisory_violations: 66        # 已知 Advisory 违规基线数
  fail_on_required: false        # Phase 1: Required 违规暂不阻塞 CI
  fail_on_advisory: false        # Phase 1: Advisory 违规暂不阻塞 CI
  fail_threshold: 13000          # 违规总数阈值(含 style)
  violations_per_kloc: 150.0     # 每千行违规密度上限
  cppcheck_std: "c11"            # C 语言标准
  rule_texts_path: ".yuleosh/misra_texts.txt"
```

#### 2.3.1 偏差管理 (`deviations:`)

项目级 MISRA 偏差列表。每个偏差指定规则、匹配文件和豁免理由：

```yaml
deviations:
  - rule: "misra-c2012-20.9"
    file: "src/**"
    reason: "AUTOSAR R21-11 §8.4 — #if defined() required for config switches"
```

当前共 **16 条** 全局偏差，覆盖 AUTOSAR 与 MISRA 的已知冲突。

#### 2.3.2 排除路径 (`exclude_paths:`)

```yaml
exclude_paths:
  - "tests/**"              # 测试代码
  - "src/**/*_test.c"       # 单元测试桩
  - "src/**/legacy/**"      # 遗留代码
  - "third_party/**"        # 第三方代码
  - "build/**"              # 构建产物
```

#### 2.3.3 代码分类 (`code_categories:`)

| 分类 | 路径 | 操作 | CI 阻断 |
|:----|:-----|:----:|:--------:|
| template | `src/yuleosh/templates/**` | 排除 | 否 |
| third_party | `third_party/**` | 告警 | 否 |
| business | `src/**` | 强制执行 | 否 |

#### 2.3.4 配置文件 (`profiles:`)

两个内置配置模板：

**safety**（安全配置）- 16 条项目级偏差许可 (DP-AUTOSAR-001~025)：
- 每个偏差含 ID、规则、范围、理由、过期时间
- `advisory_violations` 清单列出已接受的 Advisory 偏差
- `deviation_expiry_days: 180` — 偏差过期提醒周期

**testing**（测试配置）- 放宽标准库限制：
```yaml
testing:
  rule_overrides:
    - rule: "misra-c2023-21.3"
      enabled: false   # 测试配置允许使用标准库函数
  deviations: []
```

### 2.4 硬件测试配置 (`hardware_test:`)

```yaml
hardware_test:
  enabled: false         # 当前禁用(无 S32K312 硬件环境)
  mock: true             # 模拟模式
  firmware: "build-native/firmware.elf"
  boot_pattern: "Boot Complete"
  serial_port: ""        # 串口(自动检测)
  baud: 115200           # 串口波特率
  test_timeout: 30       # 测试超时(秒)
  boot_delay: 2.0        # 启动等待延迟(秒)
  test_scripts_dir: "tests/hil"  # HIL 测试脚本目录
```

---

## 3. 配置修改指南

### 3.1 调整覆盖率阈值

```yaml
coverage:
  threshold_line: 35.0        # 从 20% 提升到 35%
  c_fail_under: 50            # C 代码提升到 50%
  module_thresholds:
    src/services/e2e: 60.0    # E2E 模块独立阈值(ASIL B)
    src/services/wdgm: 60.0   # WdgM 模块独立阈值
    src/services/nvm: 60.0    # NvM 模块独立阈值
```

### 3.2 添加新 MISRA 偏差

1. 确定偏差规则和文件范围
2. 在 `misra.deviations` 列表中添加新条目
3. 在 `misra.profiles.safety.deviations` 中添加正式偏差许可记录
4. 在 `misra.profiles.safety.advisory_violations` 中添加（如果是 Advisory）
5. 更新 `docs/compliance/misra-deviation-report.md`

### 3.3 切换配置模板

```bash
# 使用安全配置
yuleosh misra run --profile safety

# 使用测试配置
yuleosh misra run --profile testing
```

---

## 4. 相关文件

| 文件 | 用途 |
|:----|:------|
| `.yuleosh/ci-config.yaml` | 主 CI 配置文件 |
| `.yuleosh/misra_texts.txt` | MISRA 规则文本缓存 |
| `.cppcheck_suppressions` | cppcheck 抑制配置 |
| `.misra_config` | MISRA 内置配置 |
| `docs/compliance/misra-deviation-report.md` | 偏差管理报告 |
| `batch10_coverage.sh` | 覆盖率批量测量脚本 |
