# RTE Generator Tool

> YuleTech AutoSAR BSW - Runtime Environment Code Generator

## Overview

The RTE Generator is a Python-based tool that automatically generates RTE interface C code from a JSON configuration file describing Software Components (SWCs), their ports, interfaces, and data elements.

## Features

- Generates `Rte_SwcName.h` and `Rte_SwcName.c` per SWC
- Supports AutoSAR Classic Platform 4.x naming conventions
- Supports multiple interface types:
  - **SenderReceiver** - Maps to `Com_SendSignal` / `Com_ReceiveSignal`
  - **NvBlock** - Maps to `NvM_ReadBlock` / `NvM_WriteBlock`
  - **ClientServer** - Generates `Rte_Call` (client) and `Rte_Result` / `Rte_Server` (server)
  - **ModeSwitch** - Generates `Rte_Switch` and `Rte_Mode` APIs
- Automatic static buffer generation for each data element
- Compatible with existing `Rte_Type.h` and `Rte_Swc.h`

## Usage

```bash
python tools/rte_generator/rte_generator.py --config <config.json> --output <output_dir>
```

### Example

```bash
python tools/rte_generator/rte_generator.py \
    --config tools/rte_generator/example_config.json \
    --output src/bsw/rte/generated/
```

## Configuration Format

The JSON configuration file contains a top-level `softwareComponents` array. Each SWC object has the following structure:

### Software Component

```json
{
  "name": "SwcName",
  "description": "Component description",
  "ports": [
    /* port definitions */
  ]
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | SWC name (used in generated file names and API prefixes) |
| `description` | string | No | Human-readable description |
| `ports` | array | Yes | List of port definitions |

### Port

```json
{
  "name": "PortName",
  "direction": "Provided|Required",
  "interfaceType": "SenderReceiver|NvBlock|ClientServer|ModeSwitch",
  "dataElements": [ /* for SR and NvBlock */ ],
  "operations": [ /* for ClientServer */ ],
  "modeGroup": "ModeGroupName" /* for ModeSwitch */
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Port name (used in API naming) |
| `direction` | string | Yes | `Provided` (P-Port) or `Required` (R-Port) |
| `interfaceType` | string | Yes | Interface type |

### Data Element (SenderReceiver / NvBlock)

```json
{
  "name": "DataElementName",
  "type": "uint16",
  "comSignalId": 100
}
```

For NvBlock with struct types:

```json
{
  "name": "EngineConfig",
  "type": {
    "kind": "struct",
    "fields": [
      { "name": "maxRpm", "type": "uint16" },
      { "name": "idleSpeed", "type": "uint8" }
    ]
  },
  "nvmBlockId": 10
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Data element name |
| `type` | string / object | Yes | C type name or struct definition |
| `comSignalId` | number | No | COM signal ID (for SenderReceiver) |
| `nvmBlockId` | number | No | NVM block ID (for NvBlock) |

### Operation (ClientServer)

```json
{
  "name": "OperationName",
  "arguments": [
    {
      "name": "argName",
      "type": "uint8",
      "direction": "in"
    },
    {
      "name": "result",
      "type": "uint16",
      "direction": "out"
    }
  ]
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Operation name |
| `arguments` | array | Yes | List of operation arguments |

## Generated Code Naming Convention

All generated code follows AutoSAR 4.x naming standards:

| Interface Type | Direction | Generated API |
|----------------|-----------|---------------|
| SenderReceiver | Required | `Rte_Read_SwcName_PortName_DE(data)` |
| SenderReceiver | Provided | `Rte_Write_SwcName_PortName_DE(data)` |
| SenderReceiver | Any | `Rte_Send_SwcName_PortName(data)` / `Rte_Receive_SwcName_PortName(data)` |
| NvBlock | Any | `Rte_Read_SwcName_PortName_DE(data)` / `Rte_Write_SwcName_PortName_DE(data)` |
| ClientServer | Required | `Rte_Call_SwcName_PortName_Operation(args...)` |
| ClientServer | Provided | `Rte_Server_SwcName_PortName_Operation(args...)` + `Rte_Result_SwcName_PortName_Operation(args...)` |
| ModeSwitch | Any | `Rte_Switch_SwcName_PortName_ModeGroup(mode)` / `Rte_Mode_SwcName_PortName_ModeGroup(mode)` |

## Output Files

For each SWC defined in the configuration, the tool generates:

- `Rte_<SwcName>.h` - Public RTE interface header with function prototypes and convenience macros
- `Rte_<SwcName>.c` - Implementation file with static buffers and function bodies

## Integration with Existing RTE Layer

The generated files include:

- `Rte.h` - Core RTE API
- `Rte_Type.h` - Common RTE types
- `Com.h` - COM services (for SenderReceiver)
- `NvM.h` - NVM services (for NvBlock)

Generated code uses `MemMap.h` sections for memory placement and follows the project's coding standards.

## Example Configuration

See `example_config.json` for a complete example with two SWCs:

- **SwcEngineCtrl** - Engine control with sender ports, NV block, and diagnostic client
- **SwcDisplay** - Dashboard display with receiver ports and display service server
