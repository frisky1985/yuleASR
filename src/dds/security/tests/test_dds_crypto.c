/*
 * test_dds_crypto.c - Unit tests for DDS Cryptography module
 */

#include "unity.h"
#include "dds_crypto.h"
#include <string.h>

static dds_security_config_t test_config;
static dds_crypto_context_t *crypto_ctx = NULL;

void setUp(void)
{
    memset(&test_config, 0, sizeof(test_config));
    test_config.crypto_alg = DDS_CRYPTO_ALG_AES_256_GCM;
    test_config.key_update_interval_ms = 600000;
}

void tearDown(void)
{
    if (crypto_ctx) {
        dds_crypto_deinit(crypto_ctx);
        crypto_ctx = NULL;
    }
}

void test_dds_crypto_init_deinit(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_ALG_AES_256_GCM, crypto_ctx->algorithm);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_MAX_SESSIONS, crypto_ctx->max_sessions);
    
    dds_crypto_deinit(crypto_ctx);
    crypto_ctx = NULL;
}

void test_dds_crypto_init_null_config(void)
{
    crypto_ctx = dds_crypto_init(NULL);
    TEST_ASSERT_NULL(crypto_ctx);
}

void test_dds_crypto_generate_key(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    uint8_t key1[32];
    uint8_t key2[32];
    
    dds_crypto_status_t status = dds_crypto_generate_key(crypto_ctx, key1, 32);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    status = dds_crypto_generate_key(crypto_ctx, key2, 32);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    /* Keys should be different (with very high probability) */
    TEST_ASSERT_NOT_EQUAL_MEMORY(key1, key2, 32);
    
    /* Keys should not be all zeros */
    uint8_t zero_key[32] = {0};
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key1, zero_key, 32));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key2, zero_key, 32));
}

void test_dds_crypto_create_session(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    rtps_guid_t local_guid = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}, 0x10};
    rtps_guid_t remote_guid = {{0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C}, 0x20};
    
    uint8_t shared_secret[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                                  0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    
    uint64_t session_id = dds_crypto_create_session(crypto_ctx, &local_guid, &remote_guid,
                                                     shared_secret, 32);
    TEST_ASSERT_GREATER_THAN(0, session_id);
    TEST_ASSERT_EQUAL(1, crypto_ctx->active_sessions);
}

void test_dds_crypto_find_session(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    rtps_guid_t local_guid = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}, 0x10};
    rtps_guid_t remote_guid = {{0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C}, 0x20};
    
    uint8_t shared_secret[32] = {0};
    
    uint64_t session_id = dds_crypto_create_session(crypto_ctx, &local_guid, &remote_guid,
                                                     shared_secret, 32);
    TEST_ASSERT_GREATER_THAN(0, session_id);
    
    dds_crypto_session_t *session = dds_crypto_find_session(crypto_ctx, &local_guid, &remote_guid);
    TEST_ASSERT_NOT_NULL(session);
    TEST_ASSERT_EQUAL(session_id, session->session_id);
}

void test_dds_crypto_close_session(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    rtps_guid_t local_guid = {{0}, 0};
    rtps_guid_t remote_guid = {{0}, 0};
    uint8_t shared_secret[32] = {0};
    
    uint64_t session_id = dds_crypto_create_session(crypto_ctx, &local_guid, &remote_guid,
                                                     shared_secret, 32);
    TEST_ASSERT_EQUAL(1, crypto_ctx->active_sessions);
    
    dds_crypto_close_session(crypto_ctx, session_id);
    TEST_ASSERT_EQUAL(0, crypto_ctx->active_sessions);
}

void test_dds_crypto_encrypt_decrypt_aes_gcm(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    uint8_t key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    uint8_t iv[12] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    
    const char *plaintext = "Hello, DDS-Security!";
    uint32_t plaintext_len = strlen(plaintext);
    
    uint8_t ciphertext[256];
    uint8_t tag[16];
    
    /* Encrypt */
    dds_crypto_status_t status = dds_crypto_encrypt_aes_gcm(crypto_ctx, key, 32, iv,
                                                             (const uint8_t*)plaintext, plaintext_len,
                                                             NULL, 0, ciphertext, tag);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    /* Ciphertext should be different from plaintext */
    TEST_ASSERT_NOT_EQUAL_MEMORY(plaintext, ciphertext, plaintext_len);
    
    /* Decrypt */
    uint8_t decrypted[256];
    status = dds_crypto_decrypt_aes_gcm(crypto_ctx, key, 32, iv,
                                        ciphertext, plaintext_len,
                                        NULL, 0, tag, decrypted);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    /* Verify decrypted matches original */
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted, plaintext_len);
}

void test_dds_crypto_decrypt_with_wrong_key(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    uint8_t key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    uint8_t wrong_key[32] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8,
                              0xF7, 0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0,
                              0xEF, 0xEE, 0xED, 0xEC, 0xEB, 0xEA, 0xE9, 0xE8,
                              0xE7, 0xE6, 0xE5, 0xE4, 0xE3, 0xE2, 0xE1, 0xE0};
    uint8_t iv[12] = {0};
    
    const char *plaintext = "Test data";
    uint32_t plaintext_len = strlen(plaintext);
    
    uint8_t ciphertext[256];
    uint8_t tag[16];
    
    /* Encrypt with correct key */
    dds_crypto_encrypt_aes_gcm(crypto_ctx, key, 32, iv,
                               (const uint8_t*)plaintext, plaintext_len,
                               NULL, 0, ciphertext, tag);
    
    /* Decrypt with wrong key - should fail */
    uint8_t decrypted[256];
    dds_crypto_status_t status = dds_crypto_decrypt_aes_gcm(crypto_ctx, wrong_key, 32, iv,
                                                             ciphertext, plaintext_len,
                                                             NULL, 0, tag, decrypted);
    TEST_ASSERT_NOT_EQUAL(DDS_CRYPTO_OK, status);
}

void test_dds_crypto_derive_session_keys(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    uint8_t shared_secret[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                                  0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    
    dds_crypto_key_material_msg_t key_material;
    dds_crypto_status_t status = dds_crypto_derive_session_keys(crypto_ctx, shared_secret, 32, &key_material);
    
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    /* Verify keys are derived (not all zeros) */
    uint8_t zero_key[32] = {0};
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key_material.sender_key, zero_key, 32));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key_material.receiver_key, zero_key, 32));
}

void test_dds_crypto_generate_iv(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    uint8_t shared_secret[32] = {0};
    
    uint64_t session_id = dds_crypto_create_session(crypto_ctx, &guid, &guid, shared_secret, 32);
    TEST_ASSERT_GREATER_THAN(0, session_id);
    
    dds_crypto_session_t *session = dds_crypto_find_session(crypto_ctx, &guid, &guid);
    TEST_ASSERT_NOT_NULL(session);
    
    uint8_t iv1[12];
    uint8_t iv2[12];
    
    dds_crypto_status_t status = dds_crypto_generate_iv(crypto_ctx, session, iv1);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    status = dds_crypto_generate_iv(crypto_ctx, session, iv2);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_OK, status);
    
    /* IVs should be different */
    TEST_ASSERT_NOT_EQUAL_MEMORY(iv1, iv2, 12);
}

void test_dds_crypto_get_stats(void)
{
    crypto_ctx = dds_crypto_init(&test_config);
    TEST_ASSERT_NOT_NULL(crypto_ctx);
    
    uint8_t key[32] = {0};
    uint8_t iv[12] = {0};
    uint8_t plaintext[] = "Test";
    uint8_t ciphertext[256];
    uint8_t tag[16];
    
    /* Perform encryption */
    dds_crypto_encrypt_aes_gcm(crypto_ctx, key, 32, iv, plaintext, 4, NULL, 0, ciphertext, tag);
    
    /* Perform decryption */
    uint8_t decrypted[256];
    dds_crypto_decrypt_aes_gcm(crypto_ctx, key, 32, iv, ciphertext, 4, NULL, 0, tag, decrypted);
    
    /* Check stats */
    uint64_t encrypted = 0, decrypted = 0, failed = 0;
    dds_crypto_get_stats(crypto_ctx, &encrypted, &decrypted, &failed);
    
    TEST_ASSERT_GREATER_THAN(0, encrypted);
    TEST_ASSERT_GREATER_THAN(0, decrypted);
}

void test_dds_crypto_get_encrypted_size(void)
{
    uint32_t plaintext_len = 100;
    uint32_t encrypted_size = dds_crypto_get_encrypted_size(plaintext_len);
    
    /* Should include header + plaintext + tag */
    TEST_ASSERT_GREATER_THAN(plaintext_len, encrypted_size);
}

void test_dds_crypto_get_decrypted_size(void)
{
    uint32_t ciphertext_len = 200;
    uint32_t decrypted_size = dds_crypto_get_decrypted_size(ciphertext_len);
    
    /* Should be less than ciphertext length */
    TEST_ASSERT_LESS_THAN(ciphertext_len, decrypted_size);
}
