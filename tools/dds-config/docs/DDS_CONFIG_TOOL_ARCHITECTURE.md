# DDS Configuration Tool Architecture

## Overview

The DDS Configuration Tool is a web-based application for configuring DDS (Data Distribution Service) entities in AUTOSAR Adaptive and ROS2 environments. It integrates with the yuleCommunity toolchain.

## Architecture

```
├─────────────────────────────────────────────────────────┐
│           DDS Configuration Tool                      │
│  ├─────────────────────────────────────────────────────┬───────┤
│  │                 Frontend (React/Vue)              ││       │
│  │  ├──────────────────────────────────────────────────┤│       │
│  │  │  Domain Editor                               │││       │
│  │  │  Topic Editor                                │││       │
│  │  │  QoS Policy Configurator                     │││       │
│  │  │  Entity Relationship Visualizer              │││       │
│  │  └──────────────────────────────────────────────────┘│       │
│  │                                                      ││       │
│  ├─────────────────────────────────────────────────────┤│       │
│  │                 Backend (Python/FastAPI)        ││       │
│  │  ├──────────────────────────────────────────────────┤│       │
│  │  │  Configuration API                           │││       │
│  │  │  Code Generator                              │││       │
│  │  │  Validation Engine                           │││       │
│  │  │  Import/Export (XML/JSON)                    │││       │
│  │  └──────────────────────────────────────────────────┘│       │
│  └─────────────────────────────────────────────────────┘       │
│                                                        │       │
│  Integration with yuleCommunity Toolchain              │◀─────┘
│  ├─────────────────────────────────────────────────────┤
│  │  ARXML Import/Export                          ││
│  │  SOME/IP Integration                          ││
│  │  IDL Compiler Interface                       ││
│  └─────────────────────────────────────────────────────┘
└─────────────────────────────────────────────────────────┘
```

## Core Features

### 1. Domain Configuration
- Create and manage DDS Domains
- Configure Domain Participants
- Set up Domain-specific QoS

### 2. Topic Management
- Define Topics with data types
- Configure Topic QoS policies
- Support keyed and unkeyed topics
- IDL (Interface Definition Language) editor

### 3. Entity Configuration
- Publishers & Subscribers
- DataWriters & DataReaders
- QoS policy inheritance
- Entity relationship visualization

### 4. QoS Policies
- Reliability (Best-Effort, Reliable)
- Durability (Volatile, Transient, Persistent)
- History (Keep-Last, Keep-All)
- Deadline, Latency Budget
- Liveliness, Lifespan
- Ownership, Destination Order

### 5. Code Generation
- Generate C++ code for DDS entities
- CMake integration
- ROS2 rmw adapter support

## Data Model

```python
# Core entities
class DDSDomain:
    domain_id: int
    participants: List[DDSParticipant]
    qos: DomainQos

class DDSParticipant:
    name: str
    qos: ParticipantQos
    publishers: List[DDSPublisher]
    subscribers: List[DDSSubscriber]

class DDSTopic:
    name: str
    data_type: str  # IDL type
    qos: TopicQos

class DDSPublisher:
    name: str
    qos: PublisherQos
    data_writers: List[DDSDataWriter]

class DDSSubscriber:
    name: str
    qos: SubscriberQos
    data_readers: List[DDSDataReader]
```

## File Formats

### Export/Import
- **XML**: Standard DDS XML format
- **JSON**: Internal tool format
- **IDL**: OMG IDL for data types
- **ARXML**: AUTOSAR Adaptive format

## Integration Points

### 1. yuleCommunity Toolchain
```
yuleASR/tools/
├─── dds-config/           # DDS Configuration Tool
│   ├─── src/              # Source code
│   ├─── docs/             # Documentation
│   ├─── templates/        # Code templates
│   └─── tests/            # Test cases
└─── toolchain/          # Main toolchain
    └─── integrator.py     # Toolchain integrator
```

### 2. CI/CD Integration
- GitHub Actions workflow
- Docker containerization
- Automated validation

## Technology Stack

- **Frontend**: React + TypeScript
- **Backend**: Python + FastAPI
- **Database**: SQLite (local) / PostgreSQL (team)
- **Code Gen**: Jinja2 templates
- **Validation**: JSON Schema

## Security

- Authentication via yuleCommunity SSO
- Role-based access control
- Audit logging

## Roadmap

### Phase 1: Core Features
- [ ] Basic domain/topic configuration
- [ ] QoS policy editor
- [ ] Code generation (C++)

### Phase 2: Advanced Features
- [ ] Visual entity editor
- [ ] Real-time validation
- [ ] Performance profiling

### Phase 3: Integration
- [ ] SOME/IP gateway
- [ ] ROS2 rmw adapter
- [ ] Cloud deployment
