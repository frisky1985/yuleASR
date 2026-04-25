/**
 * @file test_ota_manager.c
 * @brief Unit tests for OTA Manager
 * @version 1.0
 * @date 2026-04-26
 */

#include "../../unity/unity.h"
#include "../../../src/ota/ota_manager.h"
#include "../../../src/bootloader/bl_partition.h"
#include <string.h>

/* Test fixtures */
static ota_manager_context_t g_manager_ctx;
static bl_partition_manager_t g_partition_mgr;
static ota_manager_config_t g_manager_config;
static uint8_t g_download_cache[1024 * 1024]; /* 1MB cache */

static ota_state_t g_last_state = OTA_STATE_IDLE;
static ota_error_t g_last_error = OTA_ERR_OK;

/* Mock callbacks */
static void state_change_callback(
    ota_state_t old_state,
    ota_state_t new_state,
    ota_error_t error_code,
    void *user_data
)
{
    (void)old_state;
    (void)user_data;
    g_last_state = new_state;
    g_last_error = error_code;
}

/* Setup and teardown */
void setUp(void)
{
    memset(&g_manager_ctx, 0, sizeof(g_manager_ctx));
    memset(&g_partition_mgr, 0, sizeof(g_partition_mgr));
    memset(&g_manager_config, 0, sizeof(g_manager_config));
    memset(g_download_cache, 0, sizeof(g_download_cache));
    
    g_manager_config.download_timeout_ms = 60000;
    g_manager_config.verify_timeout_ms = 30000;
    g_manager_config.install_timeout_ms = 300000;
    g_manager_config.max_download_retries = 3;
    g_manager_config.max_verify_retries = 3;
    g_manager_config.max_install_retries = 3;
    g_manager_config.auto_rollback_on_failure = true;
    g_manager_config.on_state_change = state_change_callback;
    g_manager_config.user_data = NULL;
    
    g_last_state = OTA_STATE_IDLE;
    g_last_error = OTA_ERR_OK;
}

void tearDown(void)
{
    if (g_manager_ctx.initialized) {
        ota_manager_deinit(&g_manager_ctx);
    }
}

/* Test cases */
void test_ota_manager_init_success(void)
{
    ota_error_t err = ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_TRUE(g_manager_ctx.initialized);
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, g_manager_ctx.current_state);
    TEST_ASSERT_EQUAL(&g_partition_mgr, g_manager_ctx.partition_mgr);
}

void test_ota_manager_init_null_params(void)
{
    ota_error_t err;
    
    /* NULL context */
    err = ota_manager_init(NULL, &g_manager_config, &g_partition_mgr);
    TEST_ASSERT_EQUAL(OTA_ERR_INVALID_PARAM, err);
    
    /* NULL config */
    err = ota_manager_init(&g_manager_ctx, NULL, &g_partition_mgr);
    TEST_ASSERT_EQUAL(OTA_ERR_INVALID_PARAM, err);
}

void test_ota_manager_deinit(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    ota_manager_deinit(&g_manager_ctx);
    
    TEST_ASSERT_FALSE(g_manager_ctx.initialized);
}

void test_ota_manager_receive_campaign_success(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    strcpy(campaign.name, "Test Campaign");
    campaign.num_ecu_updates = 1;
    strcpy(campaign.ecu_updates[0].name, "Test_ECU");
    
    ota_error_t err = ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(OTA_STATE_CAMPAIGN_RECEIVED, g_manager_ctx.current_state);
    TEST_ASSERT_EQUAL(1, g_manager_ctx.num_campaigns);
}

void test_ota_manager_receive_campaign_invalid_id(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    /* Empty campaign ID */
    
    ota_error_t err = ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    
    TEST_ASSERT_EQUAL(OTA_ERR_CAMPAIGN_INVALID, err);
}

void test_ota_manager_receive_multiple_campaigns(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    for (int i = 0; i < 4; i++) {
        ota_campaign_info_t campaign;
        memset(&campaign, 0, sizeof(campaign));
        snprintf(campaign.campaign_id, sizeof(campaign.campaign_id), "CAMP-%03d", i);
        snprintf(campaign.name, sizeof(campaign.name), "Test Campaign %d", i);
        
        ota_error_t err = ota_manager_receive_campaign(&g_manager_ctx, &campaign);
        TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    }
    
    TEST_ASSERT_EQUAL(4, g_manager_ctx.num_campaigns);
}

void test_ota_manager_start_download_success(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* First receive a campaign */
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    campaign.num_ecu_updates = 1;
    strcpy(campaign.ecu_updates[0].firmware_from, "1.0.0");
    strcpy(campaign.ecu_updates[0].firmware_to, "2.0.0");
    campaign.ecu_updates[0].package_size = 100000;
    
    ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    ota_manager_init_download_cache(&g_manager_ctx, g_download_cache, sizeof(g_download_cache));
    
    ota_error_t err = ota_manager_start_download(&g_manager_ctx, "CAMP-001");
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(OTA_STATE_DOWNLOADING, g_manager_ctx.current_state);
    TEST_ASSERT_TRUE(g_manager_ctx.update_in_progress);
}

void test_ota_manager_start_download_invalid_campaign(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_error_t err = ota_manager_start_download(&g_manager_ctx, "NONEXISTENT");
    
    TEST_ASSERT_EQUAL(OTA_ERR_CAMPAIGN_INVALID, err);
}

void test_ota_manager_version_rollback_check(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Create campaign with rollback version */
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    campaign.num_ecu_updates = 1;
    strcpy(campaign.ecu_updates[0].firmware_from, "2.0.0");
    strcpy(campaign.ecu_updates[0].firmware_to, "1.0.0"); /* Rollback! */
    
    ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    
    ota_error_t err = ota_manager_start_download(&g_manager_ctx, "CAMP-001");
    
    TEST_ASSERT_EQUAL(OTA_ERR_VERSION_ROLLBACK, err);
}

void test_ota_manager_pause_resume_download(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Setup campaign */
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    campaign.num_ecu_updates = 1;
    strcpy(campaign.ecu_updates[0].firmware_from, "1.0.0");
    strcpy(campaign.ecu_updates[0].firmware_to, "2.0.0");
    
    ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    ota_manager_start_download(&g_manager_ctx, "CAMP-001");
    
    /* Pause */
    ota_error_t err = ota_manager_pause_download(&g_manager_ctx);
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    
    /* Resume */
    err = ota_manager_resume_download(&g_manager_ctx);
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
}

void test_ota_manager_get_state(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_manager_get_state(&g_manager_ctx));
}

void test_ota_manager_cancel_update(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Setup and start campaign */
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    campaign.num_ecu_updates = 1;
    
    ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    ota_manager_start_download(&g_manager_ctx, "CAMP-001");
    
    /* Cancel */
    ota_manager_cancel_update(&g_manager_ctx);
    
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, g_manager_ctx.current_state);
    TEST_ASSERT_FALSE(g_manager_ctx.update_in_progress);
    TEST_ASSERT_EQUAL(0, g_manager_ctx.num_campaigns);
}

void test_ota_manager_get_campaign(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_campaign_info_t campaign;
    memset(&campaign, 0, sizeof(campaign));
    strcpy(campaign.campaign_id, "CAMP-001");
    strcpy(campaign.name, "Test Campaign");
    
    ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    
    ota_campaign_info_t retrieved;
    ota_error_t err = ota_manager_get_campaign(&g_manager_ctx, "CAMP-001", &retrieved);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL_STRING("CAMP-001", retrieved.campaign_id);
    TEST_ASSERT_EQUAL_STRING("Test Campaign", retrieved.name);
}

void test_ota_manager_get_campaign_not_found(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_campaign_info_t retrieved;
    ota_error_t err = ota_manager_get_campaign(&g_manager_ctx, "NONEXISTENT", &retrieved);
    
    TEST_ASSERT_EQUAL(OTA_ERR_CAMPAIGN_INVALID, err);
}

void test_ota_manager_get_campaign_list(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Add campaigns */
    for (int i = 0; i < 3; i++) {
        ota_campaign_info_t campaign;
        memset(&campaign, 0, sizeof(campaign));
        snprintf(campaign.campaign_id, sizeof(campaign.campaign_id), "CAMP-%03d", i);
        ota_manager_receive_campaign(&g_manager_ctx, &campaign);
    }
    
    ota_campaign_info_t list[4];
    uint8_t count;
    ota_error_t err = ota_manager_get_campaign_list(&g_manager_ctx, list, 4, &count);
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL(3, count);
}

void test_ota_manager_download_cache(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    ota_error_t err = ota_manager_init_download_cache(
        &g_manager_ctx, g_download_cache, sizeof(g_download_cache));
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, err);
    TEST_ASSERT_EQUAL_PTR(g_download_cache, g_manager_ctx.download_cache.buffer);
    TEST_ASSERT_EQUAL(sizeof(g_download_cache), g_manager_ctx.download_cache.buffer_size);
}

void test_ota_manager_can_rollback(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Initially cannot rollback (no partition manager setup) */
    TEST_ASSERT_FALSE(ota_manager_can_rollback(&g_manager_ctx));
}

void test_ota_manager_cyclic_process(void)
{
    ota_manager_init(&g_manager_ctx, &g_manager_config, &g_partition_mgr);
    
    /* Should not crash */
    ota_manager_cyclic_process(&g_manager_ctx);
    
    TEST_ASSERT_TRUE(g_manager_ctx.initialized);
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_ota_manager_init_success);
    RUN_TEST(test_ota_manager_init_null_params);
    RUN_TEST(test_ota_manager_deinit);
    RUN_TEST(test_ota_manager_receive_campaign_success);
    RUN_TEST(test_ota_manager_receive_campaign_invalid_id);
    RUN_TEST(test_ota_manager_receive_multiple_campaigns);
    RUN_TEST(test_ota_manager_start_download_success);
    RUN_TEST(test_ota_manager_start_download_invalid_campaign);
    RUN_TEST(test_ota_manager_version_rollback_check);
    RUN_TEST(test_ota_manager_pause_resume_download);
    RUN_TEST(test_ota_manager_get_state);
    RUN_TEST(test_ota_manager_cancel_update);
    RUN_TEST(test_ota_manager_get_campaign);
    RUN_TEST(test_ota_manager_get_campaign_not_found);
    RUN_TEST(test_ota_manager_get_campaign_list);
    RUN_TEST(test_ota_manager_download_cache);
    RUN_TEST(test_ota_manager_can_rollback);
    RUN_TEST(test_ota_manager_cyclic_process);
    
    return UNITY_END();
}
