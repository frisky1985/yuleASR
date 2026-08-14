# yuleASR — Module Requirements by Functional Area

## BSW Services Requirements

### Diagnostic Services
| ID | Requirement | Priority |
|:---|:------------|:---------|
| DCM-REQ-01 | SHALL support ISO 14229-1 UDS diagnostic services | High |
| DCM-REQ-02 | SHALL support session management (default, programming, extended) | High |
| DEM-REQ-01 | SHALL support DTC storage and retrieval | High |
| DET-REQ-01 | SHALL report development errors with module ID and error code | Medium |
| DOIP-REQ-01 | SHALL support DoIP vehicle discovery | Medium |

### Communication Services
| ID | Requirement | Priority |
|:---|:------------|:---------|
| COM-REQ-01 | SHALL support signal-based I-PDU communication | High |
| PDUR-REQ-01 | SHALL route I-PDUs between COM and transport layers | High |
| CANSM-REQ-01 | SHALL manage CAN network state machine | High |
| LIN-REQ-01 | SHALL support LIN master/slave communication | High |

### Memory Services
| ID | Requirement | Priority |
|:---|:------------|:---------|
| NVM-REQ-01 | SHALL support NV block read/write with redundancy | High |
| FEE-REQ-01 | SHALL emulate EEPROM over flash with wear leveling | High |
| MEMIF-REQ-01 | SHALL abstract NvM from Fee/EEP driver | High |

### System Services
| ID | Requirement | Priority |
|:---|:------------|:---------|
| ECUM-REQ-01 | SHALL manage ECU startup/shutdown/wakeup sequences | High |
| BSWM-REQ-01 | SHALL implement mode-based BSW scheduling | High |
| WDGM-REQ-01 | SHALL monitor alive and deadline supervision | High |
| OS-REQ-01 | SHALL provide AUTOSAR OS SC4 compliant scheduling | High |

### Safety & Security
| ID | Requirement | Priority |
|:---|:------------|:---------|
| E2E-REQ-01 | SHALL protect safety-critical signals with CRC and sequence counter | High |
| CSM-REQ-01 | SHALL provide cryptographic service management | High |
| KEYM-REQ-01 | SHALL manage cryptographic keys | Medium |

## MCAL Driver Requirements

| ID | Module | Requirement | Priority |
|:---|:-------|:------------|:---------|
| ADC-REQ-01 | ADC | SHALL support 10-bit to 12-bit conversion | High |
| CAN-REQ-01 | CAN | SHALL support Classical CAN and CAN FD | High |
| CRYPTO-REQ-01 | Crypto | SHALL support AES-128/256, ECC, and SHA-256 | High |
| DIO-REQ-01 | DIO | SHALL support digital I/O read/write | High |
| ETH-REQ-01 | ETH | SHALL support 100BASE-T1 Ethernet | Medium |
| GPT-REQ-01 | GPT | SHALL provide hardware timer for OS ticks | High |
| I2C-REQ-01 | I2C | SHALL support I2C master/slave communication | Medium |
| ICU-REQ-01 | ICU | SHALL support input capture and signal measurement | High |
| LIN-REQ-01 | LIN | SHALL support LIN 2.2A specification | High |
| MCU-REQ-01 | MCU | SHALL manage MCU clock, reset, and power modes | High |
| PORT-REQ-01 | PORT | SHALL configure pin muxing and pad properties | High |
| PWM-REQ-01 | PWM | SHALL generate PWM signals with configurable duty cycle | Medium |
| SPI-REQ-01 | SPI | SHALL support SPI master/slave data transfer | High |
| WDG-REQ-01 | WDG | SHALL support watchdog trigger and timeout configuration | High |

## ECUAL Module Requirements

| ID | Module | Requirement | Priority |
|:---|:-------|:------------|:---------|
| CANIF-REQ-01 | CanIf | SHALL provide CAN controller abstraction interface | High |
| CANTP-REQ-01 | CanTp | SHALL support CAN transport protocol (ISO 15765-2) | High |
| CANNM-REQ-01 | CanNm | SHALL support CAN network management | High |
| LINIF-REQ-01 | LinIf | SHALL provide LIN controller abstraction interface | High |
| LINTP-REQ-01 | LinTp | SHALL support LIN transport protocol | Medium |
| ETHIF-REQ-01 | EthIf | SHALL provide Ethernet controller abstraction | Medium |
| SOAD-REQ-01 | SoAd | SHALL provide socket adaptor for SOME/IP | High |
| DLT-REQ-01 | Dlt | SHALL support diagnostic log and trace | Medium |
| XCP-REQ-01 | Xcp | SHALL support XCP calibration protocol on CAN/Ethernet | Medium |

## ASW Component Requirements

| ID | Component | Requirement | Priority |
|:---|:----------|:------------|:---------|
| COMMGR-REQ-01 | CommunicationManager | SHALL manage CAN/LIN/Ethernet message routing | High |
| DIAGMGR-REQ-01 | DiagnosticManager | SHALL coordinate diagnostic sessions and DTC handling | High |
| ENGCTRL-REQ-01 | EngineControl | SHALL provide engine control logic interface | Medium |
| IOCTRL-REQ-01 | IOControl | SHALL abstract sensor/actuator I/O | Medium |
| MODEMGR-REQ-01 | ModeManager | SHALL manage application mode transitions | Medium |
| STORMGR-REQ-01 | StorageManager | SHALL manage non-volatile data via NvM | Medium |
| VEHDYN-REQ-01 | VehicleDynamics | SHALL provide vehicle dynamics data processing | Medium |
| WDGMGR-REQ-01 | WatchdogManager | SHALL trigger software watchdog | High |
