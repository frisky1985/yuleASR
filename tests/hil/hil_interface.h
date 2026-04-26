/******************************************************************************
 * @file    hil_interface.h
 * @brief   HIL (Hardware-in-the-Loop) Hardware Abstraction Layer Interface
 *
 * 硬件在环测试框架 - C侧接口
 * 支持: CAN接口(PCAN/Vector/Kvaser), 以太网Raw Socket
 * 支持POSIX环境模拟运行 (HIL_SIMULATION_MODE)
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/
#ifndef HIL_INTERFACE_H
#define HIL_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define HIL_VERSION_MAJOR           1
#define HIL_VERSION_MINOR           0
#define HIL_VERSION_PATCH           0

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define HIL_MAX_CAN_INTERFACES      4
#define HIL_MAX_ETH_INTERFACES      2
#define HIL_MAX_CHANNELS_PER_IF     8
#define HIL_MAX_MESSAGE_SIZE        4096
#define HIL_MAX_FRAME_SIZE          64
#define HIL_DEFAULT_TIMEOUT_MS      1000

/******************************************************************************
 * Error Codes
 ******************************************************************************/
typedef enum {
    HIL_OK = 0,
    HIL_ERROR_INVALID_PARAM = -1,
    HIL_ERROR_NOT_INITIALIZED = -2,
    HIL_ERROR_NO_MEMORY = -3,
    HIL_ERROR_INTERFACE_NOT_FOUND = -4,
    HIL_ERROR_CHANNEL_BUSY = -5,
    HIL_ERROR_TIMEOUT = -6,
    HIL_ERROR_IO = -7,
    HIL_ERROR_NOT_SUPPORTED = -8,
    HIL_ERROR_SIMULATION = -9
} hil_error_t;

/******************************************************************************
 * Interface Types
 ******************************************************************************/
typedef enum {
    HIL_IF_TYPE_NONE = 0,
    HIL_IF_TYPE_CAN_PCAN,           /* PEAK PCAN */
    HIL_IF_TYPE_CAN_VECTOR,         /* Vector CAN */
    HIL_IF_TYPE_CAN_KVASER,         /* Kvaser CAN */
    HIL_IF_TYPE_CAN_SOCKETCAN,      /* Linux SocketCAN */
    HIL_IF_TYPE_ETH_RAW_SOCKET,     /* Raw Ethernet Socket */
    HIL_IF_TYPE_ETH_PCAP,           /* libpcap Ethernet */
    HIL_IF_TYPE_SIMULATION          /* Software Simulation Mode */
} hil_interface_type_t;

/******************************************************************************
 * CAN Configuration
 ******************************************************************************/
typedef enum {
    HIL_CAN_BITRATE_125K = 125000,
    HIL_CAN_BITRATE_250K = 250000,
    HIL_CAN_BITRATE_500K = 500000,
    HIL_CAN_BITRATE_1M = 1000000,
    HIL_CAN_BITRATE_2M = 2000000,
    HIL_CAN_BITRATE_5M = 5000000
} hil_can_bitrate_t;

typedef enum {
    HIL_CAN_MODE_STANDARD = 0,      /* 11-bit IDs */
    HIL_CAN_MODE_EXTENDED,          /* 29-bit IDs */
    HIL_CAN_MODE_FD                 /* CAN FD Mode */
} hil_can_mode_t;

typedef struct {
    hil_can_bitrate_t bitrate;
    hil_can_mode_t mode;
    bool fd_enabled;
    uint32_t rx_filter_id;          /* Receive filter ID */
    uint32_t rx_filter_mask;        /* Receive filter mask */
    uint16_t tx_id;                 /* Default TX CAN ID */
    uint16_t rx_id;                 /* Default RX CAN ID */
} hil_can_config_t;

/******************************************************************************
 * Ethernet Configuration
 ******************************************************************************/
typedef struct {
    uint8_t local_mac[6];           /* Local MAC address */
    uint8_t remote_mac[6];          /* Remote MAC address (optional) */
    uint16_t ethertype;             /* EtherType filter (e.g., 0x13400 for DoIP) */
    char interface_name[16];        /* Interface name (e.g., "eth0") */
    bool promiscuous_mode;          /* Promiscuous mode */
    uint32_t rx_timeout_ms;         /* Receive timeout */
} hil_eth_config_t;

/******************************************************************************
 * Message Frame Types
 ******************************************************************************/
typedef struct {
    uint32_t id;                    /* CAN ID */
    uint8_t data[64];               /* CAN data (up to 64 bytes for FD) */
    uint8_t length;                 /* Data length */
    bool is_extended;               /* Extended frame (29-bit) */
    bool is_fd;                     /* CAN FD frame */
    uint64_t timestamp_us;          /* Timestamp in microseconds */
} hil_can_frame_t;

typedef struct {
    uint8_t dst_mac[6];             /* Destination MAC */
    uint8_t src_mac[6];             /* Source MAC */
    uint16_t ethertype;             /* EtherType */
    uint8_t payload[HIL_MAX_MESSAGE_SIZE];
    uint32_t payload_len;
    uint64_t timestamp_us;
} hil_eth_frame_t;

typedef struct {
    uint8_t data[HIL_MAX_MESSAGE_SIZE];
    uint32_t length;
    uint64_t timestamp_us;
} hil_raw_message_t;

/******************************************************************************
 * Callback Types
 ******************************************************************************/
typedef void (*hil_can_rx_callback_t)(const hil_can_frame_t *frame, void *user_data);
typedef void (*hil_eth_rx_callback_t)(const hil_eth_frame_t *frame, void *user_data);

/******************************************************************************
 * Interface Handle
 ******************************************************************************/
typedef struct hil_interface_s hil_interface_t;

struct hil_interface_s {
    uint32_t handle_id;
    hil_interface_type_t type;
    bool initialized;
    bool is_open;
    
    /* Interface specific data */
    union {
        hil_can_config_t can_config;
        hil_eth_config_t eth_config;
    } config;
    
    /* Callbacks */
    union {
        hil_can_rx_callback_t can_rx_cb;
        hil_eth_rx_callback_t eth_rx_cb;
    } rx_callback;
    void *user_data;
    
    /* Simulation mode data */
    void *sim_data;
};

/******************************************************************************
 * HIL Context
 ******************************************************************************/
typedef struct {
    hil_interface_t interfaces[HIL_MAX_CAN_INTERFACES + HIL_MAX_ETH_INTERFACES];
    uint32_t num_interfaces;
    bool initialized;
    bool simulation_mode;
    
    /* Statistics */
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t errors;
} hil_context_t;

/******************************************************************************
 * Initialization / Deinitialization
 ******************************************************************************/

/**
 * @brief Initialize HIL framework
 * @param simulation_mode Enable simulation mode (no real hardware required)
 * @return HIL_OK on success
 */
hil_error_t hil_init(bool simulation_mode);

/**
 * @brief Deinitialize HIL framework
 */
void hil_deinit(void);

/**
 * @brief Check if HIL is initialized
 */
bool hil_is_initialized(void);

/**
 * @brief Check if running in simulation mode
 */
bool hil_is_simulation_mode(void);

/**
 * @brief Get HIL context
 */
hil_context_t* hil_get_context(void);

/******************************************************************************
 * CAN Interface Functions
 ******************************************************************************/

/**
 * @brief Open CAN interface
 * @param type Interface type (PCAN/Vector/Kvaser/SocketCAN)
 * @param channel Channel number (0-based)
 * @param config CAN configuration
 * @return Interface handle or NULL on error
 */
hil_interface_t* hil_can_open(hil_interface_type_t type, uint32_t channel,
                               const hil_can_config_t *config);

/**
 * @brief Close CAN interface
 */
hil_error_t hil_can_close(hil_interface_t *iface);

/**
 * @brief Send CAN frame
 */
hil_error_t hil_can_send(hil_interface_t *iface, const hil_can_frame_t *frame);

/**
 * @brief Receive CAN frame (blocking)
 */
hil_error_t hil_can_receive(hil_interface_t *iface, hil_can_frame_t *frame,
                             uint32_t timeout_ms);

/**
 * @brief Set CAN receive callback (non-blocking mode)
 */
hil_error_t hil_can_set_rx_callback(hil_interface_t *iface,
                                     hil_can_rx_callback_t callback,
                                     void *user_data);

/**
 * @brief Set CAN bitrate
 */
hil_error_t hil_can_set_bitrate(hil_interface_t *iface, hil_can_bitrate_t bitrate);

/**
 * @brief Flush CAN TX buffer
 */
hil_error_t hil_can_flush(hil_interface_t *iface);

/******************************************************************************
 * Ethernet Interface Functions
 ******************************************************************************/

/**
 * @brief Open Ethernet interface
 * @param type Interface type (Raw Socket or PCAP)
 * @param config Ethernet configuration
 * @return Interface handle or NULL on error
 */
hil_interface_t* hil_eth_open(hil_interface_type_t type,
                               const hil_eth_config_t *config);

/**
 * @brief Close Ethernet interface
 */
hil_error_t hil_eth_close(hil_interface_t *iface);

/**
 * @brief Send Ethernet frame
 */
hil_error_t hil_eth_send(hil_interface_t *iface, const hil_eth_frame_t *frame);

/**
 * @brief Receive Ethernet frame (blocking)
 */
hil_error_t hil_eth_receive(hil_interface_t *iface, hil_eth_frame_t *frame,
                             uint32_t timeout_ms);

/**
 * @brief Set Ethernet receive callback (non-blocking mode)
 */
hil_error_t hil_eth_set_rx_callback(hil_interface_t *iface,
                                     hil_eth_rx_callback_t callback,
                                     void *user_data);

/******************************************************************************
 * Simulation Mode Functions
 ******************************************************************************/

/**
 * @brief Inject simulated CAN frame (simulation mode only)
 */
hil_error_t hil_sim_inject_can_frame(uint32_t interface_idx,
                                      const hil_can_frame_t *frame);

/**
 * @brief Inject simulated Ethernet frame (simulation mode only)
 */
hil_error_t hil_sim_inject_eth_frame(uint32_t interface_idx,
                                      const hil_eth_frame_t *frame);

/**
 * @brief Get simulated TX frame (simulation mode only)
 */
hil_error_t hil_sim_get_tx_frame(uint32_t interface_idx, bool is_can,
                                  void *frame, bool *available);

/**
 * @brief Set simulation latency
 */
hil_error_t hil_sim_set_latency(uint32_t latency_ms);

/**
 * @brief Set simulation packet loss rate
 */
hil_error_t hil_sim_set_packet_loss_rate(float rate);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get error string
 */
const char* hil_error_string(hil_error_t error);

/**
 * @brief Get interface type string
 */
const char* hil_interface_type_string(hil_interface_type_t type);

/**
 * @brief Get statistics
 */
void hil_get_stats(uint64_t *tx_frames, uint64_t *rx_frames,
                   uint64_t *tx_bytes, uint64_t *rx_bytes, uint64_t *errors);

/**
 * @brief Reset statistics
 */
void hil_reset_stats(void);

/**
 * @brief Wait for multiple interfaces (poll)
 */
hil_error_t hil_poll_interfaces(hil_interface_t **ifaces, uint32_t num_ifaces,
                                 uint32_t timeout_ms);

/**
 * @brief Millisecond delay
 */
void hil_delay_ms(uint32_t ms);

/**
 * @brief Get current timestamp in microseconds
 */
uint64_t hil_get_timestamp_us(void);

#ifdef __cplusplus
}
#endif

#endif /* HIL_INTERFACE_H */
