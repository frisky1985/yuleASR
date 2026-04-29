# UDS Services Documentation

**Version**: 2.0.0  
**Last Updated**: 2026-04-29  
**ISO Reference**: ISO 14229-1:2020

---

## Table of Contents

1. [Overview](#1-overview)
2. [Service Summary](#2-service-summary)
3. [Diagnostic and Communication Management](#3-diagnostic-and-communication-management)
4. [Data Transmission](#4-data-transmission)
5. [Stored Data Transmission](#5-stored-data-transmission)
6. [Input/Output Control](#6-inputoutput-control)
7. [Remote Activation of Routine](#7-remote-activation-of-routine)
8. [Upload/Download](#8-uploaddownload)
9. [0x23 Read Memory By Address - Detailed](#9-0x23-read-memory-by-address---detailed)
10. [0x3D Write Memory By Address - Detailed](#10-0x3d-write-memory-by-address---detailed)

---

## 1. Overview

This document provides comprehensive documentation for all UDS (Unified Diagnostic Services) services implemented in the ETH-DDS Integration Framework.

### 1.1 Implementation Status

- **Total Services Implemented**: 19/20 (95%)
- **Fully Tested**: 19 services
- **Production Ready**: Yes
- **ISO 14229-1:2020 Compliant**: Yes

---

## 2. Service Summary

### 2.1 Implementation Matrix

| SID | Service Name | Status | Test Coverage | Security Required |
|-----|--------------|--------|---------------|-------------------|
| **Diagnostic and Communication Management** |
| 0x10 | Diagnostic Session Control | Implemented | 100% | No |
| 0x11 | ECU Reset | Implemented | 100% | Depends on reset type |
| 0x27 | Security Access | Implemented | 100% | Yes |
| 0x28 | Communication Control | Implemented | 100% | Level 1 |
| 0x3E | Tester Present | Implemented | 100% | No |
| 0x85 | Control DTC Setting | Implemented | 100% | Level 1 |
| **Data Transmission** |
| 0x22 | Read Data By Identifier | Implemented | 100% | No |
| **0x23** | **Read Memory By Address** | **Implemented** | **100%** | **Level 1+** |
| 0x2C | Dynamically Define Data Identifier | Implemented | 100% | Level 1 |
| 0x2E | Write Data By Identifier | Implemented | 100% | Level 1 |
| 0x2F | Input Output Control By Identifier | Implemented | 100% | Level 2 |
| 0x3D | Write Memory By Address | Implemented | 100% | Level 3 |
| **Stored Data Transmission** |
| 0x14 | Clear Diagnostic Information | Implemented | 100% | Level 1 |
| 0x19 | Read DTC Information | Implemented | 100% | No |
| **Remote Activation of Routine** |
| 0x31 | Routine Control | Implemented | 100% | Level 2 |
| **Upload/Download** |
| 0x34 | Request Download | Implemented | 100% | Level 3 |
| 0x35 | Request Upload | Implemented | 100% | Level 3 |
| 0x36 | Transfer Data | Implemented | 100% | Level 3 |
| 0x37 | Request Transfer Exit | Implemented | 100% | Level 3 |
| **Not Implemented** |
| 0x24 | Read Scaling Data By Identifier | Not Planned | N/A | N/A |

### 2.2 Session Support

| Service | Default | Programming | Extended | Safety |
|---------|---------|-------------|----------|--------|
| 0x10 | Yes | Yes | Yes | Yes |
| 0x11 | Yes | Yes | Yes | Yes |
| 0x23 | No | Yes | Yes | No |
| 0x27 | Yes | Yes | Yes | Yes |
| 0x3D | No | Yes | No | No |
| 0x34-0x37 | No | Yes | No | No |

---

## 3. Diagnostic and Communication Management

### 0x10 - Diagnostic Session Control

**Purpose**: Change diagnostic session

**Request**: `0x10 sessionType`

**Subfunctions**:
- 0x01: Default Session
- 0x02: Programming Session
- 0x03: Extended Diagnostic Session
- 0x04: Safety System Diagnostic Session

**Response**: `0x50 sessionType p2 p2*`

### 0x11 - ECU Reset

**Purpose**: Reset the ECU

**Request**: `0x11 resetType`

**Subfunctions**:
- 0x01: Hard Reset
- 0x02: Key Off/On Reset
- 0x03: Soft Reset
- 0x04: Enable Rapid Power Shutdown
- 0x05: Disable Rapid Power Shutdown

**Response**: `0x51 resetType`

### 0x27 - Security Access

**Purpose**: Unlock secured services

**Request**: `0x27 subFunction [data]`

**Subfunctions**:
- 0x01, 0x03, 0x05...: Request Seed
- 0x02, 0x04, 0x06...: Send Key

**Response**: `0x67 subFunction [seed]`

### 0x28 - Communication Control

**Purpose**: Enable/disable communication

**Request**: `0x28 controlType communicationType`

### 0x3E - Tester Present

**Purpose**: Keep session active

**Request**: `0x3E [suppressResponse]`

**Response**: `0x7E [suppressResponse]`

### 0x85 - Control DTC Setting

**Purpose**: Enable/disable DTC detection

**Request**: `0x85 settingType`

**Subfunctions**:
- 0x01: ON
- 0x02: OFF

---

## 4. Data Transmission

### 0x22 - Read Data By Identifier

**Purpose**: Read data by DID

**Request**: `0x22 DID_H DID_L`

**Response**: `0x62 DID_H DID_L data...`

### 0x23 - Read Memory By Address

See [Section 9](#9-0x23-read-memory-by-address---detailed) for detailed documentation.

### 0x2C - Dynamically Define Data Identifier

**Purpose**: Create DIDs at runtime

**Request**: `0x2C definitionMode DID_H DID_L ...`

### 0x2E - Write Data By Identifier

**Purpose**: Write data by DID

**Request**: `0x2E DID_H DID_L data...`

**Response**: `0x6E DID_H DID_L`

### 0x2F - Input Output Control By Identifier

**Purpose**: Control I/O pins

**Request**: `0x2F DID_H DID_L controlOption [controlState] [mask]`

### 0x3D - Write Memory By Address

See [Section 10](#10-0x3d-write-memory-by-address---detailed) for detailed documentation.

---

## 5. Stored Data Transmission

### 0x14 - Clear Diagnostic Information

**Purpose**: Clear DTCs

**Request**: `0x14 groupOfDTC_B1 groupOfDTC_B2 groupOfDTC_B3`

**Response**: `0x54`

### 0x19 - Read DTC Information

**Purpose**: Read DTC status

**Request**: `0x19 reportType [parameters]`

**Subfunctions**:
- 0x01: Report Number Of DTC By Status Mask
- 0x02: Report DTC By Status Mask
- 0x06: Report DTC Extended Data Record By DTC Number
- 0x0A: Report Supported DTC

---

## 6. Input/Output Control

### 0x2F - Input Output Control By Identifier

**Purpose**: Control I/O ports

**Request**: `0x2F DID_H DID_L inputOutputControlParameter [controlState] [mask]`

**Control Parameters**:
- 0x00: Return Control To ECU
- 0x01: Reset To Default
- 0x02: Freeze Current State
- 0x03: Short Term Adjustment

---

## 7. Remote Activation of Routine

### 0x31 - Routine Control

**Purpose**: Execute predefined routines

**Request**: `0x31 routineControlType routineIdentifier_H routineIdentifier_L [routineControlOptionRecord]`

**Control Types**:
- 0x01: Start Routine
- 0x02: Stop Routine
- 0x03: Request Routine Results

---

## 8. Upload/Download

### 0x34 - Request Download

**Purpose**: Initiate download to ECU

**Request**: `0x34 dataFormatIdentifier addressAndLengthFormatIdentifier address[] size[]`

### 0x35 - Request Upload

**Purpose**: Initiate upload from ECU

**Request**: `0x35 dataFormatIdentifier addressAndLengthFormatIdentifier address[] size[]`

### 0x36 - Transfer Data

**Purpose**: Transfer data block

**Request**: `0x36 blockSequenceCounter [data...]`

**Response**: `0x76 blockSequenceCounter [data...]`

### 0x37 - Request Transfer Exit

**Purpose**: Complete transfer

**Request**: `0x37`

**Response**: `0x77`

---

## 9. 0x23 Read Memory By Address - Detailed

### 9.1 Service Description

The Read Memory By Address service allows reading raw memory data from specified addresses. This service is critical for:

- Debugging and diagnostics
- Reading calibration data
- Software verification
- Runtime variable inspection

### 9.2 Request Format

```
+--------+----------+-----------------+----------------+
|  Byte  |    0     |       1         |    2..n        |
+--------+----------+-----------------+----------------+
|  Field |   SID    |    Format ID    |    Address[]   |
|        |  (0x23)  | (addrLen|sizeLen|                |
+--------+----------+-----------------+----------------+

Bytes n+1..m: Size[] (1-4 bytes)
```

### 9.3 Format Identifier Encoding

| Format Byte | Address Length | Size Length | Example Use |
|-------------|----------------|-------------|-------------|
| 0x11 | 1 byte | 1 byte | Small EEPROM |
| 0x12 | 1 byte | 2 bytes | EEPROM with larger read |
| 0x21 | 2 bytes | 1 byte | 16-bit address space |
| 0x22 | 2 bytes | 2 bytes | 16-bit with larger read |
| 0x44 | 4 bytes | 4 bytes | Full 32-bit addressing |

### 9.4 Address Format Examples

| Format | Address Bytes | Size Bytes | Min Request | Max Address | Max Size |
|--------|---------------|------------|-------------|-------------|----------|
| 0x11 | 1 | 1 | 4 bytes | 0xFF | 255 |
| 0x12 | 1 | 2 | 5 bytes | 0xFF | 65535 |
| 0x21 | 2 | 1 | 5 bytes | 0xFFFF | 255 |
| 0x22 | 2 | 2 | 6 bytes | 0xFFFF | 65535 |
| 0x44 | 4 | 4 | 10 bytes | 0xFFFFFFFF | 4095* |

*Max 4095 bytes due to ISO-TP single frame limit

### 9.5 Positive Response

```
+--------+----------+----------------+
|  Byte  |    0     |      1..n      |
+--------+----------+----------------+
|  Field | Response |   Data Record  |
|        |  (0x63)  |                |
+--------+----------+----------------+
```

### 9.6 Negative Response Codes

| NRC | Name | Condition |
|-----|------|-----------|
| 0x13 | Incorrect Message Length | Request < 4 bytes or format error |
| 0x22 | Conditions Not Correct | DCM not initialized |
| 0x31 | Request Out Of Range | Invalid format, address, or size |
| 0x33 | Security Access Denied | Insufficient security level |
| 0x7E | Subfunction Not Supported In Session | Session doesn't support service |
| 0x14 | Response Too Long | Read size exceeds buffer capacity |

### 9.7 Memory Region Access

| Region | Start | End | Read | Write | Security |
|--------|-------|-----|------|-------|----------|
| RAM | 0x20000000 | 0x2001FFFF | Yes | Yes* | Level 1 |
| Flash | 0x08000000 | 0x0807FFFF | Yes | No | Level 1 |
| Registers | Configurable | Configurable | Configurable | Configurable | Configurable |

*Write requires programming session for Flash

### 9.8 API Reference

```c
/**
 * @brief Process ReadMemoryByAddress (0x23) service request
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 * @requirement ISO 14229-1:2020 10.4
 */
Dcm_ReturnType Dcm_ReadMemoryByAddress(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Read data from memory address
 * @param memoryAddress Source memory address
 * @param data Buffer to store read data
 * @param length Data length to read
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ReadMemory(
    uint32_t memoryAddress,
    uint8_t *data,
    uint32_t length
);

/**
 * @brief Check if memory address is readable
 * @param memoryAddress Memory address to check
 * @param length Read length
 * @return bool True if readable
 */
bool Dcm_IsMemoryAddressReadable(uint32_t memoryAddress, uint32_t length);
```

### 9.9 Example Usage

```c
// Example: Read 4 bytes from address 0x20000000
uint8_t request[] = {
    0x23,        // SID
    0x44,        // Format: 4-byte address, 4-byte size
    0x20, 0x00, 0x00, 0x00,  // Address: 0x20000000
    0x00, 0x00, 0x00, 0x04   // Size: 4 bytes
};

// Expected response:
uint8_t response[] = {
    0x63,        // Response SID
    0xAA, 0xBB, 0xCC, 0xDD  // Data
};
```

---

## 10. 0x3D Write Memory By Address - Detailed

### 10.1 Service Description

The Write Memory By Address service allows writing data to specified memory addresses. Used for:

- Flash programming
- Calibration updates
- Configuration data modification

### 10.2 Request Format

```
+--------+----------+-----------------+----------------+----------------+
|  Byte  |    0     |       1         |    2..n        |    n+1..m      |
+--------+----------+-----------------+----------------+----------------+
|  Field |   SID    |    Format ID    |    Address[]   |    Size[]      |
|        |  (0x3D)  | (addrLen|sizeLen|                |                |
+--------+----------+-----------------+----------------+----------------+

Bytes m+1..p: Data Record[]
```

### 10.3 Positive Response

```
+--------+----------+-----------------+----------------+----------------+
|  Byte  |    0     |       1         |    2..n        |    n+1..m      |
+--------+----------+-----------------+----------------+----------------+
|  Field | Response |    Format ID    |    Address[]   |    Size[]      |
|        |  (0x7D)  |   (echo)        |    (echo)      |    (echo)      |
+--------+----------+-----------------+----------------+----------------+
```

### 10.4 Negative Response Codes

| NRC | Name | Condition |
|-----|------|-----------|
| 0x13 | Incorrect Message Length | Data length mismatch |
| 0x31 | Request Out Of Range | Write to non-writable region |
| 0x31 | Request Out Of Range | Address not aligned |
| 0x33 | Security Access Denied | Flash write without proper security |
| 0x72 | General Programming Failure | Write operation failed |
| 0x7E | Subfunction Not Supported In Session | Wrong session |

### 10.5 API Reference

```c
/**
 * @brief Process WriteMemoryByAddress (0x3D) service request
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 * @requirement ISO 14229-1:2020 10.18
 */
Dcm_ReturnType Dcm_WriteMemoryByAddress(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Write data to memory address
 * @param memoryAddress Target memory address
 * @param data Data to write
 * @param length Data length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_WriteMemory(
    uint32_t memoryAddress,
    const uint8_t *data,
    uint32_t length
);
```

---

## Appendix A: NRC Quick Reference

| NRC | Name | Description |
|-----|------|-------------|
| 0x10 | General Reject | Request rejected |
| 0x11 | Service Not Supported | Unknown service |
| 0x12 | Subfunction Not Supported | Unknown subfunction |
| 0x13 | Incorrect Message Length | Wrong message size |
| 0x14 | Response Too Long | Response exceeds buffer |
| 0x22 | Conditions Not Correct | Wrong state |
| 0x31 | Request Out Of Range | Invalid parameters |
| 0x33 | Security Access Denied | Not unlocked |
| 0x35 | Invalid Key | Wrong key |
| 0x36 | Exceed Number Of Attempts | Too many attempts |
| 0x37 | Required Time Delay Not Expired | Delay required |
| 0x7E | Subfunction Not Supported In Session | Wrong session |
| 0x7F | Service Not Supported In Session | Wrong session |

---

*Document maintained by ETH-DDS Integration Team*  
*Version 2.0.0 - April 2026*
