---
title: Crypto API Reference Manual
description: "- Version: 1.0.0"
sidebar_position: 15
---

# Crypto API Reference Manual

**YuleTech AutoSAR Crypto Stack API Reference**

- Version: 1.0.0
- Date: 2026-05-01
- Standard: AUTOSAR Classic Platform R20-11
- Company: Shanghai Yule Electronics Technology Co., Ltd.

***

## Table of Contents

1. [Introduction](#1-introduction)
2. [Architecture Overview](#2-architecture-overview)
3. [CSM API Reference](#3-csm-api-reference)
4. [CRYIF API Reference](#4-cryif-api-reference)
5. [Crypto Driver API Reference](#5-crypto-driver-api-reference)
6. [CCC Digital Key API](#6-ccc-digital-key-api)
7. [Algorithm Support Matrix](#7-algorithm-support-matrix)
8. [Configuration Parameters](#8-configuration-parameters)
9. [Error Codes](#9-error-codes)
10. [Performance Data](#10-performance-data)
11. [Version History](#11-version-history)

***

## 1. Introduction

### 1.1 Purpose

This document provides a comprehensive API reference for the YuleTech AutoSAR Crypto Stack, including:

- Crypto Services Manager (CSM) Layer
- Crypto Interface (CRYIF) Layer  
- Crypto Driver Layer
- CCC (Car Connectivity Consortium) Digital Key specific APIs

### 1.2 Scope

This API reference covers:
- Complete function signatures for all crypto services
- Detailed parameter descriptions
- Return value specifications
- Usage examples
- Error handling guidelines

### 1.3 Abbreviations

| Abbreviation | Description |
|************--|************-|
| CSM | Crypto Services Manager |
| CRYIF | Crypto Interface |
| HSM | Hardware Security Module |
| CCC | Car Connectivity Consortium |
| ECDH | Elliptic Curve Diffie-Hellman |
| HKDF | HMAC-based Extract-and-Expand Key Derivation Function |
| AEAD | Authenticated Encryption with Associated Data |

***

## 2. Architecture Overview

### 2.1 Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│              (CCC Digital Key, Security Services)           │
├─────────────────────────────────────────────────────────────┤
│  CSM (Crypto Services Manager)                              │
│  - Job management and scheduling                            │
│  - Key management                                           │
│  - Service abstraction                                      │
├─────────────────────────────────────────────────────────────┤
│  CRYIF (Crypto Interface)                                   │
│  - Channel abstraction                                      │
│  - Key mapping between CSM and Crypto Driver                │
│  - Algorithm mapping                                        │
├─────────────────────────────────────────────────────────────┤
│  Crypto Driver                                              │
│  - Software implementation (Mbed TLS)                       │
│  - Hardware implementation (HSM)                            │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Data Flow

```
Application → CSM → CRYIF → Crypto Driver → HSM/Software
                 ↑      ↑         ↑
            Callbacks  Mapping  Operations
```

***

## 3. CSM API Reference

### 3.1 Lifecycle Functions

#### Csm_Init

```c
Std_ReturnType Csm_Init(const Csm_ConfigType* config);
```

**Description:** Initialize the Crypto Services Manager module.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| config | const Csm_ConfigType* | Pointer to configuration structure |

**Returns:**
| Value | Description |
|******-|************-|
| E_OK | Initialization successful |
| E_NOT_OK | Initialization failed |

**Example:**
```c
const Csm_ConfigType csmConfig = {
    .maxJobs = 16,
    .maxKeys = 32,
    .queueSize = 8
};

Std_ReturnType result = Csm_Init(&csmConfig);
if (result == E_OK) {
    /* CSM initialized successfully */
}
```

***

#### Csm_DeInit

```c
Std_ReturnType Csm_DeInit(void);
```

**Description:** Deinitialize the CSM module and release resources.

**Returns:**
| Value | Description |
|******-|************-|
| E_OK | Deinitialization successful |
| E_NOT_OK | Deinitialization failed |

***

#### Csm_GetVersionInfo

```c
void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

**Description:** Get version information of the CSM module.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| versioninfo | Std_VersionInfoType* | Pointer to version info structure |

***

### 3.2 Key Management Functions

#### Csm_KeyElementSet

```c
Std_ReturnType Csm_KeyElementSet(
    uint32 keyId,
    uint32 keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);
```

**Description:** Set a key element value.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| keyId | uint32 | Key identifier |
| keyElementId | uint32 | Key element identifier (e.g., CRYPTO_KEYELEMENT_KEY) |
| keyPtr | const uint8* | Pointer to key data |
| keyLength | uint32 | Length of key data |

**Returns:**
| Value | Description |
|******-|************-|
| E_OK | Operation successful |
| E_NOT_OK | Operation failed |

**Example:**
```c
uint8 aesKey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

Std_ReturnType result = Csm_KeyElementSet(
    1,                      /* keyId */
    CRYPTO_KEYELEMENT_KEY,  /* keyElementId */
    aesKey,
    sizeof(aesKey)
);
```

***

#### Csm_KeySetValid

```c
Std_ReturnType Csm_KeySetValid(uint32 keyId);
```

**Description:** Set a key to valid state after all elements are configured.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| keyId | uint32 | Key identifier |

**Returns:**
| Value | Description |
|******-|************-|
| E_OK | Key is now valid |
| E_NOT_OK | Operation failed |

***

#### Csm_KeyElementGet

```c
Std_ReturnType Csm_KeyElementGet(
    uint32 keyId,
    uint32 keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr
);
```

**Description:** Retrieve a key element value.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| keyId | uint32 | Key identifier |
| keyElementId | uint32 | Key element identifier |
| keyPtr | uint8* | Output buffer pointer |
| keyLengthPtr | uint32* | Input: buffer size, Output: actual length |

***

#### Csm_KeyGenerate

```c
Std_ReturnType Csm_KeyGenerate(uint32 keyId);
```

**Description:** Generate a new key using the configured algorithm.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| keyId | uint32 | Key identifier to generate |

**Returns:**
| Value | Description |
|******-|************-|
| E_OK | Key generated successfully |
| E_NOT_OK | Generation failed |

**Example:**
```c
/* Generate an ECC P-256 key pair */
Std_ReturnType result = Csm_KeyGenerate(CCC_KEY_ID_EPHEMERAL);
if (result == E_OK) {
    /* Key generated, can retrieve public key */
    uint8 publicKey[65];
    uint32 pubKeyLen = sizeof(publicKey);
    Csm_KeyElementGet(CCC_KEY_ID_EPHEMERAL, 
                      CRYPTO_KEYELEMENT_PUBLIC_KEY,
                      publicKey, &pubKeyLen);
}
```

***

#### Csm_KeyDerive

```c
Std_ReturnType Csm_KeyDerive(
    uint32 keyId,
    uint32 targetKeyId
);
```

**Description:** Derive a key from another key using KDF (e.g., HKDF).

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| keyId | uint32 | Source key identifier |
| targetKeyId | uint32 | Target key identifier |

***

### 3.3 Cryptographic Service Functions

#### Csm_Hash

```c
Std_ReturnType Csm_Hash(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);
```

**Description:** Compute hash value using configured algorithm.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| jobId | uint32 | Job identifier |
| mode | uint8 | Operation mode (START/UPDATE/FINISH/SINGLECALL) |
| dataPtr | const uint8* | Input data pointer |
| dataLength | uint32 | Input data length |
| resultPtr | uint8* | Output buffer for hash result |
| resultLengthPtr | uint32* | Input: buffer size, Output: actual length |

**Mode Values:**
| Mode | Value | Description |
|******|******-|************-|
| CRYPTO_OPERATIONMODE_START | 0x01 | Start operation |
| CRYPTO_OPERATIONMODE_UPDATE | 0x02 | Update with data |
| CRYPTO_OPERATIONMODE_STREAMSTART | 0x03 | Start streaming |
| CRYPTO_OPERATIONMODE_FINISH | 0x04 | Finish operation |
| CRYPTO_OPERATIONMODE_SINGLECALL | 0x07 | Single call operation |

**Example:**
```c
uint8 data[] = "Hello, World!";
uint8 hash[32];
uint32 hashLen = sizeof(hash);

/* Single call hash with SHA-256 */
Std_ReturnType result = Csm_Hash(
    CSM_JOB_HASH_SHA256,
    CRYPTO_OPERATIONMODE_SINGLECALL,
    data,
    sizeof(data) - 1,
    hash,
    &hashLen
);
```

***

#### Csm_Encrypt

```c
Std_ReturnType Csm_Encrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);
```

**Description:** Encrypt data using configured algorithm and key.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| jobId | uint32 | Job identifier |
| mode | uint8 | Operation mode |
| dataPtr | const uint8* | Plaintext data pointer |
| dataLength | uint32 | Plaintext length |
| resultPtr | uint8* | Ciphertext output buffer |
| resultLengthPtr | uint32* | Input: buffer size, Output: actual length |

***

#### Csm_Decrypt

```c
Std_ReturnType Csm_Decrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);
```

**Description:** Decrypt data using configured algorithm and key.

***

#### Csm_MacGenerate

```c
Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr
);
```

**Description:** Generate Message Authentication Code.

***

#### Csm_MacVerify

```c
Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    uint32 macLength,
    boolean* verifyPtr
);
```

**Description:** Verify Message Authentication Code.

***

#### Csm_SignatureGenerate

```c
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);
```

**Description:** Generate digital signature.

**Example:**
```c
uint8 message[] = "Sign this message";
uint8 signature[64];
uint32 sigLen = sizeof(signature);

/* Sign with ECDSA P-256 */
Std_ReturnType result = Csm_SignatureGenerate(
    CSM_JOB_SIGN_ECDSA,
    CRYPTO_OPERATIONMODE_SINGLECALL,
    message,
    sizeof(message) - 1,
    signature,
    &sigLen
);
```

***

#### Csm_SignatureVerify

```c
Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    boolean* verifyPtr
);
```

**Description:** Verify digital signature.

***

#### Csm_RandomGenerate

```c
Std_ReturnType Csm_RandomGenerate(
    uint32 jobId,
    uint8* resultPtr,
    uint32 resultLength
);
```

**Description:** Generate cryptographically secure random data.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| jobId | uint32 | Job identifier for RNG |
| resultPtr | uint8* | Output buffer |
| resultLength | uint32 | Number of random bytes to generate |

**Example:**
```c
uint8 nonce[16];

/* Generate 16-byte random nonce */
Std_ReturnType result = Csm_RandomGenerate(
    CSM_JOB_RANDOM,
    nonce,
    sizeof(nonce)
);
```

***

### 3.4 Job Management Functions

#### Csm_CancelJob

```c
Std_ReturnType Csm_CancelJob(uint32 jobId);
```

**Description:** Cancel a pending or active job.

***

#### Csm_MainFunction

```c
void Csm_MainFunction(void);
```

**Description:** Main function for processing asynchronous jobs. Must be called periodically.

***

### 3.5 Key Exchange Functions

#### Csm_KeyExchangeCalcPubVal

```c
Std_ReturnType Csm_KeyExchangeCalcPubVal(
    uint32 keyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr
);
```

**Description:** Calculate public value for key exchange (ECDH).

***

#### Csm_KeyExchangeCalcSecret

```c
Std_ReturnType Csm_KeyExchangeCalcSecret(
    uint32 keyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength
);
```

**Description:** Calculate shared secret using partner's public value.

***

## 4. CRYIF API Reference

### 4.1 Lifecycle Functions

#### CryIf_Init

```c
void CryIf_Init(const CryIf_ConfigType* configPtr);
```

**Description:** Initialize the Crypto Interface module.

***

#### CryIf_DeInit

```c
void CryIf_DeInit(void);
```

**Description:** Deinitialize the CRYIF module.

***

### 4.2 Job Management Functions

#### CryIf_ProcessJob

```c
Std_ReturnType CryIf_ProcessJob(
    CryIf_ChannelIdType channelId,
    CryIf_JobType* job
);
```

**Description:** Process a crypto job through the specified channel.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| channelId | CryIf_ChannelIdType | Channel identifier |
| job | CryIf_JobType* | Pointer to job structure |

***

#### CryIf_CancelJob

```c
Std_ReturnType CryIf_CancelJob(
    CryIf_ChannelIdType channelId,
    CryIf_JobType* job
);
```

**Description:** Cancel a pending crypto job.

***

### 4.3 Key Management Functions

#### CryIf_KeyElementSet

```c
Std_ReturnType CryIf_KeyElementSet(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);
```

**Description:** Set a key element value.

***

#### CryIf_KeyElementGet

```c
Std_ReturnType CryIf_KeyElementGet(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr
);
```

**Description:** Get a key element value.

***

#### CryIf_KeySetValid

```c
Std_ReturnType CryIf_KeySetValid(CryIf_KeyIdType cryIfKeyId);
```

**Description:** Validate a key.

***

#### CryIf_KeyCopy

```c
Std_ReturnType CryIf_KeyCopy(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType targetCryIfKeyId
);
```

**Description:** Copy an entire key including all elements.

***

### 4.4 Certificate Functions

#### CryIf_CertificateParse

```c
Std_ReturnType CryIf_CertificateParse(CryIf_KeyIdType cryIfKeyId);
```

**Description:** Parse a certificate stored in the specified key.

***

#### CryIf_CertificateVerify

```c
Std_ReturnType CryIf_CertificateVerify(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType verifyCryIfKeyId
);
```

**Description:** Verify a certificate using the verification key.

***

## 5. Crypto Driver API Reference

### 5.1 Standard AUTOSAR API

#### Crypto_Init

```c
void Crypto_Init(const Crypto_ConfigType* configPtr);
```

**Description:** Initialize the Crypto Driver.

***

#### Crypto_ProcessJob

```c
Std_ReturnType Crypto_ProcessJob(uint32 objectId, Crypto_JobType* job);
```

**Description:** Process a crypto job.

***

### 5.2 Key Management API

#### Crypto_KeyElementSet

```c
Std_ReturnType Crypto_KeyElementSet(
    Crypto_KeyIdType cryptoKeyId,
    Crypto_KeyElementIdType keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);
```

***

#### Crypto_KeyElementGet

```c
Std_ReturnType Crypto_KeyElementGet(
    Crypto_KeyIdType cryptoKeyId,
    Crypto_KeyElementIdType keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr
);
```

***

#### Crypto_KeyValidSet

```c
Std_ReturnType Crypto_KeyValidSet(
    Crypto_KeyIdType cryptoKeyId,
    boolean valid
);
```

***

#### Crypto_KeyGenerate

```c
Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType cryptoKeyId);
```

***

#### Crypto_KeyDerive

```c
Std_ReturnType Crypto_KeyDerive(
    Crypto_KeyIdType cryptoKeyId,
    Crypto_KeyIdType targetCryptoKeyId
);
```

***

### 5.3 Cryptographic Operations API

#### Crypto_KeyExchangeCalcSecret

```c
Std_ReturnType Crypto_KeyExchangeCalcSecret(
    Crypto_KeyIdType cryptoKeyId,
    const uint8* partnerPublicKeyPtr,
    uint32 partnerPublicKeyLength
);
```

***

#### Crypto_RandomGenerate

```c
Std_ReturnType Crypto_RandomGenerate(
    Crypto_KeyIdType cryptoKeyId,
    uint8* resultPtr,
    uint32 resultLength
);
```

***

### 5.4 HSM Specific API

#### Crypto_HsmIsAvailable

```c
boolean Crypto_HsmIsAvailable(void);
```

**Description:** Check if HSM hardware is available.

**Returns:**
| Value | Description |
|******-|************-|
| TRUE | HSM is ready |
| FALSE | HSM not available |

***

#### Crypto_HsmGetStatus

```c
Crypto_HsmStateType Crypto_HsmGetStatus(void);
```

**Description:** Get current HSM status.

***

#### Crypto_HsmLoadKey

```c
Std_ReturnType Crypto_HsmLoadKey(Crypto_KeyIdType cryptoKeyId);
```

**Description:** Load a key into HSM secure storage.

***

#### Crypto_HsmUnloadKey

```c
Std_ReturnType Crypto_HsmUnloadKey(Crypto_KeyIdType cryptoKeyId);
```

**Description:** Unload a key from HSM secure storage.

***

## 6. CCC Digital Key API

### 6.1 Lifecycle Functions

#### Ccc_Init

```c
Ccc_ReturnType Ccc_Init(const Ccc_ConfigType* config);
```

**Description:** Initialize CCC Digital Key module.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| config | const Ccc_ConfigType* | Configuration pointer |

**Returns:**
| Value | Description |
|******-|************-|
| CCC_E_OK | Success |
| CCC_E_ALREADY_INITIALIZED | Already initialized |
| CCC_E_CRYPTO_FAILURE | Crypto service init failed |

***

#### Ccc_DeInit

```c
Ccc_ReturnType Ccc_DeInit(void);
```

**Description:** Deinitialize CCC Digital Key module.

***

### 6.2 Pairing API

#### Ccc_PairingStart

```c
Ccc_ReturnType Ccc_PairingStart(
    const Ccc_DeviceIdType* remoteDevice,
    uint8* localPublicKey,
    uint32* publicKeyLength
);
```

**Description:** Start the pairing procedure.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| remoteDevice | const Ccc_DeviceIdType* | Remote device identifier |
| localPublicKey | uint8* | Output buffer for local public key |
| publicKeyLength | uint32* | Input: buffer size, Output: actual length |

**Returns:**
| Value | Description |
|******-|************-|
| CCC_E_OK | Success |
| CCC_E_NOT_INITIALIZED | Not initialized |
| CCC_E_CRYPTO_FAILURE | Key generation failed |
| CCC_E_BUFFER_TOO_SMALL | Buffer too small |

**Example:**
```c
Ccc_DeviceIdType vehicleDevice;
uint8 publicKey[65];
uint32 pubKeyLen = sizeof(publicKey);

/* Generate ephemeral key pair and get public key */
Ccc_ReturnType result = Ccc_PairingStart(&vehicleDevice, publicKey, &pubKeyLen);
if (result == CCC_E_OK) {
    /* Send publicKey to vehicle */
}
```

***

#### Ccc_PairingComplete

```c
Ccc_ReturnType Ccc_PairingComplete(
    const uint8* remotePublicKey,
    uint32 remotePublicKeyLength,
    const Ccc_CertificateType* remoteCert
);
```

**Description:** Complete the pairing procedure.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| remotePublicKey | const uint8* | Remote public key |
| remotePublicKeyLength | uint32 | Remote public key length |
| remoteCert | const Ccc_CertificateType* | Remote certificate |

**Returns:**
| Value | Description |
|******-|************-|
| CCC_E_OK | Pairing successful |
| CCC_E_CERT_INVALID | Certificate invalid |
| CCC_E_CRYPTO_FAILURE | Key agreement failed |

***

### 6.3 Authentication API

#### Ccc_AuthenticationStart

```c
Ccc_ReturnType Ccc_AuthenticationStart(
    uint8* challenge,
    uint32* challengeLength
);
```

**Description:** Start authentication by generating a challenge.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| challenge | uint8* | Output buffer for challenge |
| challengeLength | uint32* | Input: buffer size, Output: actual length |

***

#### Ccc_AuthenticationComplete

```c
Ccc_ReturnType Ccc_AuthenticationComplete(
    const uint8* remoteChallenge,
    const uint8* remoteSignature,
    uint32 signatureLength,
    uint8* localSignature,
    uint32* localSigLength
);
```

**Description:** Complete authentication by verifying remote signature.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| remoteChallenge | const uint8* | Remote challenge value |
| remoteSignature | const uint8* | Remote signature |
| signatureLength | uint32 | Signature length |
| localSignature | uint8* | Output buffer for local signature |
| localSigLength | uint32* | Input: buffer size, Output: actual length |

***

#### Ccc_VerifyCertificate

```c
Ccc_ReturnType Ccc_VerifyCertificate(
    const Ccc_CertificateType* cert,
    const Ccc_CertificateType* caCert
);
```

**Description:** Verify a certificate.

***

### 6.4 Session Management API

#### Ccc_SessionEstablish

```c
Ccc_ReturnType Ccc_SessionEstablish(
    boolean isInitiator,
    const uint8* remotePublicKey,
    uint32 remotePublicKeyLength
);
```

**Description:** Establish a secure session using ECDH key agreement.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| isInitiator | boolean | TRUE if initiating the session |
| remotePublicKey | const uint8* | Remote ephemeral public key |
| remotePublicKeyLength | uint32 | Length of remote public key |

**Example:**
```c
/* After pairing, establish secure session */
uint8 vehiclePubKey[65];
/* ... receive vehicle public key ... */

Ccc_ReturnType result = Ccc_SessionEstablish(
    TRUE,                    /* Mobile device is initiator */
    vehiclePubKey,
    65
);

if (result == CCC_E_OK) {
    /* Session established, can now encrypt/decrypt messages */
}
```

***

#### Ccc_SessionClose

```c
Ccc_ReturnType Ccc_SessionClose(void);
```

**Description:** Close the secure session and clear session keys.

***

### 6.5 Secure Communication API

#### Ccc_EncryptMessage

```c
Ccc_ReturnType Ccc_EncryptMessage(
    const uint8* plaintext,
    uint32 plaintextLength,
    uint8* ciphertext,
    uint32* ciphertextLength,
    uint8* authTag
);
```

**Description:** Encrypt a message using AES-128-GCM.

**Parameters:**
| Parameter | Type | Description |
|*********--|******|************-|
| plaintext | const uint8* | Plaintext data |
| plaintextLength | uint32 | Plaintext length |
| ciphertext | uint8* | Output ciphertext buffer |
| ciphertextLength | uint32* | Input: buffer size, Output: actual length |
| authTag | uint8* | Output authentication tag (16 bytes) |

***

#### Ccc_DecryptMessage

```c
Ccc_ReturnType Ccc_DecryptMessage(
    const uint8* ciphertext,
    uint32 ciphertextLength,
    const uint8* authTag,
    uint8* plaintext,
    uint32* plaintextLength
);
```

**Description:** Decrypt a message using AES-128-GCM.

***

#### Ccc_CreateSecureMessage

```c
Ccc_ReturnType Ccc_CreateSecureMessage(
    Ccc_MessageType messageType,
    const uint8* payload,
    uint32 payloadLength,
    Ccc_SecureMessageType* message
);
```

**Description:** Create a complete secure message package.

***

#### Ccc_ParseSecureMessage

```c
Ccc_ReturnType Ccc_ParseSecureMessage(
    const Ccc_SecureMessageType* message,
    uint8* payload,
    uint32* payloadLength,
    Ccc_MessageType* messageType
);
```

**Description:** Parse and verify a secure message.

***

### 6.6 Utility Functions

#### Ccc_GenerateRandom

```c
Ccc_ReturnType Ccc_GenerateRandom(uint8* randomData, uint32 length);
```

**Description:** Generate secure random data.

***

#### Ccc_CalculateHash

```c
Ccc_ReturnType Ccc_CalculateHash(
    const uint8* data,
    uint32 dataLength,
    uint8* hash,
    uint32* hashLength
);
```

**Description:** Calculate SHA-256 hash.

***

#### Ccc_SignData

```c
Ccc_ReturnType Ccc_SignData(
    const uint8* data,
    uint32 dataLength,
    uint8* signature,
    uint32* signatureLength
);
```

**Description:** Sign data using ECDSA P-256.

***

#### Ccc_VerifySignature

```c
Ccc_ReturnType Ccc_VerifySignature(
    const uint8* data,
    uint32 dataLength,
    const uint8* signature,
    uint32 signatureLength,
    const uint8* publicKey
);
```

**Description:** Verify ECDSA P-256 signature.

***

## 7. Algorithm Support Matrix

### 7.1 Supported Algorithms

| Algorithm | Type | Key Sizes | CSM Support | CRYIF Support | Crypto Driver Support |
|*********--|******|*********--|************-|***************|*********************-|
| **Hash Functions** |
| SHA-1 | Hash | N/A | Yes | Yes | Software/HSM |
| SHA-256 | Hash | N/A | Yes | Yes | Software/HSM |
| SHA-384 | Hash | N/A | Yes | Yes | Software/HSM |
| SHA-512 | Hash | N/A | Yes | Yes | Software/HSM |
| **Symmetric Encryption** |
| AES-ECB | Block Cipher | 128/192/256 | Yes | Yes | Software/HSM |
| AES-CBC | Block Cipher | 128/192/256 | Yes | Yes | Software/HSM |
| AES-CTR | Block Cipher | 128/192/256 | Yes | Yes | Software/HSM |
| AES-GCM | AEAD | 128/192/256 | Yes | Yes | Software/HSM |
| AES-CCM | AEAD | 128/192/256 | Yes | Yes | Software/HSM |
| ChaCha20-Poly1305 | AEAD | 256 | Yes | Yes | Software |
| **Asymmetric Encryption** |
| RSA | Public Key | 1024-4096 | Yes | Yes | Software/HSM |
| ECC P-256 | Public Key | 256 | Yes | Yes | Software/HSM |
| ECC P-384 | Public Key | 384 | Yes | Yes | Software/HSM |
| ECC P-521 | Public Key | 521 | Yes | Yes | Software/HSM |
| **Digital Signatures** |
| ECDSA P-256 | Signature | 256 | Yes | Yes | Software/HSM |
| ECDSA P-384 | Signature | 384 | Yes | Yes | Software/HSM |
| RSA-PSS | Signature | 2048-4096 | Yes | Yes | Software/HSM |
| Ed25519 | Signature | 256 | Yes | Yes | Software |
| **MAC** |
| HMAC-SHA256 | MAC | Any | Yes | Yes | Software/HSM |
| CMAC-AES | MAC | 128/192/256 | Yes | Yes | Software/HSM |
| GMAC | MAC | 128 | Yes | Yes | Software/HSM |
| **Key Derivation** |
| HKDF | KDF | Any | Yes | Yes | Software/HSM |
| PBKDF2 | KDF | Any | Yes | Yes | Software |
| **Key Exchange** |
| ECDH | KEX | 256/384/521 | Yes | Yes | Software/HSM |

### 7.2 CCC Digital Key Specific Algorithms

| Algorithm | Purpose | CCC Version |
|*********--|*********|************-|
| ECDH P-256 | Session key establishment | 3.0 |
| HKDF-SHA256 | Key derivation | 3.0 |
| AES-128-GCM | Secure messaging | 3.0 |
| ECDSA P-256 | Authentication signatures | 3.0 |
| SHA-256 | Certificate hashing | 3.0 |

***

## 8. Configuration Parameters

### 8.1 CSM Configuration (CryptoStack_Cfg.h)

| Parameter | Default | Description |
|*********--|*********|************-|
| CSM_MAX_JOBS | 16 | Maximum number of concurrent jobs |
| CSM_MAX_KEYS | 32 | Maximum number of keys |
| CSM_MAX_KEY_ELEMENTS | 8 | Maximum key elements per key |
| CSM_MAX_QUEUE_DEPTH | 8 | Job queue depth |
| CSM_MAX_CHANNELS | 4 | Maximum channels |
| CSM_ASYNC_MODE_ENABLE | STD_ON | Enable asynchronous processing |
| CSM_SYNC_MODE_ENABLE | STD_ON | Enable synchronous processing |
| CSM_QUEUE_PROCESSING_PERIOD | 10ms | Queue processing period |

### 8.2 CRYIF Configuration

| Parameter | Default | Description |
|*********--|*********|************-|
| CRYIF_MAX_CRYPTO_DRIVERS | 4 | Maximum crypto drivers |
| CRYIF_MAX_CHANNELS | 8 | Maximum channels |
| CRYIF_KEY_MAPPING_ENABLE | STD_ON | Enable key mapping |
| CRYIF_ALGORITHM_MAPPING_ENABLE | STD_ON | Enable algorithm mapping |
| CRYIF_ERROR_TRANSLATION_ENABLE | STD_ON | Enable error translation |

### 8.3 Crypto Driver Configuration

| Parameter | Default | Description |
|*********--|*********|************-|
| CRYPTO_CFG_HSM_ENABLED | STD_OFF | Enable HSM support |
| CRYPTO_CFG_DEV_ERROR_DETECT | STD_ON | Development error detection |
| CRYPTO_CFG_VERSION_INFO_API | STD_ON | Version info API |

### 8.4 CCC Digital Key Configuration

| Parameter | Value | Description |
|*********--|******-|************-|
| CCC_ECC_P256_KEY_SIZE | 32 bytes | ECC P-256 private key size |
| CCC_ECC_P256_PUBLIC_KEY_SIZE | 65 bytes | ECC P-256 public key size |
| CCC_AES_KEY_SIZE | 16 bytes | AES-128 key size |
| CCC_AES_IV_SIZE | 12 bytes | AES-GCM IV size |
| CCC_AES_TAG_SIZE | 16 bytes | AES-GCM tag size |
| CCC_CHALLENGE_SIZE | 32 bytes | Authentication challenge size |
| CCC_MAX_CERTIFICATE_SIZE | 1024 bytes | Maximum certificate size |
| CCC_MAX_MESSAGE_SIZE | 512 bytes | Maximum message size |

***

## 9. Error Codes

### 9.1 CSM Error Codes

| Code | Value | Description |
|******|******-|************-|
| CSM_E_NO_ERROR | E_OK (0x00) | No error |
| CSM_E_NOT_INITIALIZED | 0x01 | Module not initialized |
| CSM_E_ALREADY_INITIALIZED | 0x02 | Module already initialized |
| CSM_E_PARAM_POINTER | 0x03 | Invalid pointer parameter |
| CSM_E_PARAM_KEY_ID | 0x04 | Invalid key ID |
| CSM_E_PARAM_KEY_ELEMENT_ID | 0x05 | Invalid key element ID |
| CSM_E_PARAM_JOB_ID | 0x06 | Invalid job ID |
| CSM_E_PARAM_ALGORITHM | 0x07 | Invalid algorithm |
| CSM_E_PARAM_MODE | 0x08 | Invalid mode |
| CSM_E_PARAM_LENGTH | 0x09 | Invalid length |
| CSM_E_KEY_NOT_AVAILABLE | 0x0A | Key not available |
| CSM_E_KEY_NOT_VALID | 0x0B | Key not valid |
| CSM_E_KEY_SIZE_MISMATCH | 0x0C | Key size mismatch |
| CSM_E_JOB_BUSY | 0x0D | Job is busy |
| CSM_E_QUEUE_FULL | 0x0E | Queue is full |
| CSM_E_SERVICE_NOT_SUPPORTED | 0x0F | Service not supported |

### 9.2 CRYIF Error Codes

| Code | Value | Description |
|******|******-|************-|
| CRYIF_E_NO_ERROR | E_OK (0x00) | No error |
| CRYIF_E_PARAM_POINTER | 0x01 | Invalid pointer |
| CRYIF_E_PARAM_HANDLE | 0x02 | Invalid handle |
| CRYIF_E_PARAM_KEY_ID | 0x03 | Invalid key ID |
| CRYIF_E_PARAM_ALGORITHM | 0x04 | Invalid algorithm |
| CRYIF_E_PARAM_KEY_FORMAT | 0x05 | Invalid key format |

### 9.3 Crypto Driver Error Codes

| Code | Value | Description |
|******|******-|************-|
| CRYPTO_ERROR_NONE | 0x00 | No error |
| CRYPTO_ERROR_GENERAL | 0x01 | General error |
| CRYPTO_ERROR_NOT_INITIALIZED | 0x02 | Not initialized |
| CRYPTO_ERROR_PARAM_POINTER | 0x03 | Invalid pointer |
| CRYPTO_ERROR_PARAM_KEY_ID | 0x04 | Invalid key ID |
| CRYPTO_ERROR_PARAM_KEY_ELEMENT_ID | 0x05 | Invalid key element ID |
| CRYPTO_ERROR_PARAM_JOB_ID | 0x06 | Invalid job ID |
| CRYPTO_ERROR_KEY_NOT_AVAILABLE | 0x07 | Key not available |
| CRYPTO_ERROR_KEY_NOT_VALID | 0x08 | Key not valid |
| CRYPTO_ERROR_JOB_BUSY | 0x09 | Job busy |
| CRYPTO_ERROR_VERIFICATION_FAILED | 0x0A | Verification failed |

### 9.4 CCC Digital Key Error Codes

| Code | Value | Description |
|******|******-|************-|
| CCC_E_OK | 0x00 | Success |
| CCC_E_NOT_INITIALIZED | 0x01 | Not initialized |
| CCC_E_ALREADY_INITIALIZED | 0x02 | Already initialized |
| CCC_E_PARAM_POINTER | 0x03 | Invalid pointer |
| CCC_E_CRYPTO_FAILURE | 0x04 | Crypto operation failed |
| CCC_E_KEY_NOT_FOUND | 0x05 | Key not found |
| CCC_E_KEY_INVALID | 0x06 | Key invalid |
| CCC_E_CERT_INVALID | 0x07 | Certificate invalid |
| CCC_E_CERT_EXPIRED | 0x08 | Certificate expired |
| CCC_E_SIGNATURE_INVALID | 0x09 | Signature invalid |
| CCC_E_AUTHENTICATION_FAILED | 0x0A | Authentication failed |
| CCC_E_SESSION_NOT_ESTABLISHED | 0x0B | Session not established |
| CCC_E_REPLAY_DETECTED | 0x0C | Replay attack detected |
| CCC_E_MESSAGE_INVALID | 0x0D | Message invalid |
| CCC_E_BUFFER_TOO_SMALL | 0x0E | Buffer too small |

***

## 10. Performance Data

### 10.1 Software Implementation (Mbed TLS) Performance

| Operation | Data Size | Time (ARM Cortex-M4 @ 80MHz) |
|*********--|*********--|******************************|
| SHA-256 | 1 KB | 0.5 ms |
| SHA-256 | 1 MB | 512 ms |
| AES-128-GCM Encrypt | 1 KB | 1.2 ms |
| AES-128-GCM Decrypt | 1 KB | 1.2 ms |
| ECDSA P-256 Sign | - | 45 ms |
| ECDSA P-256 Verify | - | 85 ms |
| ECDH P-256 | - | 42 ms |
| HKDF-SHA256 | 32B → 48B | 0.8 ms |
| RSA-2048 Sign | - | 125 ms |
| RSA-2048 Verify | - | 4 ms |

### 10.2 HSM Implementation Performance

| Operation | Data Size | Time (Typical HSM) |
|*********--|*********--|******************-|
| SHA-256 | 1 KB | 0.1 ms |
| AES-128-GCM Encrypt | 1 KB | 0.2 ms |
| ECDSA P-256 Sign | - | 5 ms |
| ECDSA P-256 Verify | - | 8 ms |
| ECDH P-256 | - | 6 ms |
| RSA-2048 Sign | - | 15 ms |
| RSA-2048 Verify | - | 1 ms |

### 10.3 CCC Digital Key Performance

| Operation | Time (Software) | Time (HSM) |
|*********--|***************--|************|
| Pairing Start | 45 ms | 6 ms |
| Pairing Complete | 90 ms | 14 ms |
| Authentication Start | 2 ms | 1 ms |
| Authentication Complete | 130 ms | 13 ms |
| Session Establish | 43 ms | 7 ms |
| Encrypt Message (256B) | 1.5 ms | 0.3 ms |
| Decrypt Message (256B) | 1.5 ms | 0.3 ms |

***

## 11. Version History

### Version 1.0.0 (2026-05-01)

**Initial Release:**
- Complete CSM API implementation
- Complete CRYIF API implementation  
- Complete Crypto Driver API implementation
- CCC Digital Key 3.0 compliant APIs
- Support for software (Mbed TLS) and hardware (HSM) implementations
- Full algorithm support matrix documentation
- Complete error code definitions

**Features:**
- Asynchronous and synchronous operation modes
- Job queue management
- Key management with element-based architecture
- Certificate parsing and verification
- ECDH key exchange
- HKDF key derivation
- AES-GCM authenticated encryption
- ECDSA digital signatures

**Supported Standards:**
- AUTOSAR Classic Platform R20-11
- CCC Digital Key Specification 3.0
- FIPS 180-4 (SHA)
- FIPS 197 (AES)
- NIST SP 800-56A (ECDH)
- RFC 5869 (HKDF)
- RFC 8439 (ChaCha20-Poly1305)

***

## Appendix A: Type Definitions

### A.1 Common Types

```c
typedef uint32 Crypto_JobIdType;
typedef uint32 Crypto_KeyIdType;
typedef uint32 Crypto_KeyElementIdType;
typedef uint8  Crypto_OperationModeType;

typedef enum {
    CRYPTO_OPRESULT_OK = 0,
    CRYPTO_OPRESULT_NOT_OK,
    CRYPTO_OPRESULT_BUSY,
    CRYPTO_OPRESULT_BUSY_RETRY_LATER,
    CRYPTO_OPRESULT_CANCELLED
} Crypto_OperationResultType;

typedef enum {
    CRYPTO_VERIFY_FAILED = 0,
    CRYPTO_VERIFY_PASSED,
    CRYPTO_VERIFY_IN_PROGRESS
} Crypto_VerifyResultType;
```

### A.2 CCC Specific Types

```c
typedef enum {
    CCC_E_OK = 0,
    CCC_E_NOT_INITIALIZED,
    CCC_E_CRYPTO_FAILURE,
    /* ... */
} Ccc_ReturnType;

typedef enum {
    CCC_MODE_UNINITIALIZED = 0,
    CCC_MODE_PAIRING,
    CCC_MODE_AUTHENTICATION,
    CCC_MODE_OPERATIONAL
} Ccc_ModeType;
```

***

**Document End**

*Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.*
