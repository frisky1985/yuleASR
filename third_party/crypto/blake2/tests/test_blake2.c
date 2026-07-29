/**********************************************************************************************************************
 * @file       test_blake2.c
 * @brief      BLAKE2 Hash Algorithm Unit Tests
 *
 * 功能: 验证BLAKE2b和BLAKE2s哈希算法的正确性
 * 包括RFC 7693测试向量
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "blake2.h"

/**********************************************************************************************************************
 * TEST RESULTS
 *********************************************************************************************************************/
static uint32 tests_passed = 0;
static uint32 tests_failed = 0;

/**********************************************************************************************************************
 * HELPER FUNCTIONS
 *********************************************************************************************************************/
static void print_hex(const char* label, const uint8* data, uint32 len)
{
    uint32 i;
    printf("%s: ", label);
    for (i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

static int compare_hex(const uint8* data, const char* expected, uint32 len)
{
    uint32 i;
    char hex[3];
    uint8 byte;

    for (i = 0; i < len; i++) {
        hex[0] = expected[i * 2];
        hex[1] = expected[i * 2 + 1];
        hex[2] = '\0';
        byte = (uint8)strtol(hex, NULL, 16);
        if (data[i] != byte) {
            return -1;
        }
    }
    return 0;
}

/**********************************************************************************************************************
 * TEST CASES - BLAKE2b
 *********************************************************************************************************************/

/* RFC 7693 Test Vector 1: BLAKE2b-512, empty input, no key */
static int test_blake2b_512_empty(void)
{
    uint8 out[BLAKE2B_OUTBYTES];
    const char* expected = 
        "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
        "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce";
    Blake2_ReturnType ret;

    ret = blake2b(out, NULL, 0, NULL, 0, BLAKE2B_OUTBYTES);

    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_512_empty - Return code error\n");
        tests_failed++;
        return -1;
    }

    if (compare_hex(out, expected, BLAKE2B_OUTBYTES) != 0) {
        printf("FAIL: test_blake2b_512_empty - Hash mismatch\n");
        print_hex("Expected", (uint8*)expected, BLAKE2B_OUTBYTES * 2);
        print_hex("Got", out, BLAKE2B_OUTBYTES);
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_512_empty\n");
    tests_passed++;
    return 0;
}

/* RFC 7693 Test Vector 2: BLAKE2b-512, input "abc", no key */
static int test_blake2b_512_abc(void)
{
    uint8 out[BLAKE2B_OUTBYTES];
    const uint8 in[] = "abc";
    const char* expected = 
        "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d1"
        "7d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923";
    Blake2_ReturnType ret;

    ret = blake2b(out, in, 3, NULL, 0, BLAKE2B_OUTBYTES);

    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_512_abc - Return code error\n");
        tests_failed++;
        return -1;
    }

    if (compare_hex(out, expected, BLAKE2B_OUTBYTES) != 0) {
        printf("FAIL: test_blake2b_512_abc - Hash mismatch\n");
        print_hex("Expected", (uint8*)expected, BLAKE2B_OUTBYTES * 2);
        print_hex("Got", out, BLAKE2B_OUTBYTES);
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_512_abc\n");
    tests_passed++;
    return 0;
}

/* BLAKE2b-256 with key */
static int test_blake2b_256_with_key(void)
{
    uint8 out[32];
    const uint8 in[] = "hello world";
    const uint8 key[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    Blake2_ReturnType ret;

    ret = blake2b(out, in, 11, key, 32, 32);

    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_256_with_key - Return code error\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_256_with_key\n");
    tests_passed++;
    return 0;
}

/* BLAKE2b incremental hashing */
static int test_blake2b_incremental(void)
{
    blake2b_state_t S;
    uint8 out[BLAKE2B_OUTBYTES];
    const uint8 part1[] = "Hello ";
    const uint8 part2[] = "World!";
    Blake2_ReturnType ret;

    ret = blake2b_init(&S, BLAKE2B_OUTBYTES);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_incremental - Init failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2b_update(&S, part1, 6);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_incremental - Update 1 failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2b_update(&S, part2, 6);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_incremental - Update 2 failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2b_final(&S, out, BLAKE2B_OUTBYTES);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2b_incremental - Final failed\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_incremental\n");
    tests_passed++;
    return 0;
}

/**********************************************************************************************************************
 * TEST CASES - BLAKE2s
 *********************************************************************************************************************/

/* RFC 7693 Test Vector 1: BLAKE2s-256, empty input, no key */
static int test_blake2s_256_empty(void)
{
    uint8 out[BLAKE2S_OUTBYTES];
    const char* expected = 
        "69217a987580e162298be34671dcb94587b68b15a04e3630802b7ee81785fda8";
    Blake2_ReturnType ret;

    ret = blake2s(out, NULL, 0, NULL, 0, BLAKE2S_OUTBYTES);

    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_256_empty - Return code error\n");
        tests_failed++;
        return -1;
    }

    if (compare_hex(out, expected, BLAKE2S_OUTBYTES) != 0) {
        printf("FAIL: test_blake2s_256_empty - Hash mismatch\n");
        print_hex("Expected", (uint8*)expected, BLAKE2S_OUTBYTES * 2);
        print_hex("Got", out, BLAKE2S_OUTBYTES);
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2s_256_empty\n");
    tests_passed++;
    return 0;
}

/* BLAKE2s-256 with input */
static int test_blake2s_256_abc(void)
{
    uint8 out[BLAKE2S_OUTBYTES];
    const uint8 in[] = "abc";
    Blake2_ReturnType ret;

    ret = blake2s(out, in, 3, NULL, 0, BLAKE2S_OUTBYTES);

    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_256_abc - Return code error\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2s_256_abc\n");
    tests_passed++;
    return 0;
}

/* BLAKE2s incremental hashing */
static int test_blake2s_incremental(void)
{
    blake2s_state_t S;
    uint8 out[BLAKE2S_OUTBYTES];
    const uint8 part1[] = "Hello ";
    const uint8 part2[] = "World!";
    Blake2_ReturnType ret;

    ret = blake2s_init(&S, BLAKE2S_OUTBYTES);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_incremental - Init failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2s_update(&S, part1, 6);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_incremental - Update 1 failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2s_update(&S, part2, 6);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_incremental - Update 2 failed\n");
        tests_failed++;
        return -1;
    }

    ret = blake2s_final(&S, out, BLAKE2S_OUTBYTES);
    if (ret != BLAKE2_ERR_NONE) {
        printf("FAIL: test_blake2s_incremental - Final failed\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2s_incremental\n");
    tests_passed++;
    return 0;
}

/**********************************************************************************************************************
 * ERROR HANDLING TESTS
 *********************************************************************************************************************/
static int test_blake2b_null_pointer(void)
{
    uint8 out[BLAKE2B_OUTBYTES];
    Blake2_ReturnType ret;

    /* Test NULL output pointer */
    ret = blake2b(NULL, NULL, 0, NULL, 0, BLAKE2B_OUTBYTES);
    if (ret != BLAKE2_ERR_NULL_POINTER) {
        printf("FAIL: test_blake2b_null_pointer - Should return NULL_POINTER error\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_null_pointer\n");
    tests_passed++;
    return 0;
}

static int test_blake2b_invalid_outlen(void)
{
    uint8 out[BLAKE2B_OUTBYTES];
    Blake2_ReturnType ret;

    /* Test invalid output length (0) */
    ret = blake2b(out, NULL, 0, NULL, 0, 0);
    if (ret != BLAKE2_ERR_INVALID_OUTLEN) {
        printf("FAIL: test_blake2b_invalid_outlen - Should return INVALID_OUTLEN error\n");
        tests_failed++;
        return -1;
    }

    /* Test invalid output length (too large) */
    ret = blake2b(out, NULL, 0, NULL, 0, BLAKE2B_OUTBYTES + 1);
    if (ret != BLAKE2_ERR_INVALID_OUTLEN) {
        printf("FAIL: test_blake2b_invalid_outlen - Should return INVALID_OUTLEN error for large len\n");
        tests_failed++;
        return -1;
    }

    printf("PASS: test_blake2b_invalid_outlen\n");
    tests_passed++;
    return 0;
}

/**********************************************************************************************************************
 * PERFORMANCE TESTS
 *********************************************************************************************************************/
#ifdef BLAKE2_ENABLE_BENCHMARK

static int test_blake2b_performance(void)
{
    uint8* data = NULL;
    uint8 out[BLAKE2B_OUTBYTES];
    uint32 sizes[] = {64, 1024, 4096, 65536};
    uint32 iterations = 1000;
    uint32 i, j;
    clock_t start, end;
    double time_ms;

    printf("\nBLAKE2b Performance Test:\n");
    printf("%-10s %-15s %-15s\n", "Size(B)", "Time(ms)", "Throughput(MB/s)");
    printf("-------------------------------------------\n");

    for (i = 0; i < 4; i++) {
        data = (uint8*)malloc(sizes[i]);
        if (data == NULL) {
            printf("FAIL: test_blake2b_performance - Memory allocation failed\n");
            tests_failed++;
            return -1;
        }

        /* Fill with test pattern */
        for (j = 0; j < sizes[i]; j++) {
            data[j] = (uint8)(j & 0xFF);
        }

        start = clock();
        for (j = 0; j < iterations; j++) {
            blake2b(out, data, sizes[i], NULL, 0, BLAKE2B_OUTBYTES);
        }
        end = clock();

        time_ms = ((double)(end - start) * 1000.0) / CLOCKS_PER_SEC;
        double throughput = ((double)sizes[i] * iterations) / (time_ms * 1000.0);

        printf("%-10u %-15.3f %-15.2f\n", sizes[i], time_ms, throughput);

        free(data);
    }

    printf("\n");
    tests_passed++;
    return 0;
}

#endif /* BLAKE2_ENABLE_BENCHMARK */

/**********************************************************************************************************************
 * MAIN TEST FUNCTION
 *********************************************************************************************************************/
int main(void)
{
    printf("=================================================================\n");
    printf("BLAKE2 Hash Algorithm Unit Tests\n");
    printf("=================================================================\n\n");

    /* BLAKE2b tests */
    printf("BLAKE2b Tests:\n");
    printf("--------------\n");
    test_blake2b_512_empty();
    test_blake2b_512_abc();
    test_blake2b_256_with_key();
    test_blake2b_incremental();

    /* BLAKE2s tests */
    printf("\nBLAKE2s Tests:\n");
    printf("--------------\n");
    test_blake2s_256_empty();
    test_blake2s_256_abc();
    test_blake2s_incremental();

    /* Error handling tests */
    printf("\nError Handling Tests:\n");
    printf("---------------------\n");
    test_blake2b_null_pointer();
    test_blake2b_invalid_outlen();

#ifdef BLAKE2_ENABLE_BENCHMARK
    /* Performance tests */
    printf("\nPerformance Tests:\n");
    printf("------------------\n");
    test_blake2b_performance();
#endif

    /* Summary */
    printf("\n=================================================================\n");
    printf("Test Summary:\n");
    printf("  Passed: %u\n", tests_passed);
    printf("  Failed: %u\n", tests_failed);
    printf("  Total:  %u\n", tests_passed + tests_failed);
    printf("=================================================================\n");

    return (tests_failed > 0) ? 1 : 0;
}
