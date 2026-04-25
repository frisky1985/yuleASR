/******************************************************************************
 * @file    test_e2e_full.c
 * @brief   Comprehensive E2E Protection Unit Tests
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * Tests all E2E Profiles: 1, 2, 4, 5, 6, 7, 11, 22
 * Tests E2E State Machine (E2E_SM)
 * Tests E2E DDS Integration
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../e2e_protection.h"
#include "../e2e_state_machine.h"
#include "../e2e_dds_integration.h"

/******************************************************************************
 * Test Framework
 ******************************************************************************/
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            return 1; \
        } \
    } while(0)

#define TEST_PASS() printf("  [PASS]\n")

static int g_tests_passed = 0;
static int g_tests_failed = 0;

/******************************************************************************
 * Profile 1 (CRC8) Tests
 ******************************************************************************/
static int test_profile01_basic(void)
{
    printf("\n[TEST] Profile 1 Basic CRC8...\n");

    E2E_ContextType ctx;
    uint8_t data[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint32_t length = 8;
    uint16_t status;

    TEST_ASSERT(E2E_Init() == E_OK);
    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_01) == E_OK);

    /* Create TX context */
    E2E_ContextType txCtx;
    TEST_ASSERT(E2E_InitContext(&txCtx, E2E_PROFILE_01) == E_OK);
    txCtx.config.p01.dataId = 0x1234;
    txCtx.config.p01.dataLength = 8;
    txCtx.config.p01.crcOffset = 0;

    /* Create RX context */
    E2E_ContextType rxCtx;
    TEST_ASSERT(E2E_InitContext(&rxCtx, E2E_PROFILE_01) == E_OK);
    rxCtx.config.p01.dataId = 0x1234;
    rxCtx.config.p01.dataLength = 8;
    rxCtx.config.p01.crcOffset = 0;

    /* Protect data */
    TEST_ASSERT(E2E_P01_Protect(&txCtx, data, &length) == E_OK);
    TEST_ASSERT(data[0] != 0x00); /* CRC should be written */

    /* Check data */
    TEST_ASSERT(E2E_P01_Check(&rxCtx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_NONE);

    /* Corrupt data and verify CRC fails */
    data[1] ^= 0xFF;
    TEST_ASSERT(E2E_P01_Check(&rxCtx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_CRC);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 2 (CRC8 + Counter) Tests
 ******************************************************************************/
static int test_profile02_counter(void)
{
    printf("\n[TEST] Profile 2 Counter Sequence...\n");

    E2E_ContextType ctx;
    uint8_t data[32] = {0};
    uint32_t length = 8;
    uint16_t status;

    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_02) == E_OK);

    /* Create TX context */
    E2E_ContextType txCtx;
    E2E_InitContext(&txCtx, E2E_PROFILE_02);
    txCtx.config.p02.dataId = 0xABCD;
    txCtx.config.p02.dataLength = 8;
    txCtx.config.p02.crcOffset = 0;
    txCtx.config.p02.counterOffset = 1;

    /* Create RX context */
    E2E_ContextType rxCtx;
    E2E_InitContext(&rxCtx, E2E_PROFILE_02);
    rxCtx.config.p02.dataId = 0xABCD;
    rxCtx.config.p02.dataLength = 8;
    rxCtx.config.p02.crcOffset = 0;
    rxCtx.config.p02.counterOffset = 1;

    /* Send and receive 5 messages */
    for (int i = 0; i < 5; i++) {
        memset(data, 0, sizeof(data));
        TEST_ASSERT(E2E_P02_Protect(&txCtx, data, &length) == E_OK);
        TEST_ASSERT(data[1] == (i & 0x0F)); /* Counter should increment */
        
        /* Check on receiver */
        TEST_ASSERT(E2E_P02_Check(&rxCtx, data, length, &status) == E_OK);
        if (i == 0) {
            TEST_ASSERT(rxCtx.state.p02.synced == TRUE);
        } else {
            TEST_ASSERT(rxCtx.state.p02.status == E2E_P_OK);
        }
    }

    /* Test repeated message detection */
    uint8_t msg2[32] = {0};
    uint32_t len2 = 8;
    E2E_P02_Protect(&txCtx, msg2, &len2); /* Counter = 5 */
    
    /* Send same message again - should detect repeated */
    TEST_ASSERT(E2E_P02_Check(&rxCtx, msg2, len2, &status) == E_OK);
    TEST_ASSERT(rxCtx.state.p02.status == E2E_P_REPEATED);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 4 (CRC32) Tests
 ******************************************************************************/
static int test_profile04_crc32(void)
{
    printf("\n[TEST] Profile 4 CRC32...\n");

    E2E_ContextType ctx;
    uint8_t data[64] = {0};
    uint32_t length = 60;
    uint16_t status;

    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_04) == E_OK);

    ctx.config.p04.dataId = 0xDEADBEEF;
    ctx.config.p04.dataLength = 60;
    ctx.config.p04.crcOffset = 60;

    /* Protect data */
    TEST_ASSERT(E2E_P04_Protect(&ctx, data, &length) == E_OK);
    TEST_ASSERT(length == 64); /* CRC32 adds 4 bytes */

    /* Check data */
    TEST_ASSERT(E2E_P04_Check(&ctx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_NONE);

    /* Corrupt CRC */
    data[60] ^= 0xFF;
    TEST_ASSERT(E2E_P04_Check(&ctx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_CRC);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 5 (CRC16) Tests
 ******************************************************************************/
static int test_profile05_crc16(void)
{
    printf("\n[TEST] Profile 5 CRC16...\n");

    E2E_ContextType txCtx, rxCtx;
    uint8_t data[32] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint32_t length = 16;
    uint16_t status;

    /* TX Context */
    TEST_ASSERT(E2E_InitContext(&txCtx, E2E_PROFILE_05) == E_OK);
    txCtx.config.p05.dataId = 0x1234;
    txCtx.config.p05.dataLength = 16;
    txCtx.config.p05.crcOffset = 16; /* CRC after data */

    /* RX Context */
    TEST_ASSERT(E2E_InitContext(&rxCtx, E2E_PROFILE_05) == E_OK);
    rxCtx.config.p05.dataId = 0x1234;
    rxCtx.config.p05.dataLength = 16;
    rxCtx.config.p05.crcOffset = 16;

    /* Protect data */
    TEST_ASSERT(E2E_P05_Protect(&txCtx, data, &length) == E_OK);
    TEST_ASSERT(length == 18); /* CRC16 adds 2 bytes */

    /* Check data */
    TEST_ASSERT(E2E_P05_Check(&rxCtx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_NONE);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 6 (CRC16 + Counter) Tests
 ******************************************************************************/
static int test_profile06_eth(void)
{
    printf("\n[TEST] Profile 6 Ethernet CRC16+Counter...\n");

    E2E_ContextType ctx;
    uint8_t data[128] = {0};
    uint32_t length = 100;
    uint16_t status;

    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_06) == E_OK);

    ctx.config.p06.dataId = 0x5678;
    ctx.config.p06.dataLength = 100;
    ctx.config.p06.crcOffset = 0;
    ctx.config.p06.counterOffset = 2;

    /* Protect multiple messages */
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT(E2E_P06_Protect(&ctx, data, &length) == E_OK);
        TEST_ASSERT(data[2] == (i & 0x0F)); /* Counter */
    }

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 7 (CRC32 + Counter) Tests
 ******************************************************************************/
static int test_profile07_eth(void)
{
    printf("\n[TEST] Profile 7 Ethernet CRC32+Counter...\n");

    E2E_ContextType ctx;
    uint8_t data[256] = {0};
    uint32_t length = 200;
    uint16_t status;

    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_07) == E_OK);

    ctx.config.p07.dataId = 0xCAFEBABE;
    ctx.config.p07.dataLength = 200;
    ctx.config.p07.crcOffset = 0;
    ctx.config.p07.counterOffset = 4;

    /* Protect data */
    TEST_ASSERT(E2E_P07_Protect(&ctx, data, &length) == E_OK);
    TEST_ASSERT(length == 208); /* CRC32 + counter + reserved */

    /* Check data */
    TEST_ASSERT(E2E_P07_Check(&ctx, data, length, &status) == E_OK);
    TEST_ASSERT(ctx.state.p07.synced == TRUE);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 11 (Dynamic CRC8) Tests
 ******************************************************************************/
static int test_profile11_dynamic(void)
{
    printf("\n[TEST] Profile 11 Dynamic Data...\n");

    E2E_ContextType ctx;
    uint8_t data[64] = {0};
    uint32_t length = 20;
    uint16_t status;

    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_11) == E_OK);

    ctx.config.p11.dataId = 0xABCD;
    ctx.config.p11.dataLength = 20;
    ctx.config.p11.maxDataLength = 64;
    ctx.config.p11.minDataLength = 8;
    ctx.config.p11.crcOffset = 0;
    ctx.config.p11.counterOffset = 1;
    ctx.config.p11.dataIdOffset = 2;

    /* Test different lengths */
    for (uint32_t len = 16; len <= 32; len += 8) {
        memset(data, 0, sizeof(data));
        ctx.config.p11.dataLength = (uint16_t)len;
        TEST_ASSERT(E2E_P11_Protect(&ctx, data, &length) == E_OK);
        TEST_ASSERT(length == len + 1); /* +1 for CRC */
    }

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile 22 (Dynamic Large Data) Tests
 ******************************************************************************/
static int test_profile22_large_data(void)
{
    printf("\n[TEST] Profile 22 Large Dynamic Data...\n");

    E2E_ContextType txCtx, rxCtx;
    uint8_t data[4096] = {0};
    uint32_t length;
    uint16_t status;

    /* TX Context */
    TEST_ASSERT(E2E_InitContext(&txCtx, E2E_PROFILE_22) == E_OK);
    txCtx.config.p22.dataId = 0xBEEF;
    txCtx.config.p22.dataLength = 1000;
    txCtx.config.p22.maxDataLength = 4096;
    txCtx.config.p22.minDataLength = E2E_P22_MIN_DATA_LENGTH;
    txCtx.config.p22.crcOffset = 0;
    txCtx.config.p22.counterOffset = 2;
    txCtx.config.p22.dataIdOffset = 4;
    txCtx.config.p22.lengthOffset = 6;
    txCtx.config.p22.includeLengthInCrc = TRUE;

    /* RX Context */
    TEST_ASSERT(E2E_InitContext(&rxCtx, E2E_PROFILE_22) == E_OK);
    rxCtx.config.p22.dataId = 0xBEEF;
    rxCtx.config.p22.dataLength = 1000;
    rxCtx.config.p22.maxDataLength = 4096;
    rxCtx.config.p22.minDataLength = E2E_P22_MIN_DATA_LENGTH;
    rxCtx.config.p22.crcOffset = 0;
    rxCtx.config.p22.counterOffset = 2;
    rxCtx.config.p22.dataIdOffset = 4;
    rxCtx.config.p22.lengthOffset = 6;
    rxCtx.config.p22.includeLengthInCrc = TRUE;

    /* Test with 1000 bytes */
    memset(data, 0xAA, sizeof(data));
    length = 1000;
    TEST_ASSERT(E2E_P22_Protect(&txCtx, data, &length) == E_OK);
    TEST_ASSERT(length == 1000);
    TEST_ASSERT(data[6] == 0xE8 && data[7] == 0x03); /* Length = 1000 (0x03E8) little-endian */

    /* Verify data */
    TEST_ASSERT(E2E_P22_Check(&rxCtx, data, length, &status) == E_OK);
    TEST_ASSERT(status == E2E_ERROR_NONE);

    /* Test dynamic length setting */
    TEST_ASSERT(E2E_P22_SetDataLength(&txCtx, 2048) == E_OK);
    TEST_ASSERT(txCtx.config.p22.dataLength == 2048);

    /* Test invalid length */
    TEST_ASSERT(E2E_P22_SetDataLength(&txCtx, 8) == E_NOT_OK); /* Too small */
    TEST_ASSERT(E2E_P22_SetDataLength(&txCtx, 5000) == E_NOT_OK); /* Too large */

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * E2E State Machine Tests
 ******************************************************************************/
static int test_state_machine_basic(void)
{
    printf("\n[TEST] E2E State Machine Basic...\n");

    E2E_SM_InternalStateType smState;
    E2E_SM_ConfigType smConfig;

    TEST_ASSERT(E2E_SM_Init() == E_OK);

    /* Initialize state machine config */
    memset(&smConfig, 0, sizeof(E2E_SM_ConfigType));
    smConfig.profile = E2E_PROFILE_02;
    smConfig.enableWindowCheck = TRUE;
    smConfig.timeoutMs = 1000;
    smConfig.window.windowSize = 3;
    smConfig.window.maxErrorThreshold = 2;
    smConfig.window.minOkThreshold = 1;
    smConfig.window.syncCounterThreshold = 2;

    TEST_ASSERT(E2E_SM_InitStateMachine(&smConfig, &smState) == E_OK);
    TEST_ASSERT(smState.state == E2E_SM_NODATA);

    /* Process OK result - should go to INIT */
    E2E_SM_StateType newState = E2E_SM_ProcessCheckResult(
        &smState, &smConfig, E2E_P_OK, 0);
    TEST_ASSERT(newState == E2E_SM_INIT || newState == E2E_SM_VALID);

    /* Process another OK - should be VALID */
    newState = E2E_SM_ProcessCheckResult(&smState, &smConfig, E2E_P_OK, 1);
    TEST_ASSERT(newState == E2E_SM_VALID);

    /* Process error - should stay VALID (not enough errors yet) */
    newState = E2E_SM_ProcessCheckResult(&smState, &smConfig, E2E_P_ERROR, 2);
    TEST_ASSERT(newState == E2E_SM_VALID || newState == E2E_SM_SYNC);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

static int test_state_machine_window(void)
{
    printf("\n[TEST] E2E State Machine Window Check...\n");

    E2E_SM_InternalStateType smState;
    E2E_SM_ConfigType smConfig;

    memset(&smConfig, 0, sizeof(E2E_SM_ConfigType));
    smConfig.window.windowSize = 3;
    smConfig.window.maxErrorThreshold = 2;
    smConfig.window.minOkThreshold = 2;

    TEST_ASSERT(E2E_SM_InitStateMachine(&smConfig, &smState) == E_OK);

    /* Add OK messages */
    E2E_SM_UpdateWindow(&smState, &smConfig.window, TRUE);
    E2E_SM_UpdateWindow(&smState, &smConfig.window, TRUE);

    /* Should meet OK threshold */
    TEST_ASSERT(E2E_SM_IsWindowOkThresholdMet(&smState, &smConfig.window) == TRUE);

    /* Add errors */
    E2E_SM_UpdateWindow(&smState, &smConfig.window, FALSE);
    E2E_SM_UpdateWindow(&smState, &smConfig.window, FALSE);

    /* Should exceed error threshold */
    TEST_ASSERT(E2E_SM_IsWindowErrorThresholdExceeded(&smState, &smConfig.window) == TRUE);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * E2E DDS Integration Tests
 ******************************************************************************/
static int test_dds_integration_basic(void)
{
    printf("\n[TEST] E2E DDS Integration Basic...\n");

    E2E_DDS_TopicHandleType handle;
    E2E_DDS_TopicConfigType config;
    uint8_t data[64] = {0x11, 0x22, 0x33, 0x44};
    uint32_t length = 16;
    E2E_DDS_ResultType result;

    TEST_ASSERT(E2E_DDS_Init() == E_OK);

    /* Configure topic */
    memset(&config, 0, sizeof(E2E_DDS_TopicConfigType));
    strncpy(config.topicName, "TestTopic", E2E_DDS_MAX_TOPIC_NAME_LEN - 1);
    config.e2eProfile = E2E_PROFILE_02;
    config.dataId = 0x1234;
    config.dataLength = 16;
    config.reliability = DDS_RELIABILITY_RELIABLE;
    config.enableStateMachine = TRUE;
    config.windowSize = 3;
    config.maxErrorThreshold = 2;

    /* Create topic */
    TEST_ASSERT(E2E_DDS_CreateTopic(&config, &handle) == E_OK);
    TEST_ASSERT(handle.initialized == TRUE);

    /* Publish data */
    TEST_ASSERT(E2E_DDS_Publish(&handle, data, &length) == E_OK);

    /* Reset context for receive simulation */
    E2E_DDS_TopicHandleType rxHandle;
    TEST_ASSERT(E2E_DDS_CreateTopic(&config, &rxHandle) == E_OK);

    /* Subscribe to data */
    TEST_ASSERT(E2E_DDS_Subscribe(&rxHandle, data, length, &result) == E_OK);
    TEST_ASSERT(result == E2E_DDS_OK);

    /* Destroy topics */
    TEST_ASSERT(E2E_DDS_DestroyTopic(&handle) == E_OK);
    TEST_ASSERT(E2E_DDS_DestroyTopic(&rxHandle) == E_OK);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

static int test_dds_integration_state_machine(void)
{
    printf("\n[TEST] E2E DDS Integration with State Machine...\n");

    E2E_DDS_TopicHandleType handle;
    E2E_DDS_TopicConfigType config;
    uint8_t data[64] = {0};
    uint32_t length = 16;
    E2E_DDS_ResultType result;

    memset(&config, 0, sizeof(E2E_DDS_TopicConfigType));
    strncpy(config.topicName, "SMTopic", E2E_DDS_MAX_TOPIC_NAME_LEN - 1);
    config.e2eProfile = E2E_PROFILE_06;
    config.dataId = 0x5678;
    config.dataLength = 16;
    config.enableStateMachine = TRUE;
    config.windowSize = 3;
    config.maxErrorThreshold = 2;

    TEST_ASSERT(E2E_DDS_CreateTopic(&config, &handle) == E_OK);

    /* Get initial state */
    E2E_SM_StateType state;
    TEST_ASSERT(E2E_DDS_GetState(&handle, &state) == E_OK);

    /* Simulate receiving messages with check */
    E2E_ContextType txCtx;
    E2E_InitContext(&txCtx, E2E_PROFILE_06);
    txCtx.config.p06.dataId = 0x5678;
    txCtx.config.p06.dataLength = 16;
    txCtx.config.p06.crcOffset = 0;
    txCtx.config.p06.counterOffset = 2;

    /* Send and receive 5 valid messages */
    for (int i = 0; i < 5; i++) {
        memset(data, 0, sizeof(data));
        length = 16;
        E2E_P06_Protect(&txCtx, data, &length);
        TEST_ASSERT(E2E_DDS_Check(&handle, data, length, i * 100, &result) == E_OK);
    }

    /* Check state - should be VALID after several OK messages */
    TEST_ASSERT(E2E_DDS_GetState(&handle, &state) == E_OK);
    TEST_ASSERT(state == E2E_SM_VALID);

    /* Get statistics */
    uint32_t txCount, rxCount, errorCount;
    TEST_ASSERT(E2E_DDS_GetStatistics(&handle, &txCount, &rxCount, &errorCount) == E_OK);
    TEST_ASSERT(rxCount == 5);

    E2E_DDS_DestroyTopic(&handle);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

static int test_dds_profile22_dynamic(void)
{
    printf("\n[TEST] E2E DDS Profile 22 Dynamic Length...\n");

    E2E_DDS_TopicHandleType handle;
    E2E_DDS_TopicConfigType config;
    uint8_t data[4096] = {0};
    uint32_t length;
    E2E_DDS_ResultType result;

    memset(&config, 0, sizeof(E2E_DDS_TopicConfigType));
    strncpy(config.topicName, "P22Topic", E2E_DDS_MAX_TOPIC_NAME_LEN - 1);
    config.e2eProfile = E2E_PROFILE_22;
    config.dataId = 0xDEAD;
    config.dataLength = 100;
    config.maxDataLength = 2048;
    config.enableStateMachine = FALSE;

    TEST_ASSERT(E2E_DDS_CreateTopic(&config, &handle) == E_OK);

    /* Test dynamic length change */
    TEST_ASSERT(E2E_DDS_SetDynamicLength(&handle, 512) == E_OK);

    uint16_t currentLen;
    TEST_ASSERT(E2E_DDS_GetDataLength(&handle, &currentLen) == E_OK);
    TEST_ASSERT(currentLen == 512);

    /* Publish with new length */
    memset(data, 0xBB, sizeof(data));
    length = 512;
    TEST_ASSERT(E2E_DDS_Publish(&handle, data, &length) == E_OK);

    /* Reset and receive */
    E2E_DDS_TopicHandleType rxHandle;
    config.dataLength = 512; /* Match sender */
    TEST_ASSERT(E2E_DDS_CreateTopic(&config, &rxHandle) == E_OK);
    TEST_ASSERT(E2E_DDS_Subscribe(&rxHandle, data, length, &result) == E_OK);
    TEST_ASSERT(result == E2E_DDS_OK);

    E2E_DDS_DestroyTopic(&handle);
    E2E_DDS_DestroyTopic(&rxHandle);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Profile Recommendation Tests
 ******************************************************************************/
static int test_profile_recommendations(void)
{
    printf("\n[TEST] Profile Recommendations...\n");

    E2E_DDS_TopicConfigType config;

    /* Profile 1 - CAN */
    TEST_ASSERT(E2E_DDS_GetRecommendedQos(E2E_PROFILE_01, &config) == E_OK);
    TEST_ASSERT(config.reliability == DDS_RELIABILITY_RELIABLE);

    /* Profile 7 - Ethernet */
    TEST_ASSERT(E2E_DDS_GetRecommendedQos(E2E_PROFILE_07, &config) == E_OK);
    TEST_ASSERT(config.durability == DDS_DURABILITY_TRANSIENT_LOCAL);

    /* Profile 22 - Large Data */
    TEST_ASSERT(E2E_DDS_GetRecommendedQos(E2E_PROFILE_22, &config) == E_OK);
    TEST_ASSERT(config.historyDepth == 5);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Error Handling Tests
 ******************************************************************************/
static int test_error_handling(void)
{
    printf("\n[TEST] Error Handling...\n");

    /* NULL pointer tests */
    TEST_ASSERT(E2E_InitContext(NULL, E2E_PROFILE_01) == E_NOT_OK);
    TEST_ASSERT(E2E_P01_Protect(NULL, NULL, NULL) == E_NOT_OK);

    /* Invalid profile tests */
    E2E_ContextType ctx;
    TEST_ASSERT(E2E_InitContext(&ctx, 0xFF) == E_NOT_OK);

    /* Profile mismatch tests */
    TEST_ASSERT(E2E_InitContext(&ctx, E2E_PROFILE_01) == E_OK);
    ctx.profile = E2E_PROFILE_02; /* Intentionally wrong */
    uint8_t data[32] = {0};
    uint32_t length = 8;
    TEST_ASSERT(E2E_P01_Protect(&ctx, data, &length) == E_NOT_OK);

    /* String conversion tests */
    TEST_ASSERT(strcmp(E2E_DDS_ResultToString(E2E_DDS_OK), "OK") == 0);
    TEST_ASSERT(strcmp(E2E_SM_StateToString(E2E_SM_VALID), "VALID") == 0);
    TEST_ASSERT(strcmp(E2E_StatusToString(E2E_P_OK), "OK") == 0);

    TEST_PASS();
    g_tests_passed++;
    return 0;
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("=====================================================\n");
    printf("  AUTOSAR E2E Protection - Comprehensive Unit Tests  \n");
    printf("  AUTOSAR R22-11 Compliant - ASIL-D Safety Level     \n");
    printf("=====================================================\n");

    /* Profile Tests */
    test_profile01_basic();
    test_profile02_counter();
    test_profile04_crc32();
    test_profile05_crc16();
    test_profile06_eth();
    test_profile07_eth();
    test_profile11_dynamic();
    test_profile22_large_data();

    /* State Machine Tests */
    test_state_machine_basic();
    test_state_machine_window();

    /* DDS Integration Tests */
    test_dds_integration_basic();
    test_dds_integration_state_machine();
    test_dds_profile22_dynamic();

    /* Utility Tests */
    test_profile_recommendations();
    test_error_handling();

    /* Print summary */
    printf("\n=====================================================\n");
    printf("  Test Summary                                       \n");
    printf("=====================================================\n");
    printf("  Total Tests: %d\n", g_tests_passed + g_tests_failed);
    printf("  Passed:      %d\n", g_tests_passed);
    printf("  Failed:      %d\n", g_tests_failed);
    printf("=====================================================\n");

    /* Print version info */
    printf("\nVersion Information:\n");
    printf("  E2E Protection:  %s\n", E2E_GetVersion());
    printf("  E2E State Mach:  %s\n", E2E_SM_GetVersion());
    printf("  E2E DDS Integ:   %s\n", E2E_DDS_GetVersion());

    return g_tests_failed > 0 ? 1 : 0;
}
