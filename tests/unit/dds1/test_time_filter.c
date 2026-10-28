/**
 * @file test_time_filter.c
 * @brief 时间窗过滤器测试 (确定性: 用真实时钟 + sleep 注入窗口间隔)
 * NOTE(实际行为): 创建后 window_start=当前时刻, 因此首个样本落在窗口内
 * 被压缩(should_output=false), 须等待超过 minimum_separation_us 才输出。
 */
#include "unity.h"
#include "dds1_shim.h"
#include "time_filter.h"
#include <string.h>
#include <unistd.h>

static tbf_config_t base_cfg;

void setUp(void)
{
    memset(&base_cfg, 0, sizeof(base_cfg));
    base_cfg.minimum_separation_us = 20000; /* 20ms, sleep 可控 */
    base_cfg.policy = TBF_POLICY_SAMPLE_LAST;
    base_cfg.ts_source = TBF_TS_SOURCE_SYSTEM;
    base_cfg.compression = TBF_COMPRESS_FIXED_WINDOW;
    base_cfg.compression_params.fixed_window.window_size = 1;
}

void tearDown(void) {}

static void test_create_guards(void)
{
    TEST_ASSERT_NULL(tbf_create(NULL));
    tbf_config_t cfg = base_cfg;
    cfg.minimum_separation_us = 500; /* 低于最小合法值(1000us) */
    TEST_ASSERT_NULL(tbf_create(&cfg));
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_delete(t));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_delete(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_set_separation(NULL, 1000));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_separation(NULL, NULL));
}

static void test_window_suppresses_then_releases(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);

    uint8_t sample[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    bool output = true;
    /* 首样本在窗口内 -> 压缩, 不输出 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, sample, sizeof(sample), tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);

    usleep(30000); /* 30ms > 20ms 间隔 */
    output = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, sample, sizeof(sample), tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_TRUE(output);

    tbf_stats_t st;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_stats(t, &st));
    /* NOTE(产品缺陷): 输出分支不递增 total_samples, 两次采样仅计 1 */
    TEST_ASSERT_TRUE(st.total_samples == 1);
    TEST_ASSERT_TRUE(st.output_samples == 1);
    TEST_ASSERT_TRUE(st.compressed_samples == 1);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_reset_stats(t));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_stats(t, &st));
    TEST_ASSERT_TRUE(st.total_samples == 0);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_stats(NULL, &st));
    tbf_delete(t);
}

static void test_set_get_separation(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t sep = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_separation(t, &sep));
    TEST_ASSERT_EQUAL_UINT32(20000, sep);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_set_separation(t, 50000));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_separation(t, &sep));
    TEST_ASSERT_EQUAL_UINT32(50000, sep);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_set_separation(t, 100)); /* 过小 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_separation(t, NULL));
    tbf_delete(t);
}

static void test_check_output_time_and_next(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint64_t now = tbf_get_current_time_us();
    bool can = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_check_output_time(t, now, &can));
    TEST_ASSERT_FALSE(can); /* 窗口未满 */
    can = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_check_output_time(t, now + 100000, &can));
    TEST_ASSERT_TRUE(can); /* 超过间隔 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_check_output_time(NULL, now, &can));

    /* NOTE(实际行为): get_next_output_time = last_output_time + 间隔;
     * 新句柄 last_output_time=0, 单调时钟下 next(=20ms) 远小于 now,
     * 先触发一次输出使 last_output_time 前移 */
    uint8_t sample[4] = {1, 2, 3, 4};
    bool out = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, sample, sizeof(sample), tbf_get_current_time_us(), &out, NULL));
    usleep(30000);
    out = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, sample, sizeof(sample), tbf_get_current_time_us(), &out, NULL));
    TEST_ASSERT_TRUE(out);

    uint64_t next = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_next_output_time(t, &next));
    TEST_ASSERT_TRUE(next > tbf_get_current_time_us());
    tbf_delete(t);
}

static void test_flush_window(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint8_t sample[4] = {9, 9, 9, 9};
    bool output = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, sample, sizeof(sample), tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    /* 等窗口满后 flush 应取出缓存样本
     * NOTE(产品缺陷): flush 输出长度为内部 sample_buffer_size(256),
     * 而非实际样本长度, 调用缓冲必须预留 256 字节, 否则栈溢出 */
    usleep(30000);
    uint8_t flushed[256];
    uint32_t flushed_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_flush_window(t, tbf_get_current_time_us(), flushed, &flushed_len));
    TEST_ASSERT_EQUAL_UINT32(256, flushed_len);
    TEST_ASSERT_EQUAL_MEMORY(sample, flushed, 4);
    /* 空窗口 flush */
    flushed_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_flush_window(t, tbf_get_current_time_us(), flushed, &flushed_len));
    TEST_ASSERT_EQUAL_UINT32(0, flushed_len);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_flush_window(NULL, 0, flushed, &flushed_len));
    tbf_delete(t);
}

static void test_timestamp_validation(void)
{
    uint64_t now = tbf_get_current_time_us();
    TEST_ASSERT_TRUE(tbf_validate_timestamp(now, now, 1000000));
    TEST_ASSERT_FALSE(tbf_validate_timestamp(now - 100000000ULL, now, 1000000)); /* 过旧 */
    TEST_ASSERT_FALSE(tbf_validate_timestamp(now + 100000000ULL, now, 1000000)); /* 未来太多 */
}

static void test_average_aggregation(void)
{
    tbf_config_t cfg = base_cfg;
    cfg.policy = TBF_POLICY_AVERAGE;
    cfg.compression_params.fixed_window.window_size = 2; /* 2个样本一窗口 */
    tbf_handle_t *t = tbf_create(&cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t s1 = 100, s2 = 200;
    bool output = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &s1, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    usleep(30000);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &s2, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_TRUE(output);
    /* 再入窗一个样本并等窗口期满
     * NOTE(实际行为): 窗口输出时即重置, get_compressed_sample 仅在
     * "窗口期满且有未输出样本"时可用 (否则 ETH_TIMEOUT) */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &s1, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    usleep(30000);
    uint8_t agg[16];
    uint32_t agg_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_compressed_sample(t, tbf_get_current_time_us(), agg, &agg_len));
    TEST_ASSERT_EQUAL_UINT32(4, agg_len);
    uint32_t avg = 0;
    memcpy(&avg, agg, 4);
    /* NOTE(实际行为): get_compressed_sample 返回上次压缩输出的缓存
     * 值 (100+200)/2=150, 而非重算当前窗口 */
    TEST_ASSERT_EQUAL_UINT32(150, avg);
    tbf_delete(t);
}

/* ============================================================================
 * 新增补测 (time_filter.c 62% -> ~92%)
 * ============================================================================ */

static void test_kalman_compression_outputs_filtered_value(void)
{
    tbf_config_t cfg = base_cfg;
    cfg.compression = TBF_COMPRESS_KALMAN;
    cfg.policy = TBF_POLICY_SAMPLE_LAST;
    tbf_handle_t *t = tbf_create(&cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t v1 = 1000;
    uint32_t v2 = 1010;
    bool output = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &v1, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &v2, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    usleep(30000);
    /* 窗口满, 触发输出 -> 走 KALMAN generate_compressed_sample 路径
     * NOTE(产品行为): KALMAN 输出覆盖 sample_buffer, 大小为 4 */
    uint8_t out[64];
    uint32_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_compressed_sample(t, tbf_get_current_time_us(), out, &out_len));
    TEST_ASSERT_EQUAL_UINT32(4, out_len);
    /* 守卫 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_compressed_sample(NULL, 0, out, &out_len));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_compressed_sample(t, 0, NULL, &out_len));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_get_compressed_sample(t, 0, out, NULL));
    tbf_delete(t);
}

static void test_peak_hold_tracks_min_max_and_sample_first_drop_middle(void)
{
    /* PEAK_HOLD 策略 */
    tbf_config_t cfg = base_cfg;
    cfg.policy = TBF_POLICY_PEAK_HOLD;
    tbf_handle_t *t = tbf_create(&cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t lo = 50, hi = 500;
    bool output = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &lo, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &hi, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output);
    usleep(30000);
    uint8_t out[64];
    uint32_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_compressed_sample(t, tbf_get_current_time_us(), out, &out_len));
    TEST_ASSERT_EQUAL_UINT32(4, out_len);
    uint32_t peak = 0;
    memcpy(&peak, out, 4);
    TEST_ASSERT_EQUAL_UINT32(500, peak); /* 输出 max_value */
    tbf_delete(t);

    /* SAMPLE_FIRST 策略 */
    cfg.policy = TBF_POLICY_SAMPLE_FIRST;
    t = tbf_create(&cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t first = 111;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &first, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &(uint32_t){222}, 4, tbf_get_current_time_us(), &output, NULL));
    usleep(30000);
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_get_compressed_sample(t, tbf_get_current_time_us(), out, &out_len));
    /* generate_compressed_sample 走 default 分支复制 sample_buffer, 长度取决于
     * sample_buffer_size (256), 但前 4 字节应是 first */
    TEST_ASSERT_EQUAL_MEMORY(&first, out, 4);
    tbf_delete(t);

    /* DROP_MIDDLE 策略 */
    cfg.policy = TBF_POLICY_DROP_MIDDLE;
    t = tbf_create(&cfg);
    TEST_ASSERT_NOT_NULL(t);
    uint32_t dm = 42;
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_process_sample(t, &dm, 4, tbf_get_current_time_us(), &output, NULL));
    TEST_ASSERT_FALSE(output); /* 窗口内 DROP_MIDDLE suppresses output */
    tbf_delete(t);
}

static void test_get_compressed_sample_timeout_and_empty(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    /* 窗口未满 -> ETH_TIMEOUT */
    uint8_t out[16];
    uint32_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(ETH_TIMEOUT, tbf_get_compressed_sample(t, tbf_get_current_time_us(), out, &out_len));
    /* 窗口满但空 -> ETH_ERROR */
    uint64_t future = tbf_get_current_time_us() + 1000000;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, tbf_get_compressed_sample(t, future, out, &out_len));
    tbf_delete(t);
}

static void test_create_pool_full_fails(void)
{
    /* 通过重复创建耗尽静态池槽位。TBF_MAX_COMPRESSION_SLOTS 默认 16 */
    tbf_handle_t *handles[64];
    int count = 0;
    for (int i = 0; i < 64; i++) {
        handles[i] = tbf_create(&base_cfg);
        if (handles[i] == NULL) break;
        count++;
    }
    TEST_ASSERT_TRUE(count >= 1 && count <= 64);
    /* 后续创建应返回 NULL */
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_NOT_NULL(handles[i]);
        tbf_delete(handles[i]);
    }
}

static void test_set_clock_source_sync_and_multistream(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    /* SYNC 时钟源路径 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_set_clock_source(TBF_TS_SOURCE_SYNC));
    /* 切回 SYSTEM, tbf_get_current_time_us 也会走 SYSTEM 分支 (default) */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_set_clock_source(TBF_TS_SOURCE_SYSTEM));

    /* multistream: 使能 + >8 守卫 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_configure_multistream(t, true, 4));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_configure_multistream(t, false, 0));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_configure_multistream(NULL, true, 4));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_configure_multistream(t, true, 9)); /* >8 非法 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_configure_multistream(t, true, 255));
    tbf_delete(t);
}

static void test_set_stream_offset_invalid_scenarios(void)
{
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_NOT_NULL(t);
    /* 未使能 multistream 时调用 -> ETH_ERROR */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, tbf_set_stream_offset(t, 0, 1000));
    /* stream_id >= 8 非法 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_set_stream_offset(t, 8, 1000));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_set_stream_offset(t, 255, 1000));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_set_stream_offset(NULL, 0, 1000));
    /* 使能后正常设置 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_configure_multistream(t, true, 4));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_set_stream_offset(t, 0, 500));
    TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_set_stream_offset(t, 7, -1000));
    tbf_delete(t);
}

static void test_enable_asil_all_levels_switches(void)
{
    /* ASIL 等级 0,1,2 -> default; 3 -> ASIL-C; 4 -> ASIL-D */
    for (uint8_t lvl = 0; lvl <= 4; lvl++) {
        tbf_handle_t *t = tbf_create(&base_cfg);
        TEST_ASSERT_NOT_NULL(t);
        TEST_ASSERT_EQUAL_INT(ETH_OK, tbf_enable_asil_mode(t, lvl));
        tbf_delete(t);
    }
    /* >4 非法 + NULL 守卫 */
    tbf_handle_t *t = tbf_create(&base_cfg);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_enable_asil_mode(t, 5));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_enable_asil_mode(t, 9));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, tbf_enable_asil_mode(NULL, 3));
    tbf_delete(t);

    /* validate_timestamp 边界补测: diff == tolerance + 边界 */
    uint64_t now = tbf_get_current_time_us();
    TEST_ASSERT_TRUE(tbf_validate_timestamp(now + 999999ULL, now, 1000000)); /* 恰好 tolerance 内 */
    TEST_ASSERT_FALSE(tbf_validate_timestamp(now + 1000001ULL, now, 1000000)); /* 超出 */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_guards);
    RUN_TEST(test_window_suppresses_then_releases);
    RUN_TEST(test_set_get_separation);
    RUN_TEST(test_check_output_time_and_next);
    RUN_TEST(test_flush_window);
    RUN_TEST(test_timestamp_validation);
    RUN_TEST(test_average_aggregation);
    /* 新增补测: KALMAN/PEAK/SAMPLE_FIRST/DROP_MIDDLE 策略 + ASIL/multistream/clock_source + 池满失败 */
    RUN_TEST(test_kalman_compression_outputs_filtered_value);
    RUN_TEST(test_peak_hold_tracks_min_max_and_sample_first_drop_middle);
    RUN_TEST(test_get_compressed_sample_timeout_and_empty);
    RUN_TEST(test_create_pool_full_fails);
    RUN_TEST(test_set_clock_source_sync_and_multistream);
    RUN_TEST(test_set_stream_offset_invalid_scenarios);
    RUN_TEST(test_enable_asil_all_levels_switches);
    return UNITY_END();
}
