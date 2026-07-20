# Traceability Matrix

> Generated: 2026-07-20T12:54:00
> Version: v2-c-source-mapping

## Requirements → Implementation → Tests

### MCAL-SHALL-001
- Req ID: MCAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/pwm/src/Pwm.c`
- 📄 Impl: `src/bsw/mcal/spi/src/Spi.c`
- 📄 Impl: `src/bsw/mcal/i2c/src/I2c.c`
- 📄 Impl: `src/bsw/mcal/uart/src/Uart.c`
- 📄 Impl: `src/bsw/mcal/lin/src/Lin.c`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write)

### MCAL-SHALL-002
- Req ID: MCAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/pwm/src/Pwm.c`
- 📄 Impl: `src/bsw/mcal/spi/src/Spi.c`
- 📄 Impl: `src/bsw/mcal/i2c/src/I2c.c`
- 📄 Impl: `src/bsw/mcal/uart/src/Uart.c`
- 📄 Impl: `src/bsw/mcal/lin/src/Lin.c`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式

### MCAL-SHALL-003
- Req ID: MCAL-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/pwm/src/Pwm.c`
- 📄 Impl: `src/bsw/mcal/spi/src/Spi.c`
- 📄 Impl: `src/bsw/mcal/i2c/src/I2c.c`
- 📄 Impl: `src/bsw/mcal/uart/src/Uart.c`
- 📄 Impl: `src/bsw/mcal/lin/src/Lin.c`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: MCAL SHALL 使用 MISRA C:2023 合规编码风格

### ECUAL-SHALL-001
- Req ID: ECUAL-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/wdgif/src/WdgIf.c`
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/dlt/src/Dlt.c`
- 📄 Impl: `src/bsw/ecual/fee/src/Fee.c`
- 📄 Impl: `src/bsw/ecual/iohwab/src/IoHwAb.c`
- 📄 Impl: `src/bsw/ecual/ea/src/Ea.c`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器

### ECUAL-SHALL-002
- Req ID: ECUAL-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/wdgif/src/WdgIf.c`
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/dlt/src/Dlt.c`
- 📄 Impl: `src/bsw/ecual/fee/src/Fee.c`
- 📄 Impl: `src/bsw/ecual/iohwab/src/IoHwAb.c`
- 📄 Impl: `src/bsw/ecual/ea/src/Ea.c`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: 看门狗管理器 SHALL 在超时前刷新

### SVC-SHALL-001
- Req ID: SVC-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 📄 Impl: `src/bsw/services/wdgm/src/WdgM.c`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务

### SVC-SHALL-002
- Req ID: SVC-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 📄 Impl: `src/bsw/services/wdgm/src/WdgM.c`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: 通信栈 (CAN/以太网) SHALL 实现 PDU 路由

### SVC-SHALL-003
- Req ID: SVC-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 📄 Impl: `src/bsw/services/wdgm/src/WdgM.c`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: 诊断事件管理器 (Dem) SHALL 记录并上报 DTC

### NFR-SHALL-001
- Req ID: NFR-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/asw/communication_manager/src/Swc_CommunicationManager.c`
- 📄 Impl: `src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c`
- 📄 Impl: `src/asw/engine_control/src/Swc_EngineControl.c`
- 📄 Impl: `src/asw/io_control/src/Swc_IOControl.c`
- 📄 Impl: `src/asw/mode_manager/src/Swc_ModeManager.c`
- 🧪 Test: `tests/e2e/test_misra_compliance.c`
- SHALL: 代码 MISRA C:2023 合规

### NFR-SHALL-002
- Req ID: NFR-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/asw/communication_manager/src/Swc_CommunicationManager.c`
- 📄 Impl: `src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c`
- 📄 Impl: `src/asw/engine_control/src/Swc_EngineControl.c`
- 📄 Impl: `src/asw/io_control/src/Swc_IOControl.c`
- 📄 Impl: `src/asw/mode_manager/src/Swc_ModeManager.c`
- 🧪 Test: `tests/e2e/test_misra_compliance.c`
- SHALL: 单元测试行覆盖率

### NFR-SHALL-003
- Req ID: NFR-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/asw/communication_manager/src/Swc_CommunicationManager.c`
- 📄 Impl: `src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c`
- 📄 Impl: `src/asw/engine_control/src/Swc_EngineControl.c`
- 📄 Impl: `src/asw/io_control/src/Swc_IOControl.c`
- 📄 Impl: `src/asw/mode_manager/src/Swc_ModeManager.c`
- 🧪 Test: `tests/e2e/test_misra_compliance.c`
- SHALL: 条件覆盖率

### NFR-SHALL-004
- Req ID: NFR-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/asw/communication_manager/src/Swc_CommunicationManager.c`
- 📄 Impl: `src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c`
- 📄 Impl: `src/asw/engine_control/src/Swc_EngineControl.c`
- 📄 Impl: `src/asw/io_control/src/Swc_IOControl.c`
- 📄 Impl: `src/asw/mode_manager/src/Swc_ModeManager.c`
- 🧪 Test: `tests/e2e/test_misra_compliance.c`
- SHALL: 静态分析 (cppcheck)

### MISRA-SHALL-001
- Req ID: MISRA-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `misra-rules.yaml`
- 🧪 Test: `tests/e2e/test_misra_compliance.c`
- SHALL: SHALL 使用 MISRA C:2023 `safety` 配置

### DCM-SHALL-001
- Req ID: DCM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1.

### DCM-SHALL-002
- Req ID: DCM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 4 concurrent diagnostic sessions.

### DCM-SHALL-003
- Req ID: DCM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL enforce P2 timeout of 50ms for diagnostic responses.

### DCM-SHALL-004
- Req ID: DCM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL enforce P2\* timeout of 500ms for diagnostic responses.

### DEM-SHALL-001
- Req ID: DEM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dem/src/Dem.c`
- 📄 Impl: `src/bsw/services/dem/include/Dem.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs).

### DEM-SHALL-002
- Req ID: DEM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dem/src/Dem.c`
- 📄 Impl: `src/bsw/services/dem/include/Dem.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support 3 event priority levels: Low, Medium, High.

### DEM-SHALL-003
- Req ID: DEM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dem/src/Dem.c`
- 📄 Impl: `src/bsw/services/dem/include/Dem.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL store diagnostic events with primary and secondary (freeze frame) data.

### DEM-SHALL-004
- Req ID: DEM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dem/src/Dem.c`
- 📄 Impl: `src/bsw/services/dem/include/Dem.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL provide a configurable aging counter with default 40 cycles.

### COM-SHALL-001
- Req ID: COM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/com/src/Com.c`
- 📄 Impl: `src/bsw/services/com/include/Com.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a configurable signal count with default of 1024 signals.

### COM-SHALL-002
- Req ID: COM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/com/src/Com.c`
- 📄 Impl: `src/bsw/services/com/include/Com.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support signal group communication.

### COM-SHALL-003
- Req ID: COM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/com/src/Com.c`
- 📄 Impl: `src/bsw/services/com/include/Com.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support I-PDU send and receive directions.

### COM-SHALL-004
- Req ID: COM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/com/src/Com.c`
- 📄 Impl: `src/bsw/services/com/include/Com.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support deadline monitoring for signal transmission.

### PDUR-SHALL-001
- Req ID: PDUR-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/pdur/src/PduR.c`
- 📄 Impl: `src/bsw/services/pdur/include/PduR.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL maintain a static routing table generated at build time.

### PDUR-SHALL-002
- Req ID: PDUR-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/pdur/src/PduR.c`
- 📄 Impl: `src/bsw/services/pdur/include/PduR.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 512 routing paths.

### PDUR-SHALL-003
- Req ID: PDUR-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/pdur/src/PduR.c`
- 📄 Impl: `src/bsw/services/pdur/include/PduR.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet.

### NVM-SHALL-001
- Req ID: NVM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support native, redundant, and dataset NVM block management.

### NVM-SHALL-002
- Req ID: NVM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL use CRC-32 for write verification.

### NVM-SHALL-003
- Req ID: NVM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support block sizes from 1 to 65536 bytes.

### NVM-SHALL-004
- Req ID: NVM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 512 NVM blocks.

### NVM-SHALL-005
- Req ID: NVM-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support 4 job priority levels.

### ECUM-SHALL-001
- Req ID: ECUM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/ecum/src/EcuM.c`
- 📄 Impl: `src/bsw/services/ecum/include/EcuM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO.

### ECUM-SHALL-002
- Req ID: ECUM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/ecum/src/EcuM.c`
- 📄 Impl: `src/bsw/services/ecum/include/EcuM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support shutdown targets OFF, RESET, and SLEEP.

### ECUM-SHALL-003
- Req ID: ECUM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/ecum/src/EcuM.c`
- 📄 Impl: `src/bsw/services/ecum/include/EcuM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer.

### OSSC4-SHALL-001
- Req ID: OSSC4-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL provide fixed cyclic schedule tables.

### OSSC4-SHALL-002
- Req ID: OSSC4-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support BCC2 and ECC2 task conformance classes.

### OSSC4-SHALL-003
- Req ID: OSSC4-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 64 tasks.

### OSSC4-SHALL-004
- Req ID: OSSC4-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 32 alarms.

### OSSC4-SHALL-005
- Req ID: OSSC4-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL implement the priority ceiling protocol for resource management.

### CANIF-SHALL-001
- Req ID: CANIF-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/canif/include/CanIf.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1).

### CANIF-SHALL-002
- Req ID: CANIF-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/canif/include/CanIf.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support up to 512 PDU IDs.

### CANIF-SHALL-003
- Req ID: CANIF-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/canif/include/CanIf.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support transmit and receive PDU modes.

### CANIF-SHALL-004
- Req ID: CANIF-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canif/src/CanIf.c`
- 📄 Impl: `src/bsw/ecual/canif/include/CanIf.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support sleep and wakeup functionality.

### CANTP-SHALL-001
- Req ID: CANTP-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/cantp/include/CanTp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL implement the ISO 15765-2 CAN transport protocol.

### CANTP-SHALL-002
- Req ID: CANTP-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/cantp/include/CanTp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support message segmentation up to 4095 bytes per message.

### CANTP-SHALL-003
- Req ID: CANTP-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/cantp/include/CanTp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support Continuous and Wait flow control modes.

### CANTP-SHALL-004
- Req ID: CANTP-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/cantp/src/CanTp.c`
- 📄 Impl: `src/bsw/ecual/cantp/include/CanTp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support Physical and Functional addressing.

### CANNM-SHALL-001
- Req ID: CANNM-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/canNm/include/CanNm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL implement AUTOSAR CAN Network Management protocol.

### CANNM-SHALL-002
- Req ID: CANNM-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/canNm/include/CanNm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a configurable 8-bit node ID.

### CANNM-SHALL-003
- Req ID: CANNM-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/canNm/include/CanNm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support configurable message cycle time with default of 100ms.

### CANNM-SHALL-004
- Req ID: CANNM-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/canNm/include/CanNm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support configurable repeat message timer with default of 1000ms.

### CANNM-SHALL-005
- Req ID: CANNM-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/canNm/src/CanNm.c`
- 📄 Impl: `src/bsw/ecual/canNm/include/CanNm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support bus synchronization.

### SOAD-SHALL-001
- Req ID: SOAD-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/soad/src/SoAd.c`
- 📄 Impl: `src/bsw/services/soad/include/SoAd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a maximum of 32 sockets.

### SOAD-SHALL-002
- Req ID: SOAD-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/soad/src/SoAd.c`
- 📄 Impl: `src/bsw/services/soad/include/SoAd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support TCP and UDP protocols.

### SOAD-SHALL-003
- Req ID: SOAD-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/soad/src/SoAd.c`
- 📄 Impl: `src/bsw/services/soad/include/SoAd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support Server and Client connection types.

### SOAD-SHALL-004
- Req ID: SOAD-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/soad/src/SoAd.c`
- 📄 Impl: `src/bsw/services/soad/include/SoAd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support SOME/IP protocol communication.

### SOMEIPSD-SHALL-001
- Req ID: SOMEIPSD-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/someip/src/SomeIpSd.c`
- 📄 Impl: `src/bsw/services/someip/include/SomeIpSd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support an offer cycle of 1000ms for service discovery.

### SOMEIPSD-SHALL-002
- Req ID: SOMEIPSD-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/someip/src/SomeIpSd.c`
- 📄 Impl: `src/bsw/services/someip/include/SomeIpSd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support a request cycle of 2000ms for service discovery.

### SOMEIPSD-SHALL-003
- Req ID: SOMEIPSD-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/someip/src/SomeIpSd.c`
- 📄 Impl: `src/bsw/services/someip/include/SomeIpSd.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support TTL multiplier of 3 for service entries.

### DLT-SHALL-001
- Req ID: DLT-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/dlt/src/Dlt.c`
- 📄 Impl: `src/bsw/ecual/dlt/include/Dlt.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose.

### DLT-SHALL-002
- Req ID: DLT-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/dlt/src/Dlt.c`
- 📄 Impl: `src/bsw/ecual/dlt/include/Dlt.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support TCP and Serial transport for DLT messages.

### DLT-SHALL-003
- Req ID: DLT-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/dlt/src/Dlt.c`
- 📄 Impl: `src/bsw/ecual/dlt/include/Dlt.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support Application ID filtering.

### XCP-SHALL-001
- Req ID: XCP-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/xcp/src/Xcp.c`
- 📄 Impl: `src/bsw/services/xcp/include/Xcp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support CAN and Ethernet transport layers for XCP.

### XCP-SHALL-002
- Req ID: XCP-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/xcp/src/Xcp.c`
- 📄 Impl: `src/bsw/services/xcp/include/Xcp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL implement XCP protocol version 1.5.

### XCP-SHALL-003
- Req ID: XCP-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/xcp/src/Xcp.c`
- 📄 Impl: `src/bsw/services/xcp/include/Xcp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support XCP slave functionality.

### XCP-SHALL-004
- Req ID: XCP-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/xcp/src/Xcp.c`
- 📄 Impl: `src/bsw/services/xcp/include/Xcp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support calibration page switching.

### XCP-SHALL-005
- Req ID: XCP-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/xcp/src/Xcp.c`
- 📄 Impl: `src/bsw/services/xcp/include/Xcp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: The system SHALL support up to 8 DAQ lists.

### ADC-SHALL-001
- Req ID: ADC-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/adc/include/Adc.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support 10-bit and 12-bit configurable ADC resolution.

### ADC-SHALL-002
- Req ID: ADC-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/adc/include/Adc.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support Single, Continuous, and Scan conversion modes.

### ADC-SHALL-003
- Req ID: ADC-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/adc/include/Adc.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support up to 16 channels per ADC instance.

### ADC-SHALL-004
- Req ID: ADC-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/adc/include/Adc.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support left and right result alignment.

### ADC-SHALL-005
- Req ID: ADC-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/adc/src/Adc.c`
- 📄 Impl: `src/bsw/mcal/adc/include/Adc.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support interrupt-based and polling notification modes.

### CANDRV-SHALL-001
- Req ID: CANDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support Classical CAN (2.0B) and CAN FD protocols.

### CANDRV-SHALL-002
- Req ID: CANDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD.

### CANDRV-SHALL-003
- Req ID: CANDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide 64 mailboxes for CAN message buffering.

### CANDRV-SHALL-004
- Req ID: CANDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support FIFO mode for CAN message reception.

### CANDRV-SHALL-005
- Req ID: CANDRV-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support loopback mode for self-test.

### CANDRV-SHALL-006
- Req ID: CANDRV-SHALL-006
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/can/src/Can.c`
- 📄 Impl: `src/bsw/mcal/can/include/Can.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification.

### CRYPTO-SHALL-001
- Req ID: CRYPTO-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes.

### CRYPTO-SHALL-002
- Req ID: CRYPTO-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support SHA-256 hashing.

### CRYPTO-SHALL-003
- Req ID: CRYPTO-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support ECC P-256 elliptic curve cryptography.

### CRYPTO-SHALL-004
- Req ID: CRYPTO-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL accelerate cryptographic operations using S32K312 HSM.

### CRYPTO-SHALL-005
- Req ID: CRYPTO-SHALL-005
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL store cryptographic keys in HSM secure NVM.

### CRYPTO-SHALL-006
- Req ID: CRYPTO-SHALL-006
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide integrated hardware TRNG.

### CRYPTO-SHALL-007
- Req ID: CRYPTO-SHALL-007
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/crypto/src/Crypto.c`
- 📄 Impl: `src/bsw/mcal/crypto/include/Crypto.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide MbedTLS fallback for SIL simulation.

### DIODRV-SHALL-001
- Req ID: DIODRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/dio/include/Dio.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support 8 ports with 32 pins each for digital I/O.

### DIODRV-SHALL-002
- Req ID: DIODRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/dio/include/Dio.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support configurable pin direction per pin.

### DIODRV-SHALL-003
- Req ID: DIODRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/dio/include/Dio.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support HIGH and LEVEL output levels.

### DIODRV-SHALL-004
- Req ID: DIODRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/dio/src/Dio.c`
- 📄 Impl: `src/bsw/mcal/dio/include/Dio.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support edge-triggered interrupt on rising, falling, and both edges.

### PORTDRV-SHALL-001
- Req ID: PORTDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/port/include/Port.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support pin mux configuration for approximately 100 pins.

### PORTDRV-SHALL-002
- Req ID: PORTDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/port/include/Port.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support ALT0 through ALT7 mux modes.

### PORTDRV-SHALL-003
- Req ID: PORTDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/port/src/Port.c`
- 📄 Impl: `src/bsw/mcal/port/include/Port.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength.

### GPTDRV-SHALL-001
- Req ID: GPTDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/gpt/include/Gpt.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide 8 hardware timer channels.

### GPTDRV-SHALL-002
- Req ID: GPTDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/gpt/include/Gpt.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide 32-bit timer resolution.

### GPTDRV-SHALL-003
- Req ID: GPTDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/gpt/include/Gpt.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support prescaler values from 1 to 65536.

### GPTDRV-SHALL-004
- Req ID: GPTDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/gpt/src/Gpt.c`
- 📄 Impl: `src/bsw/mcal/gpt/include/Gpt.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support one-shot and continuous timer modes.

### ICURV-SHALL-001
- Req ID: ICURV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/icu/src/Icu.c`
- 📄 Impl: `src/bsw/mcal/icu/include/Icu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support up to 8 input capture channels.

### ICURV-SHALL-002
- Req ID: ICURV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/icu/src/Icu.c`
- 📄 Impl: `src/bsw/mcal/icu/include/Icu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support signal period, duty cycle, and pulse width measurement.

### ICURV-SHALL-003
- Req ID: ICURV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/icu/src/Icu.c`
- 📄 Impl: `src/bsw/mcal/icu/include/Icu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support rising, falling, and both edge detection.

### MCUDRV-SHALL-001
- Req ID: MCUDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/mcu/include/Mcu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL.

### MCUDRV-SHALL-002
- Req ID: MCUDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/mcu/include/Mcu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support 4 RAM section banks.

### MCUDRV-SHALL-003
- Req ID: MCUDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/mcu/include/Mcu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes.

### MCUDRV-SHALL-004
- Req ID: MCUDRV-SHALL-004
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/mcu/src/Mcu.c`
- 📄 Impl: `src/bsw/mcal/mcu/include/Mcu.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support POR, WDG, SW, and External reset sources.

### WDGDRV-SHALL-001
- Req ID: WDGDRV-SHALL-001
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/wdg/include/Wdg.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL provide configurable watchdog timeout from milliseconds to seconds.

### WDGDRV-SHALL-002
- Req ID: WDGDRV-SHALL-002
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/wdg/include/Wdg.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support window mode watchdog operation.

### WDGDRV-SHALL-003
- Req ID: WDGDRV-SHALL-003
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/mcal/wdg/src/Wdg.c`
- 📄 Impl: `src/bsw/mcal/wdg/include/Wdg.h`
- 🧪 Test: `tests/unit/test_mcal_api_contracts.c`
- SHALL: The system SHALL support test mode for diagnostic testing.

### DCM-REQ-01
- Req ID: DCM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support ISO 14229-1 UDS diagnostic services

### DCM-REQ-02
- Req ID: DCM-REQ-02
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dcm/src/Dcm.c`
- 📄 Impl: `src/bsw/services/dcm/include/Dcm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support session management (default, programming, extended)

### DEM-REQ-01
- Req ID: DEM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/dem/src/Dem.c`
- 📄 Impl: `src/bsw/services/dem/include/Dem.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support DTC storage and retrieval

### DET-REQ-01
- Req ID: DET-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/det/src/Det.c`
- 📄 Impl: `src/bsw/services/det/include/Det.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL report development errors with module ID and error code

### DOIP-REQ-01
- Req ID: DOIP-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/doip/src/DoIP.c`
- 📄 Impl: `src/bsw/services/doip/include/DoIP.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support DoIP vehicle discovery

### COM-REQ-01
- Req ID: COM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/com/src/Com.c`
- 📄 Impl: `src/bsw/services/com/include/Com.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support signal-based I-PDU communication

### PDUR-REQ-01
- Req ID: PDUR-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/pdur/src/PduR.c`
- 📄 Impl: `src/bsw/services/pdur/include/PduR.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL route I-PDUs between COM and transport layers

### CANSM-REQ-01
- Req ID: CANSM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/cansm/src/CanSm.c`
- 📄 Impl: `src/bsw/services/cansm/include/CanSm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL manage CAN network state machine

### LIN-REQ-01
- Req ID: LIN-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/lntm/src/LinTp.c`
- 📄 Impl: `src/bsw/services/lntm/include/LinTp.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support LIN master/slave communication

### NVM-REQ-01
- Req ID: NVM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/nvm/src/NvM.c`
- 📄 Impl: `src/bsw/services/nvm/include/NvM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL support NV block read/write with redundancy

### FEE-REQ-01
- Req ID: FEE-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/ecual/fee/src/Fee.c`
- 📄 Impl: `src/bsw/ecual/fee/include/Fee.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL emulate EEPROM over flash with wear leveling

### MEMIF-REQ-01
- Req ID: MEMIF-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/memif/src/MemIf.c`
- 📄 Impl: `src/bsw/services/memif/include/MemIf.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL abstract NvM from Fee/EEP driver

### ECUM-REQ-01
- Req ID: ECUM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/ecum/src/EcuM.c`
- 📄 Impl: `src/bsw/services/ecum/include/EcuM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL manage ECU startup/shutdown/wakeup sequences

### BSWM-REQ-01
- Req ID: BSWM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/bswm/src/BswM.c`
- 📄 Impl: `src/bsw/services/bswm/include/BswM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL implement mode-based BSW scheduling

### WDGM-REQ-01
- Req ID: WDGM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/wdgm/src/WdgM.c`
- 📄 Impl: `src/bsw/services/wdgm/include/WdgM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL monitor alive and deadline supervision

### OS-REQ-01
- Req ID: OS-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/os/src/Os.c`
- 📄 Impl: `src/bsw/os/include/Os.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL provide AUTOSAR OS SC4 compliant scheduling

### E2E-REQ-01
- Req ID: E2E-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/e2e/src/E2E.c`
- 📄 Impl: `src/bsw/services/e2e/include/E2E.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL protect safety-critical signals with CRC and sequence counter

### CSM-REQ-01
- Req ID: CSM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/csm/src/Csm.c`
- 📄 Impl: `src/bsw/services/csm/include/Csm.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL provide cryptographic service management

### KEYM-REQ-01
- Req ID: KEYM-REQ-01
- SHALL statements: 1
- Status: ✅ Covered
- 📄 Impl: `src/bsw/services/keym/src/KeyM.c`
- 📄 Impl: `src/bsw/services/keym/include/KeyM.h`
- 🧪 Test: `tests/unit/test_services_api_contracts.c`
- SHALL: SHALL manage cryptographic keys

## Summary

- Total requirements: 127
- With C implementation: 127
- With test coverage: 127
- Uncovered SHALLs: 0