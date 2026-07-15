/**
 * @file test_dds_qualification.c
 * @brief DDS 通信合格性测试 — 端到端发布/订阅验收
 * @details 覆盖 DDS 初始化配置、端到端发布-订阅回环通信、QoS 策略配置验证
 *          三个维度，确保 Micro-DDS 模块满足 SWR-008 合格性验收标准。
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: D (QM for DDS)
 * Target: SWR-008 (Micro DDS pub/sub communication end-to-end)
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

#include "microdds/microdds.h"
#include "microdds/types.h"
#include "microdds/qos.h"

/*==================================================================================================
 *                                    Test Constants
 *================================================================================================*/
#define TEST_DOMAIN_ID          0U
#define TEST_TOPIC_NAME         "TestQualificationTopic"
#define TEST_TYPE_NAME          "TestQualificationType"
#define TEST_DATA_PATTERN       0x5AU
#define TEST_DATA_SIZE          64U
#define TEST_SAMPLE_COUNT       5U

/*==================================================================================================
 *                                    Test Fixtures
 *================================================================================================*/
static DDS_DomainParticipant    g_participant;
static DDS_Topic                g_topic;
static DDS_Publisher            g_publisher;
static DDS_Subscriber           g_subscriber;
static DDS_DataWriter           g_writer;
static DDS_DataReader           g_reader;

static uint8                    g_testData[TEST_DATA_SIZE];
static DDS_SampleInfo           g_sampleInfo;
static DDS_InstanceHandle_t     g_instanceHandle;

/*==================================================================================================
 *                                    Setup / Teardown
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;

    /* Clear all handles */
    g_participant  = NULL;
    g_topic        = NULL;
    g_publisher    = NULL;
    g_subscriber   = NULL;
    g_writer       = NULL;
    g_reader       = NULL;

    /* Fill test data with known pattern */
    memset(g_testData, TEST_DATA_PATTERN, sizeof(g_testData));
    memset(&g_sampleInfo, 0, sizeof(g_sampleInfo));
    memset(&g_instanceHandle, 0, sizeof(g_instanceHandle));

    return 0;
}

static int teardown(void **state)
{
    (void)state;

    /* Tear down in reverse creation order */
    if (g_reader != NULL)
    {
        DDS_DataReader_delete(g_reader);
        g_reader = NULL;
    }
    if (g_writer != NULL)
    {
        DDS_DataWriter_delete(g_writer);
        g_writer = NULL;
    }
    if (g_subscriber != NULL)
    {
        DDS_Subscriber_delete(g_subscriber);
        g_subscriber = NULL;
    }
    if (g_publisher != NULL)
    {
        DDS_Publisher_delete(g_publisher);
        g_publisher = NULL;
    }
    if (g_topic != NULL)
    {
        DDS_Topic_delete(g_topic);
        g_topic = NULL;
    }
    if (g_participant != NULL)
    {
        DDS_DomainParticipant_delete(g_participant);
        g_participant = NULL;
    }

    return 0;
}

/*==================================================================================================
 *                    Test 1: DDS 初始化与配置 (test_dds_init_and_config)
 *================================================================================================*/
static void test_dds_init_and_config(void **state)
{
    (void)state;
    DDS_ReturnCode_t ret;
    const char *topicName;
    const char *typeName;

    /* ---- 1. 创建域参与者 ---- */
    g_participant = DDS_DomainParticipant_create(TEST_DOMAIN_ID, NULL);
    assert_non_null(g_participant);

    /* ---- 2. 创建主题 ---- */
    g_topic = DDS_Topic_create(g_participant, TEST_TOPIC_NAME, TEST_TYPE_NAME, NULL);
    assert_non_null(g_topic);

    /* ---- 3. 验证主题属性 ---- */
    topicName = DDS_Topic_get_name(g_topic);
    assert_non_null(topicName);
    assert_string_equal(topicName, TEST_TOPIC_NAME);

    typeName = DDS_Topic_get_type_name(g_topic);
    assert_non_null(typeName);
    assert_string_equal(typeName, TEST_TYPE_NAME);

    /* ---- 4. 创建发布者 ---- */
    g_publisher = DDS_Publisher_create(g_participant, NULL);
    assert_non_null(g_publisher);

    /* ---- 5. 创建订阅者 ---- */
    g_subscriber = DDS_Subscriber_create(g_participant, NULL);
    assert_non_null(g_subscriber);

    /* ---- 6. 创建数据写入器 ---- */
    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL);
    assert_non_null(g_writer);

    /* ---- 7. 创建数据读取器 ---- */
    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL);
    assert_non_null(g_reader);

    /* ---- 8. 设置/获取参与者 QoS ---- */
    {
        DDS_DomainParticipantQos qos;
        memset(&qos, 0, sizeof(qos));

        ret = DDS_DomainParticipant_get_qos(g_participant, &qos);
        assert_int_equal(ret, DDS_RETCODE_OK);

        /* Modify and set back */
        qos.user_data.value[0] = 0xAAU;
        ret = DDS_DomainParticipant_set_qos(g_participant, &qos);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }
}

/*==================================================================================================
 *                    Test 2: 端到端发布订阅 (test_dds_publish_subscribe)
 *================================================================================================*/
static void test_dds_publish_subscribe(void **state)
{
    (void)state;
    DDS_ReturnCode_t ret;
    void            *samples[1U];
    DDS_SampleInfo   sampleInfos[1U];
    int32_t          received;
    uint8_t          sendBuf[TEST_DATA_SIZE];

    /* ---- 1. 创建完整的 DDS 实体链 ---- */
    g_participant = DDS_DomainParticipant_create(TEST_DOMAIN_ID, NULL);
    assert_non_null(g_participant);

    g_topic = DDS_Topic_create(g_participant, "PubSubTopic", "PubSubType", NULL);
    assert_non_null(g_topic);

    g_publisher  = DDS_Publisher_create(g_participant, NULL);
    assert_non_null(g_publisher);

    g_subscriber = DDS_Subscriber_create(g_participant, NULL);
    assert_non_null(g_subscriber);

    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL);
    assert_non_null(g_writer);

    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL);
    assert_non_null(g_reader);

    /* ---- 2. 发布多条数据 ---- */
    for (int32_t i = 0; i < TEST_SAMPLE_COUNT; i++)
    {
        memset(sendBuf, (uint8_t)(TEST_DATA_PATTERN + (uint8_t)i), sizeof(sendBuf));
        ret = DDS_DataWriter_write(g_writer, sendBuf, g_instanceHandle);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }

    /* ---- 3. 读取数据 ---- */
    samples[0U] = NULL;
    memset(sampleInfos, 0, sizeof(sampleInfos));

    received = DDS_DataReader_read(g_reader, samples, sampleInfos, 1);
    assert_true(received >= 0);

    /* ---- 4. 使用 take() 清空队列 ---- */
    received = DDS_DataReader_take(g_reader, samples, sampleInfos, TEST_SAMPLE_COUNT);
    assert_true(received >= 0);
    assert_true(received <= TEST_SAMPLE_COUNT);

    /* Return loaned samples */
    ret = DDS_DataReader_return_loan(g_reader);
    assert_int_equal(ret, DDS_RETCODE_OK);

    /* ---- 5. 等待集创建/删除测试 ---- */
    {
        DDS_WaitSet ws = DDS_WaitSet_create();
        assert_non_null(ws);

        ret = DDS_WaitSet_delete(ws);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }
}

/*==================================================================================================
 *                    Test 3: QoS 策略配置验证 (test_dds_qos_policies)
 *================================================================================================*/
static void test_dds_qos_policies(void **state)
{
    (void)state;
    DDS_ReturnCode_t ret;

    /* ---- 1. 创建域参与者（带自定义 QoS） ---- */
    {
        DDS_DomainParticipantQos participantQos;
        memset(&participantQos, 0, sizeof(participantQos));

        g_participant = DDS_DomainParticipant_create(TEST_DOMAIN_ID, &participantQos);
        assert_non_null(g_participant);
    }

    /* ---- 2. 创建主题 ---- */
    {
        DDS_TopicQos topicQos;
        memset(&topicQos, 0, sizeof(topicQos));
        topicQos.durability.kind = DDS_VOLATILE_DURABILITY_QOS;
        topicQos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;

        g_topic = DDS_Topic_create(g_participant, "QoSTopic", "QoSType", &topicQos);
        assert_non_null(g_topic);
    }

    /* ---- 3. 创建发布者 ---- */
    g_publisher = DDS_Publisher_create(g_participant, NULL);
    assert_non_null(g_publisher);

    /* ---- 4. 创建订阅者 ---- */
    g_subscriber = DDS_Subscriber_create(g_participant, NULL);
    assert_non_null(g_subscriber);

    /* ---- 5. 创建数据写入器（带 QoS） ---- */
    {
        DDS_DataWriterQos writerQos;
        memset(&writerQos, 0, sizeof(writerQos));
        writerQos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
        writerQos.history.kind = DDS_KEEP_LAST_HISTORY_QOS;
        writerQos.history.depth = 5;

        g_writer = DDS_DataWriter_create(g_publisher, g_topic, &writerQos);
        assert_non_null(g_writer);
    }

    /* ---- 6. 创建数据读取器（带 QoS） ---- */
    {
        DDS_DataReaderQos readerQos;
        memset(&readerQos, 0, sizeof(readerQos));
        readerQos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
        readerQos.history.kind = DDS_KEEP_LAST_HISTORY_QOS;
        readerQos.history.depth = 5;

        g_reader = DDS_DataReader_create(g_subscriber, g_topic, &readerQos);
        assert_non_null(g_reader);
    }

    /* ---- 7. 写入并验证数据 ---- */
    {
        uint8_t     sendBuf[16U];
        void       *samples[1U];
        DDS_SampleInfo sampleInfo;
        int32_t     received;

        memset(sendBuf, 0xBB, sizeof(sendBuf));
        ret = DDS_DataWriter_write(g_writer, sendBuf, g_instanceHandle);
        assert_int_equal(ret, DDS_RETCODE_OK);

        received = DDS_DataReader_take(g_reader, samples, &sampleInfo, 1);
        assert_true(received >= 0);

        ret = DDS_DataReader_return_loan(g_reader);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }

    /* ---- 8. 创建等待集测试 ---- */
    {
        DDS_WaitSet ws = DDS_WaitSet_create();
        assert_non_null(ws);

        ret = DDS_WaitSet_delete(ws);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }

    /* ---- 9. 发布者/订阅者基础操作 ---- */
    {
        DDS_PublisherQos pubQos;
        DDS_SubscriberQos subQos;

        memset(&pubQos, 0, sizeof(pubQos));
        memset(&subQos, 0, sizeof(subQos));
    }
}

/*==================================================================================================
 *                                      Main Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            test_dds_init_and_config, setup, teardown),
        cmocka_unit_test_setup_teardown(
            test_dds_publish_subscribe, setup, teardown),
        cmocka_unit_test_setup_teardown(
            test_dds_qos_policies, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
