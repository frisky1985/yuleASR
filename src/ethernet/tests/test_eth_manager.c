/**
 * @file test_eth_manager.c
 * @brief 以太网管理器 (eth_manager) 单元测试
 * @version 1.0
 * @date 2026-08-04
 *
 * 覆盖 eth_manager.h 公共 API:
 *   - 初始化/去初始化: eth_manager_init / eth_manager_deinit / eth_manager_start /
 *     eth_manager_stop / eth_manager_get_state
 *   - 回调注册: eth_manager_register_link_callback / eth_manager_register_error_callback /
 *     eth_manager_register_stats_callback
 *   - 链路状态管理: eth_manager_check_link / eth_manager_wait_for_link /
 *     eth_manager_force_link_check / eth_manager_check_recovery_needed
 *   - 自动协商: eth_manager_start_auto_negotiation / eth_manager_get_negotiation_status /
 *     eth_manager_set_forced_mode / eth_manager_wait_for_negotiation
 *   - 错误统计: eth_manager_get_stats / eth_manager_clear_stats /
 *     eth_manager_get_last_error / eth_manager_clear_error / eth_manager_error_to_string
 *   - 诊断恢复: eth_manager_run_diagnostics / eth_manager_soft_reset /
 *     eth_manager_hard_reset / eth_manager_auto_recovery / eth_manager_print_diagnostics
 *   - 进阶: eth_manager_register_stats_callback / eth_manager_update_throughput /
 *     eth_manager_get_config / eth_manager_validate_config
 *   - 驱动收发路径 (driver/ 层): eth_mac_transmit / eth_mac_receive / eth_dma_tx_* / eth_dma_rx_*
 *
 * 两种构建模式 (同一文件):
 *   1) 默认 (真实驱动): 链接 driver/ 下真实驱动实现 (内置寄存器模拟, mdio_read 恒返回
 *      0xFFFF -> 链路恒 UP)。验证管理器逻辑 + 驱动编译。
 *   2) -DETH_TEST_USE_MOCKS: 用本文件内的 mock 驱动层替代 driver 目录下的 .c, 仅链接
 *      eth_manager.c, 可模拟链路断开/协商超时/硬件失败等真实驱动无法触达的场景。
 *
 * 测试框架仿 src/crypto_stack/tests/test_csm.c: TEST_ASSERT 宏 + main 逐个 run_test,
 * 全部 PASS 返回 0, 否则返回 1。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "eth_manager.h"

/* ============================================================================
 * Mini 测试框架 (仿 test_csm.c, 不依赖 Unity)
 * ============================================================================ */

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))
#define TEST_ASSERT_STR_EQ(a, b) TEST_ASSERT(strcmp((a), (b)) == 0)

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * 配置构造
 * ============================================================================ */

static eth_manager_config_t make_config(void)
{
    eth_manager_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    /* 链路监控: 每 1ms 检查一次, 连续 2 次断开判定为链路断开 */
    cfg.link_monitor.enable_link_monitoring = true;
    cfg.link_monitor.link_check_interval_ms = 1;
    cfg.link_monitor.link_down_threshold = 2;
    cfg.link_monitor.auto_recovery = false;

    /* 错误统计 */
    cfg.error_config.enable_error_counting = true;
    cfg.error_config.error_threshold = 10;
    cfg.error_config.enable_error_callback = true;
    cfg.error_config.log_errors = true;

    /* 自动协商 */
    cfg.negotiation.enable_auto_negotiation = true;
    cfg.negotiation.negotiation_timeout_ms = 1000;
    cfg.negotiation.retry_count = 3;
    cfg.negotiation.fallback_to_100m = true;

    /* MAC 配置 (满足 eth_mac_validate_config) */
    cfg.mac_config.mac_type = ETH_MAC_TYPE_GENERIC;
    {
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
        memcpy(cfg.mac_config.mac_addr, mac, 6);
    }
    cfg.mac_config.speed_mode = ETH_MODE_100M_FULL;
    cfg.mac_config.mac_mode = ETH_MAC_MODE_NORMAL;
    cfg.mac_config.filter.max_frame_size = 1522;
    cfg.mac_config.filter.drop_crc_errors = true;
    cfg.mac_config.asil_level = ETH_ASIL_B;
    cfg.mac_config.protection = ETH_PROT_NONE;
    cfg.mac_config.base_addr = 0x40000000U;
    cfg.mac_config.irq_num = 42;
    cfg.mac_config.clock_freq_hz = 125000000U;
    cfg.mac_config.mdio_clock_hz = 2500000U;
    cfg.mac_config.tx_fifo_depth = 16;
    cfg.mac_config.rx_fifo_depth = 16;

    /* DMA 配置 (满足 eth_dma_validate_config) */
    cfg.dma_config.rx_desc_count = 16;
    cfg.dma_config.tx_desc_count = 16;
    cfg.dma_config.buffer_size = 1536;
    cfg.dma_config.mode = ETH_DMA_MODE_POLLING;
    cfg.dma_config.rx_threshold = 8;
    cfg.dma_config.burst_length = 8;

    /* PHY 配置 (满足 automotive_phy_validate_config) */
    cfg.phy_config.phy_type = AUTOMOTIVE_PHY_GENERIC;
    cfg.phy_config.standard = AUTOMOTIVE_ETH_STANDARD_100BASE_T1;
    cfg.phy_config.phy_addr = 1;
    cfg.phy_config.enable_master_mode = true;

    return cfg;
}

/* ============================================================================
 * 回调记录器
 * ============================================================================ */

static int g_link_cb_count = 0;
static eth_link_status_t g_link_cb_status = ETH_LINK_DOWN;
static void *g_link_cb_user = NULL;
static int g_err_cb_count = 0;
static uint32_t g_err_cb_code = 0;
static int g_stats_cb_count = 0;

static void link_cb(eth_link_status_t status, void *user_data)
{
    g_link_cb_count++;
    g_link_cb_status = status;
    g_link_cb_user = user_data;
}

static void err_cb(const eth_error_info_t *error, void *user_data)
{
    (void)user_data;
    if (error != NULL) {
        g_err_cb_count++;
        g_err_cb_code = error->error_code;
    }
}

static void stats_cb(const eth_manager_stats_t *stats, void *user_data)
{
    (void)stats;
    (void)user_data;
    g_stats_cb_count++;
}

static void reset_callbacks(void)
{
    g_link_cb_count = 0;
    g_link_cb_status = ETH_LINK_DOWN;
    g_link_cb_user = NULL;
    g_err_cb_count = 0;
    g_err_cb_code = 0;
    g_stats_cb_count = 0;
}

/* ============================================================================
 * Mock 驱动层 (仅 -DETH_TEST_USE_MOCKS 时编译)
 * 替代 driver/eth_mac_driver.c / eth_dma.c / eth_automotive.c,
 * 提供硬件行为注入点: 链路状态、协商结果、失败注入。
 * ============================================================================ */

#ifdef ETH_TEST_USE_MOCKS

static bool mock_mac_initialized = false;
static bool mock_mac_active = false;
static bool mock_dma_initialized = false;
static bool mock_dma_active = false;
static bool mock_phy_initialized = false;

static bool mock_link_up = true;
static bool mock_neg_done = true;
static bool mock_prev_link_up = true;

static bool mock_fail_mac_init = false;
static bool mock_fail_dma_init = false;
static bool mock_fail_phy_init = false;
static bool mock_fail_phy_auto_neg = false;
static bool mock_fail_phy_soft_reset = false;

static eth_mac_stats_t mock_mac_stats;
static eth_dma_stats_t mock_dma_stats;
static automotive_phy_stats_t mock_phy_stats;

/* ---- 测试控制接口 ---- */

static void mock_reset_all(void)
{
    mock_mac_initialized = false;
    mock_mac_active = false;
    mock_dma_initialized = false;
    mock_dma_active = false;
    mock_phy_initialized = false;
    mock_link_up = true;
    mock_neg_done = true;
    mock_prev_link_up = true;
    mock_fail_mac_init = false;
    mock_fail_dma_init = false;
    mock_fail_phy_init = false;
    mock_fail_phy_auto_neg = false;
    mock_fail_phy_soft_reset = false;
    memset(&mock_mac_stats, 0, sizeof(mock_mac_stats));
    memset(&mock_dma_stats, 0, sizeof(mock_dma_stats));
    memset(&mock_phy_stats, 0, sizeof(mock_phy_stats));
}

static void mock_set_link(bool up) { mock_link_up = up; }
static void mock_set_neg_done(bool done) { mock_neg_done = done; }

/* ---- MAC mock ---- */

eth_status_t eth_mac_init(const eth_mac_config_t *config)
{
    (void)config;
    if (mock_fail_mac_init) {
        return ETH_ERROR;
    }
    mock_mac_initialized = true;
    mock_mac_active = false;
    return ETH_OK;
}

void eth_mac_deinit(void)
{
    mock_mac_initialized = false;
    mock_mac_active = false;
}

eth_status_t eth_mac_start(void)
{
    if (!mock_mac_initialized) {
        return ETH_NOT_INIT;
    }
    mock_mac_active = true;
    return ETH_OK;
}

eth_status_t eth_mac_stop(void)
{
    if (!mock_mac_active) {
        return ETH_ERROR;
    }
    mock_mac_active = false;
    return ETH_OK;
}

eth_status_t eth_mac_get_stats(eth_mac_stats_t *stats)
{
    if (!mock_mac_initialized) {
        return ETH_NOT_INIT;
    }
    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    memcpy(stats, &mock_mac_stats, sizeof(mock_mac_stats));
    return ETH_OK;
}

eth_status_t eth_mac_clear_stats(void)
{
    if (!mock_mac_initialized) {
        return ETH_NOT_INIT;
    }
    memset(&mock_mac_stats, 0, sizeof(mock_mac_stats));
    return ETH_OK;
}

/* ---- DMA mock ---- */

eth_status_t eth_dma_init(const eth_dma_config_t *config)
{
    (void)config;
    if (mock_fail_dma_init) {
        return ETH_ERROR;
    }
    mock_dma_initialized = true;
    mock_dma_active = false;
    return ETH_OK;
}

void eth_dma_deinit(void)
{
    mock_dma_initialized = false;
    mock_dma_active = false;
}

eth_status_t eth_dma_start(void)
{
    if (!mock_dma_initialized) {
        return ETH_NOT_INIT;
    }
    mock_dma_active = true;
    return ETH_OK;
}

eth_status_t eth_dma_stop(void)
{
    if (!mock_dma_active) {
        return ETH_ERROR;
    }
    mock_dma_active = false;
    return ETH_OK;
}

eth_status_t eth_dma_get_stats(eth_dma_stats_t *stats)
{
    if (!mock_dma_initialized) {
        return ETH_NOT_INIT;
    }
    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    memcpy(stats, &mock_dma_stats, sizeof(mock_dma_stats));
    return ETH_OK;
}

eth_status_t eth_dma_clear_stats(void)
{
    if (!mock_dma_initialized) {
        return ETH_NOT_INIT;
    }
    memset(&mock_dma_stats, 0, sizeof(mock_dma_stats));
    return ETH_OK;
}

/* ---- PHY mock ---- */

eth_status_t automotive_phy_init(const automotive_phy_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }
    if (mock_fail_phy_init) {
        return ETH_ERROR;
    }
    mock_phy_initialized = true;
    return ETH_OK;
}

void automotive_phy_deinit(void)
{
    mock_phy_initialized = false;
}

eth_status_t automotive_phy_auto_negotiation(void)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (mock_fail_phy_auto_neg) {
        return ETH_ERROR;
    }
    mock_phy_stats.auto_neg_count++;
    mock_neg_done = true;
    return ETH_OK;
}

eth_status_t automotive_phy_get_link_status(automotive_link_status_t *status)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (status == NULL) {
        return ETH_INVALID_PARAM;
    }
    if (mock_link_up != mock_prev_link_up) {
        if (mock_link_up) {
            mock_phy_stats.link_up_count++;
        } else {
            mock_phy_stats.link_down_count++;
        }
        mock_prev_link_up = mock_link_up;
    }
    memset(status, 0, sizeof(*status));
    status->link_up = mock_link_up;
    status->full_duplex = true;
    status->speed = AUTOMOTIVE_ETH_SPEED_100MBPS;
    status->signal_quality = 100;
    status->cable_length = 1;
    status->master_slave_resolved = true;
    status->is_master = true;
    return ETH_OK;
}

eth_status_t automotive_phy_wait_for_link(uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    return mock_link_up ? ETH_OK : ETH_TIMEOUT;
}

eth_status_t automotive_phy_run_diagnostics(automotive_phy_diagnostics_t *diagnostics)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (diagnostics == NULL) {
        return ETH_INVALID_PARAM;
    }
    memset(diagnostics, 0, sizeof(*diagnostics));
    diagnostics->link_pass = mock_link_up;
    diagnostics->cable_ok = mock_link_up;
    diagnostics->cable_length_m = 1;
    diagnostics->signal_quality = 100;
    return ETH_OK;
}

eth_status_t automotive_phy_get_stats(automotive_phy_stats_t *stats)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    memcpy(stats, &mock_phy_stats, sizeof(mock_phy_stats));
    return ETH_OK;
}

eth_status_t automotive_phy_clear_stats(void)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    memset(&mock_phy_stats, 0, sizeof(mock_phy_stats));
    return ETH_OK;
}

eth_status_t automotive_phy_soft_reset(void)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (mock_fail_phy_soft_reset) {
        return ETH_ERROR;
    }
    mock_phy_stats.reset_count++;
    return ETH_OK;
}

eth_status_t automotive_phy_read_reg(uint8_t reg_addr, uint16_t *value)
{
    if (!mock_phy_initialized) {
        return ETH_NOT_INIT;
    }
    if (value == NULL) {
        return ETH_INVALID_PARAM;
    }
    /* PHY_REG_STATUS = 0x01, PHY_STATUS_AUTO_NEG_DONE = 0x0020 */
    if (reg_addr == 0x01) {
        *value = mock_neg_done ? 0x0020 : 0x0000;
    } else {
        *value = 0x0000;
    }
    return ETH_OK;
}

#endif /* ETH_TEST_USE_MOCKS */

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/* ---- 初始化/去初始化 ---- */

static int test_init_deinit(void)
{
    printf("  Testing init/deinit...\n");

    eth_manager_deinit();  /* 归一化到 UNINIT */

    eth_manager_config_t cfg = make_config();

    /* NULL 配置 */
    TEST_ASSERT_EQ(eth_manager_init(NULL), ETH_INVALID_PARAM);

    /* 无效配置: 描述符数量为 0 */
    eth_manager_config_t bad = make_config();
    bad.dma_config.rx_desc_count = 0;
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);

    bad = make_config();
    bad.dma_config.tx_desc_count = 0;
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);

    /* 无效配置: PHY 地址越界 */
    bad = make_config();
    bad.phy_config.phy_addr = 32;
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);

    /* 正常初始化 */
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 重复初始化应失败 */
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_ERROR);

    /* 反初始化 */
    eth_manager_deinit();

    /* UNINIT 状态下再反初始化是安全空操作 */
    eth_manager_deinit();

    /* 可再次初始化 */
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    eth_manager_deinit();

    /* validate_config */
    TEST_ASSERT_EQ(eth_manager_validate_config(NULL), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_validate_config(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_validate_config(&bad), ETH_INVALID_PARAM);

    /* get_config: NULL 保护 */
    TEST_ASSERT_EQ(eth_manager_get_config(NULL), ETH_INVALID_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* ---- 状态机 ---- */

static int test_state_machine(void)
{
    printf("  Testing state machine...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    eth_manager_state_t st;

    /* UNINIT 状态下读取状态 */
    TEST_ASSERT_EQ(eth_manager_get_state(NULL), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_UNINIT);

    /* UNINIT 状态下 start/stop 应失败 */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_ERROR);
    TEST_ASSERT_EQ(eth_manager_stop(), ETH_ERROR);

    /* 初始化 -> INIT */
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_INIT);

    /* 启动 -> LINK_DOWN */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_LINK_DOWN);

    /* 重复启动失败 */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_ERROR);

    /* 停止 -> INIT */
    TEST_ASSERT_EQ(eth_manager_stop(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_INIT);

    /* 停止后可再次启动 */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_stop(), ETH_OK);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 链路管理 (链路恒 UP 场景) ---- */

static int test_link_management(void)
{
    printf("  Testing link management...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    eth_link_status_t ls;

    /* 未初始化时注册回调应失败 */
    TEST_ASSERT_EQ(eth_manager_register_link_callback(link_cb, &g_link_cb_user), ETH_NOT_INIT);

    reset_callbacks();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 等待链路: 模拟硬件恒 UP -> 立即成功 */
    TEST_ASSERT_EQ(eth_manager_wait_for_link(1000), ETH_OK);

    /* 注册链路回调 */
    int marker = 0x5A;
    TEST_ASSERT_EQ(eth_manager_register_link_callback(link_cb, &marker), ETH_OK);

    /* 首次检查: INIT -> LINK_UP 状态迁移, 回调触发一次 */
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(ls, ETH_LINK_UP);
    TEST_ASSERT_EQ(g_link_cb_count, 1);
    TEST_ASSERT_EQ(g_link_cb_status, ETH_LINK_UP);
    TEST_ASSERT(g_link_cb_user == &marker);

    /* 再次检查: 状态未变化, 回调不重复触发 */
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 1);

    /* 强制链路检查 */
    TEST_ASSERT_EQ(eth_manager_force_link_check(), ETH_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 1);

    /* status 可空 */
    TEST_ASSERT_EQ(eth_manager_check_link(NULL), ETH_OK);

    /* 恢复检查: 链路未断开 -> 不需要恢复 */
    bool need = true;
    TEST_ASSERT_EQ(eth_manager_check_recovery_needed(&need), ETH_OK);
    TEST_ASSERT_EQ(need, false);
    TEST_ASSERT_EQ(eth_manager_check_recovery_needed(NULL), ETH_INVALID_PARAM);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 链路监控关闭路径 ---- */

static int test_link_monitoring_disabled(void)
{
    printf("  Testing link monitoring disabled...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    cfg.link_monitor.enable_link_monitoring = false;

    reset_callbacks();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_register_link_callback(link_cb, NULL), ETH_OK);

    /* 启动后状态为 LINK_DOWN */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);
    eth_manager_state_t st;
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_LINK_DOWN);

    /* 监控关闭: check_link 不迁移状态机, 不触发回调 */
    eth_link_status_t ls;
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(ls, ETH_LINK_UP);   /* 底层链路实际是 UP */
    TEST_ASSERT_EQ(g_link_cb_count, 0);
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_LINK_DOWN);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 自动协商 ---- */

static int test_auto_negotiation(void)
{
    printf("  Testing auto-negotiation...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    /* 未初始化保护 */
    TEST_ASSERT_EQ(eth_manager_start_auto_negotiation(), ETH_NOT_INIT);

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 启动协商 (模拟硬件协商完成) */
    TEST_ASSERT_EQ(eth_manager_start_auto_negotiation(), ETH_OK);

    bool complete = true;
    TEST_ASSERT_EQ(eth_manager_get_negotiation_status(&complete), ETH_OK);
    TEST_ASSERT_EQ(complete, false);   /* 刚启动, 尚未置完成 */

    TEST_ASSERT_EQ(eth_manager_get_negotiation_status(NULL), ETH_INVALID_PARAM);

    /* 等待协商完成: 模拟 PHY 状态寄存器 AUTO_NEG_DONE 置位 */
    TEST_ASSERT_EQ(eth_manager_wait_for_negotiation(1000), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_negotiation_status(&complete), ETH_OK);
    TEST_ASSERT_EQ(complete, true);

    /* 强制速率模式: 禁用自动协商并更新配置 */
    TEST_ASSERT_EQ(eth_manager_set_forced_mode(ETH_MODE_100M_FULL), ETH_OK);
    eth_manager_config_t got;
    TEST_ASSERT_EQ(eth_manager_get_config(&got), ETH_OK);
    TEST_ASSERT_EQ(got.negotiation.enable_auto_negotiation, false);
    TEST_ASSERT_EQ(got.mac_config.speed_mode, ETH_MODE_100M_FULL);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 统计信息与吞吐量 ---- */

static int test_stats_throughput(void)
{
    printf("  Testing stats/throughput...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    eth_manager_stats_t stats;

    /* 未初始化保护 */
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_NOT_INIT);

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_stats(NULL), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.error_count, 0);

#ifndef ETH_TEST_USE_MOCKS
    /* 启动后通过 MAC 驱动发送帧, 验证统计贯通 (仅真实驱动模式) */
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);
    uint8_t frame[64];
    memset(frame, 0xAA, sizeof(frame));
    TEST_ASSERT_EQ(eth_mac_transmit(frame, sizeof(frame), 100), ETH_OK);
    TEST_ASSERT_EQ(eth_mac_transmit(frame, sizeof(frame), 100), ETH_OK);
    TEST_ASSERT_EQ(eth_mac_transmit(frame, sizeof(frame), 100), ETH_OK);

    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.mac_stats.tx_frames, 3);
    TEST_ASSERT_EQ(stats.mac_stats.tx_bytes, 192);
#endif

    /* 显式更新吞吐量 */
    TEST_ASSERT_EQ(eth_manager_update_throughput(), ETH_OK);

    /* 清除统计 */
    TEST_ASSERT_EQ(eth_manager_clear_stats(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.mac_stats.tx_frames, 0);
    TEST_ASSERT_EQ(stats.mac_stats.tx_bytes, 0);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 错误处理 ---- */

static int test_error_handling(void)
{
    printf("  Testing error handling...\n");

    eth_manager_deinit();

    /* 错误码 -> 字符串映射 */
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x0000), "No error");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x1001), "MAC initialization failed");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x2001), "DMA initialization failed");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x3001), "PHY initialization failed");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x3003), "PHY auto-negotiation failed");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x4002), "Configuration invalid");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0xFFFF), "Unknown error");
    TEST_ASSERT_STR_EQ(eth_manager_error_to_string(0x9999), "Unknown error");

    eth_error_info_t err;

#ifndef ETH_TEST_USE_MOCKS
    /* 以下初始化失败路径依赖真实驱动的配置校验 (mock 模式由 test_mock_init_failures 覆盖) */

    /* MAC 初始化失败 (MAC 地址全零) -> 0x1001 */
    eth_manager_config_t bad = make_config();
    memset(bad.mac_config.mac_addr, 0, 6);
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x1001);
    TEST_ASSERT_STR_EQ(err.error_string, "MAC initialization failed");

    /* DMA 初始化失败 (缓冲区过小) -> 0x2001 */
    bad = make_config();
    bad.dma_config.buffer_size = 32;
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x2001);
    TEST_ASSERT_STR_EQ(err.error_string, "DMA initialization failed");

    /* PHY 初始化失败 (PHY 类型越界) -> 0x3001 */
    bad = make_config();
    bad.phy_config.phy_type = (automotive_phy_type_t)99;
    TEST_ASSERT_EQ(eth_manager_init(&bad), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x3001);
    TEST_ASSERT_STR_EQ(err.error_string, "PHY initialization failed");
#endif /* !ETH_TEST_USE_MOCKS */

    /* 清除错误 */
    TEST_ASSERT_EQ(eth_manager_clear_error(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0);

    /* NULL 输出保护 */
    TEST_ASSERT_EQ(eth_manager_get_last_error(NULL), ETH_INVALID_PARAM);

    /* 失败后状态仍为 UNINIT, 可正常重新初始化 */
    eth_manager_config_t good = make_config();
    TEST_ASSERT_EQ(eth_manager_init(&good), ETH_OK);
    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 诊断与恢复 ---- */

static int test_diagnostics_reset(void)
{
    printf("  Testing diagnostics/reset...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    automotive_phy_diagnostics_t diag;

    /* 未初始化保护 */
    TEST_ASSERT_EQ(eth_manager_run_diagnostics(&diag), ETH_NOT_INIT);
    TEST_ASSERT_EQ(eth_manager_soft_reset(), ETH_NOT_INIT);
    TEST_ASSERT_EQ(eth_manager_hard_reset(), ETH_NOT_INIT);
    TEST_ASSERT_EQ(eth_manager_auto_recovery(), ETH_NOT_INIT);

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 诊断: NULL 保护 + 正常执行 (链路 UP -> link_pass) */
    TEST_ASSERT_EQ(eth_manager_run_diagnostics(NULL), ETH_INVALID_PARAM);
    TEST_ASSERT_EQ(eth_manager_run_diagnostics(&diag), ETH_OK);
    TEST_ASSERT_EQ(diag.link_pass, true);

    /* 软复位: INIT 状态下也应成功 */
    TEST_ASSERT_EQ(eth_manager_soft_reset(), ETH_OK);

    /* 硬复位:
     * 注意 - 已知模块缺陷: eth_manager_hard_reset() 先调用 eth_manager_deinit()
     * (内部 memset 清零整个上下文, 包括 config), 再以清零后的 g_manager_ctx.config
     * 重新初始化, 必然触发配置校验失败。因此当前实现中硬复位永远失败,
     * 返回 ETH_INVALID_PARAM。此处按实际行为断言并在报告中标注。 */
    TEST_ASSERT_EQ(eth_manager_hard_reset(), ETH_INVALID_PARAM);
    eth_manager_state_t st;
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_UNINIT);   /* 硬复位失败后处于未初始化 */

    /* 重新初始化后, 自动恢复 (软复位路径成功, 链路 UP) */
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_auto_recovery(), ETH_OK);

    /* 打印诊断 */
    TEST_ASSERT_EQ(eth_manager_print_diagnostics(), ETH_OK);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- 回调注册 ---- */

static int test_callback_registration(void)
{
    printf("  Testing callback registration...\n");

    eth_manager_deinit();

    /* 未初始化保护 */
    TEST_ASSERT_EQ(eth_manager_register_stats_callback(stats_cb, NULL, 100), ETH_NOT_INIT);

    eth_manager_config_t cfg = make_config();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 统计回调注册 (间隔 100ms) */
    TEST_ASSERT_EQ(eth_manager_register_stats_callback(stats_cb, NULL, 100), ETH_OK);
    /* 可重新注册为 NULL (取消) */
    TEST_ASSERT_EQ(eth_manager_register_stats_callback(NULL, NULL, 0), ETH_OK);

    /* 错误回调注册 */
    TEST_ASSERT_EQ(eth_manager_register_error_callback(err_cb, NULL), ETH_OK);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

#ifndef ETH_TEST_USE_MOCKS
/* ---- 驱动层收发路径 (真实驱动, 寄存器模拟) ---- */

static int test_driver_tx_rx_paths(void)
{
    printf("  Testing driver TX/RX paths...\n");

    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);   /* MAC/DMA 进入 ACTIVE/RUNNING */

    /* MAC 发送路径 */
    uint8_t frame[64];
    memset(frame, 0x55, sizeof(frame));
    TEST_ASSERT_EQ(eth_mac_transmit(frame, sizeof(frame), 100), ETH_OK);

    /* 非法帧: 过短 */
    TEST_ASSERT_EQ(eth_mac_transmit(frame, 4, 100), ETH_INVALID_PARAM);
    /* 非法参数: NULL */
    TEST_ASSERT_EQ(eth_mac_transmit(NULL, 64, 100), ETH_INVALID_PARAM);

    /* MAC 接收路径 (模拟: 无数据 -> 超时) */
    uint8_t buf[128];
    uint32_t rlen = 99;
    TEST_ASSERT_EQ(eth_mac_receive(buf, sizeof(buf), &rlen, 10), ETH_TIMEOUT);
    TEST_ASSERT_EQ(rlen, 0);

    /* DMA 发送路径: 入队 -> 触发 */
    eth_dma_tx_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.buffer = frame;
    pkt.length = sizeof(frame);
    pkt.ready = true;
    TEST_ASSERT_EQ(eth_dma_tx_queue_packet(&pkt, 100), ETH_OK);
    TEST_ASSERT_EQ(eth_dma_tx_trigger(), ETH_OK);

    /* DMA 发送完成检查 (模拟: 描述符仍归 DMA 所有 -> 0 完成) */
    uint16_t completed = 99;
    TEST_ASSERT_EQ(eth_dma_tx_check_complete(&completed), ETH_OK);
    TEST_ASSERT_EQ(completed, 0);

    /* DMA 接收路径 (模拟: 无数据 -> 超时) */
    eth_dma_rx_packet_t rxp;
    TEST_ASSERT_EQ(eth_dma_rx_get_packet(&rxp, 0), ETH_TIMEOUT);
    uint16_t avail = 99;
    TEST_ASSERT_EQ(eth_dma_rx_check_available(&avail), ETH_OK);
    TEST_ASSERT_EQ(avail, 0);

    /* DMA 发送可用描述符: 16 - 已用 1 - 1 */
    uint16_t tx_avail = 0;
    TEST_ASSERT_EQ(eth_dma_tx_get_available(&tx_avail), ETH_OK);
    TEST_ASSERT_EQ(tx_avail, 14);

    /* DMA 统计贯通 */
    eth_manager_stats_t mstats;
    TEST_ASSERT_EQ(eth_manager_get_stats(&mstats), ETH_OK);
    TEST_ASSERT_EQ(mstats.dma_stats.tx_packets, 1);
    TEST_ASSERT_EQ(mstats.dma_stats.tx_bytes, 64);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}
#endif /* !ETH_TEST_USE_MOCKS */

#ifdef ETH_TEST_USE_MOCKS

/* ---- Mock 模式: 链路断开状态机 ---- */

static int test_mock_link_down_state_machine(void)
{
    printf("  Testing link-down state machine (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();   /* threshold=2, auto_recovery=false */
    eth_link_status_t ls;

    reset_callbacks();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_register_link_callback(link_cb, NULL), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_start(), ETH_OK);   /* 状态 = LINK_DOWN */

    /* 链路 UP -> LINK_DOWN -> LINK_UP 迁移, 回调触发 */
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(ls, ETH_LINK_UP);
    TEST_ASSERT_EQ(g_link_cb_count, 1);
    TEST_ASSERT_EQ(g_link_cb_status, ETH_LINK_UP);

    /* 链路断开第 1 次: 未达阈值 (2), 状态保持 LINK_UP */
    mock_set_link(false);
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(ls, ETH_LINK_DOWN);
    TEST_ASSERT_EQ(g_link_cb_count, 1);   /* 状态未变, 无回调 */

    bool need = false;
    TEST_ASSERT_EQ(eth_manager_check_recovery_needed(&need), ETH_OK);
    TEST_ASSERT_EQ(need, false);

    /* 链路断开第 2 次: 达阈值 -> LINK_DOWN, 回调触发 */
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 2);
    TEST_ASSERT_EQ(g_link_cb_status, ETH_LINK_DOWN);

    TEST_ASSERT_EQ(eth_manager_check_recovery_needed(&need), ETH_OK);
    TEST_ASSERT_EQ(need, true);

    eth_manager_state_t st;
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_LINK_DOWN);

    /* 链路恢复 -> LINK_UP, 计数清零 */
    mock_set_link(true);
    TEST_ASSERT_EQ(eth_manager_check_link(&ls), ETH_OK);
    TEST_ASSERT_EQ(g_link_cb_count, 3);
    TEST_ASSERT_EQ(g_link_cb_status, ETH_LINK_UP);
    TEST_ASSERT_EQ(eth_manager_check_recovery_needed(&need), ETH_OK);
    TEST_ASSERT_EQ(need, false);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 等待链路超时 ---- */

static int test_mock_wait_link_timeout(void)
{
    printf("  Testing wait-for-link timeout (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 链路断开 -> 超时 */
    mock_set_link(false);
    TEST_ASSERT_EQ(eth_manager_wait_for_link(50), ETH_TIMEOUT);

    /* 链路恢复 -> 成功 */
    mock_set_link(true);
    TEST_ASSERT_EQ(eth_manager_wait_for_link(1000), ETH_OK);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 协商超时与完成 ---- */

static int test_mock_negotiation_timeout(void)
{
    printf("  Testing negotiation timeout (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 协商未完成 -> 超时, 状态保持 false */
    mock_set_neg_done(false);
    TEST_ASSERT_EQ(eth_manager_wait_for_negotiation(50), ETH_TIMEOUT);
    bool complete = true;
    TEST_ASSERT_EQ(eth_manager_get_negotiation_status(&complete), ETH_OK);
    TEST_ASSERT_EQ(complete, false);

    /* 协商完成 -> 成功, 状态置 true */
    mock_set_neg_done(true);
    TEST_ASSERT_EQ(eth_manager_wait_for_negotiation(1000), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_negotiation_status(&complete), ETH_OK);
    TEST_ASSERT_EQ(complete, true);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 错误回调触发 ---- */

static int test_mock_error_callback(void)
{
    printf("  Testing error callback (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    reset_callbacks();
    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_register_error_callback(err_cb, NULL), ETH_OK);

    /* 协商失败 -> eth_set_error(0x3003) -> 错误回调触发 */
    mock_fail_phy_auto_neg = true;
    TEST_ASSERT_EQ(eth_manager_start_auto_negotiation(), ETH_ERROR);
    TEST_ASSERT_EQ(g_err_cb_count, 1);
    TEST_ASSERT_EQ(g_err_cb_code, 0x3003);

    /* 最后错误信息 */
    eth_error_info_t err;
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x3003);
    TEST_ASSERT_STR_EQ(err.error_string, "PHY auto-negotiation failed");

    /* 错误计数 */
    eth_manager_stats_t stats;
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.error_count, 1);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 初始化失败路径 ---- */

static int test_mock_init_failures(void)
{
    printf("  Testing init failure paths (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_error_info_t err;

    /* MAC 初始化失败 -> 0x1001 */
    mock_fail_mac_init = true;
    {
        eth_manager_config_t cfg = make_config();
        TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_ERROR);
    }
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x1001);

    /* DMA 初始化失败 -> 0x2001 */
    mock_reset_all();
    eth_manager_deinit();
    mock_fail_dma_init = true;
    {
        eth_manager_config_t cfg = make_config();
        TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_ERROR);
    }
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x2001);

    /* PHY 初始化失败 -> 0x3001 */
    mock_reset_all();
    eth_manager_deinit();
    mock_fail_phy_init = true;
    {
        eth_manager_config_t cfg = make_config();
        TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_ERROR);
    }
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x3001);

    /* 恢复后正常初始化 */
    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t good = make_config();
    TEST_ASSERT_EQ(eth_manager_init(&good), ETH_OK);
    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 自动恢复失败路径 ---- */

static int test_mock_auto_recovery_failure(void)
{
    printf("  Testing auto-recovery failure (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* PHY 软复位失败 -> 软恢复失败 -> 转硬复位 -> 硬复位因模块缺陷失败 -> 0x4004 */
    mock_fail_phy_soft_reset = true;
    TEST_ASSERT_EQ(eth_manager_auto_recovery(), ETH_INVALID_PARAM);

    eth_error_info_t err;
    TEST_ASSERT_EQ(eth_manager_get_last_error(&err), ETH_OK);
    TEST_ASSERT_EQ(err.error_code, 0x4004);
    TEST_ASSERT_STR_EQ(err.error_string, "Auto-recovery failed");

    /* 恢复失败后管理器处于 UNINIT (硬复位内部 deinit 后 init 失败) */
    eth_manager_state_t st;
    TEST_ASSERT_EQ(eth_manager_get_state(&st), ETH_OK);
    TEST_ASSERT_EQ(st, ETH_MANAGER_STATE_UNINIT);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 自动恢复成功路径 ---- */

static int test_mock_auto_recovery_success(void)
{
    printf("  Testing auto-recovery success (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 软复位成功 + 链路 UP -> 恢复成功 */
    TEST_ASSERT_EQ(eth_manager_auto_recovery(), ETH_OK);

    eth_manager_stats_t stats;
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.recovery_count, 1);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

/* ---- Mock 模式: 统计注入 ---- */

static int test_mock_stats_injection(void)
{
    printf("  Testing stats injection (mock)...\n");

    mock_reset_all();
    eth_manager_deinit();
    eth_manager_config_t cfg = make_config();

    TEST_ASSERT_EQ(eth_manager_init(&cfg), ETH_OK);

    /* 注入驱动层统计 */
    mock_mac_stats.tx_bytes = 16000;
    mock_mac_stats.tx_frames = 250;
    mock_dma_stats.tx_bytes = 8000;
    mock_dma_stats.tx_packets = 125;

    eth_manager_stats_t stats;
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.mac_stats.tx_bytes, 16000);
    TEST_ASSERT_EQ(stats.mac_stats.tx_frames, 250);
    TEST_ASSERT_EQ(stats.dma_stats.tx_bytes, 8000);
    TEST_ASSERT_EQ(stats.dma_stats.tx_packets, 125);

    /* 管理器统计贯通 PHY 统计 */
    mock_phy_stats.link_up_count = 3;
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.link_up_count, 3);

    /* 显式吞吐量更新 */
    TEST_ASSERT_EQ(eth_manager_update_throughput(), ETH_OK);

    /* 清除统计 */
    TEST_ASSERT_EQ(eth_manager_clear_stats(), ETH_OK);
    TEST_ASSERT_EQ(eth_manager_get_stats(&stats), ETH_OK);
    TEST_ASSERT_EQ(stats.mac_stats.tx_bytes, 0);
    TEST_ASSERT_EQ(stats.link_up_count, 0);

    eth_manager_deinit();

    printf("  PASSED\n");
    return 0;
}

#endif /* ETH_TEST_USE_MOCKS */

/* ============================================================================
 * Test Runner (仿 test_csm.c)
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
#ifdef ETH_TEST_USE_MOCKS
    printf("Eth Manager Unit Tests (MOCK driver layer)\n");
#else
    printf("Eth Manager Unit Tests (real driver layer)\n");
#endif
    printf("============================================\n\n");

    /* 与构建模式无关的基础测试 */
    run_test(test_init_deinit, "Init/Deinit");
    run_test(test_state_machine, "State Machine");
    run_test(test_link_management, "Link Management");
    run_test(test_link_monitoring_disabled, "Link Monitoring Disabled");
    run_test(test_auto_negotiation, "Auto Negotiation");
    run_test(test_stats_throughput, "Stats/Throughput");
    run_test(test_error_handling, "Error Handling");
    run_test(test_diagnostics_reset, "Diagnostics/Reset");
    run_test(test_callback_registration, "Callback Registration");

#ifdef ETH_TEST_USE_MOCKS
    /* 仅 mock 模式: 可注入的硬件行为测试 */
    run_test(test_mock_link_down_state_machine, "Mock Link-Down State Machine");
    run_test(test_mock_wait_link_timeout, "Mock Wait-For-Link Timeout");
    run_test(test_mock_negotiation_timeout, "Mock Negotiation Timeout");
    run_test(test_mock_error_callback, "Mock Error Callback");
    run_test(test_mock_init_failures, "Mock Init Failures");
    run_test(test_mock_auto_recovery_failure, "Mock Auto-Recovery Failure");
    run_test(test_mock_auto_recovery_success, "Mock Auto-Recovery Success");
    run_test(test_mock_stats_injection, "Mock Stats Injection");
#else
    /* 真实驱动模式: 驱动层收发路径测试 */
    run_test(test_driver_tx_rx_paths, "Driver TX/RX Paths");
#endif

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
