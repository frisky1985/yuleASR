# DDS Configuration Tools and Code Generator

## Overview

This document describes the enhanced configuration tools and code generator for the eth-dds-integration project.

## New Components

### 1. Code Generator (`tools/codegen/`)

A comprehensive code generation suite for DDS-based automotive systems:

| Module | Description | Lines of Code |
|--------|-------------|---------------|
| `dds_idl_parser.py` | OMG IDL 4.x parser with DDS annotation support | ~550 |
| `dds_type_generator.py` | C/C++ code generator for DDS types | ~550 |
| `autosar_arxml_parser.py` | AUTOSAR ARXML parser for SWC integration | ~750 |
| `rte_generator.py` | RTE interface generator for AUTOSAR-DDS | ~650 |
| `config_validator.py` | Configuration schema and design rule validator | ~600 |
| `dds_config_cli.py` | Unified CLI tool for all operations | ~550 |

**Features:**
- Parse IDL files and generate C++ type definitions
- Parse ARXML files and generate RTE code
- Validate configurations against schema
- Support multiple DDS implementations (FastDDS, CycloneDDS, RTI)
- Support multiple target OS (FreeRTOS, Linux, QNX)

### 2. Design-Time Tools (`tools/design/`)

Design validation and analysis tools:

| Module | Description |
|--------|-------------|
| `design_rule_checker.py` | Automotive design rule validation |
| `data_flow_analyzer.py` | Data flow graph analysis and cycle detection |
| `timing_calculator.py` | End-to-end latency calculation |

**Features:**
- Validate naming conventions (PascalCase, camelCase)
- Check safety-critical QoS requirements
- Detect data flow cycles and feedback loops
- Calculate WCET and validate deadlines
- Export to Graphviz DOT format

### 3. Configuration Templates (`tools/templates/`)

CMake-configurable header templates:

| Template | Purpose |
|----------|---------|
| `dds_config.h.in` | DDS middleware configuration |
| `os_cfg.h.in` | Operating system configuration |
| `rte_config.h.in` | RTE interface configuration |

### 4. Web GUI Enhancements (`tools/web_gui/web/js/`)

New JavaScript modules for the web interface:

| Module | Features |
|--------|----------|
| `topic_editor.js` | Visual topic configuration with QoS tabs |
| `domain_topology.js` | D3.js-based interactive topology diagram |
| `qos_wizard.js` | QoS configuration wizard with use case templates |
| `config_manager.js` | Import/export, validation, version comparison |

### 5. Example Configurations (`examples/config/`)

Production-ready configuration templates:

| Configuration | Domain | Safety Level | Use Case |
|--------------|--------|--------------|----------|
| `adas_perception.yaml` | ADAS | ASIL-D | Sensor fusion, path planning |
| `powertrain_control.yaml` | Powertrain | ASIL-D | Engine, transmission control |
| `vehicle_body.yaml` | Body Electronics | ASIL-A | Lighting, climate, locks |
| `infotainment.yaml` | Infotainment | QM | Media, navigation, HMI |

### 6. CMake Integration (`cmake/DDSCodeGen.cmake`)

CMake module for build-time code generation:

```cmake
# Generate code from configuration
dds_generate_code(
    CONFIG_FILE config.yaml
    OUTPUT_DIR generated/
    DDS_IMPL fastdds
    TARGET_OS freertos
)

# Validate configuration
dds_validate_config(CONFIG_FILE config.yaml)

# Setup full DDS with auto-regeneration
dds_setup_from_config(config.yaml)
```

## CLI Tool Usage

The `dds-config` CLI provides unified access to all features:

```bash
# Validate configuration
dds-config validate config.yaml --schema schema.yaml --strict

# Generate code
dds-config generate --input config.yaml --output generated/ \
    --type all --dds-impl fastdds --os freertos --safety ASIL_D

# Convert formats
dds-config convert config.yaml --to-json --output config.json
dds-config convert config.yaml --to-arxml --output system.arxml

# Monitor runtime
dds-config monitor --config config.yaml --interval 5 --metrics latency throughput

# Compare configurations
dds-config diff config_v1.yaml config_v2.yaml

# Create from template
dds-config template --type adas --output adas_config.yaml

# Parse IDL
dds-config parse-idl types.idl --generate-types --output generated/

# Parse ARXML
dds-config parse-arxml system.arxml --generate-rte --output generated_rte/
```

## Configuration Features

### Supported Configuration Sections

```yaml
version: "1.0.0"
system:
  name: "SystemName"
  description: "System description"
  version: "1.0.0"
  asil_level: "ASIL_D"

domains:
  - id: 0
    name: "Domain0"
    discovery:
      enabled: true
      protocol: UDPv4
    security:
      enabled: true
      authentication:
        plugin: "DDS:Auth:PKI-DH"

topics:
  - name: "TopicName"
    type: "DataType"
    domain_id: 0
    qos_profile: "reliable"

qos_profiles:
  - name: "reliable"
    reliability:
      kind: RELIABLE
    durability:
      kind: TRANSIENT_LOCAL

types:
  - name: "DataType"
    kind: struct
    fields:
      - name: field1
        type: int32
        key: true
```

## Web GUI Features

### Topic Editor
- Visual topic configuration
- QoS policy tabs (Reliability, Durability, History, etc.)
- Partition management with tags
- Real-time validation

### Domain Topology
- Interactive D3.js force-directed graph
- Zoom and pan controls
- Node highlighting and selection
- Export to various formats

### QoS Wizard
- Use case-based configuration
- Templates for sensor, control, video, event, config
- Automatic QoS policy selection
- Custom QoS fine-tuning

### Configuration Manager
- Import/export YAML, JSON, ARXML
- Version comparison (diff view)
- History and rollback
- Client-side validation

## Design Rules

### Naming Conventions
- Topics: PascalCase (e.g., `VehicleSpeed`)
- Types: PascalCase (e.g., `SpeedData`)
- QoS Profiles: camelCase (e.g., `reliableLowLatency`)

### Safety Requirements
- Safety-critical topics must use RELIABLE reliability
- Control topics should have deadline configured
- ASIL-D systems should enable E2E protection

### Performance Guidelines
- History depth should not exceed 100 samples
- Configure resource_limits for bounded memory
- Use transport priority for critical data

## Statistics

- **Total Python Code**: ~3,500 lines
- **Total JavaScript Code**: ~1,800 lines
- **Total CMake Code**: ~400 lines
- **Example Configurations**: 4 complete templates
- **Design Rules**: 12 built-in rules
- **CLI Commands**: 7 main commands

## Integration Example

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(my_dds_app)

# Include DDS code generation
include(cmake/DDSCodeGen.cmake)

# Setup DDS from configuration
dds_setup_from_config(${CMAKE_SOURCE_DIR}/config/adas_perception.yaml)

# Create executable
add_executable(my_app main.cpp)
target_link_libraries(my_app eth_dds my_dds_types)
```

## Future Enhancements

1. **Additional DDS Implementations**: OpenDDS, FastDDS-Gen integration
2. **Additional Platforms**: Zephyr RTOS, ThreadX support
3. **Advanced Analysis**: Scheduling analysis, bus load calculation
4. **IDE Integration**: VS Code extension, Eclipse plugin
5. **Cloud Integration**: Configuration repository, collaborative editing