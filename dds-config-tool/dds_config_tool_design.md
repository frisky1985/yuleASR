# DDS Configuration Tool Design Document

## 1. Overview

### 1.1 Purpose
This document describes the design of a DDS (Data Distribution Service) configuration tool that provides a comprehensive solution for configuring DDS middleware in real-time distributed systems.

### 1.2 Scope
- Topic configuration management
- QoS (Quality of Service) policy configuration
- Domain participant management
- Transport layer configuration
- Security settings

## 2. System Architecture

### 2.1 High-Level Architecture

```
+-------------------+      +-------------------+      +-------------------+
|   CLI Interface   |----->|  Config Parser    |----->|  Config Validator |
|   (dds_config)    |      |  (YAML/JSON/XML)  |      |  (Schema Check)   |
+-------------------+      +-------------------+      +-------------------+
                                                          |
                                                          v
+-------------------+      +-------------------+      +-------------------+
|   DDS Runtime     |<-----|  Config Generator |<-----|  Config Manager   |
|   Integration     |      |  (C Code Gen)     |      |  (In-Memory DB)   |
+-------------------+      +-------------------+      +-------------------+
```

### 2.2 Component Description

#### 2.2.1 CLI Interface Module
- Command-line interface for user interaction
- Supports commands: init, validate, generate, apply, export
- Interactive and batch mode support

#### 2.2.2 Configuration Parser
- Multi-format support (YAML, JSON, XML)
- Hierarchical configuration parsing
- Error reporting with line numbers
- Default value injection

#### 2.2.3 Configuration Validator
- Schema-based validation
- Cross-reference checking
- QoS policy consistency checks
- Domain ID range validation

#### 2.2.4 Configuration Manager
- In-memory configuration storage
- Transaction-based updates
- Configuration versioning
- Rollback support

#### 2.2.5 Code Generator
- C header/code generation
- DDS IDL generation
- CMake integration support

## 3. Configuration File Format

### 3.1 File Format: YAML (Primary)

**Rationale:**
- Human-readable and writable
- Native comment support
- Hierarchical structure
- Widely used in modern DevOps

### 3.2 Configuration Schema

See `config_schema.yaml` for the complete schema definition.

### 3.3 Configuration Sections

#### 3.3.1 Domain Configuration
```yaml
domains:
  - id: 0
    name: "DefaultDomain"
    description: "Default domain for system communication"
```

#### 3.3.2 Topic Configuration
```yaml
topics:
  - name: "SensorData"
    type: "SensorDataType"
    domain_id: 0
    qos_profile: "sensor_qos"
```

#### 3.3.3 QoS Profile Configuration
```yaml
qos_profiles:
  - name: "sensor_qos"
    reliability: BEST_EFFORT
    durability: VOLATILE
    history: KEEP_LAST
    history_depth: 10
    deadline: 100ms
    latency_budget: 50ms
```

## 4. Core Function Modules

### 4.1 Topic Configuration Module

**Responsibilities:**
- Topic name registration and management
- Data type association
- Domain binding
- QoS profile assignment

**Key Data Structures:**
```c
typedef struct {
    char name[256];
    char type_name[256];
    uint32_t domain_id;
    char qos_profile[128];
    dds_topic_qos_t qos;
} dds_topic_config_t;
```

### 4.2 QoS Policy Module

**Supported QoS Policies:**
1. Reliability (BEST_EFFORT, RELIABLE)
2. Durability (VOLATILE, TRANSIENT_LOCAL, TRANSIENT, PERSISTENT)
3. History (KEEP_LAST, KEEP_ALL)
4. Deadline
5. Latency Budget
6. Lifespan
7. Ownership
8. Liveliness
9. Destination Order
10. Resource Limits

**Key Data Structures:**
```c
typedef struct {
    dds_reliability_kind_t kind;
    dds_duration_t max_blocking_time;
} dds_reliability_qos_t;

typedef struct {
    dds_history_kind_t kind;
    int32_t depth;
} dds_history_qos_t;
```

### 4.3 Domain Management Module

**Responsibilities:**
- Domain ID allocation (0-230)
- Domain participant lifecycle
- Discovery configuration
- Transport binding

**Key Data Structures:**
```c
typedef struct {
    uint32_t domain_id;
    char name[256];
    char description[512];
    dds_discovery_config_t discovery;
    dds_transport_config_t transport;
} dds_domain_config_t;
```

## 5. User Interface Design

### 5.1 CLI Commands

#### 5.1.1 init - Initialize Configuration
```bash
dds-config init [--format yaml|json|xml] [--output config.yaml]
```
Creates a new configuration file with default values.

#### 5.1.2 validate - Validate Configuration
```bash
dds-config validate config.yaml
```
Validates configuration against schema and DDS constraints.

#### 5.1.3 generate - Generate Code
```bash
dds-config generate config.yaml --output-dir ./generated/
```
Generates C code and IDL files from configuration.

#### 5.1.4 apply - Apply Configuration
```bash
dds-config apply config.yaml [--domain 0]
```
Applies configuration to running DDS system.

#### 5.1.5 export - Export Configuration
```bash
dds-config export config.yaml --format json --output exported.json
```
Converts configuration to different format.

#### 5.1.6 list - List Configuration
```bash
dds-config list config.yaml --topics --qos-profiles
```
Lists configuration entities.

### 5.2 Interactive Mode
```bash
dds-config interactive
> domain add 1 "Domain1"
> topic add "CommandTopic" "CommandType" --domain 1 --qos reliable
> validate
> save config.yaml
> exit
```

## 6. Error Handling

### 6.1 Error Codes
- `DDS_CONFIG_OK` (0) - Success
- `DDS_CONFIG_ERR_INVALID_FORMAT` (-1) - Invalid file format
- `DDS_CONFIG_ERR_SCHEMA_VIOLATION` (-2) - Schema validation failed
- `DDS_CONFIG_ERR_INVALID_DOMAIN` (-3) - Domain ID out of range
- `DDS_CONFIG_ERR_DUPLICATE_TOPIC` (-4) - Duplicate topic name
- `DDS_CONFIG_ERR_INVALID_QOS` (-5) - Invalid QoS configuration
- `DDS_CONFIG_ERR_FILE_IO` (-6) - File I/O error
- `DDS_CONFIG_ERR_MEMORY` (-7) - Memory allocation failed

### 6.2 Error Reporting
- Line and column information for parsing errors
- Context-aware error messages
- Suggested fixes for common errors

## 7. Security Considerations

### 7.1 Authentication
- Support for DDS Security plugin configuration
- Certificate and key file paths
- Access control list (ACL) configuration

### 7.2 Encryption
- Transport encryption settings
- DDS-SEC encryption algorithms
- Key management configuration

## 8. Integration

### 8.1 DDS Implementations
- RTI Connext DDS
- Eclipse Cyclone DDS
- eProsima Fast DDS
- OpenDDS

### 8.2 Build System Integration
- CMake module generation
- pkg-config support
- Autotools support

## 9. Testing Strategy

### 9.1 Unit Tests
- Parser unit tests
- Validator unit tests
- Code generator tests

### 9.2 Integration Tests
- End-to-end configuration tests
- DDS runtime integration tests
- Performance benchmarks

## 10. Future Enhancements

### 10.1 Planned Features
- GUI configuration editor
- Configuration diff/merge tools
- Runtime monitoring integration
- Cloud-based configuration management

### 10.2 Extensibility
- Plugin architecture for custom QoS policies
- Custom transport layer support
- Multi-language bindings

---

**Version:** 1.0
**Date:** 2024
**Author:** DDS Config Tool Development Team
