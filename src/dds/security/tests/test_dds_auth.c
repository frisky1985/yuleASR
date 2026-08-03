/*
 * test_dds_auth.c - Unit tests for DDS Authentication module
 */

#include "unity.h"
#include "dds_auth.h"
#include <string.h>

/* Test configuration */
static dds_security_config_t test_config;
static dds_auth_context_t *auth_ctx = NULL;

void setUp(void)
{
    memset(&test_config, 0, sizeof(test_config));
    test_config.dh_key_size = 2048;
    test_config.handshake_timeout_ms = 5000;
    test_config.max_handshake_attempts = 3;
}

void tearDown(void)
{
    if (auth_ctx) {
        dds_auth_deinit(auth_ctx);
        auth_ctx = NULL;
    }
}

/* Test: Initialize and deinitialize authentication context */
void test_dds_auth_init_deinit(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    TEST_ASSERT_EQUAL(DDS_AUTH_MAX_HANDSHAKES, auth_ctx->max_handshakes);
    TEST_ASSERT_EQUAL(test_config.handshake_timeout_ms, auth_ctx->handshake_timeout_ms);
    
    dds_auth_deinit(auth_ctx);
    auth_ctx = NULL;
}

/* Test: Initialize with NULL config should fail */
void test_dds_auth_init_null_config(void)
{
    auth_ctx = dds_auth_init(NULL);
    TEST_ASSERT_NULL(auth_ctx);
}

/* Test: DH parameter initialization */
void test_dds_auth_init_dh_params(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    dds_auth_status_t status = dds_auth_init_dh_params(auth_ctx, DDS_AUTH_DH_GROUP_2048);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_EQUAL(2048, auth_ctx->dh_params.key_size);
    TEST_ASSERT_EQUAL(256, auth_ctx->dh_params.p_len);
}

/* Test: Generate DH keypair */
void test_dds_auth_generate_dh_keypair(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    dds_security_key_pair_t key_pair;
    dds_auth_status_t status = dds_auth_generate_dh_keypair(auth_ctx, &key_pair);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_GREATER_THAN(0, key_pair.private_key_len);
    TEST_ASSERT_GREATER_THAN(0, key_pair.public_key_len);
}

/* Test: Compute shared secret */
void test_dds_auth_compute_shared_secret(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    /* Generate two key pairs */
    dds_security_key_pair_t key1, key2;
    dds_auth_generate_dh_keypair(auth_ctx, &key1);
    dds_auth_generate_dh_keypair(auth_ctx, &key2);
    
    /* Compute shared secrets */
    uint8_t secret1[64], secret2[64];
    uint32_t secret1_len, secret2_len;
    
    dds_auth_compute_shared_secret(auth_ctx, 
                                    key1.private_key, 
                                    key2.public_key, key2.public_key_len,
                                    secret1, &secret1_len);
    
    dds_auth_compute_shared_secret(auth_ctx,
                                    key2.private_key,
                                    key1.public_key, key1.public_key_len,
                                    secret2, &secret2_len);
    
    /* Both parties should derive the same secret */
    TEST_ASSERT_EQUAL(secret1_len, secret2_len);
    TEST_ASSERT_EQUAL_MEMORY(secret1, secret2, secret1_len);
}

/* Test: SHA-256 hash function */
void test_dds_auth_sha256(void)
{
    uint8_t hash[32];
    const char *test_data = "Hello, World!";
    
    dds_auth_status_t status = dds_auth_sha256((const uint8_t*)test_data, 
                                               strlen(test_data), 
                                               hash);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    
    /* Verify hash is not all zeros */
    uint8_t zero_hash[32] = {0};
    TEST_ASSERT_NOT_EQUAL(0, memcmp(hash, zero_hash, 32));
}

/* Test: Generate random challenge */
void test_dds_auth_generate_challenge(void)
{
    uint8_t challenge1[32];
    uint8_t challenge2[32];
    
    dds_auth_generate_challenge(challenge1, 32);
    dds_auth_generate_challenge(challenge2, 32);
    
    /* Challenges should be different (with very high probability) */
    TEST_ASSERT_NOT_EQUAL_MEMORY(challenge1, challenge2, 32);
}

/* Test: Key derivation */
void test_dds_auth_derive_key(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    uint8_t shared_secret[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                                  0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    uint8_t salt[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
    uint8_t key[32];
    
    dds_auth_status_t status = dds_auth_derive_key(auth_ctx, shared_secret, 32, 
                                                   salt, key, 32);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    
    /* Verify key is not all zeros */
    uint8_t zero_key[32] = {0};
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key, zero_key, 32));
}

/* Test: Sign and verify */
void test_dds_auth_sign_verify(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    uint8_t data[] = "Test data to sign";
    uint8_t private_key[32] = {0};
    uint8_t signature[256];
    uint32_t sig_len;
    
    /* Generate private key */
    dds_auth_generate_random(private_key, 32);
    
    /* Sign */
    dds_auth_status_t status = dds_auth_sign(auth_ctx, data, sizeof(data), 
                                             private_key, signature, &sig_len);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    
    /* Verify (simplified - using private key as public key in test) */
    status = dds_auth_verify(auth_ctx, data, sizeof(data),
                            signature, sig_len, private_key);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    
    /* Verify should fail with wrong data */
    uint8_t wrong_data[] = "Wrong data";
    status = dds_auth_verify(auth_ctx, wrong_data, sizeof(wrong_data),
                            signature, sig_len, private_key);
    TEST_ASSERT_EQUAL(DDS_AUTH_ERROR_INVALID_SIGNATURE, status);
}

/* Test: Handshake creation */
void test_dds_auth_handshake_create(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    rtps_guid_t local_guid = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}, 0x10};
    rtps_guid_t remote_guid = {{0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C}, 0x20};
    
    dds_security_handshake_t *handshake = dds_auth_handshake_create(auth_ctx, 
                                                                     &local_guid, 
                                                                     &remote_guid);
    TEST_ASSERT_NOT_NULL(handshake);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_BEGIN, handshake->state);
    TEST_ASSERT_TRUE(guid_equal(&handshake->local_guid, &local_guid));
    TEST_ASSERT_TRUE(guid_equal(&handshake->remote_guid, &remote_guid));
}

/* Test: Complete handshake flow */
void test_dds_auth_handshake_flow(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    rtps_guid_t guid1 = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                          0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}, 0x10};
    rtps_guid_t guid2 = {{0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                          0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C}, 0x20};
    
    /* Initiator: Begin handshake request */
    dds_security_handshake_t *handshake1 = NULL;
    dds_auth_handshake_request_t request;
    dds_auth_status_t status = dds_auth_begin_handshake_request(auth_ctx, &guid1, 
                                                                &handshake1, &request);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_REQUEST_SENT, handshake1->state);
    
    /* Responder: Process request and send reply */
    dds_security_handshake_t *handshake2 = NULL;
    dds_auth_handshake_reply_t reply;
    status = dds_auth_process_handshake_request(auth_ctx, &request, 
                                                &handshake2, &reply);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_REPLY_SENT, handshake2->state);
    
    /* Initiator: Process reply and send final */
    dds_auth_handshake_final_t final;
    status = dds_auth_process_handshake_reply(auth_ctx, handshake1, 
                                              &reply, &final);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_FINAL_SENT, handshake1->state);
    TEST_ASSERT_TRUE(handshake1->verified);
    
    /* Responder: Process final */
    status = dds_auth_process_handshake_final(auth_ctx, handshake2, &final);
    TEST_ASSERT_EQUAL(DDS_AUTH_OK, status);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_COMPLETED, handshake2->state);
    TEST_ASSERT_TRUE(handshake2->verified);
}

/* Test: Handshake timeout check */
void test_dds_auth_handshake_timeout(void)
{
    auth_ctx = dds_auth_init(&test_config);
    TEST_ASSERT_NOT_NULL(auth_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    dds_security_handshake_t *handshake = dds_auth_handshake_create(auth_ctx, &guid, &guid);
    TEST_ASSERT_NOT_NULL(handshake);
    
    handshake->state = DDS_HANDSHAKE_STATE_REQUEST_SENT;
    handshake->start_time = 0;
    handshake->timeout_ms = 100;
    
    uint32_t timeouts = dds_auth_check_handshake_timeouts(auth_ctx, 200);
    TEST_ASSERT_EQUAL(1, timeouts);
    TEST_ASSERT_EQUAL(DDS_HANDSHAKE_STATE_FAILED, handshake->state);
}

/* Test helper function for GUID comparison */
static int guid_equal(const rtps_guid_t *a, const rtps_guid_t *b)
{
    return memcmp(a->prefix, b->prefix, 12) == 0 && a->entity_id == b->entity_id;
}
