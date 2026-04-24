/**
 * @file test_tsn_stack.c
 * @brief TSN协议栈测试
 * @version 1.0
 * @date 2026-04-24
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../tsn_stack.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

/* ============================================================================
 * gPTP测试
 * ============================================================================ */
void test_gptp(void) {
    printf("\n=== Testing gPTP (IEEE 802.1AS) ===\n");
    
    gptp_config_t config = {
        .domain_count = 1,
        .port_count = 2,
        .gm_capable = true,
        .sync_precision_ns = 100,
        .enable_asil_d = true
    };
    
    eth_status_t status = gptp_init(&config);
    TEST_ASSERT(status == ETH_OK, "gPTP initialization");
    
    /* 配置端口 */
    gptp_port_config_t port_config = {
        .port_id = 0,
        .log_sync_interval = -3,  /* 125ms */
        .log_pdelay_req_interval = 0,  /* 1s */
        .log_announce_interval = 0,    /* 1s */
        .enable_safety_monitoring = true
    };
    status = gptp_config_port(0, &port_config);
    TEST_ASSERT(status == ETH_OK, "gPTP port configuration");
    
    /* 配置域 */
    gptp_domain_config_t domain_config = {
        .domain_number = 0,
        .priority1 = 248,
        .priority2 = 248,
        .clock_quality = {
            .clock_class = 6,
            .clock_accuracy = 0x20,
            .offset_scaled_log_variance = 0xFFFF
        }
    };
    memcpy(domain_config.clock_identity, "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
    status = gptp_config_domain(0, &domain_config);
    TEST_ASSERT(status == ETH_OK, "gPTP domain configuration");
    
    /* 启动gPTP */
    status = gptp_start(0);
    TEST_ASSERT(status == ETH_OK, "gPTP start");
    
    /* 检查状态 */
    gptp_port_state_t port_state;
    status = gptp_get_port_state(0, &port_state);
    TEST_ASSERT(status == ETH_OK && port_state == GPTP_PORT_STATE_MASTER, 
                "gPTP port state (as GM should be MASTER)");
    
    /* 获取时间 */
    gptp_timestamp_t timestamp;
    status = gptp_get_time(0, &timestamp);
    TEST_ASSERT(status == ETH_OK && timestamp.seconds > 0, "gPTP get time");
    
    /* 安全监控 */
    status = gptp_init_safety_monitor(0);
    TEST_ASSERT(status == ETH_OK, "gPTP safety monitor init");
    
    status = gptp_run_safety_checks(0);
    TEST_ASSERT(status == ETH_OK, "gPTP safety checks");
    
    /* 停止 */
    gptp_stop(0);
    gptp_deinit();
    
    printf("gPTP tests completed\n");
}

/* ============================================================================
 * TAS测试
 * ============================================================================ */
void test_tas(void) {
    printf("\n=== Testing TAS (IEEE 802.1Qbv) ===\n");
    
    eth_status_t status = tas_init();
    TEST_ASSERT(status == ETH_OK, "TAS initialization");
    
    /* 创建GCL */
    tas_gcl_config_t gcl_config;
    status = tas_create_automotive_gcl(&gcl_config, 1);  /* 1ms循环 */
    TEST_ASSERT(status == ETH_OK, "TAS create automotive GCL");
    TEST_ASSERT(gcl_config.entry_count == 3, "TAS GCL entry count");
    
    /* 配置端口 */
    tas_port_config_t port_config = {
        .port_id = 0,
        .gate_enabled = true,
        .gcl_config = gcl_config,
        .queue_count = 8,
        .hold_advance_us = 100,
        .release_advance_us = 100,
        .enable_safety_checks = true
    };
    
    for (int i = 0; i < 8; i++) {
        port_config.queue_configs[i].queue_id = i;
        port_config.queue_configs[i].priority = i;
        port_config.queue_configs[i].max_sdu_bytes = 1522;
    }
    
    status = tas_config_port(0, &port_config);
    TEST_ASSERT(status == ETH_OK, "TAS port configuration");
    
    /* 验证GCL */
    bool valid;
    status = tas_validate_gcl(&gcl_config, &valid);
    TEST_ASSERT(status == ETH_OK && valid, "TAS validate GCL");
    
    /* 启动调度器 */
    status = tas_start_scheduler(0);
    TEST_ASSERT(status == ETH_OK, "TAS start scheduler");
    
    /* 运行调度循环 */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t current_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    status = tas_run_schedule_cycle(0, current_time);
    TEST_ASSERT(status == ETH_OK, "TAS run schedule cycle");
    
    /* 检查门状态 */
    tas_gate_status_t gate_status;
    status = tas_get_gate_status(0, &gate_status);
    TEST_ASSERT(status == ETH_OK, "TAS get gate status");
    
    /* 检查是否可传输 */
    bool can_tx;
    status = tas_can_transmit(0, 0, 100, &can_tx);
    TEST_ASSERT(status == ETH_OK, "TAS can transmit check");
    
    /* 安全监控 */
    status = tas_init_safety_monitor(0);
    TEST_ASSERT(status == ETH_OK, "TAS safety monitor init");
    
    /* 停止 */
    tas_stop_scheduler(0);
    tas_deinit();
    
    printf("TAS tests completed\n");
}

/* ============================================================================
 * CBS测试
 * ============================================================================ */
void test_cbs(void) {
    printf("\n=== Testing CBS (IEEE 802.1Qav) ===\n");
    
    eth_status_t status = cbs_init();
    TEST_ASSERT(status == ETH_OK, "CBS initialization");
    
    /* 创建车载配置 */
    cbs_port_config_t port_config;
    status = cbs_create_automotive_config(0, 1000, &port_config);  /* 1Gbps */
    TEST_ASSERT(status == ETH_OK, "CBS create automotive config");
    TEST_ASSERT(port_config.sr_class_count == 2, "CBS SR class count");
    
    /* 配置端口 */
    status = cbs_config_port(0, &port_config);
    TEST_ASSERT(status == ETH_OK, "CBS port configuration");
    
    /* 检查Idle Slope计算 */
    int64_t idle_slope;
    status = cbs_calc_idle_slope(1000000000, 25, &idle_slope);
    TEST_ASSERT(status == ETH_OK && idle_slope == 250000000, "CBS calc idle slope");
    
    /* 检查Send Slope计算 */
    int64_t send_slope;
    status = cbs_calc_send_slope(1000000000, idle_slope, &send_slope);
    TEST_ASSERT(status == ETH_OK && send_slope == -750000000, "CBS calc send slope");
    
    /* 检查传输许可 (初始状态应该允许) */
    bool can_tx;
    status = cbs_can_transmit(0, CBS_SR_CLASS_A, 300, &can_tx);
    TEST_ASSERT(status == ETH_OK, "CBS can transmit check");
    
    /* 模拟传输 */
    status = cbs_start_transmission(0, CBS_SR_CLASS_A, 300);
    TEST_ASSERT(status == ETH_OK, "CBS start transmission");
    
    /* 更新信用 */
    status = cbs_update_credit(0, CBS_SR_CLASS_A, 1000000);  /* 1ms */
    TEST_ASSERT(status == ETH_OK, "CBS update credit");
    
    /* 获取当前信用 */
    int64_t credit;
    status = cbs_get_credit(0, CBS_SR_CLASS_A, &credit);
    TEST_ASSERT(status == ETH_OK, "CBS get credit");
    
    /* 完成传输 */
    status = cbs_complete_transmission(0, CBS_SR_CLASS_A, 300, 2400);  /* 2.4us @ 1Gbps */
    TEST_ASSERT(status == ETH_OK, "CBS complete transmission");
    
    /* 获取统计 */
    cbs_stats_t stats;
    status = cbs_get_stats(0, &stats);
    TEST_ASSERT(status == ETH_OK, "CBS get stats");
    
    /* 安全监控 */
    status = cbs_init_safety_monitor(0);
    TEST_ASSERT(status == ETH_OK, "CBS safety monitor init");
    
    cbs_deinit();
    
    printf("CBS tests completed\n");
}

/* ============================================================================
 * 帧抢占测试
 * ============================================================================ */
void test_frame_preemption(void) {
    printf("\n=== Testing Frame Preemption (IEEE 802.1Qbu) ===\n");
    
    eth_status_t status = fp_init();
    TEST_ASSERT(status == ETH_OK, "Frame Preemption initialization");
    
    /* 配置端口 */
    fp_config_t config = {
        .port_id = 0,
        .preemption_enabled = true,
        .express_preemption_enabled = true,
        .min_frag_size = 64,
        .max_frag_size = 512,
        .hold_advance_ns = 100,
        .release_advance_ns = 100,
        .express_priority_mask = 0x01,      /* 优先级0为快速帧 */
        .preemptable_priority_mask = 0xFE,  /* 其他为可抢占帧 */
        .enable_safety_checks = true,
        .max_preemptions_per_frame = 16,
        .reassembly_timeout_us = 1000
    };
    
    status = fp_config_port(0, &config);
    TEST_ASSERT(status == ETH_OK, "Frame Preemption port configuration");
    
    /* 启用抢占 */
    status = fp_enable_preemption(0, true);
    TEST_ASSERT(status == ETH_OK, "Frame Preemption enable");
    
    /* 检查帧类型 */
    bool is_express = fp_is_express_frame(0, &config);
    TEST_ASSERT(is_express == true, "Check express frame priority 0");
    
    bool is_preemptable = fp_is_preemptable_frame(1, &config);
    TEST_ASSERT(is_preemptable == true, "Check preemptable frame priority 1");
    
    /* 测试分片 */
    uint8_t test_frame[1000];
    memset(test_frame, 0xAA, sizeof(test_frame));
    
    fp_mpacket_t mpackets[4];
    uint32_t mpacket_count = 4;
    
    status = fp_fragment_frame(test_frame, sizeof(test_frame), 256, 
                               mpackets, &mpacket_count);
    TEST_ASSERT(status == ETH_OK, "Frame fragmentation");
    TEST_ASSERT(mpacket_count == 4, "Fragment count");
    
    /* 检查第一个mPacket */
    TEST_ASSERT(mpackets[0].frag_type == FP_FRAME_FIRST_FRAGMENT, 
                "First fragment type");
    TEST_ASSERT(mpackets[3].frag_type == FP_FRAME_LAST_FRAGMENT, 
                "Last fragment type");
    
    /* 重组测试 */
    uint8_t reassembled[1000];
    uint32_t reassembled_len;
    bool is_complete;
    
    status = fp_reassemble_mpacket(&mpackets[3], reassembled, &reassembled_len, &is_complete);
    TEST_ASSERT(status == ETH_OK, "Reassemble mPacket");
    TEST_ASSERT(is_complete == true, "Last fragment completes frame");
    
    /* 安全监控 */
    status = fp_init_safety_monitor(0);
    TEST_ASSERT(status == ETH_OK, "Frame Preemption safety monitor init");
    
    fp_deinit();
    
    printf("Frame Preemption tests completed\n");
}

/* ============================================================================
 * 流预留测试
 * ============================================================================ */
void test_stream_reservation(void) {
    printf("\n=== Testing Stream Reservation (IEEE 802.1Qcc) ===\n");
    
    eth_status_t status = srp_init();
    TEST_ASSERT(status == ETH_OK, "Stream Reservation initialization");
    
    /* 配置端口带宽 */
    status = srp_config_port_bandwidth(0, 1000000000);  /* 1Gbps */
    TEST_ASSERT(status == ETH_OK, "SRP port bandwidth configuration");
    
    /* 创建流 */
    srp_stream_id_t stream_id;
    status = srp_create_automotive_stream("ADAS_Camera_1", 100, 3, stream_id);
    TEST_ASSERT(status == ETH_OK, "Create automotive stream");
    
    /* 创建talker声明 */
    srp_talker_declaration_t talker = {
        .dest_address = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01},
        .data_frame_spec = {
            .vlan_id = 100,
            .priority = 3,
            .rank = 0
        },
        .tspec = {
            .interval_numerator = 1,
            .interval_denominator = 8000,  /* 125us */
            .max_frame_size = 300,
            .max_interval_frames = 1,
            .preemption_enabled = false
        },
        .failed = false
    };
    memcpy(talker.stream_id, stream_id, SRP_STREAM_ID_LEN);
    
    /* 注册Talker */
    status = srp_register_talker(0, &talker);
    TEST_ASSERT(status == ETH_OK, "Register talker");
    
    /* 计算带宽 */
    uint32_t bandwidth;
    status = srp_calc_bandwidth_requirement(&talker.tspec, &bandwidth);
    TEST_ASSERT(status == ETH_OK && bandwidth > 0, "Calculate bandwidth requirement");
    
    /* 创建listener声明 */
    srp_listener_declaration_t listener = {
        .state = SRP_LISTENER_STATE_READY
    };
    memcpy(listener.stream_id, stream_id, SRP_STREAM_ID_LEN);
    
    status = srp_register_listener(0, &listener);
    TEST_ASSERT(status == ETH_OK, "Register listener");
    
    /* 获取流预留状态 */
    srp_stream_reservation_t reservation;
    status = srp_get_stream_reservation(stream_id, &reservation);
    TEST_ASSERT(status == ETH_OK, "Get stream reservation");
    TEST_ASSERT(reservation.talker_registered == true, "Talker is registered");
    TEST_ASSERT(reservation.listener_count == 1, "Listener count");
    
    /* 检查可行性 */
    bool feasible;
    status = srp_check_feasibility(0, 10000000, 500, &feasible);
    TEST_ASSERT(status == ETH_OK && feasible == true, "Check feasibility");
    
    /* 获取端口带宽状态 */
    srp_port_bandwidth_t port_bw;
    status = srp_get_port_bandwidth(0, &port_bw);
    TEST_ASSERT(status == ETH_OK, "Get port bandwidth");
    TEST_ASSERT(port_bw.reserved_bandwidth_bps > 0, "Bandwidth is reserved");
    
    /* 安全监控 */
    status = srp_init_safety_monitor(0);
    TEST_ASSERT(status == ETH_OK, "SRP safety monitor init");
    
    srp_deinit();
    
    printf("Stream Reservation tests completed\n");
}

/* ============================================================================
 * TSN整合测试
 * ============================================================================ */
void test_tsn_stack_integration(void) {
    printf("\n=== Testing TSN Stack Integration ===\n");
    
    /* 创建配置 */
    tsn_stack_config_t config;
    eth_status_t status = tsn_create_automotive_config(1000, &config);  /* 1Gbps */
    TEST_ASSERT(status == ETH_OK, "Create automotive config");
    
    /* 初始化TSN栈 */
    status = tsn_stack_init(&config);
    TEST_ASSERT(status == ETH_OK, "TSN stack initialization");
    
    /* 检查版本 */
    const char *version = tsn_stack_get_version_string();
    TEST_ASSERT(version != NULL && strlen(version) > 0, "Get version string");
    
    uint8_t major, minor, patch;
    tsn_stack_get_version(&major, &minor, &patch);
    TEST_ASSERT(major == 1 && minor == 0 && patch == 0, "Version numbers");
    
    /* 检查状态 */
    tsn_stack_status_t stack_status;
    status = tsn_stack_get_status(&stack_status);
    TEST_ASSERT(status == ETH_OK, "Get stack status");
    TEST_ASSERT(stack_status.state == TSN_STATE_INIT, "Stack state is INIT");
    TEST_ASSERT(stack_status.gptp_active == true, "gPTP is active");
    TEST_ASSERT(stack_status.tas_active == true, "TAS is active");
    TEST_ASSERT(stack_status.cbs_active == true, "CBS is active");
    TEST_ASSERT(stack_status.fp_active == true, "Frame Preemption is active");
    TEST_ASSERT(stack_status.srp_active == true, "SRP is active");
    
    /* 启动TSN栈 */
    status = tsn_stack_start();
    TEST_ASSERT(status == ETH_OK, "TSN stack start");
    
    status = tsn_stack_get_status(&stack_status);
    TEST_ASSERT(stack_status.state == TSN_STATE_RUNNING, "Stack state is RUNNING");
    
    /* 运行几个调度循环 */
    for (int i = 0; i < 10; i++) {
        status = tsn_stack_run_cycle();
        TEST_ASSERT(status == ETH_OK, "TSN stack run cycle");
        usleep(1000);  /* 1ms */
    }
    
    /* 获取统计 */
    tsn_stack_stats_t stats;
    status = tsn_stack_get_stats(&stats);
    TEST_ASSERT(status == ETH_OK, "Get stack stats");
    
    /* 打印状态 */
    status = tsn_stack_print_status();
    TEST_ASSERT(status == ETH_OK, "Print stack status");
    
    /* 清除统计 */
    status = tsn_stack_clear_stats();
    TEST_ASSERT(status == ETH_OK, "Clear stack stats");
    
    /* 停止 */
    status = tsn_stack_stop();
    TEST_ASSERT(status == ETH_OK, "TSN stack stop");
    
    tsn_stack_deinit();
    
    printf("TSN Stack Integration tests completed\n");
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("****************************************\n");
    printf("*       TSN Stack Test Suite           *\n");
    printf("*   IEEE 802.1AS/Qbv/Qbu/Qav/Qcc      *\n");
    printf("****************************************\n");
    printf("\n");
    
    /* 运行所有测试 */
    test_gptp();
    test_tas();
    test_cbs();
    test_frame_preemption();
    test_stream_reservation();
    test_tsn_stack_integration();
    
    /* 打印测试结果 */
    printf("\n========================================\n");
    printf("       Test Results Summary\n");
    printf("========================================\n");
    printf("Total Tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("Success Rate: %.1f%%\n", 
           tests_run > 0 ? (float)tests_passed / tests_run * 100 : 0);
    printf("========================================\n\n");
    
    return tests_failed > 0 ? 1 : 0;
}
