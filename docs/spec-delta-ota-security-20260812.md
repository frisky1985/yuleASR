# Spec-Delta: yuleASR OTA 信息安全增强（GB 44496-2024 / UN R156 对齐）

> 版本: v0.1 | 日期: 2026-08-12 | 作者: 小明 (Planner)
> 输入: 安当加密《OTA 安全 4 层信任链》技术文 + GB 44496-2024 + UN R156/155
> 流程: harness (Planner→Generator 小克 / Configurator 小马→Evaluator 小马)

---

## 1. 背景与差距分析（Planner 调研结论）

### 1.1 已有底座（yuleASR 现状）
| 能力 | 位置 | 状态 |
|---|---|---|
| Secure Boot 信任链（Boot ROM→BL→App） | src/bootloader/bl_secure_boot.c | ✅ 已有（R156/21434 注释对齐） |
| A/B 分区管理 | src/bootloader/bl_partition.c | ✅ 已有 |
| 失败回滚（版本历史 4 代） | src/bootloader/bl_rollback.c | ✅ 已有 |
| 签名生成/验证（ECDSA/SHA 系列） | src/bsw/services/csm | ✅ Csm_SignatureGenerate/Verify |
| 密钥管理 + 密钥轮换 API | src/bsw/services/keym + Csm_KeySetValid/KeyGenerate | ✅ 已有 API |
| 安全通信（SecOC MAC） | src/bsw/services/secoc | ✅ 已有 |
| OTA 更新流程骨架 | src/bsw/boot/Boot_Update.c | ✅ Prepare/WriteBlock/Finalize/SwapSlots |

### 1.2 缺口（对照文档 4 层信任链 + 法规条款）
| # | 缺口 | 对应文档/法规 |
|---|---|---|
| G1 | **抗回滚计数器**（硬件单调计数器，旧版本号 < 计数器则拒绝启动） | 文档 L4「抗回滚计数器」 |
| G2 | **用户告知/确认 API**（升级前告知、确认后执行） | GB 44496 §6.3 |
| G3 | **升级日志/版本可追溯**（每次升级留痕，可审计） | GB 44496 §7.2 / R156 §7.1.1 SUMS |
| G4 | **验签 3 步强化**（签名→版本绑定→完整性，版本号写入签名内容） | 文档 L3「版本签名绑定」 |
| G5 | **Configurator 配置缺失**（Secure Boot 使能/签名算法/抗回滚/证书链/密钥轮换周期/告知配置） | 文档 L1-L4 + 法规 |

### 1.3 范围界定
- **本次不改**：HSM 硬件适配（已有）、SecOC（已有）、TLS/FEK 传输层（云侧，BSW 外）
- **yuleASR 代码（小克）**：G1-G4
- **yuleASR-Configurator（小马）**：G5 + 审查

---

## 2. 需求（SHALL 语句）

### RS-OTA-01 抗回滚计数器（G1）
- SHALL 提供单调递增抗回滚计数器（存储在 NVM，只增不减）
- SHALL 启动验签时比对固件版本号与计数器，版本低于计数器则拒绝启动并返回 BL_SB_ERROR_ROLLBACK_PROTECTION
- SHALL 提供读取/写入/递增 API（Boot_AntiRollback_Read/Write/Increment）
- SHOULD 计数器持久化到独立 NVM 区（防回滚攻击）

### RS-OTA-02 用户告知与确认（G2）
- SHALL 提供升级前用户确认 API（Boot_Update_RequestUserConfirm）
- SHALL 未确认时不得开始升级写入
- SHOULD 支持超时自动取消（可配置）

### RS-OTA-03 升级日志可追溯（G3）
- SHALL 记录每次升级：时间戳/版本号/来源/签名结果/结果
- SHALL 日志持久化（NVM），可经诊断读取
- SHOULD 日志循环缓冲（可配置条数）

### RS-OTA-04 验签 3 步强化（G4）
- SHALL 验签流程严格 3 步：签名验证 → 版本绑定校验（版本号在签名内容内）→ 完整性（哈希）
- SHALL 任一失败拒绝升级并记录日志
- SHALL 支持签名算法可配置（ECDSA P-256 / SM2，经 Csm 调度）

### RS-OTA-05 Configurator 配置（G5）
- SHALL boot.json 增加：SecureBootEnable / SignatureAlgorithm / AntiRollbackEnable / CertChainMaxDepth / KeyRotationPeriod / UserConfirmRequired / UpgradeLogEnable / UpgradeLogMaxEntries
- SHALL 新增 Ota.json（或扩展 boot.json）承载以上配置，类型/枚举按 AUTOSAR 风格
- SHOULD 与 AUTOSAR 官方定义层（STM32F4_MCAL 开源项目 OTA/SECOC/CSM 模块）对照校准

---

## 3. 验收标准（sprint-contract done 定义）

### yuleASR 代码（小克）
1. 新增 bl_antrollback.c/h（或并入 bl_secure_boot）：Read/Write/Increment API + NVM 持久化
2. Boot_Update 增加 UserConfirm 流程 + 升级日志模块（bl_upgrade_log.c/h）
3. bl_secure_boot 验签 3 步强化 + 版本签名绑定
4. 单元测试覆盖：抗回滚（正常/拒绝/持久化）、确认（通过/超时/未确认阻止）、日志（写/读/循环）、验签 3 步（各失败路径）
5. `make test` 或现有测试框架全绿（不破坏现有 bootloader 测试）

### Configurator（小马）
1. boot.json 增加 8+ 配置项（类型/枚举正确）
2. 新增 Ota.json 或等价配置 schema，与 boot.json 关联
3. 对照 AUTOSAR 官方定义校准（如有对应模块）
4. `npx vitest run packages/@yuletech/core/src/schema/__tests__/schema-validation.test.ts` 全绿
5. 审查小克代码（独立 Evaluator）：4-Agent 审查矩阵 + 报告

---

## 4. 分工

| 角色 | Agent | 任务 | 产物 |
|---|---|---|---|
| Planner | 小明 | spec-delta + sprint-contract + 收口 | 本文档 |
| Generator | 小克 (claude-agent) | yuleASR 代码 G1-G4 + 单测 | 代码 + 测试全绿 |
| Configurator | 小马 (hermes-agent) | Configurator G5 + 独立审查 | schema + 审查报告 |

## 5. 风险
- 抗回滚计数器 NVM 磨损：SHOULD 用均衡写入或限制递增频率
- 用户确认与现有 Boot_Update 流程集成点：需小克确认 Boot_Update.c 现状后设计
- 版本签名绑定可能影响现有镜像格式：向后兼容（旧格式走 2 步验签）或明确升级格式版本
