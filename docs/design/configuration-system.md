# 配置系统设计

## 配置层次

```
┌──────────────────────────────────────────────┐
│              编译时配置 (Pre-compile)                    │
│  - *.h 头文件中的宏定义                                  │
├──────────────────────────────────────────────┤
│              链接时配置 (Link-time)                       │
│  - *_Lcfg.c 配置表                                      │
├──────────────────────────────────────────────┤
│              后构建配置 (Post-build)                      │
│  - *_PBcfg.c 配置结构体                                │
└──────────────────────────────────────────────┘
```

## 配置文件结构

```
src/bsw/services/<module>/
├─── include/
│   ├─── <Module>_Cfg.h      /* 编译时配置 */
│   └─── <Module>.h          /* 公共头文件 */
├─── src/
│   ├─── <Module>.c          /* 主源文件 */
│   └─── <Module>_Lcfg.c     /* 链接时配置 */
```

## 配置工具链

```
ARXML → 生成器 → 配置代码 (C文件)
```

## P0/P1 缺陷修复结论

> 更新日期: 2026-11-15
> 关联提交: `8586b0e`（配置修正）、`71130e0`（Dcm/PduR 生成与校验）、`a4bbdfe`（Com 构建接入）

| 编号 | 级别 | 缺陷现象 | 根因 | 修复结论 |
|:-----|:-----|:---------|:-----|:---------|
| P0-1 | P0 | 生成的 Com 信号表越界访问：`Com_GetSignalConfig()` 直接以 `SignalId`（0..255）索引，而生成表仅 2 个元素 | 模板将 `Com_Signals` 声明为无界数组，丢失了生产配置的可寻址容量 | `config/templates/Com_Lcfg.c.j2` 恢复 `Com_Signals[COM_MAX_SIGNALS]`；保留 2 个显式初始化项与 `.NumSignals = 2U`，不虚构业务信号；生成器回归断言固化该容量约束 |
| P1-1 | P1 | Dcm 协议配置可声明重复 Rx/Tx 端点或非连续协议 ID，运行时按 `ProtocolId` 直接索引 `ProtocolStates[]` 时不可用 | 语义校验仅覆盖 ID/名称唯一性与容量，缺少端点唯一性和运行时索引约束 | `config/tools/generate_bsw_config.py` 新增 Rx/Tx 端点唯一性校验与稠密零基 ID 校验；两个回归用例遵循先失败后通过 |
| P1-2 | P1 | PduR 路由与路由组缺乏可验证的显式模型，生成源与历史静态源并存 | 配置仅以容量整数表达，Schema 未要求完整路由元素 | `config/bsw_config.json` 显式建模 7 条路由与 4 个路由组；Schema 要求完整端点字段；语义校验覆盖跨模块 PDU 引用与组引用，生成 `config/generated/PduR_Lcfg.c` |
| P1-3 | P1 | Dcm/PduR/Com 的链接时配置存在生成源与静态源双重来源 | 构建未统一以生成结果为权威输入 | 主构建 `src/bsw/services/CMakeLists.txt` 与独立构建 `tools/build/CMakeLists.txt` 统一链接 `config/generated/*_Lcfg.c`；Dcm 仅保留生产证据支持的 `OBD_ON_CAN`（id 0 / Rx 6 / Tx 5） |
| P1-4 | P1 | 回归过程中暴露的构建与测试缺陷，阻断全量验证 | 见下列逐项根因 | 均已按根因修复，未放宽 `-Werror` 或测试阈值 |

P1-4 逐项修复：

| 子项 | 根因 | 修复 |
|:-----|:-----|:-----|
| `eth_utils.c` 编译失败 | C99 下 `sscanf` 未声明 | 补充 `<stdio.h>` |
| Clang 17 严格目标编译失败 | 不支持全局 `-Wno-unterminated-string-initialization`，被 `-Werror` 提升为错误 | CMake 使用 `check_c_compiler_flag` 探测后再追加，保留严格告警 |
| LIN 从机测试编译失败 | 同一回调被两个参数顺序相反的接口共用 | 拆分为原始错误回调与配置表适配回调 |
| LIN 主机测试 teardown 段错误 | 调度器保留指向栈上配置表的指针，测试返回后失效 | 测试夹具改为 `static`，保持生产生命周期契约不变 |
| 性能目标编译失败 | 仍强制包含已删除的 Com 255 信号影子头 | 移除陈旧包含，仅保留有意为之的 CanIf 影子配置 |
| PduR 覆盖脚本编译失败 | 仍编译已删除的 legacy `PduR_Lcfg.c` | 改为先生成配置，再编译 `config/generated/PduR_Lcfg.c` |

## 全工程回归验证结论

配置变更会影响全部构建目标，因此按全工程口径回归，结论如下：

| 验证项 | 验证方式 | 结论 |
|:-------|:---------|:-----|
| 配置生成 | `python config/tools/generate_bsw_config.py` | 通过 |
| 模板漂移 | 95 项模板逐项比对 | 无漂移 |
| 生成器单元测试 | `config/tools/test_generate_bsw_config.py` | 17 项通过 + 11 个子测试通过 |
| 主工程构建 | `cmake --build /tmp/yule-main-regression-20261114` | 退出码 0 |
| 独立工程构建 | `cmake --build /tmp/yule-standalone-regression-20261114` | 退出码 0 |
| 全量 CTest | `ctest --test-dir ... --output-on-failure` | 210/210 通过（串行） |
| 配置器 API 测试 | pytest | 12/12 通过 |
| 前端生产构建 | `npm run build`（`vue-tsc && vite build`） | 退出码 0 |
| 独立代码复审 | 全量 diff 复审 + 聚焦复审 | 无 Critical / Important 遗留问题 |

补充说明：

- 并行执行全量 CTest 时，`os_benchmark` 的 p99 唤醒抖动曾超过 5 ms 阈值；该项测量宿主机调度抖动，单独运行与串行全量运行均通过，判定为并行负载导致的宿主机噪声，未修改阈值或实现。
- 前端 Vite 的 `:deep()` 与 chunk 体积告警为既有告警，本次未以放宽配置的方式消除。
- 前端 ESLint 因项目缺少 `eslint.config.*` / `.eslintrc*` 无法启动，lint 结论保持“未验证”，不记作通过。
- `scripts/analysis/batch10_coverage.sh` 的 checkout/路径限制为既有问题，本次仅验证脚本语法与 PduR 生成步骤，未宣称端到端通过。

**总体结论**：配置系统 P0/P1 缺陷已按根因修复，生成源已成为主工程与独立工程的权威配置输入，全工程回归通过；变更已固化为本地提交 `8586b0e`。
