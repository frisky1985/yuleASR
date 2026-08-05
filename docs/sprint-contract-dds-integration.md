# Sprint Contract: ETH-DDS 9 模块完善并集成主线

> 状态: DRAFT v0.1 — 待 Generator/Evaluator 评审
> 日期: 2026-08-04
> 分支: feat/dds-integration (基线 1db70f9)

## Scope

**What**: 把归档分支 `archive/eth-dds-experiment` 的 9 个功能模块完善到可编译、可测试、可集成进主线 AUTOSAR 结构，最终合并回 `feat/1.3.0`。

**9 个模块**:
| # | 模块 | 来源 | 现状评级 | 主要缺口 |
|---|------|------|---------|---------|
| M1 | DDS RTPS 中间件 | src/dds/ | 22 个 .c 完整实现，缺核心层 | 缺 core/ 3 文件+4 头、rtps_wire、pubsub 5 文件+6 头；src/common/ 缺失导致编译全断 |
| M2 | TSN/gPTP 传输 | src/dds/transport/tsn/ | 自包含完整 | 仅依赖 tsn_stack.h 类型（不存在，需移植或对接 EthTSyn） |
| M3 | Bootloader | src/bootloader/ | partition/rollback 基本完整，secure_boot 半成品 | 缺 crypto_stack/csm+keym、dds_log；rollback 持久化 TODO；secure_boot 密码学需大修 |
| M4 | Telemetry | src/telemetry/ + include/telemetry/ | 基本完整 | telemetry_tools.h 未定义 TelEntry_t；diag 缺 Std_Types include |
| M5 | ara_com (Adaptive) | src/autosar/adaptive/ | 半成品 | 缺 dds_core.h/dds_types.h、ara/ 8 头、平台 stub 填充；2 个 .cpp 可改 C |
| M6 | eth_sm | src/eth_sm/ | 完整可用 | 无缺失依赖（最干净） |
| M7 | 以太网驱动 | src/ethernet/ | 完整可用（代码） | 缺 eth_types.h、dds_log、平台层（MDIO/时间） |
| M8 | DDSCodeGen | cmake/DDSCodeGen.cmake | 完整（工具） | 依赖 tools/codegen/dds_config_cli.py + dds-config-tool/ |
| M9 | eth_dds 统一 API | include/eth_dds.h + src/autosar/classic/rte_dds.c | rte_dds 半成品 | 缺 dds_core.h、autosar_types/errors、桥接主线 Rte |

**In Scope**:
- 补齐 DDS 核心层（core/domain/entity/qos、rtps_wire、pubsub 基础 5 文件）
- 移植 src/common/（eth_types.h、dds_log.h 等）
- 传输层适配主线 AUTOSAR SoAd/TcpIp/Eth API（替代自定义 soad.h/tcpip API）
- 构建集成（顶层 CMake add_subdirectory + 修复悬空库目标）
- 修复移植带入的链接缺陷（重复符号、security 12 个未定义函数、TCP 空路径）
- 测试接入（CTest + 主线自带 Unity）

**Out of Scope**（本 sprint）:
- Adaptive 平台函数真实实现（process/sandbox 保持 stub，仅保证编译）
- TSN 硬件级 gPTP 真实实现（对接 EthTSyn 或保持接口）
- 生产级 bootloader 掉电持久化（保留 TODO 标记）
- 与主线 Rte 的深度桥接（rte_dds 先保证编译通过）

## 架构决策 (architect-lead)

- **DDS 挂载**: 新增 `src/dds/CMakeLists.txt` + 顶层 `add_subdirectory(src/dds)`（仿 src/rte/micro-dds），不建 src/ 聚合层
- **传输层**: dds_eth_transport 改用 AUTOSAR `service_tcpip`/`mcal_eth` API；dds_soad_adapter 改用 `service_soad` API（SoAd_OpenUdpConnection/IfTransmit/Receive）
- **TSN**: 对接主线 `src/bsw/services/ethtsyn`（EthTSyn），或移植 tsn_stack.h 类型
- **类型基础**: 移植 src/common/eth_types.h + dds_log.h；AUTOSAR 交互走 Std_Types/Std_ReturnType
- **C++ 文件**: exec_manager.cpp/state_manager.cpp 保留 C++（主线已启用 CXX），不做 C 改写
- **构建选项**: 顶层定义 ENABLE_DDS/ENABLE_DDS_SECURITY/ENABLE_DDS_RUNTIME

## Testable Behaviors

### B1: 编译通过
- [ ] B1.1: `cmake -B build -S . -DBUILD_TESTING=ON` 成功 (native)
- [ ] B1.2: `cmake --build build` 全绿，零 error
- [ ] B1.3: ARM 交叉编译 (S32K312) 不引入新 error

### B2: DDS 核心功能
- [ ] B2.1: DDS 单元测试通过（CTest 收集 dds_* 测试）
- [ ] B2.2: pubsub 基础文件（publisher/subscriber/topic/writer/reader）测试覆盖 create/write/take/delete 全链路
- [ ] B2.3: rtps_wire 组帧/解帧往返一致（build/parse roundtrip）

### B3: 模块独立验证
- [ ] B3.1: eth_sm 状态机测试（UNINIT→INIT→READY 转移）
- [ ] B3.2: ethernet 驱动编译 + 基本寄存器模拟测试
- [ ] B3.3: telemetry 环形缓冲 + 事件日志测试
- [ ] B3.4: bootloader partition CRC/回滚逻辑测试

### B4: 集成
- [ ] B4.1: 传输层适配后 dds_eth_transport 通过编译（链接 service_tcpip/mcal_eth）
- [ ] B4.2: 顶层 ctest 全量运行无回归（既有测试仍全绿）
- [ ] B4.3: QEMU M33 验证不受影响（可选，若构建链无冲突）

## Acceptance Criteria

| ID | Criterion | Pass Condition | Fail Condition | Priority | Owner |
|----|-----------|----------------|----------------|----------|-------|
| A1 | DDS 可编译 | cmake+make 成功，无缺失文件 | 任一 CMake 引用文件缺失 | P0 | Generator |
| A2 | DDS 可测试 | ctest 有 dds_* 测试且全过 | 无测试或测试失败 | P0 | Generator |
| A3 | 9 模块无编译阻断 | 全部模块过预处理+编译 | 任一模块编译失败 | P0 | Generator |
| A4 | 无链接重复符号 | 全量链接成功 | duplicate symbol | P0 | Generator |
| A5 | 既有测试无回归 | ctest 全绿 | 既有测试失败 | P0 | Evaluator |
| A6 | 传输层对接主线 API | 无自定义 tcpip/soad 残留调用 | 仍引用 src/soad/、src/tcpip/ | P1 | Generator |
| A7 | 合并回主线 | feat/dds-integration → feat/1.3.0 无冲突 | 冲突无法解决 | P1 | Generator |

## Responsibility Matrix

| Criterion | Responsible | Fallback |
|-----------|-------------|----------|
| A1-A4, A6, A7 | Generator | architect-lead |
| A5 | Evaluator | Generator |
| B 系列 | Evaluator | Generator |

## Negotiation Log

| Round | Party | Action | Notes |
|-------|-------|--------|-------|
| 1 | Planner | 起草 contract | 基于三份只读评估报告 |
| - | Generator | (待评审) | |
| - | Evaluator | (待评审) | |
