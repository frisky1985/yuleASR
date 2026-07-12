#include "Boot_Hsm.h"

/*
 * PORTING: Replace with target HSM/CSEc driver calls.
 *
 * S32K312 HSM is accessed via the CSEc (Crypto Service Engine) IP:
 *   - CSEc_Poll() / CSEc_Init()
 *   - CSEc_VerifySignature(slot_id, hash, signature, result)
 *   - CSEc_GenerateRandom(buffer, length)
 *
 * This stub compiles on any target and returns BOOT_E_HSM_INIT
 * to indicate HSM is unavailable.
 */

static boolean g_hsm_available = FALSE;

Boot_Result Boot_Hsm_Init(void)
{
    /* TODO: CSEc_Init(); set g_hsm_available = TRUE on success */
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
    if (hash == NULL || signature == NULL) {
        return BOOT_E_PARAM;
    }
    /* TODO:
    CSEc_VerifySignature(key_slot, hash, 32, signature, 64, &result);
    return (result == CSEc_OK) ? BOOT_OK : BOOT_E_HSM_VERIFY;
    */
    (void)hash;
    (void)signature;
    (void)key_slot;
    return BOOT_E_HSM_VERIFY;
}

Boot_Result Boot_Hsm_Random(uint8_t *buf, uint32_t len)
{
    if (!g_hsm_available) {
        return BOOT_E_HSM_INIT;
    }
    /* TODO: CSEc_GenerateRandom(buf, len); */
    (void)buf;
    (void)len;
    return BOOT_E_HSM_INIT;
}

boolean Boot_Hsm_IsAvailable(void)
{
    return g_hsm_available;
}
