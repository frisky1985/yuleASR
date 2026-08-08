# DDS Configuration Tool

A comprehensive tool for configuring DDS (Data Distribution Service) settings for automotive Ethernet applications with AUTOSAR integration.

## Features

- **Domain Configuration**: Configure DDS domains with discovery protocols (Simple, Static, Dynamic)
- **Topic Management**: Define topics with data types, QoS policies, and security settings
- **QoS Configuration**: Set up Reliability, Durability, Deadline, Latency Budget, Liveliness policies
- **Transport Configuration**: Configure UDPv4/UDPv6, SHM, TCP transports
- **Security**: Authentication, Access Control, Encryption, and E2E Protection (P01-P22 profiles)
- **AUTOSAR Integration**: EcuM, BswM, SoAd, PduR mappings
- **Code Generation**: Generate C headers and source files for DDS configuration

## Installation

```bash
cd tools/dds_config
pip install -r requirements.txt
```

### Requirements

- Python 3.7+
- PyQt5 (for GUI mode)
- lxml (for XML parsing)

## Usage

### GUI Mode

```bash
./dds_config_tool.py
# or
python3 dds_config_tool.py gui
```

### CLI Mode

```bash
# Validate configuration
python3 dds_config_tool.py validate -i config.xml

# Generate C code
python3 dds_config_tool.py generate -i config.xml -o ./output

# Convert between formats
python3 dds_config_tool.py convert -i config.xml -o config.json

# Create template
python3 dds_config_tool.py template -o new_config.xml -t automotive_basic
```

## Directory Structure

```
tools/dds_config/
├── dds_config_tool.py       # Main entry point
├── gui/                     # GUI implementation
│   ├── main_window.py       # Main application window
│   ├── domain_tab.py        # Domain configuration
│   ├── topic_tab.py         # Topic configuration
│   ├── qos_tab.py           # QoS configuration
│   └── transport_tab.py     # Transport configuration
├── parser/                  # Configuration parsers
│   ├── xml_parser.py        # XML format parser
│   └── json_parser.py       # JSON format parser
├── generator/               # Code generators
│   ├── c_generator.py       # C code generator
│   └── code_templates.py    # Code templates
├── validator/               # Configuration validation
│   └── config_validator.py  # Configuration validator
└── templates/               # Configuration templates
    ├── automotive_basic.xml # Basic automotive config
    └── automotive_safety.xml # Safety-critical config
```

## Generated Files

The tool generates the following C configuration files:

- `include/dds/Dds_Cfg.h` - Main DDS configuration
- `include/dds/Dds_TopicCfg.h` - Topic definitions
- `include/dds/Dds_QosCfg.h` - QoS configuration
- `include/dds/Dds_TransportCfg.h` - Transport configuration
- `src/dds/Dds_Cfg.c` - Configuration data
- `include/autosar/service/BswM/BswM_DdsRules.h` - AUTOSAR BswM rules

## Configuration Workflow

1. **Start GUI**: Launch the configuration tool
2. **Import/Create**: Import existing XML or create new configuration
3. **Configure**: Set up Domain, Topics, QoS, Transport, Security
4. **Validate**: Check configuration consistency
5. **Generate**: Create C code and configuration files
6. **Export**: Optionally export to AUTOSAR ARXML

## E2E Protection Profiles

The tool supports the following E2E protection profiles:

- **P01**: Basic CRC protection
- **P02**: CRC with sequence counter
- **P04**: Profile with two CRCs
- **P05**: Profile with data ID
- **P06**: Profile with dynamic data ID
- **P07**: Extended profile
- **P11**: Safety profile
- **P22**: Automotive safety profile

## License

Copyright (c) 2024 DDS Integration Team
