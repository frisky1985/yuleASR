/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Module               : Crypto - Hash Algorithms
 * File Name            : hash_autosar.c
 * Author               : AutoSAR Team
 * Description          : AUTOSAR adapter layer for SHA hash algorithms
 *==================================================================================================*/

#include "hash_algos.h"
#include "CryIf.h"
#include "Det.h"

/*==================================================================================================
 *                                      LOCAL MACROS
==================================================================================================*/
#define HASH_AUTOSAR_INSTANCE_ID    (0x00U)

#if (HASH_DEV_ERROR_DETECT == STD_ON)
#define HASH_DET_REPORT_ERROR(ApiId, ErrorId)     Det_ReportError(HASH_MODULE_ID, HASH_AUTOSAR_INSTANCE_ID, (ApiId), (ErrorId))
#else
#define HASH_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
 *                                  GLOBAL VARIABLES
==================================================================================================*/
static Hash_AutosarContextType Hash_AutosarContexts[HASH_MAX_CONTEXTS];
static boolean Hash_AutosarInitialized = FALSE;

/*==================================================================================================
 *                                  LOCAL FUNCTIONS
==================================================================================================*/
static Hash_AutosarContextType* Hash_GetContext(uint32 contextId)
{
    if (contextId >= HASH_MAX_CONTEXTS)
    {
        return NULL_PTR;
    }
    return &Hash_AutosarContexts[contextId];
}

/*==================================================================================================
 *                                  GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Hash AUTOSAR adapter
 */
void Hash_AutosarInit(void)
{
    uint32 i;
    
    for (i = 0U; i < HASH_MAX_CONTEXTS; i++)
    {
        Hash_AutosarContexts[i].state = HASH_STATE_IDLE;
        Hash_AutosarContexts[i].algorithm = HASH_ALGO_NONE;
    }
    
    Hash_AutosarInitialized = TRUE;
}

/**
 * @brief Starts a hash operation
 */
Std_ReturnType Hash_AutosarStart(uint32 contextId, Hash_AlgorithmType algorithm)
{
    Hash_AutosarContextType* ctx;
    Std_ReturnType result = E_OK;
    
#if (HASH_DEV_ERROR_DETECT == STD_ON)
    if (!Hash_AutosarInitialized)
    {
        HASH_DET_REPORT_ERROR(0x01, 0x01); /* HASH_E_UNINIT */
        return E_NOT_OK;
    }
#endif
    
    ctx = Hash_GetContext(contextId);
    if (ctx == NULL_PTR)
    {
        HASH_DET_REPORT_ERROR(0x01, 0x02); /* HASH_E_PARAM_POINTER */
        return E_NOT_OK;
    }
    
    ctx->algorithm = algorithm;
    ctx->state = HASH_STATE_STARTED;
    
    switch (algorithm)
    {
        case HASH_ALGO_SHA1:
            Sha1_Init(&ctx->ctx.sha1);
            break;
        case HASH_ALGO_SHA224:
            Sha224_Init(&ctx->ctx.sha224);
            break;
        case HASH_ALGO_SHA256:
            Sha256_Init(&ctx->ctx.sha256);
            break;
        case HASH_ALGO_SHA384:
            Sha384_Init(&ctx->ctx.sha384);
            break;
        case HASH_ALGO_SHA512:
            Sha512_Init(&ctx->ctx.sha512);
            break;
        default:
            result = E_NOT_OK;
            break;
    }
    
    return result;
}

/**
 * @brief Updates hash with data
 */
Std_ReturnType Hash_AutosarUpdate(uint32 contextId, const uint8* data, uint32 length)
{
    Hash_AutosarContextType* ctx;
    
    ctx = Hash_GetContext(contextId);
    if (ctx == NULL_PTR || data == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    if (ctx->state != HASH_STATE_STARTED && ctx->state != HASH_STATE_UPDATED)
    {
        return E_NOT_OK;
    }
    
    switch (ctx->algorithm)
    {
        case HASH_ALGO_SHA1:
            Sha1_Update(&ctx->ctx.sha1, data, length);
            break;
        case HASH_ALGO_SHA224:
            Sha224_Update(&ctx->ctx.sha224, data, length);
            break;
        case HASH_ALGO_SHA256:
            Sha256_Update(&ctx->ctx.sha256, data, length);
            break;
        case HASH_ALGO_SHA384:
            Sha384_Update(&ctx->ctx.sha384, data, length);
            break;
        case HASH_ALGO_SHA512:
            Sha512_Update(&ctx->ctx.sha512, data, length);
            break;
        default:
            return E_NOT_OK;
    }
    
    ctx->state = HASH_STATE_UPDATED;
    return E_OK;
}

/**
 * @brief Finalizes hash operation
 */
Std_ReturnType Hash_AutosarFinish(uint32 contextId, uint8* digest, uint32* digestLength)
{
    Hash_AutosarContextType* ctx;
    
    ctx = Hash_GetContext(contextId);
    if (ctx == NULL_PTR || digest == NULL_PTR || digestLength == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    switch (ctx->algorithm)
    {
        case HASH_ALGO_SHA1:
            Sha1_Finish(&ctx->ctx.sha1, digest);
            *digestLength = SHA1_DIGEST_SIZE;
            break;
        case HASH_ALGO_SHA224:
            Sha224_Finish(&ctx->ctx.sha224, digest);
            *digestLength = SHA224_DIGEST_SIZE;
            break;
        case HASH_ALGO_SHA256:
            Sha256_Finish(&ctx->ctx.sha256, digest);
            *digestLength = SHA256_DIGEST_SIZE;
            break;
        case HASH_ALGO_SHA384:
            Sha384_Finish(&ctx->ctx.sha384, digest);
            *digestLength = SHA384_DIGEST_SIZE;
            break;
        case HASH_ALGO_SHA512:
            Sha512_Finish(&ctx->ctx.sha512, digest);
            *digestLength = SHA512_DIGEST_SIZE;
            break;
        default:
            return E_NOT_OK;
    }
    
    ctx->state = HASH_STATE_FINISHED;
    return E_OK;
}

/**
 * @brief One-shot hash computation
 */
Std_ReturnType Hash_AutosarCompute(Hash_AlgorithmType algorithm,
                                   const uint8* data, uint32 length,
                                   uint8* digest, uint32* digestLength)
{
    Std_ReturnType result;
    uint32 contextId = 0U; /* Use context 0 for one-shot operations */
    
    result = Hash_AutosarStart(contextId, algorithm);
    if (result != E_OK)
    {
        return result;
    }
    
    result = Hash_AutosarUpdate(contextId, data, length);
    if (result != E_OK)
    {
        return result;
    }
    
    return Hash_AutosarFinish(contextId, digest, digestLength);
}
