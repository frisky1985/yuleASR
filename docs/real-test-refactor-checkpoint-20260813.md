# 真实测试改造 Checkpoint（2026-08-13 16:02）

## 背景
老板 15:44 头脑风暴指出：yuleASR 测试体系存在"26 个测试文件未挂载 + 部分驱动无法 host 单测"问题。建议清单：
1. ✅ 仓库止血（已完成，`599ba12d` 已推：coverage_report 41 文件出 git + 43 个 build 缓存 trash 清理）
2. ✅ 状态文档对齐（8 critical 核实：**属于 yuleASR-Configurator 且已全部修复**（Batch B/C/D，`3e67b4c8` 收官），MEMORY.md 已修正挂错项目的信息）
3. 🔄 真实测试改造（本任务）

## 关键发现（已核实）
- **已挂载的 mock 测试是真测驱动**：`tests/mock/test_mcal_*.c` 链接真实驱动源码（Dio.c/Gpt.c/Adc.c）+ REG_READ32/REG_WRITE32 宏重定向到 mock_hal → 不是"桩测桩"，是真测。共 17 个已挂载（mcal_dio/gpt/can/icu/adc/port/wdg/spi/eep/fee/flash/i2c/lin/ocu/ramtst/uart_test）
- **真实缺口 A：5 个驱动无 REG 宏**（flash/eep/lin/ocu/ramtst 的 .c 里 REG_READ/REG_WRITE=0，直访寄存器地址）→ 无法 host 单测，测试实际是 stub 契约测试
- **真实缺口 B：26 个未挂载测试文件**在 `tests/unit/autosar/mcal/`：
  - 可编译且 mock 无对应的独有测试（值得挂载）：test_ETH(10 tests)、test_fls(29 tests)、test_linslave(6 tests)、test_Crypto(9 tests，缺 blake2.h include)
  - 可编译但与 mock 重复：test_ADC/test_CAN/test_dio/test_gpt/test_icu/test_LIN/test_ramtst（mock 已有，unit 版测试数更多但自包含 stub 实现）
  - stub 空壳（TEST_IGNORE）：test_i2c/test_port/test_spi/test_uart/test_wdg
  - 编译失败/废弃：test_flash（MEMIF_BLOCK_INCONSISTENT 未定义）、test_mcu（缺 mock_mcal.h）、Crypto_Test（语法错误）、test_linslave

## 待执行（小克）
1. **挂载 mock 无对应的独有测试**：test_ETH/test_fls/test_linslave/test_Crypto 挂载到 `tests/mock/CMakeLists_MCAL_Tests.txt`（add_mcal_test 宏），补缺失依赖（test_Crypto 需 blake2.h include 路径）
2. **补 Fls_ReadSync 实现**（声明无实现，`Fls.h:332` 声明但 Fls.c 无实现）——同步读 API，参考 Fls_Read 异步版 + Fls_ProcessRead 实现
3. **处置重复/废弃测试**：与 mock 重复的 unit 版测试（自包含 stub）建议删除或归档；stub 空壳（TEST_IGNORE）删除；编译失败废弃的删除
4. **验证**：全量构建 0 error + ctest 全绿（基线 48 + 新增挂载测试）

## 纪律
- 只改 yuleASR 工作区，**不 commit/push**（主代理统一收尾）
- 测试必须真实执行（RED→GREEN），禁止 mock 假装
- 完成后写 checkpoint 到 `docs/real-test-refactor-checkpoint-20260813.md`
