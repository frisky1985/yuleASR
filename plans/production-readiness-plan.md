# yuleASR 量产准备计划 (Production Readiness Plan)

> 版本: v0.2 (PR-0.1 基线重建后)
> 生成日期: 2026-11-01
> 目标: 基于第一性原理的量产就绪评估 + 分层任务拆解 + 反幻觉纪律

---

## 0. 反幻觉纪律 (Anti-Hallucination Rules)

1. **所有数字必须来自脚本输出** — `reports/baseline/summary.json`、`module-matrix-summary.json`、`misra-gate.json`。禁止凭记忆或估计填写。
2. **未验证的事实标记为 `[PENDING]`** — 不允许用"大约""可能"等模糊词替代真实数据。
3. **决策必须有记录** — 每个 D-n 决策记录选项、选择、理由、遗留风险。
4. **风险必须显式登记** — 不可静默接受 (如 FreeRTOS 无 ISO 26262 认证)。
5. **门禁阈值来自 `.yuleosh/ci-config.yaml`** — 不重复定义，不硬编码。

---

## 1. 事实基线 (Facts Baseline)

> 数据来源: `tools/capture_baseline.sh` → `reports/baseline/summary.json` + `module-matrix-summary.json`
> 捕获日期: 2026-10-31, git_sha: 26ac7e6d (dirty)

| ID | 事实 | 值 | 来源 |
|:---|:-----|:---|:-----|
| F1 | 模块实现总数 (src/bsw 下) | 103 | module-matrix-summary.json total_rows |
| F2 | 唯一模块名数 (大小写不敏感) | 94 | module-matrix-summary.json unique_modules |
| F3 | 精确重复实现组数 | 6 (ramtst, fim, ipdum, j1939tp, memif, xcp) | duplicate_groups |
| F4 | 大小写冲突组数 | 3 (doIP/doip, ethSm/ethsm, linSM/linsm) | name_collision_groups |
| F5 | 配置模板总数 | 92 (mcal 20 / ecual 25 / services 46 / os 1) | templates 目录计数 |
| F6 | 无模板模块数 (NO_TEMPLATE) | 2 (Boot, Cdd) | status_counts |
| F7 | 模板层级不匹配数 (TEMPLATE_LAYER_MISMATCH) | 9 (重复模块, 延迟至 PR-1.3) | status_counts |
| F8 | 无测试模块数 (NO_TEST) | 18 | status_counts |
| F9 | 覆盖率为 0 的模块数 (COVERAGE_ZERO) | 12 | status_counts |
| F10 | 低覆盖率模块数 (COVERAGE_LOW, <60%) | 17 | status_counts |
| F11 | MISRA required 违规总数 | 58 | summary.json misra.required |
| F12 | MISRA total 违规总数 | 1546 (deviated 1488) | summary.json misra.total |
| F13 | 单元测试总数 / 失败数 | 192 / 2-3 (Safety_Stack_UnitTest, test_dds1_tbf, Safety_Manager_UnitTest; 后两者偶发) | summary.json ctest |
| F14 | 行覆盖率 / 分支覆盖率 | 63.54% / 50.99% (295 文件) | summary.json coverage |
| F15 | 无需求模块数 (NO_REQUIREMENT) | 87 | status_counts |

---

## 2. 决策寄存器 (Decision Register)

| ID | 决策 | 选项 | 选择 | 理由 | 遗留风险 |
|:---|:-----|:-----|:-----|:-----|:---------|
| D1 | 安全目标等级 | ASIL-A/B/C/D | **ASIL-D** | BSW 含制动/转向/动力相关服务 (Dem/Dcm/WdgM/E2E/SecOC)，按最高等级统一要求 | — |
| D2 | OS 路线 | FreeRTOS / 自研 / 商用 RTOS | **FreeRTOS** | 现有实现基于 FreeRTOS V10.6.x/V11.x，迁移成本最低 | **RISK-01**: FreeRTOS 无 ISO 26262 认证，需在安全案例中声明使用条件或改用认证版 |
| D3 | 模块集合 (裁剪 vs 全量) | 全量 97 模块 / 核心子集 | **[PENDING]** | 待 PR-1.3 定主后决定：9 个重复模块 (doIP/FiM/Xcp/EthSM/IpduM/J1939Tp/LinSM/MemIf/RamTst) 需选 canonical 实现 | RISK-02: 重复实现可能导致链接冲突 / 维护混乱 |

### 风险登记

- **RISK-01**: FreeRTOS 缺少 ISO 26262 功能安全认证。
  - 缓解: 安全案例中声明 FreeRTOS 仅用于非安全相关调度，或采购 SafeRTOS 认证版。
  - 状态: 开放，需安全经理批准。
- **RISK-02**: 9 个模块存在 ecual/services 双份实现。
  - 缓解: PR-1.3 选定 canonical，删除另一份。
  - 状态: 开放，PR-1.3 处理。

---

## 3. 分层任务拆解 (8 Layers)

### L0 — 基础设施 (Infrastructure)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L0-1 基线捕获脚本 | `bash tools/capture_baseline.sh --record-only` 退出码 0，产出 summary.json | P0 |
| L0-2 可复现性校验 | 两次运行 summary.json 除时间戳/git_sha 外完全一致 | P0 |
| L0-3 模块关联矩阵 | `python3 tools/generate_module_matrix.py` 产出 CSV + summary JSON | P0 |

### L1 — 构建与测试 (Build & Test)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L1-1 修复失败用例 | ctest fail=0 (当前 3 个失败: Safety_Stack_UnitTest, test_dds1_tbf, Safety_Manager_UnitTest) | P0 |
| L1-2 覆盖率门禁硬化 | `coverage.enabled=true`，line≥60% / branch≥50% 阻断 CI | P0 |
| L1-3 覆盖率提升 | line≥80% / branch≥60% (量产目标) | P1 |

### L2 — 静态分析 (MISRA)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L2-1 MISRA required 清零 | `misra.gate.required=0` (当前 58) | P0 |
| L2-2 通配偏差逐条复核 | `wildcard_deviations` 从 pending 转为逐条批准 | P1 |
| L2-3 MISRA total ratchet | total ≤ 1547 (只降不升) | P0 |

### L3 — 需求可追溯性 (Traceability)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L3-1 需求覆盖率 | 所有模块 requirement_count > 0 (当前 NO_REQUIREMENT 模块清零) | P0 |
| L3-2 人工审查 | human_reviewed_requirements == requirement_count | P1 |
| L3-3 导出 required MISRA 违规 CSV | `reports/misra-required-violations.csv` 含 rule/file/line/reason | P1 |

### L4 — 模块治理 (Module Governance)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L4-1 重复模块定主 | 9 个重复模块选定 canonical，删除另一份 | P0 |
| L4-2 模板层级对齐 | TEMPLATE_LAYER_MISMATCH=0 | P0 |
| L4-3 无模板补齐 | NO_TEMPLATE=0 | P1 |

### L5 — 安全机制 (Safety Mechanisms)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L5-1 E2E 保护覆盖 | Com 信号 E2E 保护率 100% (安全相关) | P1 |
| L5-2 SecOC 集成 | SecOC 与 PduR/Com 集成测试通过 | P1 |
| L5-3 WdgM 监控 | WdgM 监督实体覆盖率 ≥ 90% | P1 |

### L6 — 集成与发布 (Integration & Release)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L6-1 系统集成测试 | tests/integration/ 全部通过 | P1 |
| L6-2 发布包构建 | `release/` 产物完整，签名校验通过 | P2 |
| L6-3 文档完整性 | API 文档 + 安全案例 + 集成指南齐全 | P2 |

### L7 — CI/CD 门禁 (CI Gates)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L7-1 三门禁接入 CI | coverage + misra + traceability 全部 fail-closed | P0 |
| L7-2 可复现性 CI | 每次 PR 运行 capture_baseline 并 diff 摘要 | P1 |

---

## 4. 优先级总览

### P0 (量产阻断)
- L0-1/2/3: 基线基础设施
- L1-1/2: 测试修复 + 覆盖率门禁硬化
- L2-1/3: MISRA required 清零 + ratchet
- L3-1: 需求覆盖
- L4-1/2: 重复模块定主 + 模板对齐
- L7-1: CI 门禁接入

### P1 (量产成熟度)
- L1-3: 覆盖率提升至 80/60
- L2-2: 通配偏差复核
- L3-2/3: 人工审查 + MISRA 导出
- L4-3: 无模板补齐
- L5-1/2/3: 安全机制
- L6-1: 系统集成测试
- L7-2: 可复现性 CI

### P2 (发布完善)
- L6-2/3: 发布包 + 文档

---

## 5. PR 路线图

| PR | 名称 | 内容 | 依赖 |
|:---|:-----|:-----|:-----|
| PR-0.1 | 基线捕获 | capture_baseline.sh + summary.json | — |
| PR-0.2 | 模块矩阵 | generate_module_matrix.py + CSV | — |
| PR-0.3 | CI 三门禁 | coverage.enabled=true + misra/trace fail-closed | PR-0.1, PR-0.2 |
| PR-1.3 | 重复模块定主 | 9 个模块选 canonical | PR-0.2 |
| PR-3.1 | MISRA required 导出 | CSV 导出 58 条 required 违规 | PR-0.1 |
| PR-4.1 | 失败用例修复 | 3 个 ctest 失败修复 | PR-0.1 |

---

## 6. 复杂度整改排期（2026-11-08 立项）

> 依据：`reports/quality/production-readiness-2026-11-08.md` §3.4 / §5.9。
> 文件规模（170 个 >500 行）已按项目决策记为偏差豁免，**不在本排期范围**。

### 6.1 基线与目标

| 指标 | 基线（lizard，2026-11-08） | 目标 | 门禁 |
|:-----|:--------------------------|:-----|:-----|
| CCN > 30 | 13 | **0** | 阻断 |
| CCN > 20 | 47 | ≤ 10（其余转偏差） | 阻断 |
| CCN > 10 | 355 | 只降不升（棘轮） | 偏差 |
| 函数 > 100 行 | 63 | **0** | 阻断 |
| 函数 > 50 行 | 587 | 只降不升（棘轮） | 偏差 |
| 新增函数 | — | 主逻辑函数 CCN ≤ 10；分支拆分产生的取值/比较助手 ≤ 15 | 硬门禁 |

### 6.2 批次划分（按风险，非单纯按 CCN 排序）

| 批次 | 范围 | 函数数 | 风险 | 认证/测试依赖 |
|:-----|:-----|-----:|:-----|:--------------|
| **B1** | 解析/判断类低风险：`TcpIp_LocalIsValidTransition`(33)、`evaluate_predicate`(32)、`lexer_read_token`(31)、`SomeIpXf_Detransform`(36)、`SomeIpXf_Transform`(31) | 5 | 低 | 现有单测即可 |
| **B2** | 核心状态机：`NvM_MainFunction`(60, 451 行)、`CanTp_ProcessRxFrame`(52, 244 行) | 2 | 高 | 需先补状态迁移单测 |
| **B3** | 加密栈：`Csm_KeyDerive`(51)、`Csm_KeyExchangeCalcSecret`(40)、`Csm_KeyExchangeCalcPubVal`(39)、`csm_execute_crypto_op`(48)、`Crypto_AesProcessDecrypt`(39)、`Crypto_AesProcessEncrypt`(36) | 6 | 高 | 需加密向量回归 |
| **B4** | CCN 21–30：34 个（含 `content_filtered_topic`、`TcpIp`、`RamTst`、`EthTrcv`、`bl_secure_boot`、`isotp_core`、`Mqtt_Tls`、`LinSlave`、`e2e_dds_integration` 等） | 34 | 中 | 按模块单测 |
| **B5** | 函数 > 100 行：63 个（与 B1–B4 有重叠，去重后按文件拆函数） | 63 | 低 | 机械拆分 |

**执行顺序**：B1 → B2 → B3 → B4 → B5。理由：B1 见效快、风险低，可先建立重构与验证范式；B2/B3 是安全相关核心栈，必须在单测补强后再动；B5 在复杂度收敛后做机械拆分，避免重复改动。

### 6.3 每批验收（缺一不可）

1. `lizard src --csv` 复测：目标函数 CCN 达标，全仓 >10 / >50 行计数**不上升**（棘轮）
2. `ctest --test-dir build-native`：**209/209 全绿**
3. ASan/UBSan 构建无新增报错
4. `git diff` 仅含目标函数及其调用点，无行为变更（纯等价重构）
5. 报告 §3.4 / §5.9 数据同步更新

### 6.4 状态跟踪

| 批次 | 状态 | 完成函数 | 剩余 |
|:-----|:-----|:---------|:-----|
| B1 | ✅ 完成 | **5/5**：`TcpIp_LocalIsValidTransition` 33→4、`evaluate_predicate` 32→6、`lexer_read_token` 31→6、`SomeIpXf_Transform` 31→10、`SomeIpXf_Detransform` 36→9 | — |
| B2 | ✅ 完成 | **2/2**：`NvM_MainFunction` 60→3（拆 13 个助手）、`CanTp_ProcessRxFrame` 52→7（按帧类型拆 4 个助手 + MetaData 助手） | — |
| B3 | ✅ 完成 | **6/6**：`Crypto_AesProcessEncrypt` 36→11、`Crypto_AesProcessDecrypt` 39→11（提取 9 个共用助手）、`csm_execute_crypto_op` 48→7（按作业类型拆 6 个分支助手 + AES 公共入口）、`Csm_KeyDerive` 51→7、`Csm_KeyExchangeCalcPubVal` 39→9、`Csm_KeyExchangeCalcSecret` 40→8（共享 KDF 助手：用途权限检查/输入拼接/硬件哈希+XOR 回退 + 每函数 HW/SW 助手） | — |
| B4 | ✅ 完成 | **34/34**（7 组提交 6faef8e/18b3ffa/7525e3a/d5c7680/696335c/c7fbe42/23be86b）：组1 bootloader `bl_secure_boot_verify_cert_chain` 29→9、`scan_slots` 23→9；组2 E2E/DDS `E2E_SM_GetNextState` 24→8、`E2E_DDS_Check` 28→7、`E2E_DDS_Subscribe` 23→5、`dds_access_parse_permissions_xml` 24→9、`dds_crypto_encrypt_aes_gcm` 23→9；组3 Lin `LinSlave_HandleRxData` 28→8、`LinMaster_StateMachineHandler` 27→7、`LinNm_StateMachine` 27→8；组4 IsoTp `IsoTp_ProcessRxFrame` 28→10、`IsoTp_TxStateMachine` 23→8；组5 Crypto/HSM `S32K312_Hsm_AesGcmEncrypt` 26→4、`S32K312_Hsm_SelfTest` 23→6、`Crypto_MbedTLS_ProcessJob` 25→10、`Csm_KeyGenerate` 24→7、`Csm_Cfg_Aes128Cbc` 23→5；组6 网络 `EthTrcv_UpdateLinkState` 29→3、`UdpNm_ProcessStateMachine` 22→8、`EthSwt_ForwardFrameVlan` 22→6、`tcpip_socket_setopt` 22→3、`CanTp_MainFunction` 22→5、`Can_Write` 21→8、`Mqtt_ProcessStateMachine` 21→9、`Mqtt_Tls_CreateContext` 28→5；组7 `RamTst_ExecuteMarchC` 29→5、`BswM_EvaluateExpression` 25→10、`Xcp_ProcessCommand` 25→10、`RamSafety_RunMarchC` 25→7、`RamSafety_TriggerTest` 23→6、`Icu_ProcessInterrupt` 23→10、`Tel_Diag_ReadData` 21→4、`Dcm_ProcessReadDTCInformation` 21→2、`Boot_Update_Finalize` 21→6 | — |
| B5 | ✅ 完成 | **29/29**（6 组提交 d96a2f6/d3a89e7/a8c1ff2/4b656b8/2784abf/a93f1cd）：组1 Dcm 传输 `Dcm_TransferProcessRequestDownload` 191→65、`Dcm_TransferProcessRequestUpload` 191→65、`Dcm_TransferProcessTransferData` 117→60、`Dcm_ProcessRoutineControl` 110→65；组2 HSM/Boot `S32K312_Hsm_AesEcbEncrypt` 111→36、`S32K312_Hsm_AesEcbDecrypt` 108→36、`S32K312_Hsm_EccVerify` 102→34、`Boot_Loader_Main` 126→49、`sbl_main_boot` 102→28（同文件顺带拆 `EccSign` CCN 19→4、`sbl_main_init` CCN 16→8）；组3 Init `Dem_Init` 122→45、`Icu_Init` 121→27、`WdgM_Init` 116→63、`RamSafety_RunStartupTest` 106→56；组4 网络 `tcpip_dhcp_main_function` 117→19、`DoIP_ProcessDiagnosticMessage` 108→70、`DoIP_SendVehicleAnnouncement` 106→28、`DoIP_SoAdIfRxIndication` 106→64、`Mqtt_EncodeConnect` 108→25；组5 `BswM_CollectRulePorts` 111→44、`SecOC_ProcessRxPdu` 111→34、`FlStSt_LocalRunMarchCStep` 110→36、`NvM_EccHandler_HandleReadError` 109→60、`HeapProtection_Malloc` 105→33、`Fee_ProcessWriteJob` 101→48；组6 测试 `test_antrollback_health_gate` 128→23、`test_secure_boot_cert_chain` 116→94、`test_secure_boot_strict_verify` 112→99、`test_sm_interface_requests` 119→16、`test_function_group_basic` 108→88（断言与执行顺序逐字保留） | — |

### 6.5 进度快照（2026-11-09，B5 完成后，全部批次收官）

| 指标 | 基线 | 当前 | 变化 |
|:-----|-----:|-----:|:-----|
| CCN > 30 | 13 | **0** | −13 ✅ 达标 |
| CCN > 20 | 47 | **0** | −47 ✅ 达标（目标 ≤10，超额完成） |
| CCN > 10 | 355 | **313** | −42 |
| 函数 > 50 行 | 587 | **568** | −19 |
| 函数 > 100 行 | 63 | **0** | −63 ✅ 达标 |
| ctest | 209/209 | **209/209** | 保持全绿 |

> **说明**：① `content_filtered_topic.c` 拆分出的 `cft_read_predicate_field`(CCN 13) 与 `cft_compare_numeric_field`(CCN 15) 属 switch 分支助手，落在 ≤15 的偏差带内，未突破"助手 ≤15"约定；主逻辑函数 `evaluate_predicate` 32→6、`lexer_read_token` 31→6。② `SomeIpXf.c` 拆分出 8 个助手函数，其中 `SerializeScalar`/`DeserializeScalar`(CCN 14、len 57/58) 同为分支助手，落在 ≤15 偏差带内；两个主函数降至 ≤10。全仓 CCN>10 净降有限即因拆分助手从原大函数中独立计数所致，属预期现象；函数 >50 行 +2 亦为此原因（但 >100 行 −2，收益真实）。③ 修复了一处命名冲突：静态助手原名 `SomeIpXf_DeserializeString` 与公共 API 同名，改为 `SomeIpXf_DeserializeStrElem`。④ **B2 行为修正记录**：`NvM_MainFunction` 原实现中 READ 冗余副本 CRC 不匹配路径的 `jobComplete = FALSE;` 处于注释状态（标记 `[MISRA Advisory] Redundant`），即原行为是该路径当周期完结作业；重构严格保留此语义（`Nvm_ValidateReadCrc` 冗余路径返回 TRUE），差异曾导致 3 个 nvm_test 用例失败，修正后 209/209 全绿。⑤ `CanTp_ProcessRxFrame` 拆分出 `CanTp_StoreRxMetaData` 与 `CanTp_ProcessRxSF/FF/CF/FC` 共 5 个助手，`CanTp_ProcessRxCF`(CCN 15) 落在 ≤15 偏差带。⑥ **B3 加密栈语义保留记录**：`Csm_KeyDerive` 软件 KDF 成功路径中 `hashOutLen` 截断赋值被注释（`[MISRA Advisory] Redundant`），即原语义为成功路径恒按 `CSM_MAX_HASH_LENGTH` 拷贝——重构以 `Csm_KdfHash(clampHwLen=FALSE)` 严格保留；PubVal/Secret 两函数该赋值有效，走 `clampHwLen=TRUE`。三处 XOR 回退系数各异（KeyDerive `counter+j`、PubVal `j*0x37`、Secret `j*0x53`），统一由 `Csm_KdfXorFallback(mul, add)` 参数化保留。⑦ B3 拆分助手 `Csm_KeyDeriveTryHw`(14)、`Csm_KeyExchangePubValTryHw`(13)、`Csm_KeyExchangeSecretTryHw`(13) 落在 ≤15 偏差带；三个主函数均降至 ≤10。⑧ 函数 >50 行较 B2 快照 +5（588→593）为拆分助手独立计数所致，>100 行 −6（59→53）收益真实。⑨ **B4 语义保留记录**：`Xcp_ProcessCommand` 保留原死代码路由（`cmd >= 0xC0` 先行短路使 DAQ/PGM 分支不可达）；`RamSafety_TriggerTest` QUICK 路径保留原实现"末尾 `result = E_OK;` 覆盖校验结果"的行为并加注释；`Csm_KeyGenerate` 保留"异步入队失败不做软件回退"语义；`S32K312_Hsm_SelfTest` 保留 Test1 未启用=整体失败、Test2/3 未启用=中性跳过的门控差异；`RamTst_ExecuteMarchC` 以函数指针表保留 M0–M5 各相位校验模式/方向切换/降序地址三元式；`tcpip_socket_setopt` 与 `Tel_Diag_ReadData` 首批拆分助手（CCN 20/18）超出 ≤15 偏差带，均已返工二次拆分达标。⑩ 全仓 CCN 16–20 带现存 63 个，属 ≤20 目标带内的棘轮观察项。⑪ 行数口径沿用量产报告权威命令（`lizard src --csv`，col2=CCN、col5=函数跨行 length，含空行与注释），与 6.1 基线一致；>100 行 63→29 中 B1–B3 贡献 −10、B4 贡献 −24。⑫ **B5 语义保留记录**：`DoIP_SendVehicleAnnouncement` 保留"声明头部 payload=32 但 syncStatus 非零会写第 33 字节"的原实现越界疑点（加注释，未修复）；`HeapProtection_Malloc` 保留冗余 `free_size` 检查；`Boot_Loader_Main`/`sbl_main_boot` 保留 goto fail 失败路径转换顺序；测试函数拆分将 `TEST_ASSERT` 宏内 `return -1` 的失败传播改为助手返回值逐级透传，断言文本与执行顺序逐字保留。⑬ 组2 验收时同文件顺带拆分 `EccSign`(CCN 19→4) 与 `sbl_main_init`(CCN 16→8)，超出 B5 行数范围但消除 CCN>15 残留，属低风险附加收益。⑭ **B5 完成后全仓**：CCN 峰值 20（`dds_security_error_string`、`CanSM_MainFunction`），CCN 16–20 带剩 49 个，平均 CCN 3.92（7,410 函数）；6.1 三项阻断门禁（CCN>30=0、CCN>20≤10、>100 行=0）全部达标，CCN>10/>50 行两项棘轮持续下降。
