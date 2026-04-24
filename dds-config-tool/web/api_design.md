# DDS Configuration Tool - Web API Design

## Overview

This document describes the REST API and WebSocket interface for the DDS Configuration Tool's web-based management interface. The API provides programmatic access to DDS configuration management, monitoring, and control operations.

## Base URL

```
http://localhost:8080/api/v1
```

## Authentication

### JWT Token Authentication

All API endpoints require authentication via JWT tokens.

**Request Header:**
```
Authorization: Bearer <jwt_token>
```

**Token Endpoints:**

#### POST /auth/login
Authenticate user and obtain JWT token.

**Request:**
```json
{
  "username": "admin",
  "password": "password",
  "mfa_code": "123456"
}
```

**Response:**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIs...",
  "refresh_token": "eyJhbGciOiJIUzI1NiIs...",
  "token_type": "Bearer",
  "expires_in": 3600,
  "permissions": ["read", "write", "admin"]
}
```

#### POST /auth/refresh
Refresh access token using refresh token.

**Request:**
```json
{
  "refresh_token": "eyJhbGciOiJIUzI1NiIs..."
}
```

#### POST /auth/logout
Invalidate current token.

## Configuration Management

### GET /configs
List all configurations.

**Query Parameters:**
- `page` (int): Page number (default: 1)
- `limit` (int): Items per page (default: 20, max: 100)
- `sort` (string): Sort field (created_at, name, version)
- `order` (string): Sort order (asc, desc)
- `search` (string): Search query
- `domain_id` (int): Filter by domain

**Response:**
```json
{
  "data": [
    {
      "id": "conf_abc123",
      "name": "Automotive ECU Config",
      "version": "2.1.0",
      "domain_id": 0,
      "created_at": "2024-01-15T10:30:00Z",
      "updated_at": "2024-01-20T14:22:00Z",
      "created_by": "admin",
      "status": "active",
      "tags": ["production", "automotive", "asil-d"]
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 20,
    "total": 156,
    "pages": 8
  }
}
```

### POST /configs
Create new configuration.

**Request:**
```json
{
  "name": "New DDS Config",
  "description": "Configuration for ADAS system",
  "domain_id": 1,
  "content": {
    "system": {
      "name": "ADAS_DDS",
      "version": "1.0.0"
    },
    "domains": [...],
    "topics": [...],
    "qos_profiles": [...]
  },
  "tags": ["adas", "safety-critical"]
}
```

**Response:**
```json
{
  "id": "conf_def456",
  "name": "New DDS Config",
  "version": "1.0.0",
  "created_at": "2024-01-21T09:00:00Z",
  "status": "draft"
}
```

### GET /configs/{id}
Get configuration details.

**Response:**
```json
{
  "id": "conf_abc123",
  "name": "Automotive ECU Config",
  "version": "2.1.0",
  "content": {
    "system": {...},
    "domains": [...],
    "topics": [...],
    "qos_profiles": [...],
    "participants": [...],
    "types": [...],
    "logging": {...},
    "monitoring": {...}
  },
  "validation_status": "valid",
  "deployment_status": "deployed",
  "metadata": {
    "created_by": "admin",
    "created_at": "2024-01-15T10:30:00Z",
    "modified_by": "engineer1",
    "modified_at": "2024-01-20T14:22:00Z",
    "deployed_at": "2024-01-20T14:25:00Z",
    "checksum": "sha256:abc123..."
  }
}
```

### PUT /configs/{id}
Update configuration.

**Request:**
```json
{
  "name": "Updated Config Name",
  "content": {...},
  "update_strategy": "hot_reload",
  "backup_before_update": true
}
```

### DELETE /configs/{id}
Delete configuration.

**Query Parameters:**
- `force` (bool): Force delete even if deployed (default: false)

### POST /configs/{id}/clone
Clone existing configuration.

**Request:**
```json
{
  "new_name": "Cloned Config",
  "version": "1.0.0",
  "inherit_tags": true
}
```

### POST /configs/{id}/validate
Validate configuration.

**Response:**
```json
{
  "valid": true,
  "errors": [],
  "warnings": [
    {
      "code": "W001",
      "message": "High latency QoS may impact real-time performance",
      "path": "qos_profiles.sensor_qos.latency_budget",
      "severity": "warning"
    }
  ],
  "suggestions": [
    {
      "code": "S001",
      "message": "Consider enabling security for production deployments",
      "path": "domains[0].security"
    }
  ]
}
```

### POST /configs/{id}/deploy
Deploy configuration to runtime.

**Request:**
```json
{
  "target_nodes": ["node1", "node2", "node3"],
  "deployment_strategy": "rolling",
  "rollback_on_failure": true,
  "health_check_timeout": 30000,
  "grace_period": 10000
}
```

**Response:**
```json
{
  "deployment_id": "dep_xyz789",
  "status": "in_progress",
  "started_at": "2024-01-21T09:15:00Z",
  "estimated_completion": "2024-01-21T09:16:30Z",
  "progress": {
    "total_nodes": 3,
    "completed_nodes": 1,
    "failed_nodes": 0,
    "percentage": 33
  }
}
```

### POST /configs/{id}/rollback
Rollback to previous version.

**Request:**
```json
{
  "to_version": "2.0.1",
  "reason": "Performance degradation detected"
}
```

### GET /configs/{id}/history
Get configuration version history.

**Response:**
```json
{
  "versions": [
    {
      "version": "2.1.0",
      "timestamp": "2024-01-20T14:22:00Z",
      "user": "engineer1",
      "change_summary": "Updated QoS settings",
      "deployment_status": "current"
    },
    {
      "version": "2.0.1",
      "timestamp": "2024-01-18T11:00:00Z",
      "user": "admin",
      "change_summary": "Added new topic",
      "deployment_status": "rollback_point"
    }
  ]
}
```

### GET /configs/{id}/diff
Compare configuration versions.

**Query Parameters:**
- `from_version` (string): Base version
- `to_version` (string): Target version

**Response:**
```json
{
  "from_version": "2.0.1",
  "to_version": "2.1.0",
  "changes": [
    {
      "type": "modified",
      "section": "qos_profiles",
      "path": "qos_profiles.sensor_qos.reliability",
      "old_value": "BEST_EFFORT",
      "new_value": "RELIABLE",
      "hot_reloadable": true
    },
    {
      "type": "added",
      "section": "topics",
      "path": "topics.new_sensor_data",
      "hot_reloadable": true
    }
  ],
  "summary": {
    "added": 1,
    "modified": 1,
    "removed": 0,
    "hot_reloadable": true
  }
}
```

## Domain Management

### GET /domains
List DDS domains.

### POST /domains
Create new domain.

### GET /domains/{id}
Get domain details.

### PUT /domains/{id}
Update domain.

### DELETE /domains/{id}
Delete domain.

### GET /domains/{id}/participants
List participants in domain.

### GET /domains/{id}/statistics
Get domain statistics.

**Response:**
```json
{
  "domain_id": 0,
  "participant_count": 5,
  "topic_count": 12,
  "publication_count": 8,
  "subscription_count": 15,
  "traffic": {
    "messages_per_second": 1250,
    "bytes_per_second": 5242880,
    "average_latency_ms": 0.5
  },
  "health": {
    "status": "healthy",
    "issues": []
  }
}
```

## Topic Management

### GET /topics
List topics.

### POST /topics
Create new topic.

### GET /topics/{id}
Get topic details.

**Response:**
```json
{
  "id": "topic_sensor123",
  "name": "VehicleSpeed",
  "type_name": "VehicleDynamics",
  "domain_id": 0,
  "qos_profile": "sensor_qos",
  "statistics": {
    "publishers": 1,
    "subscribers": 3,
    "message_rate": 100,
    "average_latency_ms": 0.3,
    "dropped_messages": 0
  }
}
```

### PUT /topics/{id}
Update topic.

### DELETE /topics/{id}
Delete topic.

## QoS Management

### GET /qos-profiles
List QoS profiles.

### POST /qos-profiles
Create QoS profile.

### GET /qos-profiles/{id}
Get QoS profile.

### PUT /qos-profiles/{id}
Update QoS profile.

### DELETE /qos-profiles/{id}
Delete QoS profile.

## Security Management

### GET /security/certificates
List certificates.

### POST /security/certificates
Upload/generate certificate.

### GET /security/certificates/{id}
Get certificate details.

### POST /security/certificates/{id}/revoke
Revoke certificate.

### GET /security/governance
Get security governance document.

### PUT /security/governance
Update governance document.

### GET /security/permissions
Get permissions document.

### PUT /security/permissions
Update permissions document.

### POST /security/permissions/validate
Validate permissions configuration.

## Automotive Configuration

### GET /automotive/ecus
List ECU configurations.

### GET /automotive/ecus/{id}
Get ECU configuration.

**Response:**
```json
{
  "ecu_id": "ECU_ADAS_01",
  "cluster_name": "ADAS_Cluster",
  "autosar_adaptive": {
    "enabled": true,
    "services": [...]
  },
  "tsn": {
    "enabled": true,
    "scheduler": "TAS",
    "gcl_configs": [...]
  },
  "safety": {
    "asil_level": "ASIL-D",
    "e2e_protection": {
      "profile": "PROFILE_2",
      "enabled": true
    }
  }
}
```

### PUT /automotive/ecus/{id}
Update ECU configuration.

### GET /automotive/tsn/streams
List TSN streams.

### POST /automotive/tsn/streams
Create TSN stream.

### GET /automotive/tsn/schedules
Get TSN schedules.

### POST /automotive/tsn/schedules/validate
Validate TSN schedule.

## Code Generation

### POST /generate/idl
Generate IDL from configuration.

**Request:**
```json
{
  "config_id": "conf_abc123",
  "options": {
    "namespace": "automotive",
    "generate_comments": true
  }
}
```

**Response:**
```json
{
  "download_url": "/api/v1/download/idl_gen_123.idl",
  "expires_at": "2024-01-21T10:00:00Z"
}
```

### POST /generate/c-code
Generate C code.

### POST /generate/cpp-code
Generate C++ code.

### POST /generate/arxml
Generate AUTOSAR ARXML.

### POST /generate/python
Generate Python bindings.

## Monitoring & Metrics

### GET /metrics
Get system metrics.

**Response:**
```json
{
  "timestamp": "2024-01-21T09:20:00Z",
  "system": {
    "cpu_usage_percent": 15.5,
    "memory_usage_mb": 512,
    "disk_usage_mb": 2048
  },
  "dds": {
    "total_domains": 5,
    "total_participants": 25,
    "total_topics": 45,
    "messages_per_second": 5000
  },
  "network": {
    "bytes_sent_per_second": 10485760,
    "bytes_received_per_second": 5242880
  }
}
```

### GET /metrics/domains/{id}
Get domain-specific metrics.

### GET /metrics/topics/{id}
Get topic-specific metrics.

### GET /logs
Get system logs.

**Query Parameters:**
- `level` (string): Log level filter (debug, info, warn, error)
- `from` (timestamp): Start time
- `to` (timestamp): End time
- `component` (string): Component filter
- `search` (string): Search query

**Response:**
```json
{
  "logs": [
    {
      "timestamp": "2024-01-21T09:20:15.123Z",
      "level": "INFO",
      "component": "ConfigManager",
      "message": "Configuration reloaded successfully",
      "context": {
        "config_id": "conf_abc123",
        "version": "2.1.0"
      }
    }
  ],
  "pagination": {...}
}
```

## WebSocket API

### Connection

```
ws://localhost:8080/ws
```

**Authentication:**
Pass JWT token in connection query parameter:
```
ws://localhost:8080/ws?token=<jwt_token>
```

### Message Format

All WebSocket messages use JSON format:

```json
{
  "type": "message_type",
  "timestamp": "2024-01-21T09:20:00Z",
  "payload": {...}
}
```

### Subscribe to Events

**Client → Server:**
```json
{
  "type": "subscribe",
  "payload": {
    "events": ["config_changed", "metrics_update", "alert"],
    "filter": {
      "domain_id": [0, 1],
      "topic_id": ["topic_123"]
    }
  }
}
```

**Server → Client:**
```json
{
  "type": "subscribed",
  "timestamp": "2024-01-21T09:20:00Z",
  "payload": {
    "subscription_id": "sub_abc123",
    "events": ["config_changed", "metrics_update", "alert"]
  }
}
```

### Configuration Change Events

**Server → Client:**
```json
{
  "type": "config_changed",
  "timestamp": "2024-01-21T09:20:05Z",
  "payload": {
    "config_id": "conf_abc123",
    "version": "2.1.1",
    "change_type": "hot_reload",
    "changes": [
      {
        "section": "qos_profiles",
        "path": "sensor_qos.history.depth",
        "old_value": "10",
        "new_value": "20"
      }
    ]
  }
}
```

### Real-time Metrics

**Server → Client:**
```json
{
  "type": "metrics_update",
  "timestamp": "2024-01-21T09:20:10Z",
  "payload": {
    "domain_id": 0,
    "metrics": {
      "messages_per_second": 1250,
      "latency_ms": 0.45,
      "dropped_messages": 0
    }
  }
}
```

### Alerts

**Server → Client:**
```json
{
  "type": "alert",
  "timestamp": "2024-01-21T09:20:15Z",
  "payload": {
    "severity": "warning",
    "code": "LATENCY_HIGH",
    "message": "Topic latency exceeded threshold",
    "details": {
      "topic_id": "topic_sensor123",
      "current_latency_ms": 5.2,
      "threshold_ms": 5.0
    }
  }
}
```

### Command Execution

**Client → Server:**
```json
{
  "type": "command",
  "payload": {
    "command": "reload_config",
    "args": {
      "config_id": "conf_abc123",
      "strategy": "hot_reload"
    },
    "request_id": "req_xyz789"
  }
}
```

**Server → Client:**
```json
{
  "type": "command_response",
  "timestamp": "2024-01-21T09:20:20Z",
  "payload": {
    "request_id": "req_xyz789",
    "status": "success",
    "result": {
      "new_version": "2.1.2",
      "applied_at": "2024-01-21T09:20:20Z"
    }
  }
}
```

## Error Responses

All errors follow this format:

```json
{
  "error": {
    "code": "E001",
    "message": "Configuration validation failed",
    "details": {
      "field": "domains[0].domain_id",
      "issue": "Domain ID out of range (0-230)"
    },
    "timestamp": "2024-01-21T09:20:00Z",
    "request_id": "req_abc123"
  }
}
```

### Error Codes

| Code | Description |
|------|-------------|
| E001 | Validation failed |
| E002 | Resource not found |
| E003 | Permission denied |
| E004 | Rate limit exceeded |
| E005 | Internal server error |
| E006 | Service unavailable |
| E007 | Conflict - resource already exists |
| E008 | Invalid request format |
| E009 | Authentication required |
| E010 | Token expired |

## Rate Limiting

API requests are rate-limited:

- **Standard:** 100 requests per minute
- **Authenticated:** 1000 requests per minute
- **WebSocket:** 100 messages per minute

Rate limit headers:
```
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1642755600
```

## Pagination

List endpoints support cursor-based pagination:

**Request:**
```
GET /configs?page=2&limit=50
```

**Response:**
```json
{
  "data": [...],
  "pagination": {
    "page": 2,
    "limit": 50,
    "total": 156,
    "pages": 4,
    "has_next": true,
    "has_prev": true,
    "next_cursor": "eyJpZCI6MTAwfQ==",
    "prev_cursor": "eyJpZCI6MX0="
  }
}
```

## Filtering

Use query parameters for filtering:

```
GET /configs?status=active&domain_id=0&tags=production,automotive
```

Complex filters using JSON:

```
GET /configs?filter={"status":{"in":["active","draft"]},"created_at":{"gte":"2024-01-01"}}
```

## Sorting

```
GET /configs?sort=-created_at,name
```

Prefix with `-` for descending order.

## Versioning

API version is specified in URL:
- `/api/v1/` - Current stable version
- `/api/v2/` - Future version (when released)

Deprecated versions will be supported for 6 months after deprecation notice.

## CORS

CORS is enabled for configured origins:

```
Access-Control-Allow-Origin: https://dds-config-ui.example.com
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization
Access-Control-Max-Age: 86400
```
