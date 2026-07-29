# PduR - PDU Router

## Overview

PduR implements AUTOSAR PDU Router, providing routing of Protocol Data Units (PDUs) between different communication protocol layers. It enables flexible routing between upper and lower layer modules.

## Standards

- AUTOSAR SWS PDU Router
- AUTOSAR Classic Platform 4.4.0
- MISRA C:2012

## Features

### Routing Capabilities
- **1:1 Routing** - Single source to single destination
- **1:N Routing** - Single source to multiple destinations (multicast)
- **Gateway Routing** - Route between different bus systems
- **FIFO Routing** - Buffer PDUs in FIFO queues

### Routing Path Types
- **Direct** - Immediate routing
- **FIFO** - Queued routing with FIFO buffer
- **Gateway** - Inter-bus routing

### Protocol Support
- **COM** - Communication services
- **DCM** - Diagnostic communication
- **CanIf** - CAN Interface
- **FrIf** - FlexRay Interface
- **EthIf** - Ethernet Interface
- **CanTp** - CAN Transport Protocol
- **FrTp** - FlexRay Transport Protocol

### Routing Path Groups
- **Group Control** - Enable/disable routing groups
- **Runtime Configuration** - Dynamic routing control

## Architecture

```
┌───────────────────────────────────────────────────────────┐
│                    Upper Layers                           │
│    (COM, DCM, J1939TP, SecOC, SOME/IP, etc.)             │
└────────────────┬────────────────────────────────────┘
                  │
                  v
┌───────────────────────────────────────────────────────────┐
│                PDU Router (PduR)                          │
│  ┌─────────────────────────────────────────────────────┐          │
│  │  Routing Engine                                         │          │
│  │  - Route lookup          ┌──────────────────────────┐          │
│  │  - Path selection -----> │  Routing Table    │          │
│  │  - Destination dispatch  │  (Src -> Dest)    │          │
│  │                         └──────────────────────────┘          │
│  └─────────────────────────────────────────────────────┘          │
│  ┌─────────────────────────────────────────────────────┐          │
│  │  FIFO Management                                        │          │
│  │  - FIFO buffers                                         │          │
│  │  - Queue management                                     │          │
│  │  - Overflow handling                                    │          │
│  └─────────────────────────────────────────────────────┘          │
│  ┌─────────────────────────────────────────────────────┐          │
│  │  TP Buffer Management                                   │          │
│  │  - TP buffer allocation                                 │          │
│  │  - Copy Rx/Tx data                                      │          │
│  │  - Flow control                                         │          │
│  └─────────────────────────────────────────────────────┘          │
└─────────────┬──────────────────────────┬─────────────┘
              │                   │
    ┌─────────┴─────────┐     v
    v                   v         ┌─────────┐
┌─────────┐      ┌─────────────┐       │  EthIf  │
│  CanIf   │      │     FrIf       │       └─────────┘
│  (CAN)   │      │  (FlexRay)    │
└─────────┘      └─────────────┘
```

## Routing Path Types

| Type | Description |
|------|-------------|
| `PDUR_ROUTING_PATH_DIRECT` | Direct immediate routing |
| `PDUR_ROUTING_PATH_FIFO` | FIFO queued routing |
| `PDUR_ROUTING_PATH_GATEWAY` | Gateway routing between buses |

## Destination PDU Processing

| Type | Description |
|------|-------------|
| `PDUR_DESTPDU_PROCESSING_IMMEDIATE` | Process immediately |
| `PDUR_DESTPDU_PROCESSING_DEFERRED` | Process deferred (queued) |

## Return Types

| Return | Value | Description |
|--------|-------|-------------|
| `PDUR_OK` | 0 | Operation successful |
| `PDUR_NOT_OK` | 1 | Operation failed |
| `PDUR_BUSY` | 2 | Module busy |
| `PDUR_E_SDU_MISMATCH` | 3 | SDU length mismatch |

## APIs

### Core APIs
| API | Function |
|-----|----------|
| `PduR_Init()` | Initialize PduR |
| `PduR_DeInit()` | Deinitialize PduR |
| `PduR_GetVersionInfo()` | Get version information |

### Transmission APIs
| API | Function |
|-----|----------|
| `PduR_Transmit()` | Transmit a PDU |
| `PduR_CancelTransmitRequest()` | Cancel transmit request |
| `PduR_CancelReceiveRequest()` | Cancel receive request |
| `PduR_ChangeParameterRequest()` | Change routing parameter |

### Routing Control APIs
| API | Function |
|-----|----------|
| `PduR_EnableRouting()` | Enable routing path group |
| `PduR_DisableRouting()` | Disable routing path group |

### Callback APIs (Upper Layer)
| API | Function |
|-----|----------|
| `PduR_TxConfirmation()` | Transmit confirmation |
| `PduR_RxIndication()` | Receive indication |
| `PduR_TriggerTransmit()` | Trigger transmit callback |

### Callback APIs (FrTp)
| API | Function |
|-----|----------|
| `PduR_FrTpTxConfirmation()` | FrTp transmit confirmation |
| `PduR_FrTpRxIndication()` | FrTp receive indication |
| `PduR_FrTpStartOfReception()` | Start of reception |
| `PduR_FrTpCopyRxData()` | Copy received data |
| `PduR_FrTpCopyTxData()` | Copy transmit data |

### Module Mappings
| API | Maps To |
|-----|---------|
| `PduR_ComTransmit()` | `PduR_Transmit()` |
| `PduR_DcmTransmit()` | `PduR_Transmit()` |
| `PduR_CanIfRxIndication()` | `PduR_RxIndication()` |
| `PduR_CanIfTxConfirmation()` | `PduR_TxConfirmation()` |
| `PduR_CanIfTriggerTransmit()` | `PduR_TriggerTransmit()` |

## Configuration

### Routing Path Configuration
```c
typedef struct {
    PduR_SrcPduConfigType SrcPdu;
    const PduR_DestPduConfigType* DestPdus;
    uint8 NumDestPdus;
    PduR_RoutingPathType PathType;
    boolean GatewayOperation;
} PduR_RoutingPathConfigType;
```

### Source PDU Configuration
```c
typedef struct {
    PduIdType SourcePduId;
    uint8 SourceModule;
    PduLengthType SduLength;
} PduR_SrcPduConfigType;
```

### Destination PDU Configuration
```c
typedef struct {
    PduIdType DestPduId;
    uint8 DestModule;
    PduR_DestPduProcessingType Processing;
    uint8 FifoDepth;
} PduR_DestPduConfigType;
```

### Routing Path Group Configuration
```c
typedef struct {
    uint8 GroupId;
    const PduIdType* PduIds;
    uint8 NumPduIds;
    boolean DefaultEnabled;
} PduR_RoutingPathGroupConfigType;
```

### Global Configuration
```c
typedef struct {
    const PduR_RoutingPathConfigType* RoutingPaths;
    uint8 NumRoutingPaths;
    const PduR_RoutingPathGroupConfigType* RoutingPathGroups;
    uint8 NumRoutingPathGroups;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} PduR_ConfigType;
```

## Usage Examples

### Basic Routing Configuration
```c
#include "PduR.h"

/* Source PDU configuration */
const PduR_SrcPduConfigType SrcPduConfig = {
    .SourcePduId = 0x100,       /* COM PDU ID */
    .SourceModule = PDUR_COM,   /* Source module */
    .SduLength = 8              /* Data length */
};

/* Destination PDU configuration */
const PduR_DestPduConfigType DestPduConfigs[] = {
    {
        .DestPduId = 0x50,          /* CanIf PDU ID */
        .DestModule = PDUR_CANIF,   /* Destination module */
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
        .FifoDepth = 0
    }
};

/* Routing path configuration */
const PduR_RoutingPathConfigType RoutingPaths[] = {
    {
        .SrcPdu = SrcPduConfig,
        .DestPdus = DestPduConfigs,
        .NumDestPdus = 1,
        .PathType = PDUR_ROUTING_PATH_DIRECT,
        .GatewayOperation = FALSE
    }
};

/* Global configuration */
const PduR_ConfigType PduR_Config = {
    .RoutingPaths = RoutingPaths,
    .NumRoutingPaths = 1,
    .RoutingPathGroups = NULL,
    .NumRoutingPathGroups = 0,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE
};
```

### Transmitting via PduR
```c
void SendPdu(void)
{
    PduInfoType pduInfo;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Std_ReturnType result;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    /* Transmit PDU through PduR */
    result = PduR_Transmit(0x100, &pduInfo);  /* COM PDU ID */
    
    if (result == E_OK) {
        /* Transmission accepted */
    }
}
```

### Gateway Routing
```c
/* Gateway routing from CAN to FlexRay */
const PduR_RoutingPathConfigType GatewayRouting = {
    .SrcPdu = {
        .SourcePduId = 0x50,        /* CanIf RX PDU */
        .SourceModule = PDUR_CANIF,
        .SduLength = 8
    },
    .DestPdus = (const PduR_DestPduConfigType[]){
        {
            .DestPduId = 0x30,      /* FrIf TX PDU */
            .DestModule = PDUR_FRIF,
            .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
            .FifoDepth = 0
        }
    },
    .NumDestPdus = 1,
    .PathType = PDUR_ROUTING_PATH_GATEWAY,
    .GatewayOperation = TRUE
};
```

### Routing Group Control
```c
void EnableDiagnosticRouting(void)
{
    /* Enable routing path group for diagnostic PDUs */
    PduR_EnableRouting(PDUR_GROUP_DIAGNOSTIC);
}

void DisableDiagnosticRouting(void)
{
    /* Disable routing path group for diagnostic PDUs */
    PduR_DisableRouting(PDUR_GROUP_DIAGNOSTIC);
}
```

### FIFO Routing
```c
/* FIFO routing configuration */
const PduR_RoutingPathConfigType FifoRouting = {
    .SrcPdu = {
        .SourcePduId = 0x200,
        .SourceModule = PDUR_COM,
        .SduLength = 64
    },
    .DestPdus = (const PduR_DestPduConfigType[]){
        {
            .DestPduId = 0x60,
            .DestModule = PDUR_CANTP,
            .Processing = PDUR_DESTPDU_PROCESSING_DEFERRED,
            .FifoDepth = 4          /* FIFO buffer depth */
        }
    },
    .NumDestPdus = 1,
    .PathType = PDUR_ROUTING_PATH_FIFO,
    .GatewayOperation = FALSE
};
```

### Multicast Routing (1:N)
```c
/* Multicast routing to multiple destinations */
const PduR_RoutingPathConfigType MulticastRouting = {
    .SrcPdu = {
        .SourcePduId = 0x100,
        .SourceModule = PDUR_COM,
        .SduLength = 8
    },
    .DestPdus = (const PduR_DestPduConfigType[]){
        {
            .DestPduId = 0x50,      /* CanIf */
            .DestModule = PDUR_CANIF,
            .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
            .FifoDepth = 0
        },
        {
            .DestPduId = 0x60,      /* FrIf */
            .DestModule = PDUR_FRIF,
            .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
            .FifoDepth = 0
        },
        {
            .DestPduId = 0x70,      /* EthIf */
            .DestModule = PDUR_ETHIF,
            .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE,
            .FifoDepth = 0
        }
    },
    .NumDestPdus = 3,
    .PathType = PDUR_ROUTING_PATH_DIRECT,
    .GatewayOperation = FALSE
};
```

## Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `PDUR_E_PARAM_POINTER` | 0x01 | Invalid pointer parameter |
| `PDUR_E_PARAM_CONFIG` | 0x02 | Invalid configuration |
| `PDUR_E_INVALID_REQUEST` | 0x03 | Invalid request |
| `PDUR_E_PDU_ID_INVALID` | 0x04 | Invalid PDU ID |
| `PDUR_E_ROUTING_PATH_GROUP_INVALID` | 0x05 | Invalid routing group |
| `PDUR_E_PARAM_INVALID` | 0x06 | Invalid parameter |
| `PDUR_E_UNINIT` | 0x07 | Module not initialized |

## Dependencies

- **COM** - Communication services
- **DCM** - Diagnostic communication
- **CanIf** - CAN Interface
- **FrIf** - FlexRay Interface
- **EthIf** - Ethernet Interface
- **CanTp** - CAN Transport Protocol
- **FrTp** - FlexRay Transport Protocol
- **DET** - Development error tracing

## Source Code

- `/home/admin/yuleASR/src/bsw/services/pdur/`
  - `include/PduR.h` - Public API
  - `include/PduR_Cfg.h` - Configuration
  - `src/PduR.c` - Core implementation
  - `src/PduR_Lcfg.c` - Link-time configuration

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/PduR/`

## References

- AUTOSAR_SWS_PDURouter
- AUTOSAR Classic Platform 4.4.0
