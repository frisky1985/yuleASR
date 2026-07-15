# yuleASR BSW Platform — Software Requirements Specification

> **Project**: yuleASR AUTOSAR BSW Platform (S32K312)
> **Standard**: AUTOSAR CP 4.4.0
> **Document Version**: 1.0

## 1. Platform-Level Requirements

### SWR-001: AUTOSAR BSW Platform
- **SWR-001.1-01**: SHALL support AUTOSAR Classic Platform 4.4.0 standard
- **SWR-001.1-02**: SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules
- **SWR-001.1-03**: SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules
- **SWR-001.1-04**: SHALL implement BSW Services layer covering 44 service modules
- **SWR-001.1-05**: SHALL target NXP S32K312 microcontroller platform
- **SWR-001.1-06**: SHALL support RTE generation for SWC-to-BSW communication
- **SWR-001.2**: Priority: Critical | Status: Implemented

### SWR-002: Safety & Security
- **SWR-002.1-01**: SHALL implement E2E communication protection for safety-critical signals
- **SWR-002.1-02**: SHALL support HSM-based cryptographic operations via Crypto module
- **SWR-002.1-03**: SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition
- **SWR-002.1-04**: SHALL implement secure boot mechanism
- **SWR-002.1-05**: SHOULD support SHE-compliant key management
- **SWR-002.2**: Priority: Critical | Status: Implemented

### SWR-003: Communication Stack
- **SWR-003.1-01**: SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm)
- **SWR-003.1-02**: SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM)
- **SWR-003.1-03**: SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd)
- **SWR-003.1-04**: SHALL implement DCM diagnostic communication manager
- **SWR-003.1-05**: SHALL implement DoIP diagnostic over IP
- **SWR-003.1-06**: SHOULD support J1939 transport and network management
- **SWR-003.1-07**: SHOULD support SOME/IP Service Discovery
- **SWR-003.2**: Priority: High | Status: Implemented

### SWR-004: Memory Stack
- **SWR-004.1-01**: SHALL implement NVRAM manager (NvM) for persistent storage
- **SWR-004.1-02**: SHALL implement Flash EEPROM emulation (Fee)
- **SWR-004.1-03**: SHALL implement internal/external EEPROM driver
- **SWR-004.1-04**: SHALL implement memory abstraction interface (MemIf)
- **SWR-004.1-05**: SHALL support flash driver for S32K312 on-chip flash
- **SWR-004.2**: Priority: High | Status: Implemented

### SWR-005: System Services
- **SWR-005.1-01**: SHALL implement ECU state manager (EcuM)
- **SWR-005.1-02**: SHALL implement BSW scheduler (BswM) with mode management
- **SWR-005.1-03**: SHALL implement Watchdog manager (WdgM)
- **SWR-005.1-04**: SHALL implement Default Error Tracer (Det)
- **SWR-005.1-05**: SHALL implement Diagnostic Event Manager (Dem)
- **SWR-005.1-06**: SHALL implement Function Inhibition Manager (FiM)
- **SWR-005.1-07**: SHALL implement CRC calculator
- **SWR-005.1-08**: SHALL implement OS (AUTOSAR SC4 compliant)
- **SWR-005.1-09**: SHALL support DLT (Diagnostic Log and Trace)
- **SWR-005.1-10**: SHOULD support XCP calibration protocol
- **SWR-005.2**: Priority: High | Status: Implemented

### SWR-006: MCAL Drivers
- **SWR-006.1-01**: SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG)
- **SWR-006.1-02**: SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap)
- **SWR-006.2**: Priority: High | Status: Implemented

### SWR-007: Application Software
- **SWR-007.1-01**: SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager
- **SWR-007.1-02**: SHALL implement RTE for component communication
- **SWR-007.2**: Priority: Medium | Status: Implemented

### SWR-008: Micro DDS Integration
- **SWR-008.1-01**: SHALL integrate micro DDS middleware for inter-ECU communication
- **SWR-008.1-02**: SHOULD support DDS QoS policies
- **SWR-008.2**: Priority: Low | Status: Implemented

---

## Traceability References

| Requirement ID | System Req ID | Test Coverage | Status |
|:--------------|:-------------|:-------------|:-------|
| SWR-001.1-01 | SYS-REQ-BSW-001 | TC-BSW-ARCH-001 | Implemented |
| SWR-001.1-02 | SYS-REQ-BSW-002 | TC-MCAL-COV-001 | Implemented |
| SWR-001.1-03 | SYS-REQ-BSW-003 | TC-ECUAL-COV-001 | Implemented |
| SWR-001.1-04 | SYS-REQ-BSW-004 | TC-SRV-COV-001 | Implemented |
| SWR-002.1-01 | SYS-REQ-SAF-001 | TC-E2E-001 | Implemented |
| SWR-002.1-02 | SYS-REQ-SEC-001 | TC-CRYPTO-001 | Implemented |
| SWR-003.1-01 | SYS-REQ-COM-001 | TC-CAN-001 | Implemented |
| SWR-003.1-02 | SYS-REQ-COM-002 | TC-LIN-001 | Implemented |
| SWR-004.1-01 | SYS-REQ-MEM-001 | TC-NVM-001 | Implemented |
| SWR-005.1-01 | SYS-REQ-SYS-001 | TC-ECUM-001 | Implemented |
| SWR-006.1-01 | SYS-REQ-MCAL-001 | TC-MCAL-001 | Implemented |
| SWR-007.1-01 | SYS-REQ-ASW-001 | TC-ASW-001 | Implemented |
