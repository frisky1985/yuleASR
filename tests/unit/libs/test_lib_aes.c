/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Lib_Aes (independent AES library) Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* B3-2 (2026-08-09): NIST FIPS-197 appendix C vectors (AES-128/192/256
* single block), SP 800-38A CBC-AES128 vectors (4 blocks), round-trips,
* in-place operation and error handling.
*
* Use: cmake -DBUILD_TESTING=ON .. && make LibAes_UnitTest && ctest -R LibAes
==================================================================================================*/

#include "../test_framework.h"
#include "Lib_Aes.h"
#include <string.h>

/*==================================================================================================
*                                      NIST VECTORS
*==================================================================================================*/

/* FIPS-197 C.1: AES-128 */
static const uint8_t Key128[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static const uint8_t PtBlock[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};
static const uint8_t Ct128[16] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
};

/* FIPS-197 C.2: AES-192 */
static const uint8_t Key192[24] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};
static const uint8_t Ct192[16] = {
    0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
    0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91
};

/* FIPS-197 C.3: AES-256 */
static const uint8_t Key256[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const uint8_t Ct256[16] = {
    0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
    0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89
};

/* SP 800-38A F.2.1: CBC-AES128 */
static const uint8_t CbcKey[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};
static const uint8_t CbcIv[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static const uint8_t CbcPt[4][16] = {
    { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
      0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a },
    { 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
      0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51 },
    { 0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
      0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef },
    { 0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
      0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 }
};
static const uint8_t CbcCt[4][16] = {
    { 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
      0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d },
    { 0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
      0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2 },
    { 0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
      0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16 },
    { 0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
      0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7 }
};

/*==================================================================================================
*                                      HELPERS
*==================================================================================================*/
static void assert_bytes_equal(const uint8_t* expected, const uint8_t* actual,
                               uint32_t len, uint32_t line)
{
    uint32_t i;
    for (i = 0u; i < len; i++)
    {
        char msg[96];
        (void)snprintf(msg, sizeof(msg),
                       "byte %lu: expected 0x%02X was 0x%02X",
                       (unsigned long)i, (unsigned)expected[i], (unsigned)actual[i]);
        UNITY_TEST_ASSERT(expected[i] == actual[i], msg, (int)line, __FILE__);
    }
}

void setUp(void)
{
    /* nothing to set up */
}

void tearDown(void)
{
    /* nothing to tear down */
}

/*==================================================================================================
*                                      FIPS-197 SINGLE BLOCK
*==================================================================================================*/
void test_aes128_fips197_encrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptBlock(&ctx, PtBlock, out));
    assert_bytes_equal(Ct128, out, 16u, __LINE__);
}

void test_aes128_fips197_decrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptBlock(&ctx, Ct128, out));
    assert_bytes_equal(PtBlock, out, 16u, __LINE__);
}

void test_aes192_fips197_encrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key192, LIB_AES_KEY_192));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptBlock(&ctx, PtBlock, out));
    assert_bytes_equal(Ct192, out, 16u, __LINE__);
}

void test_aes192_fips197_decrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key192, LIB_AES_KEY_192));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptBlock(&ctx, Ct192, out));
    assert_bytes_equal(PtBlock, out, 16u, __LINE__);
}

void test_aes256_fips197_encrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key256, LIB_AES_KEY_256));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptBlock(&ctx, PtBlock, out));
    assert_bytes_equal(Ct256, out, 16u, __LINE__);
}

void test_aes256_fips197_decrypt(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key256, LIB_AES_KEY_256));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptBlock(&ctx, Ct256, out));
    assert_bytes_equal(PtBlock, out, 16u, __LINE__);
}

/*==================================================================================================
*                                      ECB MULTI-BLOCK
*==================================================================================================*/
void test_aes128_ecb_two_blocks(void)
{
    Lib_AesContextType ctx;
    uint8_t pt[32];
    uint8_t ct[32];
    uint8_t back[32];

    (void)memcpy(pt, PtBlock, 16u);
    (void)memcpy(pt + 16, PtBlock, 16u);   /* same block twice */

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptEcb(&ctx, pt, ct, 32u));

    /* identical blocks -> identical ciphertext (ECB property) */
    assert_bytes_equal(Ct128, ct, 16u, __LINE__);
    assert_bytes_equal(Ct128, ct + 16, 16u, __LINE__);

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptEcb(&ctx, ct, back, 32u));
    assert_bytes_equal(pt, back, 32u, __LINE__);
}

void test_aes128_ecb_in_place(void)
{
    Lib_AesContextType ctx;
    uint8_t buf[32];

    (void)memcpy(buf, PtBlock, 16u);
    (void)memcpy(buf + 16, PtBlock, 16u);

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptEcb(&ctx, buf, buf, 32u));
    assert_bytes_equal(Ct128, buf, 16u, __LINE__);
    assert_bytes_equal(Ct128, buf + 16, 16u, __LINE__);
}

/*==================================================================================================
*                                      CBC (SP 800-38A)
*==================================================================================================*/
void test_aes128_cbc_four_blocks(void)
{
    Lib_AesContextType ctx;
    uint8_t pt[64];
    uint8_t ct[64];
    uint8_t back[64];

    (void)memcpy(pt, CbcPt[0], 16u);
    (void)memcpy(pt + 16, CbcPt[1], 16u);
    (void)memcpy(pt + 32, CbcPt[2], 16u);
    (void)memcpy(pt + 48, CbcPt[3], 16u);

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, CbcKey, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptCbc(&ctx, CbcIv, pt, ct, 64u));

    assert_bytes_equal(CbcCt[0], ct, 16u, __LINE__);
    assert_bytes_equal(CbcCt[1], ct + 16, 16u, __LINE__);
    assert_bytes_equal(CbcCt[2], ct + 32, 16u, __LINE__);
    assert_bytes_equal(CbcCt[3], ct + 48, 16u, __LINE__);

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptCbc(&ctx, CbcIv, ct, back, 64u));
    assert_bytes_equal(pt, back, 64u, __LINE__);
}

void test_aes128_cbc_one_block(void)
{
    Lib_AesContextType ctx;
    uint8_t ct[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, CbcKey, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK,
        Lib_AesEncryptCbc(&ctx, CbcIv, CbcPt[0], ct, 16u));
    assert_bytes_equal(CbcCt[0], ct, 16u, __LINE__);
}

void test_aes128_cbc_decrypt_matches_plaintext(void)
{
    Lib_AesContextType ctx;
    uint8_t back[16];

    /* Decrypting the FIRST ciphertext block with the IV yields plaintext 1 */
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, CbcKey, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK,
        Lib_AesDecryptCbc(&ctx, CbcIv, CbcCt[0], back, 16u));
    assert_bytes_equal(CbcPt[0], back, 16u, __LINE__);
}

void test_aes256_cbc_roundtrip_in_place(void)
{
    Lib_AesContextType ctx;
    uint8_t buf[32];

    (void)memcpy(buf, CbcPt[0], 16u);
    (void)memcpy(buf + 16, CbcPt[1], 16u);

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key256, LIB_AES_KEY_256));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesEncryptCbc(&ctx, CbcIv, buf, buf, 32u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesDecryptCbc(&ctx, CbcIv, buf, buf, 32u));
    assert_bytes_equal(CbcPt[0], buf, 16u, __LINE__);
    assert_bytes_equal(CbcPt[1], buf + 16, 16u, __LINE__);
}

/*==================================================================================================
*                                      ERROR HANDLING
*==================================================================================================*/
void test_aes_init_null(void)
{
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesInit(NULL, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesInit(NULL, NULL, LIB_AES_KEY_128));
}

void test_aes_block_null(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptBlock(NULL, PtBlock, out));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptBlock(&ctx, NULL, out));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptBlock(&ctx, PtBlock, NULL));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesDecryptBlock(&ctx, NULL, out));
}

void test_aes_uninitialized_context(void)
{
    Lib_AesContextType ctx;
    uint8_t out[16];

    (void)memset(&ctx, 0, sizeof(ctx));   /* rounds == 0 -> not initialized */
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_STATE, Lib_AesEncryptBlock(&ctx, PtBlock, out));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_STATE, Lib_AesDecryptBlock(&ctx, PtBlock, out));
}

void test_aes_ecb_bad_length(void)
{
    Lib_AesContextType ctx;
    uint8_t out[32];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_LENGTH, Lib_AesEncryptEcb(&ctx, PtBlock, out, 17u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_LENGTH, Lib_AesEncryptEcb(&ctx, PtBlock, out, 0u + 1u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_LENGTH, Lib_AesDecryptEcb(&ctx, PtBlock, out, 8u));
}

void test_aes_ecb_null_buffers(void)
{
    Lib_AesContextType ctx;
    uint8_t out[32];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, Key128, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptEcb(&ctx, NULL, out, 32u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptEcb(&ctx, PtBlock, NULL, 32u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesDecryptEcb(&ctx, PtBlock, NULL, 32u));
}

void test_aes_cbc_null_buffers(void)
{
    Lib_AesContextType ctx;
    uint8_t out[32];

    TEST_ASSERT_EQUAL_INT(LIB_AES_OK, Lib_AesInit(&ctx, CbcKey, LIB_AES_KEY_128));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptCbc(&ctx, NULL, CbcPt[0], out, 16u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesEncryptCbc(&ctx, CbcIv, NULL, out, 16u));
    TEST_ASSERT_EQUAL_INT(LIB_AES_ERR_PARAM, Lib_AesDecryptCbc(&ctx, CbcIv, CbcCt[0], NULL, 16u));
}

/*==================================================================================================
*                                      RUNNER
*==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_aes128_fips197_encrypt);
    RUN_TEST(test_aes128_fips197_decrypt);
    RUN_TEST(test_aes192_fips197_encrypt);
    RUN_TEST(test_aes192_fips197_decrypt);
    RUN_TEST(test_aes256_fips197_encrypt);
    RUN_TEST(test_aes256_fips197_decrypt);

    RUN_TEST(test_aes128_ecb_two_blocks);
    RUN_TEST(test_aes128_ecb_in_place);

    RUN_TEST(test_aes128_cbc_four_blocks);
    RUN_TEST(test_aes128_cbc_one_block);
    RUN_TEST(test_aes128_cbc_decrypt_matches_plaintext);
    RUN_TEST(test_aes256_cbc_roundtrip_in_place);

    RUN_TEST(test_aes_init_null);
    RUN_TEST(test_aes_block_null);
    RUN_TEST(test_aes_uninitialized_context);
    RUN_TEST(test_aes_ecb_bad_length);
    RUN_TEST(test_aes_ecb_null_buffers);
    RUN_TEST(test_aes_cbc_null_buffers);

    return UNITY_END();
}
