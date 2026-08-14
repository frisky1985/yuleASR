# yuleASR 远程仓库 vs 本地 yuleASR-sprint1 对比报告

**生成日期**: 2026-05-21
**远程仓库**: `github.com/frisky1985/yuleASR.git` (master)
**本地目录**: `C:\Users\admin\yuleASR-sprint1`
**同步状态**: ✅ 已完全同步（3 个本地提交已推送）

---

## 一、同步状态

| 维度 | 状态 |
|------|------|
| 本地分支 | `master` |
| 关联远程 | `origin → https://github.com/frisky1985/yuleASR.git` |
| 本地领先远程 | **0** 个提交 (已完全推送) |
| 远程领先本地 | **0** 个提交 (已完全拉取) |
| 提交历史 | `bbe36952` → `2733b8f5` → `32c3744c` → `fc7cdc0a` |

### Git 提交历史

```
bbe36952 docs(modules): 添加66个BSW模块文档和测试基础设施     ← 远程源
2733b8f5 refactor(tools): 配置工具模块化重构                   ← 本地合并提交(已推送)
32c3744c docs: 添加代码合并报告                               ← 已推送
fc7cdc0a feat(generator): 代码生成器v2.0模板驱动架构          ← 已推送
```

---

## 二、工作区差异（54 个未暂存文件）

除了已推送到远程的3个提交之外，本地工作区中还有 **54 个文件** 与远程仓库内容不一致。这些是原始 `yuleASR-sprint1` 工作区文件的残留内容（从未被 Git 提交过）。

### 差异分类统计

| 分类 | 文件数 | 差异性质 |
|------|--------|---------|
| `src/bsw/` | **14** 个 | 原始 sprint1 简化版源文件 vs 远程完整实现 |
| `docs/modules/` | **30** 个 | 原始 sprint1 文档 vs 远程更新版 |
| `tests/` | **6** 个 | 原始 sprint1 测试 vs 远程完整测试 |
| `config/input/` | **3** 个 | 原始 sprint1 配置头文件 vs 远程完整版 |
| `docs/docs-site/` | **1** 个 | Docusaurus 数据文件 |
| **合计** | **54** | **+8,329 / -8,950 行** |

### 2.1 src/bsw/ — 14 个文件差异

| 文件 | 本地(简化) | 远程(完整) |
|------|-----------|-----------|
| `src/bsw/services/doip/src/DoIP.c` | 大幅精简 | 完整 DoIP 实现 (727+ / 508-) |
| `src/bsw/services/doip/src/DoIP_Lcfg.c` | 精简版 | 完整链接配置 |
| `src/bsw/services/doip/include/DoIP.h` | 精简 47+ / 271- | 完整 doIP 接口 |
| `src/bsw/services/doip/include/DoIP_Cfg.h` | 仅 8 个宏 | 102 个宏 |
| `src/bsw/services/comM/src/ComM.c` | 318+ / 874- | 完整 ComM 状态机 |
| `src/bsw/services/comM/include/ComM.h` | 精简 | 完整 ComM 接口 |
| `src/bsw/services/comM/include/ComM_Cfg.h` | 29+ / 173- | 完整 ComM 配置 |
| `src/bsw/services/secoc/src/SecOC.c` | 97+ / 508- | 完整 SecOC 实现 |
| `src/bsw/services/secoc/include/SecOC.h` | 52+ / 341- | 完整 SecOC 接口 |
| `src/bsw/services/secoc/include/SecOC_Cfg.h` | 19+ / 173- | 完整 SecOC 配置 |
| `src/bsw/services/wdgm/src/WdgM.c` | 317+ / 735- | 完整 WdgM 实现 |
| `src/bsw/services/wdgm/include/WdgM.h` | 172+ / 383- | 完整 WdgM 接口 |
| `src/bsw/services/wdgm/include/WdgM_Cfg.h` | 58+ / 197- | 完整 WdgM 配置 |
| `src/bsw/services/cantsyn/src/CanTSyn.c` | 6+ / 512- | 完整 CanTSyn 实现 |

### 2.2 docs/modules/ — 30 个文件差异

多数文档在远程有**大量扩展**（BSWM +334, ECUM +736, E2E +644, PDUR +377 行等），本地是陈旧版本。

### 2.3 tests/ — 6 个文件差异

| 文件 | 差异说明 |
|------|---------|
| `test_EthIf.c` | 本地 **扩展了 930 行**（可能是 sprint1 新增的测试） |
| `test_DoIP.c` | 远程有完整测试 (本地仅 16+) |
| `test_LinIf.c` | 远程完整 (本地仅 16+) |
| `test_Nm.c` | 远程完整 (本地仅 16+) |
| `test_SoAd.c` | 远程完整 (本地仅 16+) |
| `test_Dlt.c` | 差异较大 (196+ / 320-) |

### 2.4 config/input/ — 3 个文件差异

| 文件 | 本地 | 远程 |
|------|------|------|
| `DoIP_Cfg.h` | 简化 (43+ / 132-) | 完整配置 |
| `SecOC_Cfg.h` | 简化 (19+ / 171-) | 完整配置 |
| `WdgM_Cfg.h` | 简化 (58+ / 197-) | 完整配置 |

---

## 三、总体评价

```
┌─────────────────────────────────────────────────────┐
│  同步状态                                           │
│  ✅ Git 提交历史: 完全同步 (0 ahead, 0 behind)      │
│  ⚠️ 工作区: 54 个文件未被 Git 跟踪                   │
├─────────────────────────────────────────────────────┤
│  本地工作区差异特征                                  │
│  • 源码(src/bsw/): 本地简化, 远程完整产品级          │
│  • 文档(docs/): 本地陈旧, 远程大量扩展               │
│  • 测试(tests/): 本地精简, 远程有完整测试套件        │
│  • 仅 test_EthIf.c: 本地比远程多 930 行测试         │
├─────────────────────────────────────────────────────┤
│  建议                                              │
│  • 54 个工作区文件是原始 sprint1 的残留              │
│  • 可用远程版本覆盖: git checkout origin/master     │
│  • 或清理工作区: git reset --hard HEAD              │
└─────────────────────────────────────────────────────┘
```

---

*报告完*
