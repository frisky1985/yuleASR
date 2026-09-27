# 差距矩阵执行结果（2026-09-26）

> 口径：**目录级覆盖**（目录存在 ≠ 功能等价/规范符合度）
> 本地根：`/Users/ingeek/workspace/AUTOSAR/src/bsw/`
> 基线快照：EasyXMen GitHub 镜像 `easyxmen/EasyXMen` @ 2026-04-30（镜像同步时点），BSWCode 52 条目

## 一、EasyXMen BSWCode → 本地映射（52/52 覆盖）

| # | EasyXMen 模块 | 本地对应 | 状态 |
|:--|:-------------|:---------|:-----|
| 1 | BswM | services/bswm | ✔ |
| 2 | CDD_FVM | cdd/Cdd_Fvm（+ Cdd_Fvm_Hw） | ✔ |
| 3 | CanIf | ecual/canif | ✔ |
| 4 | CanNm | ecual/canNm | ✔ |
| 5 | CanSM | services/cansm | ✔ |
| 6 | CanTSyn | services/cantsyn | ✔ |
| 7 | CanTp | ecual/cantp | ✔ |
| 8 | CanTrcv | ecual/cantrcv | ✔ |
| 9 | Com | services/com（另存 classic/com 早期实现） | ✔ |
| 10 | ComM | services/comM | ✔ |
| 11 | CommonInclude | ecual/include + include/autosar + src/common | ≈ 容器映射 |
| 12 | CryIf | services/cryif | ✔ |
| 13 | Crypto_62 | mcal/crypto（层级差异：本地置于 MCAL） | ✔ |
| 14 | Csm | services/csm | ✔ |
| 15 | Dcm | services/dcm | ✔ |
| 16 | Dem | services/dem | ✔ |
| 17 | Det | services/det | ✔ |
| 18 | Dlt | services/dlt | ✔ |
| 19 | DoIP | services/doip + ecual/doIP | ✔ |
| 20 | Ea | ecual/ea | ✔ |
| 21 | EcuM | services/ecum | ✔ |
| 22 | Eep_62 | mcal/eep | ✔ |
| 23 | EthIf | ecual/ethif | ✔ |
| 24 | EthSM | ecual/ethSm + services/ethsm | ✔ |
| 25 | EthSwt | ecual/ethswt | ✔ |
| 26 | EthTSyn | services/ethtsyn | ✔ |
| 27 | EthTrcv_62 | ecual/ethtrcv | ✔ |
| 28 | Fee | ecual/fee + mcal/fee | ✔ |
| 29 | FiM | ecual/fim + services/fim | ✔ |
| 30 | FlsTst | services/flstst | ✔ |
| 31 | IpduM | ecual/ipdum + services/ipdum | ✔ |
| 32 | KeyM | services/keym | ✔ |
| 33 | LdCom | services/ldcom | ✔ |
| 34 | Libraries | services/{crc, e2e, mem, ramsafety} | ≈ 容器映射 |
| 35 | LinIf | ecual/linif | ✔ |
| 36 | LinSM | ecual/linSM + services/linsm + services/linm | ✔ |
| 37 | MemIf | ecual/memif + services/memif | ✔ |
| 38 | Nm | services/nm | ✔ |
| 39 | NvM | services/nvm | ✔ |
| 40 | PduR | services/pdur | ✔ |
| 41 | RamTst | services/ramtst + mcal/ramtst | ✔ |
| 42 | SOMEIPTP | services/someiptp | ✔ |
| 43 | Sd | services/sd（+ ecual/someipsd） | ✔ |
| 44 | SecOC | services/secoc | ✔ |
| 45 | SoAd | services/soad | ✔ |
| 46 | StbM | services/stbm | ✔ |
| 47 | TcpIp | services/tcpip | ✔ |
| 48 | Tm | services/tm | ✔ |
| 49 | UdpNm | services/udpNm | ✔ |
| 50 | WdgIf | ecual/wdgif | ✔ |
| 51 | WdgM | services/wdgm | ✔ |
| 52 | Xcp | ecual/xcp + services/xcp | ✔ |

**缺口：无。** 52/52 目录级覆盖（2 条为容器概念映射：CommonInclude、Libraries）。

## 二、本地超出基线（EasyXMen BSWCode 无对应）

| 类别 | 本地模块 | 说明 |
|:-----|:---------|:-----|
| J1939 栈 | services/j1939nm、ecual/j1939tp + services/j1939tp | EasyXMen 未开源 J1939 |
| FlexRay | ecual/frif、ecual/frtp | EasyXMen 无 FlexRay |
| LIN 扩展 | ecual/linTp、ecual/lintrcv、services/lntm | EasyXMen BSWCode 仅 LinIf/LinSM |
| SOME/IP 扩展 | services/someip、services/someipxf、ecual/someipif、ecual/someipsd | EasyXMen 仅 SOMEIPTP + Sd |
| 以太网扩展 | services/mqtt | EasyXMen 无 MQTT |
| 基础库/系统 | services/e2e、services/crc、services/schm、services/ecuC、services/swc | EasyXMen 归入 Libraries/OS/工具链 |
| 安全扩展 | services/ramsafety、cdd/{Cdd_Hsm, Cdd_Lockstep, Cdd_RamEcc} | 超出 CDD_FVM 的安全机制集 |
| **完整 MCAL（21）** | mcal/{adc, can, crypto, dio, eep, eth, fee, flash, fls, gpt, i2c, icu, lin, mcu, ocu, port, pwm, ramtst, spi, uart, wdg} | **所有开源基线均不含 MCAL**（EasyXMen Drivers/ 为空，依赖芯片厂商） |
| Bootloader | boot/ | 双方均超 BSW 范围 |

## 三、统计

- 本地模块目录：MCAL 21 + ECUAL 28 + Services 52 + OS 1 + boot/cdd ≈ **102+**（含少量双层条目）
- EasyXMen BSWCode 有效模块条目：52（其中 2 容器）
- **覆盖：52/52（目录级）**；缺口：0；本地超出：8 大类

## 四、其他基线速览

- **Eclipse OpenBSW**（Apache-2.0，活跃）：现代 C++14/17，覆盖 can/diag(DoIP,UDS)/someip/lwip/logging/lifecycle；**非严格 AUTOSAR 符合**，适合对比工程实践（构建、测试、CI），不适合对比规范符合度。
- **openAUTOSAR/classic-platform**（GPL-2.0，2024-08 后停滞）：Arctic Core 分叉，AUTOSAR 4.0 时代，仅架构考古参考；GPL 许可禁止进入商业产品。

## 五、局限声明

1. 目录级覆盖 ≠ 功能等价：本地部分模块为轻量实现（此前 P0-1/P0-4 已修复多处此类缺陷）。
2. EasyXMen 为 16 年迭代产品，含安全流程资产（ISO 26262 相关文档/测试）；本矩阵未对比流程资产。
3. 快照时点：EasyXMen 镜像 2026-04-30；建议每次重跑时刷新（见 SKILL.md 执行步骤）。
4. 许可提示：EasyXMen LGPL-2.1、Arctic Core GPL-2.0——本对标仅用于比较，未引入任何基线代码。
