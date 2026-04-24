# ADAS Perception System - DDS Examples

AUTOSAR-compliant ADAS (Advanced Driver Assistance Systems) perception system examples using DDS communication with E2E protection.

## Overview

This example demonstrates a complete ADAS perception pipeline with the following components:

1. **LiDAR Point Cloud Publisher** - Simulates LiDAR sensor data
2. **Camera Image Publisher** - Simulates camera sensor data  
3. **Sensor Fusion Module** - Fuses data from multiple sensors
4. **Object Detector** - Publishes detected objects

## Safety Level

- **ASIL-D** for critical perception data
- E2E Profile 7 (CRC32 + Counter) for safety-critical topics

## Architecture

```
┌─────────────────┐      ┌─────────────────┐
│   LiDAR Publisher│      │ Camera Publisher │
│   (10 Hz)        │      │   (30 Hz)        │
└────────┬────────┘      └────────┬────────┘
         │                        │
         │ DDS Topics             │
         ▼                        ▼
┌───────────────────────────────────────┐
│         Sensor Fusion Module          │
│      (20 Hz, E2E Protected)           │
└───────────────────┬───────────────────┘
                    │
                    ▼
         ┌────────────────────┐
         │   Object Detector  │
         │   (20 Hz)          │
         └────────────────────┘
```

## Topics

| Topic | Type | Rate | E2E | Safety |
|-------|------|------|-----|--------|
| Adas/LiDAR/PointCloud | AdasLidarPointCloudType | 10 Hz | P07 | ASIL-D |
| Adas/Camera/Image | AdasCameraImageType | 30 Hz | P07 | ASIL-D |
| Adas/Fusion/Result | AdasFusionResultType | 20 Hz | P07 | ASIL-D |
| Adas/Objects/Detected | AdasObjectListType | 20 Hz | P07 | ASIL-D |

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Examples

### Option 1: Run individually

Terminal 1 - Start LiDAR Publisher:
```bash
./bin/lidar_publisher
```

Terminal 2 - Start Camera Publisher:
```bash
./bin/camera_publisher
```

Terminal 3 - Start Sensor Fusion:
```bash
./bin/sensor_fusion
```

### Option 2: Use the provided script

```bash
chmod +x run_adas_demo.sh
./run_adas_demo.sh
```

## DDS QoS Configuration

All critical topics use:
- **Reliability**: RELIABLE
- **Durability**: VOLATILE
- **Deadline**: 50-100ms depending on topic
- **Latency Budget**: 5-10ms

## E2E Protection

E2E Profile 7 provides:
- CRC32 checksum for data integrity
- 8-bit sequence counter for freshness
- Maximum allowed counter jump: 2
- CRC offset: 0, Counter offset: 4

## Data Types

See `adas_types.h` for complete type definitions including:
- 3D point clouds with intensity
- Camera images with metadata
- Detected objects with tracking
- Lane detection data
- Sensor fusion results

## Configuration Files

- `adas_config.yaml` - System and sensor configuration
- `e2e_config.yaml` - E2E protection profiles and settings

## Monitoring

View E2E statistics:
```bash
# Check E2E health
cat /var/log/adas_perception.log

# Monitor DDS topics
./dds_monitor_cli --domain 1 --topic "Adas/Fusion/Result"
```

## Integration with AUTOSAR RTE

The example can be integrated with AUTOSAR Classic RTE:

```c
// RTE Interface
Rte_Write_PPort_LiDAR_PointCloud(&lidarData);
Rte_Write_PPort_Camera_Image(&cameraData);
Rte_Read_RPort_Fusion_Result(&fusionResult);
```

## Safety Considerations

1. All safety-critical data uses E2E protection
2. Sensor data validation before fusion
3. Timeout monitoring for all topics
4. Degradation handling for sensor failures
5. Heartbeat monitoring for all nodes

## Troubleshooting

| Issue | Solution |
|-------|----------|
| E2E errors | Check E2E profile configuration |
| High latency | Verify network QoS settings |
| Data timeout | Check publisher heartbeat |
| CRC errors | Verify data alignment |

## License

Part of AUTOSAR DDS Integration Project
