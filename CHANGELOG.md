# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.5.0] - 2026-08-09

294 commits since v1.4.0 (666d17e4). 主线功能与质量数字如下。

### Added

- **TcpIp 加深 (B1)** — TcpIp 模块 992→2935 行 (TcpIp.c 694→2363 / TcpIp.h 224→474 / TcpIp_Cfg.h 74→98)
  - AUTOSAR SWS TcpIp 接口面补全 (Listen/Connect/Accept/Backlog/Rx-Tx 缓冲/options 等)，导出 API 20→49 个
  - 多连接/多 socket：静态 socket 表 TCPIP_MAX_SOCKETS=8，RFC-793 子集状态机 (CLOSED/LISTEN/SYN-*/ESTABLISHED/FIN-WAIT/TIME-WAIT) + backlog 队列 + 优雅关闭
  - VLAN 支持 (TcpIp_VlanConfigType: VID 12-bit/PCP/DropUntagged)
  - 统计能力 (TcpIp_StatisticsType 13 计数器 + Get/ResetStatistics)
  - 单测 49 项全绿；tcpip_lwip_compile_check 常驻目标 (lwIP 2.2.2 headers, -Werror)
- **EthSwt 补全 (B2)** — EthSwt 模块 763→1760 行 (EthSwt.c 504→1368 / EthSwt.h 197→318 / EthSwt_Cfg.h 62→74)
  - VLAN 成员表/PVID/VID-PCP 映射/入口出口过滤/DropUntagged (对齐 TcpIp_VlanConfigType)
  - 流控 (TxPause/RxPause/水位仿真 + MainFunction 排空)
  - 端口统计 8→15 计数器 + 镜像 (Set/GetPortMirroring + MirroredFrames)
  - 导出 API 12→33 个；单测 25→70 项全绿
- **CDD_FVM (B3-1)** — src/bsw/cdd/ 新增 Flash 虚拟内存复杂驱动：bank 注册 (编译期默认表 + 运行时 RegisterBank)/选择/查询、bank 间搬移 CopyBank (擦除→拷贝→CRC 校验镜像回滚)、擦除/写保护、故障切换 (Failover + MainFunction 周期自检)；Cdd_Fvm_Hw 硬件抽象 (RAM 镜像 native 后端 + Fls 驱动目标后端)；Cdd_Fvm_Cfg.h S32K312 2×256KB 默认 bank；模块 ~1500 行，单测 44 项全绿
- **独立算法库 (B3-2)** — src/libs/ (XMEN Libraries/ 对齐，纯 C99 零依赖，与 BSW 解耦)
  - libs_crc: CRC-8 SAE-J1850 / CRC-8 AUTOSAR(H2F) / CRC-16 CCITT-FALSE / CRC-16 XMODEM / CRC-32 ISO-HDLC，流式增量 API (12 项单测)
  - libs_aes: AES-128/192/256 FIPS-197 单块 + ECB/CBC，程序化 S-box (18 项单测，含 NIST FIPS-197/SP 800-38A 向量)
- **RTE 生成器吸收 cogu 方法论 (A2+A3)** — tools/code_generators/rte/
  - 逆层序 BFS 类型排序 (依赖类型先 typedef)、type_emitter≠RTE 过滤、symbol_name 覆盖 (SYMBOL-PROPS 优先)、RteTypeCodeBlock 确定性渲染
  - Y1 类型模型按 ref 备忘录化 (TypeModel 对齐 cogu ImplementationModel)；Y2 BehaviorSettings 事件命名前缀 (对齐 cogu element.py:2944)
  - golden-string 快照测试：类型创建顺序/type_emitter 过滤/symbol_name 覆盖/完整 Rte_Type.h 快照 (时间戳归一化)；pytest 159 test functions 全绿
- **KeyM NIST SP800-108 KDF 真实实现** — keym_sp800_108_counter/derive + HKDF-SHA256 (RFC 5869) + HMAC-SHA256 (RFC 2104) + CRC32 (IEEE 802.3)；crypto_stack 模拟后端 → mbedTLS 真实后端 (AES-128-CBC+PKCS7, 常数时间 MAC_VERIFY)；test_keym 15/15 + test_csm 9/9

### Fixed

- **技术债 T1-T4 修复**
  - T1 s0_smoke_test 无限挂起：5 根因 (Os_Cfg 配置表未链接 / macOS POSIX port 栈尺寸 / tick 线程标志非 volatile / vTaskEndScheduler 自删 / smoke 链接集不全) 全修复 + 看门狗 + ctest TIMEOUT
  - T2 mcal_uart_test SegFault：Uart.c 38 处裸 MMIO 改 REG_* 宏 (MockHAL 可重定向)，23/23 PASS
  - T3 EthTrcv.c 空编译假 0 error：真实编译 (336B→25,864B/14 函数) + 连带修复 LinNm/LinIf/ComM_Nm/Crypto/blake2 同源缺陷
  - T4 MISRA 剩余 required 核实：官方全量扫描 1447 条 required → 68 在排除路径 + 1379 被已批准 deviations 覆盖，**业务代码 required = 0**
- **MISRA required 清零** — 10.4/12.1/20.7/19.2 批量修复 (宏体 U 后缀/括号化/变体 union 改 struct/类型双关改 memcpy) + deviation 通配收窄为文件级/规则级明细 (57 条拆分)
- **P0 批修复** — mbedTLS 挂 32KB 静态内存池 (HEAP_SIZE 256KB→4KB 回收 252KB)；CryIf/EthSm/DDS 工具级联损坏编译错误修复；bootloader 安全链单测 + 回滚记录 CRC 自包含缺陷
- **P1 批清理** — doip stub 假实现删除统一走主线；safety 模块挂载但永不编译修复 (真实编译 + 交叉构建)；Release 交叉 LTO 库 armap 修复 (gcc-ar/gcc-ranlib)；CI/文档/脚本残留引用清理
- **P2 批清理** — dds-config-tool 三份收敛 + YAML 解析器状态机缺陷修复；deviation 通配收窄；legacy 动态内存标注；web_gui systemd 硬编码去除

### Quality

- 全量 native 构建 0 error
- ctest 45/45 100% PASS
- 新增单测：TcpIp 49 / EthSwt 70 / CDD_FVM 44 / libs 30 / RTE 生成器 pytest 159 全绿
- MISRA 业务代码 required = 0

## [1.2.0] - 2026-05-26

### Added
- **Docusaurus 文档站 - Batch 1** — 迁移84篇AUTOSAR模块文档到 Docusaurus 网站
  - MCAL 驱动文档 (21篇): ADC, CAN, Crypto, DIO, ETH, LIN, MCU, PORT 等
  - ECUAL 驱动文档 (29篇): CanIf, CanTp, EthIf, DoIP, FrIf, FrTp, LinIf 等
  - Services 层模块文档 (34篇): DCM, DEM, NVM, COM, BSWM, ECUM, CSM 等
  - 文件: `website/docs/drivers/mcal/*`, `website/docs/drivers/ecual/*`, `website/docs/drivers/services/*`
- **Docusaurus 文档站 - Batch 2** — 迁移44篇安全/平台/API/指南/设计文档
  - 安全与合规 (3篇): HARA分析, 安全手册, 验证报告
  - S32K312平台 (12篇): AUTOSAR集成指南, 示例代码
  - API参考 (5篇): 通用API, COM, Crypto, DCM
  - 开发指南 (13篇): COM手册, DEM设计, MISRA, HSM
  - 设计文档 (9篇): 架构概览, 数据流, 错误处理, 测试策略
  - 文件: `website/docs/api/*`, `website/docs/design/*`, `website/docs/guides/*`, `website/docs/safety/*`, `website/docs/platform/*`
  - 侧边栏文章数: 28 → 128 篇
- **RTE 客户端-服务端调用实现** — `src/rte/src/Rte_CSOperations.c` (700行, 20个API)
  - 覆盖8个ASW组件: EngineControl, VehicleDynamics, DiagnosticManager, CommunicationManager, StorageManager, IOControl, ModeManager, WatchdogManager
  - 集成 COM 信号/NvM/BSW 接口
  - DET错误检查 + MISRA C:2012规范
- **ASW 调度集成** — `src/rte/src/Rte_AswScheduler.c` (475行)
  - 创建 Rte_AswScheduler.h/c: 组件注册表、调度器
  - 为所有8个SWC添加 MainFunction + Deinit
  - 多速率调度: 10ms/50ms/100ms 内部分发
  - 调度器集成到现有 Rte_Scheduler 框架
  - 单元测试: `tests/unit/rte/test_rte_cs_operations.c` (689行, 25个测试用例, 100%通过)
- **ComM 完整状态机** — `src/bsw/services/comm/src/ComM.c` (1189行)
  - 完整6状态: NO_COM → FULL_COM → READY_SLEEP → SILENT_COM
  - 状态转换、BusSM通知、唤醒处理 (`ComM_EvaluateWakeup`)
  - DET错误检查, MISRA C:2012, gcc零警告
- **Csm 密钥操作** — `src/bsw/services/csm/src/Csm.c` (974行)
  - `Csm_KeyGenerate`: 硬件 + CryIf + 软件回退(随机)
  - `Csm_KeyDerive`: NIST SP 800-108 KDF
  - `Csm_KeyExchangeCalcPubVal` / `Csm_KeyExchangeCalcSecret`: ECDH模拟
  - CryIf 集成 + Job 异步处理
- **Dem 时间戳支持** — `src/bsw/services/dem/legacy/`
  - `Dem_GetCurrentTimestamp()` + 全局tick计数器
  - 替换所有5处TODO: 时间戳
  - `DEM_NVM_WRITE_DELAY_MS` 延迟检查
- **NvM 测试启用** — `src/bsw/services/nvm/src/NvM_test.c`
  - 启用5/6个测试: WriteProt, WriteOnce, ReadAll, WriteAll, CancelJobs
  - NvM_DeInit 未实现, 该测试保持禁用

### Changed
- **Docusaurus 升级**: 3.8.0 → 3.10.1 (`website/package.json`)
- **CI/CD 工作流修复**:
  - `ci.yml`: 修复CMake构建路径 `tools/build/` → 根目录CMakeLists.txt; 移除不存在的 arm-gcc-toolchain.cmake 引用; 修复测试job: 手动gcc编译 → CMake测试基础设施
  - `deploy-docs.yml`: 切换部署源 `docs-site/` → `website/` (Docusaurus); 更新构建缓存路径和产物目录
- **项目统计更新**: 文档站侧边栏文章从28篇增加到128篇

### Fixed
- **ComM**: 状态机边界条件处理、重复调用保护、空指针检查
- **Csm**: CryIf 回调集成、Job状态管理、MISRA违规修复
- **Dem**: 替换所有5处硬编码TODO时间戳为全新 `Dem_GetCurrentTimestamp()` 实现
- **NvM**: 根据 `NvM.c` 实际行为调整测试逻辑, 修复测试断言

## [1.1.0] - 2026-04-29

### Added
- **MCAL 层新驱动 (3个)**
  - Eth (以太网驱动, Module ID: 0x53) - 支持 10/100/1000 Mbps 以太网 MAC 操作
  - Icu (输入捕获驱动, Module ID: 0x10) - 支持边沿检测、时间戳、信号测量和边沿计数
  - Ocu (输出比较驱动, Module ID: 0x7A) - 支持绝对/相对阈值设置和引脚动作控制
- **ECUAL 层新模块 (1个)**
  - FrTp (FlexRay 传输协议, Module ID: 0x2D) - 支持 ISO TP 分段传输协议
- **完善的车载网络支持**
  - CAN (Can, CanIf, CanTp) - 已完成
  - FlexRay (FrIf, FrTp) - 已完成
  - Ethernet (Eth, EthIf) - 已完成
  - LIN (LinIf) - 已完成
- **MISRA C:2012 合规性验证**
  - 所有新模块通过 MISRA C:2012 规范检查
  - 高达 98% 以上的代码覆盖率

### Technical Details

#### Eth (以太网驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 10/100/1000 Mbps 操作速率
- 提供 MII/RMII 接口支持
- 支持 MAC 地址过滤器配置
- 支持发送/接收中断处理

#### Icu (输入捕获驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 4 种测量模式:
  - 信号边沿检测 (Signal Edge Detection)
  - 信号测量 (Signal Measurement)
  - 时间戳 (Timestamp)
  - 边沿计数 (Edge Counter)
- 支持唤醒功能 (Wakeup)

#### Ocu (输出比较驱动)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 4 种引脚动作:
  - SET_HIGH - 比较匹配时置高
  - SET_LOW - 比较匹配时置低
  - TOGGLE - 比较匹配时翻转
  - HOLD - 保持当前状态
- 支持绝对和相对阈值设置
- ASIL-D 安全等级兼容

#### FrTp (FlexRay 传输协议)
- 基于 AutoSAR Classic Platform 4.4.0 标准
- 支持 ISO TP 分段传输:
  - 单帧 (Single Frame, SF)
  - 首帧 (First Frame, FF)
  - 连续帧 (Consecutive Frame, CF)
  - 流量控制 (Flow Control, FC)
- 支持多连接管理
- 可配置的超时管理 (N_As, N_Bs, N_Cs, N_Ar, N_Br, N_Cr)

### Changed
- 更新项目统计: 模块总数从 32 个增加到 36 个
- MCAL 层驱动从 9 个增加到 12 个
- ECUAL 层模块从 9 个增加到 10 个

### Project Statistics
| 层级 | 模块数 | 状态 |
|:-----|:-------|:-----|
| MCAL | 12 | ✅ 完成 |
| ECUAL | 10 | ✅ 完成 |
| Service | 5 | ✅ 完成 |
| RTE | 1 | ✅ 完成 |
| ASW | 8 | ✅ 完成 |
| **总计** | **36** | **✅ 完成** |

## [1.0.0] - 2026-04-23

### Added
- 完整MCAL层驱动 (ADC, CAN, DIO, GPT, MCU, PORT, PWM, SPI, WDG)
- ECUAL层框架 (CanIf, CanTp, Ea, Fee)
- Services层框架 (COM, DCM, DEM, NVM)
- 130+单元测试用例
- 5层Mock系统
- Python构建系统
- Docusaurus文档站
- GitHub Actions CI/CD
- 初始发布
- Basic BSW structure
- Core MCAL drivers
- Test framework foundation
