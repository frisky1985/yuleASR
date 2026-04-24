/**
 * @file test_transport.c
 * @brief 传输层测试程序
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "udp_transport.h"
#include "shm_transport.h"
#include "transport_manager.h"

/* 测试UDP传输 */
void test_udp_transport(void)
{
    printf("Testing UDP transport...\n");
    
    udp_transport_config_t config = {0};
    config.base.local_addr = 0;
    config.base.local_port = 7400;
    config.base.remote_addr = 0x7F000001; /* 127.0.0.1 */
    config.base.remote_port = 7401;
    config.eth_speed = AUTOMOTIVE_ETH_100BASE_T1;
    config.mode = UDP_MODE_UNICAST;
    config.multicast_ttl = 1;
    config.deadline_us = 1000;
    config.offset_us = 100;
    config.enable_deadline = true;
    
    transport_handle_t *handle = udp_transport_create(&config);
    assert(handle != NULL);
    printf("  UDP transport created successfully\n");
    
    /* 测试设置TSN - 可能需要特权，忽略错误 */
    tsn_config_t tsn = {
        .enable_tsn = true,
        .traffic_class = TSN_TC_CRITICAL,
        .time_slot_us = 500,
        .bandwidth_reservation = 80,
        .max_latency_us = 500
    };
    eth_status_t status = udp_transport_set_tsn_shaping(handle, TSN_TC_CRITICAL, &tsn);
    if (status == ETH_OK) {
        printf("  TSN shaping configured\n");
    } else {
        printf("  TSN shaping not available (requires privileges)\n");
    }
    
    /* 测试统计 */
    udp_transport_stats_t stats;
    status = udp_transport_get_stats(handle, &stats);
    assert(status == ETH_OK);
    printf("  Stats: tx_packets=%lu\n", stats.tx_packets);
    
    /* 测试销毁 */
    status = udp_transport_destroy(handle);
    assert(status == ETH_OK);
    printf("  UDP transport destroyed\n");
    
    printf("UDP transport test PASSED\n\n");
}

/* 测试共享内存传输 */
void test_shm_transport(void)
{
    printf("Testing Shared Memory transport...\n");
    
    shm_transport_config_t config = {0};
    strncpy(config.shm_name, "/dds_test_shm", sizeof(config.shm_name) - 1);
    config.ring_buffer_size = 1024 * 1024; /* 1MB */
    config.max_channels = 4;
    config.det_mode = SHM_MODE_LOCK_FREE;
    config.sync_type = SHM_SYNC_SPINLOCK;
    config.allow_multi_producer = true;
    config.allow_multi_consumer = true;
    config.enable_deadline = false;
    
    transport_handle_t *handle = shm_transport_create(&config);
    assert(handle != NULL);
    printf("  SHM transport created successfully\n");
    
    /* 测试发送 */
    const char *test_data = "Hello SHM Transport!";
    eth_status_t status = shm_transport_send_zero_copy(
        handle, (void *)test_data, strlen(test_data), SHM_PRIO_HIGH, 1000);
    assert(status == ETH_OK);
    printf("  Data sent via SHM\n");
    
    /* 测试缓冲区状态 */
    uint32_t free_space, used_space;
    status = shm_transport_get_buffer_status(handle, &free_space, &used_space);
    assert(status == ETH_OK);
    printf("  Buffer status: free=%u, used=%u\n", free_space, used_space);
    
    /* 测试统计 */
    shm_transport_stats_t stats;
    status = shm_transport_get_stats(handle, &stats);
    assert(status == ETH_OK);
    printf("  Stats: tx_messages=%lu, rx_messages=%lu\n", 
           stats.tx_messages, stats.rx_messages);
    
    /* 测试销毁 */
    status = shm_transport_destroy(handle);
    assert(status == ETH_OK);
    printf("  SHM transport destroyed\n");
    
    /* 清理共享内存对象 */
    shm_unlink("/dds_test_shm");
    
    printf("SHM transport test PASSED\n\n");
}

/* 测试传输管理器 */
void test_transport_manager(void)
{
    printf("Testing Transport Manager...\n");
    
    /* 初始化管理器 */
    tm_config_t config = {
        .routing_policy = TM_ROUTE_AUTO,
        .shm_latency_threshold_us = 50,
        .udp_latency_threshold_us = 500,
        .health_check_interval_ms = 1000,
        .heartbeat_timeout_ms = 5000,
        .failover_threshold = 10
    };
    
    eth_status_t status = tm_init(&config);
    assert(status == ETH_OK);
    printf("  Transport manager initialized\n");
    
    /* 注册本地端点 */
    tm_endpoint_entry_t local_ep = {
        .type = TM_EP_LOCAL_PROCESS,
        .node_id = 1,
        .is_local = true
    };
    uint32_t local_ep_id;
    status = tm_register_endpoint(&local_ep, &local_ep_id);
    assert(status == ETH_OK);
    printf("  Local endpoint registered: id=%u\n", local_ep_id);
    
    /* 注册远程端点 */
    tm_endpoint_entry_t remote_ep = {
        .type = TM_EP_REMOTE_NODE,
        .node_id = 2,
        .ip_addr = 0x7F000001,
        .port = 7500,
        .is_local = false
    };
    uint32_t remote_ep_id;
    status = tm_register_endpoint(&remote_ep, &remote_ep_id);
    assert(status == ETH_OK);
    printf("  Remote endpoint registered: id=%u\n", remote_ep_id);
    
    /* 注册共享内存传输层 */
    shm_transport_config_t shm_config = {0};
    strncpy(shm_config.shm_name, "/dds_manager_shm", sizeof(shm_config.shm_name) - 1);
    shm_config.ring_buffer_size = 512 * 1024;
    
    uint32_t shm_transport_id;
    status = tm_register_transport(TRANSPORT_SHM, &shm_config, &shm_transport_id);
    if (status == ETH_OK) {
        printf("  SHM transport registered: id=%u\n", shm_transport_id);
    }
    
    /* 注册UDP传输层 */
    udp_transport_config_t udp_config = {0};
    udp_config.base.local_addr = 0;
    udp_config.base.local_port = 7600;
    udp_config.base.remote_addr = 0x7F000001;
    udp_config.base.remote_port = 7601;
    
    uint32_t udp_transport_id;
    status = tm_register_transport(TRANSPORT_UDP, &udp_config, &udp_transport_id);
    if (status == ETH_OK) {
        printf("  UDP transport registered: id=%u\n", udp_transport_id);
    }
    
    /* 创建路由 */
    uint32_t route_id;
    status = tm_create_route(local_ep_id, remote_ep_id, TM_ROUTE_AUTO, &route_id);
    assert(status == ETH_OK);
    printf("  Route created: id=%u\n", route_id);
    
    /* 获取推荐传输类型 */
    transport_type_t recommended;
    status = tm_get_recommended_transport(local_ep_id, remote_ep_id, &recommended);
    assert(status == ETH_OK);
    printf("  Recommended transport: %s\n", 
           recommended == TRANSPORT_SHM ? "SHM" : "UDP");
    
    /* 获取统计 */
    tm_stats_t stats;
    status = tm_get_stats(&stats);
    assert(status == ETH_OK);
    printf("  Manager stats: transports=%u, endpoints=%u, routes=%u\n",
           stats.active_transports, stats.active_endpoints, stats.active_routes);
    
    /* 反初始化 */
    status = tm_deinit();
    assert(status == ETH_OK);
    printf("  Transport manager deinitialized\n");
    
    /* 清理 */
    shm_unlink("/dds_manager_shm");
    
    printf("Transport Manager test PASSED\n\n");
}

int main(void)
{
    printf("========================================\n");
    printf("DDS Transport Layer Test Suite\n");
    printf("========================================\n\n");
    
    test_udp_transport();
    test_shm_transport();
    test_transport_manager();
    
    printf("========================================\n");
    printf("All tests PASSED!\n");
    printf("========================================\n");
    
    return 0;
}
