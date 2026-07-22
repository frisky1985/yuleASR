# BSW 模块清单

## MCAL（微控制器抽象层）— 21 模块

| 模块 | 说明 | 状态 |
|:-----|:-----|:----:|
| Adc | 模数转换器驱动 | ✅ |
| Can | CAN 控制器驱动 | ✅ |
| Crypto | 加密硬件驱动 | ✅ |
| Dio | 数字 I/O 驱动 | ✅ |
| Eep | EEPROM 驱动 | ✅ |
| Eth | 以太网控制器驱动 | ✅ |
| Fee | Flash EEPROM 仿真 | ✅ |
| Flash | Flash 驱动 | ✅ |
| Fls | Flash 测试驱动 | ✅ |
| Gpt | 通用定时器驱动 | ✅ |
| I2c | I²C 总线驱动 | ✅ |
| Icu | 输入捕获单元驱动 | ✅ |
| Lin | LIN 控制器驱动 | ✅ |
| Mcu | 微控制器驱动 | ✅ |
| Ocu | 输出比较单元驱动 | ✅ |
| Port | 端口驱动 | ✅ |
| Pwm | PWM 驱动 | ✅ |
| Ramtst | RAM 测试驱动 | ✅ |
| Spi | SPI 总线驱动 | ✅ |
| Uart | UART 驱动 | ✅ |
| Wdg | 看门狗驱动 | ✅ |

## ECUAL（ECU 抽象层）— 29 模块

| 模块 | 说明 | 状态 |
|:-----|:-----|:----:|
| CanIf | CAN 接口 | ✅ |
| CanNm | CAN 网络管理 | ✅ |
| CanSm | CAN 状态管理 | ✅ |
| CanTp | CAN 传输协议 (ISO 15765-2) | ✅ |
| CanTrcv | CAN 收发器驱动 | ✅ |
| Dlt | 诊断日志和跟踪 | ✅ |
| DoIP | IP 诊断通信 (ISO 13400-2) | ✅ |
| Ea | EEPROM 抽象层 | ✅ |
| EthIf | 以太网接口 | ✅ |
| EthSm | 以太网状态管理 | ✅ |
| EthTrcv | 以太网收发器驱动 | ✅ |
| Fee | Flash EEPROM 抽象 | ✅ |
| FiM | 功能抑制管理器 | ✅ |
| FrIf | FlexRay 接口 | ✅ |
| FrTp | FlexRay 传输协议 | ✅ |
| IoHwAb | I/O 硬件抽象 | ✅ |
| IpduM | I-PDU 多路复用器 | ✅ |
| J1939Tp | J1939 传输协议 | ✅ |
| LinIf | LIN 接口 | ✅ |
| LinNm | LIN 网络管理 | ✅ |
| LinSM | LIN 调度管理器 | ✅ |
| LinTp | LIN 传输协议 | ✅ |
| LinTrcv | LIN 收发器驱动 | ✅ |
| MemIf | 内存抽象接口 | ✅ |
| SomeIpIf | SOME/IP 接口 | ✅ |
| SomeIpSd | SOME/IP 服务发现 | ✅ |
| Srp | 同步请求协议 | ✅ |
| WdgIf | 看门狗接口 | ✅ |
| Xcp | 测量和标定协议 | ✅ |

## Services（服务层）— 46 模块

| 模块 | 说明 | 状态 |
|:-----|:-----|:----:|
| BswM | BSW 模式管理器 | ✅ |
| CanM | CAN 模式管理 | ✅ |
| CanSM | CAN 状态管理器 | ✅ |
| CanTSyn | CAN 时间同步 | ✅ |
| Com | 通信模块 | ✅ |
| ComM | 通信管理器 | ✅ |
| Crc | CRC 计算服务 | ✅ |
| CryIf | 加密接口 | ✅ |
| Csm | 加密服务管理 | ✅ |
| Dcm | 诊断通信管理器 | ✅ |
| Dem | 诊断事件管理器 | ✅ |
| Det | 开发错误跟踪 | ✅ |
| Dlt | 诊断日志跟踪 | ✅ |
| DoCan | CAN 诊断适配器 | ✅ |
| DoIP | IP 诊断通信 | ✅ |
| E2E | 端到端通信保护 | ✅ |
| EcuC | ECU 配置 | ✅ |
| EcuM | ECU 状态管理器 | ✅ |
| EthSm | 以太网状态管理器 | ✅ |
| FiM | 功能抑制管理器 | ✅ |
| IpduM | I-PDU 多路复用器 | ✅ |
| J1939Nm | J1939 网络管理 | ✅ |
| J1939Tp | J1939 传输协议 | ✅ |
| KeyM | 密钥管理器 | ✅ |
| LinM | LIN 模式管理 | ✅ |
| LinSM | LIN 调度管理器 | ✅ |
| LnTm | LIN 传输层管理 | ✅ |
| Mem | 内存服务 | ✅ |
| MemIf | 内存抽象接口 | ✅ |
| Mqtt | MQTT 客户端 | ✅ |
| Nm | 网络管理 | ✅ |
| NvM | NVRAM 管理器 | ✅ |
| PduR | PDU 路由器 | ✅ |
| RamSafety | RAM 安全服务 | ✅ |
| SchM | 调度管理器 | ✅ |
| SecOC | 安全车载通信 | ✅ |
| SoAd | 套接字适配器 | ✅ |
| SomeIp | SOME/IP 协议 | ✅ |
| SomeIpTp | SOME/IP 传输协议 | ✅ |
| SomeIpXf | SOME/IP 转换器 | ✅ |
| StbM | 同步时间基础管理器 | ✅ |
| Swc | 软件组件服务 | ✅ |
| UdpNm | UDP 网络管理 | ✅ |
| WdgM | 看门狗管理器 | ✅ |
| Xcp | 测量和标定协议 | ✅ |
| ... 更多 | | ✅ |

## 其他组件

| 组件 | 说明 | 状态 |
|:-----|:-----|:----:|
| RTE | 运行时环境 | ✅ |
| ASW | 应用层 (8 组件) | ✅ |
| OS | 操作系统 (FreeRTOS) | ✅ |
| DDS | DDS 中间件 (OMG v1.4) | ✅ |
| ARXML 工具链 | 解析/生成/检查 | ✅ |
