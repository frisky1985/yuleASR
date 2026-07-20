# Acceptance Matrix

> Generated: 2026-07-20T04:05:28
> Version: 0.1.0

| Req ID | Requirement | SHALL | 验证方法 | 测试文件 | 匹配方式 | 置信度 | 状态 |
|:------:|:-----------|:------|:---------|:--------|:--------:|:------:|:----:|
| MCAL-SHALL-001 | MCAL-SHALL-001 | MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write) | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCAL-SHALL-002 | MCAL-SHALL-002 | 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式 | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCAL-SHALL-003 | MCAL-SHALL-003 | MCAL SHALL 使用 MISRA C:2023 合规编码风格 | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ECUAL-SHALL-001 | ECUAL-SHALL-001 | ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器 | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ECUAL-SHALL-002 | ECUAL-SHALL-002 | 看门狗管理器 SHALL 在超时前刷新 | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| SVC-SHALL-001 | SVC-SHALL-001 | OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务 | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SVC-SHALL-002 | SVC-SHALL-002 | 通信栈 (CAN/以太网) SHALL 实现 PDU 路由 | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SVC-SHALL-003 | SVC-SHALL-003 | 诊断事件管理器 (Dem) SHALL 记录并上报 DTC | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NFR-SHALL-001 | NFR-SHALL-001 | 代码 MISRA C:2023 合规 | Unit Test | tests/e2e/test_misra_compliance.c | — |  | ✅ |
| NFR-SHALL-002 | NFR-SHALL-002 | 单元测试行覆盖率 | Unit Test | tests/e2e/test_misra_compliance.c | — |  | ✅ |
| NFR-SHALL-003 | NFR-SHALL-003 | 条件覆盖率 | Unit Test | tests/e2e/test_misra_compliance.c | — |  | ✅ |
| NFR-SHALL-004 | NFR-SHALL-004 | 静态分析 (cppcheck) | Unit Test | tests/e2e/test_misra_compliance.c | — |  | ✅ |
| MISRA-SHALL-001 | MISRA-SHALL-001 | SHALL 使用 MISRA C:2023 `safety` 配置 | Unit Test | tests/e2e/test_misra_compliance.c | — |  | ✅ |
| DCM-SHALL-001 | DCM-SHALL-001 | The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DCM-SHALL-002 | DCM-SHALL-002 | The system SHALL support a maximum of 4 concurrent diagnostic sessions. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DCM-SHALL-003 | DCM-SHALL-003 | The system SHALL enforce P2 timeout of 50ms for diagnostic responses. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DCM-SHALL-004 | DCM-SHALL-004 | The system SHALL enforce P2\* timeout of 500ms for diagnostic responses. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DEM-SHALL-001 | DEM-SHALL-001 | The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs). | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DEM-SHALL-002 | DEM-SHALL-002 | The system SHALL support 3 event priority levels: Low, Medium, High. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DEM-SHALL-003 | DEM-SHALL-003 | The system SHALL store diagnostic events with primary and secondary (freeze frame) data. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DEM-SHALL-004 | DEM-SHALL-004 | The system SHALL provide a configurable aging counter with default 40 cycles. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| COM-SHALL-001 | COM-SHALL-001 | The system SHALL support a configurable signal count with default of 1024 signals. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| COM-SHALL-002 | COM-SHALL-002 | The system SHALL support signal group communication. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| COM-SHALL-003 | COM-SHALL-003 | The system SHALL support I-PDU send and receive directions. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| COM-SHALL-004 | COM-SHALL-004 | The system SHALL support deadline monitoring for signal transmission. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| PDUR-SHALL-001 | PDUR-SHALL-001 | The system SHALL maintain a static routing table generated at build time. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| PDUR-SHALL-002 | PDUR-SHALL-002 | The system SHALL support a maximum of 512 routing paths. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| PDUR-SHALL-003 | PDUR-SHALL-003 | The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-SHALL-001 | NVM-SHALL-001 | The system SHALL support native, redundant, and dataset NVM block management. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-SHALL-002 | NVM-SHALL-002 | The system SHALL use CRC-32 for write verification. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-SHALL-003 | NVM-SHALL-003 | The system SHALL support block sizes from 1 to 65536 bytes. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-SHALL-004 | NVM-SHALL-004 | The system SHALL support a maximum of 512 NVM blocks. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-SHALL-005 | NVM-SHALL-005 | The system SHALL support 4 job priority levels. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| ECUM-SHALL-001 | ECUM-SHALL-001 | The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| ECUM-SHALL-002 | ECUM-SHALL-002 | The system SHALL support shutdown targets OFF, RESET, and SLEEP. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| ECUM-SHALL-003 | ECUM-SHALL-003 | The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OSSC4-SHALL-001 | OSSC4-SHALL-001 | The system SHALL provide fixed cyclic schedule tables. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OSSC4-SHALL-002 | OSSC4-SHALL-002 | The system SHALL support BCC2 and ECC2 task conformance classes. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OSSC4-SHALL-003 | OSSC4-SHALL-003 | The system SHALL support a maximum of 64 tasks. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OSSC4-SHALL-004 | OSSC4-SHALL-004 | The system SHALL support a maximum of 32 alarms. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OSSC4-SHALL-005 | OSSC4-SHALL-005 | The system SHALL implement the priority ceiling protocol for resource management. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANIF-SHALL-001 | CANIF-SHALL-001 | The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1). | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANIF-SHALL-002 | CANIF-SHALL-002 | The system SHALL support up to 512 PDU IDs. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANIF-SHALL-003 | CANIF-SHALL-003 | The system SHALL support transmit and receive PDU modes. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANIF-SHALL-004 | CANIF-SHALL-004 | The system SHALL support sleep and wakeup functionality. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANTP-SHALL-001 | CANTP-SHALL-001 | The system SHALL implement the ISO 15765-2 CAN transport protocol. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANTP-SHALL-002 | CANTP-SHALL-002 | The system SHALL support message segmentation up to 4095 bytes per message. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANTP-SHALL-003 | CANTP-SHALL-003 | The system SHALL support Continuous and Wait flow control modes. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANTP-SHALL-004 | CANTP-SHALL-004 | The system SHALL support Physical and Functional addressing. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANNM-SHALL-001 | CANNM-SHALL-001 | The system SHALL implement AUTOSAR CAN Network Management protocol. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANNM-SHALL-002 | CANNM-SHALL-002 | The system SHALL support a configurable 8-bit node ID. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANNM-SHALL-003 | CANNM-SHALL-003 | The system SHALL support configurable message cycle time with default of 100ms. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANNM-SHALL-004 | CANNM-SHALL-004 | The system SHALL support configurable repeat message timer with default of 1000ms. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANNM-SHALL-005 | CANNM-SHALL-005 | The system SHALL support bus synchronization. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOAD-SHALL-001 | SOAD-SHALL-001 | The system SHALL support a maximum of 32 sockets. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOAD-SHALL-002 | SOAD-SHALL-002 | The system SHALL support TCP and UDP protocols. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOAD-SHALL-003 | SOAD-SHALL-003 | The system SHALL support Server and Client connection types. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOAD-SHALL-004 | SOAD-SHALL-004 | The system SHALL support SOME/IP protocol communication. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOMEIPSD-SHALL-001 | SOMEIPSD-SHALL-001 | The system SHALL support an offer cycle of 1000ms for service discovery. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOMEIPSD-SHALL-002 | SOMEIPSD-SHALL-002 | The system SHALL support a request cycle of 2000ms for service discovery. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| SOMEIPSD-SHALL-003 | SOMEIPSD-SHALL-003 | The system SHALL support TTL multiplier of 3 for service entries. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DLT-SHALL-001 | DLT-SHALL-001 | The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DLT-SHALL-002 | DLT-SHALL-002 | The system SHALL support TCP and Serial transport for DLT messages. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DLT-SHALL-003 | DLT-SHALL-003 | The system SHALL support Application ID filtering. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| XCP-SHALL-001 | XCP-SHALL-001 | The system SHALL support CAN and Ethernet transport layers for XCP. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| XCP-SHALL-002 | XCP-SHALL-002 | The system SHALL implement XCP protocol version 1.5. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| XCP-SHALL-003 | XCP-SHALL-003 | The system SHALL support XCP slave functionality. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| XCP-SHALL-004 | XCP-SHALL-004 | The system SHALL support calibration page switching. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| XCP-SHALL-005 | XCP-SHALL-005 | The system SHALL support up to 8 DAQ lists. | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| ADC-SHALL-001 | ADC-SHALL-001 | The system SHALL support 10-bit and 12-bit configurable ADC resolution. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ADC-SHALL-002 | ADC-SHALL-002 | The system SHALL support Single, Continuous, and Scan conversion modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ADC-SHALL-003 | ADC-SHALL-003 | The system SHALL support up to 16 channels per ADC instance. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ADC-SHALL-004 | ADC-SHALL-004 | The system SHALL support left and right result alignment. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ADC-SHALL-005 | ADC-SHALL-005 | The system SHALL support interrupt-based and polling notification modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-001 | CANDRV-SHALL-001 | The system SHALL support Classical CAN (2.0B) and CAN FD protocols. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-002 | CANDRV-SHALL-002 | The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-003 | CANDRV-SHALL-003 | The system SHALL provide 64 mailboxes for CAN message buffering. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-004 | CANDRV-SHALL-004 | The system SHALL support FIFO mode for CAN message reception. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-005 | CANDRV-SHALL-005 | The system SHALL support loopback mode for self-test. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CANDRV-SHALL-006 | CANDRV-SHALL-006 | The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-001 | CRYPTO-SHALL-001 | The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-002 | CRYPTO-SHALL-002 | The system SHALL support SHA-256 hashing. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-003 | CRYPTO-SHALL-003 | The system SHALL support ECC P-256 elliptic curve cryptography. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-004 | CRYPTO-SHALL-004 | The system SHALL accelerate cryptographic operations using S32K312 HSM. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-005 | CRYPTO-SHALL-005 | The system SHALL store cryptographic keys in HSM secure NVM. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-006 | CRYPTO-SHALL-006 | The system SHALL provide integrated hardware TRNG. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| CRYPTO-SHALL-007 | CRYPTO-SHALL-007 | The system SHALL provide MbedTLS fallback for SIL simulation. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| DIODRV-SHALL-001 | DIODRV-SHALL-001 | The system SHALL support 8 ports with 32 pins each for digital I/O. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| DIODRV-SHALL-002 | DIODRV-SHALL-002 | The system SHALL support configurable pin direction per pin. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| DIODRV-SHALL-003 | DIODRV-SHALL-003 | The system SHALL support HIGH and LEVEL output levels. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| DIODRV-SHALL-004 | DIODRV-SHALL-004 | The system SHALL support edge-triggered interrupt on rising, falling, and both edges. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| PORTDRV-SHALL-001 | PORTDRV-SHALL-001 | The system SHALL support pin mux configuration for approximately 100 pins. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| PORTDRV-SHALL-002 | PORTDRV-SHALL-002 | The system SHALL support ALT0 through ALT7 mux modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| PORTDRV-SHALL-003 | PORTDRV-SHALL-003 | The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| GPTDRV-SHALL-001 | GPTDRV-SHALL-001 | The system SHALL provide 8 hardware timer channels. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| GPTDRV-SHALL-002 | GPTDRV-SHALL-002 | The system SHALL provide 32-bit timer resolution. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| GPTDRV-SHALL-003 | GPTDRV-SHALL-003 | The system SHALL support prescaler values from 1 to 65536. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| GPTDRV-SHALL-004 | GPTDRV-SHALL-004 | The system SHALL support one-shot and continuous timer modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ICURV-SHALL-001 | ICURV-SHALL-001 | The system SHALL support up to 8 input capture channels. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ICURV-SHALL-002 | ICURV-SHALL-002 | The system SHALL support signal period, duty cycle, and pulse width measurement. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| ICURV-SHALL-003 | ICURV-SHALL-003 | The system SHALL support rising, falling, and both edge detection. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCUDRV-SHALL-001 | MCUDRV-SHALL-001 | The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCUDRV-SHALL-002 | MCUDRV-SHALL-002 | The system SHALL support 4 RAM section banks. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCUDRV-SHALL-003 | MCUDRV-SHALL-003 | The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| MCUDRV-SHALL-004 | MCUDRV-SHALL-004 | The system SHALL support POR, WDG, SW, and External reset sources. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| WDGDRV-SHALL-001 | WDGDRV-SHALL-001 | The system SHALL provide configurable watchdog timeout from milliseconds to seconds. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| WDGDRV-SHALL-002 | WDGDRV-SHALL-002 | The system SHALL support window mode watchdog operation. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| WDGDRV-SHALL-003 | WDGDRV-SHALL-003 | The system SHALL support test mode for diagnostic testing. | Unit Test | tests/unit/test_mcal_api_contracts.c | — |  | ✅ |
| DCM-REQ-01 | DCM-REQ-01 | SHALL support ISO 14229-1 UDS diagnostic services | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DCM-REQ-02 | DCM-REQ-02 | SHALL support session management (default, programming, extended) | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DEM-REQ-01 | DEM-REQ-01 | SHALL support DTC storage and retrieval | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DET-REQ-01 | DET-REQ-01 | SHALL report development errors with module ID and error code | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| DOIP-REQ-01 | DOIP-REQ-01 | SHALL support DoIP vehicle discovery | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| COM-REQ-01 | COM-REQ-01 | SHALL support signal-based I-PDU communication | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| PDUR-REQ-01 | PDUR-REQ-01 | SHALL route I-PDUs between COM and transport layers | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CANSM-REQ-01 | CANSM-REQ-01 | SHALL manage CAN network state machine | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| LIN-REQ-01 | LIN-REQ-01 | SHALL support LIN master/slave communication | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| NVM-REQ-01 | NVM-REQ-01 | SHALL support NV block read/write with redundancy | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| FEE-REQ-01 | FEE-REQ-01 | SHALL emulate EEPROM over flash with wear leveling | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| MEMIF-REQ-01 | MEMIF-REQ-01 | SHALL abstract NvM from Fee/EEP driver | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| ECUM-REQ-01 | ECUM-REQ-01 | SHALL manage ECU startup/shutdown/wakeup sequences | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| BSWM-REQ-01 | BSWM-REQ-01 | SHALL implement mode-based BSW scheduling | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| WDGM-REQ-01 | WDGM-REQ-01 | SHALL monitor alive and deadline supervision | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| OS-REQ-01 | OS-REQ-01 | SHALL provide AUTOSAR OS SC4 compliant scheduling | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| E2E-REQ-01 | E2E-REQ-01 | SHALL protect safety-critical signals with CRC and sequence counter | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| CSM-REQ-01 | CSM-REQ-01 | SHALL provide cryptographic service management | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |
| KEYM-REQ-01 | KEYM-REQ-01 | SHALL manage cryptographic keys | Unit Test | tests/unit/test_services_api_contracts.c | — |  | ✅ |

## Summary
- Total SHALL statements: 127
- Covered by tests: 127 (100%)
- Uncovered: 0
- Threshold: 100% → ✅ PASS
