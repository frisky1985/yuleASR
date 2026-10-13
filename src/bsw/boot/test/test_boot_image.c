/**
 * @file test_boot_image.c
 * @brief Unit tests for Boot_Image — header validation, trailer read
 *
 * Covers uncovered branches:
 *   Line 102: NULL hdr check
 *   Line 108: payload_size == 0 or > 1MB
 *   Line 113: header CRC mismatch
 *   Line 134: NULL out_trailer
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "boot_platform.h"
#include "Std_Types.h"
#include "Boot_Image.h"
#include "Boot_Flash.h"

/* ---- RAM-backed Boot_Flash stub ---- */

#define TEST_FLASH_SIZE   (4U * 1024U * 1024U)

static uint8_t g_test_flash[TEST_FLASH_SIZE];

Boot_Result Boot_Flash_Init(void) { return BOOT_OK; }

Boot_Result Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0U; i < size; i++) {
        if ((addr + i) < TEST_FLASH_SIZE) {
            g_test_flash[addr + i] = 0xFFU;
        }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Write(uint32_t dst, const uint8_t *src, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++) {
        if ((dst + i) < TEST_FLASH_SIZE) {
            g_test_flash[dst + i] = src[i];
        }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Read(uint32_t src, uint8_t *dst, uint32_t len)
{
    memcpy(dst, &g_test_flash[src], len);
    return BOOT_OK;
}

Boot_Result Boot_Flash_SetProtection(uint32_t a, uint32_t s, boolean p)
{
    (void)a; (void)s; (void)p;
    return BOOT_OK;
}

/* ---- Test macros ---- */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * Test: ValidateHeader with NULL header (line 102)
 * ============================================================================ */
static int test_validate_header_null(void)
{
    printf("  Testing ValidateHeader with NULL header...\n");

    TEST_ASSERT_EQ(Boot_Image_ValidateHeader(NULL_PTR), BOOT_E_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ValidateHeader with payload_size == 0 (line 108, first branch)
 * ============================================================================ */
static int test_validate_header_payload_zero(void)
{
    printf("  Testing ValidateHeader with payload_size == 0...\n");

    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.payload_size = 0U;   /* Invalid: zero payload */
    hdr.header_crc = 0U;     /* CRC=0 skips CRC check */

    TEST_ASSERT_EQ(Boot_Image_ValidateHeader(&hdr), BOOT_E_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ValidateHeader with payload_size > 1MB (line 108, second branch)
 * ============================================================================ */
static int test_validate_header_payload_oversized(void)
{
    printf("  Testing ValidateHeader with payload_size > 1MB...\n");

    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.payload_size = (1024U * 1024U) + 1U;   /* 1MB + 1 byte */
    hdr.header_crc = 0U;     /* CRC=0 skips CRC check */

    TEST_ASSERT_EQ(Boot_Image_ValidateHeader(&hdr), BOOT_E_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ValidateHeader with CRC mismatch (line 113)
 * ============================================================================ */
static int test_validate_header_crc_mismatch(void)
{
    printf("  Testing ValidateHeader with CRC mismatch...\n");

    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.payload_size = 256U;   /* Valid size */
    hdr.header_crc = 0xDEADBEEFU;   /* Wrong CRC (non-zero, won't match) */

    TEST_ASSERT_EQ(Boot_Image_ValidateHeader(&hdr), BOOT_E_HEADER_CRC);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ValidateHeader with valid header (positive path)
 * ============================================================================ */
static int test_validate_header_valid(void)
{
    printf("  Testing ValidateHeader with valid header...\n");

    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BOOT_IMAGE_MAGIC;
    hdr.payload_size = 256U;
    hdr.header_crc = Boot_Image_CalcHeaderCrc(&hdr);   /* Correct CRC */

    TEST_ASSERT_EQ(Boot_Image_ValidateHeader(&hdr), BOOT_OK);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ReadTrailer with NULL out_trailer (line 134)
 * ============================================================================ */
static int test_read_trailer_null(void)
{
    printf("  Testing ReadTrailer with NULL out_trailer...\n");

    TEST_ASSERT_EQ(Boot_Image_ReadTrailer(0x1000U, 256U, NULL_PTR),
                   BOOT_E_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test: ReadTrailer with valid output pointer (positive path)
 * ============================================================================ */
static int test_read_trailer_valid(void)
{
    printf("  Testing ReadTrailer with valid output pointer...\n");

    /* Write a mock trailer into flash at a known location */
    uint32_t payload_addr = 0x10000U;
    uint32_t payload_size = 256U;
    uint32_t trailer_addr = payload_addr + payload_size;

    Boot_ImageTrailer trailer;
    memset(&trailer, 0, sizeof(trailer));
    trailer.signature_algo = 0x12345678U;
    trailer.signing_time = 0xAABBCCDDU;

    TEST_ASSERT_EQ(Boot_Flash_Write(trailer_addr,
                                    (const uint8_t *)&trailer,
                                    sizeof(trailer)),
                   BOOT_OK);

    /* Read it back */
    Boot_ImageTrailer out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQ(Boot_Image_ReadTrailer(payload_addr, payload_size, &out),
                   BOOT_OK);
    TEST_ASSERT_EQ(out.signature_algo, 0x12345678U);
    TEST_ASSERT_EQ(out.signing_time, 0xAABBCCDDU);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;
    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

int main(void)
{
    printf("============================================\n");
    printf("Boot Image Validation Tests\n");
    printf("============================================\n\n");

    memset(g_test_flash, 0xFF, sizeof(g_test_flash));

    run_test(test_validate_header_null, "ValidateHeader NULL");
    run_test(test_validate_header_payload_zero, "ValidateHeader payload_size=0");
    run_test(test_validate_header_payload_oversized, "ValidateHeader payload_size>1MB");
    run_test(test_validate_header_crc_mismatch, "ValidateHeader CRC mismatch");
    run_test(test_validate_header_valid, "ValidateHeader valid");
    run_test(test_read_trailer_null, "ReadTrailer NULL");
    run_test(test_read_trailer_valid, "ReadTrailer valid");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
