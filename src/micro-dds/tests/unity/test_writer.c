/** @file test_writer.c
 * @brief 数据写入器测试
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "unity.h"
#include "microdds/microdds.h"

static DDS_DomainParticipant g_participant = NULL_PTR;
static DDS_Topic g_topic = NULL_PTR;
static DDS_Publisher g_publisher = NULL_PTR;
static DDS_DataWriter g_writer = NULL_PTR;

void setUp(void) {
    MicroDDS_init();
    g_participant = DDS_DomainParticipant_create(0, NULL_PTR);
    g_topic = DDS_Topic_create(g_participant, "TestTopic", "TestType", NULL_PTR);
    g_publisher = DDS_Publisher_create(g_participant, NULL_PTR);
}

void tearDown(void) {
    if (g_writer != NULL_PTR) {
        DDS_DataWriter_delete(g_writer);
        g_writer = NULL_PTR;
    }
    if (g_publisher != NULL_PTR) {
        DDS_Publisher_delete(g_publisher);
        g_publisher = NULL_PTR;
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

void test_writer_create_with_valid_params(void) {
    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL_PTR);
    TEST_ASSERT_NOT_NULL(g_writer);
}

void test_writer_create_with_null_publisher(void) {
    g_writer = DDS_DataWriter_create(NULL_PTR, g_topic, NULL_PTR);
    TEST_ASSERT_NULL(g_writer);
}

void test_writer_create_with_null_topic(void) {
    g_writer = DDS_DataWriter_create(g_publisher, NULL_PTR, NULL_PTR);
    TEST_ASSERT_NULL(g_writer);
}

void test_writer_delete_valid_writer(void) {
    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL_PTR);
    TEST_ASSERT_NOT_NULL(g_writer);
    
    DDS_ReturnCode_t rc = DDS_DataWriter_delete(g_writer);
    TEST_ASSERT_EQUAL(DDS_RETCODE_OK, rc);
    g_writer = NULL_PTR;
}

void test_writer_delete_null(void) {
    DDS_ReturnCode_t rc = DDS_DataWriter_delete(NULL_PTR);
    TEST_ASSERT_EQUAL(DDS_RETCODE_BAD_PARAMETER, rc);
}

void test_writer_write_with_null_writer(void) {
    int32_t data = 123;
    DDS_ReturnCode_t rc = DDS_DataWriter_write(NULL_PTR, &data, DDS_HANDLE_NIL);
    TEST_ASSERT_EQUAL(DDS_RETCODE_BAD_PARAMETER, rc);
}

void test_writer_write_with_null_data(void) {
    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL_PTR);
    DDS_ReturnCode_t rc = DDS_DataWriter_write(g_writer, NULL_PTR, DDS_HANDLE_NIL);
    TEST_ASSERT_EQUAL(DDS_RETCODE_BAD_PARAMETER, rc);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_writer_create_with_valid_params);
    RUN_TEST(test_writer_create_with_null_publisher);
    RUN_TEST(test_writer_create_with_null_topic);
    RUN_TEST(test_writer_delete_valid_writer);
    RUN_TEST(test_writer_delete_null);
    RUN_TEST(test_writer_write_with_null_writer);
    RUN_TEST(test_writer_write_with_null_data);
    
    return UNITY_END();
}
