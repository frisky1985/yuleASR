# yuleASR — Integration Strategy

> **Version**: 1.0 | **Date**: 2026-07-15
> **Project**: yuleASR AUTOSAR BSW Platform (S32K312)

## 1. Integration Approach

The yuleASR BSW platform follows a **bottom-up layered integration strategy**:

```
Layer 5: Application Software (ASW + RTE)
Layer 4: BSW Services (44 modules)
Layer 3: ECUAL (29 modules)
Layer 2: MCAL (21 modules)
Layer 1: Platform (S32K312 HAL + OS)
```

## 2. Integration Sequence

### Phase 1: Platform Layer
- **Scope**: S32K312 HAL, linker script, startup code, OS
- **Dependencies**: None (foundation layer)
- **Stubs/Drivers**: N/A
- **Entry Criteria**: MCU clock config, memory map verified
- **Exit Criteria**: OS boots successfully on target

### Phase 2: MCAL Layer
- **Scope**: All 21 MCAL modules
- **Dependencies**: Phase 1 (Platform)
- **Stubs/Drivers**: Test harnesses for each MCAL module
- **Integration Order**: MCU → PORT → DIO → GPT → ICU → OCU → PWM → ADC → CAN → LIN → ETH → SPI → I2C → UART → FLASH → FLS → FEE → EEP → WDG → RAMTST → Crypto
- **Exit Criteria**: All MCAL modules functional on S32K312

### Phase 3: ECUAL Layer
- **Scope**: All 29 ECUAL modules
- **Dependencies**: Phase 2 (MCAL)
- **Stubs/Drivers**: MCAL interface stubs where MCAL not yet integrated
- **Integration Order**: CanIf → CanTp → CanNm → CanSm → LinIf → LinTp → LinNm → LinSM → EthIf → EthSm → DoIP → Fee → Ea → MemIf → SomeIpIf → SomeIpSd → FrIf → FrTp → FrIf → IpHdwAb → WdgIf → CryptoIf → Dlt → Xcp
- **Exit Criteria**: All ECUAL modules operational

### Phase 4: BSW Services Layer
- **Scope**: All 44 service modules
- **Dependencies**: Phase 3 (ECUAL)
- **Stubs/Drivers**: ECUAL stubs for bottom-level testing
- **Integration Order**: EcuM → BswM → Com → PduR → Dcm → Dem → Det → FiM → NvM → MemIf → WdgM → E2E → Csm → KeyM → Srp → StbM → SoAd → SomeIp → DoIP → EthSm → CanSM → LinSM → UdpNm → J1939Nm → J1939Tp → Dlt → Xcp
- **Exit Criteria**: Full BSW stack operational

### Phase 5: RTE + ASW Layer
- **Scope**: RTE generation, ASW components
- **Dependencies**: Phase 4 (Services)
- **Stubs/Drivers**: ASW component stubs for RTE testing
- **Integration Order**: RTE → CommunicationManager → DiagnosticManager → ModeManager → IOControl → StorageManager → EngineControl → VehicleDynamics → WatchdogManager
- **Exit Criteria**: Full application running on target

## 3. Stubs and Drivers

| Stub/Driver | Used For | Module Phase |
|:------------|:---------|:-------------|
| McuHalStub | MCAL testing without hardware | Phase 1-2 |
| CanControllerStub | CanIf/CanTp testing | Phase 3 |
| LinControllerStub | LinIf/LinTp testing | Phase 3 |
| EthControllerStub | EthIf/SoAd testing | Phase 3 |
| NvMBlockStub | NvM testing | Phase 4 |
| DiagnosticServiceStub | DCM testing | Phase 4 |
| RteStub | ASW component testing | Phase 5 |

## 4. Integration Testing

Each phase includes:
- **Build verification**: All modules compile and link
- **Smoke test**: Basic functional test on target/DIL
- **Interface test**: Verify inter-module communication
- **Regression test**: Full test suite for integrated layers
