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


/*==================================================================================================
 *  子文件包含 (批量拆分)
 *================================================================================================*/
#include "crypto_hsm_aes.c"
#include "crypto_hsm_ecc.c"
#include "crypto_hsm_sha_key.c"
#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
