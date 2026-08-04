/**
 * @file test_eth_sm.c
 * @brief EthSM (Ethernet State Manager) Unit Tests
 * @version 1.0
 * @date 2026-08-04
 *
 * @note Mini assert 框架 (仿 src/crypto_stack/tests/test_csm.c), 无 Unity 依赖
 * @note 状态机覆盖: UNINIT -> INIT -> WAIT_REQ -> READY -> WAIT_REQ
 * @note 已知模块缺陷 (不修改源码, 测试中规避):
 *   eth_sm_create_network() 将 eth_sm_network_config_t* 误传给期望
 *   eth_sm_config_t* 的 eth_sm_validate_config(): 后者读取偏移 8 处的
 *   network_count, 对应 network 配置中的 shutdown_delay_ms 低字节。
 *   因此 shutdown_delay_ms 低字节 > 4 (ETHSM_MAX_NETWORKS) 时创建网络
 *   会错误返回 ETHSM_E_PARAM_CONFIG。测试中 shutdown_delay_ms 取 <= 4。
 *
 * 已知模块缺陷 2 (错误码掩码): 所有 getter 类 API (get_state /
 *   get_network_mode / get_network_config / is_network_ready /
 *   get_link_state / has_wakeup / get_network_info) 的 else 分支无条件将
 *   result 覆盖为 ETHSM_E_PARAM_POINTER, 导致 NOT_INITIALIZED /
 *   INV_NETWORK_IDX 等错误码永远无法从这些 API 返回 (被掩码)。
 *   测试按当前实际行为断言 PARAM_POINTER, 并在断言处注明。
 *
 * 已知模块缺陷 3 (0xFF 广播注册死代码): 头文件声明 register_* 系列
 *   network_idx=0xFF 表示"所有网络", 但 eth_sm_validate_network_idx()
 *   先判断 network_idx >= ETHSM_MAX_NETWORKS, 0xFF 直接返回
 *   ETHSM_E_INV_NETWORK_IDX, 注册函数体内的 0xFF 分支不可达 (死代码),
 *   0xFF 注册既不生效也返回错误。测试按实际行为断言。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "eth_sm.h"

/* Test macros */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * Callback recording helpers
 * ============================================================================ */
static int g_state_cb_count = 0;
static uint8_t g_state_cb_net = 0u;
static eth_sm_state_t g_state_cb_old = ETHSM_STATE_UNINIT;
static eth_sm_state_t g_state_cb_new = ETHSM_STATE_UNINIT;

static int g_mode_cb_count = 0;
static eth_sm_mode_t g_mode_cb_mode = ETHSM_MODE_NO_COMM;

static int g_link_cb_count = 0;
static eth_sm_link_state_t g_link_cb_state = ETHSM_LINK_OFF;

static void test_state_cb(uint8_t net, eth_sm_state_t old_s, eth_sm_state_t new_s, void *ud)
{
    (void)ud;
    g_state_cb_count++;
    g_state_cb_net = net;
    g_state_cb_old = old_s;
    g_state_cb_new = new_s;
}

static void test_mode_cb(uint8_t net, eth_sm_mode_t mode, void *ud)
{
    (void)net;
    (void)ud;
    g_mode_cb_count++;
    g_mode_cb_mode = mode;
}

static void test_link_cb(uint8_t net, eth_sm_link_state_t ls, void *ud)
{
    (void)net;
    (void)ud;
    g_link_cb_count++;
    g_link_cb_state = ls;
}

static void reset_callbacks(void)
{
    g_state_cb_count = 0;
    g_mode_cb_count = 0;
    g_link_cb_count = 0;
}

/* BswM request callbacks */
static bool test_bswm_cb_accept(uint8_t net, eth_sm_mode_t mode)
{
    (void)net;
    (void)mode;
    return true;
}

static bool test_bswm_cb_reject(uint8_t net, eth_sm_mode_t mode)
{
    (void)net;
    (void)mode;
    return false;
}

/* ============================================================================
 * Test helpers
 * ============================================================================ */

/* 构造网络配置。startup_delay_ms 可任意; shutdown_delay_ms 必须 <= 4,
 * 否则触发 create_network -> validate_config 类型不匹配缺陷 (见文件头注释)。 */
static eth_sm_network_config_t make_net_cfg(bool start_all, uint32_t startup_ms, uint32_t shutdown_ms)
{
    eth_sm_network_config_t cfg;

    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.network_idx = 0u;
    cfg.eth_if_controller = 1u;
    cfg.start_all_channels = start_all;
    cfg.startup_delay_ms = startup_ms;
    cfg.shutdown_delay_ms = shutdown_ms;
    cfg.com_user_count = 0u;
    return cfg;
}

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

static int test_ethsm_init_deinit(void)
{
    uint8_t major = 0u, minor = 0u, patch = 0u;

    printf("  Testing EthSM init/deinit...\n");

    /* Fresh global state: not initialized */
    TEST_ASSERT(eth_sm_is_initialized() == false);

    /* Init with NULL config (default) */
    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);
    TEST_ASSERT(eth_sm_is_initialized() == true);

    /* Double init rejected */
    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_E_ALREADY_INITIALIZED);

    /* Version info */
    eth_sm_get_version(&major, &minor, &patch);
    TEST_ASSERT_EQ(major, ETHSM_MAJOR_VERSION);
    TEST_ASSERT_EQ(minor, ETHSM_MINOR_VERSION);
    TEST_ASSERT_EQ(patch, ETHSM_PATCH_VERSION);

    /* Deinit */
    eth_sm_deinit();
    TEST_ASSERT(eth_sm_is_initialized() == false);

    /* Re-init after deinit */
    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);
    eth_sm_deinit();

    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_validate_config(void)
{
    eth_sm_config_t cfg;

    printf("  Testing eth_sm_validate_config...\n");

    /* NULL -> pointer error */
    TEST_ASSERT_EQ(eth_sm_validate_config(NULL), ETHSM_E_PARAM_POINTER);

    /* Valid config (count <= max) */
    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.network_count = ETHSM_MAX_NETWORKS;
    TEST_ASSERT_EQ(eth_sm_validate_config(&cfg), ETHSM_OK);

    /* Over-limit count -> config error */
    cfg.network_count = (uint8_t)(ETHSM_MAX_NETWORKS + 1u);
    TEST_ASSERT_EQ(eth_sm_validate_config(&cfg), ETHSM_E_PARAM_CONFIG);

    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_create_delete_network(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_network_config_t cfg;
    eth_sm_network_config_t out;
    eth_sm_state_t st;

    printf("  Testing network create/delete...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* NULL config rejected */
    TEST_ASSERT_EQ(eth_sm_create_network(NULL, &idx), ETHSM_E_PARAM_POINTER);

    /* Create network (start_all=false): UNINIT -> INIT */
    cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(idx, 0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_INIT);

    /* Config roundtrip */
    TEST_ASSERT_EQ(eth_sm_get_network_config(idx, &out), ETHSM_OK);
    TEST_ASSERT_EQ(out.eth_if_controller, cfg.eth_if_controller);
    TEST_ASSERT(out.start_all_channels == cfg.start_all_channels);
    TEST_ASSERT_EQ(out.startup_delay_ms, cfg.startup_delay_ms);

    /* Delete: state back to UNINIT, slot freed */
    TEST_ASSERT_EQ(eth_sm_delete_network(idx), ETHSM_OK);
    /* 缺陷2: getter 掩码错误码, 实际返回 PARAM_POINTER 而非 INV_NETWORK_IDX */
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_E_PARAM_POINTER);

    /* Index reused */
    cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(idx, 0u);

    /* Double delete: second fails (slot gone) */
    TEST_ASSERT_EQ(eth_sm_delete_network(0u), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_delete_network(0u), ETHSM_E_INV_NETWORK_IDX);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_state_machine_uninit_init_ready(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_state_t st;
    eth_sm_mode_t mode;
    bool ready = false;
    eth_sm_network_info_t info;

    printf("  Testing UNINIT->INIT->READY transition (B3.1)...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* create with start_all=false: UNINIT -> INIT
     * (回调需在网络创建后才能注册, 故 UNINIT->INIT 转换通过
     *  trans_request_count==1 验证) */
    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(idx, 0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_INIT);
    TEST_ASSERT_EQ(eth_sm_get_network_info(idx, &info), ETHSM_OK);
    TEST_ASSERT_EQ(info.trans_request_count, 1u);  /* UNINIT->INIT */

    /* 注册状态/模式回调 (指定索引), 记录后续转换 */
    TEST_ASSERT_EQ(eth_sm_register_state_change(idx, test_state_cb, NULL), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_register_mode_indication(idx, test_mode_cb, NULL), ETHSM_OK);
    reset_callbacks();

    /* INIT -> WAIT_REQ (调试 API) */
    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_WAIT_REQ), ETHSM_OK);
    TEST_ASSERT_EQ(g_state_cb_count, 1);
    TEST_ASSERT_EQ(g_state_cb_old, ETHSM_STATE_INIT);
    TEST_ASSERT_EQ(g_state_cb_new, ETHSM_STATE_WAIT_REQ);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);

    /* WAIT_REQ -> READY: request + main function */
    reset_callbacks();
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_OK);
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_FULL_COMM);
    TEST_ASSERT_EQ(g_state_cb_count, 1);
    TEST_ASSERT_EQ(g_state_cb_new, ETHSM_STATE_READY);
    TEST_ASSERT_EQ(g_mode_cb_count, 1);
    TEST_ASSERT_EQ(g_mode_cb_mode, ETHSM_MODE_FULL_COMM);

    /* start_all_channels=true 的创建路径: 直接落在 WAIT_REQ */
    {
        uint8_t idx2 = 0xFFu;
        eth_sm_network_config_t cfg2 = make_net_cfg(true, 0u, 0u);
        TEST_ASSERT_EQ(eth_sm_create_network(&cfg2, &idx2), ETHSM_OK);
        TEST_ASSERT_EQ(idx2, 1u);
        TEST_ASSERT_EQ(eth_sm_get_state(idx2, &st), ETHSM_OK);
        TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);
        TEST_ASSERT_EQ(eth_sm_delete_network(idx2), ETHSM_OK);
    }

    /* Not ready without link */
    TEST_ASSERT_EQ(eth_sm_is_network_ready(idx, &ready), ETHSM_OK);
    TEST_ASSERT(ready == false);

    /* Link up -> network ready */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_ON), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_is_network_ready(idx, &ready), ETHSM_OK);
    TEST_ASSERT(ready == true);

    /* READY -> WAIT_REQ: release + main function */
    reset_callbacks();
    TEST_ASSERT_EQ(eth_sm_release_com_mode(idx), ETHSM_OK);
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_NO_COMM);
    TEST_ASSERT_EQ(g_state_cb_count, 1);
    TEST_ASSERT_EQ(g_state_cb_new, ETHSM_STATE_WAIT_REQ);
    TEST_ASSERT_EQ(g_mode_cb_count, 1);
    TEST_ASSERT_EQ(g_mode_cb_mode, ETHSM_MODE_NO_COMM);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_startup_shutdown_delays(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_state_t st;

    printf("  Testing startup/shutdown delay handling...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* startup_delay_ms=50; shutdown_delay_ms 取 4 (规避 validate 缺陷) */
    eth_sm_network_config_t cfg = make_net_cfg(true, 50u, 4u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);

    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_OK);

    /* 20ms < 50ms -> 仍在 WAIT_REQ */
    eth_sm_main_function(20u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);

    /* 剩余 30ms 耗尽 -> READY */
    eth_sm_main_function(30u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);

    /* shutdown_delay_ms=4: 2ms 后仍 READY */
    TEST_ASSERT_EQ(eth_sm_release_com_mode(idx), ETHSM_OK);
    eth_sm_main_function(2u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);

    /* 再 2ms -> WAIT_REQ */
    eth_sm_main_function(2u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_request_release_errors(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_state_t st;

    printf("  Testing invalid request/release...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* 网络处于 INIT: request/release 均不允许 */
    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_INIT);
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_E_INV_MODE);
    TEST_ASSERT_EQ(eth_sm_release_com_mode(idx), ETHSM_E_INV_MODE);

    /* 调试 API 强制到 WAIT_REQ */
    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_WAIT_REQ), ETHSM_OK);

    /* WAIT_REQ 下 release 不允许, request 允许 (可重复) */
    TEST_ASSERT_EQ(eth_sm_release_com_mode(idx), ETHSM_E_INV_MODE);
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_OK);

    /* READY 下 request 不允许 */
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx), ETHSM_E_INV_MODE);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_mode_switch(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_mode_t mode;

    printf("  Testing mode switching...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_register_mode_indication(idx, test_mode_cb, NULL), ETHSM_OK);
    reset_callbacks();

    /* 默认模式 NO_COMM */
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_NO_COMM);

    /* 切到 FULL_COMM: 回调触发一次 */
    TEST_ASSERT_EQ(eth_sm_set_network_mode(idx, ETHSM_MODE_FULL_COMM), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_FULL_COMM);
    TEST_ASSERT_EQ(g_mode_cb_count, 1);
    TEST_ASSERT_EQ(g_mode_cb_mode, ETHSM_MODE_FULL_COMM);

    /* 相同模式: 不触发回调 */
    TEST_ASSERT_EQ(eth_sm_set_network_mode(idx, ETHSM_MODE_FULL_COMM), ETHSM_OK);
    TEST_ASSERT_EQ(g_mode_cb_count, 1);

    /* 切到 ACTIVE */
    TEST_ASSERT_EQ(eth_sm_set_network_mode(idx, ETHSM_MODE_ACTIVE), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_ACTIVE);

    /* BswM 查询接口与当前模式一致 */
    TEST_ASSERT_EQ(eth_sm_bswm_get_current_mode(idx, &mode), ETHSM_OK);
    TEST_ASSERT_EQ(mode, ETHSM_MODE_ACTIVE);

    /* NULL 输出指针 */
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, NULL), ETHSM_E_PARAM_POINTER);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_link_state(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_link_state_t ls;
    bool ready = false;

    printf("  Testing link state handling...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_register_link_change(idx, test_link_cb, NULL), ETHSM_OK);
    reset_callbacks();

    /* 默认 LINK_OFF */
    TEST_ASSERT_EQ(eth_sm_get_link_state(idx, &ls), ETHSM_OK);
    TEST_ASSERT_EQ(ls, ETHSM_LINK_OFF);

    /* 链路 up: 回调触发一次 */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_ON), ETHSM_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 1);
    TEST_ASSERT_EQ(g_link_cb_state, ETHSM_LINK_ON);
    TEST_ASSERT_EQ(eth_sm_get_link_state(idx, &ls), ETHSM_OK);
    TEST_ASSERT_EQ(ls, ETHSM_LINK_ON);

    /* 相同状态: 不触发回调 */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_ON), ETHSM_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 1);

    /* 链路 down 再触发 */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_OFF), ETHSM_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 2);

    /* ready 标志需要 READY 状态 + LINK_ON */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_ON), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_READY), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_is_network_ready(idx, &ready), ETHSM_OK);
    TEST_ASSERT(ready == true);

    /* 链路已 up 时 wait_for_link 立即成功 */
    TEST_ASSERT_EQ(eth_sm_wait_for_link(idx, 100u), ETHSM_OK);

    /* 链路 down + 零超时 -> 失败 */
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_OFF), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_wait_for_link(idx, 0u), ETHSM_E_NOT_OK);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_callback_registration(void)
{
    uint8_t idx = 0xFFu;

    printf("  Testing callback registration...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);

    /* 注册到指定网络 */
    TEST_ASSERT_EQ(eth_sm_register_state_change(idx, test_state_cb, NULL), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_register_mode_indication(idx, test_mode_cb, NULL), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_register_link_change(idx, test_link_cb, NULL), ETHSM_OK);
    reset_callbacks();

    /* 状态变化触发回调 */
    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_WAIT_REQ), ETHSM_OK);
    TEST_ASSERT_EQ(g_state_cb_count, 1);
    TEST_ASSERT_EQ(g_state_cb_old, ETHSM_STATE_INIT);
    TEST_ASSERT_EQ(g_state_cb_new, ETHSM_STATE_WAIT_REQ);

    TEST_ASSERT_EQ(eth_sm_set_network_mode(idx, ETHSM_MODE_FULL_COMM), ETHSM_OK);
    TEST_ASSERT_EQ(g_mode_cb_count, 1);

    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_ON), ETHSM_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 1);

    /* 注销后不再触发 */
    TEST_ASSERT_EQ(eth_sm_unregister_state_change(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_unregister_mode_indication(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_unregister_link_change(idx), ETHSM_OK);
    reset_callbacks();

    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_READY), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_set_network_mode(idx, ETHSM_MODE_ACTIVE), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_link_state_change(idx, ETHSM_LINK_OFF), ETHSM_OK);
    TEST_ASSERT_EQ(g_state_cb_count, 0);
    TEST_ASSERT_EQ(g_mode_cb_count, 0);
    TEST_ASSERT_EQ(g_link_cb_count, 0);

    /* 缺陷3: 0xFF 广播注册为死代码, 返回 INV_NETWORK_IDX 且不生效 */
    reset_callbacks();
    TEST_ASSERT_EQ(eth_sm_register_state_change(0xFFu, test_state_cb, NULL), ETHSM_E_INV_NETWORK_IDX);
    TEST_ASSERT_EQ(eth_sm_force_state(idx, ETHSM_STATE_WAIT_REQ), ETHSM_OK);
    TEST_ASSERT_EQ(g_state_cb_count, 0);  /* 注册未生效, 无回调 */

    /* 0xFF 注销同样返回 INV_NETWORK_IDX */
    TEST_ASSERT_EQ(eth_sm_unregister_state_change(0xFFu), ETHSM_E_INV_NETWORK_IDX);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_bswm_integration(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_state_t st;

    printf("  Testing BswM integration...\n");

    /* 未初始化时注册 -> NOT_INITIALIZED */
    TEST_ASSERT_EQ(eth_sm_register_bswm_request(test_bswm_cb_accept, NULL), ETHSM_E_NOT_INITIALIZED);

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(true, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);

    /* 未注册回调: 请求被接受, FULL_COMM -> READY */
    TEST_ASSERT_EQ(eth_sm_bswm_request_mode(idx, ETHSM_MODE_FULL_COMM), ETHSM_OK);
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);

    /* 拒绝回调: 请求被拒 -> NOT_OK, 状态不变 */
    TEST_ASSERT_EQ(eth_sm_register_bswm_request(test_bswm_cb_reject, NULL), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_bswm_request_mode(idx, ETHSM_MODE_NO_COMM), ETHSM_E_NOT_OK);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);

    /* 接受回调: NO_COMM -> WAIT_REQ */
    TEST_ASSERT_EQ(eth_sm_register_bswm_request(test_bswm_cb_accept, NULL), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_bswm_request_mode(idx, ETHSM_MODE_NO_COMM), ETHSM_OK);
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_WAIT_REQ);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_ecum_wakeup(void)
{
    uint8_t idx = 0xFFu;
    bool has_wakeup = false;
    eth_sm_network_info_t info;

    printf("  Testing EcuM notify & wakeup...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);

    /* 唤醒置位/清除 */
    TEST_ASSERT_EQ(eth_sm_has_wakeup(idx, &has_wakeup), ETHSM_OK);
    TEST_ASSERT(has_wakeup == false);

    TEST_ASSERT_EQ(eth_sm_set_wakeup(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_has_wakeup(idx, &has_wakeup), ETHSM_OK);
    TEST_ASSERT(has_wakeup == true);

    TEST_ASSERT_EQ(eth_sm_clear_wakeup(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_has_wakeup(idx, &has_wakeup), ETHSM_OK);
    TEST_ASSERT(has_wakeup == false);

    /* EcuM 通讯通知反映到网络信息 */
    eth_sm_ecum_notify(idx, true);
    TEST_ASSERT_EQ(eth_sm_get_network_info(idx, &info), ETHSM_OK);
    TEST_ASSERT_EQ(info.comm_state, ETHSM_COMM_REQUESTED);

    eth_sm_ecum_notify(idx, false);
    TEST_ASSERT_EQ(eth_sm_get_network_info(idx, &info), ETHSM_OK);
    TEST_ASSERT_EQ(info.comm_state, ETHSM_COMM_RELEASED);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_diagnostics(void)
{
    uint8_t idx0 = 0xFFu, idx1 = 0xFFu;
    eth_sm_network_info_t info;
    eth_sm_transition_info_t trans;
    eth_sm_state_t st;

    printf("  Testing diagnostics & counters...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* 计数初始值 */
    TEST_ASSERT_EQ(eth_sm_get_network_count(), ETHSM_MAX_NETWORKS);
    TEST_ASSERT_EQ(eth_sm_get_active_network_count(), 0u);
    TEST_ASSERT_EQ(eth_sm_get_ready_network_count(), 0u);

    eth_sm_network_config_t cfg = make_net_cfg(true, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx0), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx1), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_active_network_count(), 2u);
    TEST_ASSERT_EQ(eth_sm_get_ready_network_count(), 0u);

    /* 一个网络进入 READY */
    TEST_ASSERT_EQ(eth_sm_request_com_mode(idx0), ETHSM_OK);
    eth_sm_main_function(0u);
    TEST_ASSERT_EQ(eth_sm_get_ready_network_count(), 1u);

    /* 网络信息结构体 */
    TEST_ASSERT_EQ(eth_sm_get_network_info(idx0, &info), ETHSM_OK);
    TEST_ASSERT_EQ(info.network_idx, idx0);
    TEST_ASSERT_EQ(info.sm_state, ETHSM_STATE_READY);
    TEST_ASSERT_EQ(info.current_mode, ETHSM_MODE_FULL_COMM);
    TEST_ASSERT(info.trans_request_count >= 3u);  /* UNINIT->INIT->WAIT_REQ->READY */

    /* 最后转换信息 (简化实现, 仅校验索引) */
    TEST_ASSERT_EQ(eth_sm_get_last_transition(idx0, &trans), ETHSM_OK);

    /* 调试用 force_state */
    TEST_ASSERT_EQ(eth_sm_force_state(idx1, ETHSM_STATE_READY), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_state(idx1, &st), ETHSM_OK);
    TEST_ASSERT_EQ(st, ETHSM_STATE_READY);
    TEST_ASSERT_EQ(eth_sm_get_ready_network_count(), 2u);

    /* 调试打印不崩溃 */
    eth_sm_print_debug_info(idx0);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_error_handling(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_state_t st;
    eth_sm_network_config_t cfg;

    printf("  Testing error handling...\n");

    /* 未初始化:
     * - 直接返回 result 的 API (link_state_change) 返回 NOT_INITIALIZED
     * - getter 类 API 受缺陷2影响, 错误码被掩码为 PARAM_POINTER */
    TEST_ASSERT_EQ(eth_sm_link_state_change(0u, ETHSM_LINK_ON), ETHSM_E_NOT_INITIALIZED);
    TEST_ASSERT_EQ(eth_sm_get_state(0u, &st), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(0u, NULL), ETHSM_E_PARAM_POINTER);

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    /* 无效索引 (>= MAX): set_network_mode 直接返回校验结果 */
    TEST_ASSERT_EQ(eth_sm_set_network_mode(ETHSM_MAX_NETWORKS, ETHSM_MODE_FULL_COMM), ETHSM_E_INV_NETWORK_IDX);
    /* 缺陷2: getter 掩码为 PARAM_POINTER */
    TEST_ASSERT_EQ(eth_sm_get_state(ETHSM_MAX_NETWORKS, &st), ETHSM_E_PARAM_POINTER);

    /* 未创建的网络索引: 同样被掩码为 PARAM_POINTER (缺陷2) */
    TEST_ASSERT_EQ(eth_sm_get_state(2u, &st), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_state(0u, NULL), ETHSM_E_PARAM_POINTER);

    /* NULL 输出指针 */
    cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(idx, 0u);
    TEST_ASSERT_EQ(eth_sm_get_state(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_network_mode(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_link_state(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_is_network_ready(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_has_wakeup(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_network_config(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_get_network_info(idx, NULL), ETHSM_E_PARAM_POINTER);
    TEST_ASSERT_EQ(eth_sm_create_network(NULL, &idx), ETHSM_E_PARAM_POINTER);

    /* 资源耗尽: 填满全部槽位 */
    {
        uint8_t i;
        for (i = 1u; i < ETHSM_MAX_NETWORKS; i++)
        {
            TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
            TEST_ASSERT_EQ(idx, i);
        }
    }
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_E_NO_RESOURCE);

    /* 释放一个槽位后可再次创建 */
    TEST_ASSERT_EQ(eth_sm_delete_network(0u), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);
    TEST_ASSERT_EQ(idx, 0u);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

static int test_ethsm_misc_apis(void)
{
    uint8_t idx = 0xFFu;
    eth_sm_transition_info_t trans;

    printf("  Testing misc APIs...\n");

    TEST_ASSERT_EQ(eth_sm_init(NULL), ETHSM_OK);

    eth_sm_network_config_t cfg = make_net_cfg(false, 0u, 0u);
    TEST_ASSERT_EQ(eth_sm_create_network(&cfg, &idx), ETHSM_OK);

    /* 空操作 API 不崩溃 */
    eth_sm_process_mode_request(idx);
    eth_sm_main_function(100u);
    eth_sm_print_debug_info(idx);

    /* check_transitions 校验索引 */
    TEST_ASSERT_EQ(eth_sm_check_transitions(idx), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_check_transitions(ETHSM_MAX_NETWORKS), ETHSM_E_INV_NETWORK_IDX);

    /* get_last_transition 校验索引 */
    TEST_ASSERT_EQ(eth_sm_get_last_transition(idx, &trans), ETHSM_OK);
    TEST_ASSERT_EQ(eth_sm_get_last_transition(ETHSM_MAX_NETWORKS, &trans), ETHSM_E_INV_NETWORK_IDX);

    eth_sm_deinit();
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;

    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

int main(void)
{
    printf("============================================\n");
    printf("EthSM (Ethernet State Manager) Unit Tests\n");
    printf("============================================\n\n");

    run_test(test_ethsm_init_deinit, "EthSM Init/Deinit");
    run_test(test_ethsm_validate_config, "EthSM Validate Config");
    run_test(test_ethsm_create_delete_network, "EthSM Network Create/Delete");
    run_test(test_ethsm_state_machine_uninit_init_ready, "EthSM UNINIT->INIT->READY");
    run_test(test_ethsm_startup_shutdown_delays, "EthSM Startup/Shutdown Delays");
    run_test(test_ethsm_request_release_errors, "EthSM Request/Release Errors");
    run_test(test_ethsm_mode_switch, "EthSM Mode Switch");
    run_test(test_ethsm_link_state, "EthSM Link State");
    run_test(test_ethsm_callback_registration, "EthSM Callback Registration");
    run_test(test_ethsm_bswm_integration, "EthSM BswM Integration");
    run_test(test_ethsm_ecum_wakeup, "EthSM EcuM Notify & Wakeup");
    run_test(test_ethsm_diagnostics, "EthSM Diagnostics & Counters");
    run_test(test_ethsm_error_handling, "EthSM Error Handling");
    run_test(test_ethsm_misc_apis, "EthSM Misc APIs");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
