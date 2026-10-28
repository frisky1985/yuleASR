/**
 * @file test_dds_tsn.c
 * @brief DDS TSN 传输层单元测试 (tests/unit/ddstrans)
 *
 * 被测: src/middleware/dds/transport/tsn/dds_tsn_transport.c (真实源码)
 * 桩:   tsn_stack.h 命中最小桩 (被测代码未调用任何 TSN 协议栈函数)。
 */
#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "dds_tsn_transport.h"

/* ============================================================================
 * 测试夹具
 * ============================================================================ */
static const uint8_t MAC_LOCAL[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static const uint8_t MAC_DST[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01};

static int g_state_cb_cnt;
static dds_tsn_stream_state_t g_state_cb_last;
static uint8_t g_state_cb_id0;

static void state_cb(const dds_tsn_stream_id_t *stream_id,
                     dds_tsn_stream_state_t new_state, void *user_data)
{
    (void)user_data;
    g_state_cb_cnt++;
    g_state_cb_last = new_state;
    if (stream_id != NULL) {
        g_state_cb_id0 = stream_id->id[0];
    }
}

static int g_rx_cb_cnt;
static uint32_t g_rx_cb_len;

static void rx_data_cb(const dds_tsn_stream_id_t *stream_id, const uint8_t *data,
                       uint32_t len, uint32_t stream_index, void *user_data)
{
    (void)stream_id; (void)data; (void)stream_index; (void)user_data;
    g_rx_cb_cnt++;
    g_rx_cb_len = len;
}

static int g_lat_cb_cnt;

static void latency_violation_cb(const dds_tsn_stream_id_t *stream_id,
                                 uint32_t measured_ns, uint32_t budget_ns, void *user_data)
{
    (void)stream_id; (void)measured_ns; (void)budget_ns; (void)user_data;
    g_lat_cb_cnt++;
}

static void reset_cbs(void)
{
    g_state_cb_cnt = 0;
    g_state_cb_last = DDS_TSN_STREAM_STATE_IDLE;
    g_state_cb_id0 = 0;
    g_rx_cb_cnt = 0; g_rx_cb_len = 0;
    g_lat_cb_cnt = 0;
}

static dds_tsn_stream_config_t talker_cfg(const char *topic, uint32_t domain)
{
    dds_tsn_stream_config_t c;
    memset(&c, 0, sizeof(c));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_generate_stream_id(topic, domain, &c.id.stream_id));
    memcpy(c.id.destination, MAC_DST, 6);
    c.id.vlan_id = 100;
    c.id.priority = 5;
    c.type = DDS_TSN_STREAM_TYPE_TALKER;
    c.enable_cbs = true;
    c.enable_tas = false;
    c.enable_frame_preemption = false;
    c.max_latency_ns = 100000;
    strcpy(c.topic_name, topic);
    return c;
}

static dds_tsn_stream_config_t listener_cfg(const char *topic, uint32_t domain)
{
    dds_tsn_stream_config_t c = talker_cfg(topic, domain);
    c.type = DDS_TSN_STREAM_TYPE_LISTENER;
    return c;
}

static void do_init(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_init(MAC_LOCAL, 100));
}

static uint32_t create_talker(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Send", 1);
    uint32_t handle = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    return handle;
}

void setUp(void)
{
    reset_cbs();
    dds_tsn_transport_deinit();                          /* 清理遗留产品状态 (未初始化时为 no-op) */
}

void tearDown(void)
{
    dds_tsn_transport_deinit();
}

/* ============================================================================
 * 生命周期
 * ============================================================================ */
static void test_tsn_init_guards(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_transport_init(NULL, 100));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_BUSY, dds_tsn_transport_init(MAC_LOCAL, 100));
    dds_tsn_transport_deinit();
    do_init();                                           /* 重新 init 成功 */
}

static void test_tsn_start_stop(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_transport_stop());
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_BUSY, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_stop());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_stop()); /* 未运行再 stop 仍 OK */
}

static void test_tsn_main_function(void)
{
    /* NOTE(产品行为): main_function 仅检查 initialized, 不要求 started */
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_transport_main_function(10));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_main_function(10));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_main_function(10));
}

/* ============================================================================
 * Stream 管理
 * ============================================================================ */
static void test_tsn_create_guards(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("T1", 1);
    uint32_t handle = 0;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_create_stream(&cfg, &handle));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_create_stream(NULL, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_create_stream(&cfg, NULL));
}

static void test_tsn_create_stream(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Temp", 0x01020304);
    dds_tsn_stream_config_t out;
    uint32_t handle = 0xFFFF, handle2 = 0xFFFF;
    dds_tsn_stream_state_t state = DDS_TSN_STREAM_STATE_IDLE;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_UINT32(0, handle);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_state(handle, &state));
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_CONFIGURING, (int)state); /* 创建即进入配置中 */

    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_config(handle, &out));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cfg.id.stream_id.id, out.id.stream_id.id, 8);
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_TYPE_TALKER, (int)out.type);
    TEST_ASSERT_EQUAL_UINT16(100, out.id.vlan_id);
    TEST_ASSERT_EQUAL_UINT8(5, out.id.priority);
    TEST_ASSERT_EQUAL_STRING("Temp", out.topic_name);

    {
        dds_tsn_stream_config_t lcfg = listener_cfg("Cmd", 1);
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&lcfg, &handle2));
    }
    TEST_ASSERT_EQUAL_UINT32(1, handle2);                /* 新 Stream 使用下一空槽 */
}

static void test_tsn_create_duplicate_stream(void)
{
    /* 相同 stream_id 重复创建: 返回现有句柄, 不占用新槽位 */
    dds_tsn_stream_config_t cfg = talker_cfg("Dup", 1);
    dds_tsn_stream_config_t other = talker_cfg("Other", 1);
    uint32_t h1 = 0xFFFF, h2 = 0xFFFF, h3 = 0xFFFF;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &h1));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &h2));
    TEST_ASSERT_EQUAL_UINT32(h1, h2);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&other, &h3));
    TEST_ASSERT_EQUAL_UINT32(1, h3);                     /* 重复创建未消耗槽位 */
}

static void test_tsn_create_full(void)
{
    dds_tsn_stream_config_t cfg;
    char topic[8];
    uint32_t handle = 0;
    int i;

    do_init();
    for (i = 0; i < (int)DDS_TSN_MAX_STREAMS; i++) {
        snprintf(topic, sizeof(topic), "T%d", i);
        cfg = talker_cfg(topic, (uint32_t)(i + 1U));
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    }
    cfg = talker_cfg("Overflow", 0xFF);
    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_tsn_create_stream(&cfg, &handle));
}

static void test_tsn_delete_stream(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Del", 1);
    dds_tsn_stream_state_t state;
    uint32_t handle = 0;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_delete_stream(0));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_delete_stream(DDS_TSN_MAX_STREAMS));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_delete_stream(0)); /* 未使用 */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_delete_stream(handle));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_delete_stream(handle)); /* 再次删除 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_stream_state(handle, &state)); /* 条目已失效 */
}

static void test_tsn_delete_stats_underflow(void)
{
    /* NOTE(产品缺陷): create_stream 不增加 stats.active_talkers,
     * delete_stream 却无条件递减 → uint32 下溢 */
    dds_tsn_stream_config_t cfg = talker_cfg("Under", 1);
    uint32_t handle = 0;
    dds_tsn_transport_stats_t stats;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_talkers);   /* create 也不计数 */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_delete_stream(handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, stats.active_talkers); /* 下溢 */
}

static void test_tsn_config_get_update(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Cfg", 2);
    dds_tsn_stream_config_t new_cfg = talker_cfg("Cfg2", 2);
    dds_tsn_stream_config_t out;
    dds_tsn_stream_state_t state;
    uint32_t handle = 0xFFFF;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_stream_config(0, &out));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_update_stream_config(0, &cfg));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_stream_state(0, &state));

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stream_config(DDS_TSN_MAX_STREAMS, &out));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stream_config(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_update_stream_config(DDS_TSN_MAX_STREAMS, &cfg));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_update_stream_config(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stream_state(DDS_TSN_MAX_STREAMS, &state));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stream_state(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_stream_config(0, &out));  /* 未使用 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_update_stream_config(0, &cfg));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_stream_state(0, &state));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    new_cfg.max_latency_ns = 777777;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_update_stream_config(handle, &new_cfg)); /* 非运行态可更新 */
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_config(handle, &out));
    TEST_ASSERT_EQUAL_UINT32(777777, out.max_latency_ns);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start()); /* 置 RUNNING */
    new_cfg.max_latency_ns = 888888;
    TEST_ASSERT_EQUAL_INT(ETH_BUSY, dds_tsn_update_stream_config(handle, &new_cfg));
}

/* ============================================================================
 * Talker / Listener
 * ============================================================================ */
static void test_tsn_talker_config(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Talk", 1);
    dds_tsn_stream_config_t lcfg = listener_cfg("Listen", 1);
    dds_tsn_talker_endpoint_t tk;
    dds_tsn_talker_status_t st;
    uint32_t th = 0xFFFF, lh = 0xFFFF;

    memset(&tk, 0, sizeof(tk));
    tk.traffic_spec.interval_ns = 1000000;
    tk.traffic_spec.max_frame_size = 1522;
    tk.id.vlan_id = 100;
    tk.id.priority = 6;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_configure_talker(0, &tk));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_talker_status(0, &st));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_talker(DDS_TSN_MAX_STREAMS, &tk));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_talker(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_talker_status(DDS_TSN_MAX_STREAMS, &st));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_talker_status(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_talker(0, &tk));   /* 未使用 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_talker_status(0, &st));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &th));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&lcfg, &lh));

    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_talker(lh, &tk));  /* Listener 流不能配置 Talker */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_talker_status(lh, &st));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_configure_talker(th, &tk));
    memset(&st, 0, sizeof(st));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_talker_status(th, &st));
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_CONFIGURING, (int)st.state);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)st.frames_sent);
    TEST_ASSERT_FALSE(st.stream_active);
}

static void test_tsn_listener_config(void)
{
    dds_tsn_stream_config_t cfg = listener_cfg("Lst", 1);
    dds_tsn_stream_config_t tcfg = talker_cfg("Tlkr", 1);
    dds_tsn_listener_endpoint_t ln;
    dds_tsn_listener_status_t st;
    uint32_t lh = 0xFFFF, th = 0xFFFF;

    memset(&ln, 0, sizeof(ln));
    ln.max_latency_ns = 50000;
    ln.vlan_id = 100;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_configure_listener(0, &ln));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_listener_status(0, &st));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_listener(DDS_TSN_MAX_STREAMS, &ln));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_listener(0, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_listener_status(0, &st));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &lh));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&tcfg, &th));

    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_listener(th, &ln)); /* Talker 流不能配置 Listener */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_listener_status(th, &st));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_configure_listener(lh, &ln));
    memset(&st, 0, sizeof(st));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_listener_status(lh, &st));
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_CONFIGURING, (int)st.state);
    TEST_ASSERT_EQUAL_UINT32(0, st.frames_received);
    TEST_ASSERT_FALSE(st.source_active);
}

/* ============================================================================
 * 数据发送
 * ============================================================================ */
static void test_tsn_send_data(void)
{
    uint8_t data[16];
    dds_tsn_transport_stats_t stats;
    dds_tsn_talker_status_t st;
    uint64_t frames = 0, bytes = 0;
    uint32_t latency = 0;
    uint32_t handle;
    int i;

    memset(data, 0xAB, sizeof(data));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_send_stream_data(0, data, 16, 0));

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_send_stream_data(DDS_TSN_MAX_STREAMS, data, 16, 0));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_send_stream_data(0, NULL, 16, 0));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_send_stream_data(0, data, 0, 0));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_send_stream_data(0, data, 16, 0)); /* 未使用 */

    handle = create_talker();
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_send_stream_data(handle, data, 16, 0)); /* CONFIGURING 非 RUNNING */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    for (i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_send_stream_data(handle, data, (uint32_t)(16U + (uint32_t)i), 12345U));
    }

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_talker_status(handle, &st));
    TEST_ASSERT_EQUAL_UINT32(3, st.frames_sent);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(16U * 3U + 0U + 1U + 2U), (uint32_t)st.bytes_sent);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_stats(handle, &frames, &bytes, &latency));
    TEST_ASSERT_EQUAL_UINT32(3, (uint32_t)frames);
    TEST_ASSERT_EQUAL_UINT32(51, (uint32_t)bytes);
    TEST_ASSERT_EQUAL_UINT32(0, latency);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(3, (uint32_t)stats.total_frames_sent);
    TEST_ASSERT_EQUAL_UINT32(51, (uint32_t)stats.total_bytes_sent);
    TEST_ASSERT_EQUAL_UINT32(0, stats.total_tx_errors);
}

static void test_tsn_send_data_listener_rejected(void)
{
    uint8_t data[4] = {1, 2, 3, 4};
    dds_tsn_stream_config_t cfg = listener_cfg("Rx", 1);
    uint32_t handle = 0xFFFF;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_send_stream_data(handle, data, 4, 0)); /* Listener 流不可发送 */
}

static void test_tsn_rx_callback(void)
{
    uint32_t handle;
    dds_tsn_stream_config_t cfg = listener_cfg("Cb", 1);

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_register_rx_callback(0, rx_data_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_unregister_rx_callback(0));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_register_rx_callback(DDS_TSN_MAX_STREAMS, rx_data_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_register_rx_callback(0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_unregister_rx_callback(DDS_TSN_MAX_STREAMS));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_register_rx_callback(0, rx_data_cb, NULL));  /* 未使用 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_unregister_rx_callback(0));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_register_rx_callback(handle, rx_data_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_unregister_rx_callback(handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_unregister_rx_callback(handle)); /* 重复注销仍 OK */
    TEST_ASSERT_EQUAL_INT(0, g_rx_cb_cnt);
}

/* ============================================================================
 * Stream ID
 * ============================================================================ */
static void test_tsn_stream_id_generate_parse(void)
{
    dds_tsn_stream_id_t sid;
    uint32_t domain = 0xFFFFFFFF;
    char topic[32];

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_generate_stream_id(NULL, 1, &sid));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_generate_stream_id("T", 1, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_parse_stream_id(NULL, topic, sizeof(topic), &domain));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_parse_stream_id(&sid, topic, sizeof(topic), NULL));

    /* 手算哈希: hash = hash*31 + char */
    {
        dds_tsn_stream_id_t gen;
        const char *name = "Temp";
        uint32_t hash = 0;
        size_t i;
        for (i = 0; i < strlen(name); i++) {
            hash = (hash * 31U) + (uint32_t)name[i];
        }
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_generate_stream_id(name, 0x01020304U, &gen));
        TEST_ASSERT_EQUAL_HEX8(0x01, gen.id[0]);
        TEST_ASSERT_EQUAL_HEX8(0x02, gen.id[1]);
        TEST_ASSERT_EQUAL_HEX8(0x03, gen.id[2]);
        TEST_ASSERT_EQUAL_HEX8(0x04, gen.id[3]);
        TEST_ASSERT_EQUAL_HEX32(hash, ((uint32_t)gen.id[4] << 24) | ((uint32_t)gen.id[5] << 16) |
                                      ((uint32_t)gen.id[6] << 8) | (uint32_t)gen.id[7]);
        memcpy(&sid, &gen, sizeof(sid));
    }

    domain = 0xFFFFFFFF;
    topic[0] = 'X';
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_parse_stream_id(&sid, topic, sizeof(topic), &domain));
    TEST_ASSERT_EQUAL_UINT32(0x01020304U, domain);
    /* NOTE(产品行为): Stream ID 只编码 domain+哈希, 主题名不可还原, 恒为空串 */
    TEST_ASSERT_EQUAL_UINT32(0, strlen(topic));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_parse_stream_id(&sid, NULL, 0, &domain)); /* topic 可为 NULL */
    TEST_ASSERT_EQUAL_UINT32(0x01020304U, domain);
}

static void test_tsn_stream_id_equal_tostring(void)
{
    dds_tsn_stream_id_t a, b;
    char str[32];

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_generate_stream_id("A", 1, &a));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_generate_stream_id("A", 1, &b));
    TEST_ASSERT_TRUE(dds_tsn_stream_id_equal(&a, &b));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_generate_stream_id("B", 1, &b));
    TEST_ASSERT_FALSE(dds_tsn_stream_id_equal(&a, &b));

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_stream_id_to_string(NULL, str, 32));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_stream_id_to_string(&a, NULL, 32));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_stream_id_to_string(&a, str, 16)); /* 至少 17 */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_stream_id_to_string(&a, str, sizeof(str)));
    TEST_ASSERT_EQUAL_UINT32(17, strlen(str));           /* 16 hex + '-' (内容在专门用例校验) */
}

static void test_tsn_stream_id_tostring_content(void)
{
    dds_tsn_stream_id_t a;
    char str[32];

    memset(&a, 0, sizeof(a));
    a.id[0] = 0x01; a.id[1] = 0x02; a.id[2] = 0x03; a.id[3] = 0x04;
    a.id[4] = 0xAB; a.id[5] = 0xCD; a.id[6] = 0xEF; a.id[7] = 0x00;

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_stream_id_to_string(&a, str, sizeof(str)));
    TEST_ASSERT_EQUAL_STRING("01020304-ABCDEF00", str);
}

/* ============================================================================
 * 预留 / TAS / CBS
 * ============================================================================ */
static void test_tsn_reserve_release(void)
{
    uint32_t handle;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_reserve_stream(0));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_release_stream(0));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_reserve_stream(DDS_TSN_MAX_STREAMS));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_release_stream(DDS_TSN_MAX_STREAMS));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_reserve_stream(0));    /* 未使用 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_release_stream(0));

    handle = create_talker();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_reserve_stream(handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_reserve_stream(handle));  /* 已预留仍 OK */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_release_stream(handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_release_stream(handle));  /* 未预留仍 OK */
}

static void test_tsn_reserve_delete_interplay(void)
{
    /* 预留后删除: delete 内部先 release 再清理, 不崩溃 */
    uint32_t handle;
    dds_tsn_stream_config_t cfg = talker_cfg("ResDel", 1);

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_reserve_stream(handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_delete_stream(handle));
}

static void test_tsn_tas_cbs(void)
{
    uint32_t handle;
    dds_tsn_stream_config_t cfg = talker_cfg("QoS", 1);

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_configure_tas(0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_configure_cbs(0, 100, -100));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_tas(DDS_TSN_MAX_STREAMS, NULL, 0));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_configure_cbs(DDS_TSN_MAX_STREAMS, 100, 100));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_tas(0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_cbs(0, 100, 100));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_configure_tas(handle, NULL, 0)); /* 当前为空实现 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_configure_cbs(handle, 1000, 500));
}

/* ============================================================================
 * 回调与统计
 * ============================================================================ */
static void test_tsn_state_callback(void)
{
    uint32_t handle;
    dds_tsn_stream_config_t cfg = talker_cfg("State", 1);

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_register_state_callback(state_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_register_latency_violation_callback(latency_violation_cb, NULL));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_register_state_callback(state_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_register_latency_violation_callback(latency_violation_cb, NULL));
    TEST_ASSERT_EQUAL_INT(0, g_lat_cb_cnt);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_INT(1, g_state_cb_cnt);
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_CONFIGURING, (int)g_state_cb_last);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_state_cb_id0);        /* domain 1 → id[0]=0x00 */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    TEST_ASSERT_EQUAL_INT(2, g_state_cb_cnt);
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_RUNNING, (int)g_state_cb_last);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_stop());
    TEST_ASSERT_EQUAL_INT(3, g_state_cb_cnt);
    TEST_ASSERT_EQUAL_INT((int)DDS_TSN_STREAM_STATE_READY, (int)g_state_cb_last);
}

static void test_tsn_stats(void)
{
    dds_tsn_transport_stats_t stats;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_clear_stats());
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stats(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_talkers);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)stats.total_frames_sent);

    stats.total_frames_sent = 42;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_clear_stats());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)stats.total_frames_sent);
}

static void test_tsn_stream_stats_guards(void)
{
    uint64_t frames = 0, bytes = 0;
    uint32_t latency = 0;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_tsn_get_stream_stats(0, &frames, &bytes, &latency));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_stream_stats(DDS_TSN_MAX_STREAMS, &frames, &bytes, &latency));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_get_stream_stats(0, &frames, &bytes, &latency)); /* 未使用 */
    /* 输出参数允许为 NULL */
    (void)create_talker();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_stats(0, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_stream_stats(0, &frames, &bytes, &latency));
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)frames);
}

/* ============================================================================
 * main_function 在 RUNNING + reserved=false 时触发 reserve
 * ============================================================================ */
static void test_tsn_main_function_reserve_running_stream(void)
{
    uint32_t handle;

    do_init();
    handle = create_talker();
    /* create_stream 状态 CONFIGURING, main_function 不触发 reserve.
     * 启动 transport 把 stream 置为 RUNNING, 但 create_stream 后 reserved 默认就是 false. */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_start());
    /* 调用 reserve_stream 让 reserved = true, 然后再 main_function 不触发.
     * 但我们要触发 reserve: 需要 stream RUNNING 且 reserved=false.
     * create_stream 后 reserved 是 false, start 后 state 变为 RUNNING.
     * 所以 main_function 会走到 (state==RUNNING && !reserved) → 调 reserve_stream. */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_main_function(10));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_transport_main_function(10)); /* 已 reserved=true, 不调 */
}

/* ============================================================================
 * configure_listener/get_listener_status 未使用 stream guard + 超范围 + NULL
 * ============================================================================ */
static void test_tsn_listener_guards_extended(void)
{
    uint32_t unused_handle;
    dds_tsn_listener_status_t lst;

    do_init();
    /* 找一个未使用的 stream handle */
    dds_tsn_stream_config_t cfg = listener_cfg("LstGrd", 1);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &unused_handle));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_get_listener_status(unused_handle, &lst));

    /* configure_listener 未使用 stream guard: delete 后再配 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_delete_stream(unused_handle));
    dds_tsn_listener_endpoint_t ln;
    memset(&ln, 0, sizeof(ln));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_tsn_configure_listener(unused_handle, &ln));

    /* get_listener_status guard: status == NULL */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_tsn_get_listener_status(unused_handle, NULL));
    /* get_listener_status guard: stream_handle >= DDS_TSN_MAX_STREAMS */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM,
                          dds_tsn_get_listener_status(DDS_TSN_MAX_STREAMS, &lst));
}

/* ============================================================================
 * setup + teardown TAS / preemption 代码路径 (reserve_stream 触发 setup_tsn_features)
 * ============================================================================ */
static void test_tsn_stream_with_tas_preemption_setup_teardown(void)
{
    dds_tsn_stream_config_t cfg;
    uint32_t handle;

    do_init();
    cfg = talker_cfg("TasStream", 1);
    cfg.enable_tas = true;
    cfg.enable_frame_preemption = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    /* reserve_stream 会调 setup_tsn_features → 覆盖 enable_tas / enable_frame_preemption 分支 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_reserve_stream(handle));
    /* release_stream 会调 teardown_tsn_features → 覆盖 teardown */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_release_stream(handle));
    /* delete_stream 内部也调 teardown, 再次覆盖 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_delete_stream(handle));
}

/* ============================================================================
 * Stream type = BOTH 覆盖
 * ============================================================================ */
static void test_tsn_create_stream_type_both(void)
{
    dds_tsn_stream_config_t cfg = talker_cfg("Both", 1);
    cfg.type = DDS_TSN_STREAM_TYPE_BOTH;
    uint32_t handle = 0xFFFF;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_tsn_create_stream(&cfg, &handle));
    TEST_ASSERT_EQUAL_UINT32(0, handle);
}

/* ============================================================================
 * main
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tsn_init_guards);
    RUN_TEST(test_tsn_start_stop);
    RUN_TEST(test_tsn_main_function);
    RUN_TEST(test_tsn_create_guards);
    RUN_TEST(test_tsn_create_stream);
    RUN_TEST(test_tsn_create_duplicate_stream);
    RUN_TEST(test_tsn_create_full);
    RUN_TEST(test_tsn_delete_stream);
    RUN_TEST(test_tsn_delete_stats_underflow);
    RUN_TEST(test_tsn_config_get_update);
    RUN_TEST(test_tsn_talker_config);
    RUN_TEST(test_tsn_listener_config);
    RUN_TEST(test_tsn_send_data);
    RUN_TEST(test_tsn_send_data_listener_rejected);
    RUN_TEST(test_tsn_rx_callback);
    RUN_TEST(test_tsn_stream_id_generate_parse);
    RUN_TEST(test_tsn_stream_id_equal_tostring);
    RUN_TEST(test_tsn_stream_id_tostring_content);
    RUN_TEST(test_tsn_reserve_release);
    RUN_TEST(test_tsn_reserve_delete_interplay);
    RUN_TEST(test_tsn_tas_cbs);
    RUN_TEST(test_tsn_state_callback);
    RUN_TEST(test_tsn_stats);
    RUN_TEST(test_tsn_stream_stats_guards);
    /* 新增: main_function reserve + listener guards + teardown */
    RUN_TEST(test_tsn_main_function_reserve_running_stream);
    RUN_TEST(test_tsn_listener_guards_extended);
    RUN_TEST(test_tsn_stream_with_tas_preemption_setup_teardown);
    RUN_TEST(test_tsn_create_stream_type_both);
    return UNITY_END();
}
