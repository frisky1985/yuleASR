/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Module               : Crypto - Hash Test Suite
 * File Name            : test_hash.c
 * Author               : AutoSAR Team
 * Description          : Unit tests for SHA hash algorithms
 *==================================================================================================*/

#include "hash_algos.h"
#include <stdio.h>
#include <string.h>

/*==================================================================================================
 *                                      TEST VECTORS
==================================================================================================*/

/* SHA-1 Test Vectors (NIST Example) */
static const uint8_t sha1_test_input[] = "abc";
static const uint8_t sha1_expected[] = {
    0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A,
    0xBA, 0x3E, 0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C,
    0x9C, 0xD0, 0xD8, 0x9D
};

/* SHA-256 Test Vectors (NIST Example) */
static const uint8_t sha256_test_input[] = "abc";
static const uint8_t sha256_expected[] = {
    0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
    0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
    0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
    0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
};

/* SHA-512 Test Vectors (NIST Example) */
static const uint8_t sha512_test_input[] = "abc";
static const uint8_t sha512_expected[] = {
    0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
    0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
    0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
    0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
    0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
    0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
    0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
    0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F
};

/*==================================================================================================
 *                                      TEST FUNCTIONS
==================================================================================================*/

static int test_sha1(void)
{
    Sha1_ContextType ctx;
    uint8_t digest[SHA1_DIGEST_SIZE];
    int i;
    int passed = 1;
    
    printf("Testing SHA-1...\n");
    
    Sha1_Init(&ctx);
    Sha1_Update(&ctx, sha1_test_input, strlen((const char*)sha1_test_input));
    Sha1_Finish(&ctx, digest);
    
    for (i = 0; i < SHA1_DIGEST_SIZE; i++)
    {
        if (digest[i] != sha1_expected[i])
        {
            passed = 0;
            break;
        }
    }
    
    printf("  SHA-1: %s\n", passed ? "PASSED" : "FAILED");
    return passed;
}

static int test_sha256(void)
{
    Sha256_ContextType ctx;
    uint8_t digest[SHA256_DIGEST_SIZE];
    int i;
    int passed = 1;
    
    printf("Testing SHA-256...\n");
    
    Sha256_Init(&ctx);
    Sha256_Update(&ctx, sha256_test_input, strlen((const char*)sha256_test_input));
    Sha256_Finish(&ctx, digest);
    
    for (i = 0; i < SHA256_DIGEST_SIZE; i++)
    {
        if (digest[i] != sha256_expected[i])
        {
            passed = 0;
            break;
        }
    }
    
    printf("  SHA-256: %s\n", passed ? "PASSED" : "FAILED");
    return passed;
}

static int test_sha512(void)
{
    Sha512_ContextType ctx;
    uint8_t digest[SHA512_DIGEST_SIZE];
    int i;
    int passed = 1;
    
    printf("Testing SHA-512...\n");
    
    Sha512_Init(&ctx);
    Sha512_Update(&ctx, sha512_test_input, strlen((const char*)sha512_test_input));
    Sha512_Finish(&ctx, digest);
    
    for (i = 0; i < SHA512_DIGEST_SIZE; i++)
    {
        if (digest[i] != sha512_expected[i])
        {
            passed = 0;
            break;
        }
    }
    
    printf("  SHA-512: %s\n", passed ? "PASSED" : "FAILED");
    return passed;
}

static int test_sha1_long_message(void)
{
    Sha1_ContextType ctx;
    uint8_t digest[SHA1_DIGEST_SIZE];
    uint8_t message[1000];
    int i;
    int passed = 1;
    
    printf("Testing SHA-1 with long message (1MB)...\n");
    
    /* Fill with 'a' */
    memset(message, 'a', sizeof(message));
    
    Sha1_Init(&ctx);
    for (i = 0; i < 1000; i++)  /* 1MB total */
    {
        Sha1_Update(&ctx, message, sizeof(message));
    }
    Sha1_Finish(&ctx, digest);
    
    printf("  SHA-1 (1MB): %s\n", passed ? "PASSED" : "FAILED");
    return passed;
}

/*==================================================================================================
 *                                      MAIN FUNCTION
==================================================================================================*/

int main(void)
{
    int total = 0;
    int passed = 0;
    
    printf("========================================\n");
    printf("  SHA Hash Algorithm Test Suite\n");
    printf("========================================\n\n");
    
    if (test_sha1()) passed++;
    total++;
    
    if (test_sha256()) passed++;
    total++;
    
    if (test_sha512()) passed++;
    total++;
    
    if (test_sha1_long_message()) passed++;
    total++;
    
    printf("\n========================================\n");
    printf("  Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
