# 变更提案：量产阻断缺陷修复（MCAL Crypto/Mcu/Spi + ECUAL LinIf/CanIf）

> **变更 ID**: fix-mcal-production-defects
> **状态**: Archived (2026-09-26)
> **优先级**: P0
> **负责人**: Track-A（验证实质化）
> **创建日期**: 2026-09-25
> **目标版本**: v1.7.0
> **估计工时**: 40h (5d)（P0-1 MCAL 24h + P0-4 ECUAL 16h）

## 背景

### P0-1: MCAL 缺陷-测试共生（2026-09-25）

量产就绪度评估发现 MCAL 层存在 4 处已确认的源码缺陷，接线测试（`tests/bsw/mcal/`）通过 "Source quirk (asserted as-is)" 注释将缺陷行为固化为断言，形成**缺陷-测试共生**：源码错误被测试背书，缺陷对 CI 不可见。这是量产最大隐性风险——HIL/台架阶段才会暴露。

| # | 缺陷 | 位置 | 测试适配证据 |
|:--|:-----|:-----|:------------|
| D1 | `Crypto_GetVersionInfo` 空指针 DET 报 `CRYPTO_SID_DEINIT` (0x01)，非本 API SID | `src/bsw/mcal/crypto/src/Crypto.c:185` | `tests/bsw/mcal/crypto/test_crypto.c:672` "Quirk" 注释 |
| D2 | `Mcu_DistributePllClock` 用 `currentClock==0` 作"未锁相"哨兵，但时钟配置索引 0 合法 → `Mcu_InitClock(0)` 后分发永远被拒 | `src/bsw/mcal/mcu/src/Mcu.c:79,372` | `tests/bsw/mcal/mcu/test_mcu.c:78-81` 复制配置项用索引 1 绕行 |
| D3 | `Spi_SetBaudRateInternal` 分频搜索循环 `preDiv` 匹配后多自增一次 → PERIODREG 分频值 +1，实际波特率减半 | `src/bsw/mcal/spi/src/Spi.c:224-231` | `tests/bsw/mcal/spi/test_spi.c:275-278` "Source quirk" 注释，断言 0x94 |
| D4 | `Spi_GetVersionInfo` 空指针 DET 用字面量 `0x02`，非 `SPI_SERVICE_ID_GETVERSIONINFO` (9) | `src/bsw/mcal/spi/src/Spi.c:570` | `tests/bsw/mcal/spi/test_spi.c` 断言字面量 0x02 |

### P0-4: ECUAL 空心测试 + 生产缺陷（2026-09-25）

量产收敛核查发现车窗控制器 ECU 关键路径的 3 个 ECUAL 模块测试为**空心套件**（目录与 CMake 接线存在，但用例不编译真实源码、断言缺失或无 runner）。实化过程中暴露 4 处 ECUAL 生产缺陷：

| # | 缺陷 | 位置 | 影响 |
|:--|:-----|:-----|:-----|
| D5 | `LinIf_Init` 未置 `LINIF_INIT` 状态，模块初始化后永久不可用 | `src/bsw/ecual/linif/src/LinIf.c` | 整车 LIN 通信全断（SOP 阻断） |
| D6 | `LinIf_GetVersionInfo` 空指针参数无 DET 上报 | `src/bsw/ecual/linif/src/LinIf.c` | 调试性与 AUTOSAR 符合性缺口 |
| D7 | `LinIf_Config` 声明为 `static`，外部无法链接引用 | `src/bsw/ecual/linif/src/LinIf_Lcfg.c` | 配置无法被测试/集成使用 |
| D8 | `CanIf.h` 缺 `CanIf_TxConfirmation`/`CanIf_ControllerBusOff`/`CanIf_RxIndication` 原型且未包含 `Can.h`，头文件不自洽 | `src/bsw/ecual/canif/include/CanIf.h` | 驱动侧回调对上层不可见，TU 编译失败 |

空心测试实化：CanTp（14 用例，含 N_Bs 超时复位）、LinIf（13 用例，验证 D5 修复）、CanIf（17 用例，桩 Can_Write/Can_SetControllerMode）。

## 目标

1. 修复 D1-D8 源码缺陷，行为符合 AUTOSAR SWS 与模块内部契约
2. 同步修正被适配的测试断言，删除 "Source quirk" 注释，建立**正确的黄金断言**
3. 实化 CanTp/LinIf/CanIf 三个空心测试套件并接线 CMake（cantp_test / linif_test / canif_test）
4. 为 D2 建立索引 0 回归测试；为 D3 建立 ECSPI2 通道 PERIODREG 断言
5. 记录已知问题：CanIf_Lcfg.c 使用过期类型名且未定义头文件声明的 `CanIf_Config`（移交配置生成器任务）

## 范围

### 包含内容
- `Crypto.h` / `Crypto.c` — 新增 `CRYPTO_SID_GETVERSIONINFO` (0x02)，D1 修复
- `Mcu.c` — 引入 `MCU_CLOCK_INVALID` 哨兵，D2 修复
- `Spi.c` — D3 分频循环修复 + D4 SID 修复
- `LinIf.c` — D5 状态激活修复 + D6 DET 修复
- `LinIf_Lcfg.c` / `LinIf.h` — D7 `LinIf_Config` 外部链接导出
- `CanIf.h` — D8 回调原型补全 + `Can.h` 自洽包含
- `tests/bsw/mcal/{crypto,mcu,spi}/` — 断言修正与新增回归用例
- `tests/bsw/ecual/{cantp,linif,canif}/` — 空心测试实化 + 3 个 CMakeLists.txt 接线

### 不包含内容
- MISRA Rule 11.4 / 13.5 存量违规（另变更处理）
- `tests/unit/`、`tests/unit/autosar/` 废弃测试树清理（已标记 `@deprecated`，另变更处理）
- CanIf_Lcfg.c 配置过期问题（配置生成器任务）
- S32K312 真实硬件 HIL 验证（Track C）

## 验收标准

- [x] S1: `Crypto_GetVersionInfo(NULL)` 上报 SID = `CRYPTO_SID_GETVERSIONINFO` (0x02)，`mcal_crypto_real_test` 通过
- [x] S2: `Mcu_InitClock(0)` → `Mcu_DistributePllClock()` 成功置位 `CCR.CLK_ENABLE` 且无 DET；未配置时钟时仍上报 `MCU_E_PLL_NOT_LOCKED`；`mcal_mcu_test` 通过
- [x] S3: 80MHz/1Mbit/s → PERIODREG = 0x93（preDiv=3, postDiv=9）；80MHz/8Mbit/s → PERIODREG = 0x90；`mcal_spi_test` 通过
- [x] S4: `Spi_GetVersionInfo(NULL)` 上报 SID = `SPI_SERVICE_ID_GETVERSIONINFO` (9)
- [x] S5: `LinIf_Init` 后 `LinIf_Transmit` 返回 E_OK（D5 修复生效），`linif_test` 激活用例通过
- [x] S6: `LinIf_GetVersionInfo(NULL)` 上报 `LINIF_E_PARAM_POINTER`（SID 0x08）
- [x] S7: CanIf.h 自洽包含且三回调原型可见，`canif_test` 编译通过（17/17）
- [x] S8: cantp_test（14/14）、linif_test（13/13）、canif_test（17/17）全通过
- [x] 全量 `ctest` 回归零失败（104/104）

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| 修 D2 后依赖 currentClock==0 语义的下游代码受影响 | 低 | 中 | 全仓 grep `currentClock` 确认仅 Mcu.c 内部使用 |
| 修 D3 改变 PERIODREG 输出，其他测试断言旧值 | 低 | 中 | 全仓 grep PERIODREG 断言点，全部同步修正 |
| 哨兵值 0xFFFFFFFF 与有效 ClockSetting 冲突 | 低 | 高 | `Mcu_InitClock` 已校验 `ClockSetting >= NumClockConfigs`，合法索引远小于哨兵 |
