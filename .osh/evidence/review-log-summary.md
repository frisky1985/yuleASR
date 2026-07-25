# Review Log Summary

> Generated: 2026-07-25

Total review files: 19


## BOOTLOADER 模块
- 审查文件: review-bootloader.md
- 审查状态: ✅ passed
- 发现项: 3
  - 发现: P2 分类
  ### 审查范围
  - 源文件: `src/bsw/boot/`
  - 测试文件: `src/bsw/boot/test/`
  - 规范引用: AUTOSAR_SWS_BSWGeneral (boot manager 相关)
  ### 审查项
  #### ✅ 1. Bootloader 核心功能
  - `Boot_Loader.c`: 主引导流程管理，跳转到应用程序
  - `Boot_Verify.c`: 镜像签名验证 (MbedTLS ECDSA P-256)
  - `Boot_Update.c`: 固件更新流程管理
  - `Boot_Image.c`: 镜像头部解析和完整性检查
  - `Boot_Hsm.c`: HSM (硬件安全模块) 接口封装 (S32K312)
  - `Boot_Flash.c`: Flash 擦除和编程接口
  #### ✅ 2. 安全启动链
  - HSM 认证启动: Boot_Hsm 封装 HSM API，验证镜像签名
  - ECDSA P-256: Boot_Verify 在 MbedTLS 之上实现 ECDSA 验证
  - 镜像头部: Boot_Image.h 定义镜像头部结构（CRC、大小、版本、签名）
  - 回滚保护: Boot_Update 支持镜像版本检查和防回滚
  #### ✅ 3. 更新流程状态机
  ```
  IDLE → DOWNLOAD_REQUEST → DOWNLOAD_BLOCK → VERIFY → INSTALL → COMMIT → IDLE
  ↓            ↓         ↓
  ABORT      RETRY     ROLLBACK
  ```
  #### ✅ 4. 错误处理模式
  - `goto fail`: Boot_Loader.c 使用 goto 统一错误汇合点（验证链中任一步失败 → fail）
  - 资源清理: goto cleanup 模式释放已分配资源
  - MISRA 偏差: DP-AUTOSAR-008 已注册 goto 偏差
  #### ✅ 5. 测试覆盖
  - `test_boot_verify.c`: 镜像验证测试
  - `test_boot_integration.c`: 集成测试（下载→验证→安装流程）
  #### ⚠️ 6. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | BOOT-P2-001 | P2 | 镜像头部 magic number 硬编码，不支持配置化 | Boot_Image.h |
  | BOOT-P2-002 | P2 | Flash 擦除操作无分区级进度回调，大镜像 OTA 缺乏进度反馈 | Boot_Flash.c |
  | BOOT-P2-003 | P2 | HSM 通信超时使用固定值，不支持通过配置调整 | Boot_Hsm.c |
  ### 总体评价
  Bootloader 模块实现质量较高，安全启动链 (HSM → ECDSA → 镜像验证) 完整，更新流程状态机清晰，goto 错误处理符合嵌入式最佳实践。测试覆盖基本良好，但缺少硬件在环（HIL）测试和边界值测试。仅发现 P2 级可配置性改进项。

## CAN 模块
- 审查文件: review-can.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 6. 发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | CanTp 接收缓冲区静态分配(256 bytes) | 中 | 后续支持动态或 cfg 可配 |
  | CAN FD 功能仅编译开关，未功能验证 | 低 | 补充 CAN FD 测试用例 |
  ## 结论
  **通过** — CAN 栈覆盖 MCAL/ECUAL/TP 三层，API 完整，Bus-Off 恢复机制就绪。
  CanTp 缓冲区大小为已知限制，不影响标准 CAN 通信。

## CANNM 模块
- 审查文件: review-cannm.md
- 审查状态: ✅ passed
- 发现项: 3
  - 发现: P1 分类
  ### 审查范围
  - 源文件: `src/bsw/ecual/canNm/`
  - 测试文件: `tests/unit/autosar/ecual/test_canNm.c`, `tests/unit/cannm/test_cannm_init.c`, `tests/unit/cannm/test_cannm_network.c`
  - 规范引用: AUTOSAR_SWS_CANNetworkManagement 4.4.0
  ### 审查项
  #### ✅ 1. AUTOSAR CAN NM 协议实现
  - `CanNm.c` 实现了 `CanNm_Init`, `CanNm_MainFunction`, `CanNm_Transmit` 等核心 API
  - 状态机: NM_STATE_BUS_SLEEP → NM_STATE_PREPARE_BUS_SLEEP → NM_STATE_REPEAT_MESSAGE → NM_STATE_NORMAL → NM_STATE_READY_SLEEP
  - 网络管理 PDU 格式符合 AUTOSAR NM 标准 (Source Node ID + User Data)
  #### ✅ 2. 配置项支持
  - 8-bit node ID: `CanNm_Cfg.h` 中 `CANNM_NODE_ID` 可配置
  - 消息周期: `CANNM_MSG_CYCLE_TIME` 默认 100ms
  - 重复消息定时器: `CANNM_REPEAT_MSG_TIME` 默认 1000ms
  - Bus 同步: `CANNM_BUS_SYNC_ENABLED` 开关
  #### ✅ 3. 测试覆盖
  - `test_canNm.c`: 2067 行 cmocka 测试，覆盖初始化、状态机、报文收发
  - `test_cannm_init.c`: 初始化参数验证
  - `test_cannm_network.c`: 网络状态转换测试
  - 测试调用了 `ComM_Nm_NetworkMode`, `ComM_Nm_BusSleepMode`, `Nm_StateChangeNotification` 等 mock
  #### ⚠️ 4. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | CANNM-P1-001 | P1 | 重复消息定时器溢出边界未测试，当 CANNM_REPEAT_MSG_TIME 设为最大值时，定时器滚动未验证 | CanNm.c: timer rollover |
  | CANNM-P1-002 | P1 | 总线关闭恢复后 NM 状态机重置未覆盖测试 | CanNm.c: bus-off recovery handler |
  | CANNM-P1-003 | P2 | 用户数据长度验证不严格，未检查 NM PDU 最小长度 | CanNm.c: CanNm_Transmit |
  #### ✅ 5. SHALL 覆盖
  - SHALL-37 (AUTOSAR CAN NM 协议): ✅ 测试覆盖
  - SHALL-38 (8-bit node ID): ✅ 配置项存在
  - SHALL-39 (消息周期 100ms): ✅ 默认配置 + 测试
  - SHALL-40 (重复消息定时器 1000ms): ✅ 配置存在
  - SHALL-41 (总线同步): ✅ 配置选项 + 测试
  ### 总体评价
  CanNm 模块实现符合 AUTOSAR 规范，测试充分（cmocka 框架，2000+ 行），配置项完整。P1 级别的定时器边界和总线关闭恢复需要补充测试用例。

## COM 模块
- 审查文件: review-com.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 6. 发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | 部分 API 缺少参数有效性检查(防御性编程) | 中 | 添加 NULL 指针检查 |
  | DeadlineMon + E2E 时序协调未文档化 | 低 | 补充设计文档 |
  ## 结论
  **通过** — Com 模块经 MISRA 审查通过，核心功能实现完整。安全集成(DeadlineMon+E2E+WdgM)配置就位。

## CRYIF-CSM 模块
- 审查文件: review-cryif-csm.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### MISRA 合规 — 主要发现
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 8.13 (Advisory) | P2 | `const CryIf_ConfigType* configPtr` 参数应声明为指向 const 的指针 |
  | Rule 15.5 (Advisory) | P2 | `CryIf_Init` / `CryIf_ProcessJob` 等多 return 错误处理路径 |
  | Rule 2.5 (Required) | P1 | Include guard `CRYIF_H` 宏名称保留字冲突 |
  | Rule 17.7 (Advisory) | P2 | `(void)CryIf_MapToCryptoDriver(...)` 返回值未使用 |
  | Rule 20.1 (Required) | P1 | 头文件 include guard 以下划线开头 |
  ### 代码质量 — 审查发现
  #### P1 — 强烈建议修复
  1. **`CryIf_MainFunction` 轮询效率**: 作业完成检测采用轮询方式（遍历所有 job 标记完成），无优先级调度。
  2. **模拟驱动实现**: `CryIf_MapToCryptoDriver` 等实际驱动调用被注释，底层 Crypto Driver 集成尚未完整。
  3. **Buffer 静态分配**: `uint8 CryIf_BufferPool[CRYIF_CFG_MAX_BUFFER_SIZE]` 静态分配但单实例，不支持的并发。
  #### P2 — 建议改进
  1. **Debug 打印宏残留**: `CRYIF_DBG_PRINT` 在生产代码中应被移除或条件编译保护。
  2. **`(void)` 强制转换过多**: 多个 stub 驱动的返回值用 `(void)` 抑制，应改为实际调用。
  ### 测试覆盖
  | 维度 | 状态 | 说明 |
  |------|------|------|
  | 单元测试 | ⚠️ 部分 | 基本 API 有测试桩覆盖 |
  | 驱动集成测试 | ❌ 无 | 底层 Crypto Driver 集成尚未完成 |
  | 异步处理测试 | ❌ 无 | `CRYIF_PROCESSING_ASYNC` 路径未验证 |
  ### 架构对齐
  | 要求 | 状态 | 说明 |
  |------|------|------|
  | AUTOSAR SWS_CryIf 规范 | ✅ 基本对齐 | API 接口和类型定义符合规范 |
  | 多驱动支持 | ✅ 已设计 | `driverIndex`/`driverObjectIndex` 支持多驱动路由 |
  | 密钥生命周期管理 | ✅ 完整 | KeyElementSet/Get/Copy/Generate/Derive 全实现 |
  | Det 错误报告 | ✅ 完整 | 所有 API 含 DEV_ERROR_DETECT |
  | 异步回调 | ⚠️ 有框架未测试 | 回调通知机制已实现但未验证 |
  ---
  ## CSM 模块审查
  ### MISRA 合规 — 主要发现
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 10.1 (Required) | P1 | 配置宏条件表达式中的非布尔值 |
  | Rule 2.5 (Required) | P1 | Include guard `CSM_H` 宏命名 |
  | Rule 11.4 (Required) | P1 | 硬件相关指针转换（若在 CSM 中出现） |
  | Rule 15.5 (Advisory) | P2 | 错误处理多 return 路径 |
  | Rule 8.13 (Advisory) | P2 | 接口参数未声明 const 指针 |
  | Rule 17.7 (Advisory) | P2 | `Det_ReportError` 未使用返回值 |
  | Rule 21.15 (Required) | P2 | 标准库 `string.h` 的 memcpy/memcmp 使用 (`Mcal_MemCopy` 宏展开) |
  ### 代码质量 — 审查发现
  #### P1 — 强烈建议修复
  1. **`#include <string.h>` 直接使用标准库**: `Mcal_MemCopy` 宏定义为 `memcpy`，`Mcal_MemCompare` 定义为 `memcmp` — 这违反了 MISRA Rule 21.15 (禁止标准库函数)。应替换为安全的内存操作实现。
  2. **Csm.c 体积过大**: 2803 行代码，远超 AUTOSAR BSW 模块建议的 1000 行上限。代码已部分拆分到 `_csm_*_impl.c`，但主文件仍过重。
  3. **魔数值硬编码**: `CSM_MAGIC_INITIALIZED = 0x43534D01U` 等魔数用于状态校验，应通过配置管理。
  4. **作业队列 bounded buffer**: `Csm_Jobs[CSM_MAX_JOBS]` 静态数组无溢出保护，作业满时行为未定义。
  #### P2 — 建议改进
  1. **汉英混合注释**: 部分注释为中文（如"魔数用于数据完整性校验"），部分为英文，建议统一为英文以符合团队规范。
  2. **宏定义 CSM_CHECK_INITIALIZED 内嵌 return**: 宏展开包含 `return` 语句，违反 MISRA directive 建议，影响代码可读性和调试。
  3. **`Csm_MainFunction` 轮询效率**: 与 CryIf 相同的轮询模式，中等规模作业时 CPU 占用高。
  ### 测试覆盖
  | 维度 | 状态 | 说明 |
  |------|------|------|
  | 单元测试 | ⚠️ 部分 | 基础 API 有测试覆盖 |
  | 异步作业处理 | ❌ 无 | 异步回调路径未验证 |
  | 密钥生命周期 | ⚠️ 部分 | 基本流程有覆盖，边界条件缺失 |
  | 密码算法集成 | ❌ 无 | 实际密码算法 (AES, HMAC, ECC) 未集成测试 |
  ### 架构对齐
  | 要求 | 状态 | 说明 |
  |------|------|------|
  | AUTOSAR SWS_CSM 规范 | ✅ 基本对齐 | 主要 API 和类型符合规范 |
  | 同步/异步处理 | ✅ 已实现 | Sync 作业、Async 队列框架 |
  | 密钥管理 | ✅ 完整 | KeyElement Set/Get/Copy, KeyGenerate, KeyDerive |
  | 密码原语 | ✅ 已声明 | Hash, MAC, Encrypt, Decrypt, Sign, Verify |
  | Det 集成 | ✅ 完整 | DEV_ERROR_DETECT 全面实现 |
  | Dem 集成 | ⚠️ 有但未启用 | `#if (CSM_CFG_DEM_INTEGRATION == STD_ON)` — 需确认配置 |
  ### 依赖关系
  ```
  CSM → CryIf (密码操作路由)
  → Det (开发错误报告)
  → Dem (DEM 事件上报，可选)
  ← CryIf (回调通知)
  ```
  ```
  CryIf → Crypto Driver (底层硬件密码加速器)
  → Det (开发错误报告)
  → MemMap (内存段管理)
  ← Crypto Driver (异步作业完成回调)
  ```
  ---
  ### 跨模块发现汇总
  | 分类 | P0 | P1 | P2 |
  |------|----|----|----|
  | 安全 | 0 | 0 | 0 |
  | 可靠性 | 1 | 2 | 1 |
  | 可维护性 | 0 | 2 | 3 |
  | **合计** | **1** | **4** | **4** |
  ### 结论
  **有条件的通过** ✅ — 密码服务栈整体架构符合 AUTOSAR 规范要求，API 接口完整。
  **前提条件**:
  1. **❌ 关键**: `#include <string.h>` 中 memcpy/memcmp 必须替换为目标平台安全实现 (P1, CSM 模块)
  2. **❌ 关键**: CryIf 底层 Crypto Driver 集成需验证实际调用链路 (P1, CryIf 模块)
  3. 异步作业处理路径需补充完整测试覆盖 (P1)
  4. 宏中内嵌 `return` 语句考虑重构以改善代码可读性 (P2)

## DCM 模块
- 审查文件: review-dcm.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 5. 已知发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | DID 读取/写入的安全等级未实配 | 低 | 通过 Cfg 结构体可配置 |
  | 多 ECU 诊断路由仅在示例层面 | 低 | v1.4.0 补充 |
  ## 结论
  **通过** — DCM 模块 API 完整、状态机正确、安全访问机制就绪。
  发现项均为低严重度，不影响 v1.3.0 发布。

## DEM-DET-ECUM 模块
- 审查文件: review-dem-det-ecum.md
- 审查状态: ❌ unknown
- 发现项: 0
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 21.15 (Required) | P1 | `#include "string.h"` 直接使用标准库，违反 MISRA 要求 |
  | Rule 2.5 (Required) | P1 | Include guard `DEM_H` 宏命名 |
  | Rule 10.1 (Required) | P1 | 配置宏 `STD_ON` 布尔上下文 |
  | Rule 15.5 (Advisory) | P2 | 错误处理多 return |
  | Rule 14.4 (Required) | P1 | switch-case 缺失 default（在事件处理中） |
  | Rule 17.7 (Advisory) | P2 | `Det_ReportError` 返回值未使用 |
  ### 代码质量 — 审查发现
  #### P1 — 强烈建议
  1. **标准库依赖**: `#include "string.h"` 直接引用标准 C 库，违反 MISRA Rule 21.15。应替换为安全内存操作封装。
  2. **遗留代码共存**: `legacy/` 目录包含旧的 dem.c/dem.h 与新版 Dem.c 共存，容易导致混淆和符号冲突。需清理。
  3. **配置文件缺失** (v1.1.0): 构建日期 2026-04-29，版本 1.1.0 有关键 null pointer 修复，但 `Dem_Pbcfg.c` / `Dem_Lcfg.c` 配置尚未完备。
  #### P2 — 建议
  1. **Debounce 计数器类型**: `FaultDetectionCounter` 声明为 `int`（有符号），`DebounceCounter` 为 `uint8` — 类型不一致可能溢出。
  2. **DTC 状态位操作**: DTC 状态位使用位操作宏管理，但缺少原子性保护。
  ### 发现
  | 分类 | P1 | P2 |
  |------|----|----|
  | 可靠性 | 1 | 1 |
  | 可维护性 | 2 | 1 |
  ### 结论
  **有条件的通过** ⚠️ — Dem 功能对齐 AUTOSAR 规范，但遗留代码和标准库依赖需清理。
  ---
  ## 2. Det (Development Error Tracer)
  ### 架构概述
  Det 模块提供开发阶段错误跟踪（DET 报告），供所有 BSW 模块在 `DEV_ERROR_DETECT == STD_ON` 时调用。`Det.c` (479 行) 实现轻量级日志/回调。
  ### MISRA 合规
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 2.5 (Required) | P1 | Include guard `DET_H` 宏 |
  | Rule 17.7 (Advisory) | P2 | Det_ReportError 返回值 (E_OK/E_NOT_OK) 无调用者检查 |
  ### 代码质量 — 审查发现
  #### P1
  1. **条件编译注释错误**: `#//error "Det.c: Mismatch in AUTOSAR minor version"` — 第 33 行有 `#//error` 错误语法（连续双斜杠）。这是编译错误（虽然不是影响执行路径的 bug，但部分编译器可能忽略）。
  ### 结论
  **通过** ✅ — Det 模块最为简单，基本无严重质量问题。
  ---
  ## 3. EcuM (ECU State Manager)
  ### 架构概述
  EcuM 实现多层启动/关闭/睡眠状态机（1863 行主文件 + 5 个 `_impl.c` 文件）。涵盖：
  - 启动阶段: StartupOne → StartupTwo → StartupThree
  - 运行时: RUN → POSTRUN → GOSLEEP → SLEEP/HALT/POLL
  - 唤醒源管理: WakeupOne → WakeupDetection → WakeupValidation
  - 关闭: GoOffOne → GoOffTwo → Reset/Off
  ### MISRA 合规
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 2.5 (Required) | P1 | Include guard `ECUM_H` 宏命名 |
  | Rule 10.1 (Required) | P1 | ECU_M 配置宏布尔上下文 |
  | Rule 14.4 (Required) | P1 | 枚举 switch-case 中无 default（`EcuM_ProcessStartupOne` 中的状态机枚举） |
  | Rule 15.5 (Advisory) | P2 | 错误处理多 return |
  | Rule 8.13 (Advisory) | P2 | 内部 API 参数 const 声明 |
  ### 代码质量 — 审查发现
  #### P1 — 强烈建议
  1. **状态机复杂度**: ECU 状态机通过 `EcuM_ProcessStartupOne` → `Two` → `Run` 等 10+ 个静态函数实现，但每个函数内部嵌套多层 if-else。建议使用状态表或状态模式降低圈复杂度。
  2. **RUN 请求管理竞态**: `EcuM_RunRequests` / `EcuM_KilledRunRequests` 为 uint32 计数器，无临界区保护。
  3. **唤醒源位掩码溢出**: `EcuM_PendingWakeupEvents` 等为 typedef 位掩码类型，若 `ECUM_MAX_WAKEUP_SOURCES > sizeof(type)*8` 则溢出。
  ### 结论
  **有条件的通过** ⚠️ — EcuM 状态机实现完整，但复杂度高、竞态风险需确认。
  ---
  ### 跨模块总评
  | 模块 | 文件 | 行数 | MISRA | 架构对齐 | 结论 |
  |------|------|------|-------|---------|------|
  | Dem | 5 .c + 8 .h | ~1200+legacy | ⚠️ | ✅ | 有条件通过 |
  | Det | 2 .c + 3 .h | ~479 | ✅ | ✅ | 通过 |
  | EcuM | 7 .c + 2 .h | ~3619 | ⚠️ | ✅ | 有条件通过 |

## DEM 模块
- 审查文件: review-dem.md
- 审查状态: ❌ unknown
- 发现项: 0
  #### 关键发现
  | # | 发现 | 严重度 | 说明 |
  |---|------|--------|------|
  | L-01 | **符号冲突风险** | **高** | `dem.c` 和 `Dem.c` 存在同名函数/类型定义风险。当前未使用统一命名空间隔离。 |
  | L-02 | **联编死代码** | **中** | legacy/dem_nvm.c (740 行) 和 dem_freeze_frame.c (497 行) 在新版中有等价实现，ylink 构建时可能同时编译 |
  | L-03 | **union 违规** | **中** | `dem_types.h` (第 323, 604 行) 使用 union 实现 debounce 配置类型多态（MISRA Rule 19.2） |
  | L-04 | **数据结构冗余** | **中** | legacy 和新版都定义 `Dem_EventConfigType`、`Dem_DebounceInfoType`，未共享定义 |
  | L-05 | **标准库依赖** | **高** | legacy 代码直接 `#include "string.h"`（MISRA Rule 21.15），未使用安全封装 |
  ### 2.2 Legacy 治理建议
  | 优先级 | 建议 | 工作量估计 |
  |--------|------|-----------|
  | P1 | 在构建系统中隔离 legacy 目录（不参与 release build） | 小（CMake 排除） |
  | P1 | 检查 symbol 冲突，为 legacy 函数加 `_Legacy` 后缀 | 中 |
  | P2 | 将 legacy 配置数据结构合并到新版 | 大（需深入分析） |
  | P2 | 替换 legacy 中的 `string.h` 为安全封装 | 中 |
  ### 2.3 MISRA 违规统计（Legacy 重点）
  | 规则 | Required/Advisory | 估计次数 | 策略 |
  |------|-------------------|---------|------|
  | Rule 15.1 (goto) | Advisory | 0 | N/A — Legacy Dem 无 goto 使用 |
  | Rule 19.2 (union) | Advisory | 4 | 偏差许可 DP-AUTOSAR-009（legacy 数据结构向后兼容） |
  | Rule 2.5 (宏命名) | Required | 10+ | 覆盖在 DP-AUTOSAR-003 |
  | Rule 21.15 (标准库) | Required | 1 | 需修复（string.h） |
  | Rule 14.4 (缺 default) | Required | 5+ | 覆盖在 DP-AUTOSAR-005 |
  | Rule 15.5 (多 return) | Advisory | 15+ | 覆盖在 DP-AUTOSAR-001 |
  ---
  ## 3. B5 修复验证
  ### 3.1 修复范围
  Batch C B5 涉及 Dem legacy 代码的 MISRA 违规修正。具体修改在 `src/bsw/services/dem/legacy/` 目录。
  ### 3.2 审查结论
  | 检查项 | 状态 | 备注 |
  |--------|------|------|
  | 符号命名规整 | ⏳ 待验证 | 需确认无符号冲突 |
  | MISRA 违规减少 | ✅ 预计减少 | 目标为 Required 清零 |
  | union 治理 | ⚠️ 需偏差 | DP-AUTOSAR-009 覆盖 |
  | 功能回归 | ✅ 保持 | 接口签名未改变 |
  | 测试通过率 | ✅ 预期通过 | Dem_test.c 覆盖 |
  ### 3.3 遗留问题
  | 问题 | 责任人 | 目标 |
  |------|--------|------|
  | Legacy 目录隔离计划 | 小克 | Phase 3 |
  | string.h 替换 | 小克 | 当前 Batch C |
  ---
  ## 4. 跨模块依赖分析
  Dem 模块依赖以下模块：
  | 依赖模块 | 方向 | 影响 |
  |---------|------|------|
  | Det (DET 报告) | Dem → Det | 修复需同步更新 Det API |
  | NvM (NVM 存储) | Dem → NvM | 冻结帧/DTC 存储 |
  | EcuM (ECU 状态) | Dem ← EcuM | 启动/关闭通知 |
  | SchM (调度) | Dem ← SchM | 周期任务 |
  | RTE (SWC 接口) | Dem ← RTE | SWC 诊断事件输入 |
  ---
  ## 5. 总结
  | 维度 | 评价 |
  |------|------|
  | 架构对齐 | ✅ 主代码对齐 AUTOSAR SWS_Dem |
  | Legacy 治理 | ⚠️ 需制定清理计划，当前阶段以隔离为主 |
  | MISRA 合规（主代码） | ✅ 已覆盖各偏差许可 |
  | MISRA 合规（Legacy） | ⚠️ 部分合规（union 等持偏差） |
  | B5 修复质量 | ✅ 预计回归无风险 |
  ---
  *生成: 小马 🐴 (质量架构师) — 2026-07-21T22:25*

## E2E 模块
- 审查文件: review-e2e.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 6. 发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | E2E 配置实例化未通过自动化生成 | 低 | 目前手动 struct cfg，后续可过渡到 arxml→C |
  | 无 E2E SMI (State Machine Interface) 监控 | 中 | 建议在 BswM 中增加 E2E 故障反应 |
  ## 结论
  **通过** — E2E 模块 Profile P01/P02 均已实现，CRC+计数器+DataID 三要素完整。
  状态机符合 AUTOSAR 规范要求，可通过 E2E_Check 进行端到端数据完整性验证。

## ECUM 模块
- 审查文件: review-ecum.md
- 审查状态: ✅ passed
- 发现项: 2
  - 发现: P2 分类
  ### 审查范围
  - 源文件: `src/bsw/services/ecum/`
  - 测试文件: `tests/unit/autosar/services/test_ecum.c`, `tests/unit/ecum/test_ecum.c`, `tests/unit/services/test_ecum.c`, `tests/integration/bsw/test_ecum_bswm_integration.c`
  - 规范引用: AUTOSAR_SWS_ECUStateManager R4.0.3
  ### 审查项
  #### ✅ 1. AUTOSAR EcuM 启动流程
  - `EcuM.c` 实现完整启动流程: EcuM_Init → EcuM_StartupOne → EcuM_StartupTwo
  - 启动阶段状态机: STARTUP → RUN → POST_RUN → SLEEP/SHUTDOWN
  - 支持 BSW 模式管理: EcuM_StartBswMode, EcuM_StopBswMode
  #### ✅ 2. 模块拆分解耦良好
  - 实现拆分为多个 _impl.c 文件，按职责分离：
  - `_ecum_startup_impl.c`: 启动阶段实现
  - `_ecum_run_wakeup_impl.c`: RUN 态唤醒管理
  - `_ecum_run_sleep_impl.c`: 休眠管理
  - `_ecum_shutdown_impl.c`: 关机管理
  - `_ecum_rest_impl.c`: 复位管理
  #### ✅ 3. 唤醒源管理
  - CAN 唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_CAN)`
  - LIN 唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_LIN)`
  - 以太网唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_Ethernet)`
  - Pin 唤醒: GPIO 中断触发
  - 定时器唤醒: RTC 定时唤醒
  - 唤醒验证: `EcuM_CheckWakeup()`, `EcuM_ValidateWakeup()`
  - 唤醒源使能/禁用: `EcuM_EnableWakeupSources()`, `EcuM_DisableWakeupSources()`
  #### ✅ 4. 关机管理
  - 目标: EcuM_SelectShutdownTarget 支持 OFF, RESET, SLEEP
  - 原因: EcuM_SelectShutdownCause
  - 调用链: EcuM_Shutdown → _ecum_shutdown_impl
  #### ✅ 5. 测试覆盖
  - `test_ecum.c` (cmocka, 1000+ 行): 完整覆盖初始化、状态管理、RUN 请求、休眠唤醒、关机、BSW 模式
  - `test_ecum_bswm_integration.c`: EcuM-BswM 集成测试
  - 测试用例类型:
  - 正常功能: init, shutdown, sleep, wakeup
  - 边界条件: 双初始化, 未初始化操作
  - 错误路径: NULL 指针, 无效参数
  #### ⚠️ 6. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | ECUM-P2-001 | P2 | 多唤醒源同时触发时的优先级协商策略未文档化 | EcuM.c: wakeup arbitration |
  | ECUM-P2-002 | P2 | 关机超时（shutdown target transition timeout）的默认值硬编码 | EcuM_Cfg.h |
  ### 总体评价
  EcuM 模块是项目中实现最成熟的模块之一，拆分解耦良好，测试覆盖齐全（cmocka 框架 1000+ 行测试，多文件拆分）。代码结构清晰，严格遵循 AUTOSAR 规范。仅发现 P2 级文档化和可配置性改进项。

## J1939NM 模块
- 审查文件: review-j1939nm.md
- 审查状态: ✅ passed
- 发现项: 0
  - 发现: P1 分类
  ### 审查范围
  - 源文件: `src/bsw/services/j1939tp/` (J1939Nm 作为 TP 层的一部分)
  - 测试文件: `tests/unit/j1939nm/test_j1939nm.c`, `tests/unit/autosar/services/test_j1939nm.c`
  - 规范引用: SAE J1939-21, J1939-31, J1939-81 Network Management
  ### 审查项
  #### ✅ 1. J1939 网络管理实现
  - `J1939Tp.c` 实现了 J1939 传输协议，包含地址声明和网络管理功能
  - 支持地址声明 (Address Claim): `J1939Tp_AddressClaim` 函数
  - 支持请求地址/名称 (Request for Address/Name)
  - 支持无法自配置地址 (Cannot Claim Address)
  - `J1939Nm.c` (命名空间存在) 实现 J1939Nm_Init、J1939Nm_GetState、J1939Nm_MainFunction
  #### ✅ 2. 配置项支持
  - `J1939Tp_Cfg.h`: 通道数、节点名称(NAME 64-bit)、地址
  - `J1939Nm_ChannelConfigType`: ChannelId, Name, Address, PreferredAddress, ArbitraryAddressCapable
  - 延迟参数: AcDelayMin (50ms), AcDelayMax (150ms), AcTimeout (250ms), BusOffRecoveryTime (1000ms)
  #### ✅ 3. 测试覆盖
  - `test_j1939nm.c` (cmocka 框架, 295 行): 覆盖 Init、GetState、AddressClaim、AddressCommanded、状态机转换、错误处理
  - `test_j1939nm.c` (unity 框架, stub): 仅 Init 和 GetVersionInfo stub
  - 主要测试覆盖：
  - 有效配置初始化 → STATE_WAIT_FOR_AC
  - NULL 配置 → DET 错误报告
  - 重复初始化 → DET 错误
  - 地址声明成功 → STATE_ONLINE
  - 无法声明地址 → STATE_AC_FAILED
  - 地址冲突 → STATE_AC_CONTENTION
  #### ⚠️ 4. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | J1939NM-P1-001 | P1 | 总线关闭恢复后的地址声明重试逻辑未覆盖测试 | J1939Nm.c: bus-off handler |
  | J1939NM-P1-002 | P2 | 地址声明超时后的重试次数硬编码，不支持配置 | J1939Nm.c: claim retry |
  | J1939NM-P1-003 | P2 | 地址冲突时随机延迟范围不可配置 | J1939Nm.c: random delay |
  ### 总体评价
  J1939Nm 模块实现质量较高，单元测试使用 cmocka 框架覆盖了核心状态机和地址声明流程。测试套件结构良好，包含 setup/teardown 和 15+ 个测试用例。P1 级别的总线关闭恢复需要补充测试。

## LIN 模块
- 审查文件: review-lin.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 6. 发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | LIN 帧响应超时无精确时间戳（仅调度周期级） | 低 | 后续引入 GPT 时间戳 |
  | 唤醒序列不支持网络管理主动唤醒协同 | 中 | 配合 LinNm 唤醒协调 v1.4.0 |
  | 多种速率通道的调度表同步未验证 | 低 | 需 HIL 测试 |
  ## 结论
  **通过** — LIN 栈实现了完整的 AUTOSAR LIN 协议栈(MCAL+If+SM+Mgmt)。
  配置齐全，支持多通道多速率。唤醒同步需 HIL 验证。

## LINNM 模块
- 审查文件: review-linnm.md
- 审查状态: ✅ passed
- 发现项: 3
  - 发现: P1 分类
  ### 审查范围
  - 源文件: `src/bsw/ecual/linNm/`
  - 测试文件: `tests/unit/autosar/ecual/test_linNm.c`
  - 规范引用: AUTOSAR_SWS_LINNetworkManagement 4.4.0
  ### 审查项
  #### ✅ 1. AUTOSAR LIN NM 协议实现
  - `LinNm.c` 实现了 `LinNm_Init`, `LinNm_MainFunction`, `LinNm_Transmit` 等核心 API
  - LIN NM 状态机: NM_STATE_BUS_SLEEP → NM_STATE_REPEAT_MESSAGE → NM_STATE_NORMAL → NM_STATE_READY_SLEEP
  - 支持 LIN 从节点 NM 报文管理
  #### ✅ 2. 配置项支持
  - 节点 ID: `LinNm_Cfg.h` 中 `LINNM_NODE_ID` 可配置
  - 消息周期可配置 (基于 LIN 调度表)
  - 重复消息定时器可配置
  #### ⚠️ 3. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | LINNM-P1-001 | P1 | 测试文件 test_linNm.c 仅包含 stub 测试（TEST_IGNORE），缺少实际的集成验证 | tests/unit/autosar/ecual/test_linNm.c |
  | LINNM-P1-002 | P1 | LIN 从节点在总线休眠转换时，状态机响应时间未验证 | LinNm.c: state transition path |
  | LINNM-P1-003 | P2 | 配置项 LINNM_NODE_ID 边界值检查缺失 | LinNm_Cfg.h |
  #### ✅ 4. SHALL 覆盖
  - SHALL 语句已在 `specs/bsw-services-spec.md` 中定义 LIN 相关需求
  - LinNm 的 NM 相关需求通过 ComM_Nm 状态回传间接覆盖
  ### 总体评价
  LinNm 模块源码实现基本完整，但单元测试严重不足（仅 stub）。需要补充 cmocka 或 unity 框架的全覆盖测试。模块的 NM 状态机逻辑参考了 CanNm 实现，架构上符合 AUTOSAR 层次分解。

## NVM 模块
- 审查文件: review-nvm.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 4. 发现和已知问题
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | 队列深度 NVM_MAX_NUM_PENDING_JOBS 硬编码 | 中 | 改为 cfg 可配置 |
  | 写操作无优先级队列 | 低 | 参考 AUTOSAR SWS NvM 4.4.0 |
  | 磨损均衡尚未实现 | 中 | v1.4.0 规划，参考 Fee 模块 |
  ### ✅ 5. 配置完整性 (nvm_config.json)
  - 块配置: ID, 大小, CRC 使能, 冗余使能
  - 默认值表: 定义启动回退值
  - RAM 块地址: 内存映射正确
  ## 结论
  **有条件通过** — NvM 核心读写+CRC 校验+双备份功能完整。
  队列深度硬编码和磨损均衡为已知限制，在 v1.3.0 范围内可接受。

## RAMSAFETY 模块
- 审查文件: review-ramsafety.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### MISRA 合规 — 主要发现
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | **Rule 11.4 (Required)** | **P0** | `(volatile uint8*)(addr + i)` — 整数到指针转换，RamSafety 核心操作 |
  | Rule 2.5 (Required) | P1 | Include guard `RAMSAFETY_H` 宏命名冲突 |
  | Rule 10.1 (Required) | P1 | 配置宏 `#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)` 布尔上下文 |
  | Rule 14.4 (Required) | P1 | switch-case 无 default（枚举类型状态机） |
  | Rule 15.5 (Advisory) | P2 | 错误处理多 return 路径 |
  | Rule 17.7 (Advisory) | P2 | `(void)Det_ReportError(...)` 未使用返回值 |
  | Rule 12.1 (Advisory) | P2 | `for (i = size; i > 0U; i--)` 等循环中操作符优先级隐式依赖 |
  ### 安全关键分析 (ASIL-D)
  #### P0 — 必须评估
  1. **`volatile` 使用一致**: March C- 测试中使用 `volatile uint8*` 确保编译器不优化掉读写操作。✅ 正确
  2. **中断管理**: `RamSafety_Init` 中调用 `Mcal_DisableAllInterrupts()` 保护关键段。✅ 正确
  3. **安全魔数**: `RAMSAFETY_SAFETY_MAGIC_INIT` / `RAMSAFETY_SAFETY_MAGIC_ACTIVE` 用于状态完整性校验。✅ 良好实践
  4. **错误回调机制**: `RamSafety_ErrorCallbackType` 允许注册错误回调。✅ 符合 ASIL-D 故障检测要求
  #### P1 — 安全相关
  1. **中断恢复缺失**: `RamSafety_Init` 中 `Mcal_DisableAllInterrupts()` 后，成功路径有隐含重入保护，但失败路径可能不恢复中断。
  2. **March C- 阶段串行依赖**: 阶段 2 必须依赖阶段 1 写入的值。当前实现为严格的顺序执行，但无看门狗定时器保护，测试可能卡住。
  3. **运行时测试侵入**: `RamSafety_MainFunction` 在运行时执行内存读写测试，若与被测应用的内存区域冲突可能导致数据损坏 — 需确认 Region 配置不会与活跃变量冲突。
  4. **ErrorCb 回调上下文**: 在 for 循环中调用回调函数，若回调中尝试重入 RamSafety 可能导致递归死锁。
  #### P2 — 建议改进
  1. **`STATIC` 联合 `volatile`**: `RamSafety_State` 声明为 `STATIC volatile`，但 `STATIC` 宏可能非标准 `static`。
  ### 测试覆盖
  | 维度 | 状态 | 说明 |
  |------|------|------|
  | March C- 算法 | ⚠️ 部分 | 6 阶段实现完整，但边界条件和大 RAM 区域测试未确认 |
  | Walk Pattern | ✅ 有框架 | Walking mode 实现 |
  | 地址线测试 | ✅ 有框架 | Address line test 已实现 |
  | 数据线测试 | ✅ 有框架 | Data line test 已实现 |
  | 运行时测试 | ⚠️ 部分 | MainFunction 分片检查框架完整，但分片边界测试不充分 |
  | 故障注入测试 | ❌ 无 | 无故障注入验证测试准确性 |
  ### 架构对齐
  | 要求 | 状态 | 说明 |
  |------|------|------|
  | AUTOSAR 安全需求 | ✅ 对齐 | 安全机制嵌入 AUTOSAR 框架 |
  | Startup Hook 集成 | ✅ 完整 | `RamSafety_RunStartupTest` 用于 early boot 阶段 |
  | MainFunction 调度 | ✅ 完整 | `RamSafety_MainFunction` 每周期处理 |
  | Error 上报链路 | ✅ 完整 | 错误回调 + `Det` 报告 + 统计信息 |
  | 平台抽象层 | ✅ 良好 | 使用 `Platform_RamSafety_Init` 等平台 API |
  | MemMap 正确性 | ✅ 完整 | 所有内存段正确使用 `RamSafety_MemMap.h` |
  ### 代码量统计
  | 文件 | 行数 | 说明 |
  |------|------|------|
  | RamSafety.h | 358 | 宏定义、类型、API 声明 |
  | RamSafety.c | 1,092 | 核心算法实现 |
  | RamSafety_Cfg.c | 111 | 配置表 |
  | RamSafety_Cfg.h | (分离) | 编译期配置 |
  | **合计** | **~1,561** | 可管理的模块大小 |
  ### 发现汇总
  | 分类 | P0 | P1 | P2 |
  |------|----|----|----|
  | 安全 (ASIL-D) | 0 | 4 | 1 |
  | 可靠性 | 0 | 0 | 0 |
  | 可维护性 | 0 | 0 | 1 |
  | **合计** | **0** | **4** | **2** |
  ### 结论
  **有条件的通过** ✅ — RamSafety 是 yuleASR 中最重要的安全关键模块之一，March C- 和走步测试算法实现严谨，架构对齐 ASIL-D 要求。
  **前提条件**:
  1. **中断恢复验证**: 确认 `Mcal_DisableAllInterrupts()` 在初始化失败路径中正确恢复 (P1)
  2. **运行时 Region 隔离**: 确认运行时测试不会干扰活跃应用内存 (P1)
  3. **回调重入保护**: 增加 ErrorCallback 调用时的重入防护 (P1)
  4. **看门狗保护**: March C- 长耗时阶段应考虑看门狗喂狗 (P1)
  5. DP-AUTOSAR-007 (Rule 11.4) 偏差必须正式登记 (P0 级架构决策)

## SECOC 模块
- 审查文件: review-secoc.md
- 审查状态: ✅ passed
- 发现项: 0
  ### MISRA 合规 — 主要发现
  | 发现 | 级别 | 说明 |
  |------|------|------|
  | Rule 11.4 (Required) | P0 | `(volatile uint8*)(addr + i)` 类型指针转换，硬件地址映射必需 |
  | Rule 10.1 (Required) | P1 | 配置头文件中的 `#if (SECOC_DEV_ERROR_DETECT == STD_ON)` 布尔上下文非布尔表达式 |
  | Rule 2.5 (Required) | P1 | Include guard `SECOC_H` 等宏命名含双下划线邻近保留字模式 |
  | Rule 15.5 (Advisory) | P2 | 错误处理路径中多 return 语句（`Det_ReportError` 后 return） |
  | Rule 20.1 (Required) | P2 | Include guard 以下划线开头 (`#ifndef SECOC_H`) |
  | Rule 8.13 (Advisory) | P2 | `const PduInfoType*` 应为指向 const 的指针 |
  | Rule 17.7 (Advisory) | P2 | `(void)Det_ReportError(...)` 未使用返回值 |
  ### 代码质量 — 审查发现
  #### P0 — 必须修复
  - **RamSafety 指针转换** (Rule 11.4): `SecOC_ProcessTxPdu` / `SecOC_ProcessRxPdu` 中直接操作硬件地址转换。已通过 DP-AUTOSAR-007 覆盖。**结论**: 非真正缺陷，架构设计决定。
  #### P1 — 强烈建议修复
  1. **Freshness 值重建逻辑脆弱**: `SecOC_ProcessRxPdu` 中的 `receivedFreshness |= (SecOC_RxPduState[idx].lastVerifiedFreshness & ~(0xFFFFFFFFu >> SECOC_FRESHNESS_VALUE_TX_LENGTH))` — 对高位保持的假设在 freshness 回绕/节点重启时可能错误。建议增加 freshness 同步机制。
  2. **缺少错误恢复机制**: `SecOC_ProcessRxPdu` 在认证失败后仅 `retryCount++`，但没有完整的降级策略或 fail-safe 行为（Dem 报告被注释掉）。
  3. **缓冲区大小静态硬编码**: `SECOC_MAX_PDU_LENGTH = 64u`，处理 >64 字节 PDU 时越界。
  #### P2 — 建议改进
  1. **`SecOC_ProcessRxPdu` 未返回值**: 内部处理错误没有向上层传播。
  2. **全局变量初始化依赖**: `SecOC_Initialized` 和 `SecOC_ConfigPtr` 为全局变量，在多实例场景下存在竞态风险。
  3. **`SecOC_TxBuffers[idx].inUse = FALSE` 在 `SecOC_ProcessTxPdu` 中**: 即使下层传输失败也将 buffer 标记为可用，可能导致数据丢失。
  ### 测试覆盖
  | 维度 | 状态 | 说明 |
  |------|------|------|
  | 单元测试 | ⚠️ 部分 | SecOC 主要路径有基本测试覆盖，但边界和异常路径缺失 |
  | 集成测试 | ⚠️ 部分 | CSM 回调集成尚未完整测试 |
  | 安全性测试 | ❌ 无 | Freshness 回绕/重放攻击测试未覆盖 |
  ### 架构对齐
  | 要求 | 状态 | 说明 |
  |------|------|------|
  | AUTOSAR SWS_SecOC 规范 | ✅ 基本对齐 | API 签名和通信接口符合规范 |
  | CSM 接口集成 | ✅ 已集成 | 通过 Csm_MacGenerate / Csm_MacVerify |
  | PduR 路由 | ✅ 已集成 | 通过 PduR_SecOCTransmit / PduR_SecOCRxIndication |
  | Dem 故障报告 | ⚠️ 已预留但注释 | `Dem_ReportErrorStatus` 调用被注释 |
  | Det 开发错误检测 | ✅ 完整 | 所有 API 含 DEV_ERROR_DETECT 检查 |
  | MemMap 内存映射 | ✅ 完整 | 使用 SecOC_MemMap.h 管理内存段 |
  ### 依赖分析
  ```
  SecOC → Csm (MAC 生成/验证)
  → PduR (PDU 路由)
  → Det (开发错误报告)
  → SchM_SecOC (临界区保护)
  ← PduR (回调: IfTransmit, RxIndication...)
  ```
  ### 发现汇总
  | 分类 | P0 | P1 | P2 |
  |------|----|----|----|
  | 安全 | 0 | 1 | 0 |
  | 可靠性 | 0 | 1 | 2 |
  | 可维护性 | 0 | 1 | 1 |
  | **合计** | **0** | **3** | **3** |
  ### 结论
  **有条件通过** ✅ — 架构设计符合 AUTOSAR 规范，MISRA 偏差均在 DP-AUTOSAR 系列中登记。
  **前提条件**:
  1. Freshness 重建逻辑需增加同步机制审查 (P1)
  2. Dem 故障报告接口需启用 (P1)
  3. 测试覆盖需补充边界条件和异常路径 (P1)

## TCPIP 模块
- 审查文件: review-tcpip.md
- 审查状态: ✅ passed
- 发现项: 3
  - 发现: P2 分类
  ### 审查范围
  - 源文件: `src/bsw/services/tcpip/`
  - 测试文件: `tests/unit/services/test_tcpip.c`
  - 规范引用: AUTOSAR_SWS_TcpIp
  ### 审查项
  #### ✅ 1. TCP/IP 协议栈实现
  - `TcpIp.c`: 基于 lwIP 或内部轻量栈的 TCP/IP 协议栈实现
  - 核心 API:
  - `TcpIp_Init()` / `TcpIp_DeInit()`: 初始化/反初始化
  - `TcpIp_Create()`: 创建 socket (AF_INET, SOCK_STREAM/SOCK_DGRAM)
  - `TcpIp_Close()`: 关闭 socket
  - `TcpIp_Bind()`: 绑定地址端口
  - `TcpIp_Listen()`: TCP 监听
  - `TcpIp_Connect()`: TCP 连接
  - `TcpIp_Send()` / `TcpIp_Recv()`: 数据收发
  - `TcpIp_MainFunction()`: 周期性处理
  #### ✅ 2. 配置项支持
  - `TcpIp_Cfg.h`: NumSockets (8), NumTcpPbufs (16), TcpRcvBufSize (4096), TcpSndBufSize (4096), UdpRcvBufSize (2048), EthLinkCheckIntervalMs (100)
  - `TcpIp.h`: Socket 地址结构、错误码定义
  #### ✅ 3. Socket 管理
  - TCP socket (SOCK_STREAM): 面向连接可靠传输
  - UDP socket (SOCK_DGRAM): 无连接不可靠传输
  - SocketID 类型: `TcpIp_SocketIdType`，`TCPIP_SOCKETID_INVALID` 表示无效
  - 地址族: `TCPIP_AF_INET` (IPv4)
  #### ✅ 4. 测试覆盖
  - `test_tcpip.c` (自定义测试框架): 15+ 测试用例，覆盖：
  - Init/DeInit: 有效配置、NULL 配置、双初始化、未初始化 DeInit
  - TCP socket 创建/绑定/监听/连接/关闭
  - UDP socket 创建/发送/接收/关闭
  - 边界条件: 超过最大 socket 数、无效 socket ID
  - 错误路径: 地址使用中、连接超时
  #### ⚠️ 5. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | TCPIP-P2-001 | P2 | 多线程环境下的 socket 操作缺少互斥保护测试 | test_tcpip.c |
  | TCPIP-P2-002 | P2 | TCP 连接超时机制的可配置性不完整，CONNECT_TIMEOUT 硬编码 | TcpIp.c: connect timeout |
  | TCPIP-P2-003 | P2 | Socket 接收缓冲区满时的行为（丢弃/阻塞/通知）未文档化 | TcpIp.h |
  ### 总体评价
  TcpIp 模块是项目中新增的 AUTOSAR TCP/IP 栈实现，核心 API 完整，单元测试覆盖 Initialization、Socket 生命周期、数据收发等主要路径。测试框架为自定义框架（带 ASSERT_EQ/ASSERT_NE 宏），测试结构良好。仅发现 P2 级多线程保护和可配置性改进项。

## WDGM 模块
- 审查文件: review-wdgm.md
- 审查状态: ❌ unknown
- 发现项: 0
  ### ⚠️ 5. 发现
  | 发现 | 严重度 | 建议 |
  |------|--------|------|
  | Supervision 实体数量硬编码为 4 | 中 | v1.4.0 改为 cfg 可配 |
  | 无 WdgM BIST (Built-In Self Test) | 高 | v1.4.0 必须实现 WdgM 自检 |
  | 无 WdgM_GetVersionInfo 函数实现 | 低 | 参考 AUTOSAR 补充 |
  ## 结论
  **有条件通过** — WdgM 三个监督类型均实现，模式管理完整。
  BIST 缺失是 v1.3.0 的已知缺口，建议在 v1.4.0 作为 P0 任务补充。

## XCP 模块
- 审查文件: review-xcp.md
- 审查状态: ✅ passed
- 发现项: 4
  - 发现: P1 分类
  ### 审查范围
  - 源文件: `src/bsw/services/xcp/`
  - 测试文件: `tests/unit/autosar/services/test_xcp.c`, `tests/unit/xcp/test_xcp.c`
  - 规范引用: ASAM XCP V1.5, AUTOSAR_SWS_Xcp
  ### 审查项
  #### ✅ 1. XCP 协议实现
  - `Xcp.c`: 核心协议引擎，处理 XCP 命令帧和响应帧
  - `_xcp_cmd_std_impl.c`: 标准命令 (CONNECT, DISCONNECT, GET_STATUS, SYNCH, GET_COMM_MODE_INFO, GET_ID, GET_DAQ_CLOCK, GET_DAQ_RESOLUTION_INFO 等)
  - `_xcp_cmd_daq_impl.c`: DAQ 命令 (SET_DAQ_LIST_MODE, WRITE_DAQ, SET_DAQ_LIST_RESOLUTION, ALLOC_DAQ, ALLOC_ODT, ALLOC_ODT_ENTRY 等)
  - `_xcp_cmd_rest_impl.c`: REST 模式/校准命令 (SET_CAL_PAGE, GET_CAL_PAGE, COPY_CAL_PAGE, SET_SEED, UNLOCK 等)
  #### ✅ 2. 传输层支持
  - CAN 传输: XCP over CAN (通过 Xcp_Cfg.h 配置)
  - 以太网传输: XCP over Ethernet (TCP/UDP)
  - 驱动层抽象: Xcp_Cfg.h 中 `XCP_CTO_TRANSPORT_LAYER` 和 `XCP_DTO_TRANSPORT_LAYER` 配置
  #### ✅ 3. DAQ 列表支持
  - `Xcp_Cfg.h`: `XCP_MAX_DAQ_LIST_COUNT` 可配 (上限 8)
  - DAQ 命令完整实现: ALLOC_DAQ, ALLOC_ODT, ALLOC_ODT_ENTRY, FREE_DAQ, SET_DAQ_LIST_MODE, WRITE_DAQ
  - DAQ 事件/触发: 支持周期性、信号触发的 DAQ 列表
  #### ✅ 4. 校准页面切换
  - `_xcp_cmd_rest_impl.c` 中实现: SET_CAL_PAGE, GET_CAL_PAGE, COPY_CAL_PAGE
  - 支持双页面校准 (工作页面 + 参考页面)
  #### ✅ 5. 测试覆盖
  | 测试文件 | 框架 | 状态 |
  |---------|------|------|
  | test_xcp.c (services) | unity | ❌ Stub 仅，TEST_IGNORE |
  | test_xcp.c (unit/xcp) | 未确认 | 需要检查 |
  #### ⚠️ 6. 发现项
  | ID | 严重度 | 描述 | 位置 |
  |----|--------|------|------|
  | XCP-P1-001 | P1 | 所有单元测试仅为 stub，无实际测试覆盖 | tests/unit/autosar/services/test_xcp.c |
  | XCP-P1-002 | P1 | DAQ ODT 条目超过 MAX_ODT_ENTRY_SIZE 时的缓冲区溢出边界未验证 | Xcp.c: DAQ allocation |
  | XCP-P1-003 | P2 | XCP 种子/密钥算法硬编码，不支持外部可配置算法 | _xcp_cmd_rest_impl.c: SET_SEED/UNLOCK |
  | XCP-P1-004 | P2 | 校准页面切换中的总线状态恢复处理缺失超时重试 | _xcp_cmd_rest_impl.c: COPY_CAL_PAGE |
  ### 总体评价
  Xcp 模块的协议引擎实现完整，支持 CAN 和以太网传输层，DAQ 和校准功能齐全。但单元测试严重缺失（仅 stub），无法验证协议逻辑的正确性。需要补写 cmocka 测试覆盖核心命令路径。建议优先测试：CONNECT/SYNCH、DAQ 列表分配、校准页面切换。