# AUTOSAR COM Module Configuration Guide

## Table of Contents
1. [Overview](#overview)
2. [Configuration Architecture](#configuration-architecture)
3. [Configuration Files](#configuration-files)
4. [Signal Configuration](#signal-configuration)
5. [IPDU Configuration](#ipdu-configuration)
6. [Transmission Modes](#transmission-modes)
7. [TMC - Transmission Mode Conditions](#tmc-transmission-mode-conditions)
8. [Engine Data Example](#engine-data-example)
9. [Configuration Tools](#configuration-tools)
10. [Validation](#validation)
11. [Best Practices](#best-practices)

---

## Overview

The AUTOSAR COM (Communication) module provides standardized communication services for signal-based data exchange between ECUs. This guide covers the configuration of the COM module for the Classic AUTOSAR platform.

### Key Concepts

- **Signal**: The smallest unit of data transfer, representing a single value (e.g., engine speed)
- **Signal Group**: A collection of related signals that must be sent/received atomically
- **IPDU (Interaction Layer Protocol Data Unit)**: A container for signals that are transmitted together
- **IPDU Group**: A collection of IPDUs that can be started/stopped together

---

## Configuration Architecture

### Configuration Files

```
eth-dds-integration/
├── include/autosar/classic/com/
│   ├── Com.h           # Main COM module header
│   ├── Com_Cfg.h       # Pre-compile configuration (generated)
│   └── Com_Types.h     # Type definitions
├── config/com/
│   └── Com_Lcfg.c      # Link-time configuration (generated)
├── examples/
│   └── com_config_engine.json  # Example configuration
└── tools/
    ├── com_config_generator.py  # Configuration generator
    └── com_config_validator.py  # Configuration validator
```

### Configuration Types

1. **Pre-compile Configuration (Com_Cfg.h)**: Compile-time constants
2. **Link-time Configuration (Com_Lcfg.c)**: Data structures initialized at link time
3. **Post-build Configuration**: Can be modified after build (not covered in this guide)

---

## Configuration Files

### Com_Cfg.h

Contains compile-time configuration parameters:

```c
/* Development Error Detection */
#define COM_DEV_ERROR_DETECT                STD_ON

/* Version Info API */
#define COM_VERSION_INFO_API                STD_ON

/* Enable Signal Group Array API */
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   STD_ON

/* Maximum number of elements */
#define COM_MAX_SIGNALS                     128u
#define COM_MAX_SIGNAL_GROUPS               32u
#define COM_MAX_IPDUS                       64u
#define COM_MAX_IPDU_GROUPS                 16u
```

### Com_Lcfg.c

Contains the actual configuration data structures:

```c
const Com_ConfigType ComConfig = {
    .Signals = ComSignals,
    .NumSignals = 19,
    .SignalGroups = ComSignalGroups,
    .NumSignalGroups = 3,
    .IPdus = ComIPdus,
    .NumIPdus = 4,
    .IPduGroups = ComIPduGroups,
    .NumIPduGroups = 3
};
```

---

## Signal Configuration

### Signal Attributes

| Attribute | Type | Description | Example |
|-----------|------|-------------|---------|
| `name` | string | Signal identifier | "EngineSpeed" |
| `id` | integer | Unique signal ID | 0 |
| `bit_position` | integer | Start bit in IPDU | 0 |
| `bit_size` | integer | Size in bits | 16 |
| `type` | enum | Signal data type | "COM_UINT16" |
| `endianness` | enum | Byte order | "COM_LITTLE_ENDIAN" |
| `transfer_property` | enum | When to transmit | "COM_TRIGGERED_ON_CHANGE" |
| `init_value` | varies | Initial value | 0 |
| `ipdu_id` | integer | Parent IPDU | 0 |

### Signal Types

| Type | Description | Range |
|------|-------------|-------|
| COM_BOOLEAN | Boolean value | 0 or 1 |
| COM_UINT8 | Unsigned 8-bit | 0 to 255 |
| COM_UINT16 | Unsigned 16-bit | 0 to 65535 |
| COM_UINT32 | Unsigned 32-bit | 0 to 4294967295 |
| COM_SINT8 | Signed 8-bit | -128 to 127 |
| COM_SINT16 | Signed 16-bit | -32768 to 32767 |
| COM_SINT32 | Signed 32-bit | -2147483648 to 2147483647 |
| COM_FLOAT32 | 32-bit float | IEEE 754 |
| COM_FLOAT64 | 64-bit float | IEEE 754 |
| COM_UINT8_N | Byte array | N bytes |

### Transfer Properties

| Property | Description |
|----------|-------------|
| COM_PENDING | Transmission triggered by IPDU transmission |
| COM_TRIGGERED | Signal triggers IPDU transmission |
| COM_TRIGGERED_ON_CHANGE | Signal triggers IPDU only when value changes |
| COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION | Trigger on change, no repetitions |
| COM_TRIGGERED_WITHOUT_REPETITION | Trigger always, no repetitions |

### Example Signal Configuration

```json
{
  "name": "EngineSpeed",
  "id": 0,
  "bit_position": 0,
  "bit_size": 16,
  "type": "COM_UINT16",
  "endianness": "COM_LITTLE_ENDIAN",
  "transfer_property": "COM_TRIGGERED_ON_CHANGE",
  "init_value": 0,
  "ipdu_id": 0,
  "description": "Engine Speed: 0-8000 RPM, resolution 0.125 RPM/bit"
}
```

---

## IPDU Configuration

### IPDU Attributes

| Attribute | Type | Description | Example |
|-----------|------|-------------|---------|
| `name` | string | IPDU identifier | "EngineData" |
| `id` | integer | Unique IPDU ID | 0 |
| `length` | integer | PDU size in bytes | 8 |
| `direction` | enum | Send or Receive | "COM_SEND" |
| `type` | enum | Normal or TP | "COM_NORMAL" |
| `signal_processing` | enum | Immediate or Deferred | "COM_IMMEDIATE" |
| `signal_refs` | array | Signal IDs in this PDU | [0, 1, 2] |

### IPDU Types

| Type | Description |
|------|-------------|
| COM_NORMAL | Regular IPDU (up to 8 bytes) |
| COM_TP | Transport Protocol IPDU (large PDUs) |

### Signal Processing Modes

| Mode | Description |
|------|-------------|
| COM_IMMEDIATE | Signal processing in Com_RxIndication context |
| COM_DEFERRED | Signal processing in Com_MainFunctionRx context |

### Example IPDU Configuration

```json
{
  "name": "EngineData",
  "id": 0,
  "length": 8,
  "direction": "COM_SEND",
  "type": "COM_NORMAL",
  "signal_processing": "COM_IMMEDIATE",
  "signal_refs": [0, 1, 2, 3, 4, 5],
  "signal_group_refs": [0],
  "ipdu_group_refs": [0]
}
```

---

## Transmission Modes

The COM module supports four transmission modes:

### COM_MODE_NONE

No periodic or direct transmission. PDU is only sent when explicitly triggered.

```json
{
  "mode": "COM_MODE_NONE",
  "cycle_time_ms": 0,
  "repetition_period_ms": 0,
  "num_repetitions": 0
}
```

### COM_MODE_DIRECT

Event-triggered transmission. Sent when a signal with COM_TRIGGERED property changes.

```json
{
  "mode": "COM_MODE_DIRECT",
  "cycle_time_ms": 0,
  "repetition_period_ms": 20,
  "num_repetitions": 1,
  "repeating_enabled": true
}
```

### COM_MODE_PERIODIC

Time-based transmission at fixed intervals.

```json
{
  "mode": "COM_MODE_PERIODIC",
  "cycle_time_ms": 100,
  "repetition_period_ms": 0,
  "num_repetitions": 0
}
```

### COM_MODE_MIXED

Combines periodic and event-triggered transmission.

```json
{
  "mode": "COM_MODE_MIXED",
  "cycle_time_ms": 500,
  "repetition_period_ms": 20,
  "num_repetitions": 3,
  "time_offset_ms": 100,
  "repeating_enabled": true
}
```

---

## TMC - Transmission Mode Conditions

TMC allows dynamic switching between transmission modes based on signal values.

### How It Works

1. **TxModeFalse**: Default transmission mode
2. **TxModeTrue**: Alternative mode when condition is met
3. **TMC**: Condition that triggers mode switch

### TMC Configuration

```json
{
  "use_tmc": true,
  "tmc": {
    "signal_id": 0,
    "threshold_value": 3000,
    "use_greater_than": true,
    "is_configured": true
  },
  "tx_mode_false": {
    "mode": "COM_MODE_PERIODIC",
    "cycle_time_ms": 100
  },
  "tx_mode_true": {
    "mode": "COM_MODE_MIXED",
    "cycle_time_ms": 50,
    "repetition_period_ms": 10,
    "num_repetitions": 2
  }
}
```

### Use Case: Engine Speed

- **Normal operation**: EngineData sent every 100ms (TxModeFalse)
- **High RPM (>3000)**: Switch to fast mode (TxModeTrue) with 50ms period + repetitions
- **Result**: Faster updates when engine is under load

---

## Engine Data Example

### Signal Layout

```
EngineData IPDU (8 bytes)
+----------------+----------------+----------------+----------------+
| Byte 0-1       | Byte 2         | Byte 3         | Byte 4-5       |
| EngineSpeed    | CoolantTemp    | ThrottlePos    | EngineTorque   |
| (16 bits)      | (8 bits)       | (8 bits)       | (16 bits)      |
+----------------+----------------+----------------+----------------+
| Byte 6         | Byte 7         |
| EngineState    | BatteryVoltage |
| (8 bits)       | (8 bits)       |
+----------------+----------------+
```

### Complete Configuration

See `examples/com_config_engine.json` for the complete example configuration.

### Signals Included

| Signal | Description | Range |
|--------|-------------|-------|
| EngineSpeed | Engine RPM | 0-8000 |
| CoolantTemp | Coolant temperature | -40°C to 215°C |
| ThrottlePosition | Throttle percentage | 0-100% |
| EngineTorque | Engine torque | 0-8000 Nm |
| EngineState | Engine state | Stopped/Starting/Running/Stopping |
| BatteryVoltage | Battery voltage | 0-25.5V |
| OilPressure | Oil pressure | 0-12.75 bar |
| OilTemp | Oil temperature | -40°C to 215°C |
| FuelLevel | Fuel level | 0-100% |
| VehicleSpeed | Vehicle speed | 0-300 km/h |
| WheelSpeed_FL | Front left wheel speed | 0-300 km/h |
| WheelSpeed_FR | Front right wheel speed | 0-300 km/h |
| GearPosition | Selected gear | P/R/N/D/1-7 |
| ParkingBrake | Parking brake status | Engaged/Released |
| TurnSignalLeft | Left turn signal | On/Off |
| TurnSignalRight | Right turn signal | On/Off |
| Headlights | Headlights status | On/Off |

---

## Configuration Tools

### com_config_generator.py

Generates C configuration code from JSON.

#### Usage

```bash
# Basic generation
python tools/com_config_generator.py -i examples/com_config_engine.json -o config/com/

# With separate header output
python tools/com_config_generator.py -i examples/com_config_engine.json \
    -o config/com/ \
    --header-output include/autosar/classic/com/

# Validation only
python tools/com_config_generator.py -i examples/com_config_engine.json -o /tmp --validate-only
```

#### Generated Files

1. **Com_Cfg.h**: Pre-compile configuration header
   - Symbolic names (IDs)
   - Compile-time constants
   - Feature switches

2. **Com_Lcfg.c**: Link-time configuration
   - Signal configurations
   - IPDU configurations
   - Buffer declarations
   - Transmission modes

### com_config_validator.py

Validates configuration files for correctness.

#### Usage

```bash
# Basic validation
python tools/com_config_validator.py -i examples/com_config_engine.json

# Verbose output
python tools/com_config_validator.py -i examples/com_config_engine.json -v

# Strict mode (treat warnings as errors)
python tools/com_config_validator.py -i examples/com_config_engine.json --strict
```

#### Validation Checks

- **Structure Validation**: Required fields, valid types
- **ID Uniqueness**: No duplicate IDs
- **Reference Integrity**: All references resolve
- **Signal Layout**: No overlapping signals
- **Transmission Modes**: Valid mode combinations
- **TMC Configuration**: Valid TMC settings
- **Value Ranges**: Reasonable parameter values

---

## Validation

### Validation Workflow

```
1. Create/Edit JSON configuration
         |
         v
2. Run com_config_validator.py
         |
    +----+----+
    |         |
    v         v
Valid     Invalid
    |         |
    v         v
3. Generate   Fix errors
    |         |
    v         |
4. Build      |
    |         |
    +---------+
```

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| Duplicate signal ID | Same ID used twice | Use unique IDs |
| Signal overlap | Bit positions overlap | Adjust bit_position/bit_size |
| Unknown IPDU reference | Signal references non-existent IPDU | Check ipdu_id |
| Invalid signal type | Type not in valid list | Use valid COM_* type |
| TMC without TxModeTrue | use_tmc=true but TxModeTrue=NONE | Configure TxModeTrue |

---

## Best Practices

### Signal Design

1. **Group Related Signals**: Put related signals in the same IPDU
2. **Update Rate Matching**: Signals with similar update rates should share an IPDU
3. **Byte Alignment**: Align signals to byte boundaries when possible
4. **Reserved Bits**: Leave unused bits for future expansion

### Transmission Mode Selection

| Use Case | Recommended Mode | Example |
|----------|-----------------|---------|
| Fast-changing values | PERIODIC (fast) | Engine speed |
| Slow-changing values | PERIODIC (slow) or MIXED | Coolant temp |
| Event-driven | DIRECT | Gear change |
| Safety-critical | MIXED with repetitions | Brake status |

### TMC Usage

1. **Use for Mode Switching**: Switch between normal and fast modes
2. **Keep Thresholds Simple**: Avoid complex conditions
3. **Test Edge Cases**: Verify behavior at threshold values

### Configuration Management

1. **Version Control**: Track JSON configurations in version control
2. **Naming Conventions**: Use consistent, descriptive names
3. **Documentation**: Add descriptions to all signals and IPDUs
4. **Validation**: Always validate before generating code

### Performance Considerations

1. **Buffer Sizes**: Keep IPDU sizes reasonable (8 bytes for CAN)
2. **Cycle Times**: Don't use excessively fast cycle times
3. **Signal Count**: Limit signals per IPDU for efficient processing
4. **Deferred Processing**: Use COM_DEFERRED for complex signal processing

---

## API Usage Examples

### Sending a Signal

```c
#include "Com.h"

void SendEngineSpeed(uint16 rpm)
{
    uint16 engineSpeed = rpm * 8;  // Apply scaling factor
    Com_SendSignal(ComConf_ComSignal_EngineSpeed, &engineSpeed);
}
```

### Receiving a Signal

```c
void ReceiveEngineData(void)
{
    uint16 engineSpeed;
    uint8 coolantTemp;
    
    Com_ReceiveSignal(ComConf_ComSignal_EngineSpeed, &engineSpeed);
    Com_ReceiveSignal(ComConf_ComSignal_CoolantTemp, &coolantTemp);
    
    // Process data...
    engineSpeed = engineSpeed / 8;  // Remove scaling
}
```

### IPDU Group Control

```c
void EnableEngineCommunication(void)
{
    Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);
}

void DisableEngineCommunication(void)
{
    Com_IpduGroupStop(ComConf_ComIPduGroup_EngineGroup);
}
```

### Signal Group (Atomic Access)

```c
void UpdateEngineInfo(uint16 speed, uint8 temp)
{
    // Update shadow buffer
    Com_SendSignalGroup(ComConf_ComSignalGroup_EngineCoreInfo);
    
    // Trigger transmission
    Com_SendSignal(ComConf_ComSignal_EngineSpeed, &speed);
    Com_SendSignal(ComConf_ComSignal_CoolantTemp, &temp);
    
    // Send all signals together
    Com_SendSignalGroup(ComConf_ComSignalGroup_EngineCoreInfo);
}
```

---

## References

- AUTOSAR SWS COM 4.4.0
- AUTOSAR COM Specification
- ISO 11898 (CAN Protocol)

## Support

For issues and questions:
- Check the validation output
- Review the examples in `examples/com_config_engine.json`
- Consult the AUTOSAR specification

---

*Generated for YuleTech AutoSAR Platform*
