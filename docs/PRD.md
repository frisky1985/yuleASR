# Product Requirements Document (PRD)
# ETH-DDS Integration - UDS Diagnostic Services

**Version**: 2.0.0  
**Last Updated**: 2026-04-29  
**Status**: Active  

---

## Table of Contents

1. [Overview](#1-overview)
2. [UDS Service Requirements](#2-uds-service-requirements)
3. [0x23 Read Memory By Address](#3-0x23-read-memory-by-address)
4. [0x3D Write Memory By Address](#4-0x3d-write-memory-by-address)
5. [Security Requirements](#5-security-requirements)
6. [Error Handling](#6-error-handling)
7. [Compliance](#7-compliance)

---

## 1. Overview

This document specifies the product requirements for UDS (Unified Diagnostic Services) implementation in the ETH-DDS Integration Framework, focusing on memory access services 0x23 (Read Memory By Address) and 0x3D (Write Memory By Address).

### 1.1 Scope

- UDS memory access services implementation
- Security and access control for memory operations
- Integration with AUTOSAR DCM (Diagnostic Communication Manager)
- ISO 14229-1:2020 compliance

### 1.2 References

- ISO 14229-1:2020 - Road vehicles - Unified diagnostic services (UDS)
- AUTOSAR R22-11 - DCM Specification
- AUTOSAR R22-11 - DEM Specification
- MISRA C:2012 Guidelines

---

## 2. UDS Service Requirements

### 2.1 Implemented Services

| Service ID | Service Name | Status | ISO Reference |
|------------|--------------|--------|---------------|
| 0x10 | Diagnostic Session Control | Implemented | 10.2 |
| 0x11 | ECU Reset | Implemented | 10.3 |
| 0x14 | Clear Diagnostic Information | Implemented | 10.5 |
| 0x19 | Read DTC Information | Implemented | 10.6 |
| 0x22 | Read Data By Identifier | Implemented | 10.7 |
| **0x23** | **Read Memory By Address** | **Implemented** | **10.4** |
| 0x27 | Security Access | Implemented | 10.8 |
| 0x28 | Communication Control | Implemented | 10.9 |
| 0x2C | Dynamically Define Data Identifier | Implemented | 10.11 |
| 0x2E | Write Data By Identifier | Implemented | 10.13 |
| 0x2F | Input Output Control By Identifier | Implemented | 10.14 |
| 0x31 | Routine Control | Implemented | 10.15 |
| 0x34 | Request Download | Implemented | 10.16 |
| 0x35 | Request Upload | Implemented | 10.17 |
| 0x36 | Transfer Data | Implemented | 10.18 |
| 0x37 | Request Transfer Exit | Implemented | 10.19 |
| 0x3D | Write Memory By Address | Implemented | 10.18 |
| 0x3E | Tester Present | Implemented | 10.10 |
| 0x85 | Control DTC Setting | Implemented | 10.20 |

### 2.2 Service Coverage

- **Total Services**: 19/20 (95%)
- **Remaining**: 0x24 Read Scaling Data By Identifier (low priority)

---

## 3. 0x23 Read Memory By Address

### 3.1 Service Description

The Read Memory By Address service (0x23) allows the client to request the server to read data from a specific memory address range. This service is essential for:

- Reading calibration data
- Accessing runtime variables
- Debugging and diagnostics
- Software verification

### 3.2 Request Message Format

| Byte | Parameter Name | Description |
|------|----------------|-------------|
| 0 | Service ID | 0x23 |
| 1 | addressAndLengthFormatIdentifier | High nibble: address length, Low nibble: size length |
| 2..n | memoryAddress[] | Memory address (1-4 bytes, MSB first) |
| n+1..m | memorySize[] | Read size (1-4 bytes, MSB first) |

#### Address/Length Format Identifier Encoding

| Nibble Value | Address/Size Length |
|--------------|---------------------|
| 0x1 | 1 byte |
| 0x2 | 2 bytes |
| 0x4 | 4 bytes |

**Format Identifier Examples:**
- 0x11: 1-byte address, 1-byte size
- 0x12: 1-byte address, 2-byte size
- 0x21: 2-byte address, 1-byte size
- 0x22: 2-byte address, 2-byte size
- 0x44: 4-byte address, 4-byte size

### 3.3 Positive Response Format

| Byte | Parameter Name | Description |
|------|----------------|-------------|
| 0 | Response Service ID | 0x63 (0x23 + 0x40) |
| 1..n | dataRecord[] | Read data bytes |

### 3.4 Minimum Request Length

- **Minimum**: 4 bytes (SID + formatId + 1-byte address + 1-byte size)
- **Maximum**: 10 bytes (SID + formatId + 4-byte address + 4-byte size)

### 3.5 Maximum Data Length

- **Maximum read size**: 4095 bytes (0xFFF) per request
- This limit ensures response fits in single-frame ISO-TP messages

### 3.6 Address Format Support

The implementation supports flexible address formats:

| Address Length | Range | Use Case |
|----------------|-------|----------|
| 1 byte | 0x00 - 0xFF | Small address spaces, EEPROM |
| 2 bytes | 0x0000 - 0xFFFF | Medium address spaces |
| 4 bytes | 0x00000000 - 0xFFFFFFFF | Full 32-bit addressing |

### 3.7 Supported Memory Regions

| Region Type | Start Address | End Address | Access |
|-------------|---------------|-------------|--------|
| RAM | 0x20000000 | 0x2001FFFF | Read/Write |
| Flash | 0x08000000 | 0x0807FFFF | Read/Write* |

*Flash writes require erase operation and higher security level

---

## 4. 0x3D Write Memory By Address

### 4.1 Service Description

The Write Memory By Address service (0x3D) allows the client to write data to a specific memory address range. Used for:

- Flash programming
- Calibration updates
- Configuration changes

### 4.2 Request Message Format

| Byte | Parameter Name | Description |
|------|----------------|-------------|
| 0 | Service ID | 0x3D |
| 1 | addressAndLengthFormatIdentifier | Address/size length encoding |
| 2..n | memoryAddress[] | Target memory address |
| n+1..m | memorySize[] | Write size |
| m+1..p | dataRecord[] | Data to write |

### 4.3 Positive Response Format

| Byte | Parameter Name | Description |
|------|----------------|-------------|
| 0 | Response Service ID | 0x7D (0x3D + 0x40) |
| 1 | addressAndLengthFormatIdentifier | Echo of request format |
| 2..n | memoryAddress[] | Echo of request address |
| n+1..m | memorySize[] | Echo of request size |

---

## 5. Security Requirements

### 5.1 Access Control Levels

| Security Level | Description | Memory Access |
|----------------|-------------|---------------|
| 0 | Locked | Read-only (public regions) |
| 1 | Basic Unlock | RAM read/write |
| 2 | Advanced Unlock | Extended memory access |
| 3 | Programming | Flash read/write |

### 5.2 Memory Region Protection

Each memory region has configurable security requirements:

```c
typedef struct {
    uint32_t startAddress;
    uint32_t endAddress;
    uint8_t requiredSecurityLevel;  // Minimum level required
    bool writeAllowed;
    bool readAllowed;
    bool eraseRequired;             // For flash regions
    uint32_t alignment;             // Write alignment requirement
} Dcm_MemoryRegionConfigType;
```

### 5.3 Session Requirements

Memory write operations may require Programming Session:

```c
typedef struct {
    bool requireProgrammingSession;  // If true, requires session 0x02
    uint8_t requiredSecurityLevel;
    // ... other config
} Dcm_MemoryWriteConfigType;
```

### 5.4 Boundary Validation

- Reads/writes cannot cross region boundaries
- Address + size must be within region bounds
- Alignment requirements must be met for writes

---

## 6. Error Handling

### 6.1 Negative Response Codes (NRC)

#### Common NRCs for Memory Services

| NRC | Name | Trigger Condition |
|-----|------|-------------------|
| 0x13 | Incorrect Message Length | Request too short or too long |
| 0x22 | Conditions Not Correct | Module not initialized |
| 0x31 | Request Out Of Range | Invalid address, format, or size |
| 0x33 | Security Access Denied | Security level insufficient |
| 0x7E | Subfunction Not Supported In Session | Session does not allow operation |
| 0x14 | Response Too Long | Read size exceeds buffer |

#### 0x23 Specific NRCs

| NRC | Condition |
|-----|-----------|
| 0x13 | Request length < 4 bytes |
| 0x31 | Invalid format identifier |
| 0x31 | Address not in valid region |
| 0x31 | Size exceeds maximum (4095 bytes) |
| 0x33 | Region not readable |
| 0x33 | Security level too low for region |
| 0x14 | Response buffer too small for data |

#### 0x3D Specific NRCs

| NRC | Condition |
|-----|-----------|
| 0x13 | Data length doesn't match size parameter |
| 0x31 | Write to non-writable region |
| 0x31 | Address not aligned to requirement |
| 0x33 | Flash programming without proper security |
| 0x72 | General Programming Failure | Write operation failed |

### 6.2 Error Response Format

| Byte | Parameter | Value |
|------|-----------|-------|
| 0 | Negative Response Service ID | 0x7F |
| 1 | Original Request Service ID | 0x23 or 0x3D |
| 2 | Negative Response Code | NRC value |

---

## 7. Compliance

### 7.1 ISO 14229-1:2020 Compliance

| Section | Description | Status |
|---------|-------------|--------|
| 10.4 | Read Memory By Address | Full compliance |
| 10.18 | Write Memory By Address | Full compliance |
| 7.2 | Address/Length Format Identifier | Full compliance |
| 7.5 | Negative Response Codes | Full compliance |

### 7.2 AUTOSAR R22-11 Compliance

| Module | Requirement | Status |
|--------|-------------|--------|
| DCM | SWS_Dcm_00001 | Compliant |
| DCM | SWS_Dcm_00002 | Compliant |
| DCM | Memory access API | Compliant |

### 7.3 MISRA C:2012 Compliance

- Rule violations: 0
- Deviations documented: Yes
- Static analysis passed: Yes

### 7.4 Safety Level

- **ASIL-D** compliant implementation
- Memory protection mechanisms
- Bounds checking on all operations
- Safe default behaviors

---

## Appendix A: Example Messages

### A.1 0x23 Read Memory Example

**Request - Read 4 bytes from address 0x20000000:**
```
0x23, 0x44, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04
SID   Fmt   Address (4 bytes, MSB)          Size (4 bytes)
```

**Positive Response - 4 bytes of data:**
```
0x63, 0xAA, 0xBB, 0xCC, 0xDD
SID   Data bytes
```

**Negative Response - Security Denied:**
```
0x7F, 0x23, 0x33
NR    SID   NRC
```

### A.2 0x3D Write Memory Example

**Request - Write 2 bytes to address 0x20000000:**
```
0x3D, 0x44, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xAA, 0xBB
SID   Fmt   Address (4 bytes)               Size (4 bytes)    Data
```

---

*Document maintained by ETH-DDS Integration Team*  
*Version 2.0.0 - April 2026*
