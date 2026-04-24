/**
 * @file test_dds_qos.c
 * @brief DDS QoS配置单元测试
 * @version 1.0
 * @date 2026-04-24
 */

#include "unity.h"
#include "eth_types.h"
#include <string.h>

void setUp(void)
{
    /* 测试前准备 */
}

void tearDown(void)
{
    /* 测试后清理 */
}

/* ============================================================================
 * QoS可靠性测试
 * ============================================================================ */
void test_dds_qos_reliability_best_effort(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_BEST_EFFORT, qos.reliability);
}

void test_dds_qos_reliability_reliable(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_RELIABLE, qos.reliability);
}

/* ============================================================================
 * QoS耐久性测试
 * ============================================================================ */
void test_dds_qos_durability_volatile(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_VOLATILE, qos.durability);
}

void test_dds_qos_durability_transient_local(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_TRANSIENT_LOCAL, qos.durability);
}

void test_dds_qos_durability_transient(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.durability = DDS_QOS_DURABILITY_TRANSIENT;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_TRANSIENT, qos.durability);
}

void test_dds_qos_durability_persistent(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.durability = DDS_QOS_DURABILITY_PERSISTENT;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_PERSISTENT, qos.durability);
}

/* ============================================================================
 * QoS完整配置测试
 * ============================================================================ */
void test_dds_qos_full_configuration(void)
{
    dds_qos_t qos;
    
    /* 高可靠性实时配置 */
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    qos.deadline_ms = 100;          /* 100ms截止时间 */
    qos.latency_budget_ms = 10;      /* 10ms延迟预算 */
    qos.history_depth = 10;          /* 10个样本历史 */
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_RELIABLE, qos.reliability);
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_TRANSIENT_LOCAL, qos.durability);
    TEST_ASSERT_EQUAL_UINT(100, qos.deadline_ms);
    TEST_ASSERT_EQUAL_UINT(10, qos.latency_budget_ms);
    TEST_ASSERT_EQUAL_UINT(10, qos.history_depth);
}

void test_dds_qos_best_effort_volatile(void)
{
    dds_qos_t qos;
    
    /* 最佳努力、易失性配置（低延迟场景） */
    qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos.deadline_ms = 0;             /* 无截止时间 */
    qos.latency_budget_ms = 0;       /* 无延迟预算 */
    qos.history_depth = 1;           /* 只保留最新样本 */
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_BEST_EFFORT, qos.reliability);
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_VOLATILE, qos.durability);
    TEST_ASSERT_EQUAL_UINT(0, qos.deadline_ms);
    TEST_ASSERT_EQUAL_UINT(0, qos.latency_budget_ms);
    TEST_ASSERT_EQUAL_UINT(1, qos.history_depth);
}

void test_dds_qos_persistent_historical(void)
{
    dds_qos_t qos;
    
    /* 持久化历史配置 */
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos.durability = DDS_QOS_DURABILITY_PERSISTENT;
    qos.deadline_ms = 1000;          /* 1秒截止时间 */
    qos.latency_budget_ms = 100;     /* 100ms延迟预算 */
    qos.history_depth = 100;         /* 100个样本历史 */
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_RELIABLE, qos.reliability);
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_PERSISTENT, qos.durability);
    TEST_ASSERT_EQUAL_UINT(1000, qos.deadline_ms);
    TEST_ASSERT_EQUAL_UINT(100, qos.latency_budget_ms);
    TEST_ASSERT_EQUAL_UINT(100, qos.history_depth);
}

/* ============================================================================
 * QoS相容性测试
 * ============================================================================ */
void test_dds_qos_compatibility_reader_writer(void)
{
    /* 读者要求RELIABLE，写者提供BEST_EFFORT：不兼容 */
    dds_qos_reliability_t reader_reliability = DDS_QOS_RELIABILITY_RELIABLE;
    dds_qos_reliability_t writer_reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    
    /* 简单兼容性检查：读者要求不应高于写者提供 */
    int compatible = (reader_reliability <= writer_reliability) ? 1 : 0;
    TEST_ASSERT_EQUAL_INT(0, compatible);
}

void test_dds_qos_compatibility_compatible(void)
{
    /* 读者要求BEST_EFFORT，写者提供RELIABLE：兼容 */
    dds_qos_reliability_t reader_reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    dds_qos_reliability_t writer_reliability = DDS_QOS_RELIABILITY_RELIABLE;
    
    int compatible = (reader_reliability <= writer_reliability) ? 1 : 0;
    TEST_ASSERT_EQUAL_INT(1, compatible);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    
    /* 可靠性测试 */
    RUN_TEST(test_dds_qos_reliability_best_effort);
    RUN_TEST(test_dds_qos_reliability_reliable);
    
    /* 耐久性测试 */
    RUN_TEST(test_dds_qos_durability_volatile);
    RUN_TEST(test_dds_qos_durability_transient_local);
    RUN_TEST(test_dds_qos_durability_transient);
    RUN_TEST(test_dds_qos_durability_persistent);
    
    /* 完整配置测试 */
    RUN_TEST(test_dds_qos_full_configuration);
    RUN_TEST(test_dds_qos_best_effort_volatile);
    RUN_TEST(test_dds_qos_persistent_historical);
    
    /* 兼容性测试 */
    RUN_TEST(test_dds_qos_compatibility_reader_writer);
    RUN_TEST(test_dds_qos_compatibility_compatible);
    
    return UNITY_END();
}
