# Traceability Matrix

> Generated: 2026-07-22T11:46:56.177898
> Version: 0.1.0

## Requirements → Implementation → Tests

### MCAL-SHALL-001
- Req ID: MCAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write)

### MCAL-SHALL-002
- Req ID: MCAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式

### MCAL-SHALL-003
- Req ID: MCAL-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 使用 MISRA C:2023 合规编码风格

### ECUAL-SHALL-001
- Req ID: ECUAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器

### ECUAL-SHALL-002
- Req ID: ECUAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 看门狗管理器 SHALL 在超时前刷新

### SVC-SHALL-001
- Req ID: SVC-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务

### SVC-SHALL-002
- Req ID: SVC-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 通信栈 (CAN/以太网) SHALL 实现 PDU 路由

### SVC-SHALL-003
- Req ID: SVC-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 诊断事件管理器 (Dem) SHALL 记录并上报 DTC

### NFR-SHALL-001
- Req ID: NFR-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 代码 MISRA C:2023 合规

### NFR-SHALL-002
- Req ID: NFR-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 单元测试行覆盖率

### NFR-SHALL-003
- Req ID: NFR-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 条件覆盖率

### NFR-SHALL-004
- Req ID: NFR-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 静态分析 (cppcheck)

### 7MISRA-SHALL-13
- Req ID: 7MISRA-SHALL-13
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL 使用 MISRA C:2023 `safety` 配置

### DCMDIAGN-SHALL-1
- Req ID: DCMDIAGN-SHALL-1
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x...

### DCMDIAGN-SHALL-2
- Req ID: DCMDIAGN-SHALL-2
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 4 concurrent diagnostic sessions.

### DCMDIAGN-SHALL-3
- Req ID: DCMDIAGN-SHALL-3
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL enforce P2 timeout of 50ms for diagnostic responses.

### DCMDIAGN-SHALL-4
- Req ID: DCMDIAGN-SHALL-4
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL enforce P2\* timeout of 500ms for diagnostic responses.

### DEMDIAGN-SHALL-5
- Req ID: DEMDIAGN-SHALL-5
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support storage of up to 256 diagnostic trouble codes (DTCs).

### DEMDIAGN-SHALL-6
- Req ID: DEMDIAGN-SHALL-6
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support 3 event priority levels: Low, Medium, High.

### DEMDIAGN-SHALL-7
- Req ID: DEMDIAGN-SHALL-7
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL store diagnostic events with primary and secondary (freeze frame) data.

### DEMDIAGN-SHALL-8
- Req ID: DEMDIAGN-SHALL-8
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide a configurable aging counter with default 40 cycles.

### COM-SHALL-9
- Req ID: COM-SHALL-9
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a configurable signal count with default of 1024 signals.

### COM-SHALL-10
- Req ID: COM-SHALL-10
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support signal group communication.

### COM-SHALL-11
- Req ID: COM-SHALL-11
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support I-PDU send and receive directions.

### COM-SHALL-12
- Req ID: COM-SHALL-12
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support deadline monitoring for signal transmission.

### PDUR-SHALL-13
- Req ID: PDUR-SHALL-13
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL maintain a static routing table generated at build time.

### PDUR-SHALL-14
- Req ID: PDUR-SHALL-14
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 512 routing paths.

### PDUR-SHALL-15
- Req ID: PDUR-SHALL-15
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet.

### NVM-SHALL-16
- Req ID: NVM-SHALL-16
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support native, redundant, and dataset NVM block management.

### NVM-SHALL-17
- Req ID: NVM-SHALL-17
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL use CRC-32 for write verification.

### NVM-SHALL-18
- Req ID: NVM-SHALL-18
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support block sizes from 1 to 65536 bytes.

### NVM-SHALL-19
- Req ID: NVM-SHALL-19
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 512 NVM blocks.

### NVM-SHALL-20
- Req ID: NVM-SHALL-20
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support 4 job priority levels.

### ECUM-SHALL-21
- Req ID: ECUM-SHALL-21
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support startup phases STARTUP_ONE and STARTUP_TWO.

### ECUM-SHALL-22
- Req ID: ECUM-SHALL-22
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support shutdown targets OFF, RESET, and SLEEP.

### ECUM-SHALL-23
- Req ID: ECUM-SHALL-23
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer.

### OSAUTOSA-SHALL-24
- Req ID: OSAUTOSA-SHALL-24
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide fixed cyclic schedule tables.

### OSAUTOSA-SHALL-25
- Req ID: OSAUTOSA-SHALL-25
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support BCC2 and ECC2 task conformance classes.

### OSAUTOSA-SHALL-26
- Req ID: OSAUTOSA-SHALL-26
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 64 tasks.

### OSAUTOSA-SHALL-27
- Req ID: OSAUTOSA-SHALL-27
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 32 alarms.

### OSAUTOSA-SHALL-28
- Req ID: OSAUTOSA-SHALL-28
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement the priority ceiling protocol for resource management.

### CANIFCAN-SHALL-1
- Req ID: CANIFCAN-SHALL-1
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 2 CAN controllers (CAN0, CAN1).

### CANIFCAN-SHALL-2
- Req ID: CANIFCAN-SHALL-2
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support up to 512 PDU IDs.

### CANIFCAN-SHALL-3
- Req ID: CANIFCAN-SHALL-3
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support transmit and receive PDU modes.

### CANIFCAN-SHALL-4
- Req ID: CANIFCAN-SHALL-4
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support sleep and wakeup functionality.

### CANTPCAN-SHALL-5
- Req ID: CANTPCAN-SHALL-5
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement the ISO 15765-2 CAN transport protocol.

### CANTPCAN-SHALL-6
- Req ID: CANTPCAN-SHALL-6
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support message segmentation up to 4095 bytes per message.

### CANTPCAN-SHALL-7
- Req ID: CANTPCAN-SHALL-7
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Continuous and Wait flow control modes.

### CANTPCAN-SHALL-8
- Req ID: CANTPCAN-SHALL-8
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Physical and Functional addressing.

### CANNMCAN-SHALL-9
- Req ID: CANNMCAN-SHALL-9
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement AUTOSAR CAN Network Management protocol.

### CANNMCAN-SHALL-10
- Req ID: CANNMCAN-SHALL-10
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a configurable 8-bit node ID.

### CANNMCAN-SHALL-11
- Req ID: CANNMCAN-SHALL-11
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support configurable message cycle time with default of 100ms.

### CANNMCAN-SHALL-12
- Req ID: CANNMCAN-SHALL-12
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support configurable repeat message timer with default of 1000ms.

### CANNMCAN-SHALL-13
- Req ID: CANNMCAN-SHALL-13
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support bus synchronization.

### SOADSOCK-SHALL-14
- Req ID: SOADSOCK-SHALL-14
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a maximum of 32 sockets.

### SOADSOCK-SHALL-15
- Req ID: SOADSOCK-SHALL-15
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support TCP and UDP protocols.

### SOADSOCK-SHALL-16
- Req ID: SOADSOCK-SHALL-16
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Server and Client connection types.

### SOADSOCK-SHALL-17
- Req ID: SOADSOCK-SHALL-17
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support SOME/IP protocol communication.

### SOMEIPSD-SHALL-18
- Req ID: SOMEIPSD-SHALL-18
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support an offer cycle of 1000ms for service discovery.

### SOMEIPSD-SHALL-19
- Req ID: SOMEIPSD-SHALL-19
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support a request cycle of 2000ms for service discovery.

### SOMEIPSD-SHALL-20
- Req ID: SOMEIPSD-SHALL-20
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support TTL multiplier of 3 for service entries.

### DLT-SHALL-21
- Req ID: DLT-SHALL-21
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose.

### DLT-SHALL-22
- Req ID: DLT-SHALL-22
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support TCP and Serial transport for DLT messages.

### DLT-SHALL-23
- Req ID: DLT-SHALL-23
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Application ID filtering.

### XCP-SHALL-24
- Req ID: XCP-SHALL-24
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support CAN and Ethernet transport layers for XCP.

### XCP-SHALL-25
- Req ID: XCP-SHALL-25
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement XCP protocol version 1.5.

### XCP-SHALL-26
- Req ID: XCP-SHALL-26
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support XCP slave functionality.

### XCP-SHALL-27
- Req ID: XCP-SHALL-27
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support calibration page switching.

### XCP-SHALL-28
- Req ID: XCP-SHALL-28
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support up to 8 DAQ lists.

### ADCDRIVE-SHALL-1
- Req ID: ADCDRIVE-SHALL-1
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support 10-bit and 12-bit configurable ADC resolution.

### ADCDRIVE-SHALL-2
- Req ID: ADCDRIVE-SHALL-2
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Single, Continuous, and Scan conversion modes.

### ADCDRIVE-SHALL-3
- Req ID: ADCDRIVE-SHALL-3
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support up to 16 channels per ADC instance.

### ADCDRIVE-SHALL-4
- Req ID: ADCDRIVE-SHALL-4
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support left and right result alignment.

### ADCDRIVE-SHALL-5
- Req ID: ADCDRIVE-SHALL-5
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support interrupt-based and polling notification modes.

### CANDRIVE-SHALL-6
- Req ID: CANDRIVE-SHALL-6
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support Classical CAN (2.0B) and CAN FD protocols.

### CANDRIVE-SHALL-7
- Req ID: CANDRIVE-SHALL-7
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD...

### CANDRIVE-SHALL-8
- Req ID: CANDRIVE-SHALL-8
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide 64 mailboxes for CAN message buffering.

### CANDRIVE-SHALL-9
- Req ID: CANDRIVE-SHALL-9
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support FIFO mode for CAN message reception.

### CANDRIVE-SHALL-10
- Req ID: CANDRIVE-SHALL-10
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support loopback mode for self-test.

### CANDRIVE-SHALL-11
- Req ID: CANDRIVE-SHALL-11
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide automatic bus-off recovery conforming to AUTOSAR specification.

### CRYPTODR-SHALL-12
- Req ID: CRYPTODR-SHALL-12
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes.

### CRYPTODR-SHALL-13
- Req ID: CRYPTODR-SHALL-13
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support SHA-256 hashing.

### CRYPTODR-SHALL-14
- Req ID: CRYPTODR-SHALL-14
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support ECC P-256 elliptic curve cryptography.

### CRYPTODR-SHALL-15
- Req ID: CRYPTODR-SHALL-15
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL accelerate cryptographic operations using S32K312 HSM.

### CRYPTODR-SHALL-16
- Req ID: CRYPTODR-SHALL-16
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL store cryptographic keys in HSM secure NVM.

### CRYPTODR-SHALL-17
- Req ID: CRYPTODR-SHALL-17
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide integrated hardware TRNG.

### CRYPTODR-SHALL-18
- Req ID: CRYPTODR-SHALL-18
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide MbedTLS fallback for SIL simulation.

### DIODRIVE-SHALL-19
- Req ID: DIODRIVE-SHALL-19
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support 8 ports with 32 pins each for digital I/O.

### DIODRIVE-SHALL-20
- Req ID: DIODRIVE-SHALL-20
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support configurable pin direction per pin.

### DIODRIVE-SHALL-21
- Req ID: DIODRIVE-SHALL-21
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support HIGH and LEVEL output levels.

### DIODRIVE-SHALL-22
- Req ID: DIODRIVE-SHALL-22
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support edge-triggered interrupt on rising, falling, and both edges.

### PORTDRIV-SHALL-23
- Req ID: PORTDRIV-SHALL-23
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support pin mux configuration for approximately 100 pins.

### PORTDRIV-SHALL-24
- Req ID: PORTDRIV-SHALL-24
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support ALT0 through ALT7 mux modes.

### PORTDRIV-SHALL-25
- Req ID: PORTDRIV-SHALL-25
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support configurable pad properties including pull-up, pull-down, slew rat...

### GPTDRIVE-SHALL-26
- Req ID: GPTDRIVE-SHALL-26
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide 8 hardware timer channels.

### GPTDRIVE-SHALL-27
- Req ID: GPTDRIVE-SHALL-27
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide 32-bit timer resolution.

### GPTDRIVE-SHALL-28
- Req ID: GPTDRIVE-SHALL-28
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support prescaler values from 1 to 65536.

### GPTDRIVE-SHALL-29
- Req ID: GPTDRIVE-SHALL-29
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support one-shot and continuous timer modes.

### ICUDRIVE-SHALL-30
- Req ID: ICUDRIVE-SHALL-30
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support up to 8 input capture channels.

### ICUDRIVE-SHALL-31
- Req ID: ICUDRIVE-SHALL-31
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support signal period, duty cycle, and pulse width measurement.

### ICUDRIVE-SHALL-32
- Req ID: ICUDRIVE-SHALL-32
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support rising, falling, and both edge detection.

### MCUDRIVE-SHALL-33
- Req ID: MCUDRIVE-SHALL-33
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL.

### MCUDRIVE-SHALL-34
- Req ID: MCUDRIVE-SHALL-34
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support 4 RAM section banks.

### MCUDRIVE-SHALL-35
- Req ID: MCUDRIVE-SHALL-35
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support RUN, SLEEP, STOP, and STANDBY power modes.

### MCUDRIVE-SHALL-36
- Req ID: MCUDRIVE-SHALL-36
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support POR, WDG, SW, and External reset sources.

### WDGDRIVE-SHALL-37
- Req ID: WDGDRIVE-SHALL-37
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide configurable watchdog timeout from milliseconds to seconds.

### WDGDRIVE-SHALL-38
- Req ID: WDGDRIVE-SHALL-38
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support window mode watchdog operation.

### WDGDRIVE-SHALL-39
- Req ID: WDGDRIVE-SHALL-39
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support test mode for diagnostic testing.

### DCM-SHALL-01
- Req ID: DCM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support ISO 14229-1 UDS diagnostic services

### DCM-SHALL-02
- Req ID: DCM-SHALL-02
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support session management (default, programming, extended)

### DEM-SHALL-01
- Req ID: DEM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support DTC storage and retrieval

### DET-SHALL-01
- Req ID: DET-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL report development errors with module ID and error code

### DOIP-SHALL-01
- Req ID: DOIP-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support DoIP vehicle discovery

### COM-SHALL-01
- Req ID: COM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support signal-based I-PDU communication

### PDUR-SHALL-01
- Req ID: PDUR-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL route I-PDUs between COM and transport layers

### CANSM-SHALL-01
- Req ID: CANSM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage CAN network state machine

### LIN-SHALL-01
- Req ID: LIN-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support LIN master/slave communication

### NVM-SHALL-01
- Req ID: NVM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL support NV block read/write with redundancy

### FEE-SHALL-01
- Req ID: FEE-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL emulate EEPROM over flash with wear leveling

### MEMIF-SHALL-01
- Req ID: MEMIF-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL abstract NvM from Fee/EEP driver

### ECUM-SHALL-01
- Req ID: ECUM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage ECU startup/shutdown/wakeup sequences

### BSWM-SHALL-01
- Req ID: BSWM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL implement mode-based BSW scheduling

### WDGM-SHALL-01
- Req ID: WDGM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL monitor alive and deadline supervision

### OS-SHALL-01
- Req ID: OS-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide AUTOSAR OS SC4 compliant scheduling

### E2E-SHALL-01
- Req ID: E2E-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL protect safety-critical signals with CRC and sequence counter

### CSM-SHALL-01
- Req ID: CSM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL provide cryptographic service management

### KEYM-SHALL-01
- Req ID: KEYM-SHALL-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL manage cryptographic keys

## Summary
- Total Requirements: 127
- Requirements with implementation: 127 (100%)
- Requirements with test coverage: 0 (0%)
- Uncovered SHALLs: 127
- Scenarios: 0
- Reviews: 0
- CI Runs: 31