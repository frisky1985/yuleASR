# DDS Configuration Tool

A comprehensive Python-based tool for configuring DDS (Data Distribution Service) entities including DomainParticipants, Topics, QoS profiles, and more.

## Features

- **Command Line Interface (CLI)**: Validate configurations, generate C code, convert between YAML and ARXML formats
- **Web GUI**: Browser-based visual configuration editor with real-time validation
- **GUI Components**: Qt-based desktop widgets for topic editing and QoS management
- **Format Support**: YAML (primary), JSON, and ARXML (AUTOSAR XML) import/export
- **Code Generation**: Automatic generation of C code from configuration

## Project Structure

```
dds_config/
├── __init__.py              # Package initialization
├── dds_config_cli.py        # CLI entry point
├── requirements.txt         # Python dependencies
├── README.md               # This file
├── web/                    # Web GUI
│   ├── app.py             # Flask backend
│   ├── templates/
│   │   └── index.html     # Main web interface
│   └── static/
│       ├── css/style.css  # Styles
│       └── js/app.js      # Frontend JavaScript
└── gui/                   # Desktop GUI components
    ├── __init__.py
    ├── topic_editor.py    # Topic editor widget
    └── qos_manager.py     # QoS profile manager
```

## Installation

### Prerequisites

- Python 3.8 or higher
- pip package manager

### Install Dependencies

```bash
cd /path/to/eth-dds-integration/tools/dds_config
pip install -r requirements.txt
```

### Optional GUI Dependencies

For the Qt-based GUI components:

```bash
pip install PyQt5
```

## Usage

### Command Line Interface

The CLI tool provides several commands for DDS configuration management:

#### Validate Configuration

```bash
# Validate a YAML configuration file
python dds_config_cli.py validate config.yaml

# With verbose output
python dds_config_cli.py validate config.yaml -v
```

#### Generate C Code

```bash
# Generate C code from configuration
python dds_config_cli.py generate config.yaml -o ./output

# Output directory will contain:
#   - dds_config.h    # Header file with declarations
#   - dds_config.c    # Source file with initialization
#   - dds_qos_config.c # QoS profile definitions
```

#### Convert Formats

```bash
# Convert YAML to ARXML (AUTOSAR)
python dds_config_cli.py convert --yaml2arxml config.yaml output.arxml

# Convert ARXML to YAML
python dds_config_cli.py convert --arxml2yaml input.arxml output.yaml
```

#### Help

```bash
python dds_config_cli.py --help
python dds_config_cli.py validate --help
```

### Web GUI

The web interface provides a visual editor for DDS configuration:

#### Start the Server

```bash
# Default: http://localhost:5000
python -m dds_config.web.app

# Custom host and port
python -m dds_config.web.app --host 0.0.0.0 --port 8080

# Debug mode
python -m dds_config.web.app --debug
```

#### Using Flask CLI

```bash
export FLASK_APP=dds_config/web/app.py
export FLASK_ENV=development
flask run --host=0.0.0.0 --port=5000
```

#### Web Interface Features

- **Editor**: YAML editor with syntax highlighting and live validation
- **Domains**: Visual domain and participant management
- **Topics**: Topic configuration with QoS assignment
- **QoS**: QoS profile creation and editing
- **Preview**: Real-time C code generation preview
- **Import/Export**: Load and save configuration files

### GUI Components (Qt)

For desktop applications using Qt:

```python
from dds_config.gui import TopicEditor, QoSManager
from PyQt5.QtWidgets import QApplication

app = QApplication([])

# Create topic editor
editor = TopicEditor()
editor.show()

# Create QoS manager
manager = QoSManager()
manager.show()

app.exec_()
```

## Configuration Format

### Example YAML Configuration

```yaml
system:
  name: "ADAS_Sensor_Fusion"
  version: "1.2.0"
  description: "Sensor fusion system configuration"

domains:
  - id: 1
    name: "PerceptionDomain"
    participants:
      - name: "CameraECU"
        qos_profile: "sensor_high_reliability"
        publishers:
          - topic: "CameraObjectDetection"
            type: "ObjectDetectionData"
            qos:
              reliability: RELIABLE
              deadline:
                sec: 0
                nanosec: 33000000  # 30Hz
              history:
                kind: KEEP_LAST
                depth: 1
        subscribers:
          - topic: "FusionResult"
            type: "FusionData"

      - name: "RadarECU"
        publishers:
          - topic: "RadarDetection"
            type: "RadarData"
            qos:
              reliability: RELIABLE

  - id: 2
    name: "ControlDomain"
    participants:
      - name: "PlanningECU"
        qos_profile: "control_realtime"
        subscribers:
          - topic: "CameraObjectDetection"
            type: "ObjectDetectionData"
          - topic: "RadarDetection"
            type: "RadarData"

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
      bandwidth: 10000000  # 10Mbps
      max_latency: 100000  # 100us

qos_profiles:
  default:
    reliability: BEST_EFFORT
    history:
      kind: KEEP_LAST
      depth: 1

  reliable:
    reliability: RELIABLE
    durability: TRANSIENT_LOCAL
    history:
      kind: KEEP_LAST
      depth: 10

  sensor_high_reliability:
    reliability: RELIABLE
    durability: VOLATILE
    history:
      kind: KEEP_LAST
      depth: 5
    deadline:
      sec: 0
      nanosec: 50000000  # 20Hz

  control_realtime:
    reliability: RELIABLE
    deadline:
      sec: 0
      nanosec: 10000000  # 100Hz
    latency_budget:
      sec: 0
      nanosec: 100000    # 100us
```

### Configuration Schema

#### System Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | System name |
| version | string | No | Version string |
| description | string | No | System description |

#### Domain Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | integer | Yes | Domain ID (0-232) |
| name | string | Yes | Domain name |
| participants | array | No | Participant configurations |

#### Participant Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Participant name |
| qos_profile | string | No | Default QoS profile |
| publishers | array | No | Publisher endpoints |
| subscribers | array | No | Subscriber endpoints |

#### Topic Endpoint Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| topic | string | Yes | Topic name |
| type | string | No | Data type name |
| qos | object | No | Endpoint-specific QoS |

#### QoS Policy Values

- **reliability**: `BEST_EFFORT`, `RELIABLE`
- **durability**: `VOLATILE`, `TRANSIENT_LOCAL`, `TRANSIENT`, `PERSISTENT`
- **history.kind**: `KEEP_LAST`, `KEEP_ALL`
- **liveliness**: `AUTOMATIC`, `MANUAL_BY_TOPIC`, `MANUAL_BY_PARTICIPANT`
- **destination_order**: `BY_RECEPTION_TIMESTAMP`, `BY_SOURCE_TIMESTAMP`

## REST API

The web server provides a REST API for programmatic access:

### Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET/POST/PUT | `/api/config` | Get/update configuration |
| POST | `/api/config/upload` | Upload configuration file |
| GET | `/api/config/download` | Download configuration |
| POST | `/api/validate` | Validate configuration |
| POST | `/api/generate` | Generate C code |
| POST | `/api/generate/download` | Download generated code as ZIP |
| POST | `/api/convert` | Convert between formats |
| GET/POST | `/api/domains` | List/create domains |
| GET/PUT/DELETE | `/api/domains/{id}` | Manage specific domain |
| GET/POST | `/api/topics` | List/create topics |
| GET/POST | `/api/qos` | List/create QoS profiles |
| GET/PUT/DELETE | `/api/qos/{name}` | Manage specific QoS profile |
| GET | `/api/templates` | Get configuration templates |

### Example API Usage

```bash
# Validate configuration
curl -X POST http://localhost:5000/api/validate \
  -H "Content-Type: application/json" \
  -d @config.yaml

# Generate code
curl -X POST http://localhost:5000/api/generate \
  -H "Content-Type: application/json" \
  -d @config.yaml > generated.zip
```

## Generated Code Structure

The code generator produces C code compatible with the ETH-DDS integration library:

### dds_config.h

- System configuration macros
- Domain ID definitions
- DomainParticipant extern declarations
- Topic name constants
- Topic extern declarations
- QoS profile extern declarations
- Function prototypes

### dds_config.c

- DomainParticipant instance variables
- Topic instance variables
- `dds_config_init()` - Initialization function
- `dds_config_deinit()` - Cleanup function

### dds_qos_config.c

- QoS profile constant definitions

## Validation

The tool validates configurations against:

1. **Schema Compliance**: Required fields and data types
2. **Uniqueness**: Domain IDs, participant names, topic names
3. **Range Constraints**: Domain ID (0-232), port numbers, etc.
4. **QoS Compatibility**: Policy combinations and publisher/subscriber matching
5. **References**: QoS profile names, type definitions

## Troubleshooting

### Common Issues

#### ImportError: No module named 'yaml'

```bash
pip install pyyaml
```

#### Cannot start web server - Port already in use

```bash
# Use a different port
python -m dds_config.web.app --port 8080
```

#### Validation errors for valid configuration

Ensure the configuration follows the schema exactly. Check:
- Required fields are present
- Data types are correct
- Arrays vs objects distinction

### Debug Mode

Enable debug logging:

```bash
# CLI
python dds_config_cli.py validate config.yaml -v

# Web server
python -m dds_config.web.app --debug
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

Apache-2.0 License - See project root for details.

## See Also

- [DDS Core API](../../src/dds/core/dds_core.h)
- [Architecture ADR](../../docs/architecture/adr/ADR-003-config-tool-architecture.md)
- [Web GUI Documentation](web/README.md)
