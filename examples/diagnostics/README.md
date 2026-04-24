# Diagnostics System - DDS Examples

AUTOSAR-compliant vehicle diagnostics system examples using DDS communication.

## Overview

This example demonstrates vehicle diagnostics:

1. **DTC Manager** - Diagnostic Trouble Code monitoring
2. **Routine Controller** - Diagnostic routine execution

## Safety Level

- **ASIL-B** for diagnostic data
- E2E Profile 6 (CRC16 + Counter)

## Topics

| Topic | Type | Rate | E2E | Safety |
|-------|------|------|-----|--------|
| Diagnostics/DTC/Status | DtcStatusType | 1 Hz | P06 | ASIL-B |
| Diagnostics/Routine/Results | RoutineResultType | 2 Hz | P06 | ASIL-B |

## Build & Run

```bash
mkdir build && cd build
cmake ..
make
./run_diagnostics_demo.sh
```

## Features

### DTC Management
- SAE J2012 DTC format
- Snapshot records
- Extended data
- DTC aging
- UDS services

### Routine Control
- Self-test routines
- Memory testing
- Progress reporting
- Security access

## Configuration

- `diagnostics_config.yaml` - System configuration
- `e2e_config.yaml` - E2E protection settings

## License

Part of AUTOSAR DDS Integration Project
