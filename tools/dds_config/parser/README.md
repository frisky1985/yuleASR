# DDS Configuration Parser

This module provides comprehensive parsing, validation, and conversion capabilities
for DDS configuration files in YAML and ARXML formats.

## Features

- **YAML Parser**: Parse DDS configuration from YAML files with detailed error reporting
- **Schema Validator**: Validate configurations against JSON Schema with detailed reports
- **ARXML Converter**: Bidirectional conversion between YAML and AUTOSAR ARXML formats
- **Type-Safe Models**: Full Python dataclass definitions for all DDS configuration entities

## Installation

The parser module is part of the DDS Configuration Tool. Ensure you have the required dependencies:

```bash
pip install pyyaml
```

## Module Structure

```
parser/
├── __init__.py          # Module exports
├── config_model.py      # Dataclass definitions for DDS entities
├── yaml_parser.py       # YAML parsing and writing
├── validator.py         # Schema validation
├── arxml_converter.py   # ARXML conversion
└── README.md           # This file
```

## Quick Start

### Parse YAML Configuration

```python
from parser import parse_yaml_file

# Parse a YAML file
config, warnings = parse_yaml_file("dds_config.yaml")

print(f"System: {config.system.name}")
print(f"Domains: {len(config.domains)}")
print(f"Participants: {config.get_participant_count()}")
```

### Validate Configuration

```python
from parser import SchemaValidator

validator = SchemaValidator()
report = validator.validate(config)

print(f"Valid: {report.is_valid}")
print(f"Errors: {len(report.errors)}")
print(f"Warnings: {len(report.warnings)}")

# Print full report
print(report.get_summary())
```

### Convert to ARXML

```python
from parser import yaml_to_arxml

arxml_content = yaml_to_arxml(config, package_name="MyDDSConfig")

# Save to file
with open("config.arxml", "w") as f:
    f.write(arxml_content)
```

### Convert from ARXML

```python
from parser import arxml_to_yaml, write_yaml_file

with open("config.arxml", "r") as f:
    arxml_content = f.read()

config = arxml_to_yaml(arxml_content)
write_yaml_file(config, "output.yaml")
```

## Configuration Model

### Core Classes

| Class | Description |
|-------|-------------|
| `DDSConfig` | Root configuration container |
| `SystemInfo` | System metadata (name, version) |
| `Domain` | DDS Domain with participants |
| `Participant` | DomainParticipant with publishers/subscribers |
| `Publisher` | Topic publisher configuration |
| `Subscriber` | Topic subscriber configuration |
| `Topic` | Topic definition |
| `QoS` | Quality of Service policies |
| `Transport` | Network transport settings |
| `TSN` | Time-Sensitive Networking configuration |
| `Security` | DDS Security settings |

### Creating Configuration Programmatically

```python
from parser import (
    DDSConfig, SystemInfo, Domain, Participant,
    Publisher, Subscriber, QoS, TSN, TSNStream
)

config = DDSConfig(
    system=SystemInfo(name="MySystem", version="1.0.0"),
    domains=[
        Domain(
            id=1,
            name="Domain1",
            participants=[
                Participant(
                    name="Node1",
                    publishers=[
                        Publisher(
                            topic="DataTopic",
                            type="DataType",
                            qos=QoS(reliability="RELIABLE")
                        )
                    ]
                )
            ]
        )
    ],
    tsn=TSN(
        enabled=True,
        streams=[TSNStream(name="Stream1", priority=7, bandwidth=1000000)]
    )
)
```

## YAML Format

Example DDS configuration file:

```yaml
system:
  name: "ADAS_Sensor_Fusion"
  version: "1.2.0"

domains:
  - id: 1
    name: "PerceptionDomain"
    participants:
      - name: "CameraECU"
        publishers:
          - topic: "CameraObjectDetection"
            type: "ObjectDetectionData"
            qos:
              reliability: RELIABLE
              deadline:
                sec: 0
                nanosec: 33000000
              history:
                kind: KEEP_LAST
                depth: 1

transport:
  kind: UDP
  interface: "eth0"
  multicast_address: "239.255.0.1"
  port_base: 7400

tsn:
  enabled: true
  streams:
    - name: "CameraStream"
      priority: 7
      bandwidth: 10000000
      max_latency: 100000

security:
  enabled: false
```

## ARXML Format

The ARXML converter supports AUTOSAR 4.x format with the following elements:

- `DDS-DOMAIN-PARTICIPANT-CONFIG` - Domain configuration
- `DDS-DOMAIN-PARTICIPANT` - Participant configuration
- `DDS-QOS-PROFILE` - Named QoS profiles
- `DDS-TOPIC-CONFIG` - Topic definitions
- `DDS-TRANSPORT-CONFIG` - Transport settings
- `DDS-TSN-CONFIG` - TSN configuration
- `DDS-SECURITY-CONFIG` - Security settings

## Validation Rules

The validator checks for:

- **Structure**: Required fields, correct types
- **Uniqueness**: Unique domain IDs, participant names, topic names
- **Ranges**: Domain IDs (0-232), TSN priority (0-7), port numbers (0-65535)
- **References**: QoS profile references, topic references
- **QoS Compatibility**: Publisher/subscriber QoS compatibility
- **Network**: Valid multicast addresses

## Error Handling

The parser provides detailed error information:

```python
from parser import parse_yaml_file, YAMLParseError

try:
    config, warnings = parse_yaml_file("config.yaml")
except YAMLParseError as e:
    print(f"Error: {e.message}")
    print(f"Location: Line {e.line}, Column {e.column}")
    print(f"Context: {e.context}")
```

## API Reference

### parse_yaml_file(file_path: str) -> Tuple[DDSConfig, List[str]]
Parse a YAML file and return configuration and warnings.

### parse_yaml_string(yaml_string: str, file_path: str = None) -> Tuple[DDSConfig, List[str]]
Parse a YAML string and return configuration and warnings.

### write_yaml_file(config: DDSConfig, file_path: str, include_comments: bool = True)
Write a DDSConfig to a YAML file.

### validate_config(config: DDSConfig, schema_path: str = None, strict: bool = False) -> ValidationReport
Validate a DDSConfig and return detailed report.

### validate_yaml_file(file_path: str, schema_path: str = None) -> ValidationReport
Validate a YAML file directly.

### yaml_to_arxml(config: DDSConfig, package_name: str = "DDSConfiguration") -> str
Convert DDSConfig to ARXML string.

### arxml_to_yaml(arxml_string: str) -> DDSConfig
Convert ARXML string to DDSConfig.

## License

This module is part of the ETH-DDS Integration project.
