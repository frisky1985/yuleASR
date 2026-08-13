# OTA 健康确认钩子 + DOWNLOADING 落盘 — 独立审查报告

> 日期: 2026-08-13 | 审查人: 小马 (质量架构师, Evaluator)
> 基线: master `bb77e566` | 审查对象: spec-delta-ota-healthcheck-20260813.md 的 P1 (RS-OTA-05) + P2 (RS-OTA-06) 实现
> 改动: 11 文件 +944/-49 (与 checkpoint 声明一致) | 验证: 独立构建 + ctest 48/48 + 新增用例逐项运行 + 结构编译期实测

---

## 一、结论：有条件批准（评分 85/100）

- **P1 RS-OTA-05 健康门控：通过。** 实现忠实于 spec，向后兼容真实可证（门控关 = 原 N 次制），ConfirmHealth 语义完整，测试充分且走真实代码路径。
- **P2 RS-OTA-06 DOWNLOADING 落盘：有条件通过。** BIB 生命周期/失败恢复/旧 BIB 兼容全部正确，但 **Boot_Loader 的 DOWNLOADING 回退逻辑在「Slot B 已为活动槽」场景强制启动 Slot A（半擦除目标），违反 spec「启动活动槽（旧固件照常运行）」**（P1-1）。需修复或与集成方确认生产槽位模型后批准。

**批准条件：**
1. 处理 P1-1（移除 override 或 BIB 记录目标槽，二者取一，并同步修正受影响测试断言）；
2. 主代理在 Configurator 侧跑 `linkage-regression.sh` 确认（本工作区无 Configurator 仓库，结构上零破坏面，但未实际执行）。

---

## 二、SHALL 逐条核对表

### P1 / RS-OTA-05

| # | 条款 | 结果 | 证据 |
|:--|:-----|:-----|:-----|
| 1 | 槽位记录 v2→v3 增 `health_confirmed`，CRC 范围随之扩展 | ✅ 通过 | `bl_antrollback_slot_t` 32B / crc32@28 / health@24（编译期实测）；CRC 前 7 字段；v2 兼容读为独立 28B 布局（crc32@24，前 6 字段），两遍扫描以 `found_v3` 门控 |
| 2 | 提交条件：门控开 → boots≥N **且** health_confirmed；门控关 → 原 N 次制 | ✅ 通过 | `NotifySuccessfulBoot` 双条件提交；门控关路径与基线逐字节一致（测试①回归钉住） |
| 3 | 新增 `Boot_AntiRollback_SetHealthCheckMode(ctx, enabled)` | ✅ 通过 | 运行期配置不持久化（符合 spec），默认关闭 |
| 4 | 新增 `ConfirmHealth(ctx, version, ok)`：ok&&ver==pending → true 落盘；ok==false → false 落盘（pending 保持）；版本不匹配不落盘；无 pending 无操作 | ✅ 通过 | 语义完整；错误透传；`ok==false` 不校验版本（spec 未要求，合理） |
| 5 | `sbl_main_boot` 验签后门控开不立即 notify，关走原 N 次制 | ✅ 通过 | `sbl_main_boot` ⑤a 分支；门控开时 App 经 `ConfirmBusinessHealth` + `NotifyBootSuccess` 完成确认/计数（薄封装真实转发已验证） |
| S1 | `Boot_Update` 薄封装 `ConfirmBusinessHealth(boolean)` | ✅ 通过 | 双封装经注入接口转发；未注入无操作；NULL fn-ptr 防护；get_pending 取版本 |
| S2 | 未确认 → pending 保持 → watchdog/boot_attempt 超限回滚 | ✅ 通过 | 复用现有 bl_rollback 兜底（无新代码，符合 spec 表述） |
| M1 | `IsHealthConfirmed(ctx)` 查询 | ✅ 通过 | 已实现（未初始化/NULL 返回 false） |
| G/T | 门控关闭行为与现有一致（向后兼容） | ✅ 通过 | 测试① + ctest 48/48 零破坏（独立复现） |

### P2 / RS-OTA-06

| # | 条款 | 结果 | 证据 |
|:--|:-----|:-----|:-----|
| 1 | `Boot_InfoBlock.update_state` 复用 reserved，结构尺寸/crc32 偏移不变，旧 BIB 兼容读 | ✅ 通过 | 68B / crc32@64 / update_state@40（编译期实测）；reserved 缩为 [23]；旧 reserved=0 → IDLE |
| 2 | 枚举 IDLE=0 / DOWNLOADING=1 / PENDING=2 | ✅ 通过 | `Boot_UpdateState` |
| 3 | Prepare：confirm_gate 通过后、擦除**之前**写 DOWNLOADING；擦除失败恢复 IDLE | ✅ 通过 | `Boot_Update.c:220`（先落盘）→ `:226`（擦除）；失败路径 `:230` 恢复 IDLE，测试注入擦除失败真实覆盖 |
| 4 | Finalize：校验通过 → PENDING；全部成功 → IDLE | ✅ 通过 | 6 处失败路径全部恢复 IDLE，无卡死态；成功路径 IDLE 落盘 |
| 5 | 启动决策：DOWNLOADING → 忽略 pending，启动活动槽 | ⚠️ **部分通过** | Boot_Loader 有实现，但**强制 Slot A**而非遵循 marker（0x02）语义 → **P1-1**；`sbl_main_boot` 无显式检查（架构性兜底 → P2-4） |
| S1 | `bib_pending_valid` 同步识别 DOWNLOADING | ✅ 通过 | `Boot_Update.c:69`；旧 BIB 0=IDLE 兼容（测试真实覆盖） |
| G/T | ① 中断回退活动槽 ② PENDING 正常进入验证 ③ IDLE 无残留 | ✅ 通过 | 主场景（active=A）①✅；②③ 由生命周期测试覆盖 |

---

## 三、发现的问题

### P1-1【功能缺陷】Boot_Loader DOWNLOADING 回退强制 Slot A，违反「活动槽」语义

- **位置**：`src/bsw/boot/src/Boot_Loader.c:163-169`（`ResolveBootTarget` 内 DOWNLOADING 覆盖 `use_slot_b = 0U`）；测试固化点 `src/bsw/boot/test/test_boot_update_confirm.c:411-425`
- **场景**：`status & 0x02`（Slot B 选中/活动）为 1 时发生中断升级（DOWNLOADING）→ 代码强制启动 **Slot A**。但此时 0x02 位由 `Boot_Update_SwapSlots` 置位（写后切模型，Finalize 之后才切换），**中断的升级不会翻动该位** → marker=1 表示当前活动槽是 B，本次升级目标槽是 A。强制 A = 启动**半擦除的升级目标**，而非"旧固件照常运行"。
- **实际影响链**：A 失败 → B 活动（marker=1）→ OTA 修复 A → 写入中断掉电 → 重启启动半擦除 A → 验签失败 → boot_count 递增进 recovery；**完好的 B 被跳过**——恰好击穿 P2 的掉电保护意图（该场景恰是 P2 要保护的场景之一）。
- **根因**：BIB 未记录升级目标槽，回退逻辑只能猜；"强制 A"假设 A 恒为活动槽，在 marker=1 时猜错。
- **建议**（二者取一）：
  - (a) **移除 override**：marker 本身已表达活动槽，中断升级不翻 marker → DOWNLOADING+marker=0 启动 A ✓、DOWNLOADING+marker=1 启动 B ✓，两种 marker 态均正确；或
  - (b) BIB 再记一个目标槽字节，回退到"非目标槽"，彻底消除模型歧义。
  - 无论哪种，需与集成方确认生产 SwapSlots/Prepare 时序；若采用 (a)，`test_update_state_lifecycle` 的"DOWNLOADING+marker=1 → 启动 SBL/Slot A"断言需改为"→ 启动 Slot B"。

### P2-2【设计小缺陷】ConfirmHealth 版本不匹配返回 OK，调用方无法区分"已确认/被忽略"

- **位置**：`src/bootloader/bl_antrollback.c` `ConfirmHealth`（version 不匹配 → `BL_ANTIROLLBACK_OK`）
- **影响**：`Boot_Update_ConfirmBusinessHealth` 对 App 永远返回 BOOT_OK，App 无法感知"确认被拒"。建议返回独立错误码（如 `VERSION_MISMATCH`），影响小。

### P2-3【效率/窗口】Finalize 成功路径 BIB 双写

- **位置**：`src/bsw/boot/src/Boot_Update.c`（`bib_write(&bib)` 后再 `bib_set_update_state(IDLE)`，两条注入/非注入路径均如此）
- **影响**：两次 flash 写；两次写之间掉电 → 重启见 PENDING + 已提交 counter → 走候选验证流程，**无害但非最优**。建议成功路径在 `bib_write` 前把 `update_state` 置 IDLE 一次性写入。

### P2-4【集成注意】`sbl_main_boot` 无 DOWNLOADING 显式检查

- spec 措辞含 `sbl_main_boot`，但实现仅在 `Boot_Loader.ResolveBootTarget`。架构性兜底成立：sbl_main 不做槽位选择（`app_image` 由集成层配置），Boot_Loader 是唯一 BIB 启动决策点；但**若集成层绕过 Boot_Loader 直接配置目标槽为 app_image，保护缺失**。建议在集成文档注明接线约束。低风险。

### P2-5【部署注意】v2→v3 槽位步长 28B→32B

- 若生产 NVM 区域按 v2 精确分配（N×28B），v3 首次写入可能越出区域。按生产要求"每槽一扇区"则无碍。部署注意项，非代码缺陷。

### P2-6【测试缺口】v2→v3 混合布局重扫无用例钉住

- `test_antrollback_v2_record_compat` 覆盖纯 v2 读 + v3 写，但未覆盖"v3 写入后再 Init 的混合布局重扫"。逻辑经推演正确（found_v3 门控；32B 步长下残留 v2 记录因 CRC 错位必然判无效，无假阳性），建议补用例防回归。

---

## 四、测试真实性核查结果

| 项目 | 结果 | 证据 |
|:-----|:-----|:-----|
| 新增用例真实执行 | ✅ | 独立运行：`test_bootloader` **25/25**（Health Gate PASSED + v2 Record Compat PASSED 均有运行输出）、`test_sbl_main` **7/7**、`test_boot_update_confirm` **11/11**、`test_e2e_antrollback` **6/6** —— 与 checkpoint 声明完全一致，非 skip/disable |
| 非 mock 假装 | ✅ | P1 用例走**真实 bl_antrollback** 代码路径（mock_flash 仅为 NVM 存储桩）；P2 用例走**真实 Boot_Update/Boot_Loader**（Boot_Flash 为 RAM 桩，故障注入真实触发擦除失败分支）；唯一真 mock 是 `test_confirm_business_health_forward` 的注入接口桩——用于测薄封装转发，属 mock 正确用途 |
| 边界覆盖 | ✅ 良好 | 门控开/关 × 确认/拒绝/版本不匹配/跨重启持久化、v2 兼容读（计数器+待确认状态+提交）、DOWNLOADING 生命周期、擦除失败恢复 IDLE、未注入无操作、NULL/未初始化参数。缺口见 P2-6 |
| RED→GREEN | ⚠️ 部分可信 | GREEN 已独立复现（构建 0 error + 48/48）；RED 无留存工件（未 commit，自述 17 处编译错误），无法独立复核。建议后续任务保留 RED 快照 |
| 结构不变性 | ✅ | `Boot_InfoBlock` 68B/crc32@64/update_state@40、`bl_antrollback_slot_t` 32B/crc32@28/health@24 —— 独立编译实测确认 |

---

## 五、回归风险

- **ctest 48/48 零破坏**：独立复现（基线 48 保持，新增用例挂在既有 target 内部）。
- **接口兼容**：`sbl_main.h` 未改动（sbl_main API 不变）；`bl_rollback_storage_api_t` 仅表尾追加 2 个成员，唯一实现（bl_antrollback）与测试 mock 均用 designated initializer，编译通过——向后兼容成立；新旧错误码枚举值逐位对齐（0..-6），adapter 强转安全。
- **Configurator 联动**：`Boot_Cfg.h` 零改动（独立确认），无新宏、无 schema 影响；`linkage-regression.sh` 未跑（Configurator 仓库不在本工作区），结构上无破坏面，需主代理在 Configurator 侧补跑一次。

## 六、验收标准核对

| 验收项 | 状态 |
|:-------|:-----|
| ctest 全绿（48/48，含新增用例） | ✅ 独立复现 |
| 新增单测：健康门控开/关、ConfirmHealth true/false、DOWNLOADING 中断回退、v2→v3 兼容读 | ✅ 全覆盖（另含擦除失败恢复、版本不匹配、跨重启持久化） |
| 全量 native 构建 0 error | ✅ 独立复现 |
| linkage-regression.sh 仍全绿 | ⚠️ 未跑（外部仓库）；Boot_Cfg.h 零改动 → 结构无影响 |

---

## 七、给主代理/小克的行动项

1. **P1-1 决策**：确认生产槽位模型（SwapSlots 时序）→ 按建议 (a) 或 (b) 修复 Boot_Loader 回退逻辑，同步修正 `test_update_state_lifecycle` 断言；或由小明裁决是否接受当前语义。
2. **P2-2/P2-3**：低优先级改进，可随下个迭代处理。
3. **Configurator 侧**：跑 `linkage-regression.sh` 确认联动。
4. 审查通过后按流程 commit/push（本审查未动工作区、未 commit）。

*— 审查完毕。工作区保持只读未动。*
