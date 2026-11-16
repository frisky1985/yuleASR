/**
 * @file test_eth_automotive.c
 * @brief 汽车以太网 PHY 驱动单元测试 — 诊断结果映射
 * @version 1.0
 * @date 2026-11-16
 *
 * 覆盖 automotive_phy_run_diagnostics():
 *   - 未初始化时返回 ETH_NOT_INIT
 *   - 参数为空时返回 ETH_INVALID_PARAM
 *   - 链路状态中 int8_t 的 "不可用(-1)" 质量/长度不能以无符号形式
 *     泄漏为 0xFFFFFFFF / 0xFF，需映射为 "无有效值"
 *
 * 说明: eth_automotive.c 内置 MDIO 寄存器存根(固定返回 0xFFFF -> 链路 UP)，
 * 且未实现型号级诊断，因此 link_status.signal_quality / cable_length 在
 * init 后保持 -1。
 *
 * 测试框架仿 test_eth_manager.c: TEST_ASSERT 宏 + main 逐个 run_test。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "eth_automotive.h"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

extern uint16_t g_mdio_regs[256];

static void test_eth_reset_mdio(void)
{
    memset(g_mdio_regs, 0, sizeof(g_mdio_regs));
}

static automotive_phy_config_t make_config(void)
{
    automotive_phy_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.phy_type = AUTOMOTIVE_PHY_GENERIC;
    cfg.standard = AUTOMOTIVE_ETH_STANDARD_100BASE_T1;
    cfg.phy_addr = 0U;
    cfg.enable_cable_diag = true;

    return cfg;
}

/* 必须在初始化之前执行 */
/* 访问 mdio 存根写入的控制寄存器 (PHY_REG_CONTROL = 0x00) */
#define PHY_REG_CONTROL_EXTERNAL   0x00U
#define PHY_CTRL_POWER_DOWN_EXTERNAL 0x0800U

static int test_run_diagnostics_before_init_should_not_init(void)
{
    test_eth_reset_mdio();
    automotive_phy_diagnostics_t diag;

    memset(&diag, 0, sizeof(diag));
    TEST_ASSERT_EQ(ETH_NOT_INIT, automotive_phy_run_diagnostics(&diag));
    return 0;
}

static int test_run_diagnostics_null_should_invalid_param(void)
{
    test_eth_reset_mdio();
    automotive_phy_config_t cfg = make_config();

    TEST_ASSERT_EQ(ETH_OK, automotive_phy_init(&cfg));
    TEST_ASSERT_EQ(ETH_INVALID_PARAM, automotive_phy_run_diagnostics(NULL));

    automotive_phy_deinit();
    return 0;
}

static int test_run_diagnostics_unavailable_values(void)
{
    test_eth_reset_mdio();
    automotive_phy_config_t cfg = make_config();
    automotive_phy_diagnostics_t diag;

    TEST_ASSERT_EQ(ETH_OK, automotive_phy_init(&cfg));

    /* 预填非零，确认 run_diagnostics 会把整个结构写完整 */
    memset(&diag, 0xA5, sizeof(diag));
    TEST_ASSERT_EQ(ETH_OK, automotive_phy_run_diagnostics(&diag));

    /* MDIO 存根: 读回值等于写回值，init 期间写入的 CONTROL 寄存器会让
     * link 状态保持为 UP（AUTO_NEG 被置位） */
    TEST_ASSERT(diag.link_pass);
    TEST_ASSERT(diag.cable_ok);

    /* -1 表示不可用: 无符号质量指标必须映射为 0，而不是 0xFFFFFFFF */
    TEST_ASSERT_EQ(0U, diag.signal_quality);

    /* 长度字段本身是有符号的，-1 应原样保留 */
    TEST_ASSERT_EQ(-1, diag.cable_length_m);

    TEST_ASSERT_EQ(0U, diag.pair_swaps);
    TEST_ASSERT(!diag.polarity_inverted);

    automotive_phy_deinit();
    return 0;
}

/* 访问 mdio 存根写入的控制寄存器 (PHY_REG_CONTROL = 0x00) */
extern uint16_t g_mdio_regs[256];

#define PHY_REG_CONTROL_EXTERNAL   0x00U
#define PHY_CTRL_POWER_DOWN_EXTERNAL 0x0800U

static int test_enter_low_power_with_wake_on_link(void)
{
    test_eth_reset_mdio();
    automotive_phy_config_t cfg = make_config();

    TEST_ASSERT_EQ(ETH_OK, automotive_phy_init(&cfg));

    uint16_t before = g_mdio_regs[PHY_REG_CONTROL_EXTERNAL];

    /* wake_on_link=true: 保留链路检测 — 不应置 POWER_DOWN */
    TEST_ASSERT_EQ(ETH_OK, automotive_phy_enter_low_power(true));

    uint16_t after = g_mdio_regs[PHY_REG_CONTROL_EXTERNAL];
    TEST_ASSERT_EQ((uint16_t)(before & (uint16_t)~PHY_CTRL_POWER_DOWN_EXTERNAL),
                   after);

    automotive_phy_deinit();
    return 0;
}

static int test_enter_low_power_without_wake_on_link(void)
{
    test_eth_reset_mdio();
    automotive_phy_config_t cfg = make_config();

    TEST_ASSERT_EQ(ETH_OK, automotive_phy_init(&cfg));

    uint16_t before = g_mdio_regs[PHY_REG_CONTROL_EXTERNAL];

    /* wake_on_link=false: 彻底低功耗 — 必须置 POWER_DOWN */
    TEST_ASSERT_EQ(ETH_OK, automotive_phy_enter_low_power(false));

    uint16_t after = g_mdio_regs[PHY_REG_CONTROL_EXTERNAL];
    TEST_ASSERT(after & PHY_CTRL_POWER_DOWN_EXTERNAL);
    /* 其他位保持不变 */
    TEST_ASSERT_EQ((uint16_t)(before | PHY_CTRL_POWER_DOWN_EXTERNAL), after);

    automotive_phy_deinit();
    return 0;
}

static int test_enter_low_power_not_init(void)
{
    test_eth_reset_mdio();
    TEST_ASSERT_EQ(ETH_NOT_INIT, automotive_phy_enter_low_power(true));
    TEST_ASSERT_EQ(ETH_NOT_INIT, automotive_phy_enter_low_power(false));
    return 0;
}

typedef int (*test_fn_t)(void);

typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void)
{
    static const test_case_t cases[] = {
        { "before_init_ShouldNotInit", test_run_diagnostics_before_init_should_not_init },
        { "null_ptr_ShouldInvalidParam", test_run_diagnostics_null_should_invalid_param },
        { "unavailable_values", test_run_diagnostics_unavailable_values },
        { "lowPower_NotInit", test_enter_low_power_not_init },
        { "lowPower_WithWakeOnLink", test_enter_low_power_with_wake_on_link },
        { "lowPower_WithoutWakeOnLink", test_enter_low_power_without_wake_on_link },
    };

    printf("=== ethernet PHY diagnostics tests ===\n");

    for (size_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        tests_run++;
        printf("TEST: %s\n", cases[i].name);
        if (cases[i].fn() == 0) {
            tests_passed++;
            printf("  PASSED\n");
        } else {
            printf("  FAIL\n");
        }
    }

    printf("----------------------------------------\n");
    printf("Total: %d, Passed: %d, Failed: %d\n",
           tests_run, tests_passed, tests_run - tests_passed);

    return (tests_passed == tests_run) ? 0 : 1;
}
