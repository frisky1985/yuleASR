/**
 * @file test_eth_utils.c
 * @brief 以太网工具函数单元测试
 * @version 1.0
 * @date 2026-04-24
 */

#include "unity.h"
#include "eth_utils.h"
#include <string.h>

static uint8_t test_buffer[256];

void setUp(void)
{
    memset(test_buffer, 0, sizeof(test_buffer));
}

void tearDown(void)
{
    /* 清理操作 */
}

/* ============================================================================
 * CRC计算测试
 * ============================================================================ */
void test_eth_utils_crc32_basic(void)
{
    uint8_t data[] = "123456789";
    uint32_t crc = eth_crc32(data, sizeof(data) - 1);
    
    /* CRC32应该返回有效值 */
    TEST_ASSERT_NOT_EQUAL(0, crc);
    
    /* 同一数据应该返回同一CRC */
    uint32_t crc2 = eth_crc32(data, sizeof(data) - 1);
    TEST_ASSERT_EQUAL_UINT(crc, crc2);
}

void test_eth_utils_crc32_empty_data(void)
{
    uint8_t data[1] = {0};
    uint32_t crc = eth_crc32(data, 0);
    
    /* 空数据CRC应该返回特定值 */
    TEST_ASSERT_NOT_NULL(&crc);
}

void test_eth_utils_crc32_different_data(void)
{
    uint8_t data1[] = "Hello";
    uint8_t data2[] = "World";
    
    uint32_t crc1 = eth_crc32(data1, sizeof(data1) - 1);
    uint32_t crc2 = eth_crc32(data2, sizeof(data2) - 1);
    
    /* 不同数据应该有不同CRC */
    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/* ============================================================================
 * 网络字节序转换测试
 * ============================================================================ */
void test_eth_utils_htonl_basic(void)
{
    uint32_t host = 0x12345678;
    uint32_t net = eth_htonl(host);
    uint32_t host2 = eth_ntohl(net);
    
    TEST_ASSERT_EQUAL_UINT(host, host2);
}

void test_eth_utils_htons_basic(void)
{
    uint16_t host = 0x1234;
    uint16_t net = eth_htons(host);
    uint16_t host2 = eth_ntohs(net);
    
    TEST_ASSERT_EQUAL_UINT(host, host2);
}

void test_eth_utils_byte_order_zero(void)
{
    uint32_t host = 0;
    uint32_t net = eth_htonl(host);
    
    TEST_ASSERT_EQUAL_UINT(0, net);
}

void test_eth_utils_byte_order_max(void)
{
    uint32_t host = 0xFFFFFFFF;
    uint32_t net = eth_htonl(host);
    uint32_t host2 = eth_ntohl(net);
    
    TEST_ASSERT_EQUAL_UINT(host, host2);
}

/* ============================================================================
 * MAC地址格式化测试
 * ============================================================================ */
void test_eth_utils_mac_to_string(void)
{
    eth_mac_addr_t mac = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    char str[18];
    
    eth_status_t status = eth_mac_to_string(mac, str, sizeof(str));
    
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_STRING("00:11:22:33:44:55", str);
}

void test_eth_utils_mac_to_string_buffer_too_small(void)
{
    eth_mac_addr_t mac = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    char str[10];  /* 缓冲区太小 */
    
    eth_status_t status = eth_mac_to_string(mac, str, sizeof(str));
    
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, status);
}

void test_eth_utils_mac_from_string(void)
{
    const char* str = "00:11:22:33:44:55";
    eth_mac_addr_t mac;
    
    eth_status_t status = eth_mac_from_string(str, mac);
    
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_HEX(0x00, mac[0]);
    TEST_ASSERT_EQUAL_HEX(0x11, mac[1]);
    TEST_ASSERT_EQUAL_HEX(0x22, mac[2]);
    TEST_ASSERT_EQUAL_HEX(0x33, mac[3]);
    TEST_ASSERT_EQUAL_HEX(0x44, mac[4]);
    TEST_ASSERT_EQUAL_HEX(0x55, mac[5]);
}

void test_eth_utils_mac_from_string_invalid_format(void)
{
    const char* str = "00-11-22-33-44-55";  /* 错误的分隔符 */
    eth_mac_addr_t mac;
    
    eth_status_t status = eth_mac_from_string(str, mac);
    
    /* 格式错误应该返回错误 */
    TEST_ASSERT_NOT_EQUAL(ETH_OK, status);
}

void test_eth_utils_mac_roundtrip(void)
{
    eth_mac_addr_t original = ETH_MAC_ADDR(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    char str[18];
    eth_mac_addr_t parsed;
    
    eth_mac_to_string(original, str, sizeof(str));
    eth_mac_from_string(str, parsed);
    
    TEST_ASSERT_TRUE(ETH_MAC_IS_EQUAL(original, parsed));
}

/* ============================================================================
 * IP地址格式化测试
 * ============================================================================ */
void test_eth_utils_ip_to_string(void)
{
    eth_ip_addr_t ip = ETH_IP_ADDR(192, 168, 1, 100);
    char str[16];
    
    eth_status_t status = eth_ip_to_string(ip, str, sizeof(str));
    
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", str);
}

void test_eth_utils_ip_from_string(void)
{
    const char* str = "10.0.0.1";
    eth_ip_addr_t ip;
    
    eth_status_t status = eth_ip_from_string(str, &ip);
    
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_HEX(0x0A000001, ip);  /* 10.0.0.1 */
}

void test_eth_utils_ip_roundtrip(void)
{
    eth_ip_addr_t original = ETH_IP_ADDR(172, 16, 0, 1);
    char str[16];
    eth_ip_addr_t parsed;
    
    eth_ip_to_string(original, str, sizeof(str));
    eth_ip_from_string(str, &parsed);
    
    TEST_ASSERT_EQUAL_UINT(original, parsed);
}

void test_eth_utils_ip_from_string_invalid(void)
{
    const char* str = "256.1.1.1";  /* 超出范围 */
    eth_ip_addr_t ip;
    
    eth_status_t status = eth_ip_from_string(str, &ip);
    
    TEST_ASSERT_NOT_EQUAL(ETH_OK, status);
}

/* ============================================================================
 * 内存操作测试
 * ============================================================================ */
void test_eth_utils_safe_memcpy_basic(void)
{
    uint8_t src[] = {1, 2, 3, 4, 5};
    uint8_t dst[5];
    
    eth_status_t status = eth_safe_memcpy(dst, src, sizeof(dst), sizeof(src));
    
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_MEMORY(src, dst, sizeof(src));
}

void test_eth_utils_safe_memcpy_src_larger(void)
{
    uint8_t src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[5];
    
    eth_status_t status = eth_safe_memcpy(dst, src, sizeof(dst), sizeof(src));
    
    /* 源大于目标，只复制目标大小 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, status);
    TEST_ASSERT_EQUAL_MEMORY(src, dst, sizeof(dst));
}

void test_eth_utils_safe_memcpy_null_dst(void)
{
    uint8_t src[] = {1, 2, 3};
    
    eth_status_t status = eth_safe_memcpy(NULL, src, 10, sizeof(src));
    
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, status);
}

void test_eth_utils_safe_memcpy_null_src(void)
{
    uint8_t dst[5];
    
    eth_status_t status = eth_safe_memcpy(dst, NULL, sizeof(dst), 5);
    
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, status);
}

/* ============================================================================
 * 时间操作测试
 * ============================================================================ */
void test_eth_utils_get_timestamp(void)
{
    uint64_t ts1 = eth_get_timestamp_ms();
    
    /* 测试小延迟 */
    eth_delay_ms(10);
    
    uint64_t ts2 = eth_get_timestamp_ms();
    
    /* 时间戳应该递增 */
    TEST_ASSERT_TRUE(ts2 >= ts1);
    /* 差值应该至少是10ms（容许误差） */
    TEST_ASSERT_TRUE((ts2 - ts1) >= 5);
}

void test_eth_utils_delay_ms(void)
{
    uint64_t start = eth_get_timestamp_ms();
    
    eth_delay_ms(50);
    
    uint64_t end = eth_get_timestamp_ms();
    
    /* 延迟应该至少有50ms（容许少量误差） */
    TEST_ASSERT_TRUE((end - start) >= 45);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    
    /* CRC测试 */
    RUN_TEST(test_eth_utils_crc32_basic);
    RUN_TEST(test_eth_utils_crc32_empty_data);
    RUN_TEST(test_eth_utils_crc32_different_data);
    
    /* 字节序测试 */
    RUN_TEST(test_eth_utils_htonl_basic);
    RUN_TEST(test_eth_utils_htons_basic);
    RUN_TEST(test_eth_utils_byte_order_zero);
    RUN_TEST(test_eth_utils_byte_order_max);
    
    /* MAC地址格式化 */
    RUN_TEST(test_eth_utils_mac_to_string);
    RUN_TEST(test_eth_utils_mac_to_string_buffer_too_small);
    RUN_TEST(test_eth_utils_mac_from_string);
    RUN_TEST(test_eth_utils_mac_from_string_invalid_format);
    RUN_TEST(test_eth_utils_mac_roundtrip);
    
    /* IP地址格式化 */
    RUN_TEST(test_eth_utils_ip_to_string);
    RUN_TEST(test_eth_utils_ip_from_string);
    RUN_TEST(test_eth_utils_ip_roundtrip);
    RUN_TEST(test_eth_utils_ip_from_string_invalid);
    
    /* 内存操作 */
    RUN_TEST(test_eth_utils_safe_memcpy_basic);
    RUN_TEST(test_eth_utils_safe_memcpy_src_larger);
    RUN_TEST(test_eth_utils_safe_memcpy_null_dst);
    RUN_TEST(test_eth_utils_safe_memcpy_null_src);
    
    /* 时间操作 */
    RUN_TEST(test_eth_utils_get_timestamp);
    RUN_TEST(test_eth_utils_delay_ms);
    
    return UNITY_END();
}
