/**
 * @file test_main.c
 * @brief 单元测试主文件
 */

#include <unity.h>

/* 外部测试套件 */
extern void test_telemetry_suite(void);
extern void test_dds_suite(void);
extern void test_eth_suite(void);
extern void test_diag_suite(void);
extern void test_dlt_suite(void);
extern void test_adapter_suite(void);

void setUp(void) {
    /* 每个测试前执行 */
}

void tearDown(void) {
    /* 每个测试后执行 */
}

int main(void) {
    UNITY_BEGIN();
    
    /* 运行各个测试套件 */
    test_telemetry_suite();
    test_dds_suite();
    test_eth_suite();
    test_diag_suite();
    test_dlt_suite();
    test_adapter_suite();
    
    return UNITY_END();
}
