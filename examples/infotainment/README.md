# Infotainment System - DDS Examples

Non-safety critical infotainment system examples using DDS communication.

## Overview

This example demonstrates vehicle infotainment systems:

1. **HUD Display** - Head-up display overlay data
2. **Navigation** - Route guidance and traffic
3. **Media Controller** - Audio/entertainment control

## Safety Level

- **QM** (Quality Managed) - Non-safety critical
- No E2E protection required
- BEST_EFFORT reliability for lower latency

## Topics

| Topic | Type | Rate | QoS |
|-------|------|------|-----|
| Infotainment/HUD/Display | HudDisplayType | 30 Hz | Best-Effort |
| Infotainment/Navigation/Status | NavigationStatusType | 5 Hz | Best-Effort |
| Infotainment/Media/Status | MediaStatusType | 10 Hz | Best-Effort |

## Build & Run

```bash
mkdir build && cd build
cmake ..
make
./run_infotainment_demo.sh
```

## Features

### HUD
- Speed display
- Navigation overlay
- ADAS warnings
- Lane guidance

### Navigation
- Real-time traffic
- Lane-level guidance
- Speed limit warnings
- Alternate routes

### Media
- Multiple audio sources
- Bluetooth streaming
- CarPlay/Android Auto
- Premium audio (12 speakers)

## Configuration

- `infotainment_config.yaml` - System configuration

## License

Part of AUTOSAR DDS Integration Project
