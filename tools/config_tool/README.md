# DDS Configuration Tool - Automotive Edition

A comprehensive Python-based GUI configuration tool for DDS-based automotive systems, supporting diagnostic (UDS/DCM), E2E protection, and OTA update configurations.

## Features

### 1. Diagnostic Configuration (UDS/DCM)
- **DCM Parameters**: ECU ID, logical/functional addresses, timing parameters
- **Diagnostic Services**: ISO 14229-1 compliant service configuration
- **DID Database**: Data Identifier definitions with security levels
- **DTC Configuration**: Diagnostic Trouble Code settings
- **Security Policies**: Access control with seed/key management
- **DoIP Configuration**: Diagnostic over IP (ISO 13400-2) settings

### 2. E2E Protection Configuration
- **E2E Profiles**: AUTOSAR profiles 1, 2, 4, 5, 6, 7
- **Data ID Mappings**: Signal-to-DataID mapping with safety levels
- **CRC Polynomials**: Standard and custom CRC definitions
- **Protection Settings**: Per-signal E2E configuration
- **Monitoring**: E2E state machine and error handling

### 3. OTA Update Configuration
- **Partition Layouts**: A/B partition scheme configuration
- **Download Parameters**: Chunk size, retries, compression, encryption
- **Security Verification**: Signature, hash, certificate chain validation
- **Campaign Management**: Update campaign definitions
- **Pre-conditions**: Installation requirements (battery, speed, etc.)
- **Installation Parameters**: Timing, reboot, user confirmation

### 4. Code Generation
- **C Header Generation**: Auto-generate type-safe C configurations
- **JSON Schema Validation**: Validate configurations against schemas
- **Multiple Output Formats**: C headers, JSON exports

## Installation

### Requirements
- Python 3.10+
- tkinter/ttk (usually included with Python)
- Optional: jsonschema for enhanced validation

### Setup
```bash
cd tools/config_tool
pip install -r requirements.txt  # Optional dependencies
```

## Usage

### GUI Mode
```bash
python main_window.py
```

### Programmatic Usage
```python
from diagnostic_config import DiagnosticConfig, DCMParams
from e2e_config import E2EConfig, E2EProfileConfig, E2EProfile
from ota_config import OTAConfig
from code_generator import CCodeGenerator

# Create configurations
diag = DiagnosticConfig()
diag.dcm_params = DCMParams(ecu_id=0x7E0, logical_address=0x0001, functional_address=0x7DF)

e2e = E2EConfig()
e2e.add_profile(E2EProfileConfig(profile=E2EProfile.PROFILE_5))

ota = OTAConfig()
layout = ota.get_default_partition_layout(flash_size_mb=8)
ota.add_partition_layout(layout)

# Generate C code
generator = CCodeGenerator()
generator.save_all(diag_config=diag, e2e_config=e2e, ota_config=ota, output_dir="generated")
```

## Configuration File Structure

### Diagnostic Configuration
```json
{
  "dcm_params": {
    "ecu_id": "0x7E0",
    "logical_address": "0x0001",
    "functional_address": "0x7DF",
    "p2_timeout_ms": 50
  },
  "services": [...],
  "dids": [...],
  "security_policies": [...]
}
```

### E2E Configuration
```json
{
  "profiles": {
    "5": {
      "profile": 5,
      "data_id": "0x0100",
      "data_length": 16
    }
  },
  "data_id_mappings": [...],
  "protections": [...]
}
```

### OTA Configuration
```json
{
  "partition_layouts": [...],
  "download_params": {...},
  "security_verification": {...},
  "campaigns": [...]
}
```

## Testing

Run all tests:
```bash
python -m unittest discover -s tests -v
```

Run specific test:
```bash
python -m unittest tests.test_diagnostic_config -v
python -m unittest tests.test_e2e_config -v
python -m unittest tests.test_ota_config -v
```

## Examples

See `examples/` directory for complete configuration examples:
- `diagnostic_example.json` - Diagnostic service configuration
- `e2e_example.json` - E2E protection for vehicle signals
- `ota_example.json` - OTA campaign with A/B partitions
- `complete_config.json` - Full system configuration

## Standards Compliance

- **UDS**: ISO 14229-1 (Unified Diagnostic Services)
- **DoIP**: ISO 13400-2 (Diagnostic over IP)
- **E2E**: AUTOSAR E2E Library (profiles 1-7)
- **OTA**: ISO 24089, UNECE R156, AUTOSAR FOTA
- **Crypto**: ISO/SAE 21434 (Cybersecurity)

## Module Documentation

### diagnostic_config.py
Manages diagnostic communication parameters, services, DIDs, DTCs, and security policies.

**Key Classes:**
- `DiagnosticConfig` - Main configuration manager
- `DCMParams` - DCM timing and addressing
- `DiagServiceConfig` - UDS service definitions
- `DIDDefinition` - Data Identifier database
- `SecurityPolicy` - Access control policies

### e2e_config.py
Manages end-to-end protection profiles, Data ID mappings, and CRC polynomials.

**Key Classes:**
- `E2EConfig` - Main configuration manager
- `E2EProfileConfig` - Profile parameters
- `DataIDMapping` - Signal-to-DataID mapping
- `E2EProtection` - Protection configuration
- `CRCPolynomial` - CRC polynomial definitions

### ota_config.py
Manages OTA campaigns, partition layouts, download parameters, and security verification.

**Key Classes:**
- `OTAConfig` - Main configuration manager
- `ABPartitionLayout` - A/B partition scheme
- `PartitionConfig` - Individual partition settings
- `OTACampaign` - Update campaign definition
- `ECUUpdate` - Per-ECU update configuration

### code_generator.py
Generates C header files from configuration objects with JSON Schema validation.

**Key Classes:**
- `CCodeGenerator` - C header generation
- `ConfigValidator` - Configuration validation

### main_window.py
Tkinter-based GUI for interactive configuration management.

**Key Classes:**
- `MainWindow` - Main application window
- `DiagnosticConfigFrame` - Diagnostic configuration tab
- `E2EConfigFrame` - E2E configuration tab
- `OTAConfigFrame` - OTA configuration tab

## License

This tool is part of the eth-dds-integration project.

## Author

Hermes Agent - Nous Research
