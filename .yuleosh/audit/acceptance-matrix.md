# Acceptance Matrix

> Generated: 2026-08-07
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
| None | None | SHALL 使用 MISRA C:2023 `safety` 配置 | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 4 concurrent diagnostic sessions. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL enforce P2 timeout of 50ms for diagnostic responses. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL enforce P2\* timeout of 500ms for diagnostic responses. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs). | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 3 event priority levels: Low, Medium, High. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL store diagnostic events with primary and secondary (freeze frame) data. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide a configurable aging counter with default 40 cycles. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a configurable signal count with default of 1024 signals. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support signal group communication. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support I-PDU send and receive directions. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support deadline monitoring for signal transmission. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL maintain a static routing table generated at build time. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 512 routing paths. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support native, redundant, and dataset NVM block management. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL use CRC-32 for write verification. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support block sizes from 1 to 65536 bytes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 512 NVM blocks. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 4 job priority levels. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support shutdown targets OFF, RESET, and SLEEP. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide fixed cyclic schedule tables. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support BCC2 and ECC2 task conformance classes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 64 tasks. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 32 alarms. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL implement the priority ceiling protocol for resource management. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1). | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support up to 512 PDU IDs. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support transmit and receive PDU modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support sleep and wakeup functionality. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL implement the ISO 15765-2 CAN transport protocol. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support message segmentation up to 4095 bytes per message. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Continuous and Wait flow control modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Physical and Functional addressing. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL implement AUTOSAR CAN Network Management protocol. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a configurable 8-bit node ID. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support configurable message cycle time with default of 100ms. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support configurable repeat message timer with default of 1000ms. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support bus synchronization. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a maximum of 32 sockets. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support TCP and UDP protocols. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Server and Client connection types. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support SOME/IP protocol communication. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support an offer cycle of 1000ms for service discovery. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support a request cycle of 2000ms for service discovery. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support TTL multiplier of 3 for service entries. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support TCP and Serial transport for DLT messages. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Application ID filtering. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support CAN and Ethernet transport layers for XCP. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL implement XCP protocol version 1.5. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support XCP slave functionality. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support calibration page switching. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support up to 8 DAQ lists. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 10-bit and 12-bit configurable ADC resolution. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Single, Continuous, and Scan conversion modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support up to 16 channels per ADC instance. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support left and right result alignment. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support interrupt-based and polling notification modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support Classical CAN (2.0B) and CAN FD protocols. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide 64 mailboxes for CAN message buffering. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support FIFO mode for CAN message reception. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support loopback mode for self-test. | Unit Test | (tested) | — |  | ✅ |
| None | None | The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support SHA-256 hashing. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support ECC P-256 elliptic curve cryptography. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL accelerate cryptographic operations using S32K312 HSM. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL store cryptographic keys in HSM secure NVM. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide integrated hardware TRNG. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide MbedTLS fallback for SIL simulation. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 8 ports with 32 pins each for digital I/O. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support configurable pin direction per pin. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support HIGH and LEVEL output levels. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support edge-triggered interrupt on rising, falling, and both edges. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support pin mux configuration for approximately 100 pins. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support ALT0 through ALT7 mux modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide 8 hardware timer channels. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide 32-bit timer resolution. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support prescaler values from 1 to 65536. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support one-shot and continuous timer modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support up to 8 input capture channels. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support signal period, duty cycle, and pulse width measurement. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support rising, falling, and both edge detection. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 4 RAM section banks. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support POR, WDG, SW, and External reset sources. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide configurable watchdog timeout from milliseconds to seconds. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support window mode watchdog operation. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support test mode for diagnostic testing. | Unit Test | — | — |  | ❌ |
| DCM-REQ-01 | DCM-REQ-01 | SHALL support ISO 14229-1 UDS diagnostic services | Unit Test | — | — |  | ❌ |
| DCM-REQ-02 | DCM-REQ-02 | SHALL support session management (default, programming, extended) | Unit Test | — | — |  | ❌ |
| DEM-REQ-01 | DEM-REQ-01 | SHALL support DTC storage and retrieval | Unit Test | — | — |  | ❌ |
| DET-REQ-01 | DET-REQ-01 | SHALL report development errors with module ID and error code | Unit Test | — | — |  | ❌ |
| DOIP-REQ-01 | DOIP-REQ-01 | SHALL support DoIP vehicle discovery | Unit Test | — | — |  | ❌ |
| COM-REQ-01 | COM-REQ-01 | SHALL support signal-based I-PDU communication | Unit Test | — | — |  | ❌ |
| PDUR-REQ-01 | PDUR-REQ-01 | SHALL route I-PDUs between COM and transport layers | Unit Test | — | — |  | ❌ |
| CANSM-REQ-01 | CANSM-REQ-01 | SHALL manage CAN network state machine | Unit Test | — | — |  | ❌ |
| LIN-REQ-01 | LIN-REQ-01 | SHALL support LIN master/slave communication | Unit Test | — | — |  | ❌ |
| NVM-REQ-01 | NVM-REQ-01 | SHALL support NV block read/write with redundancy | Unit Test | — | — |  | ❌ |
| FEE-REQ-01 | FEE-REQ-01 | SHALL emulate EEPROM over flash with wear leveling | Unit Test | — | — |  | ❌ |
| MEMIF-REQ-01 | MEMIF-REQ-01 | SHALL abstract NvM from Fee/EEP driver | Unit Test | — | — |  | ❌ |
| ECUM-REQ-01 | ECUM-REQ-01 | SHALL manage ECU startup/shutdown/wakeup sequences | Unit Test | — | — |  | ❌ |
| BSWM-REQ-01 | BSWM-REQ-01 | SHALL implement mode-based BSW scheduling | Unit Test | — | — |  | ❌ |
| WDGM-REQ-01 | WDGM-REQ-01 | SHALL monitor alive and deadline supervision | Unit Test | — | — |  | ❌ |
| OS-REQ-01 | OS-REQ-01 | SHALL provide AUTOSAR OS SC4 compliant scheduling | Unit Test | — | — |  | ❌ |
| E2E-REQ-01 | E2E-REQ-01 | SHALL protect safety-critical signals with CRC and sequence counter | Unit Test | — | — |  | ❌ |
| CSM-REQ-01 | CSM-REQ-01 | SHALL provide cryptographic service management | Unit Test | — | — |  | ❌ |
| KEYM-REQ-01 | KEYM-REQ-01 | SHALL manage cryptographic keys | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide a UART driver with configurable baud rate from 9600 to 921600 bps. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 8-bit, 9-bit, and 10-bit data frame formats. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support even, odd, and no parity modes. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support 1 and 2 stop bits. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide interrupt-based transmit and receive notification. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL provide a ring buffer for received data with configurable depth. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL support hardware flow control via RTS and CTS signals. | Unit Test | — | — |  | ❌ |
| None | None | The system SHALL report transmission errors including overrun, framing, and parity errors. | Unit Test | — | — |  | ❌ |
| None | None | THEN the driver SHALL return success status | Unit Test | — | — |  | ❌ |
| None | None | AND the channel SHALL be ready for transmit and receive operations | Unit Test | — | — |  | ❌ |
| None | None | THEN the driver SHALL store the bytes in the ring buffer | Unit Test | — | — |  | ❌ |
| None | None | AND the driver SHALL notify the application via the registered callback | Unit Test | — | — |  | ❌ |
| None | None | THEN the driver SHALL set the framing error flag | Unit Test | — | — |  | ❌ |
| None | None | AND the driver SHALL report the error through the error callback | Unit Test | — | — |  | ❌ |

## Summary
- Total SHALL statements: 141
- Covered by tests: 1 (0%)
- Uncovered: 140
- Threshold: 100% → ❌ FAIL
