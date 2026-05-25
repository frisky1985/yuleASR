# CRYPTO (Cryptographic Services) Module

## Overview

The CRYPTO module provides hardware-accelerated cryptographic operations for the AUTOSAR Basic Software. It abstracts the microcontroller's cryptographic hardware (HSM, SHE, TPM) and provides secure key management, encryption/decryption, hashing, and digital signature services.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Security Standard**: SHE (Secure Hardware Extension), HSM (Hardware Security Module)  
**Hardware**: NXP S32K3 (HSM/CSEc) / Infineon TC3xx (HSM) / STM32H7 (SAES/CRYP)  
**ASIL Level**: Up to ASIL-D

## Features

- **Hardware-Accelerated Crypto**: AES-128/256, RSA, ECC operations
- **Secure Key Management**: Key storage in secure hardware
- **SHE Compliance**: Compliant with Secure Hardware Extension standard
- **Random Number Generation**: True Random Number Generator (TRNG)
- **Secure Boot**: Supports secure boot verification
- **Key Derivation**: Supports HKDF, PBKDF2
- **Hash Algorithms**: SHA-1, SHA-2 (224/256/384/512)
- **MAC/HMAC**: Message authentication codes
- **Digital Signatures**: ECDSA, RSA-PSS
- **Secure Counter**: Monotonic counter for replay protection

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│  (Secure Boot, Key Management,      │
│   Secure Communication)             │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       CryIf (Crypto Interface)      │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│          Crypto Driver              │
│  ┌─────────────────────────────────────┐ │
│  │  AES Engine (ECB/CBC/CTR/GCM)    │ │
│  │  SHA Engine (SHA-1/256/384/512)  │ │
│  │  RSA/ECC Engine (Sign/Verify)    │ │
│  │  TRNG (True Random Generator)    │ │
│  │  Key Storage (Secure RAM/Flash)  │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
Std_ReturnType Crypto_Init(const Crypto_ConfigType* configPtr);
Std_ReturnType Crypto_DeInit(void);

/* Key Management */
Std_ReturnType Crypto_KeyElementSet(uint32 keyId, uint32 keyElementId, 
                                    const uint8* keyPtr, uint32 keyLength);
Std_ReturnType Crypto_KeySetValid(uint32 keyId);
Std_ReturnType Crypto_KeyElementGet(uint32 keyId, uint32 keyElementId, 
                                    uint8* keyPtr, uint32* keyLengthPtr);
Std_ReturnType Crypto_KeyExchangeCalcPubVal(uint32 keyId, uint8* publicValuePtr, 
                                            uint32* publicValueLengthPtr);
Std_ReturnType Crypto_KeyExchangeCalcSecret(uint32 keyId, const uint8* partnerPublicValuePtr, 
                                            uint32 partnerPublicValueLength);

/* Random Number Generation */
Std_ReturnType Crypto_RandomSeed(uint32 keyId, const uint8* seedPtr, uint32 seedLength);
Std_ReturnType Crypto_KeyGenerate(uint32 keyId);

/* Symmetric Encryption/Decryption */
Std_ReturnType Crypto_Encrypt(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                              const uint8* plaintextPtr, uint32 plaintextLength, 
                              uint8* ciphertextPtr, uint32* ciphertextLengthPtr);
Std_ReturnType Crypto_Decrypt(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                              const uint8* ciphertextPtr, uint32 ciphertextLength, 
                              uint8* plaintextPtr, uint32* plaintextLengthPtr);

/* AEAD (Authenticated Encryption) */
Std_ReturnType Crypto_AEADEncrypt(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                  const uint8* plaintextPtr, uint32 plaintextLength, 
                                  const uint8* associatedDataPtr, uint32 associatedDataLength, 
                                  uint8* ciphertextPtr, uint32* ciphertextLengthPtr, 
                                  uint8* tagPtr, uint32* tagLengthPtr);
Std_ReturnType Crypto_AEADDecrypt(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                  const uint8* ciphertextPtr, uint32 ciphertextLength, 
                                  const uint8* associatedDataPtr, uint32 associatedDataLength, 
                                  const uint8* tagPtr, uint32 tagLength, 
                                  uint8* plaintextPtr, uint32* plaintextLengthPtr, 
                                  Crypto_VerifyResultType* verifyPtr);

/* Hashing */
Std_ReturnType Crypto_Hash(Crypto_AlgorithmFamilyType algorithmFamily, 
                           const uint8* dataPtr, uint32 dataLength, 
                           uint8* resultPtr, uint32* resultLengthPtr);

/* MAC/HMAC */
Std_ReturnType Crypto_MacGenerate(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                  const uint8* dataPtr, uint32 dataLength, 
                                  uint8* macPtr, uint32* macLengthPtr);
Std_ReturnType Crypto_MacVerify(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                const uint8* dataPtr, uint32 dataLength, 
                                const uint8* macPtr, uint32 macLength, 
                                Crypto_VerifyResultType* verifyPtr);

/* Signature */
Std_ReturnType Crypto_SignatureGenerate(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                        const uint8* dataPtr, uint32 dataLength, 
                                        uint8* signaturePtr, uint32* signatureLengthPtr);
Std_ReturnType Crypto_SignatureVerify(uint32 keyId, Crypto_AlgorithmModeType algorithmMode, 
                                      const uint8* dataPtr, uint32 dataLength, 
                                      const uint8* signaturePtr, uint32 signatureLength, 
                                      Crypto_VerifyResultType* verifyPtr);

/* Version Info */
void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### Data Types

```c
typedef uint8 Crypto_VerifyResultType;
#define CRYPTO_E_VER_OK      0x00U
#define CRYPTO_E_VER_NOT_OK  0x01U

typedef enum {
    CRYPTO_ALGOFAM_NOT_SET = 0x00,
    CRYPTO_ALGOFAM_SHA1,
    CRYPTO_ALGOFAM_SHA2_224,
    CRYPTO_ALGOFAM_SHA2_256,
    CRYPTO_ALGOFAM_SHA2_384,
    CRYPTO_ALGOFAM_SHA2_512,
    CRYPTO_ALGOFAM_SHA3_224,
    CRYPTO_ALGOFAM_SHA3_256,
    CRYPTO_ALGOFAM_SHA3_384,
    CRYPTO_ALGOFAM_SHA3_512,
    CRYPTO_ALGOFAM_RSA,
    CRYPTO_ALGOFAM_ECDSA,
    CRYPTO_ALGOFAM_AES
} Crypto_AlgorithmFamilyType;

typedef enum {
    CRYPTO_ALGOMODE_NOT_SET = 0x00,
    CRYPTO_ALGOMODE_ECB,
    CRYPTO_ALGOMODE_CBC,
    CRYPTO_ALGOMODE_CFB,
    CRYPTO_ALGOMODE_OFB,
    CRYPTO_ALGOMODE_CTR,
    CRYPTO_ALGOMODE_GCM,
    CRYPTO_ALGOMODE_CCM
} Crypto_AlgorithmModeType;

typedef enum {
    CRYPTO_OPERATIONMODE_START = 0x01,
    CRYPTO_OPERATIONMODE_UPDATE = 0x02,
    CRYPTO_OPERATIONMODE_FINISH = 0x04,
    CRYPTO_OPERATIONMODE_SINGLECALL = 0x07
} Crypto_OperationModeType;
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `CryptoKeyType` | enum | AES-128, AES-256, RSA-2048, ECC-P256 |
| `CryptoKeyElement` | struct | Key material, IV, nonce, etc. |
| `CryptoAlgorithmFamily` | enum | SHA, AES, RSA, ECC |
| `CryptoAlgorithmMode` | enum | ECB, CBC, CTR, GCM |
| `CryptoPrimitive` | struct | Encrypt, Decrypt, Hash, Sign, Verify |
| `CryptoQueueSize` | uint16 | Job queue size |
| `CryptoMaxKeySize` | uint16 | Maximum key size in bits |
| `CryptoDriverObject` | struct | Hardware object configuration |

## Usage Example

### AES Encryption

```c
#include "Crypto.h"
#include "Crypto_Cfg.h"

void Crypto_AesEncryptExample(void)
{
    Std_ReturnType status;
    uint8 plaintext[16] = "Hello, World!!!";
    uint8 ciphertext[16];
    uint32 cipherLen = 16;
    
    /* Set AES key (keyId 1, keyElementId 1) */
    uint8 aesKey[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    status = Crypto_KeyElementSet(1, 1, aesKey, 16);
    
    if (status == E_OK) {
        /* Make key valid */
        status = Crypto_KeySetValid(1);
    }
    
    if (status == E_OK) {
        /* Encrypt using AES-128-ECB */
        status = Crypto_Encrypt(1, CRYPTO_ALGOMODE_ECB, 
                                plaintext, 16, 
                                ciphertext, &cipherLen);
        
        if (status == E_OK) {
            /* Encryption successful */
        }
    }
}
```

### SHA-256 Hashing

```c
void Crypto_Sha256Example(void)
{
    Std_ReturnType status;
    uint8 data[] = "The quick brown fox jumps over the lazy dog";
    uint8 hash[32];
    uint32 hashLen = 32;
    
    /* Calculate SHA-256 hash */
    status = Crypto_Hash(CRYPTO_ALGOFAM_SHA2_256, 
                         data, sizeof(data) - 1, 
                         hash, &hashLen);
    
    if (status == E_OK) {
        /* Hash calculation successful */
        /* hash now contains 32-byte SHA-256 digest */
    }
}
```

### HMAC Generation

```c
void Crypto_HmacExample(void)
{
    Std_ReturnType status;
    uint8 data[] = "message to authenticate";
    uint8 hmac[32];
    uint32 hmacLen = 32;
    
    /* Set HMAC key */
    uint8 hmacKey[32] = { /* 256-bit key */ };
    Crypto_KeyElementSet(2, 1, hmacKey, 32);
    Crypto_KeySetValid(2);
    
    /* Generate HMAC-SHA256 */
    status = Crypto_MacGenerate(2, CRYPTO_ALGOMODE_NOT_SET, 
                                data, sizeof(data) - 1, 
                                hmac, &hmacLen);
}
```

### Random Number Generation

```c
void Crypto_RandomExample(void)
{
    Std_ReturnType status;
    uint8 randomData[16];
    
    /* Seed the random number generator */
    uint8 seed[32] = { /* entropy seed */ };
    status = Crypto_RandomSeed(3, seed, 32);
    
    if (status == E_OK) {
        /* Generate random key */
        status = Crypto_KeyGenerate(3);
        
        if (status == E_OK) {
            /* Retrieve random data */
            uint32 randomLen = 16;
            Crypto_KeyElementGet(3, 1, randomData, &randomLen);
        }
    }
}
```

## SHE Compliance

The driver supports SHE (Secure Hardware Extension) standard:

```c
/* SHE Key Slots */
#define SHE_KEY_1   0x01U   /* Master ECU Key */
#define SHE_KEY_2   0x02U   /* Boot MAC Key */
#define SHE_KEY_3   0x03U   /* Boot Key */
#define SHE_KEY_4   0x04U   /* Key 1 */
/* ... Keys 5-10 ... */
#define SHE_KEY_11  0x0BU   /* RAM Key */
#define SHE_KEY_12  0x0CU   /* Key 11 (Key Update) */

/* SHE Commands */
Std_ReturnType Crypto_SheLoadKey(uint32 keyId, const uint8* authKeyPtr, 
                                 const uint8* newKeyPtr, const uint8* counterPtr);
Std_ReturnType Crypto_SheLoadPlainKey(const uint8* keyPtr);
Std_ReturnType Crypto_SheExportRamKey(uint8* authKeyPtr, uint8* counterPtr);
```

## Error Handling

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `CRYPTO_E_UNINIT` | Driver not initialized | API check |
| `CRYPTO_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `CRYPTO_E_PARAM_HANDLE` | Invalid key/job ID | Parameter validation |
| `CRYPTO_E_PARAM_VALUE` | Invalid parameter value | Parameter validation |
| `CRYPTO_E_BUSY` | Hardware busy | Operation check |
| `CRYPTO_E_KEY_NOT_VALID` | Key not valid | Key check |
| `CRYPTO_E_KEY_SIZE_MISMATCH` | Key size mismatch | Key validation |
| `CRYPTO_E_QUEUE_FULL` | Job queue full | Queue check |
| `CRYPTO_E_SMALL_BUFFER` | Output buffer too small | Buffer check |
| `CRYPTO_E_VERIFICATION_FAILED` | Signature/MAC verification failed | Crypto check |

## Hardware Requirements

### Supported Hardware
- NXP S32K3xx (HSM/CSEc with AES-256, SHA-256, TRNG)
- Infineon AURIX TC3xx (HSM with AES, RSA, ECC)
- STM32H7 (SAES, CRYP, HASH, RNG)
- Renesas RH850/U2A (ICUM)

### Security Features
| Feature | Support |
|---------|---------|
| AES-128/256 | Hardware acceleration |
| SHA-1/256/384/512 | Hardware acceleration |
| RSA-2048/4096 | Hardware/software hybrid |
| ECC P-256/384 | Hardware acceleration |
| TRNG | Hardware-based |
| Secure Key Storage | Battery-backed SRAM or flash |
| Side-Channel Protection | Yes (hardware-dependent) |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler`
- `Det` - Error tracing
- `CryIf` - Crypto interface (upper layer)
- `Csm` - Crypto service manager (optional)

### Optional Modules
- `KeyM` - Key management
- `SecOC` - Secure onboard communication
- `E2E` - End-to-end protection

## References

- AUTOSAR SWS Crypto Driver
- SHE (Secure Hardware Extension) Specification v1.1
- HSM (Hardware Security Module) Standards
- FIPS 180-4 (SHA Standard)
- FIPS 197 (AES Standard)
- NIST SP 800-38A (Block Cipher Modes)
- NIST SP 800-90B (Random Bit Generators)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial release with AES/SHA support |
| 1.1.0 | 2024-04 | Added RSA/ECC support |
| 1.2.0 | 2024-08 | SHE compliance certification |
| 1.3.0 | 2024-11 | HSM integration |
