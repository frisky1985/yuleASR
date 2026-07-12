/**
 * @file test_boot_verify.c
 * @brief Unit tests for Boot_Verify — hash, signature, constant-time compare
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Boot_Verify.h"

static void test_constant_cmp_equal(void)
{
    uint8_t a[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t b[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    assert(Boot_Verify_ConstantCmp(a, b, 8) == 0);
    printf("PASS: constant_cmp_equal\\n");
}

static void test_constant_cmp_not_equal(void)
{
    uint8_t a[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t b[8] = {0x01, 0x02, 0x03, 0x04, 0xFF, 0x06, 0x07, 0x08};
    assert(Boot_Verify_ConstantCmp(a, b, 8) != 0);
    printf("PASS: constant_cmp_not_equal\\n");
}

static void test_hash_empty(void)
{
    uint8_t digest[32];
    Boot_Verify_Hash((const uint8_t *)"", 0, digest);
    /* SHA-256 of empty string */
    uint8_t expected[32] = {
        0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14,
        0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
        0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C,
        0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55
    };
    assert(memcmp(digest, expected, 32) == 0);
    printf("PASS: hash_empty\\n");
}

static void test_hash_known(void)
{
    const char *data = "yuleASR Secure Boot";
    uint8_t digest[32];
    Boot_Verify_Hash((const uint8_t *)data, strlen(data), digest);
    /* Pre-computed SHA-256 */
    uint8_t expected[32] = {
        0xE1, 0xCA, 0xCF, 0x15, 0x3D, 0x08, 0x4F, 0x8B,
        0xFB, 0x82, 0x7B, 0x73, 0x5A, 0x93, 0x86, 0xD8,
        0x52, 0x88, 0x62, 0x52, 0x92, 0xB6, 0x2F, 0x21,
        0x3C, 0x34, 0xEF, 0xFE, 0x2E, 0x22, 0x3F, 0xB8
    };
    assert(memcmp(digest, expected, 32) == 0);
    printf("PASS: hash_known\\n");
}

int main(void)
{
    printf("=== Boot_Verify Unit Tests ===\\n\\n");
    test_constant_cmp_equal();
    test_constant_cmp_not_equal();
    test_hash_empty();
    test_hash_known();
    printf("\\n=== ALL TESTS PASSED ===\\n");
    return 0;
}
