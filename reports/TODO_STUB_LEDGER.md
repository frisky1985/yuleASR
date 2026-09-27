# TODO/Stub 台账（BSW 生产化差距）

> 生成日期: 2026-09-26
> 来源变更: `openspec/changes/fix-bsw-production-gaps`（P0-5 质量门禁输出）
> 扫描范围: `src/`（排除 `third_party/`、`build*/`、`config/`）
> 扫描口径: 行级命中计数
> - 显式标记: `TODO|FIXME|XXX|HACK`
> - 占位标记: `stub|placeholder|占位|待实现|not yet implemented`（大小写不敏感）
>
> 用途: 作为 P1/P2 排期输入；本台账不修改任何代码，仅登记归属与建议。

## 1. 总览

| 区域 | TODO/FIXME | Stub/占位 | 合计 |
|---|---:|---:|---:|
| `src/bsw`（本平台主交付） | 2 | 121 | **123** |
| `src/crypto_stack` | 11 | 1 | 12 |
| `src/middleware` | 0 | 5 | 5 |
| `src/autosar` | 2 | 3 | 5 |
| `src/diagnostics` | 1 | 4 | 5 |
| `src/bootloader` | 0 | 4 | 4 |
| `src/platform` | 1 | 2 | 3 |
| `src/cross` | 0 | 1 | 1 |
| **合计** | **17** | **143** | **160** |

BSW 侧 123 处与变更设计文档（AD7）口径一致；其中 P0-1..P0-4 涉及的模块（BswM/LinIf/CanIf/CanSM/CanTp/MCAL Can）遗留标记已在本变更内清零。

## 2. 显式 TODO/FIXME 明细（17 条）

| # | 位置 | 内容 | 归属 | 建议 |
|---|------|------|------|------|
| 1 | `src/platform/s32k312/include/Reg_Macros.h:8` | 替换为 SDK 寄存器宏（量产） | Platform S32K312 | **P1** |
| 2 | `src/bsw/mcal/uart/include/Dma.h:5` | 替换为真实 eDMA 驱动（量产） | MCAL Uart | **P1**（若 UART+DMA 为量产外设路径） |
| 3 | `src/bsw/services/mqtt/src/Mqtt.c:600` | 系统时间源改用 `Os_GetTimeMs` | Services Mqtt | P2 |
| 4 | `src/crypto_stack/csm/csm_core.c:326,777,782` | 作业 submit/start/complete 时间戳置 0 | CryptoStack Csm | P2 |
| 5 | `src/crypto_stack/keym/keym_core.c:565,644,669,962` | 槽位 created/activated/expired 时间置 0 | CryptoStack KeyM | P2 |
| 6 | `src/crypto_stack/keym/keym_core.c:1055,1065` | DDS 证书导入/导出未实现，已显式 fail-closed | CryptoStack KeyM | P2（行为安全，仅缺功能） |
| 7 | `src/crypto_stack/keym/keym_core.c:64` | 历史说明注释（原假实现已被真实计算替代） | CryptoStack KeyM | 信息项，无行动 |
| 8 | `src/crypto_stack/tests/test_keym.c:583` | 测试注释：DDS 证书路径显式 NOT_IMPLEMENTED | CryptoStack 测试 | 信息项 |
| 9 | `src/autosar/e2e/e2e_dds_integration.c:378,466` | 收发时间戳置 0 | Adaptive E2E | P2 |
| 10 | `src/diagnostics/isotp/isotp_core.c:45` | 平台时间函数待替换 | Diagnostics IsoTp | P2 |

> 去重后行动项 ≈ 8 个主题（2 个 P1：S32K312 寄存器宏、MCAL UART DMA）。

## 3. Stub/占位热点（模块归属）

### 3.1 `src/bsw`（121 处）

| 模块 | 处数 | 代表位置 | 性质 | 建议 |
|------|-----:|----------|------|------|
| `services/swc` | 19 | `Swc_Lcfg.c`(12), `Swc.c`(7) | Placeholder 函数体（初始化/读/处理/发送） | **P1**：需声明支撑范围或降级为显式 unsupported |
| `services/doip` | 11 | `DoIP.c` | 骨架实现（文件头显式声明 stub/skeletal） | P2（若量产走 DoIP 诊断则升 P1） |
| `boot`（src+test） | 21 | `Boot_Loader/Verify/Update/Hsm.c` | 引导加载占位 | **P1**（量产升级路径） |
| `services/nvm` | 5 | `NvM_test.c` | 测试文件内占位（未被编译） | P2 |
| `services/j1939tp` | 4 | `J1939Tp.c/_Lcfg.c` | 占位 | P2 |
| `services/dlt` | 4 | `Dlt.c`, `Dlt.h` | 占位 | P2 |
| `services/{sd,mqtt,memif,ethtsyn,dem}` | 各 3 | — | 混合（生产+测试） | P2 |
| `mcal/crypto` | 7 | `Crypto_Hsm.c`, `Crypto_S32K312_Hsm.c` | HSM 桩 | **P1**（HSM 供 SecOC/CSM 生产使用） |
| `cdd` | 5 | `Cdd_Hsm_1.0.0.c` | HSM 桩 | **P1**（同上） |
| `ecual/frtp` | 5 | `FrTp_Rx/Tx/TxSm` | FrIf/PduR 调用点 Placeholder | P2（窗口控制器未用 FlexRay 则建议整体裁剪） |
| `ecual/ethtrcv` | 4 | `EthTrcv.c` | 显式 unsupported PHY 访问骨架 | P2 |
| `ecual/canif` | 1 | `CanIf.c:275` | `CanIf_CancelTransmit` 恒 E_OK 占位 | **P1**（CAN 生产 API 语义） |
| `os` | 2 | `Os_TaskEntries.c:106,118` | 周期任务体为 placeholder | P2（需确认为设计意图） |
| 其余 services 单点 | 11 | `det/com/cryif/comM/flstst/ldcom/tcpip/tm/e2e...` | 注释/占位混合 | P2 |

### 3.2 BSW 之外（22 处）

| 区域 | 处数 | 备注 |
|------|-----:|------|
| `crypto_stack` | 12 | 主要为时间戳 TODO（见 §2） |
| `middleware` | 5 | microdds/Rte 传输与 NvM 接口占位 |
| `autosar`(adaptive) | 5 | E2E 时间戳 + exec 占位 |
| `diagnostics` | 5 | IsoTp 时间函数 + 占位 |
| `bootloader` | 4 | 测试/平台占位 |
| `platform` | 3 | 寄存器宏 TODO + 占位 |
| `cross` | 1 | hello.c |

## 4. 挂单摘要

**P1（量产路径相关，建议下个迭代排期）**
1. S32K312 SDK 寄存器宏替换（`Reg_Macros.h`）
2. MCAL UART eDMA 真实驱动（`Dma.h`）
3. HSM 桩实化：`mcal/crypto`(7) + `cdd/Cdd_Hsm`(5)
4. `CanIf_CancelTransmit` 占位语义
5. Boot 引导加载占位（21 处）
6. `services/swc` 19 处 Placeholder 需定性（实化或裁剪）

**P2（清理/低风险）**
- 全部时间戳类 TODO（csm/keym/e2e/isotp/mqtt）
- `doip`、`frtp`、`ethtrcv` 等未用量产模块的骨架/占位（建议按窗口控制器实际总线裁剪）
- 测试文件内占位标记

## 5. 本变更已消除（P0-1..P0-5）

- P0-1..P0-4 六个模块（BswM/LinIf/CanIf PN 面/CanSM/CanTp/MCAL Can FD）遗留 TODO/stub 已实现或删除，无残留。
- `legacy/` 7 目录（crypto/csm/dcm/dem/doip/nvm/xcp，75 文件）已核实无生产引用后删除。
- `tests/unit` 死目录已清理：`com/`(13) `common_stubs/`(3) `mqtt/`(7) `ecuc/`(1) `__pycache__/`(1) `shall-to-test-mapping.json`；连带清理断链引用（`batch10_coverage.sh` COM 段、`tests/dcm/`、`tests/dem/`、`tests/unit/diagnostics/`）。
- `tests/unit` 仍存活的测试树（10 个 ctest 用例 + 内置 Unity 框架）**未删除**：设计文档"整树废弃"的前提经核实不成立（详见 §6）。

## 6. 遗留观察（P2，超出本变更范围）

- `tests/unit` 下仍存在大量"构建未引用、但被 tools/docs/scripts 引用"的历史测试树（`autosar/`、`mcal/`、`middleware/`、`diagnostics`、`dcm`、`dem`、`fee`、`flash`、`pdur`、`soad` 等）。任何整树删除必须连同 `tools/generate_traceability_real.py`、`scripts/analysis/batch10_coverage.sh`、`docs/requirements.md` 等引用一并更新，另行立项。
- `docs/misra_deviations.md`、`.yuleosh/ci-config.yaml`、`docs/compliance/*` 中 `legacy/**` 排除/偏差扫描范围在目录删除后成为无害空操作，随下次文档例行同步清理。
- `tools/analysis/fix_const.sh` 中 12 条 sed 命令指向已删除的 legacy dcm/dem 文件，建议下次工具清理时移除。
