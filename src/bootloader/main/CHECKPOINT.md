# Checkpoint — 抗回滚「已建未接线」修复 Step1 + Step2

日期: 2026-08-13 | 执行: 小克 (subagent b583ccbb) | 分支: master (HEAD a68468b8, 未 commit — 主 agent 收口)

## 缺陷修复目标
bl_antrollback 0 生产调用者 / bl_secure_boot 计数器是静态值非 NVM 装载 /
Boot_Update 自建 BIB pending 与 bl_antrollback 重复 — 三层未接通。

## Step 1: SBL 集成层骨架 (P0)
- 新增 `src/bootloader/main/sbl_main.h` + `sbl_main.c` — 生产集成层入口:
  `sbl_main_init` (① Init bl_antrollback 从 NVM 装载 ② 读计数器填
  `bl_secure_boot_config_t.anti_rollback_counter`(0=禁用) ③ bl_secure_boot_init
  ④ bl_rollback_init) → `sbl_main_boot` (① 重装载计数器 ② 填 config
  ③ `sbl_main_connect_antrollback` 注入 Boot_Update ④ bl_secure_boot_verify
  ⑤ 通过 → NotifySuccessfulBoot 延后递增提交 → jump_to_app 回调;
  失败 → bl_rollback 记录/回滚 → enter_recovery 拒绝)。
  生产替换点: flash 驱动 / CSM-KeyM / 镜像来源 / jump/recovery 回调 / 回滚分区管理器。
- CMake: `src/bootloader/CMakeLists.txt` 挂载 `main/sbl_main.c` (+main include+install)。
- 测试 `src/bootloader/tests/test_sbl_main.c` (6 用例, mock flash + mock Boot_Flash):
  验签通过+延后递增提交+跳转 / 哈希失败拒绝 / NVM 计数器回滚保护 (ROLLBACK_PROTECTION,
  证明计数器来自 NVM) / 连续失败触发 bl_rollback_execute / 非法参数。

## Step 2: 抗回滚存储接口抽象 (方案 C)
- 新增共享接口 `include/autosar/bl_rollback_storage.h` (纯接口, 宿主+交叉均可见;
  放共享层因 bsw/boot 交叉构建时 bootloader 层被排除, 不能放 src/bootloader/):
  `read_counter / write_counter / increment / set_confirm_boots / stage /
  notify_successful_boot / get_pending` + 错误码 (与 bl_antrollback 1:1, 编译期断言)。
- `bl_antrollback` 实现接口: `Boot_AntiRollback_GetStorageApi()` 返回静态函数表
  (bl_antrollback.c 末尾, 显式错误码映射 + 静态断言)。
- `Boot_Update` (bsw/boot) 增加 `Boot_Update_SetAntiRollbackStorage(api, ctx)`:
  Finalize / NotifyBootSuccess / GetRollbackCounter 注入模式走接口回调;
  未注入时保持旧 BIB 行为 (兼容既有测试)。
  收敛: 注入模式下 BIB 不再写 anti_rollback_counter/pending 字段, 只做版本管理
  (sbl_version/app_version), 计数器唯一事实源 = bl_antrollback NVM。
- 修复发现的设计缺陷: SetAntiRollbackStorage 不再推送阈值 (曾覆盖存储实现已配置的
  confirm_boots 为默认 3); 阈值同步显式走 Boot_Update_SetRollbackConfirmBoots。

## 接线图
```
                    ┌────────────────────────── SBL 集成层 (bootloader/main/sbl_main) ──┐
 NVM flash (注入) ─►│ bl_antrollback (NVM 单调计数器+磨损均衡+CRC+延后递增)             │
                    │      │ ①Init/Read(装载)     │ ⑤NotifySuccessfulBoot(延后提交)      │
                    │      ▼                     │                                       │
                    │ bl_secure_boot_config.anti_rollback_counter (0=禁用)               │
                    │      │ ④verify (验签 App)                                          │
                    │      ▼                                                            │
                    │ bl_secure_boot ──通过──► jump_to_app 回调 (Boot_Loader_Jump)       │
                    │      └──失败──► bl_rollback (记录→execute) / enter_recovery 拒绝   │
                    └──────────┬─────────────────────────────────────────────────────────┘
                               │ ③注入 bl_rollback_storage_api_t (GetStorageApi)
                               ▼
              Boot_Update (bsw/boot, 独立静态库, 无 bootloader 依赖)
                Finalize/NotifyBootSuccess/GetRollbackCounter ──► 经接口回调访问 NVM
                BIB 仅版本管理 (不再存计数器) ─── 收敛双机制
```

## 测试结果
- `test_sbl_main`: 6/6 PASS (ctest #17)
- `test_e2e_antrollback`: 6/6 PASS — A 回滚拒绝 / B 正向 N 次提交 / C 延后+重启持久 /
  D1 后端契约 / **D2 Boot_Update 注入接口→NVM (WIRED)** / **E sbl_main_boot 生产链路**
  (D2/E 为该任务预置的可执行规格, 现全部变绿) (ctest #18)
- 既有回归: test_bootloader / test_boot_* 全绿 (旧 BIB 模式未破坏)
- 全量: **ctest 48/48 PASS** (原 46 + sbl_main + e2e_antrollback)
- 编译: 0 error; 新文件 0 新增 warning (仅存量 bl_partition/bl_rollback 警告)

## 遗留 / 说明
- 未 commit/push (主 agent 收口); 未触碰 yuleASR-Configurator。
- sbl_main.o 引用 Boot_Update 注入符号 — 消费 sbl_main 的可执行需链 boot 库
  (test_sbl_main 已示范); 不消费 sbl_main 的既有目标 (test_bootloader) 不受影响。
- 生产接线点: 真实 flash 驱动 / CSM-KeyM 实例 / App 镜像来源 / Boot_Loader_Jump /
  EnterRecovery / 回滚分区管理器注入 (骨架中均为 NULL/mock)。
- 既有 BIB 计数器设备切换注入模式时以 NVM 为准 (迁移策略由集成层决定, 已注释)。
