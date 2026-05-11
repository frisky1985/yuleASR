/**********************************************************************************************************************
 * @file       blake2_autosar.c
 * @brief      BLAKE2 Hash Algorithm - AUTOSAR Crypto Stack Adapter
 *
 * 功能: 提供BLAKE2算法与AUTOSAR Crypto Stack的集成适配层
 * 支持的服务:
 * - 哈希计算 (Hash)
 * - MAC生成/验证 (MAC Generate/Verify)
 * - 增量哈希 (Incremental Hash)
 *
 * 安全特性:
 * - 按照AUTOSAR安全要求实现
 * - 支持开发错误检测
 * - 安全清除敏感数据
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "blake2.h"
#include "CryptoStack_Types.h"
#include "Det.h"

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define BLAKE2_AUTOSAR_VENDOR_ID                    0x2025U
#define BLAKE2_AUTOSAR_MODULE_ID                    0xF2U
#define BLAKE2_AUTOSAR_SW_MAJOR_VERSION             1U
#define BLAKE2_AUTOSAR_SW_MINOR_VERSION             0U
#define BLAKE2_AUTOSAR_SW_PATCH_VERSION             0U

/**********************************************************************************************************************
 * DEVELOPMENT ERROR CODES
 *********************************************************************************************************************/
#define BLAKE2_E_PARAM_POINTER                      0x01U
#define BLAKE2_E_PARAM_VALUE                        0x02U
#define BLAKE2_E_STATE_INVALID                      0x03U
#define BLAKE2_E_INIT_FAILED                        0x04U

/**********************************************************************************************************************
 * MODULE ENABLE FLAG
 *********************************************************************************************************************/
#ifndef BLAKE2_DEV_ERROR_DETECT
#define BLAKE2_DEV_ERROR_DETECT                     STD_ON
#endif

/**********************************************************************************************************************
 * TYPE DEFINITIONS
 *********************************************************************************************************************/
/* AUTOSAR BLAKE2 Configuration */
typedef struct {
    uint32                      algorithmId;        /* BLAKE2 algorithm ID */
    boolean                     useKey;             /* Use keyed hashing */
    uint8                       keyLength;          /* Key length in bytes */
    uint8                       digestLength;       /* Digest length in bytes */
} Blake2_AutosarConfigType;

/* AUTOSAR BLAKE2 Context */
typedef struct {
    blake2_state_t              state;              /* BLAKE2 state */
    Blake2_AlgorithmType        algorithm;          /* Algorithm type */
    boolean                     initialized;        /* Initialization flag */
    uint8                       digestLength;       /* Output length */
} Blake2_AutosarContextType;

/**********************************************************************************************************************
 * INTERNAL CONSTANTS
 *********************************************************************************************************************/
/* BLAKE2 Algorithm Identifiers for AUTOSAR */
#define CRYPTO_ALGOFAM_BLAKE2B                      0x60U
#define CRYPTO_ALGOFAM_BLAKE2S                      0x61U
#define CRYPTO_ALGOFAM_BLAKE2BP                     0x62U
#define CRYPTO_ALGOFAM_BLAKE2SP                     0x63U

/* BLAKE2 Mode Identifiers */
#define CRYPTO_ALGOMODE_BLAKE2_256                  0x00U
#define CRYPTO_ALGOMODE_BLAKE2_384                  0x01U
#define CRYPTO_ALGOMODE_BLAKE2_512                  0x02U

/**********************************************************************************************************************
 * INTERNAL VARIABLES
 *********************************************************************************************************************/
static Blake2_AutosarContextType Blake2_Context;
static boolean Blake2_ModuleInitialized = FALSE;

/**********************************************************************************************************************
 * INTERNAL HELPER FUNCTIONS
 *********************************************************************************************************************/
/* Report development error */
#if (BLAKE2_DEV_ERROR_DETECT == STD_ON)
static void Blake2_ReportError(uint8 ApiId, uint8 ErrorId)
{
    (void)Det_ReportError(BLAKE2_AUTOSAR_MODULE_ID, 0U, ApiId, ErrorId);
}
#else
#define Blake2_ReportError(ApiId, ErrorId)
#endif

/* Get algorithm type from AUTOSAR algorithm ID */
static Blake2_ReturnType Blake2_GetAlgorithmType(uint32 algoId, Blake2_AlgorithmType* algo)
{
    if (algo == NULL) {
        return BLAKE2_ERR_NULL_POINTER;
    }

    switch (algoId) {
        case CRYPTO_ALGOFAM_BLAKE2B:
            *algo = BLAKE2B_ID;
            break;
        case CRYPTO_ALGOFAM_BLAKE2S:
            *algo = BLAKE2S_ID;
            break;
        default:
            return BLAKE2_ERR_INVALID_PARAM;
    }

    return BLAKE2_ERR_NONE;
}

/* Secure memory clear */
static void Blake2_SecureClear(void* ptr, uint32 size)
{
    volatile uint8* p = (volatile uint8*)ptr;
    uint32 i;

    for (i = 0U; i < size; i++) {
        p[i] = 0U;
    }
}

/**********************************************************************************************************************
 * PUBLIC FUNCTIONS - AUTOSAR Interface
 *********************************************************************************************************************/
/* Initialize BLAKE2 module for AUTOSAR */
Std_ReturnType Blake2_Autosar_Init(const Blake2_AutosarConfigType* config)
{
    if (config == NULL) {
        Blake2_ReportError(0x01U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Clear context */
    Blake2_SecureClear(&Blake2_Context, sizeof(Blake2_AutosarContextType));

    /* Set algorithm type */
    if (Blake2_GetAlgorithmType(config->algorithmId, &Blake2_Context.algorithm) != BLAKE2_ERR_NONE) {
        Blake2_ReportError(0x01U, BLAKE2_E_PARAM_VALUE);
        return E_NOT_OK;
    }

    /* Validate digest length */
    if (Blake2_Context.algorithm == BLAKE2B_ID) {
        if ((config->digestLength == 0U) || (config->digestLength > BLAKE2B_OUTBYTES)) {
            Blake2_ReportError(0x01U, BLAKE2_E_PARAM_VALUE);
            return E_NOT_OK;
        }
    } else {
        if ((config->digestLength == 0U) || (config->digestLength > BLAKE2S_OUTBYTES)) {
            Blake2_ReportError(0x01U, BLAKE2_E_PARAM_VALUE);
            return E_NOT_OK;
        }
    }

    Blake2_Context.digestLength = config->digestLength;
    Blake2_ModuleInitialized = TRUE;

    return E_OK;
}

/* Deinitialize BLAKE2 module */
Std_ReturnType Blake2_Autosar_DeInit(void)
{
    if (!Blake2_ModuleInitialized) {
        return E_NOT_OK;
    }

    /* Securely clear context */
    Blake2_SecureClear(&Blake2_Context, sizeof(Blake2_AutosarContextType));
    Blake2_ModuleInitialized = FALSE;

    return E_OK;
}

/* Start BLAKE2 hash computation */
Std_ReturnType Blake2_Autosar_HashStart(
    uint32 jobId,
    const uint8* key,
    uint32 keyLength,
    uint32 digestLength
)
{
    Blake2_ReturnType ret;

#if (BLAKE2_DEV_ERROR_DETECT == STD_ON)
    if (!Blake2_ModuleInitialized) {
        Blake2_ReportError(0x10U, BLAKE2_E_STATE_INVALID);
        return E_NOT_OK;
    }
#endif

    (void)jobId; /* Not used in this implementation */

    /* Initialize state */
    if (Blake2_Context.algorithm == BLAKE2B_ID) {
        if (key != NULL && keyLength > 0U) {
            ret = blake2b_init_key(&Blake2_Context.state.b, (uint8)digestLength, key, (uint8)keyLength);
        } else {
            ret = blake2b_init(&Blake2_Context.state.b, (uint8)digestLength);
        }
    } else {
        if (key != NULL && keyLength > 0U) {
            ret = blake2s_init_key(&Blake2_Context.state.s, (uint8)digestLength, key, (uint8)keyLength);
        } else {
            ret = blake2s_init(&Blake2_Context.state.s, (uint8)digestLength);
        }
    }

    if (ret != BLAKE2_ERR_NONE) {
        Blake2_ReportError(0x10U, BLAKE2_E_INIT_FAILED);
        return E_NOT_OK;
    }

    Blake2_Context.initialized = TRUE;
    return E_OK;
}

/* Update BLAKE2 hash with data */
Std_ReturnType Blake2_Autosar_HashUpdate(
    uint32 jobId,
    const uint8* dataPtr,
    uint32 dataLength
)
{
    Blake2_ReturnType ret;

#if (BLAKE2_DEV_ERROR_DETECT == STD_ON)
    if (!Blake2_ModuleInitialized) {
        Blake2_ReportError(0x11U, BLAKE2_E_STATE_INVALID);
        return E_NOT_OK;
    }
    if (!Blake2_Context.initialized) {
        Blake2_ReportError(0x11U, BLAKE2_E_STATE_INVALID);
        return E_NOT_OK;
    }
    if ((dataPtr == NULL) && (dataLength > 0U)) {
        Blake2_ReportError(0x11U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    (void)jobId; /* Not used in this implementation */

    if (dataLength == 0U) {
        return E_OK;
    }

    if (Blake2_Context.algorithm == BLAKE2B_ID) {
        ret = blake2b_update(&Blake2_Context.state.b, dataPtr, dataLength);
    } else {
        ret = blake2s_update(&Blake2_Context.state.s, dataPtr, dataLength);
    }

    if (ret != BLAKE2_ERR_NONE) {
        return E_NOT_OK;
    }

    return E_OK;
}

/* Finalize BLAKE2 hash */
Std_ReturnType Blake2_Autosar_HashFinish(
    uint32 jobId,
    uint8* resultPtr,
    uint32* resultLengthPtr
)
{
    Blake2_ReturnType ret;

#if (BLAKE2_DEV_ERROR_DETECT == STD_ON)
    if (!Blake2_ModuleInitialized) {
        Blake2_ReportError(0x12U, BLAKE2_E_STATE_INVALID);
        return E_NOT_OK;
    }
    if (!Blake2_Context.initialized) {
        Blake2_ReportError(0x12U, BLAKE2_E_STATE_INVALID);
        return E_NOT_OK;
    }
    if (resultPtr == NULL) {
        Blake2_ReportError(0x12U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (resultLengthPtr == NULL) {
        Blake2_ReportError(0x12U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    (void)jobId; /* Not used in this implementation */

    if (Blake2_Context.algorithm == BLAKE2B_ID) {
        ret = blake2b_final(&Blake2_Context.state.b, resultPtr, Blake2_Context.digestLength);
    } else {
        ret = blake2s_final(&Blake2_Context.state.s, resultPtr, Blake2_Context.digestLength);
    }

    if (ret != BLAKE2_ERR_NONE) {
        return E_NOT_OK;
    }

    *resultLengthPtr = Blake2_Context.digestLength;
    Blake2_Context.initialized = FALSE;

    return E_OK;
}

/* Single-call BLAKE2 hash */
Std_ReturnType Blake2_Autosar_Hash(
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* keyPtr,
    uint32 keyLength,
    uint8* resultPtr,
    uint32* resultLengthPtr,
    uint32 algorithmId
)
{
    Blake2_ReturnType ret;
    Blake2_AlgorithmType algo;

#if (BLAKE2_DEV_ERROR_DETECT == STD_ON)
    if (resultPtr == NULL) {
        Blake2_ReportError(0x20U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (resultLengthPtr == NULL) {
        Blake2_ReportError(0x20U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((dataPtr == NULL) && (dataLength > 0U)) {
        Blake2_ReportError(0x20U, BLAKE2_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Get algorithm type */
    ret = Blake2_GetAlgorithmType(algorithmId, &algo);
    if (ret != BLAKE2_ERR_NONE) {
        Blake2_ReportError(0x20U, BLAKE2_E_PARAM_VALUE);
        return E_NOT_OK;
    }

    /* Perform hash */
    if (algo == BLAKE2B_ID) {
        ret = blake2b(resultPtr, dataPtr, dataLength, keyPtr, (uint8)keyLength, (uint8)*resultLengthPtr);
    } else {
        ret = blake2s(resultPtr, dataPtr, dataLength, keyPtr, (uint8)keyLength, (uint8)*resultLengthPtr);
    }

    if (ret != BLAKE2_ERR_NONE) {
        return E_NOT_OK;
    }

    return E_OK;
}

/* Cancel ongoing BLAKE2 operation */
Std_ReturnType Blake2_Autosar_Cancel(uint32 jobId)
{
    (void)jobId;

    if (!Blake2_ModuleInitialized) {
        return E_NOT_OK;
    }

    /* Clear context */
    Blake2_SecureClear(&Blake2_Context, sizeof(Blake2_AutosarContextType));
    Blake2_Context.initialized = FALSE;

    return E_OK;
}

/* Get version information */
void Blake2_Autosar_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL) {
        versioninfo->vendorID = BLAKE2_AUTOSAR_VENDOR_ID;
        versioninfo->moduleID = BLAKE2_AUTOSAR_MODULE_ID;
        versioninfo->sw_major_version = BLAKE2_AUTOSAR_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = BLAKE2_AUTOSAR_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = BLAKE2_AUTOSAR_SW_PATCH_VERSION;
    }
}

/**********************************************************************************************************************
 * CRYPTO JOB INTERFACE
 *********************************************************************************************************************/
/* Process Crypto Job - Integration with CSM/CryIf */
Crypto_OperationResultType Blake2_ProcessJob(
    Crypto_JobType* job,
    Crypto_OperationModeType mode
)
{
    Crypto_OperationResultType result = CRYPTO_OPRESULT_NOT_OK;
    const Crypto_AlgorithmInfoType* algoInfo;
    const Crypto_JobPrimitiveInputOutputType* inOut;

    if (job == NULL) {
        return CRYPTO_OPRESULT_NOT_OK;
    }

    if (job->jobPrimitiveInfo == NULL) {
        return CRYPTO_OPRESULT_NOT_OK;
    }

    algoInfo = &job->jobPrimitiveInfo->algorithm;
    inOut = job->jobPrimitiveInputOutput;

    if (inOut == NULL) {
        return CRYPTO_OPRESULT_NOT_OK;
    }

    /* Check if algorithm is BLAKE2 */
    if ((algoInfo->family != CRYPTO_ALGOFAM_BLAKE2B) &&
        (algoInfo->family != CRYPTO_ALGOFAM_BLAKE2S)) {
        return CRYPTO_OPRESULT_NOT_OK;
    }

    /* Process based on mode */
    switch (mode) {
        case CRYPTO_OPERATIONMODE_START:
            if (Blake2_Autosar_HashStart(
                    job->jobId,
                    inOut->secondaryInputPtr,
                    inOut->secondaryInputLength,
                    *(inOut->outputLengthPtr)) == E_OK) {
                result = CRYPTO_OPRESULT_OK;
            }
            break;

        case CRYPTO_OPERATIONMODE_UPDATE:
            if (Blake2_Autosar_HashUpdate(
                    job->jobId,
                    inOut->inputPtr,
                    inOut->inputLength) == E_OK) {
                result = CRYPTO_OPRESULT_OK;
            }
            break;

        case CRYPTO_OPERATIONMODE_FINISH:
            if (Blake2_Autosar_HashFinish(
                    job->jobId,
                    inOut->outputPtr,
                    inOut->outputLengthPtr) == E_OK) {
                result = CRYPTO_OPRESULT_OK;
            }
            break;

        case CRYPTO_OPERATIONMODE_SINGLECALL:
            if (Blake2_Autosar_Hash(
                    inOut->inputPtr,
                    inOut->inputLength,
                    inOut->secondaryInputPtr,
                    inOut->secondaryInputLength,
                    inOut->outputPtr,
                    inOut->outputLengthPtr,
                    algoInfo->family) == E_OK) {
                result = CRYPTO_OPRESULT_OK;
            }
            break;

        default:
            result = CRYPTO_OPRESULT_NOT_OK;
            break;
    }

    return result;
}

/**********************************************************************************************************************
 * PERFORMANCE TEST INTERFACE
 *********************************************************************************************************************/
#ifdef BLAKE2_ENABLE_BENCHMARK
#include "Os.h"  /* For timestamp */

typedef struct {
    uint32 iterations;
    uint32 totalTimeUs;
    uint32 minTimeUs;
    uint32 maxTimeUs;
    uint32 dataSize;
} Blake2_BenchmarkResultType;

/* Run BLAKE2 performance benchmark */
Std_ReturnType Blake2_RunBenchmark(
    uint32 algorithmId,
    uint32 dataSize,
    uint32 iterations,
    Blake2_BenchmarkResultType* result
)
{
    uint8* data = NULL;
    uint8 hash[BLAKE2B_OUTBYTES];
    uint32 hashLen = BLAKE2B_OUTBYTES;
    uint32 i;
    uint32 startTime;
    uint32 endTime;
    uint32 duration;
    Std_ReturnType ret = E_NOT_OK;

    if ((result == NULL) || (iterations == 0U) || (dataSize == 0U)) {
        return E_NOT_OK;
    }

    /* Allocate test data */
    data = (uint8*)malloc(dataSize);
    if (data == NULL) {
        return E_NOT_OK;
    }

    /* Fill with test pattern */
    for (i = 0U; i < dataSize; i++) {
        data[i] = (uint8)(i & 0xFFU);
    }

    /* Initialize result */
    result->iterations = iterations;
    result->dataSize = dataSize;
    result->totalTimeUs = 0U;
    result->minTimeUs = 0xFFFFFFFFU;
    result->maxTimeUs = 0U;

    /* Run benchmark */
    for (i = 0U; i < iterations; i++) {
        startTime = GetCounterValue();  /* Get timestamp */

        ret = Blake2_Autosar_Hash(data, dataSize, NULL, 0U, hash, &hashLen, algorithmId);

        endTime = GetCounterValue();

        if (ret != E_OK) {
            break;
        }

        duration = endTime - startTime;
        result->totalTimeUs += duration;
        if (duration < result->minTimeUs) {
            result->minTimeUs = duration;
        }
        if (duration > result->maxTimeUs) {
            result->maxTimeUs = duration;
        }
    }

    /* Cleanup */
    free(data);

    return ret;
}
#endif /* BLAKE2_ENABLE_BENCHMARK */
