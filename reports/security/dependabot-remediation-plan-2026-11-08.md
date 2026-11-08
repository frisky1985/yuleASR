# Dependabot 剩余告警处置计划（minimatch / deepmerge）

> **制定日期**: 2026-11-08
> **代码基线**: `master@51affd6`
> **当前告警**: 3 high 已修复；剩余 **1 moderate（minimatch）+ 1 low（deepmerge）**
> **结论**: 两项均**无安全的自动升级路径**，建议 dismiss 并登记跟踪；`minimatch` 保留有条件升级预案。

---

## 1. 事实基线

| 项 | minimatch | deepmerge |
|:---|:----------|:----------|
| 公告 | GHSA-g3hv-75wg-m249 | GHSA-gcqx-w48m-w2h4 |
| 等级 | Moderate（ReDoS / globparts 放大） | Low（原型污染） |
| 影响清单 | `tools/yuleasr-configurator/web/package-lock.json` | `website/package-lock.json` |
| 实际实例 | `9.0.9`（`@electron/universal`、`@vue/language-core`，均 `^9.0.3`）<br>`10.2.6`（`app-builder-lib@26.18.1`，`^10.2.5`） | `4.3.1`（`@svgr/plugin-svgo@8.1.0`，`^4.3.1`） |
| 修复版本 | **仅 `11.0.1`+**（无 9.x/10.x 回补） | **无**（4.3.1 为最终发布版） |
| 上游现状 | `app-builder-lib` 最新仍为 `^10.2.5`；`@electron/universal`/`@vue/language-core` 仍为 `^9.0.3` | `@svgr/plugin-svgo@8.1.0` 为最新，仍依赖 `deepmerge ^4.3.1` |
| 运行时暴露 | **无**：仅 Electron 打包（asar 文件匹配）与 `vue-tsc` 类型检查阶段 | **无**：Docusaurus/SVGR 构建期合并可信配置对象 |
| CI 覆盖 | configurator **无 CI 构建**（`.github/workflows` 未引用）；Node 20（docs 工作流） | website 有 CI 构建（Node 20） |
| 技术阻碍 | `minimatch@11` 要求 Node `20 \|\| >=22`，且为**跨大版本**；`app-builder-lib` 未声明支持 | 无任何 patched 版本可安装 |

> `website` 中的 `minimatch@3.1.5`（`serve-handler` 精确锁定）**不在该公告影响范围**，OSV 未命中，无需处理。

---

## 2. 决策矩阵

| 方案 | minimatch | deepmerge | 评价 |
|:-----|:---------|:----------|:-----|
| A. Dismiss + 跟踪 | ✅ 推荐 | ✅ **唯一可行** | 零构建风险；需人工/高权限 token 操作；需定期复核 |
| B. 立即升级 | ⚠️ 有条件 | ❌ 不可行 | 需 override 强制跨大版本，且 configurator 无 CI 保护，回归风险高 |
| C. 替换依赖 | ❌ 不适用 | ❌ 不可行 | `@svgr/plugin-svgo` 无替代实现；替换成本远超收益 |

**推荐执行顺序**：先按方案 A dismiss 两项（立即清零告警），同时把 `minimatch` 升级列为方案 B 的观察项，待上游支持 `>=11` 后自动升级。

---

## 3. 方案 A：Dismiss（推荐，立即执行）

### 3.1 权限前置

当前 `git credential` 中的 40 位 classic token **缺少 `security_events` 权限**，REST `GET /dependabot/alerts` 返回 403，无法脚本化获取告警编号。二选一：

1. **Web UI（推荐，零配置）**：`https://github.com/frisky1985/yuleASR/security/dependabot`
2. **API**：使用具备 `security_events`（classic）或 `Dependabot alerts: read & write`（fine-grained）的 token

### 3.2 Dismiss 参数

| 告警 | `dismissed_reason` | `dismissed_comment` |
|:-----|:-------------------|:--------------------|
| minimatch | `tolerable_risk` | `Build-time only (electron-builder asar globbing / vue-tsc). No runtime exposure. No patched 9.x/10.x exists; v11 requires Node >=20 major bump and upstream app-builder-lib does not support it yet. Tracked for upgrade when upstream bumps to minimatch >=11.` |
| deepmerge | `vulnerable_code_cannot_be_controlled_by_adversary` | `No patched version exists (4.3.1 is final). Used only at build time by @svgr/plugin-svgo to merge trusted Docusaurus/SVGR config; no untrusted input reaches mergeObject(). Tracked upstream.` |

> 备选理由：minimatch 亦可用 `vulnerable_code_cannot_be_controlled_by_adversary`（glob 模式来自仓库配置，非攻击者可控）；deepmerge 亦可用 `vulnerable_code_not_in_execution_path`（仅构建期）。二者均属 GitHub 允许的枚举值，择一即可。

### 3.3 执行命令（API 方式）

```bash
# 1) 获取告警编号（需 security_events 权限）
gh api 'repos/frisky1985/yuleASR/dependabot/alerts?state=open&per_page=100' \
  --jq '.[] | [.number, .dependency.package.name, .security_advisory.severity] | @tsv'

# 2) 逐条 dismiss（<N> 替换为告警编号）
gh api -X PATCH repos/frisky1985/yuleASR/dependabot/alerts/<N> \
  -f state=dismissed \
  -f dismissed_reason=tolerable_risk \
  -f dismissed_comment='Build-time only; no patched 9.x/10.x; v11 needs Node>=20 major bump'

gh api -X PATCH repos/frisky1985/yuleASR/dependabot/alerts/<N> \
  -f state=dismissed \
  -f dismissed_reason=vulnerable_code_cannot_be_controlled_by_adversary \
  -f dismissed_comment='No patched version; build-time merge of trusted config only'

# 3) 复核：open 告警应为 0
gh api 'repos/frisky1985/yuleASR/dependabot/alerts?state=open' --jq 'length'
```

### 3.4 复核节奏

- 每季度检查一次上游：`npm view app-builder-lib dependencies.minimatch`、`npm view @svgr/plugin-svgo dependencies.deepmerge`
- 上游一旦支持修复版本，Dependabot 会重新打开告警（dismiss 不会永久屏蔽），届时直接升级即可

---

## 4. 方案 B：升级预案（仅 minimatch，有条件）

### 4.1 前置条件（全部满足才执行）

1. Node 运行时 ≥ 20（CI 当前为 20，满足；本地/发布环境需确认）
2. 在独立分支执行，且补齐 configurator 的构建/打包冒烟验证（当前 CI 无覆盖）
3. `npm ls minimatch` 确认无 `^3.x` 消费者被误伤（ESLint 8、`glob@7`、`filelist` 等必须保持 3.x/5.x）

### 4.2 定向 override（只影响三个父依赖，不做全局替换）

在 `tools/yuleasr-configurator/web/package.json` 增加：

```json
"overrides": {
  "tar": "^7.5.22",
  "extract-zip": "^2.0.1",
  "esbuild": "^0.28.2",
  "postcss-selector-parser": "^7.1.7",
  "app-builder-lib": { "minimatch": "^11.0.5" },
  "@electron/universal": { "minimatch": "^11.0.5" },
  "@vue/language-core": { "minimatch": "^11.0.5" }
}
```

> 禁用全局 `"minimatch": "^11.0.5"`：会把 ESLint 8 / `glob@7` / `filelist` 的 3.x/5.x 一并强升，直接破坏 lint 与旧版 glob。

### 4.3 验证清单

```bash
cd tools/yuleasr-configurator/web
rm -rf node_modules package-lock.json
npm install --registry=https://registry.npmjs.org
npm ls minimatch                      # 期望：11.0.5 仅在三个父依赖下；3.1.5/5.1.9 保持不变
npm run lint
npm run build                         # vue-tsc + vite build
npm run build:electron                # vite + tsc + electron-builder（打包冒烟）
```

### 4.4 回滚

- 任一验证失败：删除三条 override → `npm install` 重生成锁文件 → 恢复 dismiss 状态
- 变更仅涉及 `package.json` + `package-lock.json`，回滚成本极低

### 4.5 deepmerge 的升级替代（评估结论：不执行）

- 唯一可能的路径是把 `@svgr/plugin-svgo` 替换为不依赖 `deepmerge` 的实现，或等待上游更换合并库；
- `@svgr` 8.1.0 为最新稳定版，Docusaurus 依赖其 SVGR 管线，替换将导致文档站构建链路分叉；
- 收益（Low / 构建期 / 不可控输入）远小于成本，**不执行**。

---

## 5. 验收标准

- [ ] GitHub Dependabot open alerts = **0**（2 条为 dismissed，附理由与备注）
- [ ] `npm audit --registry=https://registry.npmjs.org` 三个清单 `high = 0 / critical = 0`
- [ ] `npm ci` 在 website 正常（CI 绿色）
- [ ] `reports/quality/production-readiness-2026-11-08.md` 5.8 节与本计划互相引用
- [ ] 季度复核已登记（上游 minimatch/deepmerge 版本变化时重开）

---

## 6. 执行清单

| # | 动作 | 责任 | 方式 | 状态 |
|:-:|:-----|:-----|:-----|:----:|
| 1 | dismiss minimatch（tolerable_risk） | 仓库管理员 | UI 或带 `security_events` 的 token | 待执行 |
| 2 | dismiss deepmerge（不可控输入） | 仓库管理员 | 同上 | 待执行 |
| 3 | 确认 open alerts = 0 | 仓库管理员 | Dependabot 页面/API | 待执行 |
| 4 | （可选）minimatch 定向 override 试验 | 前端 | 独立分支 + 4.3 验证 | 观察项 |
| 5 | 季度上游版本复核 | 维护者 | `npm view` | 循环 |
