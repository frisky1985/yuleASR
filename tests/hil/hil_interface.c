/******************************************************************************
 * @file    hil_interface.c
 * @brief   HIL (Hardware-in-the-Loop) Hardware Abstraction Layer Implementation
 *
 * 硬件在环测试框架 - C侧实现
 * 支持POSIX环境模拟运行 (HIL_SIMULATION_MODE)
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#include "hil_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* Platform specific includes */
#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <linux/can.h>
    #include <linux/can/raw.h>
    #include <arpa/inet.h>
    #include <netpacket/packet.h>
#endif

/******************************************************************************
 * Simulation Mode Data Structures
 ******************************************************************************/

typedef struct {
    hil_can_frame_t rx_buffer[256];
    uint32_t rx_head;
    uint32_t rx_tail;
    
    hil_can_frame_t tx_buffer[256];
    uint32_t tx_head;
    uint32_t tx_tail;
    
    uint32_t latency_ms;
    float packet_loss_rate;
} hil_sim_can_data_t;

typedef struct {
    hil_eth_frame_t rx_buffer[256];
    uint32_t rx_head;
    uint32_t rx_tail;
    
    hil_eth_frame_t tx_buffer[256];
    uint32_t tx_head;
    uint32_t tx_tail;
    
    uint32_t latency_ms;
    float packet_loss_rate;
} hil_sim_eth_data_t;

/******************************************************************************
 * Global Context
 ******************************************************************************/
static hil_context_t g_hil_ctx = {0};
static uint32_t g_handle_counter = 1;

/******************************************************************************
 * Static Function Prototypes
 ******************************************************************************/
static hil_error_t hil_sim_init_can(hil_interface_t *iface);
static hil_error_t hil_sim_init_eth(hil_interface_t *iface);
static hil_error_t hil_sim_can_send(hil_interface_t *iface, const hil_can_frame_t *frame);
static hil_error_t hil_sim_can_receive(hil_interface_t *iface, hil_can_frame_t *frame, uint32_t timeout_ms);
static hil_error_t hil_sim_eth_send(hil_interface_t *iface, const hil_eth_frame_t *frame);
static hil_error_t hil_sim_eth_receive(hil_interface_t *iface, hil_eth_frame_t *frame, uint32_t timeout_ms);

#ifndef _WIN32
static hil_error_t hil_socketcan_open(hil_interface_t *iface, uint32_t channel, const hil_can_config_t *config);
static hil_error_t hil_raw_socket_open(hil_interface_t *iface, const hil_eth_config_t *config);
#endif

/******************************************************************************
 * Initialization / Deinitialization
 ******************************************************************************/

hil_error_t hil_init(bool simulation_mode) {
    if (g_hil_ctx.initialized) {
        return HIL_OK;  /* Already initialized */
    }
    
    memset(&g_hil_ctx, 0, sizeof(g_hil_ctx));
    g_hil_ctx.simulation_mode = simulation_mode;
    g_hil_ctx.initialized = true;
    
    printf("[HIL] Framework initialized (mode: %s)\n", 
           simulation_mode ? "SIMULATION" : "HARDWARE");
    
    return HIL_OK;
}

void hil_deinit(void) {
    if (!g_hil_ctx.initialized) {
        return;
    }
    
    /* Close all open interfaces */
    for (uint32_t i = 0; i < g_hil_ctx.num_interfaces; i++) {
        if (g_hil_ctx.interfaces[i].is_open) {
            switch (g_hil_ctx.interfaces[i].type) {
                case HIL_IF_TYPE_CAN_PCAN:
                case HIL_IF_TYPE_CAN_VECTOR:
                case HIL_IF_TYPE_CAN_KVASER:
                case HIL_IF_TYPE_CAN_SOCKETCAN:
                    hil_can_close(&g_hil_ctx.interfaces[i]);
                    break;
                case HIL_IF_TYPE_ETH_RAW_SOCKET:
                case HIL_IF_TYPE_ETH_PCAP:
                    hil_eth_close(&g_hil_ctx.interfaces[i]);
                    break;
                default:
                    break;
            }
        }
    }
    
    g_hil_ctx.initialized = false;
    g_hil_ctx.num_interfaces = 0;
    
    printf("[HIL] Framework deinitialized\n");
}

bool hil_is_initialized(void) {
    return g_hil_ctx.initialized;
}

bool hil_is_simulation_mode(void) {
    return g_hil_ctx.simulation_mode;
}

hil_context_t* hil_get_context(void) {
    return &g_hil_ctx;
}

/******************************************************************************
 * CAN Interface Functions
 ******************************************************************************/

hil_interface_t* hil_can_open(hil_interface_type_t type, uint32_t channel,
                               const hil_can_config_t *config) {
    if (!g_hil_ctx.initialized) {
        printf("[HIL] Error: Framework not initialized\n");
        return NULL;
    }
    
    if (g_hil_ctx.num_interfaces >= (HIL_MAX_CAN_INTERFACES + HIL_MAX_ETH_INTERFACES)) {
        printf("[HIL] Error: Maximum interfaces reached\n");
        return NULL;
    }
    
    if (config == NULL) {
        printf("[HIL] Error: NULL config\n");
        return NULL;
    }
    
    hil_interface_t *iface = &g_hil_ctx.interfaces[g_hil_ctx.num_interfaces];
    memset(iface, 0, sizeof(hil_interface_t));
    
    iface->handle_id = g_handle_counter++;
    iface->type = type;
    memcpy(&iface->config.can_config, config, sizeof(hil_can_config_t));
    
    hil_error_t result = HIL_ERROR_NOT_SUPPORTED;
    
    if (g_hil_ctx.simulation_mode) {
        /* Simulation mode - always succeeds */
        result = hil_sim_init_can(iface);
    } else {
        /* Hardware mode - platform specific */
        #ifndef _WIN32
        switch (type) {
            case HIL_IF_TYPE_CAN_SOCKETCAN:
                result = hil_socketcan_open(iface, channel, config);
                break;
            case HIL_IF_TYPE_CAN_PCAN:
            case HIL_IF_TYPE_CAN_VECTOR:
            case HIL_IF_TYPE_CAN_KVASER:
                printf("[HIL] Warning: %s requires vendor libraries, using simulation\n",
                       hil_interface_type_string(type));
                result = hil_sim_init_can(iface);
                break;
            default:
                result = HIL_ERROR_INVALID_PARAM;
                break;
        }
        #else
        printf("[HIL] Warning: CAN hardware not supported on Windows, using simulation\n");
        result = hil_sim_init_can(iface);
        #endif
    }
    
    if (result != HIL_OK) {
        printf("[HIL] Error: Failed to open CAN interface (error %d)\n", result);
        return NULL;
    }
    
    iface->initialized = true;
    iface->is_open = true;
    g_hil_ctx.num_interfaces++;
    
    printf("[HIL] CAN interface opened (handle=%lu, type=%s, channel=%u)\n",
           iface->handle_id, hil_interface_type_string(type), channel);
    
    return iface;
}

hil_error_t hil_can_close(hil_interface_t *iface) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    /* Free simulation data if any */
    if (iface->sim_data != NULL) {
        free(iface->sim_data);
        iface->sim_data = NULL;
    }
    
    iface->is_open = false;
    iface->initialized = false;
    
    printf("[HIL] CAN interface closed (handle=%lu)\n", iface->handle_id);
    
    return HIL_OK;
}

hil_error_t hil_can_send(hil_interface_t *iface, const hil_can_frame_t *frame) {
    if (iface == NULL || !iface->is_open || frame == NULL) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    if (g_hil_ctx.simulation_mode) {
        return hil_sim_can_send(iface, frame);
    }
    
    /* Hardware mode - would use actual CAN API */
    printf("[HIL] TX CAN ID=0x%03X, Len=%u\n", frame->id, frame->length);
    g_hil_ctx.tx_frames++;
    g_hil_ctx.tx_bytes += frame->length;
    
    return HIL_OK;
}

hil_error_t hil_can_receive(hil_interface_t *iface, hil_can_frame_t *frame,
                             uint32_t timeout_ms) {
    if (iface == NULL || !iface->is_open || frame == NULL) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    if (g_hil_ctx.simulation_mode) {
        return hil_sim_can_receive(iface, frame, timeout_ms);
    }
    
    /* Hardware mode - would use actual CAN API */
    return HIL_ERROR_NOT_SUPPORTED;
}

hil_error_t hil_can_set_rx_callback(hil_interface_t *iface,
                                     hil_can_rx_callback_t callback,
                                     void *user_data) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    iface->rx_callback.can_rx_cb = callback;
    iface->user_data = user_data;
    
    return HIL_OK;
}

hil_error_t hil_can_set_bitrate(hil_interface_t *iface, hil_can_bitrate_t bitrate) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    iface->config.can_config.bitrate = bitrate;
    
    printf("[HIL] CAN bitrate set to %u\n", (unsigned int)bitrate);
    
    return HIL_OK;
}

hil_error_t hil_can_flush(hil_interface_t *iface) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    /* In simulation mode, clear TX buffer */
    if (g_hil_ctx.simulation_mode && iface->sim_data != NULL) {
        hil_sim_can_data_t *sim = (hil_sim_can_data_t*)iface->sim_data;
        sim->tx_head = sim->tx_tail = 0;
    }
    
    return HIL_OK;
}

/******************************************************************************
 * Ethernet Interface Functions
 ******************************************************************************/

hil_interface_t* hil_eth_open(hil_interface_type_t type,
                               const hil_eth_config_t *config) {
    if (!g_hil_ctx.initialized) {
        printf("[HIL] Error: Framework not initialized\n");
        return NULL;
    }
    
    if (g_hil_ctx.num_interfaces >= (HIL_MAX_CAN_INTERFACES + HIL_MAX_ETH_INTERFACES)) {
        printf("[HIL] Error: Maximum interfaces reached\n");
        return NULL;
    }
    
    if (config == NULL) {
        printf("[HIL] Error: NULL config\n");
        return NULL;
    }
    
    hil_interface_t *iface = &g_hil_ctx.interfaces[g_hil_ctx.num_interfaces];
    memset(iface, 0, sizeof(hil_interface_t));
    
    iface->handle_id = g_handle_counter++;
    iface->type = type;
    memcpy(&iface->config.eth_config, config, sizeof(hil_eth_config_t));
    
    hil_error_t result = HIL_ERROR_NOT_SUPPORTED;
    
    if (g_hil_ctx.simulation_mode) {
        result = hil_sim_init_eth(iface);
    } else {
        #ifndef _WIN32
        switch (type) {
            case HIL_IF_TYPE_ETH_RAW_SOCKET:
                result = hil_raw_socket_open(iface, config);
                break;
            default:
                printf("[HIL] Warning: Ethernet type %s not supported, using simulation\n",
                       hil_interface_type_string(type));
                result = hil_sim_init_eth(iface);
                break;
        }
        #else
        printf("[HIL] Warning: Ethernet raw sockets not supported on Windows, using simulation\n");
        result = hil_sim_init_eth(iface);
        #endif
    }
    
    if (result != HIL_OK) {
        printf("[HIL] Error: Failed to open Ethernet interface (error %d)\n", result);
        return NULL;
    }
    
    iface->initialized = true;
    iface->is_open = true;
    g_hil_ctx.num_interfaces++;
    
    printf("[HIL] Ethernet interface opened (handle=%lu, type=%s)\n",
           iface->handle_id, hil_interface_type_string(type));
    
    return iface;
}

hil_error_t hil_eth_close(hil_interface_t *iface) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    if (iface->sim_data != NULL) {
        free(iface->sim_data);
        iface->sim_data = NULL;
    }
    
    iface->is_open = false;
    iface->initialized = false;
    
    printf("[HIL] Ethernet interface closed (handle=%lu)\n", iface->handle_id);
    
    return HIL_OK;
}

hil_error_t hil_eth_send(hil_interface_t *iface, const hil_eth_frame_t *frame) {
    if (iface == NULL || !iface->is_open || frame == NULL) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    if (g_hil_ctx.simulation_mode) {
        return hil_sim_eth_send(iface, frame);
    }
    
    /* Hardware mode */
    printf("[HIL] TX Ethernet frame, Len=%u, Ethertype=0x%04X\n",
           frame->payload_len, frame->ethertype);
    g_hil_ctx.tx_frames++;
    g_hil_ctx.tx_bytes += frame->payload_len + 14;  /* + header */
    
    return HIL_OK;
}

hil_error_t hil_eth_receive(hil_interface_t *iface, hil_eth_frame_t *frame,
                             uint32_t timeout_ms) {
    if (iface == NULL || !iface->is_open || frame == NULL) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    if (g_hil_ctx.simulation_mode) {
        return hil_sim_eth_receive(iface, frame, timeout_ms);
    }
    
    return HIL_ERROR_NOT_SUPPORTED;
}

hil_error_t hil_eth_set_rx_callback(hil_interface_t *iface,
                                     hil_eth_rx_callback_t callback,
                                     void *user_data) {
    if (iface == NULL || !iface->is_open) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    iface->rx_callback.eth_rx_cb = callback;
    iface->user_data = user_data;
    
    return HIL_OK;
}

/******************************************************************************
 * Simulation Mode Functions
 ******************************************************************************/

static hil_error_t hil_sim_init_can(hil_interface_t *iface) {
    hil_sim_can_data_t *sim = calloc(1, sizeof(hil_sim_can_data_t));
    if (sim == NULL) {
        return HIL_ERROR_NO_MEMORY;
    }
    
    sim->latency_ms = 0;
    sim->packet_loss_rate = 0.0f;
    
    iface->sim_data = sim;
    
    return HIL_OK;
}

static hil_error_t hil_sim_init_eth(hil_interface_t *iface) {
    hil_sim_eth_data_t *sim = calloc(1, sizeof(hil_sim_eth_data_t));
    if (sim == NULL) {
        return HIL_ERROR_NO_MEMORY;
    }
    
    sim->latency_ms = 0;
    sim->packet_loss_rate = 0.0f;
    
    iface->sim_data = sim;
    
    return HIL_OK;
}

static hil_error_t hil_sim_can_send(hil_interface_t *iface, const hil_can_frame_t *frame) {
    hil_sim_can_data_t *sim = (hil_sim_can_data_t*)iface->sim_data;
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    /* Simulate packet loss */
    if (sim->packet_loss_rate > 0.0f) {
        float rand_val = (float)rand() / (float)RAND_MAX;
        if (rand_val < sim->packet_loss_rate) {
            printf("[HIL] SIM: Packet lost (TX)\n");
            return HIL_OK;  /* Silent drop */
        }
    }
    
    /* Add to TX buffer */
    uint32_t next_head = (sim->tx_head + 1) % 256;
    if (next_head != sim->tx_tail) {
        memcpy(&sim->tx_buffer[sim->tx_head], frame, sizeof(hil_can_frame_t));
        sim->tx_buffer[sim->tx_head].timestamp_us = hil_get_timestamp_us();
        sim->tx_head = next_head;
    }
    
    g_hil_ctx.tx_frames++;
    g_hil_ctx.tx_bytes += frame->length;
    
    printf("[HIL] SIM TX CAN ID=0x%03X, Len=%u\n", frame->id, frame->length);
    
    return HIL_OK;
}

static hil_error_t hil_sim_can_receive(hil_interface_t *iface, hil_can_frame_t *frame,
                                        uint32_t timeout_ms) {
    hil_sim_can_data_t *sim = (hil_sim_can_data_t*)iface->sim_data;
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    uint64_t start_time = hil_get_timestamp_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;
    
    while (1) {
        if (sim->rx_head != sim->rx_tail) {
            memcpy(frame, &sim->rx_buffer[sim->rx_tail], sizeof(hil_can_frame_t));
            sim->rx_tail = (sim->rx_tail + 1) % 256;
            
            g_hil_ctx.rx_frames++;
            g_hil_ctx.rx_bytes += frame->length;
            
            printf("[HIL] SIM RX CAN ID=0x%03X, Len=%u\n", frame->id, frame->length);
            return HIL_OK;
        }
        
        if (timeout_ms == 0) {
            return HIL_ERROR_TIMEOUT;
        }
        
        if ((hil_get_timestamp_us() - start_time) >= timeout_us) {
            return HIL_ERROR_TIMEOUT;
        }
        
        hil_delay_ms(1);
    }
}

static hil_error_t hil_sim_eth_send(hil_interface_t *iface, const hil_eth_frame_t *frame) {
    hil_sim_eth_data_t *sim = (hil_sim_eth_data_t*)iface->sim_data;
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    uint32_t next_head = (sim->tx_head + 1) % 256;
    if (next_head != sim->tx_tail) {
        memcpy(&sim->tx_buffer[sim->tx_head], frame, sizeof(hil_eth_frame_t));
        sim->tx_buffer[sim->tx_head].timestamp_us = hil_get_timestamp_us();
        sim->tx_head = next_head;
    }
    
    g_hil_ctx.tx_frames++;
    g_hil_ctx.tx_bytes += frame->payload_len + 14;
    
    printf("[HIL] SIM TX Ethernet frame, Len=%u, Ethertype=0x%04X\n",
           frame->payload_len, frame->ethertype);
    
    return HIL_OK;
}

static hil_error_t hil_sim_eth_receive(hil_interface_t *iface, hil_eth_frame_t *frame,
                                        uint32_t timeout_ms) {
    hil_sim_eth_data_t *sim = (hil_sim_eth_data_t*)iface->sim_data;
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    uint64_t start_time = hil_get_timestamp_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;
    
    while (1) {
        if (sim->rx_head != sim->rx_tail) {
            memcpy(frame, &sim->rx_buffer[sim->rx_tail], sizeof(hil_eth_frame_t));
            sim->rx_tail = (sim->rx_tail + 1) % 256;
            
            g_hil_ctx.rx_frames++;
            g_hil_ctx.rx_bytes += frame->payload_len + 14;
            
            printf("[HIL] SIM RX Ethernet frame, Len=%u, Ethertype=0x%04X\n",
                   frame->payload_len, frame->ethertype);
            return HIL_OK;
        }
        
        if (timeout_ms == 0) {
            return HIL_ERROR_TIMEOUT;
        }
        
        if ((hil_get_timestamp_us() - start_time) >= timeout_us) {
            return HIL_ERROR_TIMEOUT;
        }
        
        hil_delay_ms(1);
    }
}

hil_error_t hil_sim_inject_can_frame(uint32_t interface_idx, const hil_can_frame_t *frame) {
    if (!g_hil_ctx.simulation_mode) {
        return HIL_ERROR_NOT_SUPPORTED;
    }
    
    if (interface_idx >= g_hil_ctx.num_interfaces) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    hil_interface_t *iface = &g_hil_ctx.interfaces[interface_idx];
    hil_sim_can_data_t *sim = (hil_sim_can_data_t*)iface->sim_data;
    
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    uint32_t next_head = (sim->rx_head + 1) % 256;
    if (next_head == sim->rx_tail) {
        return HIL_ERROR_NO_MEMORY;  /* Buffer full */
    }
    
    memcpy(&sim->rx_buffer[sim->rx_head], frame, sizeof(hil_can_frame_t));
    sim->rx_buffer[sim->rx_head].timestamp_us = hil_get_timestamp_us();
    sim->rx_head = next_head;
    
    /* Call callback if registered */
    if (iface->rx_callback.can_rx_cb != NULL) {
        iface->rx_callback.can_rx_cb(frame, iface->user_data);
    }
    
    return HIL_OK;
}

hil_error_t hil_sim_inject_eth_frame(uint32_t interface_idx, const hil_eth_frame_t *frame) {
    if (!g_hil_ctx.simulation_mode) {
        return HIL_ERROR_NOT_SUPPORTED;
    }
    
    if (interface_idx >= g_hil_ctx.num_interfaces) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    hil_interface_t *iface = &g_hil_ctx.interfaces[interface_idx];
    hil_sim_eth_data_t *sim = (hil_sim_eth_data_t*)iface->sim_data;
    
    if (sim == NULL) {
        return HIL_ERROR_NOT_INITIALIZED;
    }
    
    uint32_t next_head = (sim->rx_head + 1) % 256;
    if (next_head == sim->rx_tail) {
        return HIL_ERROR_NO_MEMORY;
    }
    
    memcpy(&sim->rx_buffer[sim->rx_head], frame, sizeof(hil_eth_frame_t));
    sim->rx_buffer[sim->rx_head].timestamp_us = hil_get_timestamp_us();
    sim->rx_head = next_head;
    
    if (iface->rx_callback.eth_rx_cb != NULL) {
        iface->rx_callback.eth_rx_cb(frame, iface->user_data);
    }
    
    return HIL_OK;
}

hil_error_t hil_sim_get_tx_frame(uint32_t interface_idx, bool is_can,
                                  void *frame, bool *available) {
    if (!g_hil_ctx.simulation_mode) {
        return HIL_ERROR_NOT_SUPPORTED;
    }
    
    if (interface_idx >= g_hil_ctx.num_interfaces || frame == NULL || available == NULL) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    hil_interface_t *iface = &g_hil_ctx.interfaces[interface_idx];
    *available = false;
    
    if (is_can) {
        hil_sim_can_data_t *sim = (hil_sim_can_data_t*)iface->sim_data;
        if (sim != NULL && sim->tx_head != sim->tx_tail) {
            memcpy(frame, &sim->tx_buffer[sim->tx_tail], sizeof(hil_can_frame_t));
            sim->tx_tail = (sim->tx_tail + 1) % 256;
            *available = true;
        }
    } else {
        hil_sim_eth_data_t *sim = (hil_sim_eth_data_t*)iface->sim_data;
        if (sim != NULL && sim->tx_head != sim->tx_tail) {
            memcpy(frame, &sim->tx_buffer[sim->tx_tail], sizeof(hil_eth_frame_t));
            sim->tx_tail = (sim->tx_tail + 1) % 256;
            *available = true;
        }
    }
    
    return HIL_OK;
}

hil_error_t hil_sim_set_latency(uint32_t latency_ms) {
    if (!g_hil_ctx.simulation_mode) {
        return HIL_ERROR_NOT_SUPPORTED;
    }
    
    for (uint32_t i = 0; i < g_hil_ctx.num_interfaces; i++) {
        if (g_hil_ctx.interfaces[i].sim_data != NULL) {
            if (g_hil_ctx.interfaces[i].type <= HIL_IF_TYPE_CAN_SOCKETCAN) {
                ((hil_sim_can_data_t*)g_hil_ctx.interfaces[i].sim_data)->latency_ms = latency_ms;
            } else {
                ((hil_sim_eth_data_t*)g_hil_ctx.interfaces[i].sim_data)->latency_ms = latency_ms;
            }
        }
    }
    
    printf("[HIL] Simulation latency set to %u ms\n", latency_ms);
    
    return HIL_OK;
}

hil_error_t hil_sim_set_packet_loss_rate(float rate) {
    if (!g_hil_ctx.simulation_mode) {
        return HIL_ERROR_NOT_SUPPORTED;
    }
    
    if (rate < 0.0f || rate > 1.0f) {
        return HIL_ERROR_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < g_hil_ctx.num_interfaces; i++) {
        if (g_hil_ctx.interfaces[i].sim_data != NULL) {
            if (g_hil_ctx.interfaces[i].type <= HIL_IF_TYPE_CAN_SOCKETCAN) {
                ((hil_sim_can_data_t*)g_hil_ctx.interfaces[i].sim_data)->packet_loss_rate = rate;
            } else {
                ((hil_sim_eth_data_t*)g_hil_ctx.interfaces[i].sim_data)->packet_loss_rate = rate;
            }
        }
    }
    
    printf("[HIL] Simulation packet loss rate set to %.2f%%\n", rate * 100.0f);
    
    return HIL_OK;
}

/******************************************************************************
 * Linux SocketCAN Implementation
 ******************************************************************************/
#ifndef _WIN32

static hil_error_t hil_socketcan_open(hil_interface_t *iface, uint32_t channel,
                                       const hil_can_config_t *config) {
    char ifname[16];
    snprintf(ifname, sizeof(ifname), "can%u", channel);
    
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        printf("[HIL] Error: Failed to create CAN socket: %s\n", strerror(errno));
        return HIL_ERROR_IO;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        printf("[HIL] Error: Failed to get CAN interface index: %s\n", strerror(errno));
        close(sock);
        return HIL_ERROR_IO;
    }
    
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("[HIL] Error: Failed to bind CAN socket: %s\n", strerror(errno));
        close(sock);
        return HIL_ERROR_IO;
    }
    
    /* Set non-blocking mode */
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    /* Store socket in sim_data as a hack (we'd use a proper struct in production) */
    iface->sim_data = (void*)(uintptr_t)sock;
    
    printf("[HIL] SocketCAN interface '%s' opened (fd=%d)\n", ifname, sock);
    
    return HIL_OK;
}

static hil_error_t hil_raw_socket_open(hil_interface_t *iface,
                                        const hil_eth_config_t *config) {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(config->ethertype));
    if (sock < 0) {
        printf("[HIL] Error: Failed to create raw socket: %s\n", strerror(errno));
        return HIL_ERROR_IO;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, config->interface_name, IFNAMSIZ - 1);
    
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        printf("[HIL] Error: Failed to get interface index: %s\n", strerror(errno));
        close(sock);
        return HIL_ERROR_IO;
    }
    
    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(config->ethertype);
    addr.sll_ifindex = ifr.ifr_ifindex;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("[HIL] Error: Failed to bind raw socket: %s\n", strerror(errno));
        close(sock);
        return HIL_ERROR_IO;
    }
    
    /* Set non-blocking mode */
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    iface->sim_data = (void*)(uintptr_t)sock;
    
    printf("[HIL] Raw socket on '%s' opened (fd=%d, ethertype=0x%04X)\n",
           config->interface_name, sock, config->ethertype);
    
    return HIL_OK;
}

#endif  /* !_WIN32 */

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

const char* hil_error_string(hil_error_t error) {
    switch (error) {
        case HIL_OK: return "OK";
        case HIL_ERROR_INVALID_PARAM: return "Invalid parameter";
        case HIL_ERROR_NOT_INITIALIZED: return "Not initialized";
        case HIL_ERROR_NO_MEMORY: return "No memory";
        case HIL_ERROR_INTERFACE_NOT_FOUND: return "Interface not found";
        case HIL_ERROR_CHANNEL_BUSY: return "Channel busy";
        case HIL_ERROR_TIMEOUT: return "Timeout";
        case HIL_ERROR_IO: return "I/O error";
        case HIL_ERROR_NOT_SUPPORTED: return "Not supported";
        case HIL_ERROR_SIMULATION: return "Simulation error";
        default: return "Unknown error";
    }
}

const char* hil_interface_type_string(hil_interface_type_t type) {
    switch (type) {
        case HIL_IF_TYPE_NONE: return "None";
        case HIL_IF_TYPE_CAN_PCAN: return "PCAN";
        case HIL_IF_TYPE_CAN_VECTOR: return "Vector";
        case HIL_IF_TYPE_CAN_KVASER: return "Kvaser";
        case HIL_IF_TYPE_CAN_SOCKETCAN: return "SocketCAN";
        case HIL_IF_TYPE_ETH_RAW_SOCKET: return "RawSocket";
        case HIL_IF_TYPE_ETH_PCAP: return "PCAP";
        case HIL_IF_TYPE_SIMULATION: return "Simulation";
        default: return "Unknown";
    }
}

void hil_get_stats(uint64_t *tx_frames, uint64_t *rx_frames,
                   uint64_t *tx_bytes, uint64_t *rx_bytes, uint64_t *errors) {
    if (tx_frames) *tx_frames = g_hil_ctx.tx_frames;
    if (rx_frames) *rx_frames = g_hil_ctx.rx_frames;
    if (tx_bytes) *tx_bytes = g_hil_ctx.tx_bytes;
    if (rx_bytes) *rx_bytes = g_hil_ctx.rx_bytes;
    if (errors) *errors = g_hil_ctx.errors;
}

void hil_reset_stats(void) {
    g_hil_ctx.tx_frames = 0;
    g_hil_ctx.rx_frames = 0;
    g_hil_ctx.tx_bytes = 0;
    g_hil_ctx.rx_bytes = 0;
    g_hil_ctx.errors = 0;
}

hil_error_t hil_poll_interfaces(hil_interface_t **ifaces, uint32_t num_ifaces,
                                 uint32_t timeout_ms) {
    /* Simple polling implementation - in production would use select()/poll() */
    uint64_t start_time = hil_get_timestamp_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;
    
    while (1) {
        for (uint32_t i = 0; i < num_ifaces; i++) {
            if (ifaces[i] != NULL && ifaces[i]->is_open) {
                /* Check for available data based on interface type */
                if (ifaces[i]->type <= HIL_IF_TYPE_CAN_SOCKETCAN) {
                    hil_sim_can_data_t *sim = (hil_sim_can_data_t*)ifaces[i]->sim_data;
                    if (sim != NULL && sim->rx_head != sim->rx_tail) {
                        return HIL_OK;
                    }
                } else {
                    hil_sim_eth_data_t *sim = (hil_sim_eth_data_t*)ifaces[i]->sim_data;
                    if (sim != NULL && sim->rx_head != sim->rx_tail) {
                        return HIL_OK;
                    }
                }
            }
        }
        
        if (timeout_ms == 0) {
            return HIL_ERROR_TIMEOUT;
        }
        
        if ((hil_get_timestamp_us() - start_time) >= timeout_us) {
            return HIL_ERROR_TIMEOUT;
        }
        
        hil_delay_ms(1);
    }
}

void hil_delay_ms(uint32_t ms) {
    #ifdef _WIN32
    Sleep(ms);
    #else
    usleep(ms * 1000);
    #endif
}

uint64_t hil_get_timestamp_us(void) {
    #ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10;  /* Convert to microseconds */
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    #endif
}
