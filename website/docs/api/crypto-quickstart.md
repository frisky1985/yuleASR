---
title: Crypto API Quick Start Guide
description: "- Version: 1.0.0"
sidebar_position: 16
---

# Crypto API Quick Start Guide

**YuleTech AutoSAR Crypto Stack - Quick Start Guide**

- Version: 1.0.0
- Date: 2026-05-01
- Company: Shanghai Yule Electronics Technology Co., Ltd.

***

## Table of Contents

1. [Introduction](#1-introduction)
2. [Prerequisites](#2-prerequisites)
3. [Basic Setup](#3-basic-setup)
4. [Common Use Cases](#4-common-use-cases)
5. [CCC Digital Key Quick Start](#5-ccc-digital-key-quick-start)
6. [Integration Examples](#6-integration-examples)
7. [Troubleshooting](#7-troubleshooting)

***

## 1. Introduction

This guide provides quick start instructions for using the YuleTech AutoSAR Crypto Stack API. It covers the essential steps to:

- Initialize the crypto stack
- Perform basic cryptographic operations
- Implement CCC Digital Key workflows
- Integrate with your application

***

## 2. Prerequisites

### 2.1 Required Headers

```c
/* Standard AUTOSAR types */
#include "Std_Types.h"

/* CSM API */
#include "Csm.h"

/* Crypto Driver */
#include "Crypto.h"

/* CCC Digital Key (if applicable) */
#include "CccDigitalKey.h"
```

### 2.2 Configuration Files

Ensure the following configuration headers are properly set up:
- `CryptoStack_Cfg.h` - Stack configuration
- `Csm_Cfg.h` - CSM module configuration
- `Crypto_Cfg.h` - Crypto driver configuration

***

## 3. Basic Setup

### 3.1 Initialize Crypto Stack

```c
#include "Csm.h"
#include "CryIf.h"
#include "Crypto.h"

void CryptoStack_Init(void)
{
    Std_ReturnType result;
    
    /* Initialize Crypto Driver (lowest layer) */
    Crypto_Init(NULL_PTR);
    
    /* Initialize Crypto Interface */
    CryIf_Init(NULL_PTR);
    
    /* Initialize CSM */
    result = Csm_Init(NULL_PTR);
    
    if (result == E_OK) {
        printf("Crypto Stack initialized successfully\n");
    } else {
        printf("Crypto Stack initialization failed\n");
    }
}
```

### 3.2 Deinitialize Crypto Stack

```c
void CryptoStack_DeInit(void)
{
    /* Deinitialize in reverse order */
    Csm_DeInit();
    CryIf_DeInit();
    Crypto_DeInit();
}
```

### 3.3 Main Function Integration

Add to your main loop:

```c
void main_loop(void)
{
    while (1) {
        /* Process asynchronous crypto jobs */
        Csm_MainFunction();
        
        /* Process CRYIF async operations */
        CryIf_MainFunction();
        
        /* Your application code */
        Application_Process();
        
        /* Delay or sleep */
        Delay(10);  /* 10ms */
    }
}
```

***

## 4. Common Use Cases

### 4.1 Compute SHA-256 Hash

```c
#include "Csm.h"

Std_ReturnType Hash_Data(const uint8* data, uint32 dataLen, 
                         uint8* hashOut, uint32* hashLen)
{
    /* Use single-call mode for simple hashing */
    return Csm_Hash(
        CSM_JOB_HASH_SHA256,           /* Pre-configured job ID */
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        hashOut,
        hashLen
    );
}

/* Usage Example */
void Example_Hash(void)
{
    const uint8 message[] = "Hello, World!";
    uint8 hash[32];
    uint32 hashLen = sizeof(hash);
    
    Std_ReturnType result = Hash_Data(
        message, 
        sizeof(message) - 1,
        hash, 
        &hashLen
    );
    
    if (result == E_OK) {
        printf("Hash computed successfully\n");
        /* hash now contains 32-byte SHA-256 digest */
    }
}
```

### 4.2 Generate Random Data

```c
#include "Csm.h"

Std_ReturnType Generate_Nonce(uint8* nonce, uint32 length)
{
    return Csm_RandomGenerate(
        CSM_JOB_RANDOM,    /* Pre-configured RNG job */
        nonce,
        length
    );
}

/* Usage Example */
void Example_Random(void)
{
    uint8 nonce[16];
    
    if (Generate_Nonce(nonce, sizeof(nonce)) == E_OK) {
        printf("16-byte nonce generated\n");
    }
}
```

### 4.3 AES-128 Encryption

```c
#include "Csm.h"

/* First, set up the encryption key */
Std_ReturnType Setup_EncryptionKey(const uint8* key)
{
    /* Set key material */
    Std_ReturnType result = Csm_KeyElementSet(
        1,                           /* Key ID */
        CRYPTO_KEYELEMENT_KEY,       /* Key element */
        key,
        16                           /* AES-128 key size */
    );
    
    if (result == E_OK) {
        /* Validate the key */
        result = Csm_KeySetValid(1);
    }
    
    return result;
}

/* Encrypt data */
Std_ReturnType Encrypt_Data(const uint8* plaintext, uint32 plainLen,
                            uint8* ciphertext, uint32* cipherLen)
{
    /* Set IV if needed */
    uint8 iv[16] = {0};  /* Should be random in production */
    Csm_KeyElementSet(1, CRYPTO_KEYELEMENT_IV, iv, sizeof(iv));
    
    return Csm_Encrypt(
        CSM_JOB_AES128_CBC,            /* Pre-configured encryption job */
        CRYPTO_OPERATIONMODE_SINGLECALL,
        plaintext,
        plainLen,
        ciphertext,
        cipherLen
    );
}
```

### 4.4 ECDSA Sign and Verify

```c
#include "Csm.h"

/* Sign data with ECDSA P-256 */
Std_ReturnType Sign_Data(const uint8* data, uint32 dataLen,
                         uint8* signature, uint32* sigLen)
{
    return Csm_SignatureGenerate(
        CSM_JOB_ECDSA_P256_SIGN,
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        signature,
        sigLen
    );
}

/* Verify signature */
boolean Verify_Signature(const uint8* data, uint32 dataLen,
                         const uint8* signature, uint32 sigLen)
{
    boolean verifyResult = FALSE;
    
    Std_ReturnType result = Csm_SignatureVerify(
        CSM_JOB_ECDSA_P256_VERIFY,
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        signature,
        sigLen,
        &verifyResult
    );
    
    return (result == E_OK) && verifyResult;
}

/* Usage Example */
void Example_SignVerify(void)
{
    const uint8 message[] = "Sign this message";
    uint8 signature[64];
    uint32 sigLen = sizeof(signature);
    
    /* Sign */
    if (Sign_Data(message, sizeof(message) - 1, 
                  signature, &sigLen) == E_OK) {
        printf("Message signed\n");
        
        /* Verify */
        if (Verify_Signature(message, sizeof(message) - 1,
                            signature, sigLen)) {
            printf("Signature verified!\n");
        }
    }
}
```

### 4.5 HMAC Generation

```c
#include "Csm.h"

Std_ReturnType Generate_HMAC(const uint8* data, uint32 dataLen,
                             uint8* mac, uint32* macLen)
{
    return Csm_MacGenerate(
        CSM_JOB_HMAC_SHA256,
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        mac,
        macLen
    );
}
```

### 4.6 Key Derivation (HKDF)

```c
#include "Csm.h"

/* Derive a key using HKDF-SHA256 */
Std_ReturnType Derive_Key(uint32 masterKeyId, uint32 derivedKeyId)
{
    /* Set salt and info for HKDF */
    uint8 salt[32] = {0};
    uint8 info[32] = "application-specific-info";
    
    Csm_KeyElementSet(masterKeyId, CRYPTO_KEYELEMENT_KDF_SALT, 
                      salt, sizeof(salt));
    Csm_KeyElementSet(masterKeyId, CRYPTO_KEYELEMENT_KDF_INFO,
                      info, strlen((char*)info));
    
    /* Perform key derivation */
    return Csm_KeyDerive(masterKeyId, derivedKeyId);
}
```

### 4.7 ECDH Key Exchange

```c
#include "Csm.h"

/* Generate ephemeral key pair and get public value */
Std_ReturnType ECDH_GeneratePublicValue(uint8* publicValue, 
                                        uint32* pubValueLen)
{
    /* Generate ephemeral key pair */
    Std_ReturnType result = Csm_KeyGenerate(CSM_KEY_EPHEMERAL);
    
    if (result == E_OK) {
        /* Calculate public value */
        result = Csm_KeyExchangeCalcPubVal(
            CSM_KEY_EPHEMERAL,
            publicValue,
            pubValueLen
        );
    }
    
    return result;
}

/* Calculate shared secret with partner's public key */
Std_ReturnType ECDH_CalculateSharedSecret(const uint8* partnerPubKey,
                                          uint32 partnerPubKeyLen)
{
    return Csm_KeyExchangeCalcSecret(
        CSM_KEY_EPHEMERAL,
        partnerPubKey,
        partnerPubKeyLen
    );
}

/* Complete ECDH example */
void Example_ECDH(void)
{
    uint8 localPubKey[65];
    uint32 localPubKeyLen = sizeof(localPubKey);
    
    /* Generate local key pair */
    if (ECDH_GeneratePublicValue(localPubKey, &localPubKeyLen) == E_OK) {
        /* Send localPubKey to remote party */
        /* ... communication ... */
        
        /* Receive remote public key */
        uint8 remotePubKey[65];
        uint32 remotePubKeyLen = 65;
        /* ... receive from remote ... */
        
        /* Calculate shared secret */
        if (ECDH_CalculateSharedSecret(remotePubKey, remotePubKeyLen) == E_OK) {
            printf("Shared secret established\n");
            /* Shared secret is now in the key slot */
        }
    }
}
```

***

## 5. CCC Digital Key Quick Start

### 5.1 Complete Pairing Flow

```c
#include "CccDigitalKey.h"

void CCC_PairingExample(void)
{
    Ccc_ReturnType result;
    
    /* Step 1: Initialize CCC module */
    Ccc_ConfigType cccConfig = {
        .deviceId = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}},
        .role = CCC_ROLE_MOBILE_DEVICE,
        .csmKeyId = 1,
        .useSecureStorage = TRUE
    };
    
    result = Ccc_Init(&cccConfig);
    if (result != CCC_E_OK) {
        printf("CCC Init failed\n");
        return;
    }
    
    /* Step 2: Start pairing */
    Ccc_DeviceIdType vehicleDevice;
    uint8 localPubKey[65];
    uint32 pubKeyLen = sizeof(localPubKey);
    
    result = Ccc_PairingStart(&vehicleDevice, localPubKey, &pubKeyLen);
    if (result != CCC_E_OK) {
        printf("Pairing start failed\n");
        return;
    }
    
    /* Send localPubKey to vehicle */
    /* ... BLE/NFC communication ... */
    
    /* Step 3: Receive vehicle data */
    uint8 vehiclePubKey[65];
    Ccc_CertificateType vehicleCert;
    /* ... receive from vehicle ... */
    
    /* Step 4: Complete pairing */
    result = Ccc_PairingComplete(vehiclePubKey, 65, &vehicleCert);
    if (result == CCC_E_OK) {
        printf("Pairing successful!\n");
    } else {
        printf("Pairing failed: %d\n", result);
    }
}
```

### 5.2 Authentication Flow

```c
#include "CccDigitalKey.h"

void CCC_AuthenticationExample(void)
{
    Ccc_ReturnType result;
    
    /* Step 1: Start authentication (generate challenge) */
    uint8 challenge[32];
    uint32 challengeLen = sizeof(challenge);
    
    result = Ccc_AuthenticationStart(challenge, &challengeLen);
    if (result != CCC_E_OK) {
        printf("Auth start failed\n");
        return;
    }
    
    /* Send challenge to vehicle */
    /* ... communication ... */
    
    /* Step 2: Receive vehicle response */
    uint8 vehicleChallenge[32];
    uint8 vehicleSignature[64];
    /* ... receive from vehicle ... */
    
    /* Step 3: Complete authentication */
    uint8 localSignature[64];
    uint32 localSigLen = sizeof(localSignature);
    
    result = Ccc_AuthenticationComplete(
        vehicleChallenge,
        vehicleSignature,
        64,
        localSignature,
        &localSigLen
    );
    
    if (result == CCC_E_OK) {
        printf("Authentication successful!\n");
        /* Send localSignature to vehicle */
    } else {
        printf("Authentication failed: %d\n", result);
    }
}
```

### 5.3 Secure Session Establishment

```c
#include "CccDigitalKey.h"

void CCC_SessionExample(void)
{
    Ccc_ReturnType result;
    
    /* Establish secure session after authentication */
    uint8 vehicleEphemeralPubKey[65];
    /* ... receive ephemeral key from vehicle ... */
    
    result = Ccc_SessionEstablish(
        TRUE,                           /* Mobile device is initiator */
        vehicleEphemeralPubKey,
        65
    );
    
    if (result == CCC_E_OK) {
        printf("Secure session established!\n");
    }
}
```

### 5.4 Secure Communication

```c
#include "CccDigitalKey.h"

void CCC_SecureMessagingExample(void)
{
    Ccc_ReturnType result;
    
    /* Encrypt a message */
    const uint8 plaintext[] = "Unlock command";
    uint8 ciphertext[256];
    uint32 cipherLen = sizeof(ciphertext);
    uint8 authTag[16];
    
    result = Ccc_EncryptMessage(
        plaintext,
        sizeof(plaintext) - 1,
        ciphertext,
        &cipherLen,
        authTag
    );
    
    if (result == CCC_E_OK) {
        /* Send ciphertext + authTag to vehicle */
        printf("Message encrypted\n");
    }
    
    /* Decrypt a response */
    uint8 responseCipher[256];
    uint8 responseTag[16];
    uint8 responsePlain[256];
    uint32 responsePlainLen = sizeof(responsePlain);
    /* ... receive from vehicle ... */
    
    result = Ccc_DecryptMessage(
        responseCipher,
        256,
        responseTag,
        responsePlain,
        &responsePlainLen
    );
    
    if (result == CCC_E_OK) {
        printf("Response decrypted: %s\n", responsePlain);
    }
}
```

### 5.5 Complete CCC Transaction

```c
#include "CccDigitalKey.h"

void CCC_UnlockVehicle(void)
{
    Ccc_ReturnType result;
    
    /* 1. Ensure session is established */
    Ccc_SessionStateType sessionState;
    Ccc_GetSessionState(&sessionState);
    
    if (sessionState != CCC_SESSION_STATE_ACTIVE) {
        printf("No active session\n");
        return;
    }
    
    /* 2. Create unlock command */
    const uint8 unlockCmd[] = {
        0x01,       /* Command type: Unlock */
        0x01,       /* Door: All doors */
        0x00, 0x00  /* Reserved */
    };
    
    /* 3. Create secure message */
    Ccc_SecureMessageType secureMsg;
    result = Ccc_CreateSecureMessage(
        CCC_MSG_SECURE_MESSAGE,
        unlockCmd,
        sizeof(unlockCmd),
        &secureMsg
    );
    
    if (result != CCC_E_OK) {
        printf("Failed to create secure message\n");
        return;
    }
    
    /* 4. Send to vehicle */
    /* ... BLE/NFC transmission ... */
    
    /* 5. Receive response */
    Ccc_SecureMessageType response;
    /* ... receive from vehicle ... */
    
    /* 6. Parse response */
    uint8 responsePayload[64];
    uint32 responseLen = sizeof(responsePayload);
    Ccc_MessageType msgType;
    
    result = Ccc_ParseSecureMessage(
        &response,
        responsePayload,
        &responseLen,
        &msgType
    );
    
    if (result == CCC_E_OK && msgType != CCC_MSG_ERROR) {
        printf("Vehicle unlocked successfully!\n");
    } else {
        printf("Unlock failed\n");
    }
}
```

***

## 6. Integration Examples

### 6.1 Integration with DCM (Diagnostic Communication Manager)

```c
#include "Csm.h"
#include "Dcm.h"

/* Security access with crypto services */
Std_ReturnType Dcm_SecurityAccess_GenerateSeed(uint8* seed, uint8* seedLen)
{
    /* Generate 4-byte random seed */
    return Csm_RandomGenerate(
        CSM_JOB_RANDOM,
        seed,
        4
    );
}

Std_ReturnType Dcm_SecurityAccess_VerifyKey(const uint8* key, uint8 keyLen,
                                            const uint8* seed, uint8 seedLen)
{
    uint8 expectedKey[16];
    uint32 expectedKeyLen = sizeof(expectedKey);
    
    /* Derive key from seed using configured algorithm */
    /* Compare with received key */
    
    return E_OK;
}
```

### 6.2 Integration with SecOC (Secure Onboard Communication)

```c
#include "Csm.h"
#include "SecOC.h"

/* Generate freshness value */
Std_ReturnType SecOC_GetFreshnessValue(uint8* freshness, uint8* freshnessLen)
{
    return Csm_RandomGenerate(
        CSM_JOB_RANDOM,
        freshness,
        *freshnessLen
    );
}

/* Authenticate Secured PDU */
Std_ReturnType SecOC_VerifyAuthCode(const uint8* data, uint32 dataLen,
                                    const uint8* authCode, uint32 authCodeLen)
{
    uint8 computedMac[16];
    uint32 macLen = sizeof(computedMac);
    boolean verifyResult;
    
    /* Generate MAC */
    Csm_MacGenerate(
        CSM_JOB_CMAC_AES128,
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        computedMac,
        &macLen
    );
    
    /* Compare with received auth code */
    verifyResult = (memcmp(computedMac, authCode, authCodeLen) == 0);
    
    return verifyResult ? E_OK : E_NOT_OK;
}
```

### 6.3 Integration with NvM (NVRAM Manager)

```c
#include "Csm.h"
#include "NvM.h"

/* Encrypt data before storing to NvM */
Std_ReturnType NvM_WriteEncrypted(NvM_BlockIdType blockId, 
                                   const uint8* data, 
                                   uint32 dataLen)
{
    uint8 encrypted[256];
    uint32 encryptedLen = sizeof(encrypted);
    
    /* Encrypt data */
    Std_ReturnType result = Csm_Encrypt(
        CSM_JOB_AES128_CBC,
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data,
        dataLen,
        encrypted,
        &encryptedLen
    );
    
    if (result == E_OK) {
        /* Write encrypted data to NvM */
        result = NvM_WriteBlock(blockId, encrypted);
    }
    
    return result;
}
```

***

## 7. Troubleshooting

### 7.1 Common Issues

#### Issue: Csm_Init returns E_NOT_OK

**Possible Causes:**
- Crypto driver not initialized
- Invalid configuration
- Resource conflict

**Solutions:**
```c
/* Ensure proper initialization order */
Crypto_Init(NULL_PTR);      /* 1. Driver first */
CryIf_Init(NULL_PTR);       /* 2. Then interface */
Csm_Init(NULL_PTR);         /* 3. Then manager */
```

#### Issue: Key operations fail with CSM_E_KEY_NOT_VALID

**Solutions:**
```c
/* Ensure key is properly set up */
Csm_KeyElementSet(keyId, elementId, keyData, keyLen);
Csm_KeySetValid(keyId);  /* Don't forget this! */
```

#### Issue: Encryption/decryption produces wrong results

**Possible Causes:**
- IV not set (for CBC mode)
- Key not configured correctly
- Buffer size mismatch

**Solutions:**
```c
/* Always set IV for CBC mode */
uint8 iv[16];
Csm_RandomGenerate(CSM_JOB_RANDOM, iv, sizeof(iv));
Csm_KeyElementSet(keyId, CRYPTO_KEYELEMENT_IV, iv, sizeof(iv));

/* Ensure output buffer is large enough */
uint32 cipherLen = plainLen + 16;  /* Add block size for padding */
uint8* ciphertext = malloc(cipherLen);
```

#### Issue: CCC pairing fails

**Possible Causes:**
- CSM not initialized
- Certificate validation failed
- Invalid key configuration

**Solutions:**
```c
/* Verify initialization state */
if (Ccc_GetCurrentMode() == CCC_MODE_UNINITIALIZED) {
    Ccc_Init(&config);
}

/* Check CSM is ready */
Csm_KeyStatusType keyStatus;
Csm_GetKeyStatus(CCC_KEY_ID_DEVICE, &keyStatus);
```

### 7.2 Debug Tips

Enable development error detection:
```c
/* In Csm_Cfg.h */
#define CSM_DEV_ERROR_DETECT    STD_ON

/* In CryptoStack_Cfg.h */
#define CRYPTO_STACK_DEV_ERROR_DETECT   STD_ON
```

Use callback for async operations:
```c
void MyJobCallback(uint32 jobId, Crypto_OperationResultType result, void* userData)
{
    printf("Job %d completed with result %d\n", jobId, result);
}

Csm_RegisterCallback(jobId, MyJobCallback, NULL);
```

### 7.3 Performance Optimization

Use asynchronous mode for large operations:
```c
/* For large data processing, use streaming mode */
Csm_Hash(jobId, CRYPTO_OPERATIONMODE_START, NULL, 0, NULL, NULL);

while (hasMoreData) {
    Csm_Hash(jobId, CRYPTO_OPERATIONMODE_UPDATE, 
             chunk, chunkLen, NULL, NULL);
}

Csm_Hash(jobId, CRYPTO_OPERATIONMODE_FINISH, 
         NULL, 0, result, &resultLen);
```

Use HSM for performance-critical operations:
```c
/* Check HSM availability */
if (Crypto_HsmIsAvailable()) {
    /* Use HSM for ECDSA operations */
    Csm_SignatureGenerate(HSM_JOB_ECDSA_SIGN, ...);
}
```

***

## Appendix: Configuration Quick Reference

### Minimum Required Configuration

```c
/* CryptoStack_Cfg.h */
#define CSM_MAX_JOBS                    8U
#define CSM_MAX_KEYS                    16U
#define CSM_MAX_KEY_ELEMENTS            4U
#define CSM_MAX_QUEUE_DEPTH             4U
#define CSM_ASYNC_MODE_ENABLE           STD_ON
#define CSM_SYNC_MODE_ENABLE            STD_ON

/* CRYIF */
#define CRYIF_MAX_CRYPTO_DRIVERS        2U
#define CRYIF_MAX_CHANNELS              4U
```

### Memory Usage Estimation

| Component | RAM | ROM |
|*********--|***--|***--|
| CSM Core | ~2 KB | ~8 KB |
| CRYIF | ~1 KB | ~4 KB |
| Crypto Driver (SW) | ~4 KB | ~32 KB |
| Mbed TLS | ~8 KB | ~64 KB |
| **Total (Software)** | **~15 KB** | **~108 KB** |
| **With HSM** | **~7 KB** | **~16 KB** |

***

**Document End**

*Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.*
