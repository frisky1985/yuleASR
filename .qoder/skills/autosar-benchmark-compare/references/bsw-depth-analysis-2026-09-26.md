# BSW 深度对标与量产差距分析（2026-09-26）

> 主基线：开源小满 EasyXMen（普华基础软件，LGPL-2.1，GitHub 镜像 easyxmen/EasyXMen@master，pushed 2026-04-30）
> 参照：Eclipse OpenBSW（Elektrobit，Apache-2.0，非严格 AUTOSAR）
> 前置结论：目录级覆盖 52/52（见 `gap-matrix-2026-09-26.md`）。本报告回答"深度是否够量产"。
> 数据可复现：`scripts/scan-module-metrics.js`、`scripts/api-surface-diff.js`，原始数据见同目录 `api-symbol-diff-2026-09-26.txt`、`easyxmen-header-scan-2026-09-26.txt`。

---

## 0. 一句话结论

目录覆盖已满，但深度扫描显示：**CAN/诊断/存储主干具备可用深度，BswM、LinIf 为最小实现（量产风险），CAN 通信栈（CanIf/CanTp/CanSM）与 Com/Dem/Crypto 存在成体系的能力面差距；另有双 Com 实现、跨层重复模块、legacy 残留等结构性债务**。按量产标准需先关闭 §4 中 P0 项。

---

## 1. 数据总览

| 口径 | 我方（本地仓） | EasyXMen（开源仓） |
|:-----|:---------------|:-------------------|
| BSW 代码量 | mcal 47.8k / ecual 37.6k / services 116.0k / os 3.5k / cdd 5.1k / classic 6.3k LOC | BSWCode 623 文件 15.0MB；RTOS 85 文件 1.8MB |
| MCAL | 21 模块全自研（≈48k LOC） | **不开放（Drivers/ 为空）** |
| 测试 | tests/bsw 113 文件 ≈35k LOC，ctest 104 项 | **开源仓不含测试（Test/ 为空）** |
| 关键模块头文件面 | 逐模块见 §3（本次抓取 EMX 29 模块 183 头文件 / 83.3k LOC / 942 声明符号） | 同左 |
| 工程基建（参照 OpenBSW） | — | Apache-2.0；doc 6.1MB、test 43 文件、bazel/cmake 双构建 |

口径说明：
- "声明符号"= 头文件中函数原型/回调 typedef/枚举常量（已剔除 `#define` 宏噪声；多行宏续行可能残留少量）。
- EMX 仓量（KB）含 .c/.h/配置，作为量级参考（≈25 行/KB）；我方为 .c+.h 实际行数。
- services 116k LOC 含 `mqtt/include/mbedtls` 等 vendored 代码，非全部自研。

---

## 2. 方法

1. 本地：脚本扫描各层模块的 .c/.h 数、LOC、头文件声明符号数。
2. EMX：`git/trees?recursive=1` 一次拉取全仓清单；下载 29 个关键模块全部头文件（<250KB），同口径扫描。
3. 逐模块符号面 diff（EMX 独有 / 我方独有），结合代码量级给出判定。
4. 局限：未下载 EMX 的 .c（无法对比实现细节/代码质量）；符号面差异不等于功能符合性；判定为工程经验分级，量产决策需结合项目需求。

---

## 3. 关键模块对照表（按差距排序）

| 模块 | 我方实现 | EMX 仓量 / 头面 | 符号差（EMX独/我独） | 判定 | 关键差异 |
|:-----|:---------|:----------------|:---------------------|:-----|:---------|
| BswM | 292 LOC / 5 API | 40f 399KB / 3490LOC 33符 | 115 / 4 | **显著差距** | 无 Rule/ActionList 引擎、仲裁、分区通知；仅模式存取最小面 |
| LinIf | 361 LOC / 9 API | 18f 378KB / 2730LOC 54符 | 133 / 4 | **显著差距** | 缺 schedule 引擎（ChannelMainFunction）、唤醒、事件清除、取消传输 |
| CanIf | 1.4k LOC / 11 API | 10f 236KB / 2439LOC 25符 | 76 / 2 | 中等-显著 | 缺 Trcv 唤醒标志处理、CanId 元数据转换等；PN 相关面缺失 |
| CanTp | 1.5k LOC / 9 API | 5f 193KB / 1455LOC 24符 | 35 / 1 | 中等 | EMX 含 Rx 队列、填充组装、FF-DL 校验等内部机制 |
| Dem | 12.6k LOC / 77 API | 37f 1438KB / 39887LOC 231符 | 1244 / 129 | 中等-显著 | EMX 含 J1939 DTC、卫星事件、大量内部机制；按 OEM 诊断需求定档 |
| Crypto | 10.0k LOC / 44 API | 39f 1024KB / 6077LOC 162符 | 252 / 83 | 中等 | 算法族与软件回退路径差异大（EMX 含 3DES/大量内联原语） |
| Csm | 6.5k LOC / 25 API | 8f 185KB / 2308LOC 27符 | 46 / 13 | 中等 | 缺 AEAD（Encrypt/Decrypt）服务族 |
| ComM | 1.7k LOC / 11 API | 15f 155KB / 1571LOC 20符 | 52 / 14 | 中等 | 缺诊断通道管理、PN/通信允许查询类 API |
| Com | classic 6.3k(43API) + services 2.3k(16API) | 9f 654KB / 3356LOC 32符 | 93/80 ; 73/3 | 中等（结构性） | **双实现并存**；EMX 暴露 TP Copy 接口族 |
| PduR | 1.9k LOC / 11 API | 9f 171KB / 1472LOC 28符 | 66 / 27 | 相当 | 差异多在内部配置类型；我方含取消/DoIP 专属路径 |
| EcuM | 2.5k LOC / 26 API | 11f 202KB / 1745LOC 38符 | 42 / 27 | 相当（设计差异） | AL 分区设计不同；我方含睡眠/唤醒校验链 |
| NvM | 6.4k LOC / 27 API | 8f 347KB / 1982LOC 31符 | 52 / 25 | 相当-择优 | 我方含 ECC 处理器；EMX 有 mirror 重复操作 |
| Dcm | 22.7k LOC / 148 API | 50f 1008KB / 5071LOC 86符 | 135 / 332 | 相当-择优 | 我方含 cache/DDS/动态 DID；EMX 内部计时/安全索引助手 |
| StbM | 1.4k LOC / 11 API | 4f 117KB / 850LOC 14符 | 7 / 4 | 相当-略缺 | 缺 RateDeviation/TimeLeap/SyncRecord 类 API |
| CanSM | 0.9k LOC / 8 API | 4f 179KB / 714LOC 7符 | 6 / 6 | 相当-略缺 | 缺 PN（ConfirmPnAvailability）/NetworkPassive |
| Nm | 0.75k LOC / 16 API | 4f 131KB / 1102LOC 18符 | 16 / 10 | 相当-略缺 | 缺 CarWakeUp/CoordReadyToSleep 等网关场景 API |
| CanNm | 2.1k LOC / 14 API | 4f 128KB / 909LOC 16符 | 6 / 0 | 相当-略缺 | 个别 Getter/远程睡眠 API |
| LdCom | 0.17k LOC / 6 API | 5f 50KB / 629LOC 10符 | 12 / 4 | 略缺 | 我方更精简，含分段进度 API |
| WdgM | 1.95k LOC / 16 API | 5f 148KB / 928LOC 12符 | 7 / 14 | 相当-择优 | 我方含 SE 激活/锁步错误处理 |
| UdpNm | 1.8k LOC / 17 API | 4f 119KB / 837LOC 15符 | 6 / 12 | 相当 | 回调命名差异 |
| IpduM | 0.97k LOC / 6 API | 3f 170KB / 733LOC 5符 | 2 / 5 | 相当 | EMX 拆分 MainFunctionRx/Tx |
| FiM | 1.3k LOC / 7 API | 6f 76KB / 425LOC 7符 | 3 / 3 | 相当 | — |
| LinSM | 0.8k LOC / 7 API | 3f 50KB / 464LOC 7符 | 3 / 4 | 相当 | 命名差异（WakeUpConfiguration vs WakeupConfirmation） |
| Fee | 3.1k LOC / 17 API | 5f 125KB / 684LOC 12符 | 5 / 21 | 相当-择优 | 符号面基本闭合 |
| Ea | 0.87k LOC / 12 API | 4f 62KB / 412LOC 10符 | 4 / 4 | 相当 | — |
| MemIf | 0.72k LOC / 9 API | 3f 25KB / 310LOC 7符 | 0 / 2 | 相当 | **符号面完全闭合** |
| Det | 1.0k LOC / 6 API | 3f 33KB / 409LOC 5符 | 0 / 1 | 相当 | **符号面完全闭合** |
| EthSM | 0.29k LOC / 6 API | 4f 50KB / 333LOC 6符 | 2 / 3 | 相当 | — |
| OS/RTE | os 3.5k LOC 胶水（FreeRTOS V11.1.0 vendored） | RTOS 85f 1.8MB（自研） | — | 依赖第三方 | 非安全认证分支；量产需评估认证版或 MPU 方案 |

---

## 4. 量产差距清单

### P0（量产前必须处置）

1. **BswM 决策与定档**：现为 292 行最小实现。若整车需求涉及通信控制（PDU Group）、唤醒/睡眠状态仲裁、EcuM 联动，必须补 Rule/ActionList 引擎（对标 EMX 面）；若不需要，须以需求文档书面锁定简化设计并建立追溯。
2. **LinIf 加厚**（窗口控制器多 LIN 子节点场景）：补 schedule 引擎、唤醒/睡眠、事件与状态 API、节点配置；与 `lntm/lintrcv/linTp/linSM` 联动验证。
3. **双 Com 收敛**：`classic/com`（6.3k/43API，仅 include 路径引用、测试在已废弃的 tests/unit）与 `services/com`（2.3k/16API，有 tests/bsw 覆盖）并存，且互有独有符号。量产前明确唯一主线，消除配置错配风险。
4. **CAN 栈加厚与 CAN FD 确认**：CanIf 补 Trcv 唤醒标志/PN 面；CanSM 补 PN API；CanTp 强化内部机制（Rx 队列/填充/FD 校验）；核实 CAN/CAN FD 端到端能力与目标 ECU 网络一致。
5. **质量门禁落地**：MISRA 存量违规（Rule 11.4/13.5 backlog）、`legacy/` 目录清理（crypto/dcm/dem/xcp）、TODO/stub 归零或挂单（集中区：swc 多处 Placeholder、boot、doip、crypto）、覆盖率报告接入 CI。

### P1（按项目需求补齐）

6. Dem 按 OEM 诊断规范定档（若涉 J1939 或复杂去抖/卫星事件，差距放大）。
7. Crypto/Csm 按 SecOC/KeyM 需求补齐（AEAD 服务族、算法族与软件回退）。
8. ComM 诊断通道/PN 状态 API；StbM 时间同步增强；Nm 网关场景 API。
9. 配置工具模板扩面（当前 ConfigGenerator 覆盖 MCAL 5 + BSW 5）。

### P2（工程化完善）

10. LdCom/IpduM 细节对齐；跨层重复模块收敛（fee、memif、fim、ipdum、j1939tp、xcp、doip、ethsm、ramtst 等 6+ 对）。
11. 文档与测试基建（可借鉴 OpenBSW：doc/test 独立体系、双构建）；命名/版本宏（AR_RELEASE 等）如需第三方集成审查再对齐。

---

## 5. 我方领先项（对标中确认）

- **MCAL 全自研 21 模块 ≈48k LOC**：所有开源基线均不开放 MCAL，这是核心资产。
- **测试资产**：tests/bsw ≈35k LOC / 104 ctest（EMX 开源仓无测试代码）。
- **Dcm 深度**：22.7k LOC，含 cache/DDS/动态 DID 等特性面。
- **存储栈**：Fee/Ea/MemIf 符号面基本闭合，NvM 含 ECC 集成。
- **安全 CDD**：Hsm/Lockstep/RamEcc/ramsafety（EMX 无对应开放物）。
- **广度超出**：J1939 栈、FlexRay、SOME/IP、MQTT/TcpIp、DoIP、XCP、SD 等（见 gap-matrix 清单）。

---

## 6. 结构性风险（非模块级）

- `classic/` 与 `services/`、`ecual/` 间存在同名模块重复实现，构建/配置归属不清（已实测 classic/com 未被顶层目标直接编译引用）。
- `src/bsw/services/swc/src/Swc.c` 多处 "Placeholder implementation"（12+ 处标记）。
- 废弃测试树 `tests/unit`、`tests/unit/autosar`（@deprecated）仍在仓内。
- OS 依赖 vendored FreeRTOS V11.1.0；功能安全场景需另行评估。
- 各模块 ar-release/version 宏命名不统一（CANNN_AR_MAJOR_VERSION vs CANNM_AR_RELEASE_MAJOR_VERSION），跨模块集成审查前建议统一。

---

## 7. 原始数据与复现

- 逐模块符号 diff 全量：`api-symbol-diff-2026-09-26.txt`
- EMX 头文件扫描表：`easyxmen-header-scan-2026-09-26.txt`
- 复现：
  ```bash
  # 本地指标
  for L in mcal ecual services os cdd classic; do
    node .qoder/skills/autosar-benchmark-compare/scripts/scan-module-metrics.js src/bsw/$L; done
  # 基线 tree（一次拉全仓）
  curl -sL "https://api.github.com/repos/easyxmen/EasyXMen/git/trees/master?recursive=1" -o /tmp/emx_tree.json
  # 关键模块头文件下载（raw.githubusercontent，需 --retry）
  # 逐模块符号 diff
  node .qoder/skills/autosar-benchmark-compare/scripts/api-surface-diff.js <我方模块目录> /tmp/emx_hdrs/<模块>
  ```
