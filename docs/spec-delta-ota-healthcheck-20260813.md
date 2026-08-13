# Spec-Delta: OTA 健康确认钩子 + 掉电保护 DOWNLOADING 落盘

> 日期: 2026-08-13 | 来源: 文章《OTA 升级工程实践》(A/B 分区/回滚/掉电保护) 对比分析
> 报告: `reports/ota-doc-vs-yuleasr-20260813.md` | 基线: yuleASR master `bb77e566`
> 流程: 老板 2026-08-13 头脑风暴确认 → P1 显式确认 API（方案 B）、P2 BIB update_state（方案 A）

---

## 背景（为什么做）

对照文章《OTA 升级工程实践》核查 yuleASR OTA 模块，发现 2 个差距：

1. **P1 无主动健康检查钩子**：现在"能启动就算健康"（验签过 → 立即 NotifySuccessfulBoot → 启动 N 次自动提交）。文章强调健康确认必须覆盖**关键业务路径**（DCM 响应/传感器有效/无功能异常），否则新固件能启动但业务异常时，3 次启动后照样被确认，无法自动回滚。
2. **P2 掉电保护第一节点缺失**：`Boot_Update_Prepare` 确认后**直接擦除**目标 Slot，未先落盘"下载中"状态。擦除后、Finalize 前掉电 → 重启无任何"升级未完成"记录，可能误切半擦除槽。

## P1: 主动健康检查钩子（RS-OTA-05）

### SHALL
- `bl_antrollback_slot_t` 增加 `health_confirmed` 字段（记录版本 v2→v3），独立 CRC 保护范围随之扩展
- 提交条件改为：`pending_boot_count >= confirm_boots` **且** `health_confirmed == true`（健康门控开启时）
- 新增 API：`Boot_AntiRollback_SetHealthCheckMode(ctx, bool enabled)`——使能/关闭健康门控
- 新增 API：`Boot_AntiRollback_ConfirmHealth(ctx, version, bool ok)`——App 业务健康确认
  - `ok == true` 且 version == pending_counter → 置 `health_confirmed = true` 并落盘
  - `ok == false` → 置 `health_confirmed = false` 落盘（保持 pending，等待回滚）
- `sbl_main_boot` 验签通过后**不再立即 notify**，改为：健康门控关闭时走原 N 次制；门控开启时等待 App 确认

### SHOULD
- `Boot_Update` 层暴露薄封装 `Boot_Update_ConfirmBusinessHealth(boolean ok)`，转发到 antrollback
- 未调用 ConfirmHealth 且门控开启 → pending 保持 → watchdog 复位 → boot_attempt_count 超限 → 自动回滚（复用现有兜底）

### MAY
- 门控开启时提供查询 `Boot_AntiRollback_IsHealthConfirmed(ctx)`

### GIVEN/WHEN/THEN
- GIVEN 门控关闭（默认）WHEN 新版本启动 N 次 THEN 行为与现有一致（**向后兼容**，现有测试零破坏）
- GIVEN 门控开启 WHEN App 调 ConfirmHealth(true) THEN pending_boot_count 正常累计，达 N 次提交
- GIVEN 门控开启 WHEN App 从不确认（业务挂死）THEN 计数不提交，watchdog 超限后回滚

## P2: 掉电保护 DOWNLOADING 落盘（RS-OTA-06）

### SHALL
- `Boot_InfoBlock` 增加 `update_state` 字段（复用 reserved 字节，结构尺寸与 crc32 偏移不变，旧 BIB 兼容读）
- 状态枚举：`BOOT_UPDATE_IDLE = 0` / `BOOT_UPDATE_DOWNLOADING = 1` / `BOOT_UPDATE_PENDING = 2`
- `Boot_Update_Prepare`：confirm_gate 通过后、**擦除之前** → 写 BIB(`update_state=DOWNLOADING`) 并落盘
- `Boot_Update_Finalize`：hash 校验通过后 → BIB(`update_state=PENDING`)；全部成功后 → BIB(`update_state=IDLE`)
- Bootloader 启动决策（sbl_main_boot / Boot_Loader）：读到 `update_state == DOWNLOADING` → 目标槽不可信 → **忽略 pending，启动活动槽**（旧固件照常运行）

### SHOULD
- `bib_pending_valid` 逻辑同步识别 DOWNLOADING 状态（旧 BIB 读为 0 = IDLE，兼容）

### GIVEN/WHEN/THEN
- GIVEN Prepare 已落盘 DOWNLOADING WHEN 擦除/写入中途掉电重启 THEN 启动决策识别未完成升级，回退活动槽
- GIVEN Finalize 已落盘 PENDING WHEN 校验通过重启 THEN 正常进入候选槽验证流程
- GIVEN 升级成功落盘 IDLE WHEN 后续正常启动 THEN 状态机无残留

## 验收标准
- [ ] ctest 全绿（基线 48/48，含 bootloader 3 测试 + 新增用例）
- [ ] 新增单测覆盖：健康门控开/关、ConfirmHealth true/false、DOWNLOADING 中断回退、BIB v2→v3 兼容读
- [ ] 全量 native 构建 0 error
- [ ] 与 Configurator 联动：`linkage-regression.sh` 仍全绿（Boot_Cfg.h 不新增宏，无 schema 影响）
