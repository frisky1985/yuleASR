/* @req SHALL_BOOT */

#include "Boot_Hsm.h"
#include <stdint.h>

/*
 * PORTING:
 *   - S32K312 production: replace stubs with S32K312_Hsm_Init() / Crypto_HwTrng_Generate()
 *     from Crypto_S32K312_Hsm.h / Crypto_HwTrng.h (the S32K312 crypto headers have type
 *     conflicts in the project — use forward declarations as shown below).
 *   - Other HSM/CSEc targets: map CSEc_Init(), CSEc_VerifySignature(), CSEc_GenerateRandom()
 *     to the target HAL.
 *
 * This implementation compiles on any host and initializes HSM on the first call.
 */

/* S32K312 HSM HAL forward declarations (replace with #include when headers are clean) */
#define S32K312_HSM_TIMEOUT_DEFAULT  (10000U)
struct S32K312_HsmConfig_s {
    boolean enableAes;
    boolean enableEcc;
    boolean enableSha;
    boolean enableTrng;
    boolean enableKeyStore;
    uint32  timeoutUs;
    void*   callback;
};
typedef struct S32K312_HsmConfig_s S32K312_HsmConfigType;

/* Forward declarations of target HSM HAL functions */
Std_ReturnType S32K312_Hsm_Init(const S32K312_HsmConfigType* config);
Std_ReturnType Crypto_HwTrng_Generate(uint8* output, uint32 length);

/* HSM default configuration */
static const S32K312_HsmConfigType Boot_Hsm_HwConfig = {
    1U,   /* enableAes */
    1U,   /* enableEcc */
    1U,   /* enableSha */
    1U,   /* enableTrng */
    1U,   /* enableKeyStore */
    S32K312_HSM_TIMEOUT_DEFAULT,
    (void*)0    /* callback */
};

static boolean g_hsm_available = FALSE;

Boot_Result Boot_Hsm_Init(void)
{
    Std_ReturnType result;

    if ((g_hsm_available) != 0U) {
        return BOOT_OK;
    }

    /* Initialize HSM hardware via S32K312 HSM HAL */
    result = S32K312_Hsm_Init(&Boot_Hsm_HwConfig);
    if (result == E_OK) {
        g_hsm_available = TRUE;
        return BOOT_OK;
    }

    /* HSM initialization failed — hardware may not be present */
    g_hsm_available = FALSE;
    return BOOT_E_HSM_INIT;
}

Boot_Result Boot_Hsm_VerifySignature(const uint8_t *hash,
                                     const uint8_t *signature,
                                     uint32_t       key_slot)
{
    if (!g_hsm_available) {
        return BOOT_E_HSM_INIT;
    }
    if ((hash == NULL_PTR) || (signature == NULL_PTR)) {
        return BOOT_E_PARAM;
    }

    /*
     * PORTING: Call target HSM ECDSA verify here, e.g.:
     *   S32K312_Hsm_EccVerify(ctx, hash, 32, signature, 64, &verifyResult);
     *   return (verifyResult == CRYPTO_E_VER_OK) ? BOOT_OK : BOOT_E_HSM_VERIFY;
     */
    (void)key_slot;
    return BOOT_E_HSM_VERIFY;
}

Boot_Result Boot_Hsm_Random(uint8_t *buf, uint32_t len)
{
    Std_ReturnType result;

    if (!g_hsm_available) {
        /* Attempt one-shot init for random generation */
        if (Boot_Hsm_Init() != BOOT_OK) {
            return BOOT_E_HSM_INIT;
        }
    }
    if ((buf == NULL_PTR) || (len == 0U)) {
        return BOOT_E_PARAM;
    }

    /* Generate cryptographically secure random bytes via HSM TRNG */
    result = Crypto_HwTrng_Generate(buf, len);
    if (result != E_OK) {
        return BOOT_E_HSM_INIT;
    }

    return BOOT_OK;
}

boolean Boot_Hsm_IsAvailable(void)
{
    return g_hsm_available;
}
