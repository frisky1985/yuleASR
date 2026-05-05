# XCP Module (Universal Measurement and Calibration Protocol)

## Overview

This module implements the XCP protocol following the ASAM XCP 1.1 standard for automotive ECU measurement and calibration.

## Features

### Supported Transport Layers
- XCP on CAN
- XCP on Ethernet (UDP/TCP)
- XCP on FlexRay

### Protocol Features
- **Standard Commands**: Connect, Disconnect, GetStatus, GetCommModeInfo, GetID, SetMTA, Upload, ShortUpload
- **Calibration**: Download, Block Download support
- **DAQ (Data Acquisition)**: Dynamic DAQ list configuration, Event-driven sampling, Timestamp support
- **STIM (Stimulation)**: Data stimulation support
- **PGM (Programming)**: Flash programming support
- **Resource Protection**: Seed/Key authentication for secured resources

## File Structure

```
src/bsw/services/xcp/
├── include/
│   ├── Xcp.h           - Public API header
│   ├── Xcp_Cfg.h       - Configuration parameters
│   └── Xcp_MemMap.h    - Memory mapping
└── src/
    └── Xcp.c           - Implementation

tests/unit/xcp/
└── test_xcp.c          - Unit tests
```

## Configuration

Key configuration parameters in `Xcp_Cfg.h`:

| Parameter | Description | Default |
|-----------|-------------|---------|
| XCP_MAX_DAQ_LISTS | Maximum number of DAQ lists | 4 |
| XCP_MAX_ODTS_PER_DAQ | Maximum ODTs per DAQ list | 8 |
| XCP_MAX_ODT_ENTRIES_PER_ODT | Maximum entries per ODT | 16 |
| XCP_MAX_CTO_SIZE | Maximum CTO size (bytes) | 8 |
| XCP_MAX_DTO_SIZE | Maximum DTO size (bytes) | 8 |
| XCP_BLOCK_DOWNLOAD_SUPPORTED | Enable block download | ON |
| XCP_TIMESTAMP_SUPPORTED | Enable timestamps | ON |
| XCP_SEED_KEY_SUPPORTED | Enable seed/key security | ON |

## API Usage

### Initialization
```c
#include "Xcp.h"

void init(void)
{
    Xcp_Init(&Xcp_Config);
}
```

### Cyclic Processing
```c
void cyclicTask(void)
{
    Xcp_MainFunction();
}
```

### Receive Data (from transport layer)
```c
void CanIf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    Xcp_RxIndication(0, RxPduId, PduInfoPtr);
}
```

## Command Support

| Command Code | Command Name | Status |
|--------------|--------------|--------|
| 0xFF | CONNECT | Implemented |
| 0xFE | DISCONNECT | Implemented |
| 0xFD | GET_STATUS | Implemented |
| 0xFB | GET_COMM_MODE_INFO | Implemented |
| 0xFA | GET_ID | Implemented |
| 0xF6 | SET_MTA | Implemented |
| 0xF5 | UPLOAD | Implemented |
| 0xF4 | SHORT_UPLOAD | Implemented |
| 0xF0 | DOWNLOAD | Implemented |
| 0xE3 | CLEAR_DAQ_LIST | Implemented |
| 0xE2 | SET_DAQ_PTR | Implemented |
| 0xE1 | WRITE_DAQ | Implemented |
| 0xE0 | SET_DAQ_LIST_MODE | Implemented |
| 0xDF | GET_DAQ_LIST_MODE | Implemented |
| 0xDE | START_STOP_DAQ_LIST | Implemented |
| 0xDD | START_STOP_SYNCH | Implemented |
| 0xDA | GET_DAQ_PROCESSOR_INFO | Implemented |
| 0xD6 | FREE_DAQ | Implemented |
| 0xD5 | ALLOC_DAQ | Implemented |
| 0xD4 | ALLOC_ODT | Implemented |
| 0xD3 | ALLOC_ODT_ENTRY | Implemented |
| 0xD2 | PROGRAM_START | Implemented |
| 0xD1 | PROGRAM_CLEAR | Implemented |
| 0xD0 | PROGRAM | Implemented |
| 0xCF | PROGRAM_RESET | Implemented |
| 0xF8 | GET_SEED | Implemented |
| 0xF7 | UNLOCK | Implemented |

## Memory Protection

The XCP module supports configurable memory ranges with access control:
- RAM: Read/Write
- Flash: Read/Write/Erase
- EEPROM: Read/Write/Erase
- Calibration RAM: Read/Write

## Testing

Run unit tests:
```bash
cd /home/admin/yuleASR
gcc -I src/bsw/common -I src/bsw/services/xcp/include \
    tests/unit/xcp/test_xcp.c src/bsw/services/xcp/src/Xcp.c \
    -o /tmp/test_xcp
/tmp/test_xcp
```

## References

- ASAM XCP Protocol Specification 1.1
- ASAM XCP on CAN Transport Layer Specification
- ASAM XCP on Ethernet Transport Layer Specification
- ASAM XCP on FlexRay Transport Layer Specification

## Version

- Version: 1.0.0
- ASAM XCP Standard: 1.1
- AutoSAR Version: 4.4
