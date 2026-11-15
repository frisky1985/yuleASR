# yuleASR MISRA C:2012 合规评审报告

> **日期**: 2026-11-02 | **评审人**: AI Agent (Trae)
> **标准**: MISRA C:2012 (含 AMD1/AMD2 分类口径)
> **范围**: 只评审，未改动任何生产代码 / 配置 / 构建脚本
> **基线对比**: `docs/compliance/misra-deviation-report.md` v1.4、`.yuleosh/ci-config.yaml` (C:2023 基线)

---

## 1. 结论摘要

**整体合规水平: 不合规（存在已确认真实违规），但高风险违规集中、可快速收敛。**

| 指标 | 数值 |
|:-----|:-----|
| 扫描文件数 | 556 (.c/.h)，194.7 KLOC |
| 有发现的文件数 | 453 |
| 原始 MISRA 发现（去重后） | **25,136** |
| ├ Mandatory | 3,605（全部为 Rule 17.3，**抽样证实为工具误报主导**） |
| ├ Required | 5,534（其中误报/已登记偏差占大头） |
| └ Advisory | 15,997 |
| misra-config 噪声（未计入） | 6,209 |
| 密度 | ~129 条/KLOC（与 CI C:2023 基线 119.4/KLOC 同量级） |

**抽样复核结论（按官方分类，非仓库 rule-texts 口径）：**

1. **Mandatory 层面**：3,605 条 Rule 17.3（函数隐式声明）抽样 6 处全部为误报——被调函数原型经头文件可见（如 `Det_ReportError` 在 `Det.h:212` 声明，`CanIf.c:28` 已包含），是 cppcheck addon 在配置宏未完全解析时的已知局限（同批还有 6,209 条 misra-config 警告佐证配置解析不完整）。**没有证据表明存在真实 Mandatory 违规**；且 C99 编译器对隐式声明会告警/报错，构建链本身是兜底防线。
2. **最需要先处理的真实违规（Required）**：
   - **Rule 21.6/21.10（227+3 条）**：`src/bsw/mcal/lin/src/LinSlave_Hal.c` 等 5 个文件在生产代码中 `#include <stdio.h>/<time.h>` 并调用 `printf`（模拟 HAL 桩代码），这是确凿的库函数禁令违规；
   - **Rule 17.2（12 条/3 文件）**：`BswM.c:104` `BswM_EvaluateExpression` 真实递归调用（表达式树求值），Required 规则禁止递归；
   - **Rule 17.7 中未被既有偏差覆盖的部分**：214 条里 memcpy/memset/strncpy（85 条）属既有偏差 D-17.7-MEMCPY 模式；`TEST_ASSERT_*`（111 条）源于**测试文件混入生产树**（`src/bsw/services/someipxf/src/SomeIpXf_Test.c`，该文件单文件贡献 177 条 Required）；
   - **Rule 11.8（2 条）**：`NvM.c:1350`、`Rte_NvMInterface.c:293` 将 const 指针强转 `void*` 丢失 const 限定，真实且可用 `const void*` 修复；
   - **Rule 16.4（1 条）**：`CanSm.c:635` switch 缺 default 分支（仅 bl_secure_boot.c 有 16.4 偏差，不覆盖此处）；
   - **Rule 9.3（5 条）**：`IpduM_Lcfg.c:40` 等数组部分初始化 `{0x00}`（8 元素数组只初始化 1 个）；
   - **Rule 7.2（34 条）**：`Spi.c:571` `0xFFFFFFFF` 等无 `U` 后缀无符号常量；
   - **Rule 3.1（2 条）**：`Eth_Private.h:94` 注释内嵌套 `/*`；**Rule 21.1（1 条）**：`src/bsw/mcal/lin/include/Std_Types.h:104` 重复定义保留宏 `NULL`；**Rule 2.2（4 条）**：`DoIP_Lcfg.c:127` 冗余 `+ 0`。
3. **误报主导的规则（不计入真实违规）**：Rule 8.4（2,327 条，extern 声明实际可见，如 `CanIf_Cfg.h:142`）、Rule 20.9（1,881 条，`.h` 独立扫描缺少 Cfg.h 上下文所致）、Rule 2.5/2.3/2.4（约 9,600 条，配置头宏/类型被其他 TU 使用）、Rule 5.6/5.7/5.9（跨 TU 唯一性需全程序视图）、Rule 20.7（宏参数作结构体成员名无法加括号）、Rule 11.9（1 条，整数地址比较非指针）。
4. **既有偏差可覆盖的模式**：15.5（多返回，DP-AUTOSAR-001）、15.7（if-else-if 无收尾 else，DP-AUTOSAR-013）、17.7-mem*（D-17.7-MEMCPY）、12.1、7.2 部分、11.6（lwIP 回调惯用法，但现有 11.6 偏差理由写的是"硬件寄存器"，**理由与实际用途不符，需修正偏差描述**）、8.6（LINIF_WEAK 弱符号覆盖模式，有意设计）。

**与仓库既有口径的重大出入**：`docs/compliance/misra-deviation-report.md` v1.4 声称"Required rules 全部合规、0 真违规"——本轮扫描证伪：至少 **21.6/17.2/16.4/9.3/21.1/3.1/7.2 等约 280+ 条 Required 级真实违规**未被该报告覆盖（偏差报告成文于 yuleosh 工具链时代，工具与口径均不同）。CI 侧 `.yuleosh/ci-config.yaml` 的 C:2023 基线（14,494 required）反而更接近真实水平。

---

## 2. 工具与环境

| 项 | 值 |
|:---|:---|
| 工具 | Cppcheck 2.21.0 + 官方 misra addon (`/opt/homebrew/share/Cppcheck/addons/misra.py`) |
| 规则文本 | `.yuleosh/misra_texts.txt`（C:2012；**注意：该文件部分规则文本/分类与官方标准错位**，详见 §6.3） |
| C 标准 | `--std=c99`（仓库 CMake 口径；CI 用 c11，差异已记录） |
| 平台 | macOS arm64, `--platform=unix64`, `-j 4` |
| 分类口径 | MISRA C:2012 + AMD1/AMD2 官方 Mandatory/Required/Advisory（脚本内置映射表，非仓库 rule-texts 文件） |
| 抑制 | `tools/misra/cppcheck_suppressions.xml` 等效文本化（3 条 CanNm misra-config）+ `unusedFunction/missingInclude*` |
| 去重 | 按 (rule, file, line) 去重（.h 独立扫描与被 .c 包含会重复计数，原始 44,147 → 去重 25,136） |

**复现命令**（完整行参见日志目录）：

```bash
INCS=$(tr '\n' ' ' < build/misra-review-2026-11-02/includes.txt)   # 119 个 include 目录
cppcheck --enable=warning,style,performance,portability --std=c99 --language=c \
  --platform=unix64 $INCS -Isrc/common -Isrc/bsw/os \
  --inline-suppr --suppress=missingIncludeSystem --suppress=missingInclude \
  --suppress=unmatchedSuppression --suppress=unusedFunction \
  --suppressions-list=build/misra-review-2026-11-02/suppressions.txt \
  --addon=misra --addon=build/misra-review-2026-11-02/misra-addon.json \
  --xml --xml-version=2 --file-list=build/misra-review-2026-11-02/files.txt -j 4 \
  2> build/misra-review-2026-11-02/cppcheck-misra-c2012.xml
```

**覆盖范围**：`src/bsw/mcal`(139) + `src/bsw/ecual`(121) + `src/bsw/services`(259) + `src/bsw/os`(11) + `src/middleware/rte`(15) + `src/platform`(11) = **556 文件全覆盖，无缩减**。
**未覆盖**：`src/` 其余目录（application、autosar、bootloader、crypto_stack、diagnostics、ethernet、tsn、middleware/microdds 等，共约 297 文件），原因是超出本轮主目标范围；建议下一轮补齐。

**原始日志与数据**：

| 产物 | 路径 |
|:-----|:-----|
| 原始 XML 日志 (20MB) | `build/misra-review-2026-11-02/cppcheck-misra-c2012.xml` |
| 扫描文件清单 / include 清单 / addon 配置 / 抑制文件 / 解析脚本 | 同目录 `files.txt`, `includes.txt`, `misra-addon.json`, `suppressions.txt`, `analyze.py` |
| 规则统计 CSV / 文件统计 CSV / 汇总 JSON | `docs/reviews/misra-2026-11-02/rule-stats.csv`, `file-stats.csv`, `summary.json` |

**工具局限（影响判读）**：未加 `--force`，仅扫描默认预处理配置（`S32K312` 等平台宏未定义，部分 `#if` 分支未覆盖）；`.h` 独立扫描导致预处理类规则（20.9/2.5 等）系统性偏多；6,209 条 misra-config 警告表明配置解析不完整（可能存在漏报）。

---

## 3. 按规则统计（Top 25，全表见 rule-stats.csv）

| 规则 | 官方类别 | 数量 | 涉及文件 | 复核判定 |
|:-----|:------|-----:|-------:|:---------|
| 2.5 未使用宏 | Advisory | 8,993 | 332 | 误报主导（配置头被跨 TU 使用） |
| 15.5 多返回点 | Advisory | 3,885 | 142 | 真违规，已有偏差 DP-AUTOSAR-001 |
| 17.3 隐式声明 | **Mandatory** | 3,605 | 139 | **误报主导**（原型经头文件可见） |
| 8.4 定义前无可见声明 | Required | 2,327 | 196 | 误报主导（extern 在 Cfg.h 中） |
| 20.9 #if 标识符未定义 | Required | 1,881 | 186 | 误报主导（.h 独立扫描）+ 已登记偏差 |
| 5.9 内部链接名唯一 | Advisory | 1,362 | 145 | 误报/偏差（跨 TU 同名） |
| 20.1 预处理指令限制 | Advisory | 639 | 176 | 偏差（AUTOSAR include guard/MemMap） |
| 2.3 未使用类型声明 | Advisory | 392 | 117 | 误报主导 |
| 8.5 外部对象重复声明 | Required | 313 | 59 | AUTOSAR 头文件声明模型，偏差 DP 覆盖 |
| **21.6 禁用 stdio/stdlib** | **Required** | **227** | **5** | **真实违规（模拟桩 printf）** |
| 2.4 未使用标签声明 | Advisory | 221 | 78 | 误报主导 |
| **17.7 返回值未使用** | **Required** | **214** | **16** | **混合：mem* 偏差 / TEST_ASSERT 测试代码混入** |
| 5.6 typedef 名唯一 | Required | 189 | 51 | 偏差（AUTOSAR 命名约定）+ 跨 TU 误报 |
| 12.1 运算符优先级 | Advisory | 175 | 14 | 真违规，建议加括号（低风险） |
| 5.7 tag 名唯一 | Required | 106 | 41 | 偏差 DP-AUTOSAR-018 |
| 15.7 else-if 无收尾 else | Required | 86 | 45 | 真违规，偏差 DP-AUTOSAR-013 |
| 20.5 #undef | Advisory | 57 | 6 | 偏差（MemMap.h 模式） |
| 8.9 内部链接唯一定义 | Advisory | 51 | 32 | 误报/偏差 |
| 2.7 未使用参数 | Advisory | 46 | 21 | 偏差（AUTOSAR 签名统一） |
| 5.8 对象/函数名唯一 | Required | 38 | 31 | 偏差 DP-AUTOSAR-012 |
| 8.6 重复外部定义 | Required | 36 | 9 | 弱符号覆盖模式（LINIF_WEAK），需登记偏差 |
| **7.2 无符号常量缺 U** | **Required** | **34** | **3** | **真实违规（轻微）** |
| 12.3 ++/-- 混用 | Advisory | 30 | 13 | 真违规（低风险） |
| 15.1 goto | Advisory | 28 | 2 | 偏差 DP-AUTOSAR-008 |
| 12.2 移位范围 | Required | 21 | 4 | 多为 HW 寄存器模式，需逐条核实边界 |

其余小计数规则（≤20 条）：13.3、8.11、20.10、11.5、17.8、10.4、**17.2 递归(12,真实)**、10.5、15.4、9.3、7.3、2.2、9.5、21.10、8.8、3.1、16.6、9.2、11.8、20.7、11.6、11.4、21.1、16.4、20.13、11.9——判定见 §1 与 §4。

---

## 4. Top 问题清单（真实违规，按优先级）

### P0-1 Rule 21.6/21.10 — 生产代码使用 stdio/time（227+3 条）🔴

[src/bsw/mcal/lin/src/LinSlave_Hal.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/mcal/lin/src/LinSlave_Hal.c#L21-L47)

```c
#include <stdio.h>
#include <time.h>
...
(void)printf("[HAL] UART TX: 0x%02X\n", Data);
```

- **为什么是问题**：Rule 21.6 禁用 stdio.h（Required），Rule 21.10 禁用 time.h（Required）。MCAL 生产树中出现主机 I/O 库，交叉编译到目标板会直接失败或引入未定义行为；同属此问题的还有 `21.6` 涉及的其余 4 个文件（见 rule-stats.csv 与 XML）。
- **修复建议**：将模拟 HAL 移出 `src/bsw`（如 `tests/sim/` 或 `platform/sim/`），或改为可插拔接口 + 目标板实现；生产构建（S32K312 交叉编译）不应编译该文件。属于代码移动而非逻辑修改。

### P0-2 Rule 17.2 — 递归调用（12 条/3 文件）🔴

[src/bsw/services/bswm/src/BswM.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/services/bswm/src/BswM.c#L104-L105)

```c
result = (boolean)((BswM_EvaluateExpression(expr->LeftIndex, (uint8)(Depth + 1U)) == TRUE) &&
                   (BswM_EvaluateExpression(expr->RightIndex, (uint8)(Depth + 1U)) == TRUE));
```

- **为什么是问题**：递归在栈资源受限的 ECU 上是确定性安全风险（栈溢出不可静态界定），MISRA Required 禁止。虽有 `Depth` 参数，但深度上界依赖配置数据而非编译期常量。
- **修复建议**：改为显式栈/迭代求值（表达式树深度 ≤ 配置上限时用固定大小数组做工作栈）；若短期无法重构，须登记正式偏差并给出最大深度证明 + 栈裕量分析。

### P0-3 Rule 17.7 中测试代码混入生产树（~111 条）🔴

`src/bsw/services/someipxf/src/SomeIpXf_Test.c`（单文件 177 条 Required）：`TEST_ASSERT_EQ(...)` 返回值未使用。

- **为什么是问题**：该文件是单元测试，却在生产扫描范围内，既污染 MISRA 统计也会被生产构建链误编。`.yuleosh/ci-config.yaml` 的 `exclude_paths` 已含 `src/**/*_test.c`，但命名是 `_Test.c` 大写，**模式不匹配漏网**。
- **修复建议**：文件移入 `tests/unit/someipxf/`；或统一命名 `*_test.c` 对齐 CI 排除规则。无需改代码逻辑。

### P1-1 Rule 11.8 — 丢失 const 限定（2 条）🟠

[src/bsw/services/nvm/src/NvM.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/services/nvm/src/NvM.c#L1350)

```c
jobEntry.DataPtr = (void*)NvM_SrcPtr;   /* NvM_SrcPtr 为 const 限定源指针 */
```

同型：`src/middleware/rte/src/Rte_NvMInterface.c:293`。
- **为什么是问题**：const→void* 强转后，下游写操作将产生未定义行为（源数据可能在只读段）。
- **修复建议**：`DataPtr` 字段类型改 `const void*`（队列只读消费），patch 示意：

```diff
-    jobEntry.DataPtr = (void*)NvM_SrcPtr;
+    jobEntry.DataPtr = (const void*)NvM_SrcPtr;   /* 同步修改 JobEntry.DataPtr 类型 */
```

### P1-2 Rule 16.4 — switch 缺 default（1 条）🟠

[src/bsw/services/cansm/src/CanSm.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/services/cansm/src/CanSm.c#L635-L644)：`switch (netState->SubState)` 无 default。枚举扩展或数据损坏时静默落入无处理路径。建议补 `default: Det_ReportError(...); break;`。注：现有 16.4 偏差仅限 `bl_secure_boot.c`，不覆盖本例。

### P1-3 Rule 9.3 — 数组部分初始化（5 条）🟠

[src/bsw/ecual/ipdum/src/IpduM_Lcfg.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/ecual/ipdum/src/IpduM_Lcfg.c#L40)

```c
static uint8 IpduM_TxMuxPdu0_Dyn0_Data[8] = {0x00};   /* 8 元素仅显式初始化 1 个 */
```

- **修复建议**：`{0U,0U,0U,0U,0U,0U,0U,0U}` 或用指定的全量初始化器；语义不变（C 保证剩余清零），纯合规改写。

### P1-4 Rule 7.2 — 无符号常量缺 U 后缀（34 条/3 文件）🟠

[src/bsw/mcal/spi/src/Spi.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/mcal/spi/src/Spi.c#L568-L571)

```c
return (current >= StartTime) ? (current - StartTime) :
       ((0xFFFFFFFF - StartTime) + current);   /* 0xFFFFFFFF 无 U 后缀 */
```

- **修复建议**：`0xFFFFFFFFU`；顺带建议用 `UINT32_MAX`（<stdint.h>）。机械性修复，`tools/misra/fix_*` 有同类脚本可参考。

### P2-1 Rule 3.1 — 注释嵌套（2 条）🟡

[src/bsw/mcal/eth/include/Eth_Private.h](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/mcal/eth/include/Eth_Private.h#L93-L96)：装饰性注释框中 `/*` 出现在注释体内（`/*====...` 后紧跟 `/* TX BUFFER ...`）。改为 `* TX BUFFER DESCRIPTOR` 即可。

### P2-2 Rule 21.1 — 重定义保留宏 NULL（1 条）🟡

[src/bsw/mcal/lin/include/Std_Types.h](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/mcal/lin/include/Std_Types.h#L104-L109)：模块私有的第二份 `Std_Types.h` 重定义 `NULL`/`NULL_PTR`。除违规外还违反单一定义原则（全仓已有权威 `Std_Types.h`）。建议删除该私有副本，统一引用公共头。

### P2-3 Rule 2.2 — 冗余运算（4 条）🟡

[src/bsw/ecual/doIP/src/DoIP_Lcfg.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/ecual/doIP/src/DoIP_Lcfg.c#L127)：`.rxPduId = DOIP_TCP_SOAD_RX_PDU_ID_BASE + 0` —— `+ 0` 无效果。删除即可。

### P2-4 Rule 11.6 — void*↔算术类型转换（2 条）🟡

[src/bsw/services/tcpip/src/TcpIp.c](file:///Users/ingeek/workspace/AUTOSAR/src/bsw/services/tcpip/src/TcpIp.c#L574)：`socketId = (TcpIp_SocketIdType)(uintptr_t)Arg;`（lwIP 回调惯用法）。技术上违规但无法避免；**现有 11.6 项目偏差理由是"硬件寄存器访问"，与本用例不符**，需修正偏差描述或补充条目。

### 需进一步核实（本轮未定性）

- Rule 12.2（21 条，HW 寄存器移位，如 `Crypto_HwTrng.c:707`）：抽样为寄存器位域拼值，模式合理，但移位量上界未逐一证明；建议逐条核实后登记偏差。
- Rule 7.3（4 条，`CanTp.c:1087` 等）：样本未见小写 `l` 后缀，疑似误报，待核。
- Rule 8.8（2 条，`LinTp_Lcfg.c:239`）：`extern const` 与定义处链接属性一致性待核。
- Rule 8.6（36 条）：已抽 `LinIf_TxConfirmation` 证实为 `LINIF_WEAK` 弱符号有意覆盖；其余 8 个文件待同类确认后统一登记偏差。
- Rule 10.4（14 条）：`Os.c:253` `sizeof` 参与运算的 essential type 不一致，结果已有显式 cast，轻微真实违规，可顺手修。

---

## 5. 误报与豁免说明

### 5.1 抽样证实的误报模式（不计入真实违规）

| 规则 | 数量级 | 误报机理 | 抽样证据 |
|:-----|-------:|:---------|:---------|
| 17.3 (Mandatory) | 3,605 | addon 配置解析不完整时丢失跨模块原型可见性 | `Det.h:212` 有 `extern Std_ReturnType Det_ReportError(...)`，`CanIf.c:28` 已 `#include "Det.h"` |
| 8.4 | 2,327 | Lcfg.c 定义的 extern 声明在 Cfg.h 中，addon 未关联 | `CanIf_Cfg.h:142` `extern const CanIf_HohCfgType CanIf_HohCfg[...]` |
| 20.9 | 1,881 | .h 独立扫描缺 Cfg.h 上下文；`#if (X == STD_ON)` 模式 | `CanNm.h:333`，`CANNM_VERSION_INFO_API` 定义于 Cfg.h |
| 2.5/2.3/2.4 | ~9,600 | 配置头的宏/类型由其他 TU 消费，单 TU 分析必然"未使用" | 全部 `*_Cfg.h` |
| 5.6/5.7/5.9 | ~1,650 | 跨 TU 唯一性需全程序视图，超出单 TU 分析 | `ComStack_Types.h:83` |
| 20.7 | 2 | 宏参数作结构体成员名（`.field`），加括号非法 | `TcpIp.c:76` |
| 11.9 | 1 | 整数地址与 `0U` 比较，非指针 | `NvM_EccHandler.c:533` |
| misra-config | 6,209 | 工具自身配置不完整提示，非违规 | — |

### 5.2 沿用/引用既有偏差（仓内已登记）

15.5(DP-AUTOSAR-001)、17.7-mem*(D-17.7-MEMCPY)、15.7(DP-013)、5.8(DP-012)、5.7(DP-018)、20.1/20.5/20.10(AUTOSAR 头文件模式 PRJ-001~005)、15.1-goto(DP-008)、19.2(DP-009)、8.13(DP-006)、12.1/2.7/10.8 等（`.yuleosh/ci-config.yaml` deviations 区）。

### 5.3 仓库判定口径本身的问题（评审发现）

1. `.yuleosh/misra_texts.txt` 规则文本与官方错位（如 2.2/2.3/3.1 文本为相邻规则内容），且把官方 Required 的 5.6/5.7 标为 Advisory——本报告统计一律以官方分类为准。
2. `docs/compliance/misra-deviation-report.md` 把官方 Required 的 **17.7 标为 Advisory**（DP-AUTOSAR-002），导致 Required 级违规被按 Advisory 放行；与"Required 清零"结论相互矛盾。
3. `.yuleosh/misra-addon-config.json` 内 rule-texts 指向失效绝对路径 `/Users/stefan/...`，CI 实际在无规则文本下运行（不影响规则 ID，但消息不可读）。
4. `.github/workflows/misra-check.yml` 解析脚本发现 required 违规后 `sys.exit(0)`（迁移期放行之笔），与 `.yuleosh/ci-config.yaml` `fail_on_required: true` 口径冲突。
5. CI 排除模式 `src/**/*_test.c` 漏掉 `*_Test.c` 命名（见 P0-3）。

---

## 6. 分阶段整改计划

> 本轮未改动任何代码；以下为建议计划，按"风险/工作量"排序。每阶段完成后用 §2 命令回归扫描并对比 `rule-stats.csv`。

| 阶段 | 内容 | 对应条目 | 预计消减 |
|:-----|:-----|:---------|--------:|
| **P0** | ① LinSlave_Hal 等模拟桩移出生产树（21.6/21.10）② SomeIpXf_Test.c 移入 tests/ 或改名对齐 CI 排除 ③ BswM 递归改迭代或登记带深度证明的偏差 | §4 P0-1/2/3 | ~370 条 Required + 3,605 Mandatory 误报澄清后重新确认 |
| **P1** | ④ 11.8 两处 const 修复 ⑤ CanSm default ⑥ 9.3 全量初始化 ⑦ 7.2 加 U 后缀（机械修复，可借 `tools/misra/fix_*`）⑧ 17.7 中 TEST_ASSERT/printf 随 P0 消除后，剩余 mem* 类补 `(void)` 或扩偏差 | §4 P1 | ~260 条 Required |
| **P2** | ⑨ 3.1/21.1/2.2 琐碎修复 ⑩ 修正 11.6 偏差理由、登记 8.6 弱符号偏差、核实 12.2/7.3/8.8/10.4 ⑪ 修仓内口径问题（§5.3 五项：rule-texts 文本、17.7 分类、addon 失效路径、CI exit(0)、排除模式大小写） | §4 P2、§5.3 | 剩余真实小项 + 流程可信化 |
| **P3** | ⑫ Advisory 大头按既定偏差归档（15.5/2.5/5.9/20.1 等），更新 deviation-report 至 v1.5（剔除"Required 清零"表述，改为真实基线）⑬ 扩展扫描至 src/ 全量 853 文件并纳入 `--force` 多配置 | — | 统计口径收敛 |

**回归验证与 CI 接入建议**：

1. 短期（C:2012 口径）：把本报告 §2 命令封装为 `tools/run_misra_check.sh` 的 `--addon` 模式（现脚本只跑 cppcheck 核心检查，未挂 misra addon），输出同格式 XML+CSV，纳入 PR 检查（先 advisory-mode）。
2. 与现行 C:2023 CI 的关系：CI 已迁 C:2023（`.github/workflows/misra-check.yml`）。建议本报告 P0/P1 真实违规按 C:2012 编号修复（编号在 C:2023 基本一致），随后以 C:2023 基线为门禁、C:2012 报告为合规档案双轨维护；不要回退 C:2023 迁移成果。
3. 门禁阈值：修复期在 `.yuleosh/ci-config.yaml` 用当前真实值重建 baseline（禁止上调 `fail_threshold` 掩盖新增违规）；每阶段验收标准为对应规则计数单调不增。
4. 误报治理：把 §5.1 已证实误报模式写入 `tools/misra/cppcheck_suppressions.xml`（按 file+rule 精确抑制，禁止全规则通配），防止下轮统计再被淹没。

---

*附：评审严格限定为只读分析 + 新增报告/日志；未执行任何 git 提交/分支操作；未触碰覆盖率阈值等仓库硬约束。*
