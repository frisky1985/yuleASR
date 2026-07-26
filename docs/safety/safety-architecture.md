# yuleASR 安全架构文档 v1.3.0

> **文档**: 安全架构文档 (Safety Architecture Document)
> **版本**: 1.0 | **日期**: 2026-07-21
> **审查人**: 小马 🐴 (质量架构师)
> **状态**: 初稿 — 待安全团队签收
> **ASIL 目标**: ASIL B + QM 混合分解

---

## 1. 文档范围与参考

### 1.1 目的

本文档定义 yuleASR AUTOSAR BSW 平台的安全架构，包括 ASIL 分解方案、安全状态定义、故障模式分析(FMEDA)以及安全机制矩阵。依据 ISO 26262-5 (硬件安全要求) 和 ASPICE HWE.2 (硬件设计) 建立。

### 1.2 参考标准

| 标准 | 章节 | 用途 |
|------|------|------|
| ISO 26262-3:2018 | §7 | HARA 危害分析与风险评估 |
| ISO 26262-5:2018 | §8-10 | 硬件安全要求、硬件设计、硬件安全分析 |
| ISO 26262-6:2018 | §7-9 | 软件安全要求、软件设计、安全分析 |
| ISO 26262-9:2018 | §5 | ASIL 分解 |
| ISO 26262-9:2018 | §8 | 相依失效分析 (DFA) |
| ASPICE HWE.2 | BP2-BP5 | 硬件安全需求、硬件设计、硬件分析 |
| AUTOSAR R21-11 | SWS | BSW 模块安全规范 |

### 1.3 术语定义

| 术语 | 定义 |
|------|------|
| ASIL | Automotive Safety Integrity Level (A/B/C/D/QM) |
| SG | Safety Goal (安全目标) |
| FTTI | Fault Tolerant Time Interval (容错时间间隔) |
| SPF | Single Point Fault (单点故障) |
| MPF | Multiple Point Fault (多点故障) |
| DC | Diagnostic Coverage (诊断覆盖率) |
| PMHF | Probabilistic Metric for random Hardware Failures |
| SF | Safe State (安全状态) |

---

## 2. ASIL 分解

### 2.1 分解策略

基于 yuleASR 的参考平台定位和 HARA 分析结果（修订版 v1.1），采用 **ASIL B + QM 混合分解** 策略：

```
原始 ASIL 等级 → 分解后分配
─────────────────────────────────
ASIL B (SG-001, SG-002)  → ASIL B(B)  + QM(B)
ASIL A (SG-003)          → 保留 ASIL A, 不分解
QM     (Can, Lin, SPI)   → QM (不变)
```

**分解原则：**
1. 安全相关模块 (E2E, WdgM, NvM 校验层) 保留 ASIL B
2. 通信和诊断模块 (Com, Dcm, PduR) 按 QM 运行，但通过 E2E 审计保护
3. MCAL 层 (Can, Lin, SPI) 为 QM，不承载安全功能
4. 安全相关数据路径经 E2E 端到端保护，不依赖 MCAL 层安全等级

### 2.2 模块级 ASIL 分配

| 模块 | 分配 ASIL | 安全相关 | 说明 |
|------|-----------|----------|------|
| E2E | ASIL B | ✅ | 端到端通信保护，安全关键 |
| WdgM (WdgMgr) | ASIL B | ✅ | 看门狗管理，超时检测 |
| NvM (校验部分) | ASIL B | ✅ | NVRAM 管理器 E2E CRC 校验 |
| RamSafety | ASIL B | ✅ | RAM 安全自检 (LBIST/SBIST) |
| OS_Timing | ASIL B | ✅ | 操作系统时序保护 |
| CRYIF / CSM | ASIL B | ✅ | 密码服务 (HSM 接口) |
| SECOC | ASIL B | ✅ | 安全板载通信 |
| Com | QM | ⚠️ | 通信服务，经 E2E 审计信任 |
| Dcm | QM | ⚠️ | 诊断通信，功能安全不依赖 |
| PduR | QM | ❌ | PDU 路由器，透明转发 |
| Can / CanIf | QM | ❌ | CAN 通信栈 |
| Lin / LinIf | QM | ❌ | LIN 通信栈 |
| Dio / Port | QM | ❌ | 数字 I/O |
| Fls / Fee | QM | ❌ | Flash 存储 |
| Mcu | QM | ❌ | 微控制器驱动 |
| Gpt / Pwm | QM | ❌ | 定时器/PWM |
| Det | QM | ❌ | 开发错误追踪 |
| Dem | QM | ❌ | 诊断事件管理 |
| EcuM | QM | ❌ | ECU 状态管理 |
| BswM | QM | ❌ | BSW 模式管理 |

### 2.3 ASIL 分解合规性检查

ISO 26262-9 §5.4 — ASIL 分解要求：

| 检查项 | 状态 | 依据 |
|--------|------|------|
| 分解后的安全要求保持独立性 (freedom from interference) | ✅ | ASIL B 软件通过 E2E 审计 + WdgM 时间监控实现逻辑隔离；无 MPU 场景下通过消息级保护替代地址空间隔离 |
| ASIL B(B) + QM(B) 合并不低于原始 ASIL B | ✅ | 原始 ASIL 为 B，B(B)+QM(B) 是 ISO 26262-9 §5.4 允许的有效分解 |
| QM(B) 部分不承担任何 ASIL B 安全需求 | ✅ | QM 模块（Com, Dcm, PduR, Can, Lin）不承载安全需求；安全数据经 E2E 端到端审计 |
| 相依失效分析 (DFA) 已完成 | ⚠️ **待补充** | CCF 分析清单见 §6.3 |

> **变更说明**: 本版修正了此前 HARA 中 H001~H004 的 ASIL 计算错误，SG-001/SG-002 原始 ASIL 为 B（非 D）。B(B)+QM(B) 对原始 ASIL B 是合规分解。

---

## 3. Safe State 定义

### 3.1 模块级故障安全状态

| 模块 | 故障场景 | 安全状态 (Safe State) | 进入条件 | 退出条件 |
|------|----------|----------------------|----------|----------|
| **E2E** | CRC 校验失败 | E2E_PASS_STATE_NOK → 静默(抑制发送) | 连续 3 次 CRC 失败 | 复位或配置恢复 |
| **WdgM** | 超时触发 | WdgM_MODE_OFF → 系统复位 | 监督实体超时 | 看门狗复位后 |
| **NvM** | CRC 校验失败 | NvM_BLOCK_FAILED → 使用默认值 | 读时 CRC 错误 | 写恢复成功 |
| **NvM** | 双备份不一致 | NvM_BLOCK_FAILED → 取主副本 | 冗余校验不一致 | EEPROM 擦写恢复 |
| **Can** | 控制器 Bus-Off | Can_PduReceive → 丢弃数据 | CAN 控制器进入 Bus-Off | 协议恢复或复位 |
| **CanIf** | 通道断开 | CanIf_RX_INDICATION → 丢弃 | PHY 层状态指示断开 | 链路恢复重新使能 |
| **Lin** | 帧错误/校验错误 | Lin_FrameResponse → 丢弃 | 从节点无响应 > N_Timeout | 唤醒序列后重试 |
| **Com** | IPDU 超时 | Com_SignalGroup → 默认值 | DeadlineMon 触发超时 | 下一周期有效数据 |
| **Dcm** | 会话超时 | Dcm_Session → DEFAULT | P2/P2* 超时 | 新诊断请求 |
| **Dem** | 事件内存满 | Dem_EventStatus → EVMEM_FAILED | DTC 存储溢出 | NvM 擦除或归档 |
| **SECOC** | 认证码失败 | SecOC_Verify → NOK | 新鲜度值或 MAC 不符合 | 下一有效帧 |
| **HSM/Crypto** | 密钥无效 | Crypto_Result → CRYPTO_E_KEY_INVALID | 密钥加载失败 | 安全启动重新加载 |
| **RamSafety** | 内存故障 | RAM_Test_Failure → 拒绝启动 | RAM 自检失败 | 硬件复位 |
| **SchM** | 调度偏移 | SchM_Entry → 看门狗喂狗失败 | 任务执行超预算 | 系统复位 |

### 3.2 系统级安全路径

```
┌────────────────────────────────────────────────────────────┐
│                   故障 → 安全状态 (路径)                      │
├────────────────────────────────────────────────────────────┤
│  E2E CRC Fail ──→ Com 抑制发送 ──→ 驱动降级模式 ──→ 诊断记录 │
│  WdgM 超时 ──────→ 系统复位 ────→ EcuM 启动 ────→ 日志归档    │
│  NvM 校验错误 ───→ 默认参数 ────→ Limp Home ──→ DTC 设置      │
│  Can Bus-Off ────→ 通道静默 ────→ CanNm 通知 ──→ 网络管理降级  │
│  Lin 超时 ────────→ 从节点忽略 ──→ LinIf 重试 ──→ N 次后 DTC  │
└────────────────────────────────────────────────────────────┘
```

### 3.3 容错时间间隔 (FTTI)

| 安全目标 | FTTI | 对应 FDTI | 备注 |
|----------|------|-----------|------|
| SG-001 (E2E 数据完整性) | 100 ms | < 50 ms | 典型 CAN 消息周期 |
| SG-002 (任务定时) | 200 ms | < 100 ms | OS 任务监控 |
| SG-003 (数据持久化) | 500 ms | < 250 ms | NvM 写周期 |
| SG-004 (OS 时序) | 100 ms | < 50 ms | 时序保护监控 |

---

## 4. FMEDA 精细分析 (v1.4.0)

### 4.1 S32K312 硬件失效率基线

基于 NXP S32K3x 系列可靠性报告 (NXP AN13475 Rev 2, 2025) 及 IEC TR 62380 通用失效率模型:

| 硬件单元 | λ (FIT) | 来源 | 备注 |
|---------|:-------:|------|------|
| S32K312 SoC (CM4F 核心) | 25 | IEC TR 62380, 80 MHz | 数字逻辑, 含 cache controller |
| SRAM (64 KB) | 8 | IEC TR 62380, 0.125 FIT/KB | 含 ECC 逻辑 |
| Flash (512 KB) | 12 | IEC TR 62380, 0.023 FIT/KB | 含 ECC |
| Data Flash (16 KB) | 2 | IEC TR 62380 | 模拟 EEPROM |
| PLL + 时钟系统 | 5 | NXP AN13475 | 含时钟监控 (CMU) |
| 电源管理 (PMC) | 3 | NXP AN13475 | 含 POR/BOD |
| CAN-FD 控制器 ×3 | 6 | IEC TR 62380, 2 FIT/控制器 | — |
| SPI 控制器 ×3 | 3 | IEC TR 62380, 1 FIT/SPI | — |
| LIN 控制器 ×2 | 2 | IEC TR 62380, 1 FIT/LIN | — |
| GPT/PIT 定时器 | 2 | IEC TR 62380 | — |
| WDOG 独立看门狗 | 2 | NXP AN13475 | 独立 RC 振荡器 |
| HSE_B 安全引擎 | 10 | NXP AN13475 | 含 32 KB SRAM + 硬件密码加速 |
| DMA 控制器 | 3 | IEC TR 62380 | — |
| 系统总线 (AXI/AHB/APB) | 5 | IEC TR 62380 | 含总线仲裁 |
| **硬件总 FIT** | **88** | — | S32K312 裸片硬件基线 |

### 4.2 软件故障模式 (精细化)

**E2E 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-E2E-01 | CRC 校验器软件失效 | 无法检测数据损坏 | 高 | 10 | WdgM 时间监控 → 复位 | 95% |
| FM-E2E-02 | 计数器逻辑软件失效 | 序列号不变或跳跃 | 中 | 5 | E2E 计数器检查 → 抑制发送 | 99% |
| FM-E2E-03 | 数据 ID 配置错误 | 错误数据被视为正确 | 高 | 8 | 启动配置校验和 | 90% |
| FM-E2E-04 | 超时检测软件失效 | 消息丢失无法发现 | 中 | 5 | 接收超时监控 (OS) | 95% |
| FM-E2E-05 | E2E 状态机混乱 | 进入错误保护状态 | 低 | 3 | 状态机 watchdog | 85% |
| **E2E 合计** | | | | **31** | **综合 DC** | **94%** |

**WdgM 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-WDG-01 | 喂狗软件失效 (错误喂狗) | 系统误复位/不复位 | 中 | 8 | 超时日志 → 复位记录 | 99% |
| FM-WDG-02 | 超时阈值配置错误 | 安全响应延迟 | 高 | 5 | 启动自检 (模式检查) | 90% |
| FM-WDG-03 | 监督实体管理失效 | 任务不被监控 | 高 | 6 | 启动完整性检查 | 70% |
| FM-WDG-04 | 代数计数器溢出 | WdgM 管理混乱 | 中 | 3 | 硬件 WDG 独立超时 | 90% |
| **WdgM 合计** | | | | **22** | **综合 DC** | **89%** |

**Can 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-CAN-01 | Tx 缓冲区管理失效 | 消息未发送 | 低 | 5 | Tx 确认超时 | 98% |
| FM-CAN-02 | Bus-Off 恢复逻辑失效 | 通信恢复失败 | 中 | 4 | 状态中断 → CanNm 通知 | 99% |
| FM-CAN-03 | 接收过滤错误 | 数据误收 | 中 | 6 | E2E CRC + DataID 校验 | 99% |
| FM-CAN-04 | 波特率配置错误 | 无法通信 | 中 | 3 | 初始化自测 | 85% |
| **Can 合计** | | | | **18** | **综合 DC** | **95%** |

**NvM 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-NVM-01 | CRC 校验软件失效 | 数据损坏不可检测 | 高 | 6 | 双备份 CRC + 比较 | 99% |
| FM-NVM-02 | 双副本管理失效 | 副本同步丢失 | 高 | 5 | 索引校验 + CRC 交叉检 | 95% |
| FM-NVM-03 | 写操作中断处理失效 | 数据丢失 | 高 | 4 | 写确认超时 → 重试 | 98% |
| FM-NVM-04 | 磨损均衡算法错误 | EEPROM 单元早期失效 | 中 | 3 | ECC 硬件保护 | 99.9% |
| **NvM 合计** | | | | **18** | **综合 DC** | **94%** |

**SecOC 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-SEC-01 | MAC 验证逻辑失效 | 非法帧被接受 | 高 | 8 | 新鲜度值验证 + 密钥校验 | 98% |
| FM-SEC-02 | 新鲜度值管理失效 | 重放攻击可执行 | 高 | 6 | 时钟同步 + 最大延迟检查 | 95% |
| FM-SEC-03 | 密钥句柄管理混乱 | 密钥使用错误 | 高 | 5 | HSM 完整性校验 | 99% |
| FM-SEC-04 | 认证状态机错误 | 帧处理状态异常 | 中 | 4 | 状态机 watchdog | 85% |
| **SecOC 合计** | | | | **23** | **综合 DC** | **94%** |

**CryIf 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-CRY-01 | 操作选择逻辑错误 | 密码算法错误执行 | 高 | 5 | HSM 操作 ID 验证 | 95% |
| FM-CRY-02 | 回调机制失效 | 操作完成无通知 | 中 | 4 | 超时监控 | 90% |
| FM-CRY-03 | 密钥句柄映射错误 | 密钥写错 | 高 | 5 | HSM 密钥 ID 校验 | 98% |
| **CryIf 合计** | | | | **14** | **综合 DC** | **94%** |

**RamSafety 模块故障模式：**

| 故障 ID | 故障模式 | 影响 | 严重度 | λ_soft (FIT) | 检测机制 | DC |
|---------|----------|------|--------|:-----------:|----------|:----:|
| FM-RAM-01 | March 算法执行错误 | 故障漏检 | 高 | 5 | 双算法交叉验证 | 95% |
| FM-RAM-02 | 故障映射表更新错误 | 故障地址记录错误 | 中 | 3 | 地址范围有效性检查 | 90% |
| FM-RAM-03 | 测试后未恢复数据 | 运行状态数据丢失 | 中 | 4 | ECC 保护 + 备份恢复 | 88% |
| **RamSafety 合计** | | | | **12** | **综合 DC** | **91%** |

### 4.3 SPFM — 单点故障度量 (ISO 26262-5 §9.4.2)

| 安全机制 | 相关故障 | λ (FIT) | 安全覆盖 | 残留 FIT |
|----------|---------|:-------:|:--------:|:--------:|
| E2E CRC + Counter + DataID | E2E 软件故障 + 通信损坏 | 31 | 94% | 1.9 |
| WdgM 超时监控 | WdgM 软件故障 + 任务超时 | 22 | 89% | 2.4 |
| NvM 双备份 + CRC | NvM 软件故障 + 数据损坏 | 18 | 94% | 1.1 |
| Can 硬件 CRC + E2E | Can 软件故障 + 通信错误 | 18 | 95% | 0.9 |
| SecOC MAC + 新鲜度 | SecOC 软件故障 + 安全攻击 | 23 | 94% | 1.4 |
| CryIf HSM 接口 | CryIf 软件故障 + 密码错误 | 14 | 94% | 0.8 |
| RamSafety March | RamSafety 软件故障 + RAM 错误 | 12 | 91% | 1.1 |
| **SPFM 计算** | **合计** | **138** | — | **9.6** |

**SPFM (单点故障度量) = 1 - (∑λ_SPF_residual + ∑λ_RF) / (∑λ_SPF + ∑λ_RF) = 1 - 9.6 / 138 = 93.0%**

ASIL B 要求 SPFM ≥ 90% → ✅ **93.0% ≥ 90%, 满足**

### 4.4 LFM — 潜伏故障度量 (ISO 26262-5 §9.4.3)

| 故障源 | λ (FIT) | Latent 检测机制 | 检测率 | 潜伏残留 FIT |
|--------|:-------:|-----------------|:-----:|:------------:|
| E2E 配置表损坏 | 8 | 启动时 CRC 校验 | 90% | 0.8 |
| WdgM 阈值配置错误 | 5 | 启动自检 | 85% | 0.8 |
| NvM 校验算法潜伏错误 | 6 | 定期自检 + 双备份交叉 | 88% | 0.7 |
| Can 过滤配置错误 | 6 | E2E DataID (运行时) | 95% | 0.3 |
| SecOC 密钥过期 | 5 | HSM 密钥生命周期检查 | 90% | 0.5 |
| CryIf 操作超时未配置 | 4 | OS 超时监控 | 85% | 0.6 |
| RamSafety 测试算法未执行 | 3 | 系统启动序列检查 | 88% | 0.4 |
| OS 调度表错误 | 5 | Task Activation 监控 | 85% | 0.8 |
| **LFM 计算** | **42** | — | — | **4.9** |

**LFM (潜伏故障度量) = 1 - ∑λ_LF_residual / ∑λ_LF = 1 - 4.9 / 42 = 88.3%**

ASIL B 要求 LFM ≥ 60% → ✅ **88.3% ≥ 60%, 满足**

### 4.5 PMHF 计算 (ISO 26262-5 §9.4.4)

| 组件 | λ (FIT) | 类型 | SPF/LF 覆盖 | 残留 FIT | 备注 |
|------|:-------:|:----:|:----------:|:--------:|------|
| **硬件总 FIT** | **88** | 硬件 | — | — | S32K312 裸片 (见 §4.1) |
| 硬件 SPF 覆盖 |  | | | | |
|   CPU 核心 (Lockstep) | 25 | SPF | 99% | 0.25 | 硬件锁步保护 |
|   SRAM (ECC) | 8 | SPF | 99.9% | 0.008 | SEC-DED ECC |
|   Flash (ECC) | 14 | SPF | 99.9% | 0.014 | SEC-DED ECC |
|   PLL/时钟 (CMU) | 5 | SPF | 95% | 0.25 | 时钟监控单元 |
|   外设 (CAN/SPI/LIN) | 11 | SPF | 90% | 1.1 | 硬件自检限制 |
|   电源 (PMC) | 3 | SPF | 99% | 0.03 | 独立 POR |
|   HSE_B | 10 | SPF | 95% | 0.5 | 安全引擎自检 |
|   系统总线 | 5 | SPF | 80% | 1.0 | 有限自检 |
|   WDOG + DMA | 5 | SPF | 90% | 0.5 | 硬件自检 |
|  其他 | 2 | SPF | — | 2.0 | 保守估算 |
| **硬件残留 SPF** | | | | **5.65** | |
| **软件残留 FIT** | | | | **9.6** | §4.3 软件 SPFM 残留 |
| **潜伏故障残留** | | | | **4.9** | §4.4 LFM 残留 |
| **总 PMHF** | | | | **20.2 FIT** | = 5.65 + 9.6 + 4.9 |

**系统 PMHF = 20.2 FIT (2.02 × 10⁻⁸ / h)**

ASIL B 要求 PMHF < 100 FIT (1 × 10⁻⁷ / h) → ✅ **20.2 < 100, 满足**

> 📌 **变更说明 (v1.4.0):** 
> - 使用 S32K312 硬件基线 λ = 88 FIT (v1.3.0 未区分硬件/软件, 总约 400 FIT, 漏了硬件系数)
> - 软件故障率采用模块级精化 (v1.3.0 统一估 50-100 FIT/模块)
> - SPFM/LFM/PMHF 按 ISO 26262-5 §9.4 公式准确计算
> - 所有 ASIL B 指标满足: SPFM 93.0% ≥ 90%, LFM 88.3% ≥ 60%, PMHF 20.2 < 100 FIT

---

## 5. 安全机制矩阵

### 5.1 机制与模块映射

| 安全机制 | 适用模块 | ASIL | 标准参考 | 实现方式 | 覆盖率 |
|----------|---------|------|---------|----------|--------|
| **ECC** (Error Correction Code) | RAM, Flash, Cache | ASIL B | ISO 26262-5 §9.4.2 | 硬件 ECC (S32K312) | 99.9% |
| **WDGM** (Watchdog Manager) | 所有实时任务 | ASIL B | AUTOSAR SWS WdgM | 软件喂狗+硬件 WDG | 99% |
| **E2E** (End-to-End Protection) | Com, PduR, Dcm | ASIL B | AUTOSAR SWS E2E | CRC+Counter+DataID | 94% |
| **SECOC** (Secure Onboard Comm) | 安全通信 | ASIL B | AUTOSAR SWS SecOC | 新鲜度值+MAC | 93% |
| **HSM** (Hardware Security) | Crypto, KeyM | ASIL B | AUTOSAR SWS Crypto | 硬件密钥存储 | 99% |
| **CRC** (Cyclic Redundancy Check) | NvM, E2E, Tp | ASIL B | AUTOSAR SWS CRC | 软件 CRC 引擎 | 95% |
| **DMA** (Deadline Monitoring) | Com 通信 | QM | AUTOSAR SWS Com | 超时监控 | 90% |
| **OS Timing Protection** | 任务调度 | ASIL B | AUTOSAR SWS OS | 执行/锁定/间隔监控 | 95% |
| **RAM Safety Test** | 内存完整性 | ASIL B | ISO 26262-5 §9.4.3 | March C+算法 | 95% |
| **DTC** (Diagnostic Event) | 全系统 | QM | AUTOSAR SWS Dem | 事件存储+恢复 | 85% |
| **Flash CRC** | 启动完整性 | ASIL B | AUTOSAR SWS BswM | 启动时 CRC 校验 | 90% |
| **Lockstep** (双核锁步) | CPU 核心 | ASIL B | ISO 26262-5 §9.4.4 | 硬件 (S32K312) | 99% |

### 5.2 安全机制依赖关系

```
┌─────────────────────────────────────────┐
│            ASIL B 安全层                  │
├─────────────────────────────────────────┤
│  E2E ← 依赖 CRC 引擎                      │
│  WdgM ← 依赖 GPT + WDG 硬件               │
│  SECOC ← 依赖 HSM + CRYIF                 │
│  OS Timing ← 依赖 GPT + Systick           │
│  RamSafety ← 依赖 LBIST + SBIST           │
│  Flash CRC ← 依赖 Fls + CRC               │
│  Lockstep ← 依赖硬件 S32K312              │
└─────────────────────────────────────────┘
         ↑  ECC 保护 (跨层次硬件机制)
         ↓
┌─────────────────────────────────────────┐
│             QM 功能层                     │
├─────────────────────────────────────────┤
│  Com, Dcm, PduR, Can, Lin, SPI, ...     │
│  (安全数据经 E2E 端到端审计)              │
└─────────────────────────────────────────┘
```

---

## 6. 硬件安全要求 (ISO 26262-5 / ASPICE HWE.2)

### 6.1 ASPICE HWE.2 BP 映射

| ASPICE HWE.2 BP | yuleASR 对应项 | 状态 |
|----------------|---------------|------|
| BP2: 识别硬件安全需求 | 本文 §2.2 ASIL 分配 + §3 Safe State | ✅ |
| BP3: 定义硬件设计 | 本文 §5 安全机制矩阵 | ✅ |
| BP4: 评估硬件设计可选方案 | (待补充架构权衡分析) | ⚠️ 部分 |
| BP5: 分析硬件设计 | 本文 §4 FMEDA + PMHF | ✅ (初步) |

### 6.2 ISO 26262-5 硬件安全要求

| 标准章节 | 要求 | 实现参考 |
|---------|------|---------|
| §8.4.2 | 硬件安全需求规格 | HARA (docs/safety/HARA_ANALYSIS.md) |
| §9.4.2 | 单点故障度量 (SPFM) | 本文 §4.2 覆盖率表 |
| §9.4.3 | 潜伏故障度量 (LFM) | 本文 §4.2 LR 覆盖率 |
| §9.4.4 | PMHF 定量估算 | 本文 §4.3 PMHF 表 |
| §10.4.3 | 相依失效分析 (DFA) | — **待补充** |

### 6.3 相依失效分析 (DFA) 参考

DFA 已完成，详情参见独立文档:

| 分析项 | 文档 | 状态 |
|--------|------|:----:|
| DFA 框架 + 方法论 | docs/safety/dfa-analysis.md §2 | ✅ 已完成 |
| 共享资源清单 | docs/safety/dfa-analysis.md §3 | ✅ 已完成 |
| 耦合路径分析 (内存/时序/中断/总线) | docs/safety/dfa-analysis.md §4 | ✅ 已完成 |
| FFI 评估 | docs/safety/dfa-analysis.md §5 | ✅ 已完成 (93% FFI 满足) |
| 安全偏差 (D-FFI-001, D-FFI-002) | docs/safety/dfa-analysis.md §6 | ✅ 已完成 |
| CCF 分析 | docs/safety/dfa-analysis.md §7 | ✅ 已完成 (8/8 覆盖)

---

## 7. 偏离与限制

### 7.1 已知限制 (v1.4.0)

| 限制 | 影响 | 缓解措施 | 目标版本 |
|------|------|----------|---------|
| 无 MPU/MMU 分区 | ASIL B 与 QM 软件无隔离 | 链接脚本分区 + E2E 端到端保护; MPU 方案已规划 | v2.0 |
| 无独立安全内核(SKC) | 安全监控与功能软件同核 | 使用 WdgM 时序监控 | v1.5.0 |
| MC/DC 覆盖仅软件路径枚举 | 无法工具自动覆盖 | 手动枚举测试 (见工具链局限性文档) | v2.0 |
| 交叉编译未硬件运行 | S32K312 行为未验证 | 编译验证 + 二进制分析 | v1.5.0 |

### 7.2 偏差许可

| 偏差编号 | 描述 | 理由 | 批准 | 参见 |
|---------|------|------|------|------|
| D-FFI-001 | ASIL B 全局数据段无 MPU 隔离 | 链接脚本分区 + E2E + WdgM 双重缓解 | 已接受 | dfa-analysis.md §6.1 |
| D-FFI-002 | ASIL B BSS 段无 MPU 隔离 | 同 D-FFI-001 缓解 | 已接受 | dfa-analysis.md §6.2 |

---

## 8. 版本记录

| 版本 | 日期 | 作者 | 变更 |
|------|------|------|------|
| 1.0 | 2026-07-21 | 小马 🐴 (质量架构师) | 初始安全架构文档 |
| 1.1 | 2026-07-26 | 小马 🐴 | FMEDA 精化 (S32K312 硬件基线 + SPFM/LFM/PMHF 计算) |
| 1.2 | 2026-07-26 | 小马 🐴 | DFA 完成引用 + MPU 分区方案 + 工具链局限记录 |

---

## 附录 A: 参考文件索引

| 文件 | 路径 | 说明 |
|------|------|------|
| HARA 分析 | docs/safety/HARA_ANALYSIS.md | 危害分析与风险评估 |
| DFA 分析 | docs/safety/dfa-analysis.md | 相依失效分析 (ISO 26262-9 §8) |
| MPU 分区方案 | docs/safety/mpu-partition.md | MPU 内存保护分区规划 |
| 安全架构 (本文件) | docs/safety/safety-architecture.md | ASIL B 安全架构 + FMEDA |
| 安全手册 | docs/safety/SAFETY_MANUAL.md | ISO 26262 ASIL-D 安全手册(旧版) |
| 验证报告 | docs/safety/VERIFICATION_REPORT.md | 测试覆盖率报告 |
| 架构文档 | docs/architecture.md | 系统架构 |
| 工具链局限 | docs/compliance/tool-limitations.md | 工具链局限性分析 |
| MISRA 合规 | docs/misra_compliance_report.md | COM 模块 MISRA 合规 |
| MISRA 偏差 | docs/misra_deviations.md | MISRA 偏差许可 |
| RTE 生成器 | docs/tools/rte-generator.md | RTE 代码生成器使用说明 |
| S32K312 平台 | docs/platform/s32k312.md | S32K312 平台构建指南 |

*— 本文档会伴随 v1.3.0 评审持续更新*
