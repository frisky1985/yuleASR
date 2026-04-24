# Body Electronics System - DDS Examples

AUTOSAR-compliant body electronics system examples using DDS communication with E2E protection.

## Overview

This example demonstrates vehicle body electronics control:

1. **Seat Controller** - Seat position, heating, and comfort settings
2. **HVAC Controller** - Climate control and air quality
3. **Door Controller** - Door locks and window control
4. **Lighting Controller** - Exterior and interior lighting

## Safety Level

- **ASIL-B** for body electronics data
- E2E Profile 5 (CRC16) for all topics

## Topics

| Topic | Type | Rate | E2E | Safety |
|-------|------|------|-----|--------|
| Body/Seat/Status | SeatSystemStatusType | 10 Hz | P05 | ASIL-B |
| Body/HVAC/Status | HvacStatusType | 5 Hz | P05 | ASIL-B |
| Body/Door/Status | DoorSystemStatusType | 10 Hz | P05 | ASIL-B |
| Body/Lighting/Status | LightingSystemStatusType | 20 Hz | P05 | ASIL-B |

## Build & Run

```bash
mkdir build && cd build
cmake ..
make
./run_body_demo.sh
```

## Features

### Seat Control
- Multi-zone heating/cooling
- Memory positions
- Occupancy detection
- Safety belt monitoring

### HVAC
- Dual-zone climate control
- Air quality monitoring
- CO2 sensing
- Automatic recirculation

### Door Management
- Central locking
- Keyless entry
- Power windows with anti-pinch
- Walk-away locking

### Lighting
- LED headlights with adaptive beam
- Ambient lighting (RGB)
- Welcome/Leaving home functions
- Automatic high beam

## Configuration

- `body_config.yaml` - System configuration
- `e2e_config.yaml` - E2E protection settings

## License

Part of AUTOSAR DDS Integration Project
