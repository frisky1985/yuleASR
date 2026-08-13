# OTA 健康确认钩子 + DOWNLOADING 落盘 — 实现 Checkpoint

> 日期: 2026-08-13 | 任务: spec-delta-ota-healthcheck-20260813.md (P1 RS-OTA-05 + P2 RS-OTA-06)
> 基线: master `bb77e566` | 状态: **实现完成, 全量构建 0 error, ctest 48/48 全绿**
> ⚠️ 未 commit/push (按任务要求, 主代理统一收尾)

---

## 一、改动文件清单 (11 个文件, +944/-49)

| 文件 | 改动 |
|:-----|:-----|
| `src/bootloader/bl_antrollback.h` | 记录版本 v2→v3; 槽位新增 `health_confirmed` (CRC 注释 "前 6 字段"→"前 7 字段"); 上下文新增 `health_check_enabled`/`health_confirmed`; 新增 3 个 API 声明 |
| `src/bootloader/bl_antrollback.c` | 版本感知 CRC 校验 (v2/v3); 两遍扫描兼容 v2 布局 (v2 步长 28B + v2 记录布局, health_confirmed 置 false); `write_slot` 增 health 参数; Stage 重置确认标记; 提交条件含健康门控; 新增 SetHealthCheckMode/ConfirmHealth/IsHealthConfirmed; 存储接口表扩展 2 个适配器 |
| `include/autosar/bl_rollback_storage.h` | `bl_rollback_storage_api_t` 新增 `set_health_check_mode` / `confirm_health` (纯接口头, 向后兼容旧实现 — 仅 bl_antrollback 一家实现) |
| `src/bsw/boot/include/Boot_Types.h` | `Boot_InfoBlock` 新增 `update_state` (复用 reserved 首字节, 68B / crc32@64 不变); 新增 `Boot_UpdateState` 枚举 (IDLE=0/DOWNLOADING=1/PENDING=2) |
| `src/bsw/boot/include/Boot_Update.h` | 新增薄封装 `Boot_Update_SetHealthCheckMode(boolean)` + `Boot_Update_ConfirmBusinessHealth(boolean ok)` |
| `src/bsw/boot/src/Boot_Update.c` | `bib_set_update_state` 辅助; Prepare: 擦除前落盘 DOWNLOADING, 擦除失败恢复 IDLE; Finalize: 校验通过落盘 PENDING, 全部成功落盘 IDLE, 失败路径恢复 IDLE; `bib_pending_valid` 识别 DOWNLOADING; 两个薄封装实现 (经注入接口转发, 未注入无操作) |
| `src/bsw/boot/src/Boot_Loader.c` | `ResolveBootTarget`: `update_state==DOWNLOADING` → 忽略 Slot-B 切换标记, 回退活动槽 (旧 BIB 读为 IDLE 行为不变) |
| `src/bootloader/main/sbl_main.c` | `sbl_main_boot` 验签通过后: 门控关闭 → 原 N 次制立即 notify; 门控开启 → 不 notify 等 App 确认; `sbl_main_connect_antrollback` 经注入接口同步门控状态 (sbl_main API 不变) |
| `src/bootloader/tests/test_bootloader.c` | +2 用例: `test_antrollback_health_gate` (门控关/开 × 确认/不确认/拒绝/版本不匹配/持久化), `test_antrollback_v2_record_compat` (v2 布局兼容读) |
| `src/bootloader/tests/test_sbl_main.c` | +1 用例: `test_sbl_boot_health_gate` (门控开启 → boot 不上报, App 确认+上报 → 提交) |
| `src/bsw/boot/test/test_boot_update_confirm.c` | +4 用例: update_state 生命周期 (DOWNLOADING 落盘 → 中断回退活动槽 → Finalize 后 IDLE), Prepare 擦除失败恢复 IDLE, DOWNLOADING 使 pending 无效 (旧 BIB IDLE 兼容), 健康转发 (mock 注入接口); + 槽擦除故障注入 + mock 存储接口 |

## 二、新增/修改 API

### P1 健康门控 (RS-OTA-05)
- `void Boot_AntiRollback_SetHealthCheckMode(bl_antrollback_context_t *ctx, bool enabled)` — 运行期配置, 默认关闭 (向后兼容)
- `bl_antrollback_error_t Boot_AntiRollback_ConfirmHealth(ctx, uint32_t version, bool ok)` — ok&&version==pending → health_confirmed=true 落盘; ok==false → false 落盘 (pending 保持); 版本不匹配不落盘; 无 pending 无操作
- `bool Boot_AntiRollback_IsHealthConfirmed(ctx)` — MAY 项, 已实现
- 提交条件 (NotifySuccessfulBoot): 门控开 → `boots>=N && health_confirmed`; 门控关 → 原 `boots>=N` (默认向后兼容)
- 薄封装: `Boot_Update_SetHealthCheckMode(boolean)` / `Boot_Update_ConfirmBusinessHealth(boolean ok)` — 经 bl_rollback_storage_api_t 注入接口转发; 未注入无操作
- sbl_main: 门控开时 `sbl_main_boot` 不立即 notify (等待 App 确认)

### P2 掉电保护 (RS-OTA-06)
- `Boot_InfoBlock.update_state` (uint8, 复用 reserved 首字节) + `Boot_UpdateState` 枚举
- `Boot_Update_Prepare`: confirm_gate 通过 → **擦除前**写 BIB(DOWNLOADING) → 擦除; 擦除失败恢复 IDLE
- `Boot_Update_Finalize`: 校验通过 → BIB(PENDING); 全部成功 → BIB(IDLE); 失败路径恢复 IDLE
- `Boot_Loader_ResolveBootTarget`: DOWNLOADING → 忽略 pending/切换标记, 回退活动槽
- `bib_pending_valid`: DOWNLOADING → pending 无效 (旧 BIB 0=IDLE 兼容)

## 三、测试结果

- 全量构建: `cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build -j4` → **0 error**
- ctest: **48/48 全绿** (基线 48 保持零破坏, 新增用例挂载在既有 target 内部)
- 新增用例真实执行 (内部计数):
  - `test_bootloader`: **25/25** (原 23, +2) — Health Gate / v2 Record Compat
  - `test_sbl_main`: **7/7** (原 6, +1) — SBL Boot Health Gate
  - `test_boot_update_confirm`: **11/11** (原 7, +4) — Update State Lifecycle / Erase-Failure Restore / DOWNLOADING Invalidates Pending / Health Forwarding
  - `test_e2e_antrollback`: 6/6 (回归无破坏)

### RED→GREEN 证据
- RED: 先写全部新用例 → 构建失败 (compile errors: `update_state` 无成员、`BOOT_UPDATE_DOWNLOADING` 未声明、`set_health_check_mode`/`confirm_health` 非表成员、`Boot_Update_ConfirmBusinessHealth` 隐式声明等, 共 17 处) — 见首轮构建输出
- GREEN: 实现后全部用例通过, 无 mock 假装 (全部走真实 bl_antrollback/Boot_Update/Boot_Loader 代码路径)

## 四、遇到的问题与解决

1. **v2 兼容读 CRC 错位** (调试定位): v2 记录 crc32 在偏移 24, 读入 v3 结构后 `slot.crc32` 在偏移 28 (读到下一条记录 magic) → 第二遍扫描全部判无效。修复: 定义独立 `bl_antrollback_slot_v2_t` (28B), 按 v2 步长/偏移读校验。
2. **启动决策测试假绿**: 测试构造的 BIB magic=0 → `load_bib` 失败走 first-boot 路径返回 SBL, DOWNLOADING 断言"碰巧"通过。修复: 测试显式置 `bib.magic = 0x30424942U`, 使 DOWNLOADING 分支被真实覆盖 (含 IDLE 对照用例)。
3. **一次性确认授权终态语义**: Prepare 失败后 confirm_state 保持 GRANTED (终态, 原语义), 重试无需重新确认 → 测试改为直接重试 Prepare; 第二段升级流程需先 `Boot_Update_Abort()` 复位授权。
4. **结构布局核对**: `Boot_InfoBlock` 68B / crc32@64 / update_state@40 (reserved 首字节) 不变; `bl_antrollback_slot_t` 32B / crc32@28 / health@24 — 编译期实测确认。

## 五、遗留 / 待主代理确认

- **linkage-regression.sh 未跑**: 脚本位于 Configurator 仓库 (本 yuleASR 工作区不存在)。`Boot_Cfg.h` 零改动 (无新宏), 无 schema 影响, 判定联动无破坏; 建议主代理在 Configurator 侧跑一次确认。
- **未 commit/push** — 按任务要求留待主代理统一收尾。
- P1 SHOULD (watchdog 兜底回滚) 复用现有 bl_rollback, 无新代码。

## 恢复命令 (如需接力)
```sh
cd /Users/stefan/.openclaw/workspace/yuleASR
cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build -j4   # 0 error
ctest --test-dir build                                              # 48/48
```
