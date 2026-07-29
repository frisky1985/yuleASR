# yuleOSH ↔ yuleASR 全自动集成 — 5 个缺口状态报告

> **生成时间**: 2026-07-29 14:19 GMT+8  
> **状态**: ✅ 全部完成  
> **总缺口**: 5 | **已完成**: 5 | **异常**: 0

---

## Task 2: yuleASR .yuleosh.yaml 🔴 P0 — ✅ 完成

### 实现内容
- 创建 `~/.openclaw/workspace/yuleASR/.yuleosh.yaml`
- 包含完整的项目配置：type: autosar, language: c, target: s32k312
- 三层 CI 流水线配置 (L1/L2/L3)
- 交叉编译参数 (arm-cortex-m7, arm-none-eabi-)
- MISRA C:2023 规则集配置
- C 覆盖率门禁 (c_fail_under: 70)

### 创建的文件
```
yuleASR/.yuleosh.yaml
yuleOSH/src/yuleosh/project_detection.py (new)
```

### 项目自动识别 (缺口 #3)
- `src/yuleosh/project_detection.py` — 新模块，扫描项目根目录 `.yuleosh.yaml`
- 解析 project.type → autosar → `templates/autosar-classic/` 模板
- 提供 `detect_project()` 和 `resolve_pipeline_config()` API
- 已集成到 `src/yuleosh/pipeline/orchestrator.py`
- 测试覆盖: `tests/test_project_detection.py` (6 tests, all pass)

---

## Task 1: GitHub Webhook 监听器 🔴 P0 — ✅ 完成

### 实现内容
- 重写 `src/yuleosh/api/webhooks.py`
- 使用 `_trigger_pipeline()` 替代旧的 `_trigger_ci()`
- 调用 `submit_pipeline()` / `submit_full_pipeline()` (async runner)
- 仓库名 → 项目类型映射: yuleASR → autosar (full pipeline)
- 结果推 Dashboard: `.yuleosh/reports/webhook-triggers.jsonl`

### 修改的文件
```
yuleOSH/src/yuleosh/api/webhooks.py (rewritten)
yuleOSH/tests/test_api_webhooks_ext.py (updated)
```

### API 端点
```
POST /api/v1/webhooks/github
```

### 测试
- 11 tests, all pass
- 完整的单元测试覆盖: 正常推送、无 commit、异常处理、管道触发成功/失败

---

## Task 3: QEMU 测试 Stage 🟡 P1 — ✅ 完成

### 实现内容
- 创建 `src/yuleosh/pipeline/step_handlers/test_qemu.py`
- `QemuTestHandler` 类，继承 `BaseHandler`
- 自动发现 `.elf` 文件 (tests/fixtures/prebuilt/, build/, .yuleosh/pipeline/)
- 调用 `qemu-system-arm` 并解析串口输出
- PASS/FAIL 模式匹配 (TEST PASS, SUCCESS, Hard Fault 等)
- 支持多种目标架构 (cortex-m3/m4/m7, arm926, x86_64)
- 支持 `.yuleosh.yaml` 自动解析目标架构
- 注册为 QEMU 步骤 `qemu_run`

### 注册位置
```
yuleOSH/src/yuleosh/pipeline/step_handlers/__init__.py
  → "qemu-run" step in PIPELINE_STEPS
yuleOSH/src/yuleosh/templates/autosar-classic/pipeline/config.yaml
  → L2: qemu_run: true
yuleOSH/src/yuleosh/templates/yuleasr/pipeline/config.yaml
  → L2: qemu_run: true
```

---

## Task 4: C 覆盖率流水线验证 🟡 P1 — ✅ 完成

### 实现内容
- 创建 `src/yuleosh/pipeline/step_handlers/c_coverage_gate.py`
- 4 阶段验证管道:
  1. **Build**: cmake `-DENABLE_COVERAGE=ON` 编译
  2. **Test**: ctest / pytest 运行测试用例生成 `.gcda`
  3. **Coverage**: gcovr 生成 JSON 覆盖率报告
  4. **Gate**: `check_coverage_gate.py` / yuleOSH CI 门禁验证
- 注册为 pipeline step "c-coverage-gate"
- 支持从 `.yuleosh.yaml` 和 `.yuleosh/ci-config.yaml` 读取 `c_fail_under`

### 注册位置
```
yuleOSH/src/yuleosh/pipeline/step_handlers/__init__.py
  → "c-coverage-gate" step in PIPELINE_STEPS
yuleOSH/src/yuleosh/templates/autosar-classic/pipeline/config.yaml
  → L2: c_coverage_gate: true
yuleOSH/src/yuleosh/templates/yuleasr/pipeline/config.yaml
  → L2: c_coverage_gate: true
```

---

## Task 5: Configurator auto-trigger on codegen 🟢 P2 — ✅ 完成

### 实现内容
- 新增 `PipelineSettings` 到 settings store
  - `autoTrigger`: 总开关 (默认关闭)
  - `autoTriggerOnCodegen`: 代码生成后自动触发 (默认开启)
- 新增 `updatePipelineSettings` action
- 新增 Pipeline 设置页面，含 Auto-trigger 切换开关
- 在 Editor.tsx 中，代码生成成功后自动调用 `triggerPipeline()`
  - 检查 settingStore 的 autoTriggerOnCodegen
  - 成功后自动显示 PipelineStatusPanel

### 修改的文件
```
yuleASR-Configurator/apps/yuleasr-web/src/stores/settingsStore.ts
  → PipelineSettings interface + updatePipelineSettings
yuleASR-Configurator/apps/yuleasr-web/src/pages/Settings.tsx
  → Pipeline Settings section with toggle
yuleASR-Configurator/apps/yuleasr-web/src/pages/Editor.tsx
  → Codegen handler: auto-trigger after generation
```

### 用户界面
- **Settings > Pipeline Settings > Auto-trigger Pipeline on Code Generation**
- 默认关闭（用户可选开启）
- 开启后，点击 Generate 按钮完成代码生成后自动触发 pipeline

---

## 总结

| 缺口 | 优先级 | 状态 | 文件数 | 新增代码 |
|------|--------|------|--------|----------|
| Task 1: Webhook 监听器 | 🔴 P0 | ✅ | 2 | ~180行 |
| Task 2: .yuleosh.yaml | 🔴 P0 | ✅ | 2 | ~180行 |
| Task 3: QEMU 测试 | 🟡 P1 | ✅ | 3 | ~350行 |
| Task 4: C 覆盖率 | 🟡 P1 | ✅ | 3 | ~450行 |
| Task 5: Auto-trigger | 🟢 P2 | ✅ | 3 | ~80行 |
| **总计** | | **5/5** | **13** | **~1240行** |

### 注意事项
1. QEMU 需要系统预装 `qemu-system-arm`，找不到时会自动跳过
2. gcovr 覆盖率门禁需要 `cmake -DENABLE_COVERAGE=ON` 编译
3. Webhook 生产环境需要在 GitHub repo 中配置真实 webhook URL
4. Auto-trigger 默认关闭，需用户在 Settings 中手动开启
