/**
 * @file test_eth_buffer.c
 * @brief 以太网缓冲区操作单元测试
 * @version 1.0
 * @date 2026-04-24
 */

#include "unity.h"
#include "eth_types.h"
#include <string.h>

static uint8_t test_data[256];
static eth_buffer_t buffer;

void setUp(void)
{
    memset(test_data, 0, sizeof(test_data));
    buffer.data = test_data;
    buffer.len = 0;
    buffer.max_len = sizeof(test_data);
}

void tearDown(void)
{
    /* 清理操作 */
}

/* ============================================================================
 * 缓冲区初始化测试
 * ============================================================================ */
void test_eth_buffer_init(void)
{
    eth_buffer_t buf;
    uint8_t data[100];
    
    buf.data = data;
    buf.len = 0;
    buf.max_len = 100;
    
    TEST_ASSERT_NOT_NULL(buf.data);
    TEST_ASSERT_EQUAL_UINT(0, buf.len);
    TEST_ASSERT_EQUAL_UINT(100, buf.max_len);
}

void test_eth_buffer_init_null_data(void)
{
    eth_buffer_t buf;
    buf.data = NULL;
    buf.len = 0;
    buf.max_len = 0;
    
    TEST_ASSERT_NULL(buf.data);
}

/* ============================================================================
 * 缓冲区大小管理测试
 * ============================================================================ */
void test_eth_buffer_remaining_space(void)
{
    buffer.len = 50;
    buffer.max_len = 100;
    
    uint32_t remaining = buffer.max_len - buffer.len;
    TEST_ASSERT_EQUAL_UINT(50, remaining);
}

void test_eth_buffer_full(void)
{
    buffer.len = 100;
    buffer.max_len = 100;
    
    TEST_ASSERT_EQUAL_UINT(buffer.len, buffer.max_len);
}

void test_eth_buffer_empty(void)
{
    buffer.len = 0;
    buffer.max_len = 100;
    
    TEST_ASSERT_EQUAL_UINT(0, buffer.len);
}

/* ============================================================================
 * 缓冲区数据操作测试
 * ============================================================================ */
void test_eth_buffer_write_data(void)
{
    uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t len = sizeof(input);
    
    if (len <= buffer.max_len) {
        memcpy(buffer.data, input, len);
        buffer.len = len;
    }
    
    TEST_ASSERT_EQUAL_UINT(len, buffer.len);
    TEST_ASSERT_EQUAL_MEMORY(input, buffer.data, len);
}

void test_eth_buffer_append_data(void)
{
    uint8_t data1[] = {0x01, 0x02};
    uint8_t data2[] = {0x03, 0x04, 0x05};
    
    memcpy(buffer.data, data1, sizeof(data1));
    buffer.len = sizeof(data1);
    
    if ((buffer.len + sizeof(data2)) <= buffer.max_len) {
        memcpy(buffer.data + buffer.len, data2, sizeof(data2));
        buffer.len += sizeof(data2);
    }
    
    TEST_ASSERT_EQUAL_UINT(5, buffer.len);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer.data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[4]);
}

void test_eth_buffer_overflow_protection(void)
{
    uint8_t large_data[300];
    memset(large_data, 0xAA, sizeof(large_data));
    
    /* 模拟防溢出检查 */
    size_t to_copy = (sizeof(large_data) > buffer.max_len) ? buffer.max_len : sizeof(large_data);
    
    memcpy(buffer.data, large_data, to_copy);
    buffer.len = to_copy;
    
    TEST_ASSERT_EQUAL_UINT(buffer.max_len, buffer.len);
    TEST_ASSERT_TRUE(buffer.len <= buffer.max_len);
}

/* ============================================================================
 * 缓冲区清理测试
 * ============================================================================ */
void test_eth_buffer_clear(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    memcpy(buffer.data, data, sizeof(data));
    buffer.len = sizeof(data);
    
    /* 清理缓冲区 */
    memset(buffer.data, 0, buffer.len);
    buffer.len = 0;
    
    TEST_ASSERT_EQUAL_UINT(0, buffer.len);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[2]);
}

void test_eth_buffer_reset(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    memcpy(buffer.data, data, sizeof(data));
    buffer.len = sizeof(data);
    
    /* 重置长度而不清除数据 */
    buffer.len = 0;
    
    TEST_ASSERT_EQUAL_UINT(0, buffer.len);
    /* 数据仍在但被视为无效 */
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[0]);
}

/* ============================================================================
 * 缓冲区比较测试
 * ============================================================================ */
void test_eth_buffer_compare_equal(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    eth_buffer_t buf1 = {
        .data = test_data,
        .len = sizeof(data),
        .max_len = sizeof(test_data)
    };
    
    memcpy(buf1.data, data, sizeof(data));
    
    TEST_ASSERT_EQUAL_MEMORY(data, buf1.data, sizeof(data));
}

void test_eth_buffer_compare_different(void)
{
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x04};
    
    eth_buffer_t buf1;
    uint8_t buf1_data[10];
    buf1.data = buf1_data;
    buf1.len = sizeof(data1);
    buf1.max_len = sizeof(buf1_data);
    memcpy(buf1.data, data1, sizeof(data1));
    
    /* 比较发现不同 */
    int cmp_result = memcmp(buf1.data, data2, sizeof(data1));
    TEST_ASSERT_NOT_EQUAL(0, cmp_result);
}

/* ============================================================================
 * 缓冲区裁剪测试
 * ============================================================================ */
void test_eth_buffer_truncate(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    memcpy(buffer.data, data, sizeof(data));
    buffer.len = sizeof(data);
    
    /* 裁剪到3字节 */
    buffer.len = 3;
    
    TEST_ASSERT_EQUAL_UINT(3, buffer.len);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[2]);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    
    /* 初始化测试 */
    RUN_TEST(test_eth_buffer_init);
    RUN_TEST(test_eth_buffer_init_null_data);
    
    /* 大小管理测试 */
    RUN_TEST(test_eth_buffer_remaining_space);
    RUN_TEST(test_eth_buffer_full);
    RUN_TEST(test_eth_buffer_empty);
    
    /* 数据操作测试 */
    RUN_TEST(test_eth_buffer_write_data);
    RUN_TEST(test_eth_buffer_append_data);
    RUN_TEST(test_eth_buffer_overflow_protection);
    
    /* 清理测试 */
    RUN_TEST(test_eth_buffer_clear);
    RUN_TEST(test_eth_buffer_reset);
    
    /* 比较测试 */
    RUN_TEST(test_eth_buffer_compare_equal);
    RUN_TEST(test_eth_buffer_compare_different);
    
    /* 裁剪测试 */
    RUN_TEST(test_eth_buffer_truncate);
    
    return UNITY_END();
}
