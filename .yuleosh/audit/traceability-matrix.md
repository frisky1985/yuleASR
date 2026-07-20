# Traceability Matrix

> Generated: 2026-07-20T03:53:25
> Version: 0.1.0

## Requirements → Implementation → Tests

### MCAL-SHALL-001
- Req ID: MCAL-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write)

### MCAL-SHALL-002
- Req ID: MCAL-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式

### MCAL-SHALL-003
- Req ID: MCAL-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 使用 MISRA C:2023 合规编码风格

### ECUAL-SHALL-001
- Req ID: ECUAL-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器

### ECUAL-SHALL-002
- Req ID: ECUAL-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 看门狗管理器 SHALL 在超时前刷新

### SVC-SHALL-001
- Req ID: SVC-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务

### SVC-SHALL-002
- Req ID: SVC-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 通信栈 (CAN/以太网) SHALL 实现 PDU 路由

### SVC-SHALL-003
- Req ID: SVC-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 诊断事件管理器 (Dem) SHALL 记录并上报 DTC

### NFR-SHALL-001
- Req ID: NFR-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 代码 MISRA C:2023 合规

### NFR-SHALL-002
- Req ID: NFR-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 单元测试行覆盖率

### NFR-SHALL-003
- Req ID: NFR-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 条件覆盖率

### NFR-SHALL-004
- Req ID: NFR-SHALL-004
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 静态分析 (cppcheck)

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL 使用 MISRA C:2023 `safety` 配置

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 4 concurrent diagnostic sessions.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL enforce P2 timeout of 50ms for diagnostic responses.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL enforce P2\* timeout of 500ms for diagnostic responses.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs).

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support 3 event priority levels: Low, Medium, High.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL store diagnostic events with primary and secondary (freeze frame) data.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide a configurable aging counter with default 40 cycles.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a configurable signal count with default of 1024 signals.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support signal group communication.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support I-PDU send and receive directions.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support deadline monitoring for signal transmission.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL maintain a static routing table generated at build time.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 512 routing paths.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support native, redundant, and dataset NVM block management.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL use CRC-32 for write verification.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support block sizes from 1 to 65536 bytes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 512 NVM blocks.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support 4 job priority levels.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support shutdown targets OFF, RESET, and SLEEP.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide fixed cyclic schedule tables.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support BCC2 and ECC2 task conformance classes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 64 tasks.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 32 alarms.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL implement the priority ceiling protocol for resource management.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1).

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support up to 512 PDU IDs.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support transmit and receive PDU modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support sleep and wakeup functionality.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL implement the ISO 15765-2 CAN transport protocol.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support message segmentation up to 4095 bytes per message.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Continuous and Wait flow control modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Physical and Functional addressing.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL implement AUTOSAR CAN Network Management protocol.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a configurable 8-bit node ID.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support configurable message cycle time with default of 100ms.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support configurable repeat message timer with default of 1000ms.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support bus synchronization.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a maximum of 32 sockets.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support TCP and UDP protocols.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Server and Client connection types.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support SOME/IP protocol communication.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support an offer cycle of 1000ms for service discovery.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support a request cycle of 2000ms for service discovery.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support TTL multiplier of 3 for service entries.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support TCP and Serial transport for DLT messages.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Application ID filtering.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support CAN and Ethernet transport layers for XCP.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL implement XCP protocol version 1.5.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support XCP slave functionality.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support calibration page switching.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support up to 8 DAQ lists.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support 10-bit and 12-bit configurable ADC resolution.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Single, Continuous, and Scan conversion modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support up to 16 channels per ADC instance.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support left and right result alignment.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support interrupt-based and polling notification modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support Classical CAN (2.0B) and CAN FD protocols.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide 64 mailboxes for CAN message buffering.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support FIFO mode for CAN message reception.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support loopback mode for self-test.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support SHA-256 hashing.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support ECC P-256 elliptic curve cryptography.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL accelerate cryptographic operations using S32K312 HSM.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL store cryptographic keys in HSM secure NVM.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide integrated hardware TRNG.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide MbedTLS fallback for SIL simulation.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support 8 ports with 32 pins each for digital I/O.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support configurable pin direction per pin.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support HIGH and LEVEL output levels.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support edge-triggered interrupt on rising, falling, and both edges.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support pin mux configuration for approximately 100 pins.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support ALT0 through ALT7 mux modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide 8 hardware timer channels.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide 32-bit timer resolution.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support prescaler values from 1 to 65536.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support one-shot and continuous timer modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support up to 8 input capture channels.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support signal period, duty cycle, and pulse width measurement.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support rising, falling, and both edge detection.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support 4 RAM section banks.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support POR, WDG, SW, and External reset sources.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL provide configurable watchdog timeout from milliseconds to seconds.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support window mode watchdog operation.

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ The system SHALL support test mode for diagnostic testing.

### DCM-REQ-01
- Req ID: DCM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support ISO 14229-1 UDS diagnostic services

### DCM-REQ-02
- Req ID: DCM-REQ-02
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support session management (default, programming, extended)

### DEM-REQ-01
- Req ID: DEM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support DTC storage and retrieval

### DET-REQ-01
- Req ID: DET-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL report development errors with module ID and error code

### DOIP-REQ-01
- Req ID: DOIP-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support DoIP vehicle discovery

### COM-REQ-01
- Req ID: COM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support signal-based I-PDU communication

### PDUR-REQ-01
- Req ID: PDUR-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL route I-PDUs between COM and transport layers

### CANSM-REQ-01
- Req ID: CANSM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage CAN network state machine

### LIN-REQ-01
- Req ID: LIN-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support LIN master/slave communication

### NVM-REQ-01
- Req ID: NVM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support NV block read/write with redundancy

### FEE-REQ-01
- Req ID: FEE-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL emulate EEPROM over flash with wear leveling

### MEMIF-REQ-01
- Req ID: MEMIF-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL abstract NvM from Fee/EEP driver

### ECUM-REQ-01
- Req ID: ECUM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage ECU startup/shutdown/wakeup sequences

### BSWM-REQ-01
- Req ID: BSWM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement mode-based BSW scheduling

### WDGM-REQ-01
- Req ID: WDGM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL monitor alive and deadline supervision

### OS-REQ-01
- Req ID: OS-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide AUTOSAR OS SC4 compliant scheduling

### E2E-REQ-01
- Req ID: E2E-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL protect safety-critical signals with CRC and sequence counter

### CSM-REQ-01
- Req ID: CSM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide cryptographic service management

### KEYM-REQ-01
- Req ID: KEYM-REQ-01
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage cryptographic keys

## Summary
- Total Requirements: 127
- Requirements with implementation: 85 (66%)
- Requirements with test coverage: 0 (0%)
- Uncovered SHALLs: 127
- Scenarios: 0
- Reviews: 0
- CI Runs: 0
