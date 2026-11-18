# 设计文档：P0-P2 量产整改

> **变更 ID**: fix-p0-p2-production-remediation
> **关联提案**: `openspec/changes/fix-p0-p2-production-remediation/proposal.md`
> **版本**: v1.1
> **日期**: 2026-10-08
> **最近更新**: 2026-11-19（补充 P0/P1 修复与全工程回归结论）

---

## 1. P0-1: 4 对重复模块收敛（Tombstone Shim 模式）

### 1.1 设计原则

沿用 Phase 2 已验证的 **tombstone shim** 模式：
- **保留方**（canonical）：真实实现，保留全部源码和 CMake target
- **Shim 方**（duplicate）：`.c` 文件替换为墓碑注释，CMake target 保留（喂入生成的空 stub），include 路径不变

### 1.2 收敛方向

| 重复对 | 保留方（canonical） | Shim 方（duplicate） | 理由 |
|:--------|:---------------------|:----------------------|:-----|
| CAN NM | `ecual/canNm` (~1409 LOC, CanIf/ComM 集成, 2-ch Lcfg) | `services/canm` (~507 LOC, 无外部集成) | ecual 版本有完整 AUTOSAR 集成 |
| LIN SM | `ecual/linSM` (ComM/EcuM 集成, wakeup source config) | `services/linsm` (自包含, 非标准类型) | ecual 版本符合 AUTOSAR 标准类型 |
| Fee | `ecual/fee` (block-based API, MemIf 契约匹配) | `mcal/fee` (address-based, 错误抽象) | ecual 版本匹配 MemIf_Cfg.h 宏签名 |
| RAM Test | `mcal/ramtst` (v2.0.0, 6 算法, error record, Lcfg) | `services/ramtst` (v1.0.0, 4 算法, 无 error record) | mcal 版本功能完整度高 |

### 1.3 实施细节

#### 1.3.1 services/CMakeLists.txt 修改

```cmake
# 在 SERVICES_SHIMMED_MODULES 列表中追加 3 个模块
set(SERVICES_SHIMMED_MODULES
    doip
    ipdum
    ethsm
    canm      # -> ecual/canNm (Phase 3 convergence)
    linsm     # -> ecual/linSM (Phase 3 convergence)
    ramtst    # -> mcal/ramtst (Phase 3 convergence)
)
```

#### 1.3.2 mcal/CMakeLists.txt 修改

新增 `MCAL_SHIMMED_MODULES` 列表（仿照 services 模式）：

```cmake
set(MCAL_SHIMMED_MODULES
    fee       # -> ecual/fee (Phase 3 convergence)
)

# 在 foreach 循环中添加 shim 检测
list(FIND MCAL_SHIMMED_MODULES "${module_name}" _shimmed_idx)
if(NOT _shimmed_idx EQUAL -1)
    set(module_sources "${CMAKE_BINARY_DIR}/generated/shim-stubs/mcal_${module_name}_shim.c")
    file(WRITE "${module_sources}"
        "/* Phase 3 duplicate-module convergence: forwarding shim for mcal_${module_name}.\n"
        " * Canonical implementation lives in the ECUAL layer.\n"
        " */\n")
endif()
```

#### 1.3.3 测试 include 路径更新

`tests/bsw/services/canm/CMakeLists.txt` 更新 include 路径：
```cmake
# 从 services/canm/include 改为 ecual/canNm/include
target_include_directories(canm_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canNm/include
)
# 链接目标从 service_canm 改为 ecual_canNm
target_link_libraries(canm_test PRIVATE ecual_canNm)
```

同理更新 `tests/bsw/services/linsm/` 和 `tests/bsw/services/ramtst/`。

#### 1.3.4 全局 include 路径清理

`services/CMakeLists.txt` 第 18 行 `mcal/fee/include` 需保留（MemIf 依赖 Fee.h），但确保 ecual/fee 的 include 路径优先级高于 mcal/fee。

---

## 2. P0-2: SecOC Freshness NvM 持久化

### 2.1 问题分析

SecOC 内部维护两个 freshness counter：
- `SecOC_TxPduState[i].freshnessValue` — 每个 TX PDU 的发送计数器
- `SecOC_SyncMasterFreshness` — Master-Slave 同步计数器

重启后这些值归零，导致：
1. Slave 端检测到 freshness 回退 → 拒绝合法消息（replay attack 误判）
2. Master-Slave 同步状态丢失 → 需要重新执行同步流程

### 2.2 设计方案

#### 2.2.1 NvM Block 定义

在 `SecOC_Cfg.h` 中定义：
```c
#define SECOC_FRESHNESS_NVM_BLOCK_ID       ((NvM_BlockIdType)0x50U)
#define SECOC_FRESHNESS_NVM_BLOCK_SIZE     (4U + SECOC_NUM_TX_PDUS * 4U)
/* 4 bytes sync master + 4 bytes per TX PDU */
```

#### 2.2.2 持久化数据结构

```c
typedef struct {
    uint32 syncMasterFreshness;
    uint32 txFreshness[SECOC_NUM_TX_PDUS];
    uint32 crc32;  /* CRC-32 IEEE 802.3 for integrity */
} SecOC_PersistedDataType;
```

#### 2.2.3 Init 时恢复

```c
void SecOC_Init(const SecOC_ConfigType* ConfigPtr) {
    /* ... existing init code ... */
    
    /* Restore freshness from NvM */
    SecOC_PersistedDataType persisted;
    Std_ReturnType ret = NvM_ReadBlock(SECOC_FRESHNESS_NVM_BLOCK_ID, &persisted);
    if (ret == E_OK) {
        if (SecOC_VerifyCRC32(&persisted)) {
            SecOC_SyncMasterFreshness = persisted.syncMasterFreshness;
            for (uint16 i = 0; i < SECOC_NUM_TX_PDUS; i++) {
                SecOC_TxPduState[i].freshnessValue = persisted.txFreshness[i];
            }
        }
    }
}
```

#### 2.2.4 Verify 后写入

```c
/* 在 SecOC_Verify 成功后触发写入 */
static void SecOC_PersistFreshness(void) {
    SecOC_PersistedDataType persisted;
    persisted.syncMasterFreshness = SecOC_SyncMasterFreshness;
    for (uint16 i = 0; i < SECOC_NUM_TX_PDUS; i++) {
        persisted.txFreshness[i] = SecOC_TxPduState[i].freshnessValue;
    }
    persisted.crc32 = SecOC_CalculateCRC32(&persisted);
    (void)NvM_WriteBlock(SECOC_FRESHNESS_NVM_BLOCK_ID, &persisted);
}
```

#### 2.2.5 写入频率控制

避免每次 Verify 都写 NvM（flash 寿命），使用计数器节流：
```c
#define SECOC_PERSIST_INTERVAL  100U  /* 每 100 次 Verify 持久化一次 */
static uint16 SecOC_PersistCounter = 0;

/* 在 SecOC_IncrementFreshness 中 */
SecOC_PersistCounter++;
if (SecOC_PersistCounter >= SECOC_PERSIST_INTERVAL) {
    SecOC_PersistFreshness();
    SecOC_PersistCounter = 0;
}
```

### 2.3 依赖关系

- SecOC → NvM（通过 NvM_ReadBlock / NvM_WriteBlock API）
- 无循环依赖（NvM 不调用 SecOC）
- NvM block 需在 NvM_Cfg.h 中注册

### 2.4 测试策略

1. **Init 恢复测试**：预设 NvM block 数据 → 调用 SecOC_Init → 验证 freshness counter 恢复
2. **Verify 持久化测试**：调用 SecOC_Verify 100 次 → 验证 NvM_WriteBlock 被调用
3. **CRC 完整性测试**：篡改 NvM block 数据 → 验证 Init 拒绝恢复
4. **重启模拟测试**：Init → 多次 Verify → 模拟重启（重新 Init）→ 验证 freshness 连续

---

## 3. P0-3: Safety Case 模块评审证据

### 3.1 评审记录模板

创建 `.osh/evidence/reviews/TEMPLATE.md`：

```markdown
# Module Review Record: [MODULE_NAME]

| Field | Value |
|:------|:------|
| Module | [MODULE_NAME] |
| ASIL | B |
| Review Date | [DATE] |
| Reviewer | [NAME] |
| Review Scope | Code review, coverage analysis, MISRA compliance |

## 1. Code Review Summary

- **Lines of Code**: [LOC]
- **Functions**: [COUNT]
- **Complexity (avg)**: [CC]
- **Review Findings**: [SUMMARY]

## 2. Coverage Data

| Metric | Value | Target | Status |
|:-------|:------|:-------|:-------|
| Statement Coverage | [X]% | 80% | PASS/FAIL |
| Branch Coverage | [Y]% | 70% | PASS/FAIL |
| MC/DC Coverage | [Z]% | 60% | PASS/FAIL |

## 3. MISRA Compliance

| Category | Count |
|:---------|:------|
| Mandatory violations | 0 |
| Required violations (with deviation permit) | [N] |
| Advisory violations | [M] |

Deviation permits: [LIST_OF_DEVIATION_IDS]

## 4. Open Issues

| ID | Description | Severity | Status |
|:---|:------------|:---------|:-------|
| [ISSUE_ID] | [DESCRIPTION] | [SEVERITY] | [STATUS] |

## 5. Review Conclusion

[CONCLUSION: APPROVED / CONDITIONALLY_APPROVED / REJECTED]

**Reviewer Signature**: ________________
**Date**: ________________
```

### 3.2 13 个 ASIL-B 模块评审记录

为以下模块生成评审记录（基于现有覆盖率数据和 MISRA 扫描结果）：

1. **Can** — MCAL CAN 驱动
2. **CanIf** — ECUAL CAN 接口
3. **CanSM** — Services CAN 状态管理
4. **CanTp** — ECUAL CAN 传输协议
5. **Com** — Services 通信模块
6. **PduR** — Services PDU 路由
7. **NvM** — Services 非易失存储管理
8. **Dcm** — Services 诊断通信管理
9. **Dem** — Services 诊断事件管理
10. **E2E** — Services 端到端保护
11. **WdgM** — Services 看门狗管理
12. **SecOC** — Services 安全车载通信
13. **BswM** — Services BSW 模式管理

### 3.3 数据来源

- 覆盖率数据：`.yuleosh/reports/branch-coverage-report.md`
- MISRA 偏差清单：`.yuleosh/ci-config.yaml` deviation permits
- 代码审查结果：基于现有代码质量（无历史审查记录，标注为 "Initial review based on static analysis"）

---

## 4. P1-1: MISRA Error-Severity 违规修复

### 4.1 违规定位

运行 `python tools/analysis/static_analysis.py` 获取违规列表，重点关注 error-severity 的 51 项。

### 4.2 修复策略

对每个违规：
1. 确认是否为真阳性（排除误报）
2. 真阳性：修复代码（添加类型转换、移除不可达代码、修正指针运算等）
3. 误报：更新 deviation permit 并添加注释说明

### 4.3 常见违规类型

基于历史报告（`docs/reports/MISRA_Issues_Summary.txt`）：
- Rule 11.4 (指针类型转换) — 添加显式 cast
- Rule 13.5 (短路求值副作用) — 重构为多语句
- Rule 8.1 (函数声明/定义不一致) — 统一签名
- Rule 17.7 (返回值未使用) — 添加 (void) cast

---

## 5. P1-2: TEST_IGNORE Stub 实现

### 5.1 分批策略

| 批次 | 层级 | 数量 | 优先级 |
|:-----|:-----|:-----|:-------|
| Batch 1 | ECUAL | ~30 | 高（CAN/LIN 栈核心路径） |
| Batch 2 | Services | ~50 | 中（Com/NvM/Dcm/Dem 核心 API） |
| Batch 3 | MCAL | ~50 | 低（驱动层，硬件依赖多） |

### 5.2 实现模式

每个 TEST_IGNORE 替换为真实测试逻辑：
```c
/* Before */
void test_CanIf_Transmit_Stub(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

/* After */
void test_CanIf_Transmit_ValidPdu_ShouldReturnOk(void) {
    /* Setup */
    CanIf_Init(&config);
    
    /* Exercise */
    Std_ReturnType ret = CanIf_Transmit(0x10, &pduInfo);
    
    /* Verify */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE(mock_Can_Write_called);
}
```

### 5.3 Mock 策略

- 使用 Unity 框架的内置 mock 机制
- MCAL 层测试使用 QEMU 模拟硬件
- Services 层测试使用 stub 函数

---

## 6. P1-3: 覆盖率门禁提升

### 6.1 分阶段提升

| 阶段 | 行覆盖 | 分支覆盖 | 时间 |
|:-----|:-------|:---------|:-----|
| 当前 | 40% | 25% | — |
| Phase 1 | 60% | 50% | +2 周 |
| Phase 2 | 80% | 70% | +4 周 |

### 6.2 配置文件修改

`.yuleosh/ci-config.yaml`:
```yaml
coverage:
  line_coverage_threshold: 60    # was 40
  branch_coverage_threshold: 50  # was 25
```

---

## 7. P1-4: CI Job 恢复

### 7.1 缺失测试源文件

| CI Job | 缺失文件 | 测试内容 |
|:-------|:---------|:---------|
| e2e-dds-tests | test_e2e_dds.c, test_e2e_secoc.c, test_e2e_e2e.c | E2E 安全验证 |
| uds-tests | test_e2e_uds.c | UDS 诊断协议 |
| ota-tests | test_e2e_ota.c | OTA 更新流程 |
| performance-tests | test_latency.c, test_throughput.c, test_memory.c | 性能基准 |
| security-tests | test_security.c | 安全测试 |
| multi-ecu-tests | tests/multi_ecu/ 目录 | 多 ECU 集成 |

### 7.2 测试骨架

每个测试文件遵循 Unity 框架标准结构：
```c
#include "unity.h"

void setUp(void) { /* init */ }
void tearDown(void) { /* cleanup */ }

void test_case_1(void) {
    /* real test logic */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_case_1);
    return UNITY_END();
}
```

---

## 8. P1-5: 量产 MCU MCAL 桩文件集

### 8.1 目录结构

```
src/bsw/mcal/production/
├── Mcu.c
├── Port.c
├── Dio.c
├── Can.c
├── Spi.c
├── Gpt.c
├── Pwm.c
├── Adc.c
├── Wdg.c
├── Icu.c
└── Lin.c
```

### 8.2 桩文件内容

每个文件包含：
```c
/**
 * @file Mcu.c
 * @brief Production MCU MCAL - S32K312
 * @note This is a STUB file. Replace with vendor-provided MCAL IP for production.
 */

#error "Production MCAL — replace with vendor IP (NXP S32K312)"

/* Stub implementations for SIL testing only */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr) {
    /* stub */
}
/* ... */
```

---

## 9. P1-6: 配置生成覆盖扩展

### 9.1 模块扫描

```bash
find src/bsw -name "*.c" -path "*/src/*" | \
    sed 's|.*/\([^/]*\)/src/.*|\1|' | \
    sort -u
```

### 9.2 bsw_config.json 扩展

为每个缺失模块添加配置节点：
```json
{
  "modules": {
    "canm": {
      "version": "4.4.0",
      "enabled": true,
      "config": { ... }
    },
    ...
  }
}
```

### 9.3 Jinja2 模板

为每个模块创建 `config/templates/<Module>_Cfg.h.j2`。

---

## 10. P1-8: Doxygen 注释骨架

### 10.1 最小注释集

每个公共 API header 至少包含：
```c
/**
 * @file ModuleName.h
 * @brief Module description
 */

/**
 * @brief Function description
 * @param[in] Param1 Description
 * @param[out] Param2 Description
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType ModuleName_Function(ParamType1 Param1, ParamType2* Param2);
```

### 10.2 自动化脚本

创建 `tools/docs/add_doxygen_skeleton.py`：
- 扫描所有 `.h` 文件
- 为缺少 `@brief` 的函数声明添加注释骨架
- 保留现有注释不覆盖

---

## 11. P1-9: MC/DC 覆盖率测量

### 11.1 工具链

使用 gcov 的条件/决策覆盖数据：
```bash
gcc -ftest-coverage -fprofile-arcs -fcondition-coverage
```

### 11.2 测量脚本

`coverage_run/mcdc_measure.sh`:
```bash
#!/bin/bash
# Measure MC/DC coverage for ASIL-B modules

MODULES="Can CanIf CanSM CanTp Com PduR NvM Dcm Dem E2E WdgM SecOC BswM"

for mod in $MODULES; do
    echo "=== $mod ==="
    gcov -c -d build-native/CMakeFiles/$mod.dir/src/ | \
        grep -E "^(File|Conditions|Decisions)"
done
```

---

## 12. P1-11: ISO 21434 Clauses 7/8/11 补充

### 12.1 Clause 7: Distributed Activities

补充章节：
- 7.1 供应链安全管理
- 7.2 第三方组件安全评估
- 7.3 分布式开发环境安全要求

### 12.2 Clause 8: Continuous Monitoring

补充章节：
- 8.1 安全事件监控
- 8.2 安全指标收集
- 8.3 异常检测与告警

### 12.3 Clause 11: Vulnerability Management

补充章节：
- 11.1 漏洞识别与评估（CVSS 评分）
- 11.2 漏洞响应流程
- 11.3 安全更新发布机制

---

## 13. P1-12: DoIP 全链集成测试

### 13.1 测试链路

```
DoIP → SoAd → EthIf → Eth (loopback)
```

### 13.2 测试用例

1. **DoIP 连接建立**：发送 DoIP header → 验证 SoAd 接收 → 验证 EthIf 派发
2. **DoIP 诊断请求**：发送 UDS 请求 → 验证 DoIP 封装 → 验证 SoAd 传输 → 验证 EthIf 接收
3. **DoIP 断开**：发送 DoIP close → 验证连接释放

---

## 14. P1-13: CAN FD 支持完善

### 14.1 Can 驱动 FD 配置

```c
/* Can_Cfg.h */
#define CAN_FD_SUPPORT  STD_ON

typedef struct {
    boolean fdEnabled;
    uint32  nominalBaudRate;
    uint32  dataBaudRate;
    boolean brsEnabled;  /* Bit Rate Switch */
} Can_ControllerConfigType;
```

### 14.2 CanIf BRS 处理

```c
/* CanIf.c */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    /* ... */
    if (CanIf_Config->fdEnabled && CanIf_Config->brsEnabled) {
        can_info.brs = CANFD_BRS_ON;
    }
    /* ... */
}
```

---

## 15. P2 各项设计要点

### 15.1 P2-1/P2-2: 返回类型统一

将 `uint8 Com_*` 和 `Can_ReturnType Can_*` 统一为 `Std_ReturnType`：
- 修改 header 文件中的函数声明
- 修改 .c 文件中的函数定义
- 更新所有调用点的类型匹配

### 15.2 P2-4: Release 脚本

```bash
#!/bin/bash
# tools/release/create_release.sh

VERSION=$1
git tag -a "v$VERSION" -m "Release v$VERSION"
git archive --format=tar.gz --prefix=yuleasr-$VERSION/ HEAD > yuleasr-$VERSION.tar.gz
sha256sum yuleasr-$VERSION.tar.gz > SHA256SUMS
gpg --detach-sign --armor SHA256SUMS
```

### 15.3 P2-5: Doxygen 配置

创建 `Doxyfile`：
```
PROJECT_NAME = "yuleASR AUTOSAR BSW"
INPUT = src/bsw
RECURSIVE = YES
FILE_PATTERNS = *.h
OUTPUT_DIRECTORY = docs/api
GENERATE_HTML = YES
```

### 15.4 P2-6: ROM/RAM 追踪

```bash
#!/bin/bash
# tools/analysis/track_memory.sh

arm-none-eabi-size build-native/*.elf | \
    awk 'NR>1 {text+=$1; data+=$2; bss+=$3} END {
        printf "ROM: %d KB (.text + .data)\n", (text+data)/1024
        printf "RAM: %d KB (.data + .bss)\n", (data+bss)/1024
    }'
```

### 15.5 P2-7: 性能基线

`tools/performance/baseline.json`:
```json
{
  "task_switch_ns": 940,
  "canif_rx_dispatch_ns": 1.1,
  "com_pack_signal_ns": 29,
  "crc32_throughput_mbps": 850
}
```

CI job 对比当前值与基线，偏差 >10% 时告警。

---

## 16. 实施依赖关系

```
P0-1 (模块收敛) ──┐
P0-2 (SecOC NvM) ──┼── P1-1 (MISRA) ── P2-1/P2-2 (返回类型)
P0-3 (评审证据) ──┘
                    │
P1-2 (TEST_IGNORE) ─┤
P1-3 (覆盖率) ──────┤
P1-4 (CI job) ──────┘
                    
P1-5 (MCAL 桩) ──── 独立
P1-6 (配置扩展) ──── 独立
P1-8 (Doxygen) ───── 独立
P1-9 (MC/DC) ─────── 依赖 P1-3
P1-10 (追溯) ─────── 独立
P1-11 (ISO 21434) ── 独立
P1-12 (DoIP 测试) ── 独立
P1-13 (CAN FD) ───── 独立
```

---

## 17. 验证策略

每个 Phase 完成后执行：
1. `cmake --build build-native` — 全量编译通过
2. `ctest --test-dir build-native --output-on-failure` — 全量测试通过
3. `python tools/analysis/static_analysis.py` — MISRA 违规数不增
4. `bash config/tools/check_config_drift.sh` — 配置无漂移
5. `python tools/traceability/trace_requirements.py` — 追溯一致性

---

## 18. P0/P1 修复与全工程回归结论（2026-11-19 补录）

### 18.1 P0 阻断性缺陷：已闭环

2026-11-08 整改批次的 5 个 cppcheck P0 缺陷均已修复：

| 模块 | 修复结论 |
|:-----|:---------|
| I2c | 移除未使用的 `bestDivider` 及空操作语句 |
| Boot_Flash | `retry` 在声明处初始化为 `BOOT_E_FLASH_ERASE`，消除未初始化使用 |
| Fls_Hw | 空指针、地址范围及对齐检查移到 DET 配置开关之外，安全校验始终生效 |
| Dcm Transfer | 删除与外层条件重复的冗余长度判断 |
| DDS Log | 修正 `uint32_t` 日志格式化类型及显式转换 |

**静态复核结论**：对整改文件按同一 cppcheck 口径复扫，过滤已确认误报后为 **0 error / 0 warning**。

### 18.2 P1 重要缺陷：已闭环

- **MISRA 17.2（递归）**：BswM 改为迭代后序遍历；NvM 冗余副本改为失败重试；Mqtt_CertMgr 消除互递归；dds_access 通配符匹配改为迭代回溯；content_filtered_topic 使用显式栈并限制解析深度。全量 MISRA 复扫 **17.2 = 0**，未依赖新增抑制。
- **MISRA 20.9（stdio）**：日志统一收口到 `Debug.h/Debug.c` 抽象层；生产代码原始 `printf/fprintf/snprintf` 调用为 **0**。保留 `<stdio.h>` 的 6 个文件均有明确依据：`Debug.c` 封装，以及 5 个执行真实文件 I/O 的 DDS 文件。

### 18.3 全工程回归结论

| 验证基线 | 构建与测试 | 结果 |
|:---------|:-----------|:-----|
| 2026-11-08 P0/P1 整改验收 | Native / ASan / UBSan / Coverage 四种构建分别执行全量 CTest | **各 193/193 通过**；ASan、UBSan 均无 sanitizer 报错 |
| 2026-11-19 当前复验（`83b75427`） | Debug 全量构建后串行执行 `ctest -j1` | **216/216 通过，0 失败** |

当前复验期间，DDS 签名长度局部变量曾一度误用 `uint16_t`，与 API 的 `uint32_t*` 输出参数不匹配，导致 `dds_log`、`dds_security`、`test_ddssec_auth` 3 项失败；改为 `uint32_t` 后定向复测及全量回归均通过。该过程确认回归套件能够捕获此次修复引入的类型宽度回归，最终结果不是沿用历史绿状态的推断。

**结论边界**：上述结果证明本批 P0/P1 静态分析与运行时缺陷已闭环，且当前主机侧全工程测试无回归；不将其等同于真实硬件 HIL/SIL、工具鉴定或全部量产成熟度条目的验收。详细证据见 `reports/quality/production-readiness-2026-11-08.md` 第 5.5 节及本轮验证记录。
