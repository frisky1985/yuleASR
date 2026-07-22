# Acceptance Matrix

> Generated: 2026-07-22T11:35:19.901260
> Version: 0.1.0

| Req ID | Requirement | SHALL | 验证方法 | 测试文件 | 匹配方式 | 置信度 | 状态 |
|:------:|:-----------|:------|:---------|:--------|:--------:|:------:|:----:|
| MCAL-SHALL-001 | MCAL-SHALL-001 | MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write) | Unit Test | — | — |  | ❌ |
| MCAL-SHALL-002 | MCAL-SHALL-002 | 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式 | Unit Test | — | — |  | ❌ |
| MCAL-SHALL-003 | MCAL-SHALL-003 | MCAL SHALL 使用 MISRA C:2023 合规编码风格 | Unit Test | — | — |  | ❌ |
| ECUAL-SHALL-001 | ECUAL-SHALL-001 | ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器 | Unit Test | — | — |  | ❌ |
| ECUAL-SHALL-002 | ECUAL-SHALL-002 | 看门狗管理器 SHALL 在超时前刷新 | Unit Test | — | — |  | ❌ |
| SVC-SHALL-001 | SVC-SHALL-001 | OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务 | Unit Test | — | — |  | ❌ |
| SVC-SHALL-002 | SVC-SHALL-002 | 通信栈 (CAN/以太网) SHALL 实现 PDU 路由 | Unit Test | — | — |  | ❌ |
| SVC-SHALL-003 | SVC-SHALL-003 | 诊断事件管理器 (Dem) SHALL 记录并上报 DTC | Unit Test | — | — |  | ❌ |
| NFR-SHALL-001 | NFR-SHALL-001 | 代码 MISRA C:2023 合规 | Unit Test | — | — |  | ❌ |
| NFR-SHALL-002 | NFR-SHALL-002 | 单元测试行覆盖率 | Unit Test | — | — |  | ❌ |
| NFR-SHALL-003 | NFR-SHALL-003 | 条件覆盖率 | Unit Test | — | — |  | ❌ |
| NFR-SHALL-004 | NFR-SHALL-004 | 静态分析 (cppcheck) | Unit Test | — | — |  | ❌ |
| 7MISRA-SHALL-13 | 7MISRA-SHALL-13 | SHALL 使用 MISRA C:2023 `safety` 配置 | Unit Test | — | — |  | ❌ |
| DCMDIAGN-SHALL-1 | DCMDIAGN-SHALL-1 | SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1. | Unit Test | — | — |  | ❌ |
| DCMDIAGN-SHALL-2 | DCMDIAGN-SHALL-2 | SHALL support a maximum of 4 concurrent diagnostic sessions. | Unit Test | — | — |  | ❌ |
| DCMDIAGN-SHALL-3 | DCMDIAGN-SHALL-3 | SHALL enforce P2 timeout of 50ms for diagnostic responses. | Unit Test | — | — |  | ❌ |
| DCMDIAGN-SHALL-4 | DCMDIAGN-SHALL-4 | SHALL enforce P2\* timeout of 500ms for diagnostic responses. | Unit Test | — | — |  | ❌ |
| DEMDIAGN-SHALL-5 | DEMDIAGN-SHALL-5 | SHALL support storage of up to 256 diagnostic trouble codes (DTCs). | Unit Test | — | — |  | ❌ |
| DEMDIAGN-SHALL-6 | DEMDIAGN-SHALL-6 | SHALL support 3 event priority levels: Low, Medium, High. | Unit Test | — | — |  | ❌ |
| DEMDIAGN-SHALL-7 | DEMDIAGN-SHALL-7 | SHALL store diagnostic events with primary and secondary (freeze frame) data. | Unit Test | — | — |  | ❌ |
| DEMDIAGN-SHALL-8 | DEMDIAGN-SHALL-8 | SHALL provide a configurable aging counter with default 40 cycles. | Unit Test | — | — |  | ❌ |
| COM-SHALL-9 | COM-SHALL-9 | SHALL support a configurable signal count with default of 1024 signals. | Unit Test | — | — |  | ❌ |
| COM-SHALL-10 | COM-SHALL-10 | SHALL support signal group communication. | Unit Test | — | — |  | ❌ |
| COM-SHALL-11 | COM-SHALL-11 | SHALL support I-PDU send and receive directions. | Unit Test | — | — |  | ❌ |
| COM-SHALL-12 | COM-SHALL-12 | SHALL support deadline monitoring for signal transmission. | Unit Test | — | — |  | ❌ |
| PDUR-SHALL-13 | PDUR-SHALL-13 | SHALL maintain a static routing table generated at build time. | Unit Test | — | — |  | ❌ |
| PDUR-SHALL-14 | PDUR-SHALL-14 | SHALL support a maximum of 512 routing paths. | Unit Test | — | — |  | ❌ |
| PDUR-SHALL-15 | PDUR-SHALL-15 | SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet. | Unit Test | — | — |  | ❌ |
| NVM-SHALL-16 | NVM-SHALL-16 | SHALL support native, redundant, and dataset NVM block management. | Unit Test | — | — |  | ❌ |
| NVM-SHALL-17 | NVM-SHALL-17 | SHALL use CRC-32 for write verification. | Unit Test | — | — |  | ❌ |
| NVM-SHALL-18 | NVM-SHALL-18 | SHALL support block sizes from 1 to 65536 bytes. | Unit Test | — | — |  | ❌ |
| NVM-SHALL-19 | NVM-SHALL-19 | SHALL support a maximum of 512 NVM blocks. | Unit Test | — | — |  | ❌ |
| NVM-SHALL-20 | NVM-SHALL-20 | SHALL support 4 job priority levels. | Unit Test | — | — |  | ❌ |
| ECUM-SHALL-21 | ECUM-SHALL-21 | SHALL support startup phases STARTUP_ONE and STARTUP_TWO. | Unit Test | — | — |  | ❌ |
| ECUM-SHALL-22 | ECUM-SHALL-22 | SHALL support shutdown targets OFF, RESET, and SLEEP. | Unit Test | — | — |  | ❌ |
| ECUM-SHALL-23 | ECUM-SHALL-23 | SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer. | Unit Test | — | — |  | ❌ |
| OSAUTOSA-SHALL-24 | OSAUTOSA-SHALL-24 | SHALL provide fixed cyclic schedule tables. | Unit Test | — | — |  | ❌ |
| OSAUTOSA-SHALL-25 | OSAUTOSA-SHALL-25 | SHALL support BCC2 and ECC2 task conformance classes. | Unit Test | — | — |  | ❌ |
| OSAUTOSA-SHALL-26 | OSAUTOSA-SHALL-26 | SHALL support a maximum of 64 tasks. | Unit Test | — | — |  | ❌ |
| OSAUTOSA-SHALL-27 | OSAUTOSA-SHALL-27 | SHALL support a maximum of 32 alarms. | Unit Test | — | — |  | ❌ |
| OSAUTOSA-SHALL-28 | OSAUTOSA-SHALL-28 | SHALL implement the priority ceiling protocol for resource management. | Unit Test | — | — |  | ❌ |
| CANIFCAN-SHALL-1 | CANIFCAN-SHALL-1 | SHALL support a maximum of 2 CAN controllers (CAN0, CAN1). | Unit Test | — | — |  | ❌ |
| CANIFCAN-SHALL-2 | CANIFCAN-SHALL-2 | SHALL support up to 512 PDU IDs. | Unit Test | — | — |  | ❌ |
| CANIFCAN-SHALL-3 | CANIFCAN-SHALL-3 | SHALL support transmit and receive PDU modes. | Unit Test | — | — |  | ❌ |
| CANIFCAN-SHALL-4 | CANIFCAN-SHALL-4 | SHALL support sleep and wakeup functionality. | Unit Test | — | — |  | ❌ |
| CANTPCAN-SHALL-5 | CANTPCAN-SHALL-5 | SHALL implement the ISO 15765-2 CAN transport protocol. | Unit Test | — | — |  | ❌ |
| CANTPCAN-SHALL-6 | CANTPCAN-SHALL-6 | SHALL support message segmentation up to 4095 bytes per message. | Unit Test | — | — |  | ❌ |
| CANTPCAN-SHALL-7 | CANTPCAN-SHALL-7 | SHALL support Continuous and Wait flow control modes. | Unit Test | — | — |  | ❌ |
| CANTPCAN-SHALL-8 | CANTPCAN-SHALL-8 | SHALL support Physical and Functional addressing. | Unit Test | — | — |  | ❌ |
| CANNMCAN-SHALL-9 | CANNMCAN-SHALL-9 | SHALL implement AUTOSAR CAN Network Management protocol. | Unit Test | — | — |  | ❌ |
| CANNMCAN-SHALL-10 | CANNMCAN-SHALL-10 | SHALL support a configurable 8-bit node ID. | Unit Test | — | — |  | ❌ |
| CANNMCAN-SHALL-11 | CANNMCAN-SHALL-11 | SHALL support configurable message cycle time with default of 100ms. | Unit Test | — | — |  | ❌ |
| CANNMCAN-SHALL-12 | CANNMCAN-SHALL-12 | SHALL support configurable repeat message timer with default of 1000ms. | Unit Test | — | — |  | ❌ |
| CANNMCAN-SHALL-13 | CANNMCAN-SHALL-13 | SHALL support bus synchronization. | Unit Test | — | — |  | ❌ |
| SOADSOCK-SHALL-14 | SOADSOCK-SHALL-14 | SHALL support a maximum of 32 sockets. | Unit Test | — | — |  | ❌ |
| SOADSOCK-SHALL-15 | SOADSOCK-SHALL-15 | SHALL support TCP and UDP protocols. | Unit Test | — | — |  | ❌ |
| SOADSOCK-SHALL-16 | SOADSOCK-SHALL-16 | SHALL support Server and Client connection types. | Unit Test | — | — |  | ❌ |
| SOADSOCK-SHALL-17 | SOADSOCK-SHALL-17 | SHALL support SOME/IP protocol communication. | Unit Test | — | — |  | ❌ |
| SOMEIPSD-SHALL-18 | SOMEIPSD-SHALL-18 | SHALL support an offer cycle of 1000ms for service discovery. | Unit Test | — | — |  | ❌ |
| SOMEIPSD-SHALL-19 | SOMEIPSD-SHALL-19 | SHALL support a request cycle of 2000ms for service discovery. | Unit Test | — | — |  | ❌ |
| SOMEIPSD-SHALL-20 | SOMEIPSD-SHALL-20 | SHALL support TTL multiplier of 3 for service entries. | Unit Test | — | — |  | ❌ |
| DLT-SHALL-21 | DLT-SHALL-21 | SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose. | Unit Test | — | — |  | ❌ |
| DLT-SHALL-22 | DLT-SHALL-22 | SHALL support TCP and Serial transport for DLT messages. | Unit Test | — | — |  | ❌ |
| DLT-SHALL-23 | DLT-SHALL-23 | SHALL support Application ID filtering. | Unit Test | — | — |  | ❌ |
| XCP-SHALL-24 | XCP-SHALL-24 | SHALL support CAN and Ethernet transport layers for XCP. | Unit Test | — | — |  | ❌ |
| XCP-SHALL-25 | XCP-SHALL-25 | SHALL implement XCP protocol version 1.5. | Unit Test | — | — |  | ❌ |
| XCP-SHALL-26 | XCP-SHALL-26 | SHALL support XCP slave functionality. | Unit Test | — | — |  | ❌ |
| XCP-SHALL-27 | XCP-SHALL-27 | SHALL support calibration page switching. | Unit Test | — | — |  | ❌ |
| XCP-SHALL-28 | XCP-SHALL-28 | SHALL support up to 8 DAQ lists. | Unit Test | — | — |  | ❌ |
| ADCDRIVE-SHALL-1 | ADCDRIVE-SHALL-1 | SHALL support 10-bit and 12-bit configurable ADC resolution. | Unit Test | — | — |  | ❌ |
| ADCDRIVE-SHALL-2 | ADCDRIVE-SHALL-2 | SHALL support Single, Continuous, and Scan conversion modes. | Unit Test | — | — |  | ❌ |
| ADCDRIVE-SHALL-3 | ADCDRIVE-SHALL-3 | SHALL support up to 16 channels per ADC instance. | Unit Test | — | — |  | ❌ |
| ADCDRIVE-SHALL-4 | ADCDRIVE-SHALL-4 | SHALL support left and right result alignment. | Unit Test | — | — |  | ❌ |
| ADCDRIVE-SHALL-5 | ADCDRIVE-SHALL-5 | SHALL support interrupt-based and polling notification modes. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-6 | CANDRIVE-SHALL-6 | SHALL support Classical CAN (2.0B) and CAN FD protocols. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-7 | CANDRIVE-SHALL-7 | SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-8 | CANDRIVE-SHALL-8 | SHALL provide 64 mailboxes for CAN message buffering. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-9 | CANDRIVE-SHALL-9 | SHALL support FIFO mode for CAN message reception. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-10 | CANDRIVE-SHALL-10 | SHALL support loopback mode for self-test. | Unit Test | — | — |  | ❌ |
| CANDRIVE-SHALL-11 | CANDRIVE-SHALL-11 | SHALL provide automatic bus-off recovery conforming to AUTOSAR specification. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-12 | CRYPTODR-SHALL-12 | SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-13 | CRYPTODR-SHALL-13 | SHALL support SHA-256 hashing. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-14 | CRYPTODR-SHALL-14 | SHALL support ECC P-256 elliptic curve cryptography. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-15 | CRYPTODR-SHALL-15 | SHALL accelerate cryptographic operations using S32K312 HSM. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-16 | CRYPTODR-SHALL-16 | SHALL store cryptographic keys in HSM secure NVM. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-17 | CRYPTODR-SHALL-17 | SHALL provide integrated hardware TRNG. | Unit Test | — | — |  | ❌ |
| CRYPTODR-SHALL-18 | CRYPTODR-SHALL-18 | SHALL provide MbedTLS fallback for SIL simulation. | Unit Test | — | — |  | ❌ |
| DIODRIVE-SHALL-19 | DIODRIVE-SHALL-19 | SHALL support 8 ports with 32 pins each for digital I/O. | Unit Test | — | — |  | ❌ |
| DIODRIVE-SHALL-20 | DIODRIVE-SHALL-20 | SHALL support configurable pin direction per pin. | Unit Test | — | — |  | ❌ |
| DIODRIVE-SHALL-21 | DIODRIVE-SHALL-21 | SHALL support HIGH and LEVEL output levels. | Unit Test | — | — |  | ❌ |
| DIODRIVE-SHALL-22 | DIODRIVE-SHALL-22 | SHALL support edge-triggered interrupt on rising, falling, and both edges. | Unit Test | — | — |  | ❌ |
| PORTDRIV-SHALL-23 | PORTDRIV-SHALL-23 | SHALL support pin mux configuration for approximately 100 pins. | Unit Test | — | — |  | ❌ |
| PORTDRIV-SHALL-24 | PORTDRIV-SHALL-24 | SHALL support ALT0 through ALT7 mux modes. | Unit Test | — | — |  | ❌ |
| PORTDRIV-SHALL-25 | PORTDRIV-SHALL-25 | SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength. | Unit Test | — | — |  | ❌ |
| GPTDRIVE-SHALL-26 | GPTDRIVE-SHALL-26 | SHALL provide 8 hardware timer channels. | Unit Test | — | — |  | ❌ |
| GPTDRIVE-SHALL-27 | GPTDRIVE-SHALL-27 | SHALL provide 32-bit timer resolution. | Unit Test | — | — |  | ❌ |
| GPTDRIVE-SHALL-28 | GPTDRIVE-SHALL-28 | SHALL support prescaler values from 1 to 65536. | Unit Test | — | — |  | ❌ |
| GPTDRIVE-SHALL-29 | GPTDRIVE-SHALL-29 | SHALL support one-shot and continuous timer modes. | Unit Test | — | — |  | ❌ |
| ICUDRIVE-SHALL-30 | ICUDRIVE-SHALL-30 | SHALL support up to 8 input capture channels. | Unit Test | — | — |  | ❌ |
| ICUDRIVE-SHALL-31 | ICUDRIVE-SHALL-31 | SHALL support signal period, duty cycle, and pulse width measurement. | Unit Test | — | — |  | ❌ |
| ICUDRIVE-SHALL-32 | ICUDRIVE-SHALL-32 | SHALL support rising, falling, and both edge detection. | Unit Test | — | — |  | ❌ |
| MCUDRIVE-SHALL-33 | MCUDRIVE-SHALL-33 | SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL. | Unit Test | — | — |  | ❌ |
| MCUDRIVE-SHALL-34 | MCUDRIVE-SHALL-34 | SHALL support 4 RAM section banks. | Unit Test | — | — |  | ❌ |
| MCUDRIVE-SHALL-35 | MCUDRIVE-SHALL-35 | SHALL support RUN, SLEEP, STOP, and STANDBY power modes. | Unit Test | — | — |  | ❌ |
| MCUDRIVE-SHALL-36 | MCUDRIVE-SHALL-36 | SHALL support POR, WDG, SW, and External reset sources. | Unit Test | — | — |  | ❌ |
| WDGDRIVE-SHALL-37 | WDGDRIVE-SHALL-37 | SHALL provide configurable watchdog timeout from milliseconds to seconds. | Unit Test | — | — |  | ❌ |
| WDGDRIVE-SHALL-38 | WDGDRIVE-SHALL-38 | SHALL support window mode watchdog operation. | Unit Test | — | — |  | ❌ |
| WDGDRIVE-SHALL-39 | WDGDRIVE-SHALL-39 | SHALL support test mode for diagnostic testing. | Unit Test | — | — |  | ❌ |
| DCM-SHALL-01 | DCM-SHALL-01 | SHALL support ISO 14229-1 UDS diagnostic services | Unit Test | — | — |  | ❌ |
| DCM-SHALL-02 | DCM-SHALL-02 | SHALL support session management (default, programming, extended) | Unit Test | — | — |  | ❌ |
| DEM-SHALL-01 | DEM-SHALL-01 | SHALL support DTC storage and retrieval | Unit Test | — | — |  | ❌ |
| DET-SHALL-01 | DET-SHALL-01 | SHALL report development errors with module ID and error code | Unit Test | — | — |  | ❌ |
| DOIP-SHALL-01 | DOIP-SHALL-01 | SHALL support DoIP vehicle discovery | Unit Test | — | — |  | ❌ |
| COM-SHALL-01 | COM-SHALL-01 | SHALL support signal-based I-PDU communication | Unit Test | — | — |  | ❌ |
| PDUR-SHALL-01 | PDUR-SHALL-01 | SHALL route I-PDUs between COM and transport layers | Unit Test | — | — |  | ❌ |
| CANSM-SHALL-01 | CANSM-SHALL-01 | SHALL manage CAN network state machine | Unit Test | — | — |  | ❌ |
| LIN-SHALL-01 | LIN-SHALL-01 | SHALL support LIN master/slave communication | Unit Test | — | — |  | ❌ |
| NVM-SHALL-01 | NVM-SHALL-01 | SHALL support NV block read/write with redundancy | Unit Test | — | — |  | ❌ |
| FEE-SHALL-01 | FEE-SHALL-01 | SHALL emulate EEPROM over flash with wear leveling | Unit Test | — | — |  | ❌ |
| MEMIF-SHALL-01 | MEMIF-SHALL-01 | SHALL abstract NvM from Fee/EEP driver | Unit Test | — | — |  | ❌ |
| ECUM-SHALL-01 | ECUM-SHALL-01 | SHALL manage ECU startup/shutdown/wakeup sequences | Unit Test | — | — |  | ❌ |
| BSWM-SHALL-01 | BSWM-SHALL-01 | SHALL implement mode-based BSW scheduling | Unit Test | — | — |  | ❌ |
| WDGM-SHALL-01 | WDGM-SHALL-01 | SHALL monitor alive and deadline supervision | Unit Test | — | — |  | ❌ |
| OS-SHALL-01 | OS-SHALL-01 | SHALL provide AUTOSAR OS SC4 compliant scheduling | Unit Test | — | — |  | ❌ |
| E2E-SHALL-01 | E2E-SHALL-01 | SHALL protect safety-critical signals with CRC and sequence counter | Unit Test | — | — |  | ❌ |
| CSM-SHALL-01 | CSM-SHALL-01 | SHALL provide cryptographic service management | Unit Test | — | — |  | ❌ |
| KEYM-SHALL-01 | KEYM-SHALL-01 | SHALL manage cryptographic keys | Unit Test | — | — |  | ❌ |

## Summary
- Total SHALL statements: 127
- Covered by tests: 0 (0%)
- Uncovered: 127
- Threshold: 100% → ❌ FAIL