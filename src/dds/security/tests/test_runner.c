/*
 * test_runner.c - Test runner for DDS-Security module tests
 */

#include "unity.h"
#include <stdio.h>

/* External test functions */
extern void test_dds_auth_init_deinit(void);
extern void test_dds_auth_init_null_config(void);
extern void test_dds_auth_init_dh_params(void);
extern void test_dds_auth_generate_dh_keypair(void);
extern void test_dds_auth_compute_shared_secret(void);
extern void test_dds_auth_sha256(void);
extern void test_dds_auth_generate_challenge(void);
extern void test_dds_auth_derive_key(void);
extern void test_dds_auth_sign_verify(void);
extern void test_dds_auth_handshake_create(void);
extern void test_dds_auth_handshake_flow(void);
extern void test_dds_auth_handshake_timeout(void);

extern void test_dds_access_init_deinit(void);
extern void test_dds_access_init_null_config(void);
extern void test_dds_access_match_topic_pattern(void);
extern void test_dds_access_check_permission_allow(void);
extern void test_dds_access_check_permission_deny(void);
extern void test_dds_access_load_participant_permissions(void);
extern void test_dds_access_unload_participant_permissions(void);
extern void test_dds_access_unload_unknown_subject(void);
extern void test_dds_access_get_stats(void);
extern void test_dds_access_clear_permission_cache(void);

extern void test_dds_crypto_init_deinit(void);
extern void test_dds_crypto_init_null_config(void);
extern void test_dds_crypto_generate_key(void);
extern void test_dds_crypto_create_session(void);
extern void test_dds_crypto_find_session(void);
extern void test_dds_crypto_close_session(void);
extern void test_dds_crypto_encrypt_decrypt_aes_gcm(void);
extern void test_dds_crypto_decrypt_with_wrong_key(void);
extern void test_dds_crypto_derive_session_keys(void);
extern void test_dds_crypto_generate_iv(void);
extern void test_dds_crypto_get_stats(void);
extern void test_dds_crypto_get_encrypted_size(void);
extern void test_dds_crypto_get_decrypted_size(void);

extern void test_dds_security_manager_init_deinit(void);
extern void test_dds_security_manager_init_null_config(void);
extern void test_dds_security_register_participant(void);
extern void test_dds_security_register_duplicate_participant(void);
extern void test_dds_security_unregister_participant(void);
extern void test_dds_security_find_nonexistent_participant(void);
extern void test_dds_security_log_audit(void);
extern void test_dds_security_configure_audit(void);
extern void test_dds_security_trigger_event(void);
extern void test_dds_security_error_string(void);
extern void test_dds_security_participant_state_string(void);
extern void test_dds_security_check_access_allowed(void);
extern void test_dds_security_detect_replay(void);
extern void test_dds_security_get_statistics(void);
extern void test_dds_security_maintain(void);
extern void test_dds_security_check_certificate_expiry(void);

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    
    printf("\n=== DDS-Security Module Tests ===\n\n");
    
    printf("--- Authentication Module Tests ---\n");
    RUN_TEST(test_dds_auth_init_deinit);
    RUN_TEST(test_dds_auth_init_null_config);
    RUN_TEST(test_dds_auth_init_dh_params);
    RUN_TEST(test_dds_auth_generate_dh_keypair);
    RUN_TEST(test_dds_auth_compute_shared_secret);
    RUN_TEST(test_dds_auth_sha256);
    RUN_TEST(test_dds_auth_generate_challenge);
    RUN_TEST(test_dds_auth_derive_key);
    RUN_TEST(test_dds_auth_sign_verify);
    RUN_TEST(test_dds_auth_handshake_create);
    RUN_TEST(test_dds_auth_handshake_flow);
    RUN_TEST(test_dds_auth_handshake_timeout);
    
    printf("\n--- Access Control Module Tests ---\n");
    RUN_TEST(test_dds_access_init_deinit);
    RUN_TEST(test_dds_access_init_null_config);
    RUN_TEST(test_dds_access_match_topic_pattern);
    RUN_TEST(test_dds_access_check_permission_allow);
    RUN_TEST(test_dds_access_check_permission_deny);
    RUN_TEST(test_dds_access_load_participant_permissions);
    RUN_TEST(test_dds_access_unload_participant_permissions);
    RUN_TEST(test_dds_access_unload_unknown_subject);
    RUN_TEST(test_dds_access_get_stats);
    RUN_TEST(test_dds_access_clear_permission_cache);
    
    printf("\n--- Cryptography Module Tests ---\n");
    RUN_TEST(test_dds_crypto_init_deinit);
    RUN_TEST(test_dds_crypto_init_null_config);
    RUN_TEST(test_dds_crypto_generate_key);
    RUN_TEST(test_dds_crypto_create_session);
    RUN_TEST(test_dds_crypto_find_session);
    RUN_TEST(test_dds_crypto_close_session);
    RUN_TEST(test_dds_crypto_encrypt_decrypt_aes_gcm);
    RUN_TEST(test_dds_crypto_decrypt_with_wrong_key);
    RUN_TEST(test_dds_crypto_derive_session_keys);
    RUN_TEST(test_dds_crypto_generate_iv);
    RUN_TEST(test_dds_crypto_get_stats);
    RUN_TEST(test_dds_crypto_get_encrypted_size);
    RUN_TEST(test_dds_crypto_get_decrypted_size);
    
    printf("\n--- Security Manager Tests ---\n");
    RUN_TEST(test_dds_security_manager_init_deinit);
    RUN_TEST(test_dds_security_manager_init_null_config);
    RUN_TEST(test_dds_security_register_participant);
    RUN_TEST(test_dds_security_register_duplicate_participant);
    RUN_TEST(test_dds_security_unregister_participant);
    RUN_TEST(test_dds_security_find_nonexistent_participant);
    RUN_TEST(test_dds_security_log_audit);
    RUN_TEST(test_dds_security_configure_audit);
    RUN_TEST(test_dds_security_trigger_event);
    RUN_TEST(test_dds_security_error_string);
    RUN_TEST(test_dds_security_participant_state_string);
    RUN_TEST(test_dds_security_check_access_allowed);
    RUN_TEST(test_dds_security_detect_replay);
    RUN_TEST(test_dds_security_get_statistics);
    RUN_TEST(test_dds_security_maintain);
    RUN_TEST(test_dds_security_check_certificate_expiry);
    
    printf("\n=== All Tests Complete ===\n");
    
    return UNITY_END();
}
