---
title: LinIf
sidebar_label: LinIf
description: "LinIf implements the AUTOSAR LIN Interface, providing an abstraction layer between the LIN driver (MCAL) and upper layer"
sidebar_position: 19
---

# LinIf - LIN Interface Module

## Overview

LinIf implements the AUTOSAR LIN Interface, providing an abstraction layer between the LIN driver (MCAL) and upper layers (PduR, LinTp). It manages LIN communication scheduling and frame handling for automotive LIN bus networks.

## Standards

- AUTOSAR SWS LIN Interface
- AUTOSAR Classic Platform 4.4.0
- LIN 2.2A / ISO 17987

## Features

### Frame Types

| Type | Description |
|------|-------------|
| Unconditional Frame | Standard scheduled frame |
| Event Triggered Frame | Collision-resolving frame |
| Sporadic Frame | On-demand transmission frame |

### Schedule Table Management

1. **Null Schedule** - No communication
2. **Diagnostic Schedules** - Master request / Slave response
3. **Normal Schedule** - Regular communication
4. **Master Schedule** - Master node only
5. **Sporadic Schedule** - Mixed unconditional/sporadic frames

### Node Management

- Master node scheduling control
- Slave node response handling
- Sleep/wake-up management

## APIs

### Core APIs

| API | Function |
|-----|----------|
| `LinIf_Init()` | Initialize LIN Interface |
| `LinIf_DeInit()` | Deinitialize LIN Interface |
| `LinIf_GetVersionInfo()` | Get version information |
| `LinIf_MainFunction()` | Periodic processing |

### Transmission APIs

| API | Function |
|-----|----------|
| `LinIf_Transmit()` | Transmit LIN frame |
| `LinIf_ScheduleRequest()` | Request schedule table change |

### Power Management APIs

| API | Function |
|-----|----------|
| `LinIf_WakeUp()` | Send wake-up signal |
| `LinIf_GotoSleep()` | Request sleep mode |

### Callback APIs

| API | Function |
|-----|----------|
| `LinIf_RxIndication()` | Reception callback |
| `LinIf_TxConfirmation()` | Transmission confirmation |
| `LinIf_WakeUpConfirmation()` | Wake-up confirmation |

## Configuration

### Pre-compile

| Parameter | Description |
|-----------|-------------|
| `LINIF_VERSION_INFO_API` | Enable version info API |
| `LINIF_DEV_ERROR_DETECT` | Enable error detection |
| `LINIF_MAX_CHANNELS` | Maximum number of LIN channels |

### Schedule Configuration

```c
typedef struct {
    uint16 Delay;           /* Entry delay in ticks */
    uint16 FrameIdx;        /* Frame index */
} LinIf_ScheduleEntryType;

typedef struct {
    LinIf_ScheduleTableType Schedule;
    uint8 EntryCount;
    const LinIf_ScheduleEntryType* Entries;
} LinIf_ScheduleTableConfigType;
```

### Frame Configuration

| Parameter | Description |
|-----------|-------------|
| FrameIdx | Frame index |
| Pid | Protected identifier (0-63) |
| Dlc | Data length (1-8 bytes) |
| FrameType | Unconditional/Event/Sporadic |
| IsPublish | Publish or subscribe |

## Schedule Types

| Schedule | Usage |
|----------|-------|
| LINIF_NULL_SCHEDULE | Idle state |
| LINIF_DIAGRequest | Diagnostic master request |
| LINIF_DIAGResponse | Diagnostic slave response |
| LINIF_MasterReqSchedule | Master command schedule |
| LINIF_SlaveRespSchedule | Slave response schedule |
| LINIF_Normal | Normal operational schedule |
| LINIF_Master | Master-only schedule |
| LINIF_Sporadic | Mixed frame schedule |

## Dependencies

- **Lin** (MCAL) - LIN hardware driver
- **PduR** - PDU Router for diagnostic frames
- **LinTp** - LIN Transport Protocol (optional)
- **DET** - Development Error Tracer (optional)
- **SchM** - Schedule Manager for critical sections

## Usage Example

```c
#include "LinIf.h"

void LinIf_Example(void)
{
    /* Initialize LIN Interface */
    LinIf_Init(&LinIf_Config);

    /* Request diagnostic schedule */
    Std_ReturnType result = LinIf_ScheduleRequest(0, LINIF_DIAGRequest);

    if (result == E_OK) {
        /* Schedule change accepted */
    }

    /* Transmit frame */
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;

    LinIf_Transmit(LINIF_TX_PDU_ID, &pduInfo);

    /* Main processing loop */
    while (1) {
        LinIf_MainFunction();
        /* Schedule processing happens here */
    }
}
```

## Error Handling

### DET Error Codes

| Code | Description |
|------|-------------|
| LINIF_E_PARAM_POINTER | NULL pointer error |
| LINIF_E_UNINIT | Module not initialized |
| LINIF_E_INVALID_CHANNEL | Invalid channel ID |
| LINIF_E_INVALID_PDU | Invalid PDU ID |

## State Machine

```
UNINIT → INIT → RUNNING
  ↑       ↓       ↓
  └───────┴───────┘
```

## Source Code

- `/home/admin/yuleASR/src/bsw/ecual/linif/`
  - `include/LinIf.h` - Public API
  - `include/LinIf_Cfg.h` - Configuration
  - `src/LinIf.c` - Implementation
  - `src/LinIf_Lcfg.c` - Link-time configuration

## References

- AUTOSAR_SWS_LINInterface
- LIN Specification 2.2A
- ISO 17987 - Road vehicles - Local Interconnect Network (LIN)
