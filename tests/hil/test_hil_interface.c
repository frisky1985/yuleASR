/*
 * HIL Interface Unit Tests
 * Simple test runner for C HIL interface
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hil_interface.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n  Assertion failed: %s\n", #cond); \
        tests_run++; \
        return; \
    } \
} while(0)

/* Test: Initialization */
TEST(init_deinit) {
    hil_error_t result = hil_init(true);  /* Simulation mode */
    ASSERT(result == HIL_OK);
    ASSERT(hil_is_initialized() == true);
    ASSERT(hil_is_simulation_mode() == true);
    
    hil_deinit();
    ASSERT(hil_is_initialized() == false);
}

/* Test: CAN Interface Simulation */
TEST(can_simulation) {
    hil_error_t result = hil_init(true);
    ASSERT(result == HIL_OK);
    
    hil_can_config_t config = {
        .bitrate = HIL_CAN_BITRATE_500K,
        .mode = HIL_CAN_MODE_STANDARD,
        .fd_enabled = false,
        .tx_id = 0x123,
        .rx_id = 0x456
    };
    
    hil_interface_t *iface = hil_can_open(HIL_IF_TYPE_SIMULATION, 0, &config);
    ASSERT(iface != NULL);
    ASSERT(iface->is_open == true);
    
    /* Send a frame */
    hil_can_frame_t frame = {
        .id = 0x123,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
        .length = 8,
        .is_extended = false,
        .is_fd = false
    };
    
    result = hil_can_send(iface, &frame);
    ASSERT(result == HIL_OK);
    
    /* Close */
    result = hil_can_close(iface);
    ASSERT(result == HIL_OK);
    
    hil_deinit();
}

/* Test: Ethernet Interface Simulation */
TEST(eth_simulation) {
    hil_error_t result = hil_init(true);
    ASSERT(result == HIL_OK);
    
    hil_eth_config_t config = {
        .local_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .ethertype = 0x13400,  /* DoIP */
        .interface_name = "eth0",
        .promiscuous_mode = false
    };
    
    hil_interface_t *iface = hil_eth_open(HIL_IF_TYPE_SIMULATION, &config);
    ASSERT(iface != NULL);
    ASSERT(iface->is_open == true);
    
    /* Send a frame */
    hil_eth_frame_t frame = {
        .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .src_mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .ethertype = 0x13400,
        .payload = {0x01, 0x02, 0x03, 0x04},
        .payload_len = 4
    };
    
    result = hil_eth_send(iface, &frame);
    ASSERT(result == HIL_OK);
    
    /* Close */
    result = hil_eth_close(iface);
    ASSERT(result == HIL_OK);
    
    hil_deinit();
}

/* Test: Frame Injection in Simulation Mode */
TEST(frame_injection) {
    hil_error_t result = hil_init(true);
    ASSERT(result == HIL_OK);
    
    hil_can_config_t config = {
        .bitrate = HIL_CAN_BITRATE_500K,
        .mode = HIL_CAN_MODE_STANDARD
    };
    
    hil_interface_t *iface = hil_can_open(HIL_IF_TYPE_SIMULATION, 0, &config);
    ASSERT(iface != NULL);
    
    /* Inject a frame */
    hil_can_frame_t rx_frame = {
        .id = 0x456,
        .data = {0xAA, 0xBB, 0xCC, 0xDD},
        .length = 4
    };
    
    result = hil_sim_inject_can_frame(0, &rx_frame);
    ASSERT(result == HIL_OK);
    
    /* Receive the injected frame */
    hil_can_frame_t received;
    result = hil_can_receive(iface, &received, 1000);
    ASSERT(result == HIL_OK);
    ASSERT(received.id == 0x456);
    ASSERT(received.length == 4);
    
    hil_can_close(iface);
    hil_deinit();
}

/* Test: Error Handling */
TEST(error_handling) {
    /* Test error strings */
    const char *str = hil_error_string(HIL_OK);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    
    str = hil_error_string(HIL_ERROR_TIMEOUT);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    
    /* Test interface type strings */
    str = hil_interface_type_string(HIL_IF_TYPE_CAN_PCAN);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    
    str = hil_interface_type_string(HIL_IF_TYPE_SIMULATION);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
}

/* Test: Statistics */
TEST(statistics) {
    hil_error_t result = hil_init(true);
    ASSERT(result == HIL_OK);
    
    /* Reset stats */
    hil_reset_stats();
    
    uint64_t tx_frames, rx_frames, tx_bytes, rx_bytes, errors;
    hil_get_stats(&tx_frames, &rx_frames, &tx_bytes, &rx_bytes, &errors);
    
    ASSERT(tx_frames == 0);
    ASSERT(rx_frames == 0);
    
    hil_deinit();
}

/* Test: Timestamp and Delay */
TEST(timing) {
    uint64_t ts1 = hil_get_timestamp_us();
    hil_delay_ms(10);
    uint64_t ts2 = hil_get_timestamp_us();
    
    /* Should be at least 10ms difference */
    ASSERT(ts2 > ts1);
    ASSERT((ts2 - ts1) >= 10000);  /* 10ms in microseconds */
}

int main(void) {
    printf("HIL Interface Unit Tests\n");
    printf("========================\n\n");
    
    RUN_TEST(init_deinit);
    RUN_TEST(can_simulation);
    RUN_TEST(eth_simulation);
    RUN_TEST(frame_injection);
    RUN_TEST(error_handling);
    RUN_TEST(statistics);
    RUN_TEST(timing);
    
    printf("\n========================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("========================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
