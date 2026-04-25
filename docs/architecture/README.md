# Architecture Overview

**Version**: 2.0.0  
**Last Updated**: 2025-04-25  
**Status**: Production Ready

---

## Table of Contents

1. [System Context](#system-context)
2. [Design Principles](#design-principles)
3. [Layered Architecture](#layered-architecture)
4. [Data Flow](#data-flow)
5. [Configuration Management](#configuration-management)
6. [Deployment Architecture](#deployment-architecture)
7. [Architecture Decision Records](#architecture-decision-records)

---

## System Context

### Purpose

The ETH-DDS Integration Framework provides a complete automotive-grade middleware solution for deterministic data communication over Ethernet, combining:

- **DDS (Data Distribution Service)**: Publish-subscribe middleware
- **TSN (Time-Sensitive Networking)**: Deterministic Ethernet transport
- **AUTOSAR**: Classic RTE and Adaptive ara::com integration
- **Functional Safety**: ASIL-D certified safety mechanisms

### Target Applications

| Domain | Use Case | ASIL Level |
|--------|----------|------------|
| ADAS | Sensor fusion, perception | ASIL-D |
| Powertrain | Engine control, transmission | ASIL-D |
| Body | Lighting, climate | ASIL-B |
| Infotainment | Media, navigation | QM |
| Diagnostics | OBD, logging | ASIL-B |

### Stakeholders

- **System Architects**: Define integration patterns
- **Embedded Developers**: Implement ECU software
- **Safety Engineers**: Certify ASIL-D compliance
- **Network Engineers**: Configure TSN parameters

---

## Design Principles

### 1. Separation of Concerns

```
┌────────────────0────────────────────────────────┐
│                    Application Layer                      │
│         (DDS Topics, QoS Policies, Safety Monitors)       │
├────────────────────────────────────────────────┤
│                 Middleware Layer                          │
│    (DDS Core, RTPS Protocol, TSN Stack, AUTOSAR RTE)      │
├────────────────────────────────────────────────┤
│                 Transport Layer                           │
│         (UDP/IP, Shared Memory, Ethernet MAC)             │
├────────────────────────────────────────────────┤
│                Platform Abstraction                       │
│      (FreeRTOS, Hardware Drivers, Safety Modules)         │
└────────────────────────────────────────────────┘
```

### 2. Configurability

All major features are compile-time configurable:

```cmake
# CMake options for customization
option(ENABLE_TSN "Enable TSN support" ON)
option(ENABLE_DDS_SECURITY "Enable DDS-Security" ON)
option(ENABLE_AUTOSAR_CLASSIC "Enable Classic RTE" ON)
option(FREERTOS_PORT "Target architecture" ARM_CM7)
```

### 3. Testability

- **Unit tests**: Component isolation with mocking
- **Integration tests**: Module interaction verification
- **System tests**: End-to-end scenarios
- **HIL tests**: Hardware-in-the-loop validation

---

## Layered Architecture

### Component Diagram

```
                            +------------------+
                            |   Applications   |
                            |  (ADAS, Powertrain|
                            |   Diagnostics)   |
                            +--------+---------+
                                     |
                    +----------------v------------------+
                    |         AUTOSAR Layer             |
                    |  +------------+  +-------------+  |
                    |  |   Classic  |  |  Adaptive   |  |
                    |  |    RTE     |  |   ara::com  |  |
                    |  +------------+  +-------------+  |
                    |  +------------+  +-------------+  |
                    |  |    E2E     |  |   ARXML     |  |
                    |  | Protection |  |   Parser    |  |
                    |  +------------+  +-------------+  |
                    +----------------+------------------+
                                     |
                    +----------------v------------------+
                    |           DDS Layer               |
                    |  +------------+  +-------------+  |
                    |  |   Domain   |  |    Topic    |  |
                    |  |   Entity   |  |   Entity    |  |
                    |  +------------+  +-------------+  |
                    |  +------------+  +-------------+  |
                    |  |   RTPS     |  |   Security  |  |
                    |  |  Protocol  |  |   Manager   |  |
                    |  +------------+  +-------------+  |
                    +----------------+------------------+
                                     |
                    +----------------v------------------+
                    |           TSN Layer               |
                    |  +------+ +------+ +------+       |
                    |  | gPTP | | TAS  | | CBS  |       |
                    |  | Sync | |Shaper| |Shaper|       |
                    |  +------+ +------+ +------+       |
                    |  +------+ +------------------+    |
                    |  | Frame | | Stream Reservation |  |
                    |  | Preempt| |      (SRP)        |  |
                    |  +------+ +------------------+    |
                    +----------------+------------------+
                                     |
                    +----------------v------------------+
                    |        Transport Layer            |
                    |  +------------+  +-------------+  |
                    |  |    UDP     |  |   Shared    |  |
                    |  | Transport  |  |   Memory    |  |
                    |  +------------+  +-------------+  |
                    +----------------+------------------+
                                     |
                    +----------------v------------------+
                    |      Platform Abstraction         |
                    |  +------------+  +-------------+  |
                    |  |  Ethernet  |  |   FreeRTOS  |  |
                    |  |   Driver   |  |    Kernel   |  |
                    |  +------------+  +-------------+  |
                    |  +------------+  +-------------+  |
                    |  |   Safety   |  |    NVM      |  |
                    |  |   (ECC)    |  |  Manager    |  |
                    |  +------------+  +-------------+  |
                    +-----------------------------------+
```

### Module Responsibilities

| Module | Responsibility | Key Files |
|--------|----------------|-----------|
| **DDS Core** | Topic management, QoS enforcement | `dds/core/` |
| **RTPS** | Wire protocol, discovery | `dds/rtps/` |
| **TSN** | Time synchronization, traffic shaping | `tsn/` |
| **Transport** | Network abstraction | `transport/` |
| **Ethernet** | MAC/DMA driver | `ethernet/driver/` |
| **Safety** | RAM ECC, SafeRAM, NVM | `safety/` |
| **Platform** | OS abstraction, HW drivers | `platform/` |

---

## Data Flow

### Publish-Subscribe Flow

```
Publisher Application
        |
        v
+-----------------------------------+
| DDS Writer                        |
| - History cache                   |
| - QoS policies                    |
+-----------------------------------+
        |
        v
+-----------------------------------+
| RTPS Protocol                     |
| - Serialize data                  |
| - Create RTPS messages            |
+-----------------------------------+
        |
        v
+-----------------------------------+
| Transport Layer                   |
| - UDP multicast                   |
| - Or shared memory                |
+-----------------------------------+
        |
        v
Subscriber Application
        |
        v
+-----------------------------------+
| RTPS Protocol                     |
| - Parse messages                  |
| - Deliver to readers              |
+-----------------------------------+
        |
        v
+-----------------------------------+
| DDS Reader                        |
| - Apply filters                   |
| - Notify application              |
+-----------------------------------+
```

### TSN Data Flow with Time-Triggered Traffic

```
Application Data
        |
        v
+------------------+
| Traffic Classifier|
| (IEEE 802.1Qci)  |
+------------------+
        |
   +----+----+
   |         |
   v         v
+------+  +------+
| TAS  |  | CBS  |
| Gate |  | Queue|
|(Qbv) |  |(Qav) |
+------+  +------+
   |         |
   +----+----+
        |
        v
+------------------+
| Frame Preemption |
|   (802.1Qbu)     |
+------------------+
        |
        v
+------------------+
| Ethernet MAC     |
+------------------+
```

### QoS to TSN Mapping

| DDS QoS Policy | TSN Feature | Purpose |
|----------------|-------------|---------|
| DEADLINE | TAS Time-Aware Shaping | Deterministic latency |
| RELIABILITY | Frame Preemption | Protect critical traffic |
| TRANSPORT_PRIORITY | CBS Credit-Based Shaper | Bandwidth allocation |
| LIFESPAN | gPTP Time Sync | Time synchronization |

---

## Configuration Management

### Configuration Hierarchy

```
System Configuration
├── Domain Configuration (DDS domain, global settings)
│   ├── Participant Configuration
│   │   ├── Topic Configuration
│   │   │   ├── Publisher/Subscriber QoS
│   │   │   └── Writer/Reader Settings
│   │   └── Transport Configuration
│   └── TSN Configuration
│       ├── gPTP Settings
│       ├── TAS Schedule
│       └── Stream Reservations
└── Safety Configuration
    ├── ECC Monitoring Regions
    ├── Stack/Heap Protection
    └── NVM Block Configuration
```

### Configuration Files

| File | Purpose | Format |
|------|---------|--------|
| `dds_config.yaml` | DDS entity configuration | YAML |
| `tsn_schedule.json` | TAS gate schedule | JSON |
| `e2e_config.xml` | E2E protection config | ARXML |
| `safety_config.h` | Safety module parameters | C header |

### Configuration Tools

- **CLI Tool**: `dds_config_tool` for validation and code generation
- **Web GUI**: Browser-based configuration editor
- **ARXML Parser**: Import AUTOSAR configurations

---

## Deployment Architecture

### Network Topology Example

```
                    +------------------+
                    |   Central ECU    |
                    |  (Domain Controller|
                    |   + TSN Bridge)  |
                    +--------+---------+
                             |
            +----------------+----------------+
            |                |                |
    +-------v------+ +-------v------+ +-------v------+
    |   ADAS ECU   | | Powertrain   | |  Body ECU    |
    |   (ASIL-D)   | |   ECU        | |  (ASIL-B)    |
    |              | |  (ASIL-D)    | |              |
    | - Cameras    | | - Engine     | | - Lighting   |
    | - Lidar      | | - Gearbox    | | - Climate    |
    | - Radar      | | - Battery    | | - Doors      |
    +--------------+ +--------------+ +--------------+
            |                |                |
            +----------------+----------------+
                             |
                    +--------v---------+
                    |   Diagnostics    |
                    |     Gateway      |
                    +------------------+
```

### TSN Stream Mapping

| Stream | Source | Destination | Period | Deadline | TSN Feature |
|--------|--------|-------------|--------|----------|-------------|
| Camera | ADAS | Central | 33ms | 5ms | TAS |
| Engine | Powertrain | Central | 10ms | 2ms | Frame Preemption |
| Climate | Body | Central | 100ms | 50ms | CBS |

### Hardware Deployment Options

| Platform | MCU | Use Case |
|----------|-----|----------|
| S32G3 | Cortex-M7 | Domain controller |
| AURIX TC3xx | TriCore | Powertrain control |
| STM32H7 | Cortex-M7 | General purpose |
| POSIX | x86/ARM64 | Development/simulation |

---

## Architecture Decision Records

Key architectural decisions are documented in ADRs:

| ADR | Title | Status |
|-----|-------|--------|
| [ADR-001](adr/ADR-001-freertos-multi-architecture.md) | FreeRTOS Multi-Architecture Support | Accepted |
| [ADR-002](adr/ADR-002-dds-tsn-integration.md) | DDS-TSN Integration Strategy | Accepted |
| [ADR-003](adr/ADR-003-config-tool-architecture.md) | Configuration Tool Architecture | Accepted |
| [ADR-004](adr/ADR-004-security-architecture.md) | Security Architecture | Accepted |

---

## Related Documentation

- [Domain Model](domain-model.md) - DDD entity definitions
- [Testing Strategy](../testing/strategy.md) - L1-L4 test approach
- [User Stories](../product/user-stories/automotive-dds.md) - Use cases
- [Safety Reviews](../safety/MODULE_DESIGN_REVIEWS_SUMMARY.md) - ASIL-D compliance

---

## Archived Documents

Original detailed documents (superseded by this consolidation):

- [01-architecture-overview.md](../archive/v1/01-architecture-overview.md) - Original architecture overview
- [02-data-flow-design.md](../archive/v1/02-data-flow-design.md) - Original data flow
- [03-config-sync-mechanism.md](../archive/v1/03-config-sync-mechanism.md) - Original config docs
- [04-deployment-architecture.md](../archive/v1/04-deployment-architecture.md) - Original deployment docs

---

**Last Updated**: 2025-04-25  
**Document Version**: 2.0.0
