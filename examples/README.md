# AUTOSAR DDS Integration - Vehicle Application Examples

Complete set of automotive DDS application examples demonstrating various vehicle domains with appropriate safety levels and E2E protection.

## Examples Overview

| Example | Domain | Safety Level | E2E Profile | Domain ID |
|---------|--------|--------------|-------------|-----------|
| **ADAS Perception** | Advanced Driver Assistance | ASIL-D | P07 (CRC32+Counter) | 1 |
| **Powertrain Control** | EV Powertrain | ASIL-D | P07 (CRC32+Counter) | 2 |
| **Body Electronics** | Comfort & Convenience | ASIL-B | P05 (CRC16) | 3 |
| **Infotainment** | Entertainment & Info | QM (None) | None | 4 |
| **Diagnostics** | Vehicle Diagnostics | ASIL-B | P06 (CRC16+Counter) | 5 |

## Directory Structure

```
examples/
├── adas_perception/          # ADAS Perception System
│   ├── lidar_publisher.c
│   ├── camera_publisher.c
│   ├── sensor_fusion.c
│   ├── object_detector.c
│   ├── adas_types.h
│   ├── CMakeLists.txt
│   ├── adas_config.yaml
│   ├── e2e_config.yaml
│   ├── README.md
│   └── run_adas_demo.sh
├── powertrain_control/       # Powertrain Control System
│   ├── motor_publisher.c
│   ├── bms_publisher.c
│   ├── vcu_publisher.c
│   ├── regen_manager.c
│   ├── powertrain_types.h
│   ├── CMakeLists.txt
│   ├── powertrain_config.yaml
│   ├── e2e_config.yaml
│   ├── README.md
│   └── run_powertrain_demo.sh
├── body_electronics/         # Body Electronics System
│   ├── seat_controller.c
│   ├── hvac_controller.c
│   ├── door_controller.c
│   ├── lighting_controller.c
│   ├── body_types.h
│   ├── CMakeLists.txt
│   ├── body_config.yaml
│   ├── e2e_config.yaml
│   ├── README.md
│   └── run_body_demo.sh
├── infotainment/             # Infotainment System
│   ├── hud_publisher.c
│   ├── navigation_publisher.c
│   ├── media_controller.c
│   ├── infotainment_types.h
│   ├── CMakeLists.txt
│   ├── infotainment_config.yaml
│   ├── README.md
│   └── run_infotainment_demo.sh
├── diagnostics/              # Diagnostics System
│   ├── dtc_manager.c
│   ├── routine_controller.c
│   ├── diagnostics_types.h
│   ├── CMakeLists.txt
│   ├── diagnostics_config.yaml
│   ├── e2e_config.yaml
│   ├── README.md
│   └── run_diagnostics_demo.sh
├── README.md                 # This file
└── run_all_examples.sh       # Master script
```

## Quick Start

### Build All Examples

```bash
cd /home/admin/eth-dds-integration/examples

# Build ADAS
mkdir -p adas_perception/build && cd adas_perception/build
cmake .. && make
cd ../..

# Build Powertrain
mkdir -p powertrain_control/build && cd powertrain_control/build
cmake .. && make
cd ../..

# Build Body Electronics
mkdir -p body_electronics/build && cd body_electronics/build
cmake .. && make
cd ../..

# Build Infotainment
mkdir -p infotainment/build && cd infotainment/build
cmake .. && make
cd ../..

# Build Diagnostics
mkdir -p diagnostics/build && cd diagnostics/build
cmake .. && make
cd ../..
```

### Run Individual Examples

```bash
# Run ADAS demo
cd adas_perception && ./run_adas_demo.sh

# Run Powertrain demo
cd powertrain_control && ./run_powertrain_demo.sh

# Run Body Electronics demo
cd body_electronics && ./run_body_demo.sh

# Run Infotainment demo
cd infotainment && ./run_infotainment_demo.sh

# Run Diagnostics demo
cd diagnostics && ./run_diagnostics_demo.sh
```

### Run All Examples

```bash
./run_all_examples.sh
```

## DDS Domain Separation

Each example uses a separate DDS domain to prevent interference:

| Domain | Purpose |
|--------|---------|
| 1 | ADAS Perception (ASIL-D) |
| 2 | Powertrain Control (ASIL-D) |
| 3 | Body Electronics (ASIL-B) |
| 4 | Infotainment (QM) |
| 5 | Diagnostics (ASIL-B) |

## E2E Protection Matrix

| Profile | CRC | Counter | Use Case |
|---------|-----|---------|----------|
| P05 | CRC16 | None | Body Electronics |
| P06 | CRC16 | 4-bit | Diagnostics |
| P07 | CRC32 | 8-bit | ASIL-D Critical Data |

## Topic Naming Convention

All topics follow the naming convention: `Domain/Subsystem/Function`

Examples:
- `Adas/LiDAR/PointCloud`
- `Powertrain/Motor/Status`
- `Body/HVAC/Status`
- `Infotainment/HUD/Display`
- `Diagnostics/DTC/Status`

## Integration with AUTOSAR RTE

All examples can be integrated with AUTOSAR Classic RTE:

```c
// RTE Interface Example
#include "Rte_DDS.h"

// Read from DDS topic via RTE
Std_ReturnType status = Rte_Read_RPort_LidarPointCloud(&lidarData);

// Write to DDS topic via RTE
Rte_Write_PPort_MotorStatus(&motorStatus);
```

## Monitoring

Use the DDS Monitor tool to observe topic traffic:

```bash
# Monitor ADAS topics
./dds_monitor_cli --domain 1 --topic "Adas/#"

# Monitor all Powertrain topics
./dds_monitor_cli --domain 2 --topic "Powertrain/#"

# Monitor specific topic latency
./dds_monitor_cli --domain 1 --topic "Adas/Fusion/Result" --measure-latency
```

## License

Part of AUTOSAR DDS Integration Project
Copyright (c) 2024
