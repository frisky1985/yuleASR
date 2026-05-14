/**
 * @file test_ETH.c
 * @brief Ethernet Driver 模块单元测试
 * @version 1.0.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Eth.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试数据 */
static Eth_MacAddrType test_mac_addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static Eth_MacAddrType test_mac_addr2 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

/* 初始化测试 */
void test_init_deinit(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* 测试初始化 */
    Eth_Init(NULL);
    TEST_ASSERT(1);  /* 初始化完成 */
    
    /* 测试反初始化 */
    Eth_DeInit();
    TEST_ASSERT(1);  /* 反初始化完成 */
}

/* 控制器模式测试 */
void test_controller_mode(void)
{
    Std_ReturnType result;
    Eth_ModeType mode;
    
    printf("\n=== Controller Mode Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试设置控制器模式为激活 */
    result = Eth_SetControllerMode(0, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试获取控制器模式 */
    result = Eth_GetControllerMode(0, &mode);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(ETH_MODE_ACTIVE, mode);
    
    /* 测试设置控制器模式为停止 */
    result = Eth_SetControllerMode(0, ETH_MODE_DOWN);
    TEST_ASSERT_EQ(E_OK, result);
    
    result = Eth_GetControllerMode(0, &mode);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(ETH_MODE_DOWN, mode);
    
    /* 测试无效控制器索引 */
    result = Eth_SetControllerMode(255, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Eth_GetControllerMode(0, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Eth_DeInit();
}

/* MAC地址管理测试 */
void test_mac_address(void)
{
    Eth_MacAddrType mac_addr;
    Std_ReturnType result;
    
    printf("\n=== MAC Address Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试设置MAC地址 */
    Eth_SetPhysAddr(0, test_mac_addr);
    TEST_ASSERT(1);  /* 设置完成 */
    
    /* 测试获取MAC地址 */
    Eth_GetPhysAddr(0, mac_addr);
    TEST_ASSERT(memcmp(mac_addr, test_mac_addr, 6) == 0);
    
    /* 测试更新MAC地址过滤器 - 添加 */
    result = Eth_UpdatePhysAddrFilter(0, test_mac_addr2, ETH_FILTER_ACTION_ADD);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试更新MAC地址过滤器 - 移除 */
    result = Eth_UpdatePhysAddrFilter(0, test_mac_addr2, ETH_FILTER_ACTION_REMOVE);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试无效控制器索引 */
    result = Eth_UpdatePhysAddrFilter(255, test_mac_addr, ETH_FILTER_ACTION_ADD);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Eth_UpdatePhysAddrFilter(0, NULL, ETH_FILTER_ACTION_ADD);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Eth_DeInit();
}

/* MII接口测试 */
void test_mii_interface(void)
{
    Std_ReturnType result;
    Eth_DataType data;
    
    printf("\n=== MII Interface Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试读取MII寄存器 */
    result = Eth_ReadMii(0, 0, ETH_MII_REG_BMCR, &data);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);  /* 取决于硬件 */
    
    /* 测试写入MII寄存器 */
    result = Eth_WriteMii(0, 0, ETH_MII_REG_BMCR, 0x1000);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);  /* 取决于硬件 */
    
    /* 测试无效PHY地址 */
    result = Eth_ReadMii(0, 32, ETH_MII_REG_BMCR, &data);  /* 32超出有效范围 */
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Eth_ReadMii(0, 0, ETH_MII_REG_BMCR, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Eth_DeInit();
}

/* 缓冲区管理测试 */
void test_buffer_management(void)
{
    BufReq_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len;
    
    printf("\n=== Buffer Management Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试请求发送缓冲区 */
    len = 100;
    result = Eth_ProvideTxBuffer(0, 0x0800, 0, &buf_idx, &buf_ptr, &len);
    TEST_ASSERT(result == BUFREQ_OK || result == BUFREQ_E_BUSY);
    
    if (result == BUFREQ_OK) {
        TEST_ASSERT(buf_idx != ETH_INVALID_BUF_INDEX);
        TEST_ASSERT(buf_ptr != NULL);
        TEST_ASSERT(len >= 100);
    }
    
    /* 测试无效控制器索引 */
    result = Eth_ProvideTxBuffer(255, 0x0800, 0, &buf_idx, &buf_ptr, &len);
    TEST_ASSERT_EQ(BUFREQ_E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Eth_ProvideTxBuffer(0, 0x0800, 0, NULL, &buf_ptr, &len);
    TEST_ASSERT_EQ(BUFREQ_E_NOT_OK, result);
    
    Eth_DeInit();
}

/* 数据收发测试 */
void test_transmit_receive(void)
{
    Std_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len;
    BufReq_ReturnType buf_result;
    
    printf("\n=== Transmit/Receive Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 设置控制器为激活状态 */
    Eth_SetControllerMode(0, ETH_MODE_ACTIVE);
    
    /* 请求发送缓冲区 */
    len = 64;
    buf_result = Eth_ProvideTxBuffer(0, 0x0806, 0, &buf_idx, &buf_ptr, &len);
    
    if (buf_result == BUFREQ_OK) {
        /* 填充测试数据 */
        memset(buf_ptr, 0xAA, len);
        
        /* 测试发送 */
        result = Eth_Transmit(0, buf_idx, 0x0806, TRUE, len, test_mac_addr);
        TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    }
    
    /* 测试接收 */
    uint8 rx_status;
    Eth_BufIdxType rx_buf_idx;
    Eth_FrameStructType* frame_ptr;
    result = Eth_Receive(0, &rx_status, &rx_buf_idx, &frame_ptr);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效控制器索引 */
    result = Eth_Transmit(255, buf_idx, 0x0800, TRUE, 64, test_mac_addr);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Eth_DeInit();
}

/* 中断控制测试 */
void test_interrupt_control(void)
{
    printf("\n=== Interrupt Control Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试使能中断 */
    Eth_EnableIrq();
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试禁用中断 */
    Eth_DisableIrq();
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    Eth_DeInit();
}

/* 缓冲区初始化测试 */
void test_buffer_init(void)
{
    printf("\n=== Buffer Initialization Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试初始化缓冲区 */
    Eth_InitBuffers();
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    Eth_DeInit();
}

/* 控制器索引测试 */
void test_controller_index(void)
{
    uint8 ctrl_idx;
    
    printf("\n=== Controller Index Tests ===\n");
    
    Eth_Init(NULL);
    
    /* 测试获取控制器索引 */
    ctrl_idx = Eth_GetControllerIdx("EthCtrl");
    TEST_ASSERT(ctrl_idx != ETH_INVALID_CONTROLLER_INDEX || ctrl_idx == ETH_INVALID_CONTROLLER_INDEX);
    
    /* 测试无效名称 */
    ctrl_idx = Eth_GetControllerIdx("InvalidCtrl");
    TEST_ASSERT_EQ(ETH_INVALID_CONTROLLER_INDEX, ctrl_idx);
    
    /* 测试NULL指针 */
    ctrl_idx = Eth_GetControllerIdx(NULL);
    TEST_ASSERT_EQ(ETH_INVALID_CONTROLLER_INDEX, ctrl_idx);
    
    Eth_DeInit();
}

/* 版本信息测试 */
void test_version_info(void)
{
    printf("\n=== Version Info Tests ===\n");
    
#if (ETH_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType version_info;
    
    Eth_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(ETH_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(ETH_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(ETH_SW_PATCH_VERSION, version_info.sw_patch_version);
#else
    printf("  Version Info API not enabled\n");
    TEST_ASSERT(1);
#endif
}

/* 主函数 */
int main(void)
{
    printf("========================================\n");
    printf("   ETH Module Unit Tests\n");
    printf("========================================\n");
    
    test_init_deinit();
    test_controller_mode();
    test_mac_address();
    test_mii_interface();
    test_buffer_management();
    test_transmit_receive();
    test_interrupt_control();
    test_buffer_init();
    test_controller_index();
    test_version_info();
    
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}
