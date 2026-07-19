# DoIP - Diagnostic over IP Module

## Overview

DoIP implements ISO 13400-2 diagnostic communication over IP for modern vehicles equipped with Ethernet-based diagnostic interfaces.

## Standards

- ISO 13400-2:2019 - Diagnostic communication over Internet Protocol
- AUTOSAR SWS Diagnostic over IP

## Features

### Vehicle Discovery
- Vehicle identification request/response
- Vehicle announcement message
- Entity status request/response

### Connection Management
- Routing activation (0x0005)
- Alive check (0x0007)
- Diagnostic power mode (0x0040)

### Diagnostic Communication
- Diagnostic message (0x8001)
- Diagnostic message positive/negative acknowledge

## Payload Types

| Type | Value | Description |
|------|-------|-------------|
| Vehicle Identification Request | 0x0001 | Request vehicle ID |
| Vehicle Identification Response | 0x0004 | Vehicle ID info |
| Routing Activation Request | 0x0005 | Request routing activation |
| Routing Activation Response | 0x0006 | Routing activation response |
| Alive Check Request | 0x0007 | Check connection alive |
| Alive Check Response | 0x0008 | Alive check response |
| Diagnostic Message | 0x8001 | UDS diagnostic message |
| Diagnostic Message ACK | 0x8002 | Positive acknowledge |
| Diagnostic Message NACK | 0x8003 | Negative acknowledge |

## APIs

| API | Function |
|-----|----------|
| `DoIP_Init()` | Initialize DoIP module |
| `DoIP_DeInit()` | Deinitialize DoIP module |
| `DoIP_GetVersionInfo()` | Get version info |
| `DoIP_MainFunction()` | Periodic processing |
| `DoIP_IfTransmit()` | Transmit callback |
| `DoIP_IfRxIndication()` | Reception callback |

## Configuration

### Pre-compile
- `DOIP_VERSION_INFO_API` - Enable version info
- `DOIP_DEV_ERROR_DETECT` - Enable error detection
- `DOIP_VEHICLE_ANNOUNCEMENT_INTERVAL` - Announcement interval in ms

### Link-time
- Connection configurations
- Routing activation types
- UDS buffer configurations

## Dependencies

- SoAd (Socket Adapter)
- PduR (PDU Router)
- DCM (Diagnostic Communication Manager)
- DET (Development Error Tracer)

## Usage Example

```c
#include "DoIP.h"

void DoIP_Example(void)
{
    /* Initialize DoIP */
    DoIP_Init(&DoIP_Config);

    /* Main processing loop */
    while (1) {
        DoIP_MainFunction();
        /* Handle routing activation requests */
        /* Process diagnostic messages */
    }
}
```

## Source Code

- `/home/admin/yuleASR/src/bsw/services/doip/`

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/DoIP/`
