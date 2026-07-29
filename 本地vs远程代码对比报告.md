# yuleASR-Configurator 本地代码 vs 远程仓库 对比报告

**生成日期**: 2026-05-21
**本地目录**: `C:\Users\admin\yuleASR-sprint1`
**远程仓库**: `https://github.com/frisky1985/yuleASR.git` (master)

> ⚠ 注意：本地目录**没有 `.git` 仓库**，无法使用 git diff 工具。本报告基于逐文件手动对比。

---

## 一、项目概况

| 项目 | 本地 (sprint1) | 远程 (master) |
|------|---------------|---------------|
| 文件总数 | **18** | 5 个同名文件 |
| 本地新增文件 | **13** | — |
| 同名文件 | 5 个 (Cfg.h) | 5 个 (Cfg.h + config_tool.py) |
| 代码总量 | ~33 KB | — |

---

## 二、文件级差异总览

### 2.1 本地新增（远程完全不存在）— 13 个文件

#### A) `tools/config/src/` — 架构级重写（7 个文件）

| 文件 | 大小 | 说明 |
|------|------|------|
| `module_registry.py` | 1,397 B | 模块注册表基类：ModuleSchema + 注册API |
| `register_all.py` | 517 B | 统一注册入口，串联所有模块注册函数 |
| `mcal_schemas.py` | 5,105 B | 12 个 MCAL 模块配置 (Port/Dio/Can/Lin/Spi/Gpt/Mcu/Adc/Icu/Pwm/Fls/Crc) |
| `ecual_schemas_1.py` | 2,169 B | 4 个 ECUAL 模块 (CanIf/CanTp/EthIf/IoHwAb) |
| `ecual_schemas_2.py` | 2,205 B | 5 个 ECUAL 模块 (MemIf/Fee/Ea/FrIf/LinIf) |
| `services_schemas_1.py` | 2,008 B | 4 个 Services 模块 (Com/PduR/NvM/Dcm) |
| `remaining_schemas.py` | 2,588 B | 5 个补充模块 (WdgIf/Dem/EcuM/BswM/WdgM) |

**远程版本**: 只有 1 个文件 `config_tool.py` (~4 KB)，基于 Python dataclass 的简化配置管理器，仅支持 **Mcu + Can 2 个模块**。

**差异本质**: 单体架构 → 插件式注册表架构。覆盖模块从 **2 → 30** 个，覆盖 AutoSAR 4.4 所有三层。

---

#### B) `tools/generator/templates/` — 全新功能（5 个文件）

| 文件 | 大小 | 说明 |
|------|------|------|
| `wdgif_cfg.h.j2` | 1,048 B | WdgIf 配置头文件 Jinja2 模板 |
| `bswm_cfg.h.j2` | 1,133 B | BswM 配置头文件模板 |
| `dem_cfg.h.j2` | 1,248 B | Dem 配置头文件模板 |
| `ecum_cfg.h.j2` | 1,141 B | EcuM 配置头文件模板 |
| `wdgm_cfg.h.j2` | 1,298 B | WdgM 配置头文件模板 |

**远程**: 无 `tools/generator/` 目录。
**本地**: 新增完整的模板驱动代码生成能力（Jinja2 变量插值、条件判断、生成日期标记）。

---

#### C) `tools/import_export/dbc_importer.py` — 全新功能

| 文件 | 大小 | 说明 |
|------|------|------|
| `dbc_importer.py` | 7,439 B | DBC (CAN Database) 文件解析器 |

支持解析 BO_ / SG_ / CM_ / BA_ 语法，Intel/Motorola 字节序、信号比例/偏移等，输出 yuleASR 兼容格式。

---

### 2.2 同名文件 — 5 个头文件，内容完全不同

| 文件 | 本地 | 远程 |
|------|------|------|
| **WdgIf_Cfg.h** | 7 个 define，694 B — 精简模板示例 | 11 个 define，1,664 B — 含 timeout 触发/回调配置 |
| **BswM_Cfg.h** | 8 个 define，742 B | 16 个 define，854 B — 含 mode request/action/rule 完整计数 |
| **Dem_Cfg.h** | 9 个 define，823 B — 基本事件/计数器 | **68 个 define**，8,846 B — 含 DTC/freeze frame/OBD/J1939/debounce 完整配置 |
| **EcuM_Cfg.h** | 8 个 define，755 B | **69 个 define**，7,820 B — 含多阶段启动/32 唤醒源/sleep/shutdown 完整配置 |
| **WdgM_Cfg.h** | 9 个 define，839 B | **39 个 define**，6,594 B — 含 WWD/IWD/lockstep/Dem 集成/回调声明 |

---

## 三、分类差异总结

### 3.1 本地改进（可合并回远程）

| 分类 | 差异 | 建议 |
|------|------|------|
| `tools/config/src/` | 远程: 单体 1 文件 / 本地: 模块化 7 文件 | **值得合并** — 架构升级，30 模块覆盖 |
| `tools/generator/` | 本地新增 5 个 Jinja2 模板 | **可以合入** — 新增的代码生成能力 |
| `tools/import_export/` | 本地新增 DBC 解析器 | **可以合入** — 新增的 CAN 配置导入能力 |

### 3.2 本地简化（需用远程版本替换）

| 分类 | 差异 | 建议 |
|------|------|------|
| `src/bsw/` 所有 Cfg.h | 本地为精简演示版 | **需用远程完整版替换** — 本地丢失了大量产品级配置 |
| AR_RELEASE 版本 | 本地: 4.4.0 / 远程 Dem/EcuM: 无版本号 | 需对齐版本号 |

---

## 四、数据统计

```
┌─────────────────────────────────────────────────────┐
│                     统计汇总                          │
├─────────────────────────────────────────────────────┤
│ 本地文件总数:       18                               │
│ 远程存在的:         5  (全部是 Cfg.h)                │
│ 本地新增:          13                               │
│ 代码总量:           ~33 KB                           │
└─────────────────────────────────────────────────────┘
```

---

## 五、建议的下一步行动

1. **初始化 Git 仓库**: `git init && git remote add origin https://github.com/frisky1985/yuleASR.git`
2. **合并策略**:
   - `tools/config/src/` — 保留本地模块化版本（删除远程的 `config_tool.py`）
   - `tools/generator/` + `tools/import_export/` — 直接新增至远程
   - `src/bsw/*/include/*_Cfg.h` — **用远程完整版覆盖本地简化版**
3. **完善代码生成器**: 本地已有 5 个模板，可扩展至覆盖全部 30 个模块
4. **清理 __pycache__**: 提交前清理缓存目录

---

*报告完*
