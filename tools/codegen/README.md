# DDS Code Generation Tools

This package provides comprehensive code generation capabilities for DDS-based automotive systems.

## Components

### 1. IDL Parser (`dds_idl_parser.py`)
Parses OMG IDL 4.x files and extracts type definitions.

```python
from dds_idl_parser import DDSIDLParser

parser = DDSIDLParser()
parser.parse_file("types.idl")
structs = parser.get_structs()
```

### 2. Type Generator (`dds_type_generator.py`)
Generates C/C++ code from IDL definitions.

```python
from dds_type_generator import DDSTypeGenerator

generator = DDSTypeGenerator(dds_impl="fastdds")
files = generator.generate_from_idl("types.idl", "generated/")
```

### 3. ARXML Parser (`autosar_arxml_parser.py`)
Parses AUTOSAR ARXML files for SWC and RTE integration.

```python
from autosar_arxml_parser import AutosarARXMLParser

parser = AutosarARXMLParser()
parser.parse_file("system.arxml")
mappings = parser.get_dds_topic_mappings()
```

### 4. RTE Generator (`rte_generator.py`)
Generates RTE interface code for AUTOSAR-DDS integration.

```python
from rte_generator import RTEGenerator, RTEConfig

config = RTEConfig(target_os="FreeRTOS", dds_implementation="FastDDS")
generator = RTEGenerator(config)
files = generator.generate_from_arxml("system.arxml", "generated_rte/")
```

### 5. Config Validator (`config_validator.py`)
Validates DDS configuration files against schema and design rules.

```python
from config_validator import ConfigValidator

validator = ConfigValidator()
result = validator.validate_file("config.yaml")
print(f"Valid: {result.is_valid}")
```

### 6. CLI Tool (`dds_config_cli.py`)
Command-line interface for all code generation functions.

```bash
# Validate configuration
dds-config validate config.yaml

# Generate code
dds-config generate --input config.yaml --output generated/ --dds-impl fastdds

# Convert formats
dds-config convert config.yaml --to-json

# Monitor runtime
dds-config monitor --config config.yaml --interval 5

# Create from template
dds-config template --type adas --output adas_config.yaml
```

## Generated Code Structure

```
generated/
├── dds_types.hpp          # C++ type definitions
├── dds_types.cpp          # Type implementations
├── dds_type_support.hpp   # DDS type support code
├── dds_serialization.hpp  # Serialization utilities
├── dds_config.h           # Configuration header
└── CMakeLists.txt         # Build configuration
```

## CMake Integration

Add to your CMakeLists.txt:

```cmake
include(DDSCodeGen)

# Generate code from config
dds_generate_code(
    CONFIG_FILE ${CMAKE_SOURCE_DIR}/config.yaml
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
    DDS_IMPL fastdds
    TARGET_OS freertos
    SAFETY_LEVEL ASIL_D
)

# Create library from generated code
dds_add_generated_library(my_dds_types
    CONFIG_FILE ${CMAKE_SOURCE_DIR}/config.yaml
)

# Or use the convenience macro
dds_setup_from_config(${CMAKE_SOURCE_DIR}/config.yaml)
```

## Supported DDS Implementations

- **FastDDS**: eProsima Fast DDS (default)
- **CycloneDDS**: Eclipse Cyclone DDS
- **RTI Connext**: RTI Connext DDS

## Supported Operating Systems

- FreeRTOS
- Linux/POSIX
- QNX
- Bare-metal (S32G3, AURIX TC3xx)