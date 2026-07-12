#include "Boot_Verify.h"
#include <string.h>

/*
 * Software ECDSA P-256 verification.
 *
 * Strategy: Use mbedTLS when available; otherwise provide a minimal
 * constant-time verify stub so the framework compiles on any target.
 *
 * Production path: define MBEDTLS_USE and #include "mbedtls_wrapper.h"
 *   which wraps mbedtls_pk_verify_ext() for ECDSA P-256 + SHA-256.
 *
 * Stub path: returns BOOT_E_SIGNATURE to force HSM fallback or recovery.
 */

#if defined(MBEDTLS_USE)
#include "mbedtls_wrapper.h"
#endif

/* ---- Public keys (DER-encoded placeholders — replace with actual keys) ---- */

static const uint8_t g_sbl_pubkey_der[] = {
    /* ECDSA P-256 public key in SEC1 uncompressed format:
       04 || X(32) || Y(32) = 65 bytes + ASN.1 wrapper (~91B total) */
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
    0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
    0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04,
    /* 64 bytes of uncompressed point go here */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const Boot_PubKey g_boot_pubkey_sbl = {
    .data   = g_sbl_pubkey_der,
    .length = sizeof(g_sbl_pubkey_der)
};

static const uint8_t g_app_pubkey_der[] = {
    /* Replace with actual App public key in same format */
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
    0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
    0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const Boot_PubKey g_boot_pubkey_app = {
    .data   = g_app_pubkey_der,
    .length = sizeof(g_app_pubkey_der)
};

/* ---- Signature Verification ---- */

Boot_Result Boot_Verify_Signature(const uint8_t    *hash,
                                  const uint8_t    *signature,
                                  const Boot_PubKey *pub_key)
{
    if (hash == NULL || signature == NULL || pub_key == NULL) {
        return BOOT_E_PARAM;
    }

#if defined(MBEDTLS_USE)
    int ret = mbedtls_ecdsa_verify_wrapper(pub_key->data, pub_key->length,
                                           hash, 32, signature, 64);
    return (ret == 0) ? BOOT_OK : BOOT_E_SIGNATURE;
#else
    (void)hash;
    (void)signature;
    (void)pub_key;
    /* HSM fallback expected — software verify not compiled in */
    return BOOT_E_SIGNATURE;
#endif
}

/* ---- SHA-256 Hash ---- */

#if defined(MBEDTLS_USE)
#include "mbedtls/sha256.h"

void Boot_Verify_Hash(const uint8_t *data, uint32_t len, uint8_t *digest)
{
    mbedtls_sha256_ret(data, len, digest, 0);  /* 0 = SHA-256 */
}
#else
#include "third_party/crypto/hash/include/hash_algos.h"

void Boot_Verify_Hash(const uint8_t *data, uint32_t len, uint8_t *digest)
{
    sha256_compute(data, len, digest);
}
#endif

/* ---- Constant-time Compare ---- */

int32_t Boot_Verify_ConstantCmp(const uint8_t *a,
                                const uint8_t *b,
                                uint32_t       len)
{
    uint8_t diff = 0U;
    for (uint32_t i = 0U; i < len; i++) {
        diff |= a[i] ^ b[i];
    }
    /* Return 0 on match, non-zero on mismatch */
    return (int32_t)diff;
}
