/** @file test_reader.c
 * @brief 数据读取器测试
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "unity.h"
#include "microdds/microdds.h"

static DDS_DomainParticipant g_participant = NULL_PTR;
static DDS_Topic g_topic = NULL_PTR;
static DDS_Subscriber g_subscriber = NULL_PTR;
static DDS_DataReader g_reader = NULL_PTR;

void setUp(void) {
    MicroDDS_init();
    g_participant = DDS_DomainParticipant_create(0, NULL_PTR);
    g_topic = DDS_Topic_create(g_participant, "TestTopic", "TestType", NULL_PTR);
    g_subscriber = DDS_Subscriber_create(g_participant, NULL_PTR);
}

void tearDown(void) {
    if (g_reader != NULL_PTR) {
        DDS_DataReader_delete(g_reader);
        g_reader = NULL_PTR;
    }
    if (g_subscriber != NULL_PTR) {
        DDS_Subscriber_delete(g_subscriber);
        g_subscriber = NULL_PTR;
    }
    if (g_topic != NULL_PTR) {
        DDS_Topic_delete(g_topic);
        g_topic = NULL_PTR;
    }
    if (g_participant != NULL_PTR) {
        DDS_DomainParticipant_delete(g_participant);
        g_participant = NULL_PTR;
    }
    MicroDDS_shutdown();
}

void test_reader_create_with_valid_params(void) {
    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL_PTR);
    TEST_ASSERT_NOT_NULL(g_reader);
}

void test_reader_create_with_null_subscriber(void) {
    g_reader = DDS_DataReader_create(NULL_PTR, g_topic, NULL_PTR);
    TEST_ASSERT_NULL(g_reader);
}

void test_reader_create_with_null_topic(void) {
    g_reader = DDS_DataReader_create(g_subscriber, NULL_PTR, NULL_PTR);
    TEST_ASSERT_NULL(g_reader);
}

void test_reader_delete_valid_reader(void) {
    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL_PTR);
    TEST_ASSERT_NOT_NULL(g_reader);
    
    DDS_ReturnCode_t rc = DDS_DataReader_delete(g_reader);
    TEST_ASSERT_EQUAL(DDS_RETCODE_OK, rc);
    g_reader = NULL_PTR;
}

void test_reader_read_with_null_reader(void) {
    void* samples[1];
    DDS_SampleInfo info;
    int32_t count = DDS_DataReader_read(NULL_PTR, samples, &info, 1);
    TEST_ASSERT_EQUAL(0, count);
}

void test_reader_return_loan(void) {
    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL_PTR);
    DDS_ReturnCode_t rc = DDS_DataReader_return_loan(g_reader);
    TEST_ASSERT_EQUAL(DDS_RETCODE_OK, rc);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_reader_create_with_valid_params);
    RUN_TEST(test_reader_create_with_null_subscriber);
    RUN_TEST(test_reader_create_with_null_topic);
    RUN_TEST(test_reader_delete_valid_reader);
    RUN_TEST(test_reader_read_with_null_reader);
    RUN_TEST(test_reader_return_loan);
    
    return UNITY_END();
}
