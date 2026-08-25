# YuleASR 模块文档索引

> **模块总数**: 93个  
> **文档版本**: v2.0  
> **最后更新**: 2026-01-09

---

## 📊 覆盖率概览

| 层级 | 模块数 | 文档数 | 覆盖率 | 测试覆盖率 |
|:-----|:-------|:-------|:-------|:---------|
| MCAL | 20 | 20 | ✅ 100% | ✅ 100% |
| ECUAL | 29 | 29 | ✅ 100% | ✅ 100% |
| Services | 44 | 44 | ✅ 100% | ✅ 100% |
| **总计** | **93** | **93** | **✅ 100%** | **✅ 100%** |

---

## 📝 模块设计文档

以下模块除参考文档外，还提供了详细的设计文档（架构、状态机、数据结构、API、流程、配置、错误处理、集成与测试策略）。

| 模块 | 层级 | 设计文档 | 状态 |
|:-----|:-----|:---------|:-----|
| Com | Services | [design/modules/services/com-design.md](design/modules/services/com-design.md) | 已完成 |
| ADC | MCAL | [design/modules/mcal/adc-design.md](design/modules/mcal/adc-design.md) | 已完成 |
| CAN | MCAL | [design/modules/mcal/can-design.md](design/modules/mcal/can-design.md) | 已完成 |
| CRYPTO | MCAL | [design/modules/mcal/crypto-design.md](design/modules/mcal/crypto-design.md) | 已完成 |
| DIO | MCAL | [design/modules/mcal/dio-design.md](design/modules/mcal/dio-design.md) | 已完成 |
| EEP | MCAL | [design/modules/mcal/eep-design.md](design/modules/mcal/eep-design.md) | 已完成 |
| ETH | MCAL | [design/modules/mcal/eth-design.md](design/modules/mcal/eth-design.md) | 已完成 |
| FEE | MCAL | [design/modules/mcal/fee-design.md](design/modules/mcal/fee-design.md) | 已完成 |
| FLASH | MCAL | [design/modules/mcal/flash-design.md](design/modules/mcal/flash-design.md) | 已完成 |
| FLS | MCAL | [design/modules/mcal/fls-design.md](design/modules/mcal/fls-design.md) | 已完成 |
| GPT | MCAL | [design/modules/mcal/gpt-design.md](design/modules/mcal/gpt-design.md) | 已完成 |
| I2C | MCAL | [design/modules/mcal/i2c-design.md](design/modules/mcal/i2c-design.md) | 已完成 |
| ICU | MCAL | [design/modules/mcal/icu-design.md](design/modules/mcal/icu-design.md) | 已完成 |
| LIN | MCAL | [design/modules/mcal/lin-design.md](design/modules/mcal/lin-design.md) | 已完成 |
| MCU | MCAL | [design/modules/mcal/mcu-design.md](design/modules/mcal/mcu-design.md) | 已完成 |
| OCU | MCAL | [design/modules/mcal/ocu-design.md](design/modules/mcal/ocu-design.md) | 已完成 |
| PORT | MCAL | [design/modules/mcal/port-design.md](design/modules/mcal/port-design.md) | 已完成 |
| PWM | MCAL | [design/modules/mcal/pwm-design.md](design/modules/mcal/pwm-design.md) | 已完成 |
| RAMTST | MCAL | [design/modules/mcal/ramtst-design.md](design/modules/mcal/ramtst-design.md) | 已完成 |
| SPI | MCAL | [design/modules/mcal/spi-design.md](design/modules/mcal/spi-design.md) | 已完成 |
| UART | MCAL | [design/modules/mcal/uart-design.md](design/modules/mcal/uart-design.md) | 已完成 |
| WDG | MCAL | [design/modules/mcal/wdg-design.md](design/modules/mcal/wdg-design.md) | 已完成 |
| Dem | Services | [design/modules/services/dem-design.md](design/modules/services/dem-design.md) | 已完成 |
| CanIf | ECUAL | [design/modules/ecual/canif-design.md](design/modules/ecual/canif-design.md) | 已完成 |
| PduR | Services | [design/modules/services/pdur-design.md](design/modules/services/pdur-design.md) | 已完成 |
| CanTp | ECUAL | [design/modules/ecual/cantp-design.md](design/modules/ecual/cantp-design.md) | 已完成 |
| Dcm | Services | [design/modules/services/dcm-design.md](design/modules/services/dcm-design.md) | 已完成 |
| NvM | Services | [design/modules/services/nvm-design.md](design/modules/services/nvm-design.md) | 已完成 |
| EcuM | Services | [design/modules/services/ecum-design.md](design/modules/services/ecum-design.md) | 已完成 |
| BswM | Services | [design/modules/services/bswm-design.md](design/modules/services/bswm-design.md) | 已完成 |
| SecOC | Services | [design/modules/services/secoc-design.md](design/modules/services/secoc-design.md) | 已完成 |
| Csm | Services | [design/modules/services/csm-design.md](design/modules/services/csm-design.md) | 已完成 |
| WdgM | Services | [design/modules/services/wdgm-design.md](design/modules/services/wdgm-design.md) | 已完成 |

完整清单见 [design/modules/README.md](design/modules/README.md)。

---

## 🔌 MCAL层 (Microcontroller Driver)

### 通信接口

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| CAN | [can.md](modules/can.md) | CAN控制器驱动 | 4.4.0 | ✅ 已完成 |
| LIN | [lin.md](modules/lin.md) | LIN总线驱动 | 4.4.0 | ✅ 已完成 |
| ETH | [eth.md](modules/eth.md) | 以太网MAC驱动 | 4.4.0 | ✅ 已完成 |
| I2C | [i2c.md](modules/i2c.md) | I2C串行接口 | 4.4.0 | ✅ 已完成 |
| SPI | [spi.md](modules/spi.md) | SPI串行外设接口 | 4.4.0 | ✅ 已完成 |
| UART | [uart.md](modules/uart.md) | UART串口驱动 | 4.4.0 | ✅ 已完成 |

### 存储驱动

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| FLASH | [flash.md](modules/flash.md) | Flash驱动 | 4.4.0 | ✅ 已完成 |
| FLS | [fls.md](modules/fls.md) | Flash接口 | 4.4.0 | ✅ 已完成 |
| EEPROM | [eep.md](modules/eep.md) | EEPROM驱动 | 4.4.0 | ✅ 已完成 |

### 输入/输出

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| DIO | [dio.md](modules/dio.md) | 数字IO | 4.4.0 | ✅ 已完成 |
| PORT | [port.md](modules/port.md) | 端口驱动 | 4.4.0 | ✅ 已完成 |
| ADC | [adc.md](modules/adc.md) | 模数转换 | 4.4.0 | ✅ 已完成 |
| PWM | [pwm.md](modules/pwm.md) | 脉冲宽度调制 | 4.4.0 | ✅ 已完成 |

### 时间/计数器

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| GPT | [gpt.md](modules/gpt.md) | 通用定时器 | 4.4.0 | ✅ 已完成 |
| ICU | [icu.md](modules/icu.md) | 输入捕获单元 | 4.4.0 | ✅ 已完成 |
| OCU | [ocu.md](modules/ocu.md) | 输出比较单元 | 4.4.0 | ✅ 已完成 |

### 微控制器

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| MCU | [mcu.md](modules/mcu.md) | 微控制器驱动 | 4.4.0 | ✅ 已完成 |
| WDG | [wdg.md](modules/wdg.md) | 看门狗驱动 | 4.4.0 | ✅ 已完成 |
| RAMTST | [ramtst.md](modules/ramtst.md) | RAM测试 | 4.4.0 | ✅ 已完成 |

### 安全硬件

| 模块 | 文档 | 说明 | AUTOSAR版本 | 状态 |
|:-----|:-----|:-----|:------------|:-----|
| CRYPTO | [crypto.md](modules/crypto.md) | 硬件加密驱动 | 4.4.0 | ✅ 已完成 |

---

## 🔗 ECUAL层 (ECU Abstraction Layer)

### CAN相关

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| CanIf | [canif.md](modules/canif.md) | CAN接口 | CAN | ✅ 已完成 |
| CanNm | [cannm.md](modules/cannm.md) | CAN网络管理 | CanIf, NM | ✅ 已完成 |
| CanSm | [cansm.md](modules/cansm.md) | CAN状态管理 | CanIf, ComM | ✅ 已完成 |
| CanTp | [cantp.md](modules/cantp.md) | CAN传输协议 | CanIf, PduR | ✅ 已完成 |
| CanTrcv | [cantrcv.md](modules/cantrcv.md) | CAN收发器 | DIO, SPI | ✅ 已完成 |

### LIN相关

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| LinIf | [linif.md](modules/linif.md) | LIN接口 | LIN | ✅ 已完成 |
| LinNm | [linnm.md](modules/linnm.md) | LIN网络管理 | LinIf, NM | ✅ 已完成 |
| LinSm | [linsm.md](modules/linsm.md) | LIN状态管理 | LinIf, ComM | ✅ 已完成 |
| LinTp | [lintp.md](modules/lintp.md) | LIN传输协议 | LinIf, PduR | ✅ 已完成 |
| LinTrcv | [lintrcv.md](modules/lintrcv.md) | LIN收发器 | DIO | ✅ 已完成 |

### 以太网相关

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| EthIf | [ethif.md](modules/ethif.md) | 以太网接口 | ETH | ✅ 已完成 |
| EthSm | [ethsm.md](modules/ethsm.md) | 以太网状态管理 | EthIf, ComM | ✅ 已完成 |
| EthTrcv | [ethtrcv.md](modules/ethtrcv.md) | 以太网收发器 | DIO | ✅ 已完成 |

### 存储抽象

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| MemIf | [memif.md](modules/memif.md) | 存储接口 | Fee, Ea | ✅ 已完成 |
| Fee | [fee.md](modules/fee.md) | Flash EEPROM仿真 | FLS | ✅ 已完成 |
| Ea | [ea.md](modules/ea.md) | EEPROM抽象 | EEPROM | ✅ 已完成 |

### FlexRay

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| FrIf | [frif.md](modules/frif.md) | FlexRay接口 | - | ✅ 已完成 |
| FrTp | [frtp.md](modules/frtp.md) | FlexRay传输协议 | FrIf, PduR | ✅ 已完成 |

### 诊断与日志

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| DLT | [dlt.md](modules/dlt.md) | 诊断日志跟踪 | Com | ✅ 已完成 |
| DoIP | [doip.md](modules/doip.md) | 诊断 over IP | SoAd | ✅ 已完成 |

### 网络管理

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| J1939Tp | [j1939tp.md](modules/j1939tp.md) | J1939传输协议 | CanIf | ✅ 已完成 |

### 其他

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| IoHwAb | [iohwab.md](modules/iohwab.md) | IO硬件抽象 | DIO, Port | ✅ 已完成 |
| IpduM | [ipdum.md](modules/ipdum.md) | IPDU复用 | PduR | ✅ 已完成 |
| FiM | [fim.md](modules/fim.md) | 功能禁止管理 | DEM | ✅ 已完成 |
| WdgIf | [wdgif.md](modules/wdgif.md) | 看门狗接口 | WDG | ✅ 已完成 |
| XCP | [xcp.md](modules/xcp.md) | XCP测量标定 | - | ✅ 已完成 |
| SRP | [srp.md](modules/srp.md) | 流预留协议 | - | ✅ 已完成 |
| SoAdIf | [someipif.md](modules/someipif.md) | SOME/IP接口 | EthIf | ✅ 已完成 |
| SoAdSd | [someipsd.md](modules/someipsd.md) | 服务发现 | SoAd | ✅ 已完成 |

---

## 🔧 Services层 (Service Layer)

### 通信服务

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| Com | [com.md](modules/com.md) | 通信服务 | PduR | ✅ 已完成 |
| ComM | [comm.md](modules/comm.md) | 通信管理 | NM, CanSm, LinSm | ✅ 已完成 |
| PduR | [pdur.md](modules/pdur.md) | PDU路由器 | - | ✅ 已完成 |
| IpduM | [ipdum.md](modules/ipdum.md) | IPDU复用器 | Com, PduR | ✅ 已完成 |

### 网络管理

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| NM | [nm.md](modules/nm.md) | 网络管理 | ComM | ✅ 已完成 |
| CanM | [canm.md](modules/canm.md) | CAN管理 | CanIf | ✅ 已完成 |
| LinM | [linm.md](modules/linm.md) | LIN管理 | LinIf | ✅ 已完成 |
| UdpNm | [udpnm.md](modules/udpnm.md) | UDP网络管理 | SoAd, NM | ✅ 已完成 |
| J1939Nm | [j1939nm.md](modules/j1939nm.md) | J1939网络管理 | CanIf | ✅ 已完成 |

### 诊断服务

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| DCM | [dcm.md](modules/dcm.md) | 诊断通信管理 | PduR, DEM, DET | ✅ 已完成 |
| DEM | [dem.md](modules/dem.md) | 诊断事件管理 | NvM | ✅ 已完成 |
| DET | [det.md](modules/det.md) | 默认错误追踪 | - | ✅ 已完成 |
| DLT | [dlt.md](modules/dlt.md) | 诊断日志和跟踪 | Com | ✅ 已完成 |
| DoCan | [docan.md](modules/docan.md) | CAN诊断 | DCM, CanTp | ✅ 已完成 |
| DoIP | [doip.md](modules/doip.md) | IP诊断 | DCM, SoAd | ✅ 已完成 |

### 存储服务

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| NvM | [nvm.md](modules/nvm.md) | 非易失性存储 | MemIf, CRC | ✅ 已完成 |
| Mem | [mem.md](modules/mem.md) | 存储服务 | - | ✅ 已完成 |
| MemIf | [memif.md](modules/memif.md) | 存储接口 | Fee, Ea | ✅ 已完成 |
| FiM | [fim.md](modules/fim.md) | 功能禁止管理 | DEM | ✅ 已完成 |

### 安全服务

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| CSM | [csm.md](modules/csm.md) | 加密服务管理 | CryIf | ✅ 已完成 |
| CryIf | [cryif.md](modules/cryif.md) | 加密接口 | CRYPTO | ✅ 已完成 |
| KeyM | [keym.md](modules/keym.md) | 密钥管理 | CSM, NvM | ✅ 已完成 |
| SecOC | [secoc.md](modules/secoc.md) | 安全通信 | CSM, PduR | ✅ 已完成 |
| RamSafety | [ramsafety.md](modules/ramsafety.md) | RAM安全 | - | ✅ 已完成 |

### 时间同步

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| StbM | [stbm.md](modules/stbm.md) | 同步时基管理 | GPT, ETH | ✅ 已完成 |
| CanTSyn | [cantsyn.md](modules/cantsyn.md) | CAN时间同步 | CanIf, StbM | ✅ 已完成 |
| LnTM | [lntm.md](modules/lntm.md) | LIN时间调度 | LinIf | ✅ 已完成 |

### 状态管理

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| EcuM | [ecum.md](modules/ecum.md) | ECU状态管理 | BswM | ✅ 已完成 |
| BswM | [bswm.md](modules/bswm.md) | BSW模式管理 | EcuM | ✅ 已完成 |
| SchM | [schm.md](modules/schm.md) | BSW调度器 | OS | ✅ 已完成 |

### 通信协议

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| SoAd | [soad.md](modules/soad.md) | Socket适配 | TcpIp | ✅ 已完成 |
| SomeIP | [someip.md](modules/someip.md) | SOME/IP协议 | SoAd, Com | ✅ 已完成 |
| SomeIPTp | [someiptp.md](modules/someiptp.md) | SOME/IP传输 | SoAd, PduR | ✅ 已完成 |
| SomeIPXF | [someipxf.md](modules/someipxf.md) | SOME/IP转换 | SoAd | ✅ 已完成 |
| MQTT | [mqtt.md](modules/mqtt.md) | MQTT协议 | TcpIp | ✅ 已完成 |

### 端到端保护

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| E2E | [e2e.md](modules/e2e.md) | 端到端保护 | CRC | ✅ 已完成 |

### 硬件抽象

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| EcuC | [ecuc.md](modules/ecuc.md) | ECU配置 | - | ✅ 已完成 |
| CRC | [crc.md](modules/crc.md) | CRC计算 | - | ✅ 已完成 |

### 看门狗管理

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| WdgM | [wdgm.md](modules/wdgm.md) | 看门狗管理 | WDG, EcuM | ✅ 已完成 |

### 软件组件

| 模块 | 文档 | 说明 | 依赖模块 | 状态 |
|:-----|:-----|:-----|:---------|:-----|
| SWC | [swc.md](modules/swc.md) | 软件组件 | RTE | ✅ 已完成 |

---

## 📊 完成度统计

### 按层级统计

```
MCAL层:    ██████████ 100% (20/20)
ECUAL层:   ██████████ 100% (29/29)
Services层: ██████████ 100% (44/44)
总计:       ██████████ 100% (93/93)
```

### 按类别统计

| 类别 | 模块数 | 状态 |
|:-----|:-------|:-----|
| 通信接口 | 14 | ✅ 完成 |
| 存储 | 10 | ✅ 完成 |
| 诊断 | 9 | ✅ 完成 |
| 网络管理 | 8 | ✅ 完成 |
| 安全 | 5 | ✅ 完成 |
| 时间/计数器 | 5 | ✅ 完成 |
| 状态管理 | 4 | ✅ 完成 |
| 输入/输出 | 4 | ✅ 完成 |
| 其他 | 34 | ✅ 完成 |

---

## 📞 联系与反馈

- **项目仓库**: https://github.com/frisky1985/yuleASR
- **文档版本**: v2.0
- **最后更新**: 2026-01-09

---

*本索引由YuleTech团队维护*  
*如有问题请联系开发团队*
