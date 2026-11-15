# MISRA C:2012 修复台账 (2026-11-02)

> 依据报告 `docs/reviews/misra-c-review-2026-11-02.md` 逐条整改。
> 真实违规修复与登记偏差分开记录。验证基线：报告去重统计 (21.6=227, 21.10=3, 17.2=12, ...)。
> 构建验证命令：`./build.sh`（本机 native, 输出 build-native/）；测试构建目录 build-test/。
> 定向扫描命令模板：见报告 §2，将目标目录替换为受影响模块。

## A. 真实违规修复记录

| # | 条目 | 规则 | 文件 | 修复方式 | 验证 | 规则计数变化 | 状态 |
|---|------|------|------|---------|------|-------------|------|
| P0-1a | 模拟 HAL 桩出生产树 | 21.6 (10) / 21.10 (1) | src/bsw/mcal/lin/src/LinSlave_Hal.c → tests/mock/LinSlave_Hal.c | 文件迁移；更新 tests/mock/CMakeLists_MCAL_Tests.txt:634 引用为 ${MOCK_DIR}/LinSlave_Hal.c | ① ./build.sh native 通过 ② build-test 重配+构建 mcal_linslave_real_test 通过，运行 41/41 PASS ③ cppcheck 定向扫 src/bsw/mcal/lin：21.6/21.10 = 0 | 21.6: 227→217 (lin 模块清零)；21.10: 3→2 | ✅ |
| P0-1b | 无效 time.h 引用 | 21.10 (1) | src/bsw/services/mqtt/include/mbedtls/x509_crt.h:13 | 删除未使用的 `#include <time.h>`（全模块无 time_t/clock_gettime 使用，实测确认） | ① build-native 增量重建通过 ② cppcheck 定向扫 src/bsw/services/mqtt：21.x = 0 | 21.10: 2→1 | ✅ |
| P0-2 | 测试文件混入生产树 | 21.6 (217) / 17.7 (~111) | SoAd_Test.c→tests/unit/soad/、SomeIpTp_Test.c→tests/unit/someiptp/、SomeIpXf_Test.c→tests/unit/someipxf/、StbM_Test.c→tests/unit/stbm/ | 文件迁移（生产 services CMake 本就过滤 *_Test.c，无构建引用需更新；4 文件为孤儿遗留测试）；.yuleosh/ci-config.yaml exclude_paths 增加 `src/**/*_Test.c`/`src/**/*_Test.h` | ① build-native 重配+重建通过 ② yaml 解析通过 (exclude_paths 14 条) ③ cppcheck 定向扫 4 模块目录：21.6=0、17.7=0 | 21.6: 217→0（规则清零）；17.7: 214→约103（测试部分移除） | ✅ |
| P0-3a | 表达式树递归 | 17.2 (8) | src/bsw/services/bswm/src/BswM.c BswM_EvaluateExpression + BswM_CollectRulePorts | 递归改迭代：显式帧栈（ENTER/MID/EXIT 状态机），深度上界 BSWM_EXPR_MAX_DEPTH=16，栈容量 17 帧有数学上界证明；深度/索引越界子树贡献 FALSE/0，与原递归守卫语义一致 | ① build-native 重建通过 ② build-test 构建 bswm_svc_test/bswm_unit_test，ctest 3/3 PASS ③ cppcheck 定向扫 bswm：17.2=0 | 17.2: 12→4 | ✅ |
| P0-3b | 冗余块读/写重试尾递归 | 17.2 (2) | src/bsw/services/nvm/src/NvM.c:763 NvM_ReadRedundantBlock、:889 NvM_WriteRedundantBlock | 尾递归改 while(retry) 循环（最多 2 副本），状态流转语义不变 | ① build-native 通过 ② NvM_UnitTest + NvM_Redundant_UnitTest 构建并 PASS ③ 定向扫 nvm：17.2=0 | 17.2: 4→2 | ✅ |
| P0-3c | 证书管理相互递归 | 17.2 (2) | src/bsw/services/mqtt/src/Mqtt_CertMgr.c AddCert↔UpdateCert | 提取内部函数 CertMgr_StoreNewEntry（存新条目），UpdateCert 改调内部函数打破循环；公开 API 与行为不变 | ① build-native 通过（service_mqtt）② 定向扫 mqtt：17.2=0（mqtt 无现成单测目标，构建+扫描验证） | 17.2: 2→0（规则清零） | ✅ |
| P1-1 | 丢 const 限定（方案 A：拆字段） | 11.8 (2) | src/bsw/services/nvm/src/NvM.c、src/middleware/rte/src/Rte_NvMInterface.c | 队列条目 `void* DataPtr` 拆为 `void* DstPtr`（READ/RESTORE 目标）+ `const void* SrcPtr`（WRITE 源）；入队按作业方向赋对应字段、另一字段置 NULL_PTR；消费点按方向取用（NvM: 6 处 Dst 消费 + 2 处 Src 消费 + 7 处入队；Rte: 3 处消费 + 3 处入队 + Init）。删除 `(void*)NvM_SrcPtr` 与 `(void*)dataPtr` 两处去 const 强转。对外 AUTOSAR API 签名不变 | ① `cmake --build build-native` 全量通过 ② build-test 构建 NvM_UnitTest/NvM_Redundant_UnitTest/Rte_test_rte_cs_operations_UnitTest/nvm_test/test_e2e_nvm_stack 后 ctest：5/5 PASS ③ cppcheck 定向扫 nvm+rte：11.8=0，11.6=0，11.9 仅剩既有 NvM_EccHandler.c:533（未触碰） | 11.8: 2→0（规则清零） | ✅ |
| P1-2 | switch 缺 default | 16.4 (1) | src/bsw/services/cansm/src/CanSm.c:635 CanSm_ProcessFullComState | 补 `default:` 分支：DET 开启时以 CANSM_E_UNEXPECTED_EXECUTION 报告（沿用本文件 CANSM_DEV_ERROR_DETECT 门控），并把 SubState 复位到 CANSM_S_FULLCOM_NOP（与同文件 NOCOM/SILCOM/BOR 状态机的 default 复位模式一致），避免枚举扩展或状态损坏时静默无处理 | ① build-native service_cansm 构建通过 ② build-test cansm_test 构建 + ctest 1/1 PASS ③ 定向扫 cansm：16.4=0；其余规则仅存既有项（17.8 位于 L803，非本次改动） | 16.4: 1→0（规则清零） | ✅ |
| P1-3 | 数组部分初始化 | 9.3 (实测 4) | src/bsw/ecual/ipdum/src/IpduM_Lcfg.c:40-43 | 4 个 `uint8[8]` 动态数据数组由 `{0x00}` 改为 8 个显式 `0x00` 初始化器（与同文件 IpduM_TxMuxPdu0_StaticData 风格一致），语义不变 | ① build-native ecual_ipdum/service_ipdum 构建通过 ② 定向扫 ipdum+linSM：9.3=0（LinSM_Lcfg.c:162 未被复现，见 C 节）③ 其余规则仅存既有项 | 9.3: 5→0（规则清零） | ✅ |
| P1-4 | 无符号常量缺 U 后缀 | 7.2 (34) | src/bsw/services/crc/src/Crc_Lcfg.c:122-177、src/bsw/mcal/spi/src/Spi.c:571、src/bsw/mcal/uart/src/Uart.c:1086 | Crc_32_Table 中 128 个高位（>INT_MAX）字面量加 `U` 后缀；Spi/Uart 的 `0xFFFFFFFF` → `0xFFFFFFFFU`。用一次性脚本按行精修，复核：剥掉 U 后与 HEAD 内容逐字节一致（纯后缀改动，无数值变化） | ① build-native service_crc/mcal_spi/mcal_uart 构建通过 ② build-test 构建 mcal_spi_test/mcal_uart_test/crc_test 后 ctest 3/3 PASS ③ 定向扫三模块：7.2=0，无新增规则 | 7.2: 34→0（规则清零） | ✅ |

## A2. P2 配置/文档/脚本修正记录

| # | 条目 | 涉及文件 | 修正方式 | 验证 | 状态 |
|---|------|---------|---------|------|------|
| P2-1 | 偏差报告结论与 17.7 分类 | docs/compliance/misra-deviation-report.md | 版本升 v1.5：撤销 v1.4 "Required 全部清零"结论，新增 §2.3 真实基线（评审 25,136 → P1 后复扫 24,454）；DP-AUTOSAR-002 由 Advisory 更正为 **Required**、数量 367→75、范围扩到 det*/trace*/mem*；§3.1/§3.2 改为引用真实基线与 Required 偏差清单；§2.1/§2.2 合计行标注"偏差登记项，不含待修违规"；附录 C 增 v1.5 记录 | 文档内数值与 `cppcheck-after-p1.xml` 去重统计一致 | ✅ |
| P2-2 | CI 偏差登记（8.6 / 11.6 / DP-002） | .yuleosh/ci-config.yaml | 8.6 理由重写为弱符号默认钩子模式（LINIF_WEAK / OS_WEAK / EcuM_AL_* / 通知回调），说明 cppcheck TU 级分析把弱默认+强覆盖当重复定义，数量 24（P0/P1 前 36）；11.6 理由修正为 MCAL MMIO + TcpIp.c:574 lwIP 回调 uintptr 转换两类（原"硬件寄存器"与实际不符）；DP-AUTOSAR-002 scope 由 `src/**/det*` 扩为 `src/**`，理由补 mem* 与 17.7 Required | `python3 -c yaml.safe_load` 解析通过，deviations 92 条 | ✅ |
| P2-3 | 规则文本与 addon 配置失效路径 | .yuleosh/misra_texts.txt、.yuleosh/misra_c2023_texts.txt、.yuleosh/misra-addon-config.json | 重写规则文本：修正 2.2/2.3/2.4、7.2、9.3、15.4、18.4、20.2、8.5 的错误文本，分类更正 5.6/5.7/5.8/8.8/9.3=Required、17.3=Mandatory、8.9/8.11/11.4/11.5/13.3=Advisory（对齐 rule-stats.csv 官方口径）；C:2023 文本由 2012 复制并改头；addon json 中 `/Users/stefan/.openclaw/...` 失效绝对路径改为 `.yuleosh/misra_c2023_texts.txt` | `misra.py --rule-texts=.yuleosh/misra_texts.txt --verify-rule-texts /dev/null` 解析成功 | ✅ |
| P2-4 | 扫描脚本 addon 模式 + 精确误报抑制 | tools/run_misra_check.sh、tools/misra/cppcheck_suppressions.xml | 脚本新增 `--addon`/`-h`/位置参数；addon 模式固定 `--std=c99`、加载 misra addon 与 addon json；把 XML 抑制表用 python 展平为 cppcheck 2.21 可接受的 `rule:file` 文本（`--suppressions-list`）；修复 TIMESTAMP 提前导致的顺序 bug。XML 抑制新增 header 范围（`*/include/*.h` 的 2.3/2.4/2.5/5.6/5.7）与 file 范围（Csm_Cfg.c 21.10 = DEV-01、NvM_EccHandler.c 11.9、TcpIp.c 20.7） | `bash tools/run_misra_check.sh --addon src/bsw/mcal/lin` 冒烟通过（Error 0、生成 XML/摘要/抑制文本）；抑制表生成条目数与 XML 一致 | ✅ |

## B. 登记偏差 (Deviation)

| # | 规则 | 文件:行 | 偏差理由 | 批准/生效方式 | 状态 |
|---|------|---------|---------|--------------|------|
| DEV-01 | 21.10 (1) | src/bsw/services/csm/src/Csm_Cfg.c:55 | `#include <time.h>` 位于 `#if defined(__unix__) \|\| defined(__APPLE__)` 主机仿真分支，用于 Csm_Cfg_GetTimestamp() 的 POSIX clock_gettime(CLOCK_MONONIC)；目标平台（交叉编译）该分支不编译，裸机路径返回 0。与 Dlt_GetTimestampUs 既有模式一致，仅用于作业排队时间戳、非安全决策 | 已写入 tools/misra/cppcheck_suppressions.xml（file+rule 精确抑制，`misra-c2012-21.10:src/bsw/services/csm/src/Csm_Cfg.c`），在 `--addon` 模式生效 | ✅ P2-4 已落地 |

## C. 差异记录（报告 vs 实测）

| 项 | 报告口径 | 实测 | 说明 |
|----|---------|------|------|
| 21.6 文件构成 | "LinSlave_Hal.c 等 5 个文件" | LinSlave_Hal.c(10) + SoAd_Test.c(40) + SomeIpTp_Test.c(42) + SomeIpXf_Test.c(87) + StbM_Test.c(48) = 227 | 4/5 是 *_Test.c 单元测试，非 HAL 桩；统一在 P0-2 迁移 |
| 21.10 文件构成 | 3 条/3 文件 | LinSlave_Hal.c(1) + Csm_Cfg.c(1) + mqtt mbedtls/x509_crt.h(1) | Csm_Cfg.c 为主机仿真分支，登记偏差 DEV-01 |
| 生产树残留小写测试文件 | 报告仅列 `*_Test.c` 大写 4 例 | 另发现 7 个 `*_test.c` 小写孤儿测试仍留在生产树：com/src/Com_test.c、dcm/src/Dcm_test.c、dem/src/Dem_test.c、docan/src/DoCan_test.c、doip/src/DoIp_test.c、nvm/src/NvM_test.c、pdur/src/PduR_test.c | 生产 CMake 已用 `[_Tt]est\.c$` 过滤，不会被编译；但 CI `exclude_paths` 的 `src/**/*_Test.c` 模式不匹配小写，扫描仍会计入（如 NvM_test.c 贡献 17.7/21.6）。待决策：迁移到 tests/unit/ 或把排除模式放宽为 `src/**/*_[Tt]est.c` |
| 9.3 数量 | 5 条 / 2 文件（含 LinSM_Lcfg.c:162） | 实测 4 条，全部在 IpduM_Lcfg.c:40-43 | 复扫 ipdum+linSM 未再出现 LinSM_Lcfg.c:162（`LINSM_SCHEDULE_COUNT_PER_CHANNEL`=4 且恰有 4 个初始izer，9.3 不成立）；报告该条疑为全量扫描的 .h 上下文误报 |
| 7.2 数量 | 34 条 / 3 文件 | 34 条：Crc_Lcfg.c 32 + Spi.c 1 + Uart.c 1 | 与报告一致（Crc 每条为 4 个字面量，按列计数） |

## E. 本轮（2026-11-05）迁移与收尾修复

### E-1 7 个孤儿小写测试迁移（生产树 → tests/unit）

| 原路径 | 新路径 | 用例数 | 结果 | ctest 登记 |
|--------|--------|-------|------|-----------|
| src/bsw/services/com/src/Com_test.c | tests/unit/com/legacy_com_cases.c | 16 | 16/16 PASS | ✅ com_legacy_test |
| src/bsw/services/dcm/src/Dcm_test.c | tests/unit/dcm/legacy_dcm_cases.c | 7 | 4 PASS / 3 FAIL | ❌ 见 E-4 |
| src/bsw/services/dem/src/Dem_test.c | tests/unit/dem/legacy_dem_cases.c | 30 | 30/30 PASS | ✅ dem_legacy_test |
| src/bsw/services/docan/src/DoCan_test.c | tests/unit/docan/legacy_docan_cases.c | 9 | 8 PASS / 1 FAIL | ❌ 见 E-4 |
| src/bsw/services/doip/src/DoIp_test.c | tests/unit/doip/legacy_doip_cases.c | 9 | 未构建（API 已废弃） | ❌ 见 E-4 |
| src/bsw/services/nvm/src/NvM_test.c | tests/unit/nvm/legacy_nvm_cases.c | 16 | 12 PASS / 4 FAIL | ❌ 见 E-4 |
| src/bsw/services/pdur/src/PduR_test.c | tests/unit/pdur/legacy_pdur_cases.c | 10 | 10/10 PASS | ✅ pdur_legacy_test |

- 合计 97 个用例：通过 80、失败 8、未构建 9（doip）。文件均 `git mv` 保留全部用例内容，非桩/辅助代码。
- 迁移配套修改：
  - `RUN_TEST(x)` → `RUN_TEST(test_x)`（tests/unit/test_framework.h 内 unity.h 先定义宏，必须传完整函数名）
  - `target_include_directories(... BEFORE PRIVATE tests/unit ...)`，避免 `include/autosar/test_framework.h` 空桩遮蔽
  - dem：`sint8 i` → `sint16 i`（原 `for (i=0; i<200; i++)` 溢出后死循环，测试挂起）；`ASSERT_EQ(NULL_PTR, ...)` → `TEST_ASSERT_NULL`
  - nvm：新增 `#include "MemIf.h"` 与 `MemIf_GetJobResult` mock
  - dcm：额外链接 `service_dem`（DEM 集成路径）
  - com：`Com_ReceiveSignal(99U, ...)` → `Com_ReceiveSignal(COM_NUM_OF_SIGNALS, ...)`（生产 `COM_NUM_OF_SIGNALS=256`，99 已不再是非法 ID）
- 测试数量对比：ctest 注册数 **192 → 195**（新增 com_legacy_test / dem_legacy_test / pdur_legacy_test）；另 4 个套件已构建未注册（原因见 E-4）。
- CI/扫描排除模式：`.yuleosh/ci-config.yaml` 已同时覆盖 `src/**/*_test.c`（L653）与 `src/**/*_Test.c`（L655），无需改动。
- 全量扫描文件清单同步更新：`files-after-p1.txt`(551) → `files-final.txt`(544)，剔除上述 7 个已迁移文件。

### E-2 Rule 17.7 返回值未使用（75 处）

| 文件 | 处数 |
|------|------|
| src/bsw/ecual/canNm/src/CanNm.c | 28 |
| src/bsw/ecual/linNm/src/LinNm.c | 28 |
| src/bsw/ecual/linTp/src/LinTp.c | 26 |
| src/bsw/services/mqtt/src/Mqtt.c | 26 |
| src/bsw/ecual/ethif/src/EthIf.c | 6 |
| src/bsw/services/nvm/src/NvM_Redundant.c | 4 |
| src/bsw/ecual/someipif/src/SomeIpIf.c | 3 |
| src/bsw/ecual/srp/src/Srp.c | 9 |
| src/bsw/services/mqtt/src/Mqtt_CertMgr.c | 2 |
| src/bsw/services/mqtt/src/Mqtt_Tls.c | 1 |
| src/bsw/services/xcp/src/Xcp.c | 1 |

- 逐文件语义核对结论：全部为 `memcpy`(36) / `memset`(21) / `strncpy`(18)，返回值为指针或拷贝长度，**不携带状态语义**；无"写失败需上报"类调用需要单独处理。
- 修复方式：调用前加 `(void)`；每个文件首个修改处前统一注释
  `/* memcpy/memset/strncpy return values carry no status; cast to void (MISRA 17.7). */`
- 验证：build-native 构建 `ecual_canNm ecual_ethif ecual_linNm ecual_linTp ecual_someipif ecual_srp ecual_xcp service_mqtt service_nvm service_xcp` 通过；build-test 构建并运行 `NvM_Redundant_UnitTest NvM_UnitTest Xcp_Services_UnitTest CanNm_UnitTest mqtt_test mqtt_tls_test mqtt_certmgr_test nvm_test ethif_test test_e2e_nvm_stack` → 10/10 PASS。

### E-3 剩余真实违规 3.1 / 21.1 / 2.2

| 条目 | 规则 | 文件:行 | 修复方式 | 验证 |
|------|------|---------|---------|------|
| E-3a | 3.1 | src/bsw/mcal/eth/include/Eth_Private.h:94 | 注释块内嵌套 `/*====` 开头的分隔线，改为单层 `/*====...====*/` | build-native mcal_eth 通过；mcal_eth_real_test PASS |
| E-3b | 3.1 | src/bsw/mcal/fee/src/Fee_Lcfg.c:212 | 同上，去掉注释块内嵌套的 `/*` 分隔线 | build-native mcal_fee 通过；Fee_test_fee_init/read/write PASS |
| E-3c | 21.1 | src/bsw/mcal/lin/include/Std_Types.h | 删除本地 `#define NULL ((void*)0U)`，改为 `#include <stddef.h>`（LIN 代码均用 `NULL_PTR`，`grep` 确认无裸 `NULL` 使用） | build-native mcal_lin/linNm/linTp/linif 通过；lintrcv_test/linif_test PASS |
| E-3d | 2.2 | src/bsw/ecual/doIP/src/DoIP_Lcfg.c:127,128,267,273 | 去掉表达式中的 `+ 0`（redundantAssignment），保留 `+1/+2/+3` 偏移 | build-native ecual_doIP 通过；DoIP_UnitTest PASS |

### E-4 迁移用例未登记 ctest 的原因（未修改生产行为）

| 套件 | 失败/未构建 | 根因 | 处置 |
|------|-----------|------|------|
| dcm_legacy_test | 3 FAIL（期望正响应 SID 0x50/0x62/0x7E，实得请求 SID 0x10/0x22/0x3E） | `Dcm_SendPositiveResponse` 写 `SID + DCM_E_POSITIVERESPONSE`(=0)，与注释"+0x40"不符；`tests/unit/dcm/test_dcm_uds_matrix.c:22` 已记为已知偏差 | 不改生产行为，先构建不注册；待决策 |
| docan_legacy_test | 1 FAIL（RxIndication 未路由到 Dcm） | 用例期望的 PduMapping 在生产 `DoCan_Lcfg.c` 中不存在 | 同上 |
| nvm_legacy_test | 4 FAIL（DET 错误码、SetRamBlockStatus、CancelJobs 行为差异） | `NvM.c` 本地重复定义 `NVM_E_PARAM_POINTER 0x01U`，与公共头 `NvM.h` 的 `0x0E` 冲突，`Det_ReportError` 的 err 恒等于 api id | 同上（属对外行为变化） |
| doip_legacy_cases.c | 未构建 | 调用已移除的 `DoIp_*` API，生产现为 `src/bsw/ecual/doIP` 的 `DoIP_*`；等价覆盖见 `tests/unit/netcom/test_doip.c` | 保留文件 + 文件头迁移说明，不编译 |

## 待办进度

- [x] P0-1 模拟 HAL 桩迁移 + 21.10 清理
- [x] P0-2 4 个 *_Test.c 移出生产树 + CI 排除模式
- [x] P0-3 17.2 递归改迭代（BswM/NvM/Mqtt 三处）
- [x] P1-1 11.8 拆字段（NvM + Rte_NvMInterface）
- [x] P1-2 16.4 CanSm 补 default
- [x] P1-3 9.3 数组全量初始化（IpduM）
- [x] P1-4 7.2 补 U 后缀（Crc/Spi/Uart）
- [x] P2-1 偏差报告结论与 17.7 分类修正
- [x] P2-2 CI 偏差登记（8.6 弱符号 / 11.6 理由 / DP-002 范围）
- [x] P2-3 规则文本与 addon 配置失效路径
- [x] P2-4 扫描脚本 --addon 模式 + 精确误报抑制（DEV-01 已落地）
- [x] E-1 7 个孤儿小写测试迁移 + ctest 登记（192→195，195/195 PASS）
- [x] E-2 17.7 的 75 处 mem*/strncpy 加 `(void)`
- [x] E-3 3.1 / 21.1 / 2.2 逐条修复
- [x] F 最终全量复扫基线 + 口径更正（17.7 真实 1,553 需决策）
- [x] G 扫描配置固化（tools/run_misra_check.sh：仓库根目录 / 227 个 .c / 144 个 include / 串行可复现 / 抑制表由 XML 展平）
- [x] G 编译器交叉验证 + 17.3（87 条）全量定性（真实项 11 条，均在未启用的 #ifdef 分支）
- [x] H 17.7（1,611 条）统一 `(void)` 批处理 + 宏体内一次解决 + 真实 API 按语义处理 → **17.7=0**
- [x] I 四个测试套件定性（dcm 4/3、nvm 15/1、docan 8/1、doip 归档）
- [x] J Advisory 大桶抽样（2.5 / 20.1 / 8.4 / 8.5）+ 15.5 登记偏差
- [x] K 分层门禁写入 CI（Mandatory=0、Required 增量零新增、Advisory 看趋势）+ 开放 `-Wmissing-prototypes`

## D. 全量复扫基线（P1 完成后）

- 文件清单：`build/misra-review-2026-11-02/files-after-p1.txt`（551 个 .c/.h，已排除迁移出的 5 个文件）
- 原始结果：`build/misra-review-2026-11-02/cppcheck-after-p1.xml`（19MB）
- 去重后（rule, file, line）合计 **24,454** 条（评审基线 25,136）
- 目标规则：21.6=0、21.10=0、17.2=0、11.8=0、16.4=0、9.3=0、7.2=0
- 剩余待修真实违规：17.7=75（检测/追踪 + mem* 返回值）、3.1=2、21.1=1、2.2=4
- 已登记偏差：8.6=24、11.6=2、11.9=1、17.8=14
- 扫描须在仓库根目录运行（addon json 内 rule-texts 为相对路径）

## F. 最终复扫基线（2026-11-05，本轮修复后）

- 文件清单：`build/misra-review-2026-11-02/files-final.txt`（544 个 .c/.h，已剔除 7 个迁移测试）
- 结果文件：`build/misra-review-2026-11-02/cppcheck-final-reportcmd.xml`（按报告 §2 命令，仓库根目录执行）
- 去重后（rule, file, line）合计 **17,336** 条；Required 3,170 / Advisory 13,502 / Mandatory 664；唯一规则 53
- 本轮目标规则：**3.1=0、21.1=0、2.2=0**（已清零）；21.6/21.10/17.2/11.8/16.4/7.2 维持 0
- 17.7：**1,553 条 / 81 文件**（详见 F-1）

### F-1 重要更正：旧基线 17.7=75 属于"包含路径失效"下的低估

证据（同一文件 `src/bsw/ecual/ethswt/src/EthSwt.c`，同一 addon/抑制参数）：

| 运行方式 | 17.7 | 17.3 |
|---------|------|------|
| 仓库根目录（`-Isrc/...` 可解析） | 152 | 0 |
| `build/misra-review-2026-11-02/` 目录（`-I../../src/...` 部分失效） | 0 | 152 |
| 去掉 `-Isrc/bsw/services/det/include`（Det.h 变隐式声明） | 0 | 152 |

- 机制：项目头找不到时，`Det_ReportError` 等被当作隐式声明，cppcheck 不再产生"返回值未使用"诊断 → 17.7 被低估、17.3（函数隐式声明）虚增。Nov-2 的 `cppcheck-misra-c2012.xml` / `cppcheck-after-p1.xml` 正是这一形态（17.3 ≈ 3,494、17.7 仅 75），说明当时扫描未能在仓库根目录解析相对 include。
- 因此 F 节的 17,336 才是可信基线；旧基线 24,454 不可与本次直接比较（其中 Required/Mandatory 含大量隐式声明噪声）。
- 17.7 真实构成（本轮复扫）：DET 上报类 1,450（`Det_ReportError` 854 + 各模块 DET 宏 596）、校验宏 60+、其余为少数真实 API（`I2c_SendStop` 9、`DoIP_SendGenericNack` 4、`SoAd_IfTransmit` 3、`EcuM_EnableWakeupSources` 3、`Rte_Start` 2、`Mqtt_Tls_Close` 2、`NvM_RedundantWriteBlock` 2 等）。
- 本轮已处理的 75 处均为 `memcpy`/`memset`/`strncpy`，语义无语义返回值，处理方式正确；上述 1,450 条 DET 上报调用返回 `Std_ReturnType`，按 AUTOSAR 惯例通常忽略，属于需决策项，未擅自批量 `(void)`。

## G. 17.3（Mandatory）抽样定性（2026-11-05）

### G-1 编译器交叉验证（判定"是否真有隐式声明"）

- 独立构建：`cmake -S . -B build-warn -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_C_FLAGS="-Wimplicit-function-declaration -Wmissing-prototypes"`，`BUILD_EXIT=0`
- `-Wimplicit-function-declaration` 告警 **0** 条；`-Wmissing-prototypes` 告警 **0** 条（项目 flags 末尾自带 `-Wno-missing-prototypes`，该开关实际被关闭，需在 CI 中显式前置打开才有意义）
- 全量 `warning:` 合计 43 条（与 17.3 无关，属既有告警）
- 结论：**当前构建配置下不存在真实的隐式函数声明**；cppcheck 的 87 条 17.3 全部来自 addon 的语法/预处理建模，而非编译语义

### G-2 87 条全量分类（逐条核对源码，61 条直接读取上下文）

| 类别 | 数量 | 典型证据（file:line → 事实） | 定性 |
|------|------|------------------------------|------|
| A. 原型声明行被当作调用 | 19 | `LinNm.h:402/410/416`、`LinIf.h:285/291`、`EcuM.h:396`、`LinSlave_CfgTable.h:227/228` 均为函数原型行；`Crypto_MbedTLS.c:44` 是 `extern int mbedtls_ctr_drbg_random(...)`，`:860-869` 是 10 行 `Crypto_MbedTLS_*` 原型 | 工具误报（cppcheck 把带参数的声明解析为调用） |
| B. 宏未被 cppcheck 展开 | 30 | `Wdg_Hw.c:555/557/563/566/572/577/581/914/915` 的 `WDG_HW_WDOG_WCR()/WSR()` 寄存器宏；`Os.c:296/402/522...` 的 `taskYIELD()/portENABLE_INTERRUPTS()/portDISABLE_INTERRUPTS()`；`Xcp.c:924/1777/1780/1791` 的 `XCP_*_RESOURCE_PROTECTION()`；`Platform_Lockstep.c` 9 处 `PLATFORM_LOCKSTEP_DSB/ISB/NOP()` | 工具误报（宏在 FreeRTOS/平台头中定义，cppcheck 配置分支未展开） |
| C. 内联汇编/编译器内建 | 7 | `Mcal.c:28/31/46/49` `__asm volatile(...)`、`:74` `__builtin_trap()`；`LinTrcv.c:151` `__asm("nop")`；`NvM_EccHandler.c:411` `__asm volatile ("dsb sy")` | 工具误报（`__asm` 被当作函数名） |
| D. 已声明 API 但 cppcheck 配置未解析 | 20 | `Csm.c` 17 处 `Csm_Cfg_HwService`/`Csm_Cfg_RandomGenerate`，原型在 `Csm_Cfg.h:132/148` 且 `Csm.c:29` 已 include；`SomeIpSd.c:260` 原型在 `SomeIp.h:251`（经 `SomeIpSd.h:25` 引入）；`WdgM.c:965/975` 原型在 `WdgM_Cfg.h:72/80` | 工具误报（include 链与原型均存在） |
| E. 真实缺原型（条件编译未启用分支） | 11 | `NvM_EccHandler.c:556` `NvM_SetBlockStatus`（`#ifdef NVM_SET_BLOCK_STATUS`，全仓库无声明）；`NvM_EccHandler.c:694/703/712/721` 与 `Platform_EccHandler.c:443/507/675/684/693` 的 `Dem_ReportErrorStatus`（`#ifdef DEM_E_*`，仓库 Dem 公开 API 为 `Dem_ReportError` 等，无此函数）；`Platform_EccHandler.c:651` `Platform_Fccu_NonFaultyFault`（定义在 `Platform_RamSafety.c:39`，无头文件原型，`#ifdef PLATFORM_FCCU_FAULT_RAM_ECC`） | **真实项**（当前宏未启用故不参与编译，故编译器交叉验证为 0） |

- 合计 19+30+7+20+11 = **87**，与基线计数一致。
- 处置原则：A~D 属工具误报，不修改源码、不加抑制（Mandatory 不允许靠抑制清零）；E 类 11 条需在启用对应特性宏前补齐原型/改用仓库现有 Dem API，属于**特性启用前置条件**，列入待办，不在本轮改代码（避免触碰未启用的安全/诊断特性分支）。
- 与旧基线对比：旧口径 17.3=664，其中约 577 条是包含路径失效导致的隐式声明虚增（见 F-1）；新口径 87 条中真实项 11 条，说明**"17.3 大部分是工具误报"的初判成立**，无需大规模改代码。

## H. 17.7（Required，1,611 条 / 83 文件）处理结果（2026-11-07）

### H-1 前置确认：返回类型与宏展开形态

| 检查项 | 事实 | 结论 |
|--------|------|------|
| `Det_ReportError` 原型 | `Det.h:212` — `Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)` | **非 void**，`(void)` 强制转换是标准且正确的修复（非 addon 建模问题） |
| `Det_ReportRuntimeError` | 同为 `Std_ReturnType` | 同上 |
| 各模块 DET 封装宏（如 `CANIF_DET_REPORT_ERROR`/`IPDUM_DET_REPORT_ERROR`/`DIO_DET_REPORT_ERROR` 等 36 个） | 宏体为**单条** `Det_ReportError(...)` 调用（无 `do{}while(0)` 多语句块） | 在宏体内加一次 `(void)` 即可覆盖该宏的全部调用点，无需逐调用点处理 |
| 校验类块宏（`UART_VALIDATE_CHANNEL`/`MEMIF_VALIDATE_*`/`SPI_VALIDATE_*`） | 宏体为 `do{...}while(0)` | **不能**加 `(void)` 前置（`(void)do{}while(0)` 非法），已从批量脚本排除 |

- 未新建 `src/common/include/Misra_Compliance.h`：该目录**不在** `CMakeLists.txt` 的全局 `include_directories()` 中（生产代码只使用其中的 `ModuleId.h` 且按相对路径/局部 include 引入），全局新增头文件会改变头文件解析顺序；且 17.7 的规范修复本来就是 `(void)`，包一层 `MISRA_IGNORE(expr)` 宏只会增加间接层。**故采用等价方案：统一 `(void)` + 场景注释**，并在本节记录该决策。

### H-2 批量变更（可复核的边界控制）

- 变更前统计：`baseline/verify-violations-pre-h.csv`（17,352 条全量）中 17.7 逐文件计数 = **1,611 条 / 83 文件**（`EthSwt.c` 76、`CanIf.c` 71、`TcpIp.c` 68、`EcuM.c` 59、`Dem.c` 56 …）。
- 脚本化改动合计 **1,611 个报告点**：`Det_ReportError`/`Det_ReportRuntimeError` 直调 909 处 + 其它非 DET 调用 702 处；另有 **36 个模块 DET 宏体**一次性修复（覆盖其全部调用点）。
- 边界复核：
  - 只匹配"表达式语句"形态（行首为调用、行尾为 `;`），不匹配声明/赋值右值/`return` 语句；
  - `#define` 行、函数原型行、注释行、`if (...)` 条件内的调用**不在替换范围**；
  - 首轮误加到 `do{}while(0)` 块宏的 `(void)` 已用正则批量回退，`cmake --build build-native -j8` 复验 **EXIT=0 / 0 error**。
- 逐文件核对：复扫后 83 个文件 17.7 计数**全部为 0**。

### H-3 真实 API 逐条按语义处理（不"顺手 (void)"）

| API | 条数 | 核对结果与处置 |
|-----|------|----------------|
| `I2c_SendStop` | 9 | **语义修复**：TX/RX 传输完成路径捕获 STOP 返回值，失败时 `chInfo->Result = I2C_RESULT_TIMEOUT`（不再把总线非空闲的传输报成成功）；错误清理路径为"保留原始错误码 + best-effort STOP"，带 `MISRA 17.7 deviation` 注释 |
| `NvM_RedundantWriteBlock` | 2 | **偏差登记（不吞错）**：恢复写失败时块保持 `INCONSISTENT`，下次读会重试；读路径失败由后续 CRC 校验拦截。注释写明可观测失败路径 |
| `DoIP_SendGenericNack` | 4 | **偏差登记 + 待评审**：DoIP SWS 未定义 TX 失败 DET 错误码，新增错误码需改公共头（超出本轮范围）；失败由 SoAd 自身 DET 与 tester 超时两条路径可见。未新增错误码 |
| `SoAd_IfTransmit` | 3 | 同上（SoAd DET + 上层超时可见） |
| `EcuM_EnableWakeupSources` / `DisableWakeupSources` | 3 + 1 | **偏差登记**：启动/睡眠/停机路径 best-effort，唤醒源配置失败由后续唤醒事件缺失暴露 |
| `Mqtt_Tls_Close` | 2 | **偏差登记**：拆除路径，错误已无可恢复动作 |
| `Wdg_Hw` 早期预警触发 | 1 | **偏差登记**：失败指示由 MCU 复位承担 |
| `Csm_Cfg` (mbedtls_sha256) | 1 | **偏差登记**：定长输入下不可失败（库契约） |
| `Rte_Start` | 2 | **核对后保留原样**：签名 `Rte_StatusType Rte_Start(void)`；两处均在 `#if (ECUM_RTE_ENABLED == STD_ON)` 内，当前配置未启用、不参与编译（故也不产生诊断）。**启用该特性前**须改为检查返回值并走 DET |

### H-4 复扫计数变化（同一命令、`JOBS=1` 串行、`--verify`）

| 规则 | 处理前 | 处理后 | Δ |
|------|-------:|-------:|---:|
| **17.7** | **1,611** | **0** | **−1,611** |
| 21.1 | 1 | 0 | −1 |
| 21.10 | 1 | 0 | −1 |
| 11.9 | 5 | 4 | −1 |
| 20.7 | 12 | 10 | −2 |
| 20.1 | 676 | 677 | +1（MemMap 段切换随代码位移，见 J） |
| **合计** | **17,352** | **15,737** | **−1,615** |

- 17.3 维持 87（工具误报定性不变，见 G）；21.6/21.10/17.2/11.8/16.4/7.2 维持 0。
- 分类口径（MISRA C:2012 Appendix A 官方类别，与评审报告 §2 一致）：Mandatory **87** / Required **1,664** / Advisory **13,534** / cppcheck 核心检查（非 MISRA）**452**。

### H-5 受影响模块单测

- `mcal_i2c_test`、`mcal_wdg_test`、`mcal_wdg_offmode_test`、`test_csm`、`test_doip_chain`、`test_e2e_nvm_stack`：**全部 PASS**（`build-test/bin/`）。
- 生产构建：`build-native`（含全部改动）EXIT=0；独立 `build-proto` 亦 EXIT=0。

## I. 四个测试套件定性（2026-11-07）

判定依据：AUTOSAR SWS + 仓库公共头 + ISO 14229-1；**凡判定为"实现错、需改对外行为"的一律只汇报不改**。

| 套件 | 结果 | 定性 | 规范依据 / 证据 | 处置 |
|------|------|------|-----------------|------|
| DCM | 4 PASS / **3 FAIL** | **实现错**（未改） | 三个失败均为肯定响应 SID：测试期望 `0x50/0x62/0x7E`（= 请求 SID\|0x40，符合 ISO 14229-1），实现返回 `0x10/0x22/0x3E`。根因 `Dcm.c:191`：`SID + (uint32_t)DCM_E_POSITIVERESPONSE`，而 `Dcm.h:164` 中 `DCM_E_POSITIVERESPONSE == 0x00`；`dcm_transfer.c` 正确使用 `DCM_TRANSFER_RESPONSE_SID_OFFSET (0x40U)`。**测试正确、实现错误** | 待批准（改则改变对外响应字节） |
| NvM | **15 PASS / 1 FAIL**（原 4 FAIL） | 3 修测试、1 为实现错（未改） | ①`NvM.c` 本地重复定义 `NVM_E_PARAM_POINTER/BLOCK_ID/NOT_INITIALIZED/BLOCK_PENDING/BLOCK_CONFIG` = 0x01~0x05，遮蔽公共头 0x0E/0x0A/0x14/0x15/0x16 → **删除本地定义**（以公共头为准）；②`NvM_CancelJobs` 取消成功按 SWS 返回 `E_OK`，测试旧注释期望 `E_NOT_OK` → 修期望；③SWS 无 `NvM_DeInit`，未初始化用例须先运行（静态状态持久）→ 调整顺序；④`test_nvm_setramblockstatus`：`NvM_WriteBlock` 入队只置 `jobEntry.Result = NVM_REQ_PENDING`/`JobPending`，**未更新 `LastResult`**，`NvM_GetErrorStatus` 返回陈旧 `NVM_REQ_OK` → 实现缺口，待批准 | ①~③已改（仅测试/重复定义），④待批准 |
| DoCan | 8 PASS / **1 FAIL** | **配置缺陷**（未改） | `test_docan_rxindication_routes_to_dcm` 期望 Dcm 回调 1 次，实际 0。临时插桩（已完全回退，`grep DBG` = 0）显示 `id=0 mappings=4 enabled=1`，`DoCan_FindPduMappingByCanTpId` 先命中 `cand 0 = 0`：`DoCan_Cfg.h` 中 `DOCAN_CANTP_TX_DIAG_PHYSICAL (0U)` 与 `DOCAN_CANTP_RX_DIAG_PHYSICAL (0U)` **取值相同**，TX 映射（`RxIndicationEnabled=FALSE`）遮挡 RX 映射。**测试正确、配置错误** | 待批准（补夹具=改配置数据，优先方案） |
| DoIP | 旧套件不参与构建 | 过期套件 | 目标 API `DoIp_*` 已从生产树移除；等量覆盖在 `tests/unit/netcom/test_doip.c` | `tests/unit/doip/legacy_doip_cases.c` → `.archived`（**不复活旧 API**） |

- DoIP 归档命令：`mv tests/unit/doip/legacy_doip_cases.c tests/unit/doip/legacy_doip_cases.c.archived`；该目录无 CMakeLists，归档后不再进入任何构建。
- 复跑证据：`./build-test/tests/{dcm,nvm,docan}_legacy_test`（构建 EXIT=0）。

## J. Advisory 大桶抽样（2026-11-07）

抽样方法：`random.seed=7` 各取 28 条（2.5/20.1 另做全量自动化分类）；证据文件 `baseline/violations.csv`（15,737 条）。

| 规则 | 条数 | 抽样结论 | 证据 | 处置 |
|------|-----:|---------|------|------|
| **2.5** 未使用宏 | 5,902 | **41% 跨 TU 建模误报 + 56% 真实死宏** | 全量自动化：按定义行提取宏名并全仓计数 → 2,403 条（41%）在**其它 TU/文件被使用**（典型 `CANNM_CBV_RESERVED_MASK`、`SWC_DIAG_SESSION_TIMEOUT_PROGRAMMING`）；3,285 条（56%）全仓库零引用（集中在 vendor `S32K312.h` 351、`include/autosar/MemMap.h` 228、`Rte_Cfg.h` 124、`ModuleId.h` 99）；214 条定位落在宏续行/注释中（无法解析宏名） | **登记偏差 + 趋势门禁**，不清零、不删 vendor 宏；死宏清理单列技术债（低优先） |
| **20.1** 预处理指令 | 677 | **97% 为 AUTOSAR MemMap 模式** | 全量：654 条为 `#include "MemMap.h"` / `"<Mod>_MemMap.h"`（AUTOSAR 内存分段必需，规则 20.1 要求 include 前只有预处理指令/注释，MemMap 必须出现在段边界）；22 条非 MemMap：其中约 10 条为 include 晚于代码（`FrTp_Rx.c:47`/`FrTp_Tx.c:47`/`FrTp_TxSm.c:49` 的 `<string.h>`、`Crypto_MbedTLS.c:45-46`、`Spi.c:35`、`Fls.h:69-71`、`Flash_Lcfg.c:241`）→ 可平移至文件头；2 条为 addon 定位漂移（`I2c.c:1603` 指向 `if` 语句） | MemMap 部分**保留偏差**（DP-AUTOSAR-010/CI deviations）；10 条 include 顺序问题列入待办（不改行为） |
| **8.4** 定义前无可见声明 | 240 | **真实存量** | 28 条抽样全部为无可见原型的全局定义：`EthTrcv_GetInterfaceType`、`FrTp_RxStateMachine`、`Crypto_MainFunction`、`Crypto_Hsm_SelfTest`、`Dio_MaskedWritePort`、ISR（`Icu_TPM4_IRQHandler`/`Ocu_Channel2_IrqHandler`）、`OsTask_Init_Entry`/`Os_TimingProtectionMainFunction`、`_Lcfg.c` 配置对象（`LinNm_Config`/`Dlt_*`）。**独立编译器交叉验证**：开启 `-Wmissing-prototypes` 后 149 条 / 63 文件（此前被全局 `-Wno-` 掩盖） | Required：**登记基线 + 增量零新增**；补原型属独立整改批次（涉及公共头） |
| **8.5** 外部声明唯一 | 86 | **真实存量** | 28 条抽样为重复 `extern` 声明：`Ea.c:29-33` 本地声明 `Eep_*`（且 `Eep_GetStatus` 返回类型与 `Eep.h:208` 不一致：`MemIf_StatusType` vs `Eep_StatusType`）；`PduR.c:29-35` 本地声明 `CanIf_Transmit`/`Com_TxConfirmation`/`Dcm_TxConfirmation`；`Spi.c:30-31` 与 `Dma.h:35` 重复声明 `Dma_*`；`DoIP_Lcfg.c:30` 与 `DoIP_Cfg.h:199` 重复声明 `DoIP_Config`；`IpduM_Lcfg.c:279` 重复 `IpduM_ConfigPtr`；`Flash.h` 声明 `Fls_*` | Required：登记基线 + 增量零新增；**类型不一致项（Eep_GetStatus）列为优先修复**（潜在接口缺陷，未改） |
| **15.5** 单一出口 | 3,767 | 已知存量 | AUTOSAR 错误处理多返回点（DET 后立即 return） | **登记偏差，不做重构**（DP-AUTOSAR-001 / CI deviations 已覆盖） |

- 额外发现（cppcheck 核心检查，非 MISRA 规则，未计入 15,737 的规则统计口径）：4 条 `error` — `LinTrcv.c:750` 未初始化 `actualMode`、`CanTrcv.c:462` 非 void 函数缺 return、`I2c.c:309` `bestDivider` 未初始化（**存量**，处理前 CSV 已存在）、`config/generated/SecOC_Cfg.h:8` 非 ASCII 字符（字符码 208）。列为独立待办。
- 9 条 `warning` 中含 `SomeIpSd.c:218` 声明/定义参数顺序不一致、`Fls_Hw.c:452`/`dcm_transfer.c:587,786` 恒真内层条件、`Dio.c:141,292,293` 移位位数越界 — 均为存量，本轮未改。

## K. 分层门禁与 `-Wmissing-prototypes`（2026-11-07）

### K-1 开放编译器交叉检查

- `CMakeLists.txt`：从全局 `add_compile_options` 删除 `-Wno-missing-prototypes`（原第 110 行），保留 `BUILD_TESTING` 块内测试目标的豁免（Unity 约定）。
- 实测（`cmake -S . -B build-proto -DBUILD_TESTING=OFF && cmake --build build-proto -j8`）：**EXIT=0**；`-Wmissing-prototypes` **149 条 / 63 文件**（此前该开关被置后关闭，导致 G-1 的"0 告警"是构造出来的）。分布：`Os_TimingProtection.c` 26、`Crypto_MbedTLS.c` 24、`Rte_Scheduler.c` 22、`Com.c` 20、`blake2_autosar.c` 18。
- 交叉验证结论：8.4 的 240 条与编译器 149 条**互相印证为真实存量**（不再视为 addon 误报）。

### K-2 CI 分层门禁

写入 `.yuleosh/ci-config.yaml` 的 `misra.gates`（数值为本次 `--verify` 复扫基线，见 H-4）：

| 层级 | 基线 | 门禁策略 |
|------|-----:|---------|
| Mandatory | 87 | **零新增**（存量 87 全部为 17.3 工具误报/未启用 `#ifdef` 分支，见 G；不允许用抑制清零） |
| Required | 1,665 | **增量零新增**（存量按规则登记基线，见 J 表；新增一条即阻断） |
| Advisory | 13,539 | **只看趋势**（不阻断；连续 3 次复扫上升则告警） |
| cppcheck 核心检查 | 446 | 不阻断（**error = 0**，见 L） |

- `fail_threshold` 37000 → **17000**（真实 15,737 + 约 8%），`violations_per_kloc` 200 → **145.0**（实测 132.6/KLOC，118,647 行 / 227 文件）。
- 旧条目中"当前 misra-report 0 违规"的快照说明已统一加注（以 `baseline_*` 与本节为准）。
- ~~执行侧现状（未决）~~ **已解决（见 N 节）**：workflow 已从报告模式改为改动行级比对。

## L. 4 条 cppcheck 核心 error 清零（2026-11-07）

| 位置 | 缺陷 | 处理 | 依据 |
|------|------|------|------|
| `LinTrcv.c:750` | `LinTrcv_OpmodeType actualMode;` 未初始化即作为 out 参数传入 | 初始化为 `LINTRCV_OPMODE_SLEEP`（收发器已知初态），消除未定义读风险 | 即使 `LinTrcv_GetTja1021Mode` 当前 if/else 两分支都会写 `*OpMode`，初始化属零成本防御，且消除 addon 误判 |
| `CanTrcv.c:462` | `CanTrcv_CheckWakeupByTransceiver` 声明 `Std_ReturnType` 却无 return（fall-through = 未定义行为） | 末尾补 `return E_OK;`，无效通道静默忽略 | SWS_CanTrcv：唤醒检查成功返回 E_OK；EcuM 回调路径无对应 DET 错误码 |
| `I2c.c:309` | 死语句 `bestDivider ;`（标识符表达式语句 = 未初始化读）+ 未使用局部变量 | 删除该语句与 `uint32 bestDivider;` 声明 | 处理前 CSV 已有此条，**确认与 `I2c_SendStop` 改动无关、仍成立** |
| `config/generated/SecOC_Cfg.h:8` | 头文件保护宏与非 ASCII 字符：`SECO**С**_CFG_H` 中的 `С` 是西里尔字母 U+0421 | 在 `config/templates/SecOC_Cfg.h.j2` 与生成物中统一替换为拉丁 `C` | 生成目录在模块目录**之后**加入 include 路径，权威头 `src/bsw/services/secoc/include/SecOC_Cfg.h`（拉丁）未变 → **配置语义不变**；模板同步修正，避免重新生成时复发 |

- 验证：`cmake --build build-native -j8` **EXIT=0 / 0 error**；三份 .c 文件单独 cppcheck 已无 `error` 级诊断。
- 全量复扫（`JOBS=1 --verify`）：**TOTAL=15,737 / error=0 / warning=8**（此前 error=4）。
- 副作用（已核对，均属"字符修正后 addon 才建模到"的存量，不是新缺陷）：
  - `config/generated/SecOC_Cfg.h` 2.5 +2（两个宏此前因西里尔字母未被识别为宏）；
  - `SecOc_Lcfg.c` 5.9 +3、8.4 +1（该生成头现在真正进入 TU 的包含链）；
  - `src/bsw/os/include/Std_Types.h` 20.1 +1（21.1 修复引入的 `<stddef.h>`，MemMap 类偏差已覆盖）。

## M. 三处生产改动（Dcm → NvM → DoCan）

### M-1 Dcm 正响应 SID

- 改动：`src/bsw/services/dcm/src/Dcm.c` — `protocolState->TxBuffer[0] = (uint8)(SID + 0x40U);`（原 `SID + (uint32_t)DCM_E_POSITIVERESPONSE`，而该宏为 0x00）。
- 依据：**ISO 14229-1:2020 §10.2** —— 肯定响应 SID = 请求 SID | 0x40；与 `dcm_transfer.c` 的 `DCM_TRANSFER_RESPONSE_SID_OFFSET (0x40U)` 一致。
- 同步修正**编码了旧错误行为**的测试期望（测试侧，非放宽）：
  - `tests/unit/dcm/test_dcm_uds_matrix.c` 21 处 → `(SID | 0x40U)`，并更正文件头 NOTE；
  - `tests/bsw/services/dcm/test_dcm.c` 36 处 → `0xNN|0x40`（对已含 0x40 的值幂等，0x7F 负响应保持 0x7F）。
- 测试：`dcm_legacy_test` 4 PASS/3 FAIL → **7/7 PASS**；`dcm_uds_matrix_test` 158 断言全通过；`dcm_test` 30 条失败 → **0 失败**。

### M-2 NvM 作业挂起状态

- 改动：`NvM_WriteBlock` 入队成功分支新增 `LastResult = NVM_REQ_PENDING`。
- 依据：**SWS_NvM_00122** —— 作业挂起期间 `NvM_GetErrorStatus` 必须返回 `NVM_REQ_PENDING`。
- 同步：`tests/bsw/services/nvm/test_nvm.c` 的本地错误码宏改为直接引用 `NvM.h`（单一权威定义）。
- 测试：`nvm_legacy_test` 15 PASS/1 FAIL → **16/16 PASS**；`nvm_test` 失败 → **PASS**。

### M-3 DoCan TX/RX CanTp PDU id 冲突

- 改动：`src/bsw/services/docan/include/DoCan_Cfg.h` — RX 的 CanTp PDU id 由 0/1 改为 **2/3**（与 DCM RX id 对齐），并注明冲突成因。
- 依据：映射查找 `DoCan_FindPduMappingByCanTpId()` 仅按 CanTpPduId 检索，TX/RX 同值时 TX 映射（`RxIndicationEnabled=FALSE`）会遮挡 RX 映射，导致诊断请求无法送达 Dcm。
- 防回归用例：`tests/unit/docan/legacy_docan_cases.c` 新增 `test_docan_tx_rx_paths_are_independent` —— 断言 TX/RX PDU id 互异、TX 路径不触发 Dcm RX 回调、RX 路径不重触发 CanTp 发送。
- 测试：`docan_legacy_test` 8 PASS/1 FAIL → **10/10 PASS**。
- 全量回归：`ctest -j4` **195/195 PASS**（此前 4 个失败：matrix / dcm / nvm / os_benchmark；`os_benchmark` 为并行计时抖动，单跑与全量复跑均通过）。

## N. CI diff 门禁（改动行级）与负向验证

- `.github/workflows/misra-check.yml`：
  1. `detect-changes` 新增 **Extract added line ranges**：`git diff -U0` 的 hunk 头生成 `changed-lines.json`（文件 → 新增/修改行区间），并入 artifact；
  2. 报告解析修正：cppcheck 2.x 主位置在 `file0` / 首个 `<location>`，原实现只读 `file` 属性会拿到空路径 —— 已改为三级回退；
  3. 分类口径改为 **MANDATORY / ADVISORY / 其余=Required**（与 `ci-config.yaml misra.gates` 一致），不再把 2.5/12.1/15.1 等 Advisory 误当 Required；
  4. 阻断条件：**落在改动行上的 Mandatory（17.3 工具误报除外）+ Required 违规** → `exit 1`；存量只统计；Advisory 只打印趋势。
- 负向验证（等价复现，未触碰工作区）：
  - 构造含 Rule 8.4（Required）的临时用例 + `changed-lines.json` 指向该行 → **GATE_EXIT=1**；
  - 同报告但改动行指向其它行（模拟存量）→ **0**；空改动集合 → **0**；
  - 临时目录 `/tmp/gate_test` 已删除，工作区无残留。

## O. DET 错误码（本轮 2）——不新增，登记偏差

- 结论：**不新增 DET 错误码**。
- 登记（DoIP/SoAd 忽略点）：

| 位置 | API | 条数 | 偏差理由 |
|------|-----|-----:|---------|
| `src/bsw/ecual/doIP/src/DoIP.c` | `DoIP_SendGenericNack` | 4 | 新增错误码需改公共头（`DoIP.h`），波及面大于收益，**待 DET 表统一调整时一并处理**；失败可观测路径：SoAd 自身 DET + tester 超时 |
| 同上 | `SoAd_IfTransmit` | 3 | 同上（SoAd 已有 DET，DoIP 侧无 SWS 规定的 TX 失败码） |

- 其余 H-3 表中的 best-effort 忽略点（`EcuM_Enable/DisableWakeupSources`、`Mqtt_Tls_Close`、`Wdg_Hw` 早期预警、`Csm` mbedtls、`NvM_RedundantWriteBlock`）沿用 H-3 的理由。
