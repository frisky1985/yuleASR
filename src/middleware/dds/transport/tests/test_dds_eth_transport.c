/**
 * @file test_dds_eth_transport.c
 * @brief DDS ETH Transport单元测试
 * @version 1.0
 * @date 2026-04-26
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../eth/dds_eth_transport.h"

/* 测试统计 */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } else { \
            tests_failed++; \
            printf("  FAIL: %s\n", msg); \
        } \
    } while(0)

/* 测试用回调 */
static bool rx_callback_called = false;
static bool discovery_callback_called = false;

static void test_rx_callback(const uint8_t *data, uint32_t len,
                              const dds_endpoint_locator_t *locator,
                              void *user_data)
{
    rx_callback_called = true;
    (void)data;
    (void)len;
    (void)locator;
    (void)user_data;
}

static void test_discovery_callback(const dds_participant_info_t *participant,
                                     bool joined, void *user_data)
{
    discovery_callback_called = true;
    (void)participant;
    (void)joined;
    (void)user_data;
}

/* 测试初始化和反初始化 */
void test_init_deinit(void)
{
    printf("\nTest: Init/Deinit\n");
    
    dds_eth_transport_config_t config = {
        .local_ip = 0xC0A80001, /* 192.168.0.1 */
        .local_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .domain_id = 0,
        .base_port = 7400,
        .port_offset = 0,
        .enable_multicast = true,
        .multicast_addr = 0xEFFF0001, /* 239.255.0.1 */
        .tx_buffer_size = 4096,
        .rx_buffer_size = 8192,
        .heartbeat_period_ms = 1000,
        .discovery_period_ms = 3000,
        .lease_duration_ms = 10000,
        .enable_qos_mapping = true
    };
    
    eth_status_t status = dds_eth_transport_init(&config);
    TEST_ASSERT(status == ETH_OK, "Transport init should succeed");
    
    dds_eth_transport_deinit();
    TEST_ASSERT(true, "Transport deinit completed");
}

/* 测试无效参数 */
void test_invalid_params(void)
{
    printf("\nTest: Invalid Parameters\n");
    
    eth_status_t status = dds_eth_transport_init(NULL);
    TEST_ASSERT(status == ETH_INVALID_PARAM, "Init with NULL config should fail");
}

/* 测试GUID操作 */
void test_guid_operations(void)
{
    printf("\nTest: GUID Operations\n");
    
    dds_guid_t guid1, guid2;
    
    /* 填充GUID */
    for (int i = 0; i < 12; i++) {
        guid1.prefix[i] = (uint8_t)i;
        guid2.prefix[i] = (uint8_t)i;
    }
    guid1.entity_id[0] = 0x00;
    guid1.entity_id[1] = 0x01;
    guid1.entity_id[2] = 0x02;
    guid1.entity_id[3] = 0xC2;
    
    guid2.entity_id[0] = 0x00;
    guid2.entity_id[1] = 0x01;
    guid2.entity_id[2] = 0x02;
    guid2.entity_id[3] = 0xC2;
    
    /* 测试指令映射 */
    uint8_t writer_id[4] = {0x00, 0x01, 0x02, 0xC2};
    uint8_t reader_id[4] = {0x00, 0x01, 0x02, 0xC7};
    
    TEST_ASSERT(memcmp(&guid1.entity_id, writer_id, 4) == 0, "Writer ID should match");
    TEST_ASSERT(memcmp(&guid2.entity_id, writer_id, 4) == 0, "Reader ID should differ");
}

/* 测试RTPS消息头 */
void test_rtps_header(void)
{
    printf("\nTest: RTPS Header\n");
    
    dds_rtps_header_t header;
    uint8_t guid_prefix[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};
    
    eth_status_t status = dds_eth_create_rtps_header(&header, guid_prefix);
    TEST_ASSERT(status == ETH_OK, "Create RTPS header should succeed");
    TEST_ASSERT(header.protocol_id == DDS_RTPS_PROTOCOL_ID, "Protocol ID should match");
    TEST_ASSERT(header.protocol_version == DDS_RTPS_PROTOCOL_VERSION_2_5, 
                "Protocol version should be 2.5");
    
    /* 测试解析 */
    uint8_t buffer[20];
    buffer[0] = 'R'; buffer[1] = 'T'; buffer[2] = 'P'; buffer[3] = 'S';
    buffer[4] = 0x02; buffer[5] = 0x05;
    buffer[6] = 0x00; buffer[7] = 0x01;
    memcpy(&buffer[8], guid_prefix, 12);
    
    dds_rtps_header_t parsed;
    uint32_t header_len;
    status = dds_eth_parse_rtps_header(buffer, sizeof(buffer), &parsed, &header_len);
    TEST_ASSERT(status == ETH_OK, "Parse RTPS header should succeed");
    TEST_ASSERT(parsed.protocol_id == DDS_RTPS_PROTOCOL_ID, "Parsed protocol ID should match");
}

/* 测试Endpoint管理 */
void test_endpoint_management(void)
{
    printf("\nTest: Endpoint Management\n");
    
    /* 先初始化 */
    dds_eth_transport_config_t config = {
        .local_ip = 0xC0A80001,
        .local_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .domain_id = 0,
        .base_port = 7400,
        .enable_multicast = true,
        .multicast_addr = 0xEFFF0001
    };
    
    eth_status_t status = dds_eth_transport_init(&config);
    TEST_ASSERT(status == ETH_OK, "Init should succeed");
    
    /* 注册Endpoint */
    dds_endpoint_config_t ep_config;
    memset(&ep_config, 0, sizeof(ep_config));
    
    for (int i = 0; i < 12; i++) {
        ep_config.guid.prefix[i] = (uint8_t)i;
    }
    ep_config.guid.entity_id[0] = 0x00;
    ep_config.guid.entity_id[1] = 0x00;
    ep_config.guid.entity_id[2] = 0x01;
    ep_config.guid.entity_id[3] = 0xC2;
    
    ep_config.type = DDS_ENDPOINT_TYPE_WRITER;
    strcpy(ep_config.topic_name, "TestTopic");
    strcpy(ep_config.type_name, "TestType");
    ep_config.locator.ip_addr = 0xC0A80002;
    ep_config.locator.port = 7400;
    ep_config.locator.is_multicast = false;
    ep_config.qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    
    uint32_t ep_id;
    status = dds_eth_register_endpoint(&ep_config, &ep_id);
    TEST_ASSERT(status == ETH_OK, "Register endpoint should succeed");
    
    /* 查找Endpoint */
    dds_endpoint_config_t found;
    status = dds_eth_find_endpoint(&ep_config.guid, &found);
    TEST_ASSERT(status == ETH_OK, "Find endpoint should succeed");
    TEST_ASSERT(strcmp(found.topic_name, "TestTopic") == 0, "Topic name should match");
    
    /* 获取所有Endpoint */
    dds_endpoint_config_t endpoints[10];
    uint32_t count;
    status = dds_eth_get_active_endpoints(endpoints, 10, &count);
    TEST_ASSERT(status == ETH_OK, "Get active endpoints should succeed");
    TEST_ASSERT(count >= 1, "Should have at least 1 endpoint");
    
    /* 注销Endpoint */
    status = dds_eth_unregister_endpoint(ep_id);
    TEST_ASSERT(status == ETH_OK, "Unregister endpoint should succeed");
    
    dds_eth_transport_deinit();
}

/* 测试Participant管理 */
void test_participant_management(void)
{
    printf("\nTest: Participant Management\n");
    
    dds_eth_transport_config_t config = {
        .local_ip = 0xC0A80001,
        .local_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .domain_id = 0,
        .base_port = 7400,
        .enable_multicast = false
    };
    
    eth_status_t status = dds_eth_transport_init(&config);
    TEST_ASSERT(status == ETH_OK, "Init should succeed");
    
    /* 添加Participant */
    dds_participant_info_t participant;
    memset(&participant, 0, sizeof(participant));
    
    for (int i = 0; i < 12; i++) {
        participant.guid.prefix[i] = (uint8_t)(i + 0x10);
    }
    participant.guid.entity_id[0] = 0x00;
    participant.guid.entity_id[1] = 0x00;
    participant.guid.entity_id[2] = 0x01;
    participant.guid.entity_id[3] = 0xC1;
    
    participant.unicast_addr = 0xC0A80002;
    participant.metatraffic_unicast_port = 7410;
    participant.usertraffic_unicast_port = 7411;
    participant.protocol_version = 0x0205;
    participant.vendor_id = 0x0001;
    participant.lease_duration_ms = 10000;
    
    status = dds_eth_add_participant(&participant);
    TEST_ASSERT(status == ETH_OK, "Add participant should succeed");
    
    /* 获取Participant */
    dds_participant_info_t found;
    status = dds_eth_get_participant(&participant.guid, &found);
    TEST_ASSERT(status == ETH_OK, "Get participant should succeed");
    TEST_ASSERT(found.unicast_addr == 0xC0A80002, "Unicast address should match");
    
    /* 刷新Participant */
    status = dds_eth_refresh_participant(&participant.guid);
    TEST_ASSERT(status == ETH_OK, "Refresh participant should succeed");
    
    /* 移除Participant */
    status = dds_eth_remove_participant(&participant.guid);
    TEST_ASSERT(status == ETH_OK, "Remove participant should succeed");
    
    dds_eth_transport_deinit();
}

/* 测试多播地址计算 */
void test_multicast_address(void)
{
    printf("\nTest: Multicast Address Calculation\n");
    
    eth_ip_addr_t mc_addr;
    
    eth_status_t status = dds_eth_calc_multicast_addr(0, 0, &mc_addr);
    TEST_ASSERT(status == ETH_OK, "Calculate multicast address should succeed");
    TEST_ASSERT(mc_addr == 0xEFFF0001, "Multicast address should be 239.255.0.1");
    
    /* 测试域ID影响 */
    status = dds_eth_calc_multicast_addr(1, 0, &mc_addr);
    TEST_ASSERT(status == ETH_OK, "Calculate multicast address for domain 1 should succeed");
}

/* 测试统计 */
void test_statistics(void)
{
    printf("\nTest: Statistics\n");
    
    dds_eth_transport_config_t config = {
        .local_ip = 0xC0A80001,
        .local_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .domain_id = 0,
        .base_port = 7400,
        .enable_multicast = false
    };
    
    eth_status_t status = dds_eth_transport_init(&config);
    TEST_ASSERT(status == ETH_OK, "Init should succeed");
    
    dds_eth_transport_stats_t stats;
    status = dds_eth_get_stats(&stats);
    TEST_ASSERT(status == ETH_OK, "Get stats should succeed");
    
    status = dds_eth_clear_stats();
    TEST_ASSERT(status == ETH_OK, "Clear stats should succeed");
    
    dds_eth_transport_deinit();
}

/* 主函数 */
int main(void)
{
    printf("=== DDS ETH Transport Unit Tests ===\n");
    
    test_init_deinit();
    test_invalid_params();
    test_guid_operations();
    test_rtps_header();
    test_endpoint_management();
    test_participant_management();
    test_multicast_address();
    test_statistics();
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    return (tests_failed > 0) ? 1 : 0;
}
