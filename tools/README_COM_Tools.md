# COM Configuration Tools

This directory contains tools for generating and validating AUTOSAR COM module configurations.

## Tools Overview

### 1. com_config_generator.py

Generates C configuration code from JSON configuration files.

**Features:**
- Parse JSON configuration files
- Generate Com_Cfg.h (pre-compile configuration)
- Generate Com_Lcfg.c (link-time configuration)
- Built-in validation before generation
- Support for all AUTOSAR COM features:
  - Signals and Signal Groups
  - IPDUs with transmission modes
  - TMC (Transmission Mode Conditions)
  - Transmission confirmation
  - IPDU Groups

**Usage:**
```bash
# Basic generation
python tools/com_config_generator.py -i examples/com_config_engine.json -o config/com/

# With separate header output directory
python tools/com_config_generator.py -i examples/com_config_engine.json \
    -o config/com/ \
    --header-output include/autosar/classic/com/

# Validation only (no generation)
python tools/com_config_generator.py -i examples/com_config_engine.json \
    -o /tmp --validate-only
```

### 2. com_config_validator.py

Validates COM configuration files for correctness and consistency.

**Features:**
- Structure validation (required fields, types)
- ID uniqueness checks
- Reference integrity validation
- Signal layout validation (no overlaps)
- Transmission mode validation
- TMC configuration validation
- Value range checks

**Usage:**
```bash
# Basic validation
python tools/com_config_validator.py -i examples/com_config_engine.json

# Verbose output (show info messages)
python tools/com_config_validator.py -i examples/com_config_engine.json -v

# Strict mode (warnings treated as errors)
python tools/com_config_validator.py -i examples/com_config_engine.json --strict
```

**Exit Codes:**
- 0: Validation passed
- 1: Validation failed (errors found)
- 2: Configuration error (invalid JSON, etc.)
- 3: File not found

## Configuration File Format

See `examples/com_config_engine.json` for a complete example.

### Basic Structure

```json
{
  "global": {
    "max_signals": 128,
    "max_ipdus": 64,
    "dev_error_detect": true
  },
  "signals": [...],
  "signal_groups": [...],
  "ipdus": [...],
  "ipdu_groups": [...]
}
```

### Signal Configuration

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
  "ipdu_id": 0
}
```

### IPDU Configuration

```json
{
  "name": "EngineData",
  "id": 0,
  "length": 8,
  "direction": "COM_SEND",
  "signal_refs": [0, 1, 2],
  "tx_mode_false": {
    "mode": "COM_MODE_PERIODIC",
    "cycle_time_ms": 100
  },
  "use_tmc": true,
  "tmc": {
    "signal_id": 0,
    "threshold_value": 3000,
    "use_greater_than": true
  }
}
```

## Workflow

```
1. Create/edit JSON configuration
          |
          v
2. Validate: python tools/com_config_validator.py -i config.json
          |
          v
3. Generate: python tools/com_config_generator.py -i config.json -o output/
          |
          v
4. Build and test
```

## Examples

### Engine Data Configuration

The `examples/com_config_engine.json` file contains a complete example with:

- 19 signals (engine, chassis, body)
- 4 IPDUs:
  - `EngineData`: 100ms periodic with TMC (switches to 50ms when RPM > 3000)
  - `EngineStatus`: 500ms periodic
  - `VehicleSpeed`: Event-triggered direct mode
  - `BodyControl`: Mixed mode with TMC
- 3 IPDU Groups: EngineGroup, ChassisGroup, BodyGroup
- 3 Signal Groups: EngineCoreInfo, EngineDiagnostics, VehicleDynamics

### Generating Engine Example

```bash
# Validate the example
python tools/com_config_validator.py -i examples/com_config_engine.json -v

# Generate C code
python tools/com_config_generator.py \
    -i examples/com_config_engine.json \
    -o config/com/ \
    --header-output include/autosar/classic/com/
```

## Signal Types

| Type | Description | Size |
|------|-------------|------|
| COM_BOOLEAN | Boolean | 1 bit |
| COM_UINT8 | Unsigned 8-bit | 8 bits |
| COM_UINT16 | Unsigned 16-bit | 16 bits |
| COM_UINT32 | Unsigned 32-bit | 32 bits |
| COM_SINT8 | Signed 8-bit | 8 bits |
| COM_SINT16 | Signed 16-bit | 16 bits |
| COM_SINT32 | Signed 32-bit | 32 bits |
| COM_FLOAT32 | IEEE 754 float | 32 bits |
| COM_FLOAT64 | IEEE 754 double | 64 bits |

## Transfer Properties

| Property | Description |
|----------|-------------|
| COM_PENDING | Transmission triggered by IPDU transmission |
| COM_TRIGGERED | Signal triggers IPDU transmission |
| COM_TRIGGERED_ON_CHANGE | Signal triggers IPDU only when value changes |
| COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION | Trigger on change, no repetitions |
| COM_TRIGGERED_WITHOUT_REPETITION | Trigger always, no repetitions |

## Transmission Modes

| Mode | Description |
|------|-------------|
| COM_MODE_NONE | No automatic transmission |
| COM_MODE_DIRECT | Event-triggered transmission |
| COM_MODE_PERIODIC | Time-based periodic transmission |
| COM_MODE_MIXED | Periodic + event-triggered |

## Documentation

See `docs/com_configuration_guide.md` for comprehensive documentation on:
- Configuration architecture
- Signal configuration
- IPDU configuration
- Transmission modes
- TMC (Transmission Mode Conditions)
- Best practices
- API usage examples

## Troubleshooting

### Common Issues

**Signal overlap error:**
```
[ERROR] IPDU 0: Signal overlap: 'SignalA' ends at bit 15, 'SignalB' starts at bit 10
```
Solution: Adjust `bit_position` or `bit_size` so signals don't overlap.

**Unknown reference error:**
```
[ERROR] Signal 'MySignal' references unknown IPDU ID: 99
```
Solution: Ensure all `ipdu_id` references exist in the `ipdus` array.

**Duplicate ID error:**
```
[ERROR] Duplicate signal ID 5 used by: SignalA, SignalB
```
Solution: Use unique IDs for each signal.

## Dependencies

- Python 3.7+
- No external packages required (uses only standard library)

## License

Part of YuleTech AutoSAR Platform
