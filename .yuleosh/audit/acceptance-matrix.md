# Acceptance Matrix

> Generated: 2026-08-25
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
| SWR-001.1-01 | SWR-001.1-01 | **SWR-001.1-01**: SHALL support AUTOSAR Classic Platform 4.4.0 standard | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c::test_Dcm_Init_ValidConfig | — |  | ✅ |
| SWR-001.1-02 | SWR-001.1-02 | **SWR-001.1-02**: SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules | Unit Test | tests/unit/autosar/mcal/test_ADC.c::test_init_deinit | — |  | ✅ |
| SWR-001.1-03 | SWR-001.1-03 | **SWR-001.1-03**: SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules | Unit Test | tests/unit/autosar/ecual/test_CanIf.c::test_CanIf_Init_ValidConfig | — |  | ✅ |
| SWR-001.1-04 | SWR-001.1-04 | **SWR-001.1-04**: SHALL implement BSW Services layer covering 44 service modules | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c::test_Dcm_MainFunction_Initialized | — |  | ✅ |
| SWR-001.1-05 | SWR-001.1-05 | **SWR-001.1-05**: SHALL target NXP S32K312 microcontroller platform | Unit Test | tests/unit/autosar/mcal/test_mcu.c::mcu_init_valid_config | — |  | ✅ |
| SWR-001.1-06 | SWR-001.1-06 | **SWR-001.1-06**: SHALL support RTE generation for SWC-to-BSW communication | Unit Test | — | — |  | ❌ |
| SWR-002.1-01 | SWR-002.1-01 | **SWR-002.1-01**: SHALL implement E2E communication protection for safety-critical signals | Unit Test | tests/unit/autosar/services/E2E/test_E2E.c::test_E2E_P01_Protect_Check_RoundTrip | — |  | ✅ |
| SWR-002.1-02 | SWR-002.1-02 | **SWR-002.1-02**: SHALL support HSM-based cryptographic operations via Crypto module | Unit Test | tests/unit/autosar/mcal/test_Crypto.c::test_init_deinit | — |  | ✅ |
| SWR-002.1-03 | SWR-002.1-03 | **SWR-002.1-03**: SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition | Unit Test | — | — |  | ❌ |
| SWR-002.1-04 | SWR-002.1-04 | **SWR-002.1-04**: SHALL implement secure boot mechanism | Unit Test | — | — |  | ❌ |
| SWR-003.1-01 | SWR-003.1-01 | **SWR-003.1-01**: SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm) | Unit Test | tests/unit/autosar/mcal/test_CAN.c::test_init | — |  | ✅ |
| SWR-003.1-02 | SWR-003.1-02 | **SWR-003.1-02**: SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM) | Unit Test | tests/unit/autosar/mcal/test_LIN.c::test_init_deinit | — |  | ✅ |
| SWR-003.1-03 | SWR-003.1-03 | **SWR-003.1-03**: SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd) | Unit Test | — | — |  | ❌ |
| SWR-003.1-04 | SWR-003.1-04 | **SWR-003.1-04**: SHALL implement DCM diagnostic communication manager | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c::test_Dcm_MainFunction_Uninit | — |  | ✅ |
| SWR-003.1-05 | SWR-003.1-05 | **SWR-003.1-05**: SHALL implement DoIP diagnostic over IP | Unit Test | — | — |  | ❌ |
| SWR-004.1-01 | SWR-004.1-01 | **SWR-004.1-01**: SHALL implement NVRAM manager (NvM) for persistent storage | Unit Test | tests/unit/autosar/services/Nvm/test_Nvm.c::test_NvM_Init_ValidConfig | — |  | ✅ |
| SWR-004.1-02 | SWR-004.1-02 | **SWR-004.1-02**: SHALL implement Flash EEPROM emulation (Fee) | Unit Test | — | — |  | ❌ |
| SWR-004.1-03 | SWR-004.1-03 | **SWR-004.1-03**: SHALL implement internal/external EEPROM driver | Unit Test | — | — |  | ❌ |
| SWR-004.1-04 | SWR-004.1-04 | **SWR-004.1-04**: SHALL implement memory abstraction interface (MemIf) | Unit Test | — | — |  | ❌ |
| SWR-004.1-05 | SWR-004.1-05 | **SWR-004.1-05**: SHALL support flash driver for S32K312 on-chip flash | Unit Test | — | — |  | ❌ |
| SWR-005.1-01 | SWR-005.1-01 | **SWR-005.1-01**: SHALL implement ECU state manager (EcuM) | Unit Test | tests/unit/services/test_ecum.c::ecum_init_startup_state | — |  | ✅ |
| SWR-005.1-02 | SWR-005.1-02 | **SWR-005.1-02**: SHALL implement BSW scheduler (BswM) with mode management | Unit Test | — | — |  | ❌ |
| SWR-005.1-03 | SWR-005.1-03 | **SWR-005.1-03**: SHALL implement Watchdog manager (WdgM) | Unit Test | tests/unit/autosar/mcal/test_wdg.c::test_wdg_Init_should_initialize_successfully | — |  | ✅ |
| SWR-005.1-04 | SWR-005.1-04 | **SWR-005.1-04**: SHALL implement Default Error Tracer (Det) | Unit Test | — | — |  | ❌ |
| SWR-005.1-05 | SWR-005.1-05 | **SWR-005.1-05**: SHALL implement Diagnostic Event Manager (Dem) | Unit Test | — | — |  | ❌ |
| SWR-005.1-06 | SWR-005.1-06 | **SWR-005.1-06**: SHALL implement Function Inhibition Manager (FiM) | Unit Test | — | — |  | ❌ |
| SWR-005.1-07 | SWR-005.1-07 | **SWR-005.1-07**: SHALL implement CRC calculator | Unit Test | — | — |  | ❌ |
| SWR-005.1-08 | SWR-005.1-08 | **SWR-005.1-08**: SHALL implement OS (AUTOSAR SC4 compliant) | Unit Test | tests/unit/test_os_timing.c::test_Os_Timing_Execution_Budget | — |  | ✅ |
| SWR-005.1-09 | SWR-005.1-09 | **SWR-005.1-09**: SHALL support DLT (Diagnostic Log and Trace) | Unit Test | — | — |  | ❌ |
| SWR-006.1-01 | SWR-006.1-01 | **SWR-006.1-01**: SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG) | Unit Test | tests/unit/autosar/mcal/test_gpt.c::test_init_valid | — |  | ✅ |
| SWR-006.1-02 | SWR-006.1-02 | **SWR-006.1-02**: SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap) | Unit Test | — | — |  | ❌ |
| SWR-007.1-01 | SWR-007.1-01 | **SWR-007.1-01**: SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager | Unit Test | tests/unit/services/test_comm.c::comm_init_valid_config | — |  | ✅ |
| SWR-007.1-02 | SWR-007.1-02 | **SWR-007.1-02**: SHALL implement RTE for component communication | Unit Test | — | — |  | ❌ |
| SWR-008.1-01 | SWR-008.1-01 | **SWR-008.1-01**: SHALL integrate micro DDS middleware for inter-ECU communication | Unit Test | tests/unit/test_dds_qualification.c::test_dds_init_and_config | — |  | ✅ |

## Summary
- Total SHALL statements: 47
- Covered by tests: 17 (36%)
- Uncovered: 30
- Threshold: 100% → ❌ FAIL
