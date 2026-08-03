/*
 * test_dds_security_manager.c - Unit tests for DDS Security Manager
 */

#include "unity.h"
#include "dds_security_manager.h"
#include <string.h>

static dds_security_config_t test_config;
static dds_security_context_t *sec_ctx = NULL;

void setUp(void)
{
    memset(&test_config, 0, sizeof(test_config));
    test_config.policy_flags = DDS_SEC_POLICY_ALL;
    test_config.auth_plugin = DDS_AUTH_PLUGIN_PKIDH;
    test_config.access_plugin = DDS_ACCESS_PLUGIN_PERMISSIONS;
    test_config.crypto_plugin = DDS_CRYPTO_PLUGIN_AES_GCM_GMAC;
    test_config.required_asil_level = DDS_SECURITY_ASIL_B;
    test_config.enable_audit_log = true;
    test_config.dh_key_size = 2048;
    test_config.handshake_timeout_ms = 5000;
    test_config.key_update_interval_ms = 600000;
}

void tearDown(void)
{
    if (sec_ctx) {
        dds_security_manager_deinit(sec_ctx);
        sec_ctx = NULL;
    }
}

void test_dds_security_manager_init_deinit(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    TEST_ASSERT_TRUE(dds_security_manager_is_initialized(sec_ctx));
    TEST_ASSERT_EQUAL(DDS_SECMGR_STATE_READY, dds_security_manager_get_state(sec_ctx));
    
    dds_security_manager_deinit(sec_ctx);
    sec_ctx = NULL;
}

void test_dds_security_manager_init_null_config(void)
{
    sec_ctx = dds_security_manager_init(NULL);
    TEST_ASSERT_NULL(sec_ctx);
}

void test_dds_security_register_participant(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                          0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C}, 0x10};
    
    dds_security_status_t status = dds_security_register_participant(sec_ctx, &guid, NULL);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
    
    dds_sec_participant_t *participant = dds_security_find_participant(sec_ctx, &guid);
    TEST_ASSERT_NOT_NULL(participant);
    TEST_ASSERT_EQUAL(DDS_SEC_PARTICIPANT_UNAUTHENTICATED, participant->state);
}

void test_dds_security_register_duplicate_participant(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    
    dds_security_register_participant(sec_ctx, &guid, NULL);
    dds_security_status_t status = dds_security_register_participant(sec_ctx, &guid, NULL);
    TEST_ASSERT_EQUAL(DDS_SECURITY_ERROR_ALREADY_INITIALIZED, status);
}

void test_dds_security_unregister_participant(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    
    dds_security_register_participant(sec_ctx, &guid, NULL);
    dds_security_status_t status = dds_security_unregister_participant(sec_ctx, &guid);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
    
    dds_sec_participant_t *participant = dds_security_find_participant(sec_ctx, &guid);
    TEST_ASSERT_NULL(participant);
}

void test_dds_security_find_nonexistent_participant(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    dds_sec_participant_t *participant = dds_security_find_participant(sec_ctx, &guid);
    TEST_ASSERT_NULL(participant);
}

void test_dds_security_log_audit(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    dds_security_status_t status = dds_security_log_audit(sec_ctx, 
                                                           DDS_SEC_EVT_AUTH_SUCCESS,
                                                           DDS_SEC_SEVERITY_INFO,
                                                           NULL,
                                                           "Test audit message");
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
}

void test_dds_security_configure_audit(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    dds_security_status_t status = dds_security_configure_audit(sec_ctx, true, 
                                                                 "/tmp/test_audit.log",
                                                                 DDS_SEC_SEVERITY_WARNING);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
    TEST_ASSERT_TRUE(sec_ctx->audit_mgr.file_logging_enabled);
}

void test_dds_security_trigger_event(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    
    dds_security_status_t status = dds_security_trigger_event(sec_ctx,
                                                               DDS_SEC_EVT_ACCESS_DENIED,
                                                               DDS_SEC_SEVERITY_WARNING,
                                                               &guid,
                                                               "Access denied for topic %s", "TestTopic");
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
}

void test_dds_security_error_string(void)
{
    const char *str = dds_security_error_string(DDS_SECURITY_OK);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("OK", str);
    
    str = dds_security_error_string(DDS_SECURITY_ERROR_ACCESS_DENIED);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
}

void test_dds_security_participant_state_string(void)
{
    const char *str = dds_security_participant_state_string(DDS_SEC_PARTICIPANT_UNAUTHENTICATED);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("UNAUTHENTICATED", str);
    
    str = dds_security_participant_state_string(DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("SECURE_ESTABLISHED", str);
}

void test_dds_security_check_access_allowed(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    dds_security_register_participant(sec_ctx, &guid, NULL);
    
    /* Access control enabled in config but should allow by default */
    dds_security_status_t status = dds_security_check_access(sec_ctx, &guid, 0, 
                                                              "Vehicle/Temperature",
                                                              DDS_PERM_READ);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
}

void test_dds_security_detect_replay(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    dds_security_register_participant(sec_ctx, &guid, NULL);
    
    dds_security_status_t status = dds_security_detect_replay(sec_ctx, &guid, 100);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
}

void test_dds_security_get_statistics(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    rtps_guid_t guid = {{0}, 0};
    dds_security_register_participant(sec_ctx, &guid, NULL);
    dds_security_unregister_participant(sec_ctx, &guid);
    
    uint64_t auth_success = 0, auth_failures = 0, access_violations = 0, replay_detected = 0;
    dds_security_get_statistics(sec_ctx, &auth_success, &auth_failures, 
                                &access_violations, &replay_detected);
    
    /* Stats should be zero initially */
    TEST_ASSERT_EQUAL(0, auth_success);
}

void test_dds_security_maintain(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    dds_security_status_t status = dds_security_maintain(sec_ctx, 0);
    TEST_ASSERT_EQUAL(DDS_SECURITY_OK, status);
}

void test_dds_security_check_certificate_expiry(void)
{
    sec_ctx = dds_security_manager_init(&test_config);
    TEST_ASSERT_NOT_NULL(sec_ctx);
    
    uint32_t expired = dds_security_check_certificate_expiry(sec_ctx, 0);
    /* No participants, should be 0 */
    TEST_ASSERT_EQUAL(0, expired);
}
