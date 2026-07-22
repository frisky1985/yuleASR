# Traceability Matrix

> Generated: 2026-07-21T17:42:10
> Version: 0.1.0

## Requirements → Implementation → Tests

### MCAL-SHALL-001
- Req ID: MCAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write)

### MCAL-SHALL-002
- Req ID: MCAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式

### MCAL-SHALL-003
- Req ID: MCAL-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ MCAL SHALL 使用 MISRA C:2023 合规编码风格

### ECUAL-SHALL-001
- Req ID: ECUAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器

### ECUAL-SHALL-002
- Req ID: ECUAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ 看门狗管理器 SHALL 在超时前刷新

### SVC-SHALL-001
- Req ID: SVC-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务

### SVC-SHALL-002
- Req ID: SVC-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ 通信栈 (CAN/以太网) SHALL 实现 PDU 路由

### SVC-SHALL-003
- Req ID: SVC-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ 诊断事件管理器 (Dem) SHALL 记录并上报 DTC

### NFR-SHALL-001
- Req ID: NFR-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/e2e/test_misra_compliance.c
- SHALL details:
  ❌ 代码 MISRA C:2023 合规

### NFR-SHALL-002
- Req ID: NFR-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/e2e/test_misra_compliance.c
- SHALL details:
  ❌ 单元测试行覆盖率

### NFR-SHALL-003
- Req ID: NFR-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/e2e/test_misra_compliance.c
- SHALL details:
  ❌ 条件覆盖率

### NFR-SHALL-004
- Req ID: NFR-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/e2e/test_misra_compliance.c
- SHALL details:
  ❌ 静态分析 (cppcheck)

### MISRA-SHALL-001
- Req ID: MISRA-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/e2e/test_misra_compliance.c
- SHALL details:
  ❌ SHALL 使用 MISRA C:2023 `safety` 配置

### DCM-SHALL-001
- Req ID: DCM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1.

### DCM-SHALL-002
- Req ID: DCM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 4 concurrent diagnostic sessions.

### DCM-SHALL-003
- Req ID: DCM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL enforce P2 timeout of 50ms for diagnostic responses.

### DCM-SHALL-004
- Req ID: DCM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL enforce P2\* timeout of 500ms for diagnostic responses.

### DEM-SHALL-001
- Req ID: DEM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs).

### DEM-SHALL-002
- Req ID: DEM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support 3 event priority levels: Low, Medium, High.

### DEM-SHALL-003
- Req ID: DEM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL store diagnostic events with primary and secondary (freeze frame) data.

### DEM-SHALL-004
- Req ID: DEM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide a configurable aging counter with default 40 cycles.

### COM-SHALL-001
- Req ID: COM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a configurable signal count with default of 1024 signals.

### COM-SHALL-002
- Req ID: COM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support signal group communication.

### COM-SHALL-003
- Req ID: COM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support I-PDU send and receive directions.

### COM-SHALL-004
- Req ID: COM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support deadline monitoring for signal transmission.

### PDUR-SHALL-001
- Req ID: PDUR-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL maintain a static routing table generated at build time.

### PDUR-SHALL-002
- Req ID: PDUR-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 512 routing paths.

### PDUR-SHALL-003
- Req ID: PDUR-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet.

### NVM-SHALL-001
- Req ID: NVM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support native, redundant, and dataset NVM block management.

### NVM-SHALL-002
- Req ID: NVM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL use CRC-32 for write verification.

### NVM-SHALL-003
- Req ID: NVM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support block sizes from 1 to 65536 bytes.

### NVM-SHALL-004
- Req ID: NVM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 512 NVM blocks.

### NVM-SHALL-005
- Req ID: NVM-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support 4 job priority levels.

### ECUM-SHALL-001
- Req ID: ECUM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO.

### ECUM-SHALL-002
- Req ID: ECUM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support shutdown targets OFF, RESET, and SLEEP.

### ECUM-SHALL-003
- Req ID: ECUM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer.

### OSSC4-SHALL-001
- Req ID: OSSC4-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide fixed cyclic schedule tables.

### OSSC4-SHALL-002
- Req ID: OSSC4-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support BCC2 and ECC2 task conformance classes.

### OSSC4-SHALL-003
- Req ID: OSSC4-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 64 tasks.

### OSSC4-SHALL-004
- Req ID: OSSC4-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 32 alarms.

### OSSC4-SHALL-005
- Req ID: OSSC4-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL implement the priority ceiling protocol for resource management.

### CANIF-SHALL-001
- Req ID: CANIF-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1).

### CANIF-SHALL-002
- Req ID: CANIF-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support up to 512 PDU IDs.

### CANIF-SHALL-003
- Req ID: CANIF-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support transmit and receive PDU modes.

### CANIF-SHALL-004
- Req ID: CANIF-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support sleep and wakeup functionality.

### CANTP-SHALL-001
- Req ID: CANTP-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL implement the ISO 15765-2 CAN transport protocol.

### CANTP-SHALL-002
- Req ID: CANTP-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support message segmentation up to 4095 bytes per message.

### CANTP-SHALL-003
- Req ID: CANTP-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Continuous and Wait flow control modes.

### CANTP-SHALL-004
- Req ID: CANTP-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Physical and Functional addressing.

### CANNM-SHALL-001
- Req ID: CANNM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL implement AUTOSAR CAN Network Management protocol.

### CANNM-SHALL-002
- Req ID: CANNM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a configurable 8-bit node ID.

### CANNM-SHALL-003
- Req ID: CANNM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support configurable message cycle time with default of 100ms.

### CANNM-SHALL-004
- Req ID: CANNM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support configurable repeat message timer with default of 1000ms.

### CANNM-SHALL-005
- Req ID: CANNM-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support bus synchronization.

### SOAD-SHALL-001
- Req ID: SOAD-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a maximum of 32 sockets.

### SOAD-SHALL-002
- Req ID: SOAD-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support TCP and UDP protocols.

### SOAD-SHALL-003
- Req ID: SOAD-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Server and Client connection types.

### SOAD-SHALL-004
- Req ID: SOAD-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support SOME/IP protocol communication.

### SOMEIPSD-SHALL-001
- Req ID: SOMEIPSD-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support an offer cycle of 1000ms for service discovery.

### SOMEIPSD-SHALL-002
- Req ID: SOMEIPSD-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support a request cycle of 2000ms for service discovery.

### SOMEIPSD-SHALL-003
- Req ID: SOMEIPSD-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support TTL multiplier of 3 for service entries.

### DLT-SHALL-001
- Req ID: DLT-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose.

### DLT-SHALL-002
- Req ID: DLT-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support TCP and Serial transport for DLT messages.

### DLT-SHALL-003
- Req ID: DLT-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Application ID filtering.

### XCP-SHALL-001
- Req ID: XCP-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support CAN and Ethernet transport layers for XCP.

### XCP-SHALL-002
- Req ID: XCP-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL implement XCP protocol version 1.5.

### XCP-SHALL-003
- Req ID: XCP-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support XCP slave functionality.

### XCP-SHALL-004
- Req ID: XCP-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support calibration page switching.

### XCP-SHALL-005
- Req ID: XCP-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ The system SHALL support up to 8 DAQ lists.

### ADC-SHALL-001
- Req ID: ADC-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support 10-bit and 12-bit configurable ADC resolution.

### ADC-SHALL-002
- Req ID: ADC-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Single, Continuous, and Scan conversion modes.

### ADC-SHALL-003
- Req ID: ADC-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support up to 16 channels per ADC instance.

### ADC-SHALL-004
- Req ID: ADC-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support left and right result alignment.

### ADC-SHALL-005
- Req ID: ADC-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support interrupt-based and polling notification modes.

### CANDRV-SHALL-001
- Req ID: CANDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support Classical CAN (2.0B) and CAN FD protocols.

### CANDRV-SHALL-002
- Req ID: CANDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD.

### CANDRV-SHALL-003
- Req ID: CANDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide 64 mailboxes for CAN message buffering.

### CANDRV-SHALL-004
- Req ID: CANDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support FIFO mode for CAN message reception.

### CANDRV-SHALL-005
- Req ID: CANDRV-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support loopback mode for self-test.

### CANDRV-SHALL-006
- Req ID: CANDRV-SHALL-006
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification.

### CRYPTO-SHALL-001
- Req ID: CRYPTO-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes.

### CRYPTO-SHALL-002
- Req ID: CRYPTO-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support SHA-256 hashing.

### CRYPTO-SHALL-003
- Req ID: CRYPTO-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support ECC P-256 elliptic curve cryptography.

### CRYPTO-SHALL-004
- Req ID: CRYPTO-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL accelerate cryptographic operations using S32K312 HSM.

### CRYPTO-SHALL-005
- Req ID: CRYPTO-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL store cryptographic keys in HSM secure NVM.

### CRYPTO-SHALL-006
- Req ID: CRYPTO-SHALL-006
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide integrated hardware TRNG.

### CRYPTO-SHALL-007
- Req ID: CRYPTO-SHALL-007
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide MbedTLS fallback for SIL simulation.

### DIODRV-SHALL-001
- Req ID: DIODRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support 8 ports with 32 pins each for digital I/O.

### DIODRV-SHALL-002
- Req ID: DIODRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support configurable pin direction per pin.

### DIODRV-SHALL-003
- Req ID: DIODRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support HIGH and LEVEL output levels.

### DIODRV-SHALL-004
- Req ID: DIODRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support edge-triggered interrupt on rising, falling, and both edges.

### PORTDRV-SHALL-001
- Req ID: PORTDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support pin mux configuration for approximately 100 pins.

### PORTDRV-SHALL-002
- Req ID: PORTDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support ALT0 through ALT7 mux modes.

### PORTDRV-SHALL-003
- Req ID: PORTDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength.

### GPTDRV-SHALL-001
- Req ID: GPTDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide 8 hardware timer channels.

### GPTDRV-SHALL-002
- Req ID: GPTDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide 32-bit timer resolution.

### GPTDRV-SHALL-003
- Req ID: GPTDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support prescaler values from 1 to 65536.

### GPTDRV-SHALL-004
- Req ID: GPTDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support one-shot and continuous timer modes.

### ICURV-SHALL-001
- Req ID: ICURV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support up to 8 input capture channels.

### ICURV-SHALL-002
- Req ID: ICURV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support signal period, duty cycle, and pulse width measurement.

### ICURV-SHALL-003
- Req ID: ICURV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support rising, falling, and both edge detection.

### MCUDRV-SHALL-001
- Req ID: MCUDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL.

### MCUDRV-SHALL-002
- Req ID: MCUDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support 4 RAM section banks.

### MCUDRV-SHALL-003
- Req ID: MCUDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes.

### MCUDRV-SHALL-004
- Req ID: MCUDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support POR, WDG, SW, and External reset sources.

### WDGDRV-SHALL-001
- Req ID: WDGDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL provide configurable watchdog timeout from milliseconds to seconds.

### WDGDRV-SHALL-002
- Req ID: WDGDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support window mode watchdog operation.

### WDGDRV-SHALL-003
- Req ID: WDGDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_mcal_api_contracts.c
- SHALL details:
  ❌ The system SHALL support test mode for diagnostic testing.

### DCM-REQ-01
- Req ID: DCM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support ISO 14229-1 UDS diagnostic services

### DCM-REQ-02
- Req ID: DCM-REQ-02
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support session management (default, programming, extended)

### DEM-REQ-01
- Req ID: DEM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support DTC storage and retrieval

### DET-REQ-01
- Req ID: DET-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL report development errors with module ID and error code

### DOIP-REQ-01
- Req ID: DOIP-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support DoIP vehicle discovery

### COM-REQ-01
- Req ID: COM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support signal-based I-PDU communication

### PDUR-REQ-01
- Req ID: PDUR-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL route I-PDUs between COM and transport layers

### CANSM-REQ-01
- Req ID: CANSM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL manage CAN network state machine

### LIN-REQ-01
- Req ID: LIN-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support LIN master/slave communication

### NVM-REQ-01
- Req ID: NVM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL support NV block read/write with redundancy

### FEE-REQ-01
- Req ID: FEE-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL emulate EEPROM over flash with wear leveling

### MEMIF-REQ-01
- Req ID: MEMIF-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL abstract NvM from Fee/EEP driver

### ECUM-REQ-01
- Req ID: ECUM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL manage ECU startup/shutdown/wakeup sequences

### BSWM-REQ-01
- Req ID: BSWM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL implement mode-based BSW scheduling

### WDGM-REQ-01
- Req ID: WDGM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL monitor alive and deadline supervision

### OS-REQ-01
- Req ID: OS-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL provide AUTOSAR OS SC4 compliant scheduling

### E2E-REQ-01
- Req ID: E2E-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL protect safety-critical signals with CRC and sequence counter

### CSM-REQ-01
- Req ID: CSM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL provide cryptographic service management

### KEYM-REQ-01
- Req ID: KEYM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_services_api_contracts.c
- SHALL details:
  ❌ SHALL manage cryptographic keys

## Summary
- Total Requirements: 127
- Requirements with implementation: 85 (66%)
- Requirements with test coverage: 127 (100%)
- Uncovered SHALLs: 0
- Scenarios: 0
- Reviews: 0
- CI Runs: 0
