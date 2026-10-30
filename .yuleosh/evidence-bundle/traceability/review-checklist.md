# Requirement Review Checklist

> Auto-generated from `traceability-report.json` (127 requirements).

> **Honesty rule**: reviews are human process artifacts. A requirement is

> counted as reviewed ONLY when a human runs `mark_requirement_reviewed.py`

> with `--decision approve` and a real reviewer + date. Pending rows are

> gaps — do not mark them approved without an actual review.


## How to use

1. Pick a requirement row below.

2. Read the statement and its `code_files` / `matched_tests` in the report.

3. Perform the review (code inspection, test check, etc.).

4. Record the decision with the CLI tool:

   ```

   python3 tools/mark_requirement_reviewed.py <REQ_ID> \

       --reviewer "<name>" --date YYYY-MM-DD \

       --decision approve [--findings "..."]

   ```

5. Re-run this generator to refresh the checklist badges.


## Checklist

| # | Req ID | Section | Statement | Status | Reviewer | Date | Decision | Findings |
|--:|:-------|:--------|:----------|:-------|:---------|:-----|:---------|:---------|
| 1 | `MCAL-SHALL-001` | 3.1 MCAL 模块 | MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write) | ⭕ pending |  |  |  |  |
| 2 | `MCAL-SHALL-002` | 3.1 MCAL 模块 | 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式 | ⭕ pending |  |  |  |  |
| 3 | `MCAL-SHALL-003` | 3.1 MCAL 模块 | MCAL SHALL 使用 MISRA C:2023 合规编码风格 | ⭕ pending |  |  |  |  |
| 4 | `ECUAL-SHALL-001` | 3.2 ECUAL 模块 | ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器 | ⭕ pending |  |  |  |  |
| 5 | `ECUAL-SHALL-002` | 3.2 ECUAL 模块 | 看门狗管理器 SHALL 在超时前刷新 | ⭕ pending |  |  |  |  |
| 6 | `SVC-SHALL-001` | 3.3 Services 层 | OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务 | ⭕ pending |  |  |  |  |
| 7 | `SVC-SHALL-002` | 3.3 Services 层 | 通信栈 (CAN/以太网) SHALL 实现 PDU 路由 | ⭕ pending |  |  |  |  |
| 8 | `SVC-SHALL-003` | 3.3 Services 层 | 诊断事件管理器 (Dem) SHALL 记录并上报 DTC | ⭕ pending |  |  |  |  |
| 9 | `NFR-SHALL-001` | 4. 非功能需求 | 代码 MISRA C:2023 合规 | ⭕ pending |  |  |  |  |
| 10 | `NFR-SHALL-002` | 4. 非功能需求 | 单元测试行覆盖率 | ⭕ pending |  |  |  |  |
| 11 | `NFR-SHALL-003` | 4. 非功能需求 | 条件覆盖率 | ⭕ pending |  |  |  |  |
| 12 | `NFR-SHALL-004` | 4. 非功能需求 | 静态分析 (cppcheck) | ⭕ pending |  |  |  |  |
| 13 | `MISRA-SHALL-001` | 7. MISRA 合规策略 | SHALL 使用 MISRA C:2023 `safety` 配置 | ⭕ pending |  |  |  |  |
| 14 | `DCM-SHALL-001` | DCM (Diagnostic Communication Manager) | The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x… | ⭕ pending |  |  |  |  |
| 15 | `DCM-SHALL-002` | DCM (Diagnostic Communication Manager) | The system SHALL support a maximum of 4 concurrent diagnostic sessions. | ⭕ pending |  |  |  |  |
| 16 | `DCM-SHALL-003` | DCM (Diagnostic Communication Manager) | The system SHALL enforce P2 timeout of 50ms for diagnostic responses. | ⭕ pending |  |  |  |  |
| 17 | `DCM-SHALL-004` | DCM (Diagnostic Communication Manager) | The system SHALL enforce P2\* timeout of 500ms for diagnostic responses. | ⭕ pending |  |  |  |  |
| 18 | `DEM-SHALL-001` | DEM (Diagnostic Event Manager) | The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs). | ⭕ pending |  |  |  |  |
| 19 | `DEM-SHALL-002` | DEM (Diagnostic Event Manager) | The system SHALL support 3 event priority levels: Low, Medium, High. | ⭕ pending |  |  |  |  |
| 20 | `DEM-SHALL-003` | DEM (Diagnostic Event Manager) | The system SHALL store diagnostic events with primary and secondary (freeze fra… | ⭕ pending |  |  |  |  |
| 21 | `DEM-SHALL-004` | DEM (Diagnostic Event Manager) | The system SHALL provide a configurable aging counter with default 40 cycles. | ⭕ pending |  |  |  |  |
| 22 | `COM-SHALL-001` | COM | The system SHALL support a configurable signal count with default of 1024 signa… | ⭕ pending |  |  |  |  |
| 23 | `COM-SHALL-002` | COM | The system SHALL support signal group communication. | ⭕ pending |  |  |  |  |
| 24 | `COM-SHALL-003` | COM | The system SHALL support I-PDU send and receive directions. | ⭕ pending |  |  |  |  |
| 25 | `COM-SHALL-004` | COM | The system SHALL support deadline monitoring for signal transmission. | ⭕ pending |  |  |  |  |
| 26 | `PDUR-SHALL-001` | PduR | The system SHALL maintain a static routing table generated at build time. | ⭕ pending |  |  |  |  |
| 27 | `PDUR-SHALL-002` | PduR | The system SHALL support a maximum of 512 routing paths. | ⭕ pending |  |  |  |  |
| 28 | `PDUR-SHALL-003` | PduR | The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet. | ⭕ pending |  |  |  |  |
| 29 | `NVM-SHALL-001` | NvM | The system SHALL support native, redundant, and dataset NVM block management. | ⭕ pending |  |  |  |  |
| 30 | `NVM-SHALL-002` | NvM | The system SHALL use CRC-32 for write verification. | ⭕ pending |  |  |  |  |
| 31 | `NVM-SHALL-003` | NvM | The system SHALL support block sizes from 1 to 65536 bytes. | ⭕ pending |  |  |  |  |
| 32 | `NVM-SHALL-004` | NvM | The system SHALL support a maximum of 512 NVM blocks. | ⭕ pending |  |  |  |  |
| 33 | `NVM-SHALL-005` | NvM | The system SHALL support 4 job priority levels. | ⭕ pending |  |  |  |  |
| 34 | `ECUM-SHALL-001` | EcuM | The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO. | ⭕ pending |  |  |  |  |
| 35 | `ECUM-SHALL-002` | EcuM | The system SHALL support shutdown targets OFF, RESET, and SLEEP. | ⭕ pending |  |  |  |  |
| 36 | `ECUM-SHALL-003` | EcuM | The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and … | ⭕ pending |  |  |  |  |
| 37 | `OSSC4-SHALL-001` | OS (AUTOSAR SC4) | The system SHALL provide fixed cyclic schedule tables. | ⭕ pending |  |  |  |  |
| 38 | `OSSC4-SHALL-002` | OS (AUTOSAR SC4) | The system SHALL support BCC2 and ECC2 task conformance classes. | ⭕ pending |  |  |  |  |
| 39 | `OSSC4-SHALL-003` | OS (AUTOSAR SC4) | The system SHALL support a maximum of 64 tasks. | ⭕ pending |  |  |  |  |
| 40 | `OSSC4-SHALL-004` | OS (AUTOSAR SC4) | The system SHALL support a maximum of 32 alarms. | ⭕ pending |  |  |  |  |
| 41 | `OSSC4-SHALL-005` | OS (AUTOSAR SC4) | The system SHALL implement the priority ceiling protocol for resource managemen… | ⭕ pending |  |  |  |  |
| 42 | `CANIF-SHALL-001` | CanIf (CAN Interface) | The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1). | ⭕ pending |  |  |  |  |
| 43 | `CANIF-SHALL-002` | CanIf (CAN Interface) | The system SHALL support up to 512 PDU IDs. | ⭕ pending |  |  |  |  |
| 44 | `CANIF-SHALL-003` | CanIf (CAN Interface) | The system SHALL support transmit and receive PDU modes. | ⭕ pending |  |  |  |  |
| 45 | `CANIF-SHALL-004` | CanIf (CAN Interface) | The system SHALL support sleep and wakeup functionality. | ⭕ pending |  |  |  |  |
| 46 | `CANTP-SHALL-001` | CanTp (CAN Transport Protocol) | The system SHALL implement the ISO 15765-2 CAN transport protocol. | ⭕ pending |  |  |  |  |
| 47 | `CANTP-SHALL-002` | CanTp (CAN Transport Protocol) | The system SHALL support message segmentation up to 4095 bytes per message. | ⭕ pending |  |  |  |  |
| 48 | `CANTP-SHALL-003` | CanTp (CAN Transport Protocol) | The system SHALL support Continuous and Wait flow control modes. | ⭕ pending |  |  |  |  |
| 49 | `CANTP-SHALL-004` | CanTp (CAN Transport Protocol) | The system SHALL support Physical and Functional addressing. | ⭕ pending |  |  |  |  |
| 50 | `CANNM-SHALL-001` | CanNm (CAN Network Management) | The system SHALL implement AUTOSAR CAN Network Management protocol. | ⭕ pending |  |  |  |  |
| 51 | `CANNM-SHALL-002` | CanNm (CAN Network Management) | The system SHALL support a configurable 8-bit node ID. | ⭕ pending |  |  |  |  |
| 52 | `CANNM-SHALL-003` | CanNm (CAN Network Management) | The system SHALL support configurable message cycle time with default of 100ms. | ⭕ pending |  |  |  |  |
| 53 | `CANNM-SHALL-004` | CanNm (CAN Network Management) | The system SHALL support configurable repeat message timer with default of 1000… | ⭕ pending |  |  |  |  |
| 54 | `CANNM-SHALL-005` | CanNm (CAN Network Management) | The system SHALL support bus synchronization. | ⭕ pending |  |  |  |  |
| 55 | `SOAD-SHALL-001` | SoAd (Socket Adaptor) | The system SHALL support a maximum of 32 sockets. | ⭕ pending |  |  |  |  |
| 56 | `SOAD-SHALL-002` | SoAd (Socket Adaptor) | The system SHALL support TCP and UDP protocols. | ⭕ pending |  |  |  |  |
| 57 | `SOAD-SHALL-003` | SoAd (Socket Adaptor) | The system SHALL support Server and Client connection types. | ⭕ pending |  |  |  |  |
| 58 | `SOAD-SHALL-004` | SoAd (Socket Adaptor) | The system SHALL support SOME/IP protocol communication. | ⭕ pending |  |  |  |  |
| 59 | `SOMEIPSD-SHALL-001` | SomeIpSd (SOME/IP Service Discovery) | The system SHALL support an offer cycle of 1000ms for service discovery. | ⭕ pending |  |  |  |  |
| 60 | `SOMEIPSD-SHALL-002` | SomeIpSd (SOME/IP Service Discovery) | The system SHALL support a request cycle of 2000ms for service discovery. | ⭕ pending |  |  |  |  |
| 61 | `SOMEIPSD-SHALL-003` | SomeIpSd (SOME/IP Service Discovery) | The system SHALL support TTL multiplier of 3 for service entries. | ⭕ pending |  |  |  |  |
| 62 | `DLT-SHALL-001` | DLT | The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, … | ⭕ pending |  |  |  |  |
| 63 | `DLT-SHALL-002` | DLT | The system SHALL support TCP and Serial transport for DLT messages. | ⭕ pending |  |  |  |  |
| 64 | `DLT-SHALL-003` | DLT | The system SHALL support Application ID filtering. | ⭕ pending |  |  |  |  |
| 65 | `XCP-SHALL-001` | XCP | The system SHALL support CAN and Ethernet transport layers for XCP. | ⭕ pending |  |  |  |  |
| 66 | `XCP-SHALL-002` | XCP | The system SHALL implement XCP protocol version 1.5. | ⭕ pending |  |  |  |  |
| 67 | `XCP-SHALL-003` | XCP | The system SHALL support XCP slave functionality. | ⭕ pending |  |  |  |  |
| 68 | `XCP-SHALL-004` | XCP | The system SHALL support calibration page switching. | ⭕ pending |  |  |  |  |
| 69 | `XCP-SHALL-005` | XCP | The system SHALL support up to 8 DAQ lists. | ⭕ pending |  |  |  |  |
| 70 | `ADC-SHALL-001` | ADC Driver | The system SHALL support 10-bit and 12-bit configurable ADC resolution. | ⭕ pending |  |  |  |  |
| 71 | `ADC-SHALL-002` | ADC Driver | The system SHALL support Single, Continuous, and Scan conversion modes. | ⭕ pending |  |  |  |  |
| 72 | `ADC-SHALL-003` | ADC Driver | The system SHALL support up to 16 channels per ADC instance. | ⭕ pending |  |  |  |  |
| 73 | `ADC-SHALL-004` | ADC Driver | The system SHALL support left and right result alignment. | ⭕ pending |  |  |  |  |
| 74 | `ADC-SHALL-005` | ADC Driver | The system SHALL support interrupt-based and polling notification modes. | ⭕ pending |  |  |  |  |
| 75 | `CANDRV-SHALL-001` | CAN Driver | The system SHALL support Classical CAN (2.0B) and CAN FD protocols. | ⭕ pending |  |  |  |  |
| 76 | `CANDRV-SHALL-002` | CAN Driver | The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbp… | ⭕ pending |  |  |  |  |
| 77 | `CANDRV-SHALL-003` | CAN Driver | The system SHALL provide 64 mailboxes for CAN message buffering. | ⭕ pending |  |  |  |  |
| 78 | `CANDRV-SHALL-004` | CAN Driver | The system SHALL support FIFO mode for CAN message reception. | ⭕ pending |  |  |  |  |
| 79 | `CANDRV-SHALL-005` | CAN Driver | The system SHALL support loopback mode for self-test. | ⭕ pending |  |  |  |  |
| 80 | `CANDRV-SHALL-006` | CAN Driver | The system SHALL provide automatic bus-off recovery conforming to AUTOSAR speci… | ⭕ pending |  |  |  |  |
| 81 | `CRYPTO-SHALL-001` | Crypto Driver | The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes. | ⭕ pending |  |  |  |  |
| 82 | `CRYPTO-SHALL-002` | Crypto Driver | The system SHALL support SHA-256 hashing. | ⭕ pending |  |  |  |  |
| 83 | `CRYPTO-SHALL-003` | Crypto Driver | The system SHALL support ECC P-256 elliptic curve cryptography. | ⭕ pending |  |  |  |  |
| 84 | `CRYPTO-SHALL-004` | Crypto Driver | The system SHALL accelerate cryptographic operations using S32K312 HSM. | ⭕ pending |  |  |  |  |
| 85 | `CRYPTO-SHALL-005` | Crypto Driver | The system SHALL store cryptographic keys in HSM secure NVM. | ⭕ pending |  |  |  |  |
| 86 | `CRYPTO-SHALL-006` | Crypto Driver | The system SHALL provide integrated hardware TRNG. | ⭕ pending |  |  |  |  |
| 87 | `CRYPTO-SHALL-007` | Crypto Driver | The system SHALL provide MbedTLS fallback for SIL simulation. | ⭕ pending |  |  |  |  |
| 88 | `DIODRV-SHALL-001` | DIO Driver | The system SHALL support 8 ports with 32 pins each for digital I/O. | ⭕ pending |  |  |  |  |
| 89 | `DIODRV-SHALL-002` | DIO Driver | The system SHALL support configurable pin direction per pin. | ⭕ pending |  |  |  |  |
| 90 | `DIODRV-SHALL-003` | DIO Driver | The system SHALL support HIGH and LEVEL output levels. | ⭕ pending |  |  |  |  |
| 91 | `DIODRV-SHALL-004` | DIO Driver | The system SHALL support edge-triggered interrupt on rising, falling, and both … | ⭕ pending |  |  |  |  |
| 92 | `PORTDRV-SHALL-001` | PORT Driver | The system SHALL support pin mux configuration for approximately 100 pins. | ⭕ pending |  |  |  |  |
| 93 | `PORTDRV-SHALL-002` | PORT Driver | The system SHALL support ALT0 through ALT7 mux modes. | ⭕ pending |  |  |  |  |
| 94 | `PORTDRV-SHALL-003` | PORT Driver | The system SHALL support configurable pad properties including pull-up, pull-do… | ⭕ pending |  |  |  |  |
| 95 | `GPTDRV-SHALL-001` | GPT Driver | The system SHALL provide 8 hardware timer channels. | ⭕ pending |  |  |  |  |
| 96 | `GPTDRV-SHALL-002` | GPT Driver | The system SHALL provide 32-bit timer resolution. | ⭕ pending |  |  |  |  |
| 97 | `GPTDRV-SHALL-003` | GPT Driver | The system SHALL support prescaler values from 1 to 65536. | ⭕ pending |  |  |  |  |
| 98 | `GPTDRV-SHALL-004` | GPT Driver | The system SHALL support one-shot and continuous timer modes. | ⭕ pending |  |  |  |  |
| 99 | `ICURV-SHALL-001` | ICU Driver | The system SHALL support up to 8 input capture channels. | ⭕ pending |  |  |  |  |
| 100 | `ICURV-SHALL-002` | ICU Driver | The system SHALL support signal period, duty cycle, and pulse width measurement. | ⭕ pending |  |  |  |  |
| 101 | `ICURV-SHALL-003` | ICU Driver | The system SHALL support rising, falling, and both edge detection. | ⭕ pending |  |  |  |  |
| 102 | `MCUDRV-SHALL-001` | MCU Driver | The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL. | ⭕ pending |  |  |  |  |
| 103 | `MCUDRV-SHALL-002` | MCU Driver | The system SHALL support 4 RAM section banks. | ⭕ pending |  |  |  |  |
| 104 | `MCUDRV-SHALL-003` | MCU Driver | The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes. | ⭕ pending |  |  |  |  |
| 105 | `MCUDRV-SHALL-004` | MCU Driver | The system SHALL support POR, WDG, SW, and External reset sources. | ⭕ pending |  |  |  |  |
| 106 | `WDGDRV-SHALL-001` | WDG Driver | The system SHALL provide configurable watchdog timeout from milliseconds to sec… | ⭕ pending |  |  |  |  |
| 107 | `WDGDRV-SHALL-002` | WDG Driver | The system SHALL support window mode watchdog operation. | ⭕ pending |  |  |  |  |
| 108 | `WDGDRV-SHALL-003` | WDG Driver | The system SHALL support test mode for diagnostic testing. | ⭕ pending |  |  |  |  |
| 109 | `DCM-REQ-01` | Diagnostic Services | SHALL support ISO 14229-1 UDS diagnostic services | ⭕ pending |  |  |  |  |
| 110 | `DCM-REQ-02` | Diagnostic Services | SHALL support session management (default, programming, extended) | ⭕ pending |  |  |  |  |
| 111 | `DEM-REQ-01` | Diagnostic Services | SHALL support DTC storage and retrieval | ⭕ pending |  |  |  |  |
| 112 | `DET-REQ-01` | Diagnostic Services | SHALL report development errors with module ID and error code | ⭕ pending |  |  |  |  |
| 113 | `DOIP-REQ-01` | Diagnostic Services | SHALL support DoIP vehicle discovery | ⭕ pending |  |  |  |  |
| 114 | `COM-REQ-01` | Communication Services | SHALL support signal-based I-PDU communication | ⭕ pending |  |  |  |  |
| 115 | `PDUR-REQ-01` | Communication Services | SHALL route I-PDUs between COM and transport layers | ⭕ pending |  |  |  |  |
| 116 | `CANSM-REQ-01` | Communication Services | SHALL manage CAN network state machine | ⭕ pending |  |  |  |  |
| 117 | `LIN-REQ-01` | Communication Services | SHALL support LIN master/slave communication | ⭕ pending |  |  |  |  |
| 118 | `NVM-REQ-01` | Memory Services | SHALL support NV block read/write with redundancy | ⭕ pending |  |  |  |  |
| 119 | `FEE-REQ-01` | Memory Services | SHALL emulate EEPROM over flash with wear leveling | ⭕ pending |  |  |  |  |
| 120 | `MEMIF-REQ-01` | Memory Services | SHALL abstract NvM from Fee/EEP driver | ⭕ pending |  |  |  |  |
| 121 | `ECUM-REQ-01` | System Services | SHALL manage ECU startup/shutdown/wakeup sequences | ⭕ pending |  |  |  |  |
| 122 | `BSWM-REQ-01` | System Services | SHALL implement mode-based BSW scheduling | ⭕ pending |  |  |  |  |
| 123 | `WDGM-REQ-01` | System Services | SHALL monitor alive and deadline supervision | ⭕ pending |  |  |  |  |
| 124 | `OS-REQ-01` | System Services | SHALL provide AUTOSAR OS SC4 compliant scheduling | ⭕ pending |  |  |  |  |
| 125 | `E2E-REQ-01` | Safety & Security | SHALL protect safety-critical signals with CRC and sequence counter | ⭕ pending |  |  |  |  |
| 126 | `CSM-REQ-01` | Safety & Security | SHALL provide cryptographic service management | ⭕ pending |  |  |  |  |
| 127 | `KEYM-REQ-01` | Safety & Security | SHALL manage cryptographic keys | ⭕ pending |  |  |  |  |

## Summary

- Total requirements: **127**

- ✅ Approved: **0**

- ⭕ Pending: **127**

- Review progress: **0/127** (0.0%)
