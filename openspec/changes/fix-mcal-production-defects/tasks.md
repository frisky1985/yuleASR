# 任务清单

## 已完成任务

### Ground Truth 核实 (2026-09-25)
- [x] 确认 D1 Crypto.c:185 使用 CRYPTO_SID_DEINIT，Crypto.h 缺 CRYPTO_SID_GETVERSIONINFO 定义
- [x] 确认 D2 Mcu.c:79/372 以 0 为哨兵，test_mcu.c:78-81 用复制配置项（索引 1）绕行
- [x] 确认 D3 Spi.c:224-231 循环 post-increment 导致 preDiv+1，test_spi.c:279 断言错误黄金值 0x94
- [x] 确认 D4 Spi.c:570 字面量 0x02，Spi_Cfg.h:26 已有 SPI_SERVICE_ID_GETVERSIONINFO (9)
- [x] ModuleId 全表查重（awk 值唯一性校验）— 无重复，P0-2 无需代码变更
- [x] 确认 tests/unit、tests/unit/autosar 为 @deprecated 废弃树，不在本次范围

### 源码修复 (2026-09-25)
- [x] D1: Crypto.h 新增 CRYPTO_SID_GETVERSIONINFO (0x02)；Crypto.c:185 替换使用
- [x] D2: Mcu.c 新增 MCU_CLOCK_INVALID 哨兵；init/校验点替换；清理重复 @req 注释
- [x] D2 补充: Mcu_Init 内重置 currentClock=MCU_CLOCK_INVALID（静态初始化器每进程仅执行一次，防止跨会话/跨测试泄漏）
- [x] D3: Spi_SetBaudRateInternal 捕获变量重构 + 未匹配回退 15/15
- [x] D4: Spi.c:570 0x02U → SPI_SERVICE_ID_GETVERSIONINFO

### 测试修正（黄金断言回归）(2026-09-25)
- [x] test_crypto.c: GetVersionInfo_NullPtr 断言 CRYPTO_SID_GETVERSIONINFO，删 Quirk 注释
- [x] test_mcu.c: 删除 testClocks[1] hack（NumClockConfigs 2→1）
- [x] test_mcu.c: AfterInitClock 用例改 InitClock(0U)（索引 0 回归）；NoClockSelected 移至 InitClock 用例前声明（runner 按声明序执行，静态状态跨用例保留）
- [x] test_mcu.c: NoClockSelected 注释改写（哨兵语义）
- [x] test_spi.c: PERIODREG 0x94→0x93 + 黄金值推导注释；ECSPI2 (8Mbit/s) = 0x90 断言
- [x] test_spi.c: SyncTransmit_DeviceOnChannel1 PERIODREG 0x91→0x90（同一 off-by-one 的第二处黄金值）
- [x] test_spi.c: GetVersionInfo_NullPtr 断言 SPI_SERVICE_ID_GETVERSIONINFO，删 Quirk 注释
- [x] tests/mock/CMakeLists_MCAL_Tests.txt: 接线 mcal_mcu_test 目标（此前 test_mcu.c 存在但未注册）

### 回归验证 (2026-09-25)
- [x] `mcal_crypto_real_test` / `mcal_mcu_test` / `mcal_spi_test` 三个目标通过
- [x] 全量 `ctest --test-dir build-native` 零失败（101/101；7 个 Not Run 为历史未构建目标，补建后全通过）

### ECUAL 测试实化 + 生产修复 (P0-4, 2026-09-25)
- [x] CanTp: test_cantp.c 重写（14 用例，桩 CanIf_Transmit/PduR 回调 + Det 计数），链接生产 CanTp_Lcfg.c；含 N_Bs 超时复位、双 Init 配置替换用例；接线 CMake 目标 cantp_test
- [x] LinIf: 生产缺陷修复 — LinIf_Init 未置 LINIF_INIT 导致模块初始化后永久不可用（SOP 级缺陷），修复后状态正确激活
- [x] LinIf: 生产修复 — GetVersionInfo 空指针参数缺少 DET 上报（补 LINIF_E_PARAM_POINTER）
- [x] LinIf: 生产修复 — LinIf_Config 原为 static 无法被测试/集成引用，改外部链接并在 LinIf.h 增加 extern 声明
- [x] LinIf: test_linif.c 重写（13 用例，验证激活修复 + 版本黄金值 0x0055/0x27）；接线 CMake 目标 lintrcv 同风格 linif_test
- [x] CanIf: 生产修复 — CanIf.h 缺 CanIf_TxConfirmation/CanIf_ControllerBusOff/CanIf_RxIndication 原型（驱动侧回调不可见），补原型 + #include "Can.h" 使头文件自洽
- [x] CanIf: test_canif.c 重写（17 用例，桩 Can_Write/Can_SetControllerMode/PduR 回调；生产 Lcfg.c 类型过期故用测试内有效静态配置）；接线 CMake 目标 canif_test
- [x] 已知问题记录: CanIf_Lcfg.c 使用过期类型名（CanIf_HohCfgType 等）且未定义头文件声明的 CanIf_Config，列为后续配置生成器任务
- [x] 全量回归 `ctest --test-dir build-native` 104/104 通过（原 101 + cantp_test/linif_test/canif_test）

## 待完成任务

（无）

### 归档记录 (2026-09-26)
- [x] 变更归档评审 — 用户批准直接归档，跳过 /triple-verify 门禁；archive.json 已建立

## 进度统计

- 总任务数: 26
- 已完成: 26
- 完成率: 100%
