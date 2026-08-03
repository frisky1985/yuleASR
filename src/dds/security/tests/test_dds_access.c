/*
 * test_dds_access.c - Unit tests for DDS Access Control module
 */

#include "unity.h"
#include "dds_access.h"
#include <string.h>

static dds_security_config_t test_config;
static dds_access_context_t *access_ctx = NULL;

void setUp(void)
{
    memset(&test_config, 0, sizeof(test_config));
}

void tearDown(void)
{
    if (access_ctx) {
        dds_access_deinit(access_ctx);
        access_ctx = NULL;
    }
}

void test_dds_access_init_deinit(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    TEST_ASSERT_EQUAL(DDS_ACCESS_MAX_SUBJECTS, access_ctx->max_subjects);
    
    dds_access_deinit(access_ctx);
    access_ctx = NULL;
}

void test_dds_access_init_null_config(void)
{
    access_ctx = dds_access_init(NULL);
    TEST_ASSERT_NULL(access_ctx);
}

void test_dds_access_match_topic_pattern(void)
{
    /* Exact match */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Vehicle/Temperature", "Vehicle/Temperature"));
    
    /* Wildcard at end */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Vehicle/*", "Vehicle/Temperature"));
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Vehicle/*", "Vehicle/Pressure"));
    
    /* Wildcard at beginning */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("*/Temperature", "Vehicle/Temperature"));
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("*/Temperature", "Engine/Temperature"));
    
    /* Wildcard in middle */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Vehicle/*/Sensor", "Vehicle/Temp/Sensor"));
    
    /* No match */
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("Vehicle/Temperature", "Engine/Temperature"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("Vehicle/*", "Engine/Temperature"));
}

void test_dds_access_check_permission_allow(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    dds_access_request_t request = {
        .subject_name = "CN=TestUser,O=DDS",
        .domain_id = 0,
        .topic_name = "Vehicle/Temperature",
        .partition_name = NULL,
        .requested_action = DDS_PERM_READ,
        .asil_level = DDS_SECURITY_ASIL_QM,
        .is_encrypted = true,
        .is_authenticated = true
    };
    
    /* With default policy (allow by default) */
    access_ctx->default_policy.default_deny = false;
    dds_access_decision_t decision = dds_access_check_permission(access_ctx, &request);
    TEST_ASSERT_EQUAL(DDS_ACCESS_DECISION_ALLOW, decision);
}

void test_dds_access_check_permission_deny(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    dds_access_request_t request = {
        .subject_name = "CN=TestUser,O=DDS",
        .domain_id = 0,
        .topic_name = "Vehicle/Temperature",
        .partition_name = NULL,
        .requested_action = DDS_PERM_READ,
        .asil_level = DDS_SECURITY_ASIL_QM,
        .is_encrypted = true,
        .is_authenticated = true
    };
    
    /* With strict policy (deny by default) */
    access_ctx->default_policy.default_deny = true;
    dds_access_decision_t decision = dds_access_check_permission(access_ctx, &request);
    TEST_ASSERT_EQUAL(DDS_ACCESS_DECISION_DENY, decision);
}

void test_dds_access_load_participant_permissions(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    const char *subject_name = "CN=TestUser,O=DDS";
    const uint8_t permissions_xml[] = "<permissions/>";
    
    dds_access_status_t status = dds_access_load_participant_permissions(
        access_ctx, subject_name, permissions_xml, sizeof(permissions_xml));
    
    TEST_ASSERT_EQUAL(DDS_ACCESS_OK, status);
    TEST_ASSERT_EQUAL(1, access_ctx->active_subjects);
}

void test_dds_access_unload_participant_permissions(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    const char *subject_name = "CN=TestUser,O=DDS";
    const uint8_t permissions_xml[] = "<permissions/>";
    
    /* Load first */
    dds_access_load_participant_permissions(access_ctx, subject_name, 
                                             permissions_xml, sizeof(permissions_xml));
    TEST_ASSERT_EQUAL(1, access_ctx->active_subjects);
    
    /* Unload */
    dds_access_status_t status = dds_access_unload_participant_permissions(
        access_ctx, subject_name);
    TEST_ASSERT_EQUAL(DDS_ACCESS_OK, status);
    TEST_ASSERT_EQUAL(0, access_ctx->active_subjects);
}

void test_dds_access_unload_unknown_subject(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    dds_access_status_t status = dds_access_unload_participant_permissions(
        access_ctx, "CN=Unknown,O=DDS");
    TEST_ASSERT_EQUAL(DDS_ACCESS_ERROR_SUBJECT_NOT_FOUND, status);
}

void test_dds_access_get_stats(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    /* Make some access checks */
    dds_access_request_t request = {
        .subject_name = "CN=TestUser,O=DDS",
        .domain_id = 0,
        .topic_name = "Vehicle/Temperature",
        .requested_action = DDS_PERM_READ,
        .asil_level = DDS_SECURITY_ASIL_QM,
        .is_encrypted = true,
        .is_authenticated = true
    };
    
    access_ctx->default_policy.default_deny = false;
    dds_access_check_permission(access_ctx, &request);
    dds_access_check_permission(access_ctx, &request);
    
    uint64_t checks = 0, denied = 0, granted = 0;
    dds_access_get_stats(access_ctx, &checks, &denied, &granted);
    
    TEST_ASSERT_EQUAL(2, checks);
    TEST_ASSERT_EQUAL(0, denied);
    TEST_ASSERT_EQUAL(2, granted);
}

void test_dds_access_clear_permission_cache(void)
{
    access_ctx = dds_access_init(&test_config);
    TEST_ASSERT_NOT_NULL(access_ctx);
    
    /* Set cache entry */
    strncpy(access_ctx->permission_cache.last_subject, "CN=TestUser,O=DDS", 
            sizeof(access_ctx->permission_cache.last_subject));
    access_ctx->permission_cache.cache_time = 1000;
    
    /* Clear cache */
    dds_access_clear_permission_cache(access_ctx);
    
    /* Verify cleared */
    TEST_ASSERT_EQUAL(0, access_ctx->permission_cache.last_subject[0]);
    TEST_ASSERT_EQUAL(0, access_ctx->permission_cache.cache_time);
}
