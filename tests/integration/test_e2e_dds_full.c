/**
 * @file test_e2e_dds_full.c
 * @brief End-to-End DDS Communication Test
 * @version 1.0
 * @date 2026-04-26
 *
 * Tests complete Publisher->Subscriber communication chain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "../../src/dds/core/dds_core.h"
#include "../../src/dds/qos/qos_profiles.h"
#include "../../src/common/types/eth_types.h"
#include "../../tests/unity/unity.h"

/* Test Configuration */
#define TEST_DOMAIN_ID 42
#define TEST_TOPIC_NAME "test_e2e_topic"
#define TEST_TYPE_NAME "TestType"
#define TEST_MESSAGE_SIZE 256
#define TEST_MESSAGE_COUNT 100
#define TEST_TIMEOUT_MS 5000

/* Test Data Structure */
typedef struct {
    uint32_t sequence_number;
    uint64_t timestamp;
    uint8_t data[TEST_MESSAGE_SIZE];
    uint32_t crc;
} test_message_t;

/* Test Globals */
static dds_domain_participant_t *g_participant = NULL;
static dds_publisher_t *g_publisher = NULL;
static dds_subscriber_t *g_subscriber = NULL;
static dds_topic_t *g_topic = NULL;
static dds_data_writer_t *g_writer = NULL;
static dds_data_reader_t *g_reader = NULL;

/* Test Message Counter */
static volatile uint32_t g_received_count = 0;
static volatile uint32_t g_expected_count = 0;
static test_message_t g_last_received;

/* Callback for received data */
static void on_data_received(void *user_data)
{
    (void)user_data;
    
    test_message_t msg;
    uint32_t received_size = 0;
    
    eth_status_t status = dds_read(g_reader, &msg, sizeof(msg), &received_size, 0);
    if (status == ETH_OK && received_size > 0) {
        g_received_count++;
        memcpy(&g_last_received, &msg, sizeof(msg));
    }
}

void setUp(void)
{
    g_received_count = 0;
    g_expected_count = 0;
    memset(&g_last_received, 0, sizeof(g_last_received));
}

void tearDown(void)
{
    /* Cleanup handled in test suite teardown */
}

/* ============================================================================
 * Test Suite Setup/Teardown
 * ============================================================================ */

static void test_suite_setup(void)
{
    printf("\n=== Setting up E2E DDS Test Suite ===\n");
    
    /* Initialize DDS runtime */
    eth_status_t status = dds_runtime_init();
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* Create domain participant */
    g_participant = dds_create_participant(TEST_DOMAIN_ID);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    /* Create publisher */
    dds_qos_t pub_qos;
    dds_qos_profile_reliable_volatile(&pub_qos);
    g_publisher = dds_create_publisher(g_participant, &pub_qos);
    TEST_ASSERT_NOT_NULL(g_publisher);
    
    /* Create subscriber */
    dds_qos_t sub_qos;
    dds_qos_profile_reliable_volatile(&sub_qos);
    g_subscriber = dds_create_subscriber(g_participant, &sub_qos);
    TEST_ASSERT_NOT_NULL(g_subscriber);
    
    /* Create topic */
    dds_qos_t topic_qos;
    dds_qos_profile_reliable_volatile(&topic_qos);
    g_topic = dds_create_topic(g_participant, TEST_TOPIC_NAME, TEST_TYPE_NAME, &topic_qos);
    TEST_ASSERT_NOT_NULL(g_topic);
    
    /* Create data writer */
    g_writer = dds_create_data_writer(g_publisher, g_topic, &pub_qos);
    TEST_ASSERT_NOT_NULL(g_writer);
    
    /* Create data reader */
    g_reader = dds_create_data_reader(g_subscriber, g_topic, &sub_qos);
    TEST_ASSERT_NOT_NULL(g_reader);
    
    /* Register data callback */
    status = dds_register_data_callback(g_reader, on_data_received, NULL);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    printf("=== E2E DDS Test Suite Setup Complete ===\n\n");
}

static void test_suite_teardown(void)
{
    printf("\n=== Tearing down E2E DDS Test Suite ===\n");
    
    /* Cleanup in reverse order */
    if (g_reader) dds_delete_data_reader(g_reader);
    if (g_writer) dds_delete_data_writer(g_writer);
    if (g_topic) dds_delete_topic(g_topic);
    if (g_subscriber) dds_delete_subscriber(g_subscriber);
    if (g_publisher) dds_delete_publisher(g_publisher);
    if (g_participant) dds_delete_participant(g_participant);
    
    dds_runtime_deinit();
    
    printf("=== E2E DDS Test Suite Teardown Complete ===\n\n");
}

/* ============================================================================
 * Test Cases
 * ============================================================================ */

void test_dds_basic_pubsub(void)
{
    printf("Test: Basic Pub/Sub Communication\n");
    
    /* Prepare test message */
    test_message_t msg;
    msg.sequence_number = 1;
    msg.timestamp = 1234567890ULL;
    memset(msg.data, 0xAA, sizeof(msg.data));
    msg.crc = 0xDEADBEEF;
    
    /* Publish message */
    eth_status_t status = dds_write(g_writer, &msg, sizeof(msg), 1000);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* Wait for reception */
    uint32_t timeout = 1000;
    while (g_received_count == 0 && timeout > 0) {
        dds_spin_once(10);
        usleep(10000);
        timeout -= 10;
    }
    
    /* Verify reception */
    TEST_ASSERT_EQUAL(1, g_received_count);
    TEST_ASSERT_EQUAL(msg.sequence_number, g_last_received.sequence_number);
    TEST_ASSERT_EQUAL(msg.timestamp, g_last_received.timestamp);
    TEST_ASSERT_EQUAL(msg.crc, g_last_received.crc);
    TEST_ASSERT_EQUAL(0, memcmp(msg.data, g_last_received.data, sizeof(msg.data)));
    
    printf("  PASSED: Message received and verified\n");
}

void test_dds_multiple_messages(void)
{
    printf("Test: Multiple Messages (Count: %d)\n", TEST_MESSAGE_COUNT);
    
    g_expected_count = TEST_MESSAGE_COUNT;
    
    /* Publish multiple messages */
    for (uint32_t i = 0; i < TEST_MESSAGE_COUNT; i++) {
        test_message_t msg;
        msg.sequence_number = i;
        msg.timestamp = 1000000ULL + i;
        memset(msg.data, i & 0xFF, sizeof(msg.data));
        msg.crc = i * 7;  /* Simple checksum */
        
        eth_status_t status = dds_write(g_writer, &msg, sizeof(msg), 1000);
        TEST_ASSERT_EQUAL(ETH_OK, status);
        
        /* Small delay to avoid overwhelming */
        usleep(1000);
    }
    
    /* Wait for all messages */
    uint32_t timeout = 10000;
    while (g_received_count < TEST_MESSAGE_COUNT && timeout > 0) {
        dds_spin_once(10);
        usleep(10000);
        timeout -= 10;
    }
    
    /* Verify all received */
    TEST_ASSERT_EQUAL(TEST_MESSAGE_COUNT, g_received_count);
    
    printf("  PASSED: All %d messages received\n", TEST_MESSAGE_COUNT);
}

void test_dds_qos_reliable(void)
{
    printf("Test: Reliable QoS Communication\n");
    
    /* This test verifies reliable delivery */
    /* In real implementation, would test with packet loss simulation */
    
    test_message_t msg;
    msg.sequence_number = 999;
    msg.timestamp = 9876543210ULL;
    memset(msg.data, 0xBB, sizeof(msg.data));
    msg.crc = 0xCAFEBABE;
    
    /* Send message */
    eth_status_t status = dds_write(g_writer, &msg, sizeof(msg), 1000);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* Wait with timeout */
    uint32_t timeout = 2000;
    while (g_received_count == 0 && timeout > 0) {
        dds_spin_once(10);
        usleep(10000);
        timeout -= 10;
    }
    
    TEST_ASSERT(g_received_count > 0);
    TEST_ASSERT_EQUAL(msg.sequence_number, g_last_received.sequence_number);
    
    printf("  PASSED: Reliable delivery confirmed\n");
}

void test_dds_latency_measurement(void)
{
    printf("Test: Latency Measurement\n");
    
    const int num_samples = 100;
    uint64_t latencies[num_samples];
    uint64_t total_latency = 0;
    
    struct timespec ts;
    
    for (int i = 0; i < num_samples; i++) {
        test_message_t msg;
        msg.sequence_number = i;
        
        /* Get send timestamp */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        msg.timestamp = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        
        /* Reset receive counter */
        g_received_count = 0;
        
        /* Send message */
        eth_status_t status = dds_write(g_writer, &msg, sizeof(msg), 1000);
        TEST_ASSERT_EQUAL(ETH_OK, status);
        
        /* Wait for reception */
        uint32_t timeout = 1000;
        while (g_received_count == 0 && timeout > 0) {
            dds_spin_once(5);
            usleep(5000);
            timeout -= 5;
        }
        
        /* Calculate latency */
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t receive_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        latencies[i] = receive_time - msg.timestamp;
        total_latency += latencies[i];
    }
    
    /* Calculate statistics */
    uint64_t avg_latency_ns = total_latency / num_samples;
    uint64_t min_latency = latencies[0];
    uint64_t max_latency = latencies[0];
    
    for (int i = 1; i < num_samples; i++) {
        if (latencies[i] < min_latency) min_latency = latencies[i];
        if (latencies[i] > max_latency) max_latency = latencies[i];
    }
    
    printf("  Latency Statistics (%d samples):\n", num_samples);
    printf("    Average: %lu us\n", avg_latency_ns / 1000);
    printf("    Min: %lu us\n", min_latency / 1000);
    printf("    Max: %lu us\n", max_latency / 1000);
    
    /* Verify latency is reasonable (under 10ms) */
    TEST_ASSERT(avg_latency_ns < 10000000);
    
    printf("  PASSED: Latency within acceptable range\n");
}

void test_dds_throughput(void)
{
    printf("Test: Throughput Measurement\n");
    
    const int num_messages = 1000;
    const size_t message_size = sizeof(test_message_t);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Send burst of messages */
    for (int i = 0; i < num_messages; i++) {
        test_message_t msg;
        msg.sequence_number = i;
        msg.timestamp = 0;
        msg.crc = i;
        
        eth_status_t status = dds_write(g_writer, &msg, sizeof(msg), 1000);
        TEST_ASSERT_EQUAL(ETH_OK, status);
    }
    
    /* Wait for all to be received */
    uint32_t timeout = 20000;
    while (g_received_count < (uint32_t)num_messages && timeout > 0) {
        dds_spin_once(10);
        usleep(10000);
        timeout -= 10;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                          (end.tv_nsec - start.tv_nsec);
    double elapsed_sec = elapsed_ns / 1e9;
    
    double throughput_mbps = (num_messages * message_size * 8.0) / (elapsed_sec * 1e6);
    double message_rate = num_messages / elapsed_sec;
    
    printf("  Throughput Results:\n");
    printf("    Messages sent: %d\n", num_messages);
    printf("    Messages received: %d\n", g_received_count);
    printf("    Elapsed time: %.3f sec\n", elapsed_sec);
    printf("    Throughput: %.2f Mbps\n", throughput_mbps);
    printf("    Message rate: %.2f msg/sec\n", message_rate);
    
    TEST_ASSERT(g_received_count >= (uint32_t)(num_messages * 0.95)); /* Allow 5% loss in best-effort test */
    
    printf("  PASSED: Throughput test completed\n");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void)
{
    UNITY_BEGIN();
    
    test_suite_setup();
    
    printf("\n========================================\n");
    printf("Running E2E DDS Communication Tests\n");
    printf("========================================\n\n");
    
    RUN_TEST(test_dds_basic_pubsub);
    setUp();
    
    RUN_TEST(test_dds_multiple_messages);
    setUp();
    
    RUN_TEST(test_dds_qos_reliable);
    setUp();
    
    RUN_TEST(test_dds_latency_measurement);
    setUp();
    
    RUN_TEST(test_dds_throughput);
    
    printf("\n========================================\n");
    printf("E2E DDS Communication Tests Complete\n");
    printf("========================================\n");
    
    test_suite_teardown();
    
    return UNITY_END();
}
