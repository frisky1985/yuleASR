---
title: Dcm
sidebar_label: Dcm
description: "DCM implements AUTOSAR Diagnostic Communication Manager with complete UDS (Unified Diagnostic Services) support per ISO"
sidebar_position: 8
---

# DCM - Diagnostic Communication Manager

## Overview

DCM implements AUTOSAR Diagnostic Communication Manager with complete UDS (Unified Diagnostic Services) support per ISO 14229-1. It handles diagnostic communication between tester and ECU.

## Standards

- ISO 14229-1:2020 - UDS diagnostic services
- ISO 15765-2:2016 - CAN transport protocol
- ISO 13400-2:2019 - Diagnostic over IP
- AUTOSAR SWS Diagnostic Communication Manager

## UDS Services

### Core Services
| Service | SID | Description |
|---------|-----|-------------|
| Diagnostic Session Control | 0x10 | Manage diagnostic sessions |
| ECU Reset | 0x11 | Reset the ECU |
| Security Access | 0x27 | Authentication/authorization |
| Tester Present | 0x3E | Keep session alive |
| Read Data By Identifier | 0x22 | Read DIDs |
| Write Data By Identifier | 0x2E | Write DIDs |
| Routine Control | 0x31 | Execute routines |

### Data Transfer Services
| Service | SID | Description |
|---------|-----|-------------|
| Request Download | 0x34 | Initiate software download |
| Request Upload | 0x35 | Initiate software upload |
| Transfer Data | 0x36 | Transfer data blocks |
| Request Transfer Exit | 0x37 | Complete transfer |

### OBD-II Services
| Service | SID | Description |
|---------|-----|-------------|
| Read DTC Information | 0x19 | Read diagnostic trouble codes |
| Clear DTC | 0x14 | Clear DTCs |
| Input Output Control | 0x2F | Control I/O |

## Architecture

```
Tester
  |
  | UDS on CAN/DoIP
  v
DCM (Diagnostic Services)
  |
  +---> DCM Core (Session/Security)
  +---> DCM Services (0x10, 0x11, 0x22, ...)
  +---> Transfer Services (0x34-0x37)
  +---> OBD-II Handler
  |
  v
DEM (Diagnostic Event Manager)
NVM (Non-Volatile Memory)
SwC (Application Software)
```

## Session Management

### Diagnostic Sessions
| Session | Value | Description |
|---------|-------|-------------|
| Default | 0x01 | Standard operational session |
| Programming | 0x02 | Software update session |
| Extended | 0x03 | Extended diagnostic session |
| Safety | 0x04 | Safety system diagnostics |

### Security Levels
| Level | Value | Access |
|-------|-------|--------|
| Locked | 0x00 | No security access |
| Level 1 | 0x01 | Basic diagnostics |
| Level 2 | 0x02 | Advanced diagnostics |
| Level 3 | 0x03 | Programming access |

## Transfer Services (0x34-0x37)

### Software Update Flow
```
1. Diagnostic Session Control (0x10) -> Programming
2. Security Access (0x27) -> Unlock
3. Request Download (0x34) -> Specify address/size
4. Transfer Data (0x36) [repeat] -> Send data blocks
5. Request Transfer Exit (0x37) -> Complete transfer
6. Routine Control (0x31) -> Verify/Activate
```

### Block Sequence Counter
- 8-bit counter (0x00-0xFF)
- Wraps from 0xFF to 0x00
- Used for flow control

## OBD-II Support

### Mode 01 - Current Data
- PIDs (Parameter IDs) for real-time data
- Engine RPM, Vehicle Speed, Coolant Temp, etc.

### Mode 03 - DTCs
- Read stored diagnostic trouble codes
- Format: Pxxxx, Bxxxx, Cxxxx, Uxxxx

### Mode 04 - Clear DTCs
- Erase all stored DTCs
- Reset readiness monitors

## APIs

| API | Function |
|-----|----------|
| `Dcm_Init()` | Initialize DCM |
| `Dcm_DeInit()` | Deinitialize DCM |
| `Dcm_GetVersionInfo()` | Get version |
| `Dcm_MainFunction()` | Periodic processing |
| `Dcm_ProcessRequest()` | Process UDS request |
| `Dcm_RxIndication()` | Reception callback |

## Configuration

### Pre-compile
- `DCM_VERSION_INFO_API` - Enable version info
- `DCM_DEV_ERROR_DETECT` - Enable error detection
- `DCM_OBD_SUPPORT` - Enable OBD-II support

### Link-time
- Session configurations
- Security level settings
- DID database
- Routine identifiers

## Dependencies

- PduR (PDU Router)
- DEM (Diagnostic Event Manager)
- NVM (Non-Volatile Memory)
- DET (Development Error Tracer)

## Source Code

- `/home/admin/yuleASR/src/bsw/services/dcm/`
  - `include/Dcm.h` - Public API
  - `include/Dcm_Cfg.h` - Configuration
  - `include/dcm_transfer.h` - Transfer services
  - `include/Dcm_Obd.h` - OBD-II support
  - `src/Dcm.c` - Core implementation (1454 lines)
  - `src/dcm_transfer.c` - Transfer services (1278 lines)
  - `src/Dcm_Obd.c` - OBD-II implementation (209 lines)

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/Dcm/`

## References

- ISO 14229-1:2020 - UDS
- ISO 15765-2:2016 - DoCAN
- ISO 13400-2:2019 - DoIP
- AUTOSAR_SWS_DiagnosticCommunicationManager
