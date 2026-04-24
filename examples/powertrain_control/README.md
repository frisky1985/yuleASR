# Powertrain Control System - DDS Examples

AUTOSAR-compliant powertrain control system examples using DDS communication with E2E protection for electric vehicles.

## Overview

This example demonstrates a complete electric vehicle powertrain control system with the following components:

1. **Motor Status Publisher** - Publishes motor speed, torque, and temperature
2. **BMS Publisher** - Publishes battery SOC, voltage, and cell data
3. **VCU Publisher** - Publishes vehicle speed, gear, and driver inputs
4. **Regeneration Manager** - Manages energy recovery during braking

## Safety Level

- **ASIL-D** for all critical powertrain data
- E2E Profile 7 (CRC32 + Counter) for all topics
- Command validation and torque limiting

## Architecture

```
┌───────────────────────────────────────────────────────────────────┐
│              Powertrain Control Domain (Domain ID: 2)                    │
├───────────────────────────────────────────────────────────────────┤
│                                                                         │
│   ┌─────────────┐      ┌───────────────┐      ┌───────────────┐      ┌───────────────┐   │
│   │    Motor      │      │      BMS       │      │      VCU       │      │    Regen      │   │
│   │  (100 Hz)     │      │    (10 Hz)     │      │    (50 Hz)     │      │   (50 Hz)     │   │
│   └────────┬────────┘      └────────┬───────────┘      └────────┬───────────┘      └────────┬───────────┘   │
│          │                     │                     │                     │   │
│          │                     │                     │                     │   │
│          ▼                     ▼                     ▼                     ▼   │
│   ┌───────────────────────────────────────────────────────────────────┐   │
│   │              Powertrain/Motor/Status (E2E P07)                       │   │
│   │              Powertrain/BMS/Status (E2E P07)                         │   │
│   │              Powertrain/VCU/Status (E2E P07)                         │   │
│   │              Powertrain/Regen/Status (E2E P07)                       │   │
│   └───────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└───────────────────────────────────────────────────────────────────┘
```

## Topics

| Topic | Type | Rate | E2E | Safety |
|-------|------|------|-----|--------|
| Powertrain/Motor/Status | MotorSystemStatusType | 100 Hz | P07 | ASIL-D |
| Powertrain/BMS/Status | BmsStatusType | 10 Hz | P07 | ASIL-D |
| Powertrain/VCU/Status | VcuStatusType | 50 Hz | P07 | ASIL-D |
| Powertrain/Regen/Status | RegenStatusType | 50 Hz | P07 | ASIL-D |
| Powertrain/Command | PowertrainCommandType | 100 Hz | P07 | ASIL-D |

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Examples

### Option 1: Run individually

Terminal 1 - Start Motor Publisher:
```bash
./bin/motor_publisher
```

Terminal 2 - Start BMS Publisher:
```bash
./bin/bms_publisher
```

Terminal 3 - Start VCU Publisher:
```bash
./bin/vcu_publisher
```

Terminal 4 - Start Regeneration Manager:
```bash
./bin/regen_manager
```

### Option 2: Use the provided script

```bash
chmod +x run_powertrain_demo.sh
./run_powertrain_demo.sh
```

## DDS QoS Configuration

All powertrain topics use:
- **Reliability**: RELIABLE
- **Durability**: VOLATILE
- **Deadline**: 15-150ms depending on topic
- **Latency Budget**: 2-10ms

## E2E Protection

E2E Profile 7 provides:
- CRC32 checksum for data integrity
- 8-bit sequence counter for freshness
- Maximum allowed counter jump: 2
- CRC offset: 0, Counter offset: 4

## Data Types

See `powertrain_types.h` for complete type definitions including:
- Motor electrical and mechanical parameters
- Battery cell-level monitoring
- Vehicle dynamics and driver inputs
- Energy recovery statistics

## Configuration Files

- `powertrain_config.yaml` - System and component configuration
- `e2e_config.yaml` - E2E protection profiles and settings

## Safety Features

1. **Command Validation**: All powertrain commands validated
2. **Torque Limiting**: Maximum torque limits enforced
3. **Thermal Management**: Temperature-based power derating
4. **Isolation Monitoring**: HV isolation fault detection
5. **E2E Protection**: All safety-critical data protected

## Monitoring

View powertrain status:
```bash
# Check system logs
cat /var/log/powertrain.log

# Monitor specific topics
./dds_monitor_cli --domain 2 --topic "Powertrain/BMS/Status"
```

## Integration with AUTOSAR RTE

The example can be integrated with AUTOSAR Classic RTE:

```c
// RTE Interface for BMS
Rte_Write_PPort_BMS_Status(&bmsStatus);
Rte_Read_RPort_VCU_Status(&vcuStatus);

// RTE Interface for Motor Control
Rte_Write_PPort_Motor_TorqueCommand(torqueValue);
Rte_Read_RPort_Motor_ActualTorque(&actualTorque);

// RTE Interface for VCU
Rte_Write_PPort_VCU_VehicleSpeed(vehicleSpeed);
Rte_Read_RPort_Driver_Inputs(&driverInputs);
```

## Safety Considerations

1. All ASIL-D data uses E2E protection
2. Command timeout handling
3. Safe state on communication loss
4. Torque validation and limiting
5. Thermal runaway protection

## Troubleshooting

| Issue | Solution |
|-------|----------|
| E2E errors | Check E2E profile configuration |
| High latency | Verify network QoS settings |
| Communication loss | Check DDS domain ID matching |
| CRC errors | Verify data alignment |

## License

Part of AUTOSAR DDS Integration Project
