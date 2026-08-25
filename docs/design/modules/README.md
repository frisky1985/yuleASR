# AUTOSAR 模块设计文档

本目录存放 yuleASR 各 AUTOSAR 模块的详细设计文档，补充 `docs/modules/` 中已有的模块参考文档。

## 目录结构

```
docs/design/modules/
├── TEMPLATE.md           # 模块设计文档模板
├── README.md             # 本文档
├── mcal/                 # MCAL 层模块设计文档
├── ecual/                # ECUAL 层模块设计文档
└── services/             # Services 层模块设计文档
```

## 文档标准

每个设计文档应遵循 `TEMPLATE.md` 中的 14 个章节结构，包含架构、状态机、数据结构、API、流程、配置、错误处理、集成与测试策略。

## 完成进度

### MCAL 层

| 模块 | 层级 | 文件 | 状态 |
|------|------|------|------|
| ADC | MCAL | [mcal/adc-design.md](mcal/adc-design.md) | 已完成 |
| CAN | MCAL | [mcal/can-design.md](mcal/can-design.md) | 已完成 |
| CRYPTO | MCAL | [mcal/crypto-design.md](mcal/crypto-design.md) | 已完成 |
| DIO | MCAL | [mcal/dio-design.md](mcal/dio-design.md) | 已完成 |
| EEP | MCAL | [mcal/eep-design.md](mcal/eep-design.md) | 已完成 |
| ETH | MCAL | [mcal/eth-design.md](mcal/eth-design.md) | 已完成 |
| FEE | MCAL | [mcal/fee-design.md](mcal/fee-design.md) | 已完成 |
| FLASH | MCAL | [mcal/flash-design.md](mcal/flash-design.md) | 已完成 |
| FLS | MCAL | [mcal/fls-design.md](mcal/fls-design.md) | 已完成 |
| GPT | MCAL | [mcal/gpt-design.md](mcal/gpt-design.md) | 已完成 |
| I2C | MCAL | [mcal/i2c-design.md](mcal/i2c-design.md) | 已完成 |
| ICU | MCAL | [mcal/icu-design.md](mcal/icu-design.md) | 已完成 |
| LIN | MCAL | [mcal/lin-design.md](mcal/lin-design.md) | 已完成 |
| MCU | MCAL | [mcal/mcu-design.md](mcal/mcu-design.md) | 已完成 |
| OCU | MCAL | [mcal/ocu-design.md](mcal/ocu-design.md) | 已完成 |
| PORT | MCAL | [mcal/port-design.md](mcal/port-design.md) | 已完成 |
| PWM | MCAL | [mcal/pwm-design.md](mcal/pwm-design.md) | 已完成 |
| RAMTST | MCAL | [mcal/ramtst-design.md](mcal/ramtst-design.md) | 已完成 |
| SPI | MCAL | [mcal/spi-design.md](mcal/spi-design.md) | 已完成 |
| UART | MCAL | [mcal/uart-design.md](mcal/uart-design.md) | 已完成 |
| WDG | MCAL | [mcal/wdg-design.md](mcal/wdg-design.md) | 已完成 |

### Tier 1 — 核心/复杂/安全模块（12 个）

| 模块 | 层级 | 文件 | 状态 |
|------|------|------|------|
| Com | Services | [services/com-design.md](services/com-design.md) | 已完成 |
| Dem | Services | [services/dem-design.md](services/dem-design.md) | 已完成 |
| CanIf | ECUAL | [ecual/canif-design.md](ecual/canif-design.md) | 已完成 |
| PduR | Services | [services/pdur-design.md](services/pdur-design.md) | 已完成 |
| CanTp | ECUAL | [ecual/cantp-design.md](ecual/cantp-design.md) | 已完成 |
| Dcm | Services | [services/dcm-design.md](services/dcm-design.md) | 已完成 |
| NvM | Services | [services/nvm-design.md](services/nvm-design.md) | 已完成 |
| EcuM | Services | [services/ecum-design.md](services/ecum-design.md) | 已完成 |
| BswM | Services | [services/bswm-design.md](services/bswm-design.md) | 已完成 |
| SecOC | Services | [services/secoc-design.md](services/secoc-design.md) | 已完成 |
| Csm | Services | [services/csm-design.md](services/csm-design.md) | 已完成 |
| WdgM | Services | [services/wdgm-design.md](services/wdgm-design.md) | 已完成 |

### Tier 2 — 重要模块（19 个）

| 模块 | 层级 | 文件 | 状态 |
|------|------|------|------|
| Det | Services | [services/det-design.md](services/det-design.md) | 已完成 |
| CRC | Services | [services/crc-design.md](services/crc-design.md) | 已完成 |
| E2E | Services | [services/e2e-design.md](services/e2e-design.md) | 已完成 |
| CanNm | ECUAL | [ecual/cannm-design.md](ecual/cannm-design.md) | 已完成 |
| LinSM | Services | [services/linsm-design.md](services/linsm-design.md) | 已完成 |
| LinIf | ECUAL | [ecual/linif-design.md](ecual/linif-design.md) | 已完成 |
| EthIf | ECUAL | [ecual/ethif-design.md](ecual/ethif-design.md) | 已完成 |
| Mem | Services | [services/mem-design.md](services/mem-design.md) | 已完成 |
| EA | ECUAL | [ecual/ea-design.md](ecual/ea-design.md) | 已完成 |
| FrIf | ECUAL | [ecual/frif-design.md](ecual/frif-design.md) | 已完成 |
| FrTp | ECUAL | [ecual/frtp-design.md](ecual/frtp-design.md) | 已完成 |
| SomeIp | Services | [services/someip-design.md](services/someip-design.md) | 已完成 |
| DoIP | Services | [services/doip-design.md](services/doip-design.md) | 已完成 |
| DoCan | Services | [services/docan-design.md](services/docan-design.md) | 已完成 |
| SchM | Services | [services/schm-design.md](services/schm-design.md) | 已完成 |
| WdgIf | ECUAL | [ecual/wdgif-design.md](ecual/wdgif-design.md) | 已完成 |
| ComM | Services | [services/comm-design.md](services/comm-design.md) | 已完成 |
| Dlt | Services | [services/dlt-design.md](services/dlt-design.md) | 已完成 |
| StbM | Services | [services/stbm-design.md](services/stbm-design.md) | 已完成 |

### Tier 3 — 其余模块（37 个）

#### Services 层（26 个）

| 模块 | 文件 | 状态 |
|------|------|------|
| CanM | [services/canm-design.md](services/canm-design.md) | 已完成 |
| CanSM | [services/cansm-design.md](services/cansm-design.md) | 已完成 |
| CanTpSyn | [services/cantsyn-design.md](services/cantsyn-design.md) | 已完成 |
| CryIf | [services/cryif-design.md](services/cryif-design.md) | 已完成 |
| EcuC | [services/ecuc-design.md](services/ecuc-design.md) | 已完成 |
| FiM | [services/fim-design.md](services/fim-design.md) | 已完成 |
| FlsStst | [services/flstst-design.md](services/flstst-design.md) | 已完成 |
| J1939Nm | [services/j1939nm-design.md](services/j1939nm-design.md) | 已完成 |
| J1939Tp | [services/j1939tp-design.md](services/j1939tp-design.md) | 已完成 |
| KeyM | [services/keym-design.md](services/keym-design.md) | 已完成 |
| LdCom | [services/ldcom-design.md](services/ldcom-design.md) | 已完成 |
| LinM | [services/linm-design.md](services/linm-design.md) | 已完成 |
| LnTm | [services/lntm-design.md](services/lntm-design.md) | 已完成 |
| MemIf | [services/memif-design.md](services/memif-design.md) | 已完成 |
| Mqtt | [services/mqtt-design.md](services/mqtt-design.md) | 已完成 |
| Nm | [services/nm-design.md](services/nm-design.md) | 已完成 |
| RamSafety | [services/ramsafety-design.md](services/ramsafety-design.md) | 已完成 |
| Sd | [services/sd-design.md](services/sd-design.md) | 已完成 |
| SoAd | [services/soad-design.md](services/soad-design.md) | 已完成 |
| SomeIpTp | [services/someiptp-design.md](services/someiptp-design.md) | 已完成 |
| SomeIpXF | [services/someipxf-design.md](services/someipxf-design.md) | 已完成 |
| Swc | [services/swc-design.md](services/swc-design.md) | 已完成 |
| TcpIp | [services/tcpip-design.md](services/tcpip-design.md) | 已完成 |
| Tm | [services/tm-design.md](services/tm-design.md) | 已完成 |
| UdpNm | [services/udpnm-design.md](services/udpnm-design.md) | 已完成 |
| Xcp | [services/xcp-design.md](services/xcp-design.md) | 已完成 |

#### ECUAL 层（11 个）

| 模块 | 文件 | 状态 |
|------|------|------|
| CanTrcv | [ecual/cantrcv-design.md](ecual/cantrcv-design.md) | 已完成 |
| EthSM | [ecual/ethsm-design.md](ecual/ethsm-design.md) | 已完成 |
| EthTSyn | [ecual/ethtsyn-design.md](ecual/ethtsyn-design.md) | 已完成 |
| EthSwt | [ecual/ethswt-design.md](ecual/ethswt-design.md) | 已完成 |
| EthTrcv | [ecual/ethtrcv-design.md](ecual/ethtrcv-design.md) | 已完成 |
| IoHwAb | [ecual/iohwab-design.md](ecual/iohwab-design.md) | 已完成 |
| LinTp | [ecual/lintp-design.md](ecual/lintp-design.md) | 已完成 |
| LinTrcv | [ecual/lintrcv-design.md](ecual/lintrcv-design.md) | 已完成 |
| SomeIpIf | [ecual/someipif-design.md](ecual/someipif-design.md) | 已完成 |
| SomeIpSd | [ecual/someipsd-design.md](ecual/someipsd-design.md) | 已完成 |
| Srp | [ecual/srp-design.md](ecual/srp-design.md) | 已完成 |

---

**总计: 89 个模块设计文档 (21 MCAL + 20 ECUAL + 48 Services)**
