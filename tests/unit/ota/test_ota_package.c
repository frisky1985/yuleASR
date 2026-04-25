/**
 * @file test_ota_package.c
 * @brief Unit tests for OTA Package Parser
 * @version 1.0
 * @date 2026-04-26
 */

#include "../../unity/unity.h"
#include "../../../src/ota/ota_package.h"
#include <string.h>

/* Test fixtures */
static ota_parser_context_t g_parser_ctx;
static ota_parser_config_t g_parser_config;

static void* test_alloc(uint32_t size)
{
    return malloc(size);
}

static void test_free(void *ptr)
{
    free(ptr);
}

/* Setup and teardown */
void setUp(void)
{
    memset(&g_parser_ctx, 0, sizeof(g_parser_ctx));
    memset(&g_parser_config, 0, sizeof(g_parser_config));
    
    g_parser_config.max_manifest_size = 128 * 1024;
    g_parser_config.max_payload_size = 512 * 1024 * 1024;
    g_parser_config.verify_signature = true;
    g_parser_config.verify_hash = true;
    g_parser_config.decompress = true;
    g_parser_config.alloc = test_alloc;
    g_parser_config.free = test_free;
}

void tearDown(void)
{
    if (g_parser_ctx.initialized) {
        ota_package_parser_deinit(&g_parser_ctx);
    }
}

/* Helper to create a mock vpkg header */
static void create_mock_vpkg_header(uint8_t *buffer, uint32_t *size)
{
    ota_vpkg_header_t *header = (ota_vpkg_header_t*)buffer;
    
    header->magic = OTA_PKG_MAGIC_VPCK;
    header->version = OTA_PKG_CURRENT_VERSION;
    memset(header->package_id, 0xAA, 16);
    header->timestamp = 0x1234567890ABCDEF;
    strncpy(header->vin, "WBA12345678901234", sizeof(header->vin));
    strncpy(header->hw_version, "HW_V2.0", sizeof(header->hw_version));
    
    header->header_size = sizeof(ota_vpkg_header_t);
    header->manifest_offset = sizeof(ota_vpkg_header_t);
    header->manifest_size = 1000;
    header->payload_offset = sizeof(ota_vpkg_header_t) + 1000;
    header->payload_size = 100000;
    header->signature_offset = sizeof(ota_vpkg_header_t) + 1000 + 100000;
    header->signature_size = 256;
    
    header->compression = OTA_COMPRESS_NONE;
    header->encryption = OTA_ENCRYPT_NONE;
    
    *size = sizeof(ota_vpkg_header_t);
}

/* Helper to create a mock epkg header */
static void create_mock_epkg_header(uint8_t *buffer, uint32_t *size)
{
    ota_epkg_header_t *header = (ota_epkg_header_t*)buffer;
    
    header->magic = OTA_PKG_MAGIC_EPCK;
    header->version = OTA_PKG_CURRENT_VERSION;
    header->ecu_id = 0x0101;
    strncpy(header->hw_version, "HW_V2.0", sizeof(header->hw_version));
    strncpy(header->fw_version_from, "1.0.0", sizeof(header->fw_version_from));
    strncpy(header->fw_version_to, "2.0.0", sizeof(header->fw_version_to));
    
    header->header_size = sizeof(ota_epkg_header_t);
    header->firmware_offset = sizeof(ota_epkg_header_t);
    header->firmware_size = 50000;
    header->firmware_compressed_size = 50000;
    header->delta_offset = 0;
    header->delta_size = 0;
    header->layout_offset = 0;
    header->layout_size = 0;
    header->hash_offset = sizeof(ota_epkg_header_t) + 50000;
    header->hash_size = 32;
    
    header->compression = OTA_COMPRESS_NONE;
    header->encryption = OTA_ENCRYPT_NONE;
    header->download_address = 0x08020000;
    
    memset(header->firmware_hash, 0xBB, 32);
    
    *size = sizeof(ota_epkg_header_t);
}

/* Test cases */
void test_ota_package_parser_init_success(void)
{
    ota_error_t err = ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(g_parser_ctx.initialized);
    TEST_ASSERT_EQUAL(test_alloc, g_parser_ctx.config.alloc);
    TEST_ASSERT_EQUAL(test_free, g_parser_ctx.config.free);
}

void test_ota_package_parser_init_null_params(void)
{
    ota_error_t err;
    
    /* NULL context */
    err = ota_package_parser_init(NULL, &g_parser_config);
    TEST_ASSERT_EQUAL(OTA_ERR_INVALID_PARAM, err);
    
    /* NULL config */
    err = ota_package_parser_init(&g_parser_ctx, NULL);
    TEST_ASSERT_EQUAL(OTA_ERR_INVALID_PARAM, err);
}

void test_ota_package_parser_deinit(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    ota_package_parser_deinit(&g_parser_ctx);
    
    TEST_ASSERT_FALSE(g_parser_ctx.initialized);
}

void test_ota_package_parse_vpkg_header_success(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t buffer[256];
    uint32_t size;
    create_mock_vpkg_header(buffer, &size);
    
    ota_package_info_t info;
    ota_error_t err = ota_package_parse_header(&g_parser_ctx, buffer, size, &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(info.is_vpkg);
    TEST_ASSERT_EQUAL(OTA_PKG_MAGIC_VPCK, info.header.vpkg.magic);
    TEST_ASSERT_EQUAL(OTA_PKG_CURRENT_VERSION, info.header.vpkg.version);
    TEST_ASSERT_EQUAL(1000, info.header.vpkg.manifest_size);
    TEST_ASSERT_EQUAL(100000, info.header.vpkg.payload_size);
    TEST_ASSERT_TRUE(g_parser_ctx.header_parsed);
}

void test_ota_package_parse_epkg_header_success(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t buffer[256];
    uint32_t size;
    create_mock_epkg_header(buffer, &size);
    
    ota_package_info_t info;
    ota_error_t err = ota_package_parse_header(&g_parser_ctx, buffer, size, &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_FALSE(info.is_vpkg);
    TEST_ASSERT_EQUAL(OTA_PKG_MAGIC_EPCK, info.header.epkg.magic);
    TEST_ASSERT_EQUAL(OTA_PKG_CURRENT_VERSION, info.header.epkg.version);
    TEST_ASSERT_EQUAL(0x0101, info.header.epkg.ecu_id);
    TEST_ASSERT_EQUAL(OTA_COMPRESS_NONE, info.header.epkg.compression);
}

void test_ota_package_parse_header_invalid_magic(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t buffer[256];
    memset(buffer, 0xFF, sizeof(buffer));
    
    ota_package_info_t info;
    ota_error_t err = ota_package_parse_header(&g_parser_ctx, buffer, sizeof(buffer), &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_PACKAGE_CORRUPTED, err);
}

void test_ota_package_parse_header_too_small(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t buffer[2] = {0};
    
    ota_package_info_t info;
    ota_error_t err = ota_package_parse_header(&g_parser_ctx, buffer, sizeof(buffer), &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_PACKAGE_CORRUPTED, err);
}

void test_ota_package_parse_header_unsupported_version(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t buffer[256];
    uint32_t size;
    create_mock_vpkg_header(buffer, &size);
    
    /* Change version to unsupported */
    ota_vpkg_header_t *header = (ota_vpkg_header_t*)buffer;
    header->version = 99;
    
    ota_package_info_t info;
    ota_error_t err = ota_package_parse_header(&g_parser_ctx, buffer, size, &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_UNSUPPORTED_FORMAT, err);
}

void test_ota_package_parse_manifest_success(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    /* Simple JSON manifest */
    const char *json = "{"
        "\"manifest_version\":\"1.0.0\","
        "\"id\":\"550e8400-e29b-41d4-a716-446655440000\","
        "\"vin\":\"WBA12345678901234\","
        "\"hw_platform\":\"EV_PLATFORM_V2\","
        "\"campaign_id\":\"CAMP-001\","
        "\"name\":\"Test Campaign\","
        "\"ecu_updates\":["
            "{\"ecu_id\":257,\"name\":\"ECU_1\",\"hardware_version\":\"2.1.0\"}"
        "]"
    "}";
    
    ota_manifest_header_t manifest;
    ota_ecu_update_info_t ecu_updates[8];
    uint8_t num_ecus;
    
    ota_error_t err = ota_package_parse_manifest(
        &g_parser_ctx, json, strlen(json), &manifest, ecu_updates, 8, &num_ecus);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL_STRING("1.0.0", manifest.manifest_version);
    TEST_ASSERT_EQUAL_STRING("550e8400-e29b-41d4-a716-446655440000", manifest.package_id);
    TEST_ASSERT_EQUAL_STRING("WBA12345678901234", manifest.vin);
    TEST_ASSERT_EQUAL_STRING("EV_PLATFORM_V2", manifest.hw_platform);
    TEST_ASSERT_EQUAL_STRING("CAMP-001", manifest.campaign_id);
    TEST_ASSERT_EQUAL_STRING("Test Campaign", manifest.campaign_name);
    TEST_ASSERT_TRUE(g_parser_ctx.manifest_parsed);
}

void test_ota_package_parse_signature(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t sig_data[256];
    memset(sig_data, 0xCC, sizeof(sig_data));
    
    ota_signature_info_t info;
    ota_error_t err = ota_package_parse_signature(
        &g_parser_ctx, sig_data, sizeof(sig_data), OTA_SIG_RAW_ECDSA_P256, &info);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(OTA_SIG_RAW_ECDSA_P256, info.type);
    TEST_ASSERT_EQUAL(256, info.signature_len);
    TEST_ASSERT_NOT_NULL(info.signature_data);
    TEST_ASSERT_TRUE(g_parser_ctx.signature_parsed);
    
    /* Cleanup */
    g_parser_config.free(info.signature_data);
}

void test_ota_package_verify_hash_mock(void)
{
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t data[100];
    memset(data, 0xDD, sizeof(data));
    uint8_t expected_hash[32] = {0};
    bool valid = false;
    
    ota_error_t err = ota_package_verify_hash(
        &g_parser_ctx, data, sizeof(data), expected_hash, OTA_HASH_SHA_256, &valid);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(valid);
}

void test_ota_package_verify_hash_disabled(void)
{
    g_parser_config.verify_hash = false;
    ota_package_parser_init(&g_parser_ctx, &g_parser_config);
    
    uint8_t data[100];
    uint8_t expected_hash[32] = {0};
    bool valid = false;
    
    ota_error_t err = ota_package_verify_hash(
        &g_parser_ctx, data, sizeof(data), expected_hash, OTA_HASH_SHA_256, &valid);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(valid); /* Should pass without verification */
}

void test_ota_decompress_init(void)
{
    uint8_t output_buffer[1024];
    ota_compress_context_t ctx;
    
    ota_error_t err = ota_decompress_init(&ctx, OTA_COMPRESS_NONE, output_buffer, sizeof(output_buffer));
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(ctx.initialized);
    TEST_ASSERT_EQUAL(OTA_COMPRESS_NONE, ctx.type);
    TEST_ASSERT_EQUAL_PTR(output_buffer, ctx.output);
    TEST_ASSERT_EQUAL(1024, ctx.output_size);
}

void test_ota_decompress_block_no_compression(void)
{
    uint8_t output_buffer[1024];
    ota_compress_context_t ctx;
    ota_decompress_init(&ctx, OTA_COMPRESS_NONE, output_buffer, sizeof(output_buffer));
    
    uint8_t input[100];
    memset(input, 0xAA, sizeof(input));
    uint8_t output[200];
    uint32_t output_len;
    bool finished = false;
    
    ota_error_t err = ota_decompress_block(
        &ctx, input, sizeof(input), output, sizeof(output), &output_len, &finished);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(100, output_len);
    TEST_ASSERT_TRUE(finished);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, output, 100);
}

void test_ota_decompress_full(void)
{
    uint8_t output_buffer[1024];
    ota_compress_context_t ctx;
    ota_decompress_init(&ctx, OTA_COMPRESS_NONE, output_buffer, sizeof(output_buffer));
    
    uint8_t input[100];
    memset(input, 0xBB, sizeof(input));
    uint8_t output[200];
    uint32_t output_len;
    
    ota_error_t err = ota_decompress_full(
        &ctx, input, sizeof(input), output, sizeof(output), &output_len);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(100, output_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, output, 100);
}

void test_ota_package_create_vpkg_header(void)
{
    ota_vpkg_header_t header;
    uint8_t package_id[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                               0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};
    
    ota_error_t err = ota_package_create_vpkg_header(&header, package_id,
                                                      "WBA12345678901234", "HW_V2.0");
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(OTA_PKG_MAGIC_VPCK, header.magic);
    TEST_ASSERT_EQUAL(OTA_PKG_CURRENT_VERSION, header.version);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(package_id, header.package_id, 16);
    TEST_ASSERT_EQUAL_STRING("WBA12345678901234", header.vin);
    TEST_ASSERT_EQUAL_STRING("HW_V2.0", header.hw_version);
    TEST_ASSERT_EQUAL(sizeof(ota_vpkg_header_t), header.header_size);
}

void test_ota_package_create_epkg_header(void)
{
    ota_epkg_header_t header;
    
    ota_error_t err = ota_package_create_epkg_header(&header, 0x0101,
                                                      "HW_V2.0", "1.0.0", "2.0.0");
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(OTA_PKG_MAGIC_EPCK, header.magic);
    TEST_ASSERT_EQUAL(OTA_PKG_CURRENT_VERSION, header.version);
    TEST_ASSERT_EQUAL(0x0101, header.ecu_id);
    TEST_ASSERT_EQUAL_STRING("HW_V2.0", header.hw_version);
    TEST_ASSERT_EQUAL_STRING("1.0.0", header.fw_version_from);
    TEST_ASSERT_EQUAL_STRING("2.0.0", header.fw_version_to);
    TEST_ASSERT_EQUAL(sizeof(ota_epkg_header_t), header.header_size);
}

void test_ota_compression_type_name(void)
{
    TEST_ASSERT_EQUAL_STRING("none", ota_compression_type_name(OTA_COMPRESS_NONE));
    TEST_ASSERT_EQUAL_STRING("zstd", ota_compression_type_name(OTA_COMPRESS_ZSTD));
    TEST_ASSERT_EQUAL_STRING("lz4", ota_compression_type_name(OTA_COMPRESS_LZ4));
    TEST_ASSERT_EQUAL_STRING("gzip", ota_compression_type_name(OTA_COMPRESS_GZIP));
    TEST_ASSERT_EQUAL_STRING("unknown", ota_compression_type_name(99));
}

void test_ota_package_magic_to_string(void)
{
    TEST_ASSERT_EQUAL_STRING("VECK", ota_package_magic_to_string(OTA_PKG_MAGIC_VPCK));
    TEST_ASSERT_EQUAL_STRING("EPCK", ota_package_magic_to_string(OTA_PKG_MAGIC_EPCK));
}

void test_ota_decompress_deinit(void)
{
    uint8_t output_buffer[1024];
    ota_compress_context_t ctx;
    ota_decompress_init(&ctx, OTA_COMPRESS_NONE, output_buffer, sizeof(output_buffer));
    
    ota_decompress_deinit(&ctx);
    
    TEST_ASSERT_FALSE(ctx.initialized);
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_ota_package_parser_init_success);
    RUN_TEST(test_ota_package_parser_init_null_params);
    RUN_TEST(test_ota_package_parser_deinit);
    RUN_TEST(test_ota_package_parse_vpkg_header_success);
    RUN_TEST(test_ota_package_parse_epkg_header_success);
    RUN_TEST(test_ota_package_parse_header_invalid_magic);
    RUN_TEST(test_ota_package_parse_header_too_small);
    RUN_TEST(test_ota_package_parse_header_unsupported_version);
    RUN_TEST(test_ota_package_parse_manifest_success);
    RUN_TEST(test_ota_package_parse_signature);
    RUN_TEST(test_ota_package_verify_hash_mock);
    RUN_TEST(test_ota_package_verify_hash_disabled);
    RUN_TEST(test_ota_decompress_init);
    RUN_TEST(test_ota_decompress_block_no_compression);
    RUN_TEST(test_ota_decompress_full);
    RUN_TEST(test_ota_package_create_vpkg_header);
    RUN_TEST(test_ota_package_create_epkg_header);
    RUN_TEST(test_ota_compression_type_name);
    RUN_TEST(test_ota_package_magic_to_string);
    RUN_TEST(test_ota_decompress_deinit);
    
    return UNITY_END();
}
