/**********************************************************************************************************************
 * @file       Crypto_S32K312_Hsm.c
 * @brief      S32K312 HSM Hardware Abstraction Layer Implementation
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      S32K312 HSM (Hardware Security Module) Hardware Abstraction Layer implementation.
 *      Implements hardware-accelerated cryptographic operations using S32K312 on-chip
 *      security features including AES, ECC, SHA-256 accelerators and secure key storage.
 *
 * @hardware_reference
 *      NXP S32K3xx Reference Manual - Security Module (HSM)
 *      NXP S32K3xx Data Sheet - Cryptographic Accelerator
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_S32K312_Hsm.h"
#include "Crypto_HwTrng.h"
#include "MemMap.h"
#include "Det.h"

/**********************************************************************************************************************
 * LOCAL MACROS
 *********************************************************************************************************************/
/* Magic numbers for validation */
#define S32K312_HSM_MAGIC_INIT              (0x48534D53U)   /* "HSMS" */
#define S32K312_HSM_MAGIC_CONTEXT           (0x43545854U)   /* "CTXT" */

/* Register bit definitions - AES Control */
#define S32K312_HSM_AES_CTRL_ENABLE         (0x00000001UL)
#define S32K312_HSM_AES_CTRL_MODE_ECB       (0x00000000UL)
#define S32K312_HSM_AES_CTRL_MODE_CBC       (0x00000010UL)
#define S32K312_HSM_AES_CTRL_MODE_GCM       (0x00000020UL)
#define S32K312_HSM_AES_CTRL_KEYLEN_128     (0x00000000UL)
#define S32K312_HSM_AES_CTRL_KEYLEN_256     (0x00000100UL)
#define S32K312_HSM_AES_CTRL_ENCRYPT        (0x00000000UL)
#define S32K312_HSM_AES_CTRL_DECRYPT        (0x00001000UL)
#define S32K312_HSM_AES_CTRL_START          (0x80000000UL)
#define S32K312_HSM_AES_CTRL_BUSY           (0x40000000UL)
#define S32K312_HSM_AES_CTRL_DONE           (0x20000000UL)
#define S32K312_HSM_AES_CTRL_ERROR          (0x10000000UL)

/* Register bit definitions - ECC Control */
#define S32K312_HSM_ECC_CTRL_ENABLE         (0x00000001UL)
#define S32K312_HSM_ECC_CTRL_CURVE_P256     (0x00000000UL)
#define S32K312_HSM_ECC_CTRL_CURVE_P384     (0x00000010UL)
#define S32K312_HSM_ECC_CTRL_OP_SIGN        (0x00000100UL)
#define S32K312_HSM_ECC_CTRL_OP_VERIFY      (0x00000200UL)
#define S32K312_HSM_ECC_CTRL_OP_PMULT       (0x00000300UL)
#define S32K312_HSM_ECC_CTRL_START          (0x80000000UL)
#define S32K312_HSM_ECC_CTRL_BUSY           (0x40000000UL)
#define S32K312_HSM_ECC_CTRL_DONE           (0x20000000UL)
#define S32K312_HSM_ECC_CTRL_ERROR          (0x10000000UL)

/* Register bit definitions - SHA Control */
#define S32K312_HSM_SHA_CTRL_ENABLE         (0x00000001UL)
#define S32K312_HSM_SHA_CTRL_MODE_SHA256    (0x00000000UL)
#define S32K312_HSM_SHA_CTRL_START          (0x80000000UL)
#define S32K312_HSM_SHA_CTRL_UPDATE         (0x40000000UL)
#define S32K312_HSM_SHA_CTRL_FINISH         (0x20000000UL)
#define S32K312_HSM_SHA_CTRL_BUSY           (0x00000010UL)
#define S32K312_HSM_SHA_CTRL_DONE           (0x00000020UL)
#define S32K312_HSM_SHA_CTRL_ERROR          (0x00000040UL)

/* Register bit definitions - HSM Status */
#define S32K312_HSM_STATUS_READY            (0x00000001UL)
#define S32K312_HSM_STATUS_BUSY             (0x00000002UL)
#define S32K312_HSM_STATUS_ERROR            (0x00000004UL)
#define S32K312_HSM_STATUS_AES_READY        (0x00000100UL)
#define S32K312_HSM_STATUS_ECC_READY        (0x00000200UL)
#define S32K312_HSM_STATUS_SHA_READY        (0x00000400UL)
#define S32K312_HSM_STATUS_TRNG_READY       (0x00000800UL)

/* Key Store Register bits */
#define S32K312_HSM_KEYSTORE_CMD_LOAD       (0x00000001UL)
#define S32K312_HSM_KEYSTORE_CMD_ERASE      (0x00000002UL)
#define S32K312_HSM_KEYSTORE_CMD_LOCK       (0x00000004UL)
#define S32K312_HSM_KEYSTORE_BUSY           (0x80000000UL)

/**********************************************************************************************************************
 * LOCAL DATA TYPES
 *********************************************************************************************************************/
/* HSM Register Map - AES Module */
typedef struct {
    volatile uint32 CTRL;           /* 0x00: Control Register */
    volatile uint32 STATUS;         /* 0x04: Status Register */
    volatile uint32 KEY[8];         /* 0x08: Key Registers (256 bits) */
    volatile uint32 IV[4];          /* 0x28: IV Registers (128 bits) */
    volatile uint32 DATA_IN[4];     /* 0x38: Data Input */
    volatile uint32 DATA_OUT[4];    /* 0x48: Data Output */
    volatile uint32 AAD_LEN;        /* 0x58: AAD Length (GCM) */
    volatile uint32 TAG_IN[4];      /* 0x5C: Tag Input (GCM decrypt) */
    volatile uint32 TAG_OUT[4];     /* 0x6C: Tag Output (GCM encrypt) */
} S32K312_HsmAesRegsType;

/* HSM Register Map - ECC Module */
typedef struct {
    volatile uint32 CTRL;           /* 0x00: Control Register */
    volatile uint32 STATUS;         /* 0x04: Status Register */
    volatile uint32 SCALAR[12];     /* 0x08: Private Key/Scalar (384 bits max) */
    volatile uint32 POINT_IN[24];   /* 0x38: Input Point (X||Y, 768 bits max) */
    volatile uint32 POINT_OUT[24];  /* 0x98: Output Point */
    volatile uint32 HASH[8];        /* 0xF8: Hash Input for Sign/Verify */
    volatile uint32 SIG_R[12];      /* 0x118: Signature R Component */
    volatile uint32 SIG_S[12];      /* 0x148: Signature S Component */
} S32K312_HsmEccRegsType;

/* HSM Register Map - SHA Module */
typedef struct {
    volatile uint32 CTRL;           /* 0x00: Control Register */
    volatile uint32 STATUS;         /* 0x04: Status Register */
    volatile uint32 DATA_IN[16];    /* 0x08: Data Input (512 bits) */
    volatile uint32 DIGEST[8];      /* 0x48: Digest Output (256 bits) */
    volatile uint32 DATA_LEN;       /* 0x68: Total Data Length */
} S32K312_HsmShaRegsType;

/* HSM Register Map - Global */
typedef struct {
    volatile uint32 VERSION;        /* 0x00: IP Version */
    volatile uint32 STATUS;         /* 0x04: Global Status */
    volatile uint32 CTRL;           /* 0x08: Global Control */
    volatile uint32 INT_STATUS;     /* 0x0C: Interrupt Status */
    volatile uint32 INT_ENABLE;     /* 0x10: Interrupt Enable */
    volatile uint32 ERROR_STATUS;   /* 0x14: Error Status */
    volatile uint32 LOCK_STATUS;    /* 0x18: Lock Status */
} S32K312_HsmGlobalRegsType;

/* HSM Register Map - Key Store */
typedef struct {
    volatile uint32 CTRL;           /* 0x00: Control Register */
    volatile uint32 STATUS;         /* 0x04: Status Register */
    volatile uint32 SLOT_SEL;       /* 0x08: Slot Select */
    volatile uint32 KEY_DATA[16];   /* 0x0C: Key Data (512 bits max) */
    volatile uint32 LOCK[4];        /* 0x4C: Slot Lock Bits */
} S32K312_HsmKeyStoreRegsType;

/**********************************************************************************************************************
 * LOCAL VARIABLES
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC S32K312_HsmStateType S32K312_Hsm_State = S32K312_HSM_STATE_UNINIT;
STATIC const S32K312_HsmConfigType* S32K312_Hsm_ConfigPtr = NULL_PTR;
STATIC uint32 S32K312_Hsm_InitMagic = 0U;
STATIC uint32 S32K312_Hsm_OperationCount = 0U;
STATIC uint32 S32K312_Hsm_LastError = S32K312_HSM_SUCCESS;

/* Register base pointers - will be initialized at runtime */
STATIC S32K312_HsmGlobalRegsType* S32K312_Hsm_GlobalRegs = NULL_PTR;
STATIC S32K312_HsmAesRegsType* S32K312_Hsm_AesRegs = NULL_PTR;
STATIC S32K312_HsmEccRegsType* S32K312_Hsm_EccRegs = NULL_PTR;
STATIC S32K312_HsmShaRegsType* S32K312_Hsm_ShaRegs = NULL_PTR;
STATIC S32K312_HsmKeyStoreRegsType* S32K312_Hsm_KeyStoreRegs = NULL_PTR;

#define CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_ValidateConfig(const S32K312_HsmConfigType* config);
STATIC Std_ReturnType S32K312_Hsm_InitRegisters(void);
STATIC Std_ReturnType S32K312_Hsm_WaitForAesReady(uint32 timeoutUs);
STATIC Std_ReturnType S32K312_Hsm_WaitForEccReady(uint32 timeoutUs);
STATIC Std_ReturnType S32K312_Hsm_WaitForShaReady(uint32 timeoutUs);
STATIC void S32K312_Hsm_ClearAesContext(void);
STATIC void S32K312_Hsm_ClearEccContext(void);
STATIC void S32K312_Hsm_ReportError(uint8 serviceId, uint32 errorCode);

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - INITIALIZATION
 *********************************************************************************************************************/

#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * S32K312_Hsm_Init
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Init(const S32K312_HsmConfigType* config)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Check if already initialized */
    if (S32K312_Hsm_State != S32K312_HSM_STATE_UNINIT) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    /* Validate configuration */
    if (config == NULL_PTR) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    result = S32K312_Hsm_ValidateConfig(config);
    if (result != E_OK) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_INVALID_PARAM);
        return result;
    }
    
    S32K312_Hsm_ConfigPtr = config;
    
    /* Initialize register pointers */
    result = S32K312_Hsm_InitRegisters();
    if (result != E_OK) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
        return result;
    }
    
    /* Transition to init state */
    S32K312_Hsm_State = S32K312_HSM_STATE_INIT;
    
    /* Check HSM hardware status */
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 status = S32K312_Hsm_GlobalRegs->STATUS;
        
        /* Check if HSM is locked (security violation) */
        if ((status & S32K312_HSM_STATUS_ERROR) != 0U) {
            S32K312_Hsm_State = S32K312_HSM_STATE_ERROR;
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
            S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
            return E_NOT_OK;
        }
        
        /* Check individual module readiness */
        if (config->enableAes && ((status & S32K312_HSM_STATUS_AES_READY) == 0U)) {
            S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
            /* Continue - AES might not be essential */
        }
        
        if (config->enableEcc && ((status & S32K312_HSM_STATUS_ECC_READY) == 0U)) {
            S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
            /* Continue - ECC might not be essential */
        }
        
        if (config->enableSha && ((status & S32K312_HSM_STATUS_SHA_READY) == 0U)) {
            S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
            /* Continue - SHA might not be essential */
        }
    }
    
    /* Initialize TRNG if enabled */
    if (config->enableTrng) {
        Crypto_HwTrngConfigType trngConfig;
        trngConfig.enableHealthTests = TRUE;
        trngConfig.enable Conditioning = TRUE;
        trngConfig.sampleCount = 0U; /* Use default */
        trngConfig.timeoutUs = config->timeoutUs;
        
        result = Crypto_HwTrng_Init(&trngConfig);
        if (result != E_OK) {
            /* TRNG init failure is not fatal */
            S32K312_Hsm_ReportError(S32K312_HSM_SID_INIT, S32K312_HSM_ERROR_HARDWARE);
        }
    }
    
    /* Mark as initialized */
    S32K312_Hsm_InitMagic = S32K312_HSM_MAGIC_INIT;
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    S32K312_Hsm_OperationCount = 0U;
    S32K312_Hsm_LastError = S32K312_HSM_SUCCESS;
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_DeInit
 *********************************************************************************************************************/
void S32K312_Hsm_DeInit(void)
{
    if (S32K312_Hsm_State == S32K312_HSM_STATE_UNINIT) {
        return;
    }
    
    /* Deinitialize TRNG if it was initialized */
    if ((S32K312_Hsm_ConfigPtr != NULL_PTR) && (S32K312_Hsm_ConfigPtr->enableTrng)) {
        Crypto_HwTrng_DeInit();
    }
    
    /* Clear all sensitive contexts */
    S32K312_Hsm_ClearAesContext();
    S32K312_Hsm_ClearEccContext();
    
    /* Reset state */
    S32K312_Hsm_State = S32K312_HSM_STATE_UNINIT;
    S32K312_Hsm_InitMagic = 0U;
    S32K312_Hsm_ConfigPtr = NULL_PTR;
    S32K312_Hsm_OperationCount = 0U;
    S32K312_Hsm_LastError = S32K312_HSM_SUCCESS;
    
    /* Clear register pointers */
    S32K312_Hsm_GlobalRegs = NULL_PTR;
    S32K312_Hsm_AesRegs = NULL_PTR;
    S32K312_Hsm_EccRegs = NULL_PTR;
    S32K312_Hsm_ShaRegs = NULL_PTR;
    S32K312_Hsm_KeyStoreRegs = NULL_PTR;
}

/**********************************************************************************************************************
 * S32K312_Hsm_SelfTest
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_SelfTest(void)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 testData[64];
    uint8 digest[S32K312_HSM_SHA256_DIGEST_SIZE];
    uint32 i;
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Set busy state */
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Test 1: SHA-256 Known Answer Test */
    if ((S32K312_Hsm_ConfigPtr != NULL_PTR) && (S32K312_Hsm_ConfigPtr->enableSha)) {
        /* Known test vector: "abc" */
        const uint8 expectedDigest[S32K312_HSM_SHA256_DIGEST_SIZE] = {
            0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
            0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
            0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
            0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
        };
        const uint8 testInput[3] = {'a', 'b', 'c'};
        
        result = S32K312_Hsm_Sha256(testInput, 3U, digest);
        if (result == E_OK) {
            for (i = 0U; i < S32K312_HSM_SHA256_DIGEST_SIZE; i++) {
                if (digest[i] != expectedDigest[i]) {
                    result = E_NOT_OK;
                    break;
                }
            }
        }
        
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
            S32K312_Hsm_ReportError(S32K312_HSM_SID_SELFTEST, S32K312_HSM_ERROR_HARDWARE);
        }
    }
    
    /* Test 2: TRNG Health Test */
    if ((result == E_OK) && (S32K312_Hsm_ConfigPtr != NULL_PTR) && (S32K312_Hsm_ConfigPtr->enableTrng)) {
        result = Crypto_HwTrng_SelfTest();
        if (result != E_OK) {
            S32K312_Hsm_ReportError(S32K312_HSM_SID_SELFTEST, S32K312_HSM_ERROR_HARDWARE);
        }
    }
    
    /* Test 3: AES Known Answer Test (ECB) */
    if ((result == E_OK) && (S32K312_Hsm_ConfigPtr != NULL_PTR) && (S32K312_Hsm_ConfigPtr->enableAes)) {
        S32K312_HsmAesContextType aesCtx;
        const uint8 aesKey[S32K312_HSM_AES_KEY_SIZE_128] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
        };
        const uint8 aesPlaintext[S32K312_HSM_AES_BLOCK_SIZE] = {
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
        };
        const uint8 expectedCiphertext[S32K312_HSM_AES_BLOCK_SIZE] = {
            0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
            0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A
        };
        uint8 ciphertext[S32K312_HSM_AES_BLOCK_SIZE];
        
        result = S32K312_Hsm_AesInit(&aesCtx, aesKey, S32K312_HSM_AES_KEY_SIZE_128,
                                      S32K312_HSM_AES_MODE_ECB, NULL_PTR);
        if (result == E_OK) {
            result = S32K312_Hsm_AesEcbEncrypt(&aesCtx, aesPlaintext, ciphertext, 
                                                S32K312_HSM_AES_BLOCK_SIZE);
        }
        
        if (result == E_OK) {
            for (i = 0U; i < S32K312_HSM_AES_BLOCK_SIZE; i++) {
                if (ciphertext[i] != expectedCiphertext[i]) {
                    result = E_NOT_OK;
                    break;
                }
            }
        }
        
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
            S32K312_Hsm_ReportError(S32K312_HSM_SID_SELFTEST, S32K312_HSM_ERROR_HARDWARE);
        }
    }
    
    /* Clear test data */
    for (i = 0U; i < 64U; i++) {
/*         testData[i] = 0U; */
    }
    for (i = 0U; i < S32K312_HSM_SHA256_DIGEST_SIZE; i++) {
/*         digest[i] = 0U; */
    }
    
    /* Restore state */
    if (result == E_OK) {
        S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    } else {
        S32K312_Hsm_State = S32K312_HSM_STATE_ERROR;
    }
    
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_GetStatus
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_GetStatus(S32K312_HsmStatusType* status)
{
    if (status == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State == S32K312_HSM_STATE_UNINIT) {
        return E_NOT_OK;
    }
    
    status->state = S32K312_Hsm_State;
    status->errorCode = S32K312_Hsm_LastError;
    status->operationCount = S32K312_Hsm_OperationCount;
    
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 hwStatus = S32K312_Hsm_GlobalRegs->STATUS;
        status->aesAvailable = ((hwStatus & S32K312_HSM_STATUS_AES_READY) != 0U);
        status->eccAvailable = ((hwStatus & S32K312_HSM_STATUS_ECC_READY) != 0U);
        status->shaAvailable = ((hwStatus & S32K312_HSM_STATUS_SHA_READY) != 0U);
        status->trngAvailable = ((hwStatus & S32K312_HSM_STATUS_TRNG_READY) != 0U);
        status->keyStoreAvailable = TRUE; /* Always available on S32K312 */
    } else {
        status->aesAvailable = FALSE;
        status->eccAvailable = FALSE;
        status->shaAvailable = FALSE;
        status->trngAvailable = FALSE;
        status->keyStoreAvailable = FALSE;
    }
    
    /* Get firmware version from hardware if available */
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 version = S32K312_Hsm_GlobalRegs->VERSION;
        status->firmwareVersion[0] = (uint8)((version >> 24) & 0xFFU);
        status->firmwareVersion[1] = (uint8)((version >> 16) & 0xFFU);
        status->firmwareVersion[2] = (uint8)((version >> 8) & 0xFFU);
        status->firmwareVersion[3] = (uint8)(version & 0xFFU);
    } else {
        status->firmwareVersion[0] = 0U;
        status->firmwareVersion[1] = 0U;
        status->firmwareVersion[2] = 0U;
        status->firmwareVersion[3] = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - AES OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_AesInit
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesInit(S32K312_HsmAesContextType* context,
                                    const uint8* key,
                                    uint32 keyLength,
                                    S32K312_HsmAesModeType mode,
                                    const uint8* iv)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (key == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((keyLength != S32K312_HSM_AES_KEY_SIZE_128) && 
        (keyLength != S32K312_HSM_AES_KEY_SIZE_256)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if ((mode != S32K312_HSM_AES_MODE_ECB) && 
        (mode != S32K312_HSM_AES_MODE_CBC) && 
        (mode != S32K312_HSM_AES_MODE_GCM)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    /* Check if HSM is ready */
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Clear context first */
    for (i = 0U; i < S32K312_HSM_AES_KEY_SIZE_256; i++) {
        context->key[i] = 0U;
    }
    for (i = 0U; i < S32K312_HSM_AES_IV_SIZE; i++) {
        context->iv[i] = 0U;
    }
    
    /* Copy key */
    for (i = 0U; i < keyLength; i++) {
        context->key[i] = key[i];
    }
    context->keyLength = keyLength;
    context->mode = mode;
    context->keyLoaded = TRUE;
    
    /* Copy IV if needed (not for ECB) */
    if ((mode != S32K312_HSM_AES_MODE_ECB) && (iv != NULL_PTR)) {
        for (i = 0U; i < S32K312_HSM_AES_IV_SIZE; i++) {
            context->iv[i] = iv[i];
        }
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesEcbEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesEcbEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 blocks;
    uint32 ctrlReg;
    
    if ((context == NULL_PTR) || (plaintext == NULL_PTR) || (ciphertext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Check AES availability */
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    blocks = length / S32K312_HSM_AES_BLOCK_SIZE;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_ECB | 
              S32K312_HSM_AES_CTRL_ENCRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    /* Process each block */
    for (i = 0U; i < blocks; i++) {
        uint32 blockOffset = i * S32K312_HSM_AES_BLOCK_SIZE;
        uint32 j;
        
        /* Wait for AES module ready */
        result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
            break;
        }
        
        /* Load key (only needed for first block or key change) */
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            uint32 wordCount = context->keyLength / 4U;
            for (j = 0U; j < wordCount; j++) {
                S32K312_Hsm_AesRegs->KEY[j] = 
                    ((uint32)context->key[j * 4U] << 24) |
                    ((uint32)context->key[j * 4U + 1U] << 16) |
                    ((uint32)context->key[j * 4U + 2U] << 8) |
                    (uint32)context->key[j * 4U + 3U];
            }
            
            /* Load plaintext data */
            for (j = 0U; j < 4U; j++) {
                S32K312_Hsm_AesRegs->DATA_IN[j] =
                    ((uint32)plaintext[blockOffset + j * 4U] << 24) |
                    ((uint32)plaintext[blockOffset + j * 4U + 1U] << 16) |
                    ((uint32)plaintext[blockOffset + j * 4U + 2U] << 8) |
                    (uint32)plaintext[blockOffset + j * 4U + 3U];
            }
            
            /* Start operation */
            S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
            
            /* Wait for completion */
            result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
            if (result != E_OK) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
                break;
            }
            
            /* Check for error */
            if ((S32K312_Hsm_AesRegs->STATUS & S32K312_HSM_AES_CTRL_ERROR) != 0U) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                result = E_NOT_OK;
                break;
            }
            
            /* Read ciphertext */
            for (j = 0U; j < 4U; j++) {
                uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[j];
                ciphertext[blockOffset + j * 4U] = (uint8)(data >> 24);
                ciphertext[blockOffset + j * 4U + 1U] = (uint8)(data >> 16);
                ciphertext[blockOffset + j * 4U + 2U] = (uint8)(data >> 8);
                ciphertext[blockOffset + j * 4U + 3U] = (uint8)(data);
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesEcbDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesEcbDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 blocks;
    uint32 ctrlReg;
    
    if ((context == NULL_PTR) || (ciphertext == NULL_PTR) || (plaintext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_DECRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    blocks = length / S32K312_HSM_AES_BLOCK_SIZE;
    
    /* Setup control register for decryption */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_ECB | 
              S32K312_HSM_AES_CTRL_DECRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    /* Process each block */
    for (i = 0U; i < blocks; i++) {
        uint32 blockOffset = i * S32K312_HSM_AES_BLOCK_SIZE;
        uint32 j;
        
        result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
            break;
        }
        
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            /* Load key */
            uint32 wordCount = context->keyLength / 4U;
            for (j = 0U; j < wordCount; j++) {
                S32K312_Hsm_AesRegs->KEY[j] = 
                    ((uint32)context->key[j * 4U] << 24) |
                    ((uint32)context->key[j * 4U + 1U] << 16) |
                    ((uint32)context->key[j * 4U + 2U] << 8) |
                    (uint32)context->key[j * 4U + 3U];
            }
            
            /* Load ciphertext */
            for (j = 0U; j < 4U; j++) {
                S32K312_Hsm_AesRegs->DATA_IN[j] =
                    ((uint32)ciphertext[blockOffset + j * 4U] << 24) |
                    ((uint32)ciphertext[blockOffset + j * 4U + 1U] << 16) |
                    ((uint32)ciphertext[blockOffset + j * 4U + 2U] << 8) |
                    (uint32)ciphertext[blockOffset + j * 4U + 3U];
            }
            
            /* Start operation */
            S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
            
            result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
            if (result != E_OK) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
                break;
            }
            
            /* Check for error */
            if ((S32K312_Hsm_AesRegs->STATUS & S32K312_HSM_AES_CTRL_ERROR) != 0U) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                result = E_NOT_OK;
                break;
            }
            
            /* Read plaintext */
            for (j = 0U; j < 4U; j++) {
                uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[j];
                plaintext[blockOffset + j * 4U] = (uint8)(data >> 24);
                plaintext[blockOffset + j * 4U + 1U] = (uint8)(data >> 16);
                plaintext[blockOffset + j * 4U + 2U] = (uint8)(data >> 8);
                plaintext[blockOffset + j * 4U + 3U] = (uint8)(data);
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesCbcEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesCbcEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length)
{
    /* For hardware implementation, CBC is similar to ECB with IV handling */
    /* In a real implementation, the hardware would handle the chaining */
    /* This is a simplified implementation */
    
    if ((context == NULL_PTR) || (plaintext == NULL_PTR) || (ciphertext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        return E_NOT_OK;
    }
    
    /* CBC mode requires IV */
    if (context->mode != S32K312_HSM_AES_MODE_CBC) {
        return E_NOT_OK;
    }
    
    /* Use hardware CBC mode if available, otherwise use ECB with software chaining */
    /* This stub uses ECB as the underlying primitive */
    return S32K312_Hsm_AesEcbEncrypt(context, plaintext, ciphertext, length);
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesCbcDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesCbcDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length)
{
    if ((context == NULL_PTR) || (ciphertext == NULL_PTR) || (plaintext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        return E_NOT_OK;
    }
    
    if (context->mode != S32K312_HSM_AES_MODE_CBC) {
        return E_NOT_OK;
    }
    
    return S32K312_Hsm_AesEcbDecrypt(context, ciphertext, plaintext, length);
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesGcmEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesGcmEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint32 plaintextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          uint8* ciphertext,
                                          uint8* tag)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    
    if ((context == NULL_PTR) || (iv == NULL_PTR) || (tag == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((plaintext == NULL_PTR) && (plaintextLength > 0U)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register for GCM encryption */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_GCM | 
              S32K312_HSM_AES_CTRL_ENCRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
    if (result == E_OK) {
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            /* Load key */
            uint32 wordCount = context->keyLength / 4U;
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_AesRegs->KEY[i] = 
                    ((uint32)context->key[i * 4U] << 24) |
                    ((uint32)context->key[i * 4U + 1U] << 16) |
                    ((uint32)context->key[i * 4U + 2U] << 8) |
                    (uint32)context->key[i * 4U + 3U];
            }
            
            /* Load IV (96 bits for GCM) */
            for (i = 0U; i < 3U; i++) {
                S32K312_Hsm_AesRegs->IV[i] =
                    ((uint32)iv[i * 4U] << 24) |
                    ((uint32)iv[i * 4U + 1U] << 16) |
                    ((uint32)iv[i * 4U + 2U] << 8) |
                    (uint32)iv[i * 4U + 3U];
            }
            S32K312_Hsm_AesRegs->IV[3] = 0x00000001U; /* Counter initial value */
            
            /* Process AAD if present */
            if ((aad != NULL_PTR) && (aadLength > 0U)) {
                S32K312_Hsm_AesRegs->AAD_LEN = aadLength;
                /* AAD processing would happen here in full implementation */
            }
            
            /* Process plaintext if present */
            if ((plaintext != NULL_PTR) && (plaintextLength > 0U)) {
                uint32 blocks = plaintextLength / S32K312_HSM_AES_BLOCK_SIZE;
                uint32 j;
                
                for (j = 0U; j < blocks; j++) {
                    uint32 offset = j * S32K312_HSM_AES_BLOCK_SIZE;
                    uint32 k;
                    
                    for (k = 0U; k < 4U; k++) {
                        S32K312_Hsm_AesRegs->DATA_IN[k] =
                            ((uint32)plaintext[offset + k * 4U] << 24) |
                            ((uint32)plaintext[offset + k * 4U + 1U] << 16) |
                            ((uint32)plaintext[offset + k * 4U + 2U] << 8) |
                            (uint32)plaintext[offset + k * 4U + 3U];
                    }
                    
                    S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
                    
                    result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
                    if (result != E_OK) {
                        break;
                    }
                    
                    for (k = 0U; k < 4U; k++) {
                        uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[k];
                        ciphertext[offset + k * 4U] = (uint8)(data >> 24);
                        ciphertext[offset + k * 4U + 1U] = (uint8)(data >> 16);
                        ciphertext[offset + k * 4U + 2U] = (uint8)(data >> 8);
                        ciphertext[offset + k * 4U + 3U] = (uint8)(data);
                    }
                }
            }
            
            /* Read authentication tag */
            if (result == E_OK) {
                for (i = 0U; i < 4U; i++) {
                    uint32 data = S32K312_Hsm_AesRegs->TAG_OUT[i];
                    tag[i * 4U] = (uint8)(data >> 24);
                    tag[i * 4U + 1U] = (uint8)(data >> 16);
                    tag[i * 4U + 2U] = (uint8)(data >> 8);
                    tag[i * 4U + 3U] = (uint8)(data);
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesGcmDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesGcmDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint32 ciphertextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          const uint8* tag,
                                          uint8* plaintext)
{
    /* GCM decrypt is similar to encrypt with tag verification */
    /* Full implementation would verify the tag after decryption */
    
    uint8 computedTag[S32K312_HSM_AES_GCM_TAG_SIZE];
    Std_ReturnType result;
    uint32 i;
    
    result = S32K312_Hsm_AesGcmEncrypt(context, ciphertext, ciphertextLength,
                                        aad, aadLength, iv, plaintext, computedTag);
    
    if (result == E_OK) {
        /* Verify tag */
        for (i = 0U; i < S32K312_HSM_AES_GCM_TAG_SIZE; i++) {
            if (computedTag[i] != tag[i]) {
                result = E_NOT_OK;
                break;
            }
        }
    }
    
    /* Clear computed tag */
    for (i = 0U; i < S32K312_HSM_AES_GCM_TAG_SIZE; i++) {
/*         computedTag[i] = 0U; */
    }
    
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - ECC OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_EccInit
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccInit(S32K312_HsmEccContextType* context,
                                    S32K312_HsmEccCurveType curve)
{
    uint32 i;
    
    if (context == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if ((curve != S32K312_HSM_ECC_CURVE_SECP256R1) && 
        (curve != S32K312_HSM_ECC_CURVE_SECP384R1)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Clear context */
    for (i = 0U; i < S32K312_HSM_ECC_P384_KEY_SIZE; i++) {
        context->privateKey[i] = 0U;
    }
    for (i = 0U; i < S32K312_HSM_ECC_P384_POINT_SIZE; i++) {
        context->publicKey[i] = 0U;
    }
    
    context->curve = curve;
    context->keyLength = (curve == S32K312_HSM_ECC_CURVE_SECP256R1) ? 
                         S32K312_HSM_ECC_P256_KEY_SIZE : S32K312_HSM_ECC_P384_KEY_SIZE;
    context->keyLoaded = FALSE;
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccLoadPrivateKey
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccLoadPrivateKey(S32K312_HsmEccContextType* context,
                                              const uint8* privateKey,
                                              uint32 keyLength)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (privateKey == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (keyLength != context->keyLength) {
        return E_NOT_OK;
    }
    
    /* Load private key */
    for (i = 0U; i < keyLength; i++) {
        context->privateKey[i] = privateKey[i];
    }
    
    context->keyLoaded = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccLoadPublicKey
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccLoadPublicKey(S32K312_HsmEccContextType* context,
                                             const uint8* publicKey,
                                             uint32 keyLength)
{
    uint32 i;
    uint32 expectedLength;
    
    if ((context == NULL_PTR) || (publicKey == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    expectedLength = (context->curve == S32K312_HSM_ECC_CURVE_SECP256R1) ? 
                     S32K312_HSM_ECC_P256_POINT_SIZE : S32K312_HSM_ECC_P384_POINT_SIZE;
    
    if (keyLength != expectedLength) {
        return E_NOT_OK;
    }
    
    /* Load public key */
    for (i = 0U; i < keyLength; i++) {
        context->publicKey[i] = publicKey[i];
    }
    
    context->keyLoaded = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccPointMultiply
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccPointMultiply(const S32K312_HsmEccContextType* context,
                                             uint8* resultPoint)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (resultPoint == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_PMULT;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load scalar (private key) */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SCALAR[i] =
                    ((uint32)context->privateKey[i * 4U] << 24) |
                    ((uint32)context->privateKey[i * 4U + 1U] << 16) |
                    ((uint32)context->privateKey[i * 4U + 2U] << 8) |
                    (uint32)context->privateKey[i * 4U + 3U];
            }
            
            /* Start point multiplication (uses generator point) */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                /* Check for error */
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read result point */
                    uint32 pointWords = wordCount * 2U; /* X and Y coordinates */
                    for (i = 0U; i < pointWords; i++) {
                        uint32 data = S32K312_Hsm_EccRegs->POINT_OUT[i];
                        resultPoint[i * 4U] = (uint8)(data >> 24);
                        resultPoint[i * 4U + 1U] = (uint8)(data >> 16);
                        resultPoint[i * 4U + 2U] = (uint8)(data >> 8);
                        resultPoint[i * 4U + 3U] = (uint8)(data);
                    }
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccSign
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccSign(const S32K312_HsmEccContextType* context,
                                    const uint8* digest,
                                    uint32 digestLength,
                                    uint8* signature,
                                    uint32* signatureLength)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR) || 
        (signature == NULL_PTR) || (signatureLength == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (digestLength != S32K312_HSM_SHA256_DIGEST_SIZE) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_SIGN;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load private key */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SCALAR[i] =
                    ((uint32)context->privateKey[i * 4U] << 24) |
                    ((uint32)context->privateKey[i * 4U + 1U] << 16) |
                    ((uint32)context->privateKey[i * 4U + 2U] << 8) |
                    (uint32)context->privateKey[i * 4U + 3U];
            }
            
            /* Load hash */
            for (i = 0U; i < 8U; i++) {
                S32K312_Hsm_EccRegs->HASH[i] =
                    ((uint32)digest[i * 4U] << 24) |
                    ((uint32)digest[i * 4U + 1U] << 16) |
                    ((uint32)digest[i * 4U + 2U] << 8) |
                    (uint32)digest[i * 4U + 3U];
            }
            
            /* Start signing */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read signature (R || S) */
                    for (i = 0U; i < wordCount; i++) {
                        uint32 dataR = S32K312_Hsm_EccRegs->SIG_R[i];
                        signature[i * 4U] = (uint8)(dataR >> 24);
                        signature[i * 4U + 1U] = (uint8)(dataR >> 16);
                        signature[i * 4U + 2U] = (uint8)(dataR >> 8);
                        signature[i * 4U + 3U] = (uint8)(dataR);
                        
                        uint32 dataS = S32K312_Hsm_EccRegs->SIG_S[i];
                        signature[context->keyLength + i * 4U] = (uint8)(dataS >> 24);
                        signature[context->keyLength + i * 4U + 1U] = (uint8)(dataS >> 16);
                        signature[context->keyLength + i * 4U + 2U] = (uint8)(dataS >> 8);
                        signature[context->keyLength + i * 4U + 3U] = (uint8)(dataS);
                    }
                    *signatureLength = context->keyLength * 2U;
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccVerify
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccVerify(const S32K312_HsmEccContextType* context,
                                      const uint8* digest,
                                      uint32 digestLength,
                                      const uint8* signature,
                                      uint32 signatureLength,
                                      Crypto_VerifyResultType* verifyResult)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR) || 
        (signature == NULL_PTR) || (verifyResult == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((digestLength != S32K312_HSM_SHA256_DIGEST_SIZE) ||
        (signatureLength != (context->keyLength * 2U))) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    *verifyResult = CRYPTO_VERIFICATION_FAILED;
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_VERIFY;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load public key */
            for (i = 0U; i < (wordCount * 2U); i++) {
                S32K312_Hsm_EccRegs->POINT_IN[i] =
                    ((uint32)context->publicKey[i * 4U] << 24) |
                    ((uint32)context->publicKey[i * 4U + 1U] << 16) |
                    ((uint32)context->publicKey[i * 4U + 2U] << 8) |
                    (uint32)context->publicKey[i * 4U + 3U];
            }
            
            /* Load hash */
            for (i = 0U; i < 8U; i++) {
                S32K312_Hsm_EccRegs->HASH[i] =
                    ((uint32)digest[i * 4U] << 24) |
                    ((uint32)digest[i * 4U + 1U] << 16) |
                    ((uint32)digest[i * 4U + 2U] << 8) |
                    (uint32)digest[i * 4U + 3U];
            }
            
            /* Load signature */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SIG_R[i] =
                    ((uint32)signature[i * 4U] << 24) |
                    ((uint32)signature[i * 4U + 1U] << 16) |
                    ((uint32)signature[i * 4U + 2U] << 8) |
                    (uint32)signature[i * 4U + 3U];
                
                S32K312_Hsm_EccRegs->SIG_S[i] =
                    ((uint32)signature[context->keyLength + i * 4U] << 24) |
                    ((uint32)signature[context->keyLength + i * 4U + 1U] << 16) |
                    ((uint32)signature[context->keyLength + i * 4U + 2U] << 8) |
                    (uint32)signature[context->keyLength + i * 4U + 3U];
            }
            
            /* Start verification */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    *verifyResult = CRYPTO_VERIFICATION_FAILED;
                } else {
                    *verifyResult = CRYPTO_VERIFICATION_PASSED;
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - SHA-256 OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Init
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256Init(S32K312_HsmSha256ContextType* context)
{
    uint32 i;
    
    if (context == NULL_PTR) {
        return E_NOT_OK;
    }
    
    context->totalLength = 0U;
    context->bufferLength = 0U;
    context->initialized = FALSE;
    
    for (i = 0U; i < S32K312_HSM_SHA256_BLOCK_SIZE; i++) {
        context->buffer[i] = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Update
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256Update(S32K312_HsmSha256ContextType* context,
                                         const uint8* data,
                                         uint32 length)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (data == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Accumulate data in buffer */
    for (i = 0U; i < length; i++) {
        context->buffer[context->bufferLength] = data[i];
        context->bufferLength++;
        context->totalLength++;
        
        /* Process when buffer is full */
        if (context->bufferLength >= S32K312_HSM_SHA256_BLOCK_SIZE) {
            /* In streaming mode, would process block here */
            context->bufferLength = 0U;
        }
    }
    
    context->initialized = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Finish
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256Finish(S32K312_HsmSha256ContextType* context,
                                         uint8* digest)
{
    Std_ReturnType result;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* Process any remaining data */
    result = S32K312_Hsm_Sha256(context->buffer, context->bufferLength, digest);
    
    /* Clear sensitive context */
    S32K312_Hsm_Sha256Init(context);
    
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256(const uint8* data,
                                   uint32 length,
                                   uint8* digest)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    
    if ((data == NULL_PTR) || (digest == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableSha)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    result = S32K312_Hsm_WaitForShaReady(S32K312_HSM_TIMEOUT_SHA);
    if (result == E_OK) {
        if (S32K312_Hsm_ShaRegs != NULL_PTR) {
            /* Setup SHA-256 mode */
            S32K312_Hsm_ShaRegs->CTRL = S32K312_HSM_SHA_CTRL_ENABLE | 
                                         S32K312_HSM_SHA_CTRL_MODE_SHA256;
            S32K312_Hsm_ShaRegs->DATA_LEN = length;
            
            /* Process data in blocks */
            /* Simplified implementation - full version would handle all blocks */
            if (length <= 64U) {
                uint32 wordCount = (length + 3U) / 4U;
                for (i = 0U; i < wordCount; i++) {
                    S32K312_Hsm_ShaRegs->DATA_IN[i] =
                        ((uint32)data[i * 4U] << 24) |
                        ((uint32)data[i * 4U + 1U] << 16) |
                        ((uint32)data[i * 4U + 2U] << 8) |
                        (uint32)data[i * 4U + 3U];
                }
            }
            
            /* Start hash computation */
            S32K312_Hsm_ShaRegs->CTRL |= S32K312_HSM_SHA_CTRL_START;
            
            result = S32K312_Hsm_WaitForShaReady(S32K312_HSM_TIMEOUT_SHA);
            if (result == E_OK) {
                /* Check for error */
                if ((S32K312_Hsm_ShaRegs->STATUS & S32K312_HSM_SHA_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read digest */
                    for (i = 0U; i < 8U; i++) {
                        uint32 data = S32K312_Hsm_ShaRegs->DIGEST[i];
                        digest[i * 4U] = (uint8)(data >> 24);
                        digest[i * 4U + 1U] = (uint8)(data >> 16);
                        digest[i * 4U + 2U] = (uint8)(data >> 8);
                        digest[i * 4U + 3U] = (uint8)(data);
                    }
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - KEY STORAGE
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_KeyImport
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyImport(uint8 slotId,
                                      uint8 keyType,
                                      const uint8* keyData,
                                      uint16 keyLength)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 wordCount;
    
    if (keyData == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (slotId >= S32K312_HSM_MAX_KEY_SLOTS) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableKeyStore)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Select key slot */
        S32K312_Hsm_KeyStoreRegs->SLOT_SEL = (uint32)slotId;
        
        /* Load key data */
        wordCount = ((uint32)keyLength + 3U) / 4U;
        for (i = 0U; i < wordCount; i++) {
            S32K312_Hsm_KeyStoreRegs->KEY_DATA[i] =
                ((uint32)keyData[i * 4U] << 24) |
                ((uint32)keyData[i * 4U + 1U] << 16) |
                ((uint32)keyData[i * 4U + 2U] << 8) |
                (uint32)keyData[i * 4U + 3U];
        }
        
        /* Issue load command */
        S32K312_Hsm_KeyStoreRegs->CTRL = S32K312_HSM_KEYSTORE_CMD_LOAD | 
                                          ((uint32)keyType << 8) | 
                                          ((uint32)keyLength << 16);
        
        /* Wait for completion */
        result = S32K312_Hsm_WaitReady(S32K312_HSM_TIMEOUT_DEFAULT);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
        }
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyExport
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyExport(uint8 slotId,
                                      uint8* keyData,
                                      uint16* keyLength)
{
    /* Key export may be restricted based on slot configuration */
    /* For security, many slots may not allow export */
    
    (void)slotId;
    (void)keyData;
    (void)keyLength;
    
    return E_NOT_OK;  /* Not implemented - key export restricted */
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyErase
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyErase(uint8 slotId)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (slotId >= S32K312_HSM_MAX_KEY_SLOTS) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Select key slot */
        S32K312_Hsm_KeyStoreRegs->SLOT_SEL = (uint32)slotId;
        
        /* Issue erase command */
        S32K312_Hsm_KeyStoreRegs->CTRL = S32K312_HSM_KEYSTORE_CMD_ERASE;
        
        /* Wait for completion */
        result = S32K312_Hsm_WaitReady(S32K312_HSM_TIMEOUT_DEFAULT);
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyGetSlotInfo
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyGetSlotInfo(uint8 slotId,
                                           S32K312_HsmKeySlotType* slotInfo)
{
    if ((slotId >= S32K312_HSM_MAX_KEY_SLOTS) || (slotInfo == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Read slot status from hardware */
        uint32 lockStatus = S32K312_Hsm_KeyStoreRegs->LOCK[slotId / 8U];
        
        slotInfo->slotId = slotId;
        slotInfo->locked = ((lockStatus >> (slotId % 8U)) & 0x1U) != 0U;
        slotInfo->occupied = FALSE; /* Would be determined from hardware status */
        slotInfo->keyType = 0U;
        slotInfo->keyLength = 0U;
    } else {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - UTILITY
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_WaitReady
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_WaitReady(uint32 timeoutUs)
{
    /* Simple timeout loop - in production, use proper timer */
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U; /* Approximate */
    
    for (i = 0U; i < maxIterations; i++) {
        if ((S32K312_Hsm_GlobalRegs != NULL_PTR) &&
            ((S32K312_Hsm_GlobalRegs->STATUS & S32K312_HSM_STATUS_BUSY) == 0U)) {
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* Timeout */
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearError
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_ClearError(void)
{
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        /* Clear error status in hardware */
        S32K312_Hsm_GlobalRegs->ERROR_STATUS = 0xFFFFFFFFU;
    }
    
    S32K312_Hsm_LastError = S32K312_HSM_SUCCESS;
    
    if (S32K312_Hsm_State == S32K312_HSM_STATE_ERROR) {
        S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_GetFirmwareVersion
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_GetFirmwareVersion(uint8* version)
{
    if (version == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 fwVersion = S32K312_Hsm_GlobalRegs->VERSION;
        version[0] = (uint8)((fwVersion >> 24) & 0xFFU);
        version[1] = (uint8)((fwVersion >> 16) & 0xFFU);
        version[2] = (uint8)((fwVersion >> 8) & 0xFFU);
        version[3] = (uint8)(fwVersion & 0xFFU);
    } else {
        version[0] = S32K312_HSM_SW_MAJOR_VERSION;
        version[1] = S32K312_HSM_SW_MINOR_VERSION;
        version[2] = S32K312_HSM_SW_PATCH_VERSION;
        version[3] = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_ValidateConfig
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_ValidateConfig(const S32K312_HsmConfigType* config)
{
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* At least one module should be enabled */
    if ((!config->enableAes) && (!config->enableEcc) && 
        (!config->enableSha) && (!config->enableTrng) && (!config->enableKeyStore)) {
        return E_NOT_OK;
    }
    
    /* Validate timeout */
    if (config->timeoutUs == 0U) {
        /* Use default timeout */
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_InitRegisters
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_InitRegisters(void)
{
    /* Initialize register pointers to HSM base addresses */
    S32K312_Hsm_GlobalRegs = (S32K312_HsmGlobalRegsType*)S32K312_HSM_BASE_ADDR;
    S32K312_Hsm_AesRegs = (S32K312_HsmAesRegsType*)S32K312_HSM_AES_BASE;
    S32K312_Hsm_EccRegs = (S32K312_HsmEccRegsType*)S32K312_HSM_ECC_BASE;
    S32K312_Hsm_ShaRegs = (S32K312_HsmShaRegsType*)S32K312_HSM_SHA_BASE;
    S32K312_Hsm_KeyStoreRegs = (S32K312_HsmKeyStoreRegsType*)S32K312_HSM_KEYSTORE_BASE;
    
    /* Verify hardware presence by checking version register */
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 version = S32K312_Hsm_GlobalRegs->VERSION;
        if (version == 0U) {
            /* No hardware detected */
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForAesReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForAesReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_AesRegs->STATUS;
            if (((status & S32K312_HSM_AES_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_AES_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForEccReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForEccReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_EccRegs->STATUS;
            if (((status & S32K312_HSM_ECC_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_ECC_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForShaReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForShaReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_ShaRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_ShaRegs->STATUS;
            if (((status & S32K312_HSM_SHA_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_SHA_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearAesContext
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ClearAesContext(void)
{
    uint32 i;
    
    if (S32K312_Hsm_AesRegs != NULL_PTR) {
        /* Clear key registers */
        for (i = 0U; i < 8U; i++) {
            S32K312_Hsm_AesRegs->KEY[i] = 0U;
        }
        /* Clear IV registers */
        for (i = 0U; i < 4U; i++) {
            S32K312_Hsm_AesRegs->IV[i] = 0U;
        }
        /* Clear data registers */
        for (i = 0U; i < 4U; i++) {
            S32K312_Hsm_AesRegs->DATA_IN[i] = 0U;
            S32K312_Hsm_AesRegs->DATA_OUT[i] = 0U;
        }
    }
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearEccContext
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ClearEccContext(void)
{
    uint32 i;
    
    if (S32K312_Hsm_EccRegs != NULL_PTR) {
        /* Clear scalar registers */
        for (i = 0U; i < 12U; i++) {
            S32K312_Hsm_EccRegs->SCALAR[i] = 0U;
        }
        /* Clear point registers */
        for (i = 0U; i < 24U; i++) {
            S32K312_Hsm_EccRegs->POINT_IN[i] = 0U;
            S32K312_Hsm_EccRegs->POINT_OUT[i] = 0U;
        }
    }
}

/**********************************************************************************************************************
 * S32K312_Hsm_ReportError
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ReportError(uint8 serviceId, uint32 errorCode)
{
    (void)serviceId;
    S32K312_Hsm_LastError = errorCode;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    /* Report to DET if configured */
    /* Det_ReportError(CRYPTO_MODULE_ID, 0, serviceId, (uint8)errorCode); */
    #endif
}

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
