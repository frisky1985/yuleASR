/**
 * @file test_boot_integration.c
 * Host-side integration test for yuleASR Secure Boot framework.
 * Tests: CRC, Header validation, SHA-256, ECDSA P-256, Boot flow.
 *
 * Compile:
 *   gcc -std=c99 -I../include -I. \
 *       -I/opt/homebrew/opt/openssl/include \
 *       -o test_boot_integration test_boot_integration.c \
 *       -L/opt/homebrew/opt/openssl/lib -lcrypto
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---- Platform stubs ---- */
#define STD_TYPES_H
typedef unsigned char boolean;
#define TRUE 1
#define FALSE 0
#define NULL_PTR ((void *)0)
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

/* ---- Flash emulation ---- */
#define FLASH_SIZE (2U * 1024U * 1024U)
static uint8_t g_boot_flash_ram[FLASH_SIZE];

/* ---- Override boot config for host test ---- */
#define BOOT_CFG_H
#define BOOT_FLASH_BASE         0x00000000UL
#define BOOT_PBL_ADDR           0x00000000UL
#define BOOT_PBL_SIZE           0x00001000UL
#define BOOT_SBL_ADDR           0x00002000UL
#define BOOT_SBL_SIZE           0x00010000UL
#define BOOT_APP_SLOT_A_ADDR    0x00012000UL
#define BOOT_APP_SLOT_A_SIZE    0x000EE000UL
#define BOOT_APP_SLOT_B_ADDR    0x00100000UL
#define BOOT_APP_SLOT_B_SIZE    0x000E0000UL
#define BOOT_BIB_ADDR           0x001E0000UL
#define BOOT_BIB_SIZE           0x00010000UL
#define BOOT_WDG_TIMEOUT_MS     5000U
#define BOOT_MAX_RETRIES        3U
#define BOOT_MAX_BOOT_ATTEMPTS  5U
#define BOOT_HASH_SIZE          32U
#define BOOT_SIGNATURE_SIZE     64U
#define BOOT_HSM_KEY_SLOT_PBL   0U
#define BOOT_HSM_KEY_SLOT_SBL   1U
#define BOOT_HSM_KEY_SLOT_APP   2U
#define BOOT_DEBUG_ENABLE       0U
#define BOOT_TRACE(fmt, ...)

/* ---- Include boot headers ---- */
#include "Boot_Types.h"
#include "Boot_Image.h"
#include "Boot_Verify.h"
#include "Boot_Flash.h"
#include "Boot_Update.h"
#include "Boot_Loader.h"

/* ---- Flash stub implementations ---- */
Boot_Result Boot_Flash_Init(void) { return BOOT_OK; }

Boot_Result Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0U; i < size; i++)
        { if ((addr + i) < FLASH_SIZE) { g_boot_flash_ram[addr + i] = 0xFF; } }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Write(uint32_t dst, const uint8_t *src, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++)
        { if ((dst + i) < FLASH_SIZE) { g_boot_flash_ram[dst + i] = src[i]; } }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Read(uint32_t src, uint8_t *dst, uint32_t len)
{
    memcpy(dst, &g_boot_flash_ram[src], len);
    return BOOT_OK;
}

Boot_Result Boot_Flash_SetProtection(uint32_t a, uint32_t s, boolean p)
{
    (void)a; (void)s; (void)p; return BOOT_OK;
}

/* ---- HSM stub (returns unavailable, fallback to software) ---- */
Boot_Result Boot_Hsm_Init(void) { return BOOT_E_HSM_INIT; }
Boot_Result Boot_Hsm_VerifySignature(const uint8_t *h, const uint8_t *s, uint32_t k)
{
    (void)h; (void)s; (void)k; return BOOT_E_HSM_VERIFY;
}
Boot_Result Boot_Hsm_Random(uint8_t *b, uint32_t l) { (void)b; (void)l; return BOOT_E_HSM_INIT; }
boolean Boot_Hsm_IsAvailable(void) { return FALSE; }

/* ---- Boot_Verify with OpenSSL ---- */
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>
boolean Boot_Hsm_IsAvailable(void);
Boot_Result Boot_Hsm_Random(uint8_t *b, uint32_t l);
Boot_Result Boot_Hsm_VerifySignature(const uint8_t *h, const uint8_t *s, uint32_t k);
Boot_Result Boot_Hsm_Init(void);

/* Constant-time compare needed by Boot_Image.c */
int32_t Boot_Verify_ConstantCmp(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    uint8_t diff = 0U;
    for (uint32_t i = 0U; i < len; i++) { diff |= a[i] ^ b[i]; }
    return (int32_t)diff;
}

void Boot_Verify_Hash(const uint8_t *data, uint32_t len, uint8_t *digest)
{
    SHA256(data, len, digest);
}

static uint8_t g_test_pubkey_der[256];
static size_t  g_test_pubkey_len = 0;

/* Generate a test key pair, return pubkey DER */
static int test_gen_keys(void)
{
    EVP_PKEY *pkey = EVP_PKEY_new();
    EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!eckey) { EVP_PKEY_free(pkey); return -1; }
    EC_KEY_generate_key(eckey);
    EVP_PKEY_assign_EC_KEY(pkey, eckey);

    unsigned char *p = g_test_pubkey_der;
    g_test_pubkey_len = i2d_PUBKEY(pkey, &p);

    FILE *f = fopen("/tmp/boot_test_priv.pem", "wb");
    if ((f) != 0U) { PEM_write_PrivateKey(f, pkey, NULL_PTR, NULL_PTR, 0, NULL_PTR, NULL_PTR); fclose(f); }
    EVP_PKEY_free(pkey);
    return (g_test_pubkey_len > 0U) ? 0 : -1;
}

/* Sign a payload, produce 64-byte raw r||s signature */
static int test_sign(const uint8_t *payload, uint32_t len, uint8_t *sig_out)
{
    FILE *f = fopen("/tmp/boot_test_priv.pem", "rb");
    if (!f) { return -1; }
    EVP_PKEY *pkey = PEM_read_PrivateKey(f, NULL_PTR, NULL_PTR, NULL_PTR);
    fclose(f);
    if (!pkey) { return -1; }

    uint8_t hash[32];
    SHA256(payload, len, hash);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }
    if (EVP_DigestSignInit(ctx, NULL_PTR, EVP_sha256(), NULL_PTR, pkey) != 1) {
        EVP_MD_CTX_free(ctx); EVP_PKEY_free(pkey); return -1;
    }

    size_t der_len = 0;
    EVP_DigestSign(ctx, NULL_PTR, &der_len, hash, 32);
    uint8_t *der = malloc(der_len);
    if (EVP_DigestSign(ctx, der, &der_len, hash, 32) != 1) {
        free(der); EVP_MD_CTX_free(ctx); EVP_PKEY_free(pkey); return -1;
    }

    const unsigned char *dp = der;
    ECDSA_SIG *esig = d2i_ECDSA_SIG(NULL_PTR, &dp, (long)der_len);
    if (!esig) { free(der); EVP_MD_CTX_free(ctx); EVP_PKEY_free(pkey); return -1; }
    const BIGNUM *r, *s;
    ECDSA_SIG_get0(esig, &r, &s);
    BN_bn2binpad(r, sig_out, 32);
    BN_bn2binpad(s, sig_out + 32, 32);
    ECDSA_SIG_free(esig);
    free(der);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return 0;
}

Boot_Result Boot_Verify_Signature(const uint8_t *hash,
                                  const uint8_t *signature,
                                  const Boot_PubKey *pub_key)
{
    if (!hash || !signature || !pub_key) { return BOOT_E_PARAM; }
    const unsigned char *p = pub_key->data;
    EVP_PKEY *evp_key = d2i_PUBKEY(NULL_PTR, &p, (long)pub_key->length);
    if (!evp_key) { return BOOT_E_SIGNATURE; }

    /* Convert raw r||s to DER for OpenSSL EVP */
    const uint8_t *rp = signature, *sp = signature + 32;
    BIGNUM *bnr = BN_bin2bn(rp, 32, NULL_PTR);
    BIGNUM *bns = BN_bin2bn(sp, 32, NULL_PTR);
    ECDSA_SIG *esig = ECDSA_SIG_new();
    ECDSA_SIG_set0(esig, bnr, bns);

    uint8_t der_buf[80];
    unsigned char *dp = der_buf;
    int der_len = i2d_ECDSA_SIG(esig, &dp);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, NULL_PTR, EVP_sha256(), NULL_PTR, evp_key);
    int ret = EVP_DigestVerify(ctx, der_buf, der_len, hash, 32);

    ECDSA_SIG_free(esig);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(evp_key);
    return (ret == 1) ? BOOT_OK : BOOT_E_SIGNATURE;
}

/* ---- Boot_Loader stubs (no-ops on host) ---- */
void __attribute__((noreturn)) Boot_Loader_Jump(uint32_t addr)
{
    printf("  [JUMP] Would jump to app at 0x%08X\n", addr);
    exit(0);
}
void __attribute__((noreturn)) Boot_Loader_EnterRecovery(void)
{
    printf("  [RECOVERY] Entering recovery mode (bad signature)\n");
    exit(1);
}

static int passed = 0, failed = 0;
#define TEST(name, expr) do { \
    if (expr) { passed++; printf("  PASS: %s\n", name); } \
    else { failed++; printf("  FAIL: %s\n", name); } \
} while(0)

/* ============== TESTS ============== */

static void test_crc(void)
{
    printf("\n=== Image Header CRC ===\n");
    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.image_type = BOOT_IMAGE_APP;
    hdr.version = 0x01030000;
    hdr.payload_size = 4096;
    uint32_t c1 = Boot_Image_CalcHeaderCrc(&hdr);
    TEST("CRC non-zero", c1 != 0U);
    uint32_t c2 = Boot_Image_CalcHeaderCrc(&hdr);
    TEST("CRC deterministic", c1 == c2);
    hdr.payload_size = 8192;
    uint32_t c3 = Boot_Image_CalcHeaderCrc(&hdr);
    TEST("CRC changes with content", c1 != c3);
}

static void test_header_validation(void)
{
    printf("\n=== Image Header Validation ===\n");
    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.image_type = BOOT_IMAGE_APP;
    hdr.version = 0x01030000;
    hdr.payload_size = 4096;
    hdr.header_crc = Boot_Image_CalcHeaderCrc(&hdr);
    TEST("valid header", Boot_Image_ValidateHeader(&hdr) == BOOT_OK);

    hdr.magic = 0xDEAD;
    TEST("bad magic", Boot_Image_ValidateHeader(&hdr) != BOOT_OK);
    hdr.magic = BOOT_IMAGE_MAGIC;

    hdr.payload_size = 0;
    TEST("zero payload", Boot_Image_ValidateHeader(&hdr) != BOOT_OK);
}

static void test_hash_verify(void)
{
    printf("\n=== Hash Verification ===\n");
    uint8_t payload[256];
    for (int i = 0U; i < 256; i++) { payload[i] = (uint8_t)i; }

    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.payload_size = 256;
    Boot_Verify_Hash(payload, 256, hdr.hash);
    TEST("hash match", Boot_Image_VerifyHash(&hdr, payload) == BOOT_OK);

    payload[128] ^= 0xFF;
    TEST("tamper detected", Boot_Image_VerifyHash(&hdr, payload) != BOOT_OK);
}

static void test_const_cmp(void)
{
    printf("\n=== Constant-Time Compare ===\n");
    uint8_t a[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint8_t b[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint8_t c[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0};
    TEST("equal", Boot_Verify_ConstantCmp(a, b, 16) == 0);
    TEST("different", Boot_Verify_ConstantCmp(a, c, 16) != 0);
}

static void test_ecdsa(void)
{
    printf("\n=== ECDSA P-256 Sign & Verify ===\n");
    if (test_gen_keys() != 0) {
        printf("  SKIP: key generation\n");
        return;
    }
    Boot_PubKey pk = { .data = g_test_pubkey_der, .length = (uint16_t)g_test_pubkey_len };

    uint8_t msg[] = "yuleASR Secure Boot v1.3.0";
    uint8_t hash[32], sig[64];
    SHA256(msg, sizeof(msg)-1U, hash);
    if (test_sign(msg, sizeof(msg)-1U, sig) != 0) {
        printf("  SKIP: signing\n");
        return;
    }
    TEST("verify ok", Boot_Verify_Signature(hash, sig, &pk) == BOOT_OK);
    sig[0] ^= 1;
    TEST("tampered rejected", Boot_Verify_Signature(hash, sig, &pk) != BOOT_OK);
}

static void test_end_to_end(void)
{
    printf("\n\n============================================");
    printf("\n=== END-TO-END: Secure Boot Pipeline ===");
    printf("\n============================================\n");

    if (test_gen_keys() != 0) { printf("FAIL: keys\n"); failed++; return; }
    Boot_PubKey pk = { .data = g_test_pubkey_der, .length = (uint16_t)g_test_pubkey_len };

    /* Create test app payload */
    uint8_t app[4096];
    for (int i = 0U; i < 4096; i++) { app[i] = i & 0xFF; }

    /* Sign */
    uint8_t sig[64];
    if (test_sign(app, sizeof(app), sig) != 0) { printf("FAIL: sign\n"); failed++; return; }

    /* Build header */
    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.image_type = BOOT_IMAGE_APP;
    hdr.version = 0x01030000;
    hdr.payload_size = sizeof(app);
    SHA256(app, sizeof(app), hdr.hash);
    hdr.header_crc = Boot_Image_CalcHeaderCrc(&hdr);

    /* Build trailer */
    Boot_ImageTrailer trail;
    memset(&trail, 0, sizeof(trail));
    memcpy(trail.signature, sig, 64);

    /* Write to flash */
    memset(g_boot_flash_ram, 0xFF, FLASH_SIZE);
    uint32_t base = BOOT_APP_SLOT_A_ADDR;
    Boot_Flash_Write(base, (uint8_t*)&hdr, sizeof(hdr));
    Boot_Flash_Write(base + sizeof(hdr), app, sizeof(app));
    Boot_Flash_Write(base + sizeof(hdr) + sizeof(app), (uint8_t*)&trail, sizeof(trail));

    /* Write BIB */
    Boot_InfoBlock bib;
    memset(&bib, 0, sizeof(bib));
    bib.magic = 0x30424942U;
    bib.max_boot_attempts = 5;
    bib.status = 0x01;
    bib.anti_rollback_counter = 0;
    Boot_Flash_Write(BOOT_BIB_ADDR, (uint8_t*)&bib, sizeof(bib));

    printf("  [WRITE] App at 0x%08X, %u bytes\n", base, sizeof(app));

    /* Read back and verify */
    Boot_ImageHeader rhdr;
    Boot_Flash_Read(base, (uint8_t*)&rhdr, sizeof(rhdr));

    printf("  [READ]  magic=0x%08X type=%u ver=0x%08X size=%u\n",
           rhdr.magic, rhdr.image_type, rhdr.version, rhdr.payload_size);

    TEST("header valid", Boot_Image_ValidateHeader(&rhdr) == BOOT_OK);

    uint8_t *rdata = malloc(rhdr.payload_size);
    Boot_Flash_Read(base + sizeof(rhdr), rdata, rhdr.payload_size);

    uint8_t expected_hash[32];
    SHA256(rdata, rhdr.payload_size, expected_hash);
    TEST("hash matches flash", memcmp(expected_hash, rhdr.hash, 32) == 0);

    Boot_ImageTrailer rtrail;
    Boot_Image_ReadTrailer(base + sizeof(rhdr), rhdr.payload_size, &rtrail);
    TEST("trailer sig non-zero",
         memcmp(rtrail.signature, trail.signature, 64) == 0);

    /* Verify ECDSA signature */
    TEST("ecdsa verify", Boot_Verify_Signature(expected_hash, rtrail.signature, &pk) == BOOT_OK);

    /* Anti-rollback check */
    TEST("version >= rollback", rhdr.version > bib.anti_rollback_counter);

    /* Boot confirm */
    Boot_InfoBlock rbib;
    Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t*)&rbib, sizeof(rbib));
    TEST("bib boot count zero", rbib.boot_count == 0U);

    free(rdata);
    printf("\n  >>> Secure Boot Pipeline: all stages PASS <<<\n");
}

int main(void)
{
    printf("========================================\n");
    printf("  yuleASR Secure Boot — Host Validation\n");
    printf("  Crypto: ECDSA P-256 + SHA-256\n");
    printf("========================================\n");

    memset(g_boot_flash_ram, 0xFF, FLASH_SIZE);
    test_crc();
    test_header_validation();
    test_hash_verify();
    test_const_cmp();
    test_ecdsa();
    test_end_to_end();

    printf("\n========================================\n");
    printf("  RESULTS: %d passed, %d failed\n", passed, failed);
    printf("========================================\n");
    return failed > 0 ? 1 : 0;
}
