# DDS-Security Module

## Overview

This module implements the OMG DDS-Security specification v1.1 for secure DDS communications in automotive environments.

## Features

### 1. Identity Authentication (DDS:Auth:PKI-DH)

- **X.509 Certificate Management**: Load, validate, and manage certificates
- **Diffie-Hellman Key Exchange**: Secure key establishment using RFC 3526 groups
- **Handshake Protocol**: Three-way handshake for mutual authentication
- **Digital Signatures**: SHA-256 based signing and verification

### 2. Access Control (DDS:Access:Permissions)

- **Permission XML Parsing**: Load and parse governance and permissions XML
- **Topic-level Access Control**: Fine-grained publish/subscribe permissions
- **Pattern Matching**: Wildcard-based topic name matching
- **ASIL Level Support**: Integration with ISO 26262 safety levels

### 3. Encryption (DDS:Crypto:AES-GCM-GMAC)

- **AES-256-GCM Encryption**: Authenticated encryption for data confidentiality
- **AES-128-GCM**: Lightweight option for resource-constrained systems
- **GMAC**: Message authentication for data integrity
- **Session Management**: Secure session establishment and key rotation
- **Replay Protection**: Sequence number based replay attack detection

### 4. Security Manager

- **Unified Security Management**: Coordinates authentication, access control, and encryption
- **Security Event Handling**: Event logging and callback mechanisms
- **Audit Logging**: Comprehensive security audit trail
- **Certificate Management**: Automatic expiry checking and renewal

## Architecture

```
DDS-Security
├── dds_auth.h/c          - Authentication module (PKI-DH)
├── dds_access.h/c        - Access control module (Permissions)
├── dds_crypto.h/c        - Cryptography module (AES-GCM-GMAC)
├── dds_security_manager.h/c - Central security manager
└── dds_security_types.h  - Common type definitions
```

## Usage

### Basic Initialization

```c
#include "dds_security_manager.h"

// Configure security
dds_security_config_t config = {
    .policy_flags = DDS_SEC_POLICY_ALL,
    .auth_plugin = DDS_AUTH_PLUGIN_PKIDH,
    .access_plugin = DDS_ACCESS_PLUGIN_PERMISSIONS,
    .crypto_plugin = DDS_CRYPTO_PLUGIN_AES_GCM_GMAC,
    .required_asil_level = DDS_SECURITY_ASIL_B,
    .identity_ca_cert_path = "/path/to/ca-cert.pem",
    .identity_cert_path = "/path/to/identity.pem",
    .private_key_path = "/path/to/private-key.pem",
    .permissions_xml_path = "/path/to/permissions.xml",
    .enable_audit_log = true
};

// Initialize security manager
dds_security_context_t *sec_ctx = dds_security_manager_init(&config);
if (!sec_ctx) {
    // Handle error
}
```

### Participant Registration

```c
rtps_guid_t participant_guid = { /* ... */ };

// Register participant
dds_security_status_t status = dds_security_register_participant(
    sec_ctx, &participant_guid, "CN=VehicleECU,O=Automotive");

if (status != DDS_SECURITY_OK) {
    // Handle error
}
```

### Data Protection

```c
// Protect (encrypt) outgoing data
uint8_t plaintext[] = "Sensitive vehicle data";
uint8_t protected_data[256];
uint32_t protected_len = sizeof(protected_data);

status = dds_security_protect_data(sec_ctx, &participant_guid,
                                   "Vehicle/Speed",
                                   plaintext, sizeof(plaintext),
                                   protected_data, &protected_len);

// Unprotect (decrypt) incoming data
uint8_t received_plaintext[256];
uint32_t plaintext_len = sizeof(received_plaintext);

status = dds_security_unprotect_data(sec_ctx, &participant_guid,
                                     &remote_guid,
                                     protected_data, protected_len,
                                     received_plaintext, &plaintext_len);
```

### Access Control

```c
// Check write permission
status = dds_security_check_access(sec_ctx, &participant_guid,
                                   0, "Vehicle/Speed",
                                   DDS_PERM_WRITE);

if (status != DDS_SECURITY_OK) {
    // Access denied
}
```

### Event Handling

```c
// Register event callback
void security_event_handler(dds_security_event_type_t event,
                            dds_security_severity_t severity,
                            const rtps_guid_t *guid,
                            const char *description,
                            void *user_data) {
    printf("Security Event [%d]: %s\n", severity, description);
}

dds_security_register_event_callback(sec_ctx, security_event_handler, NULL);
```

## Configuration

### Security Policies

| Flag | Description |
|------|-------------|
| `DDS_SEC_POLICY_AUTH` | Enable identity authentication |
| `DDS_SEC_POLICY_ACCESS_CONTROL` | Enable access control |
| `DDS_SEC_POLICY_ENCRYPTION` | Enable data encryption |
| `DDS_SEC_POLICY_INTEGRITY` | Enable data integrity protection |
| `DDS_SEC_POLICY_REPLAY_PROTECTION` | Enable replay attack protection |
| `DDS_SEC_POLICY_ALL` | Enable all security features |

### ASIL Levels

| Level | Description |
|-------|-------------|
| `DDS_SECURITY_ASIL_QM` | Quality Management (non-safety) |
| `DDS_SECURITY_ASIL_A` | ASIL A (lowest safety level) |
| `DDS_SECURITY_ASIL_B` | ASIL B (medium safety level) |
| `DDS_SECURITY_ASIL_C` | ASIL C (high safety level) |
| `DDS_SECURITY_ASIL_D` | ASIL D (highest safety level) |

## Building

```bash
mkdir -p build && cd build
cmake ..
make dds_security
```

### CMake Options

- `DDS_SECURITY_ENABLE_ASIL_D`: Enable ASIL-D safety features
- `DDS_SECURITY_ENABLE_FIPS_MODE`: Enable FIPS 140-2 compliance mode
- `BUILD_TESTING`: Build unit tests

## Testing

```bash
cd build
make test_dds_security
./src/dds/security/tests/test_dds_security
```

## Security Considerations

### Certificate Management

- Use properly signed certificates from a trusted CA
- Implement certificate revocation checking (CRL/OCSP)
- Set appropriate certificate validity periods
- Monitor certificate expiry and plan renewal

### Key Management

- Keys are automatically rotated based on configured interval
- Shared secrets are cleared from memory after use
- Session keys are derived using HKDF-like construction

### Replay Protection

- Sequence numbers are tracked in a sliding window
- Messages outside the window are rejected
- Window size is configurable (default: 64)

### Audit Logging

- All security events are logged
- File-based audit logging available
- Log rotation and size limits should be configured

## Compliance

This implementation targets compliance with:

- **OMG DDS-Security v1.1**: Core security specification
- **ISO/SAE 21434**: Cybersecurity engineering for road vehicles
- **ISO 26262**: Functional safety (ASIL levels)

## Limitations

- Simplified cryptographic implementation (not production-ready)
- Limited certificate chain validation
- No hardware security module (HSM) integration yet

## Future Enhancements

- Hardware security module (HSM) support
- FIPS 140-2 certified cryptographic library integration
- Certificate transparency logging
- Advanced intrusion detection
