/*
 * gptp_hw.h
 * NXP S32G3 Hardware PTP/IEEE 1588 Timestamp Support
 * Hardware time synchronization for gPTP (IEEE 802.1AS)
 */

#ifndef GPTP_HW_H
#define GPTP_HW_H

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * PTP Hardware Clock Definitions
 *============================================================================*/

/* PTP Clock Frequency (50MHz typical for S32G3) */
#define PTP_CLOCK_FREQ_HZ       50000000UL
#define PTP_NS_PER_TICK         20UL        /* 1/50MHz = 20ns */

/* PTP Time Structure */
typedef struct {
    uint64_t seconds;           /* Seconds since epoch */
    uint32_t nanoseconds;       /* Nanoseconds (0-999999999) */
} ptp_time_t;

/* Time Correction Structure */
typedef struct {
    int64_t offset_ns;          /* Offset correction in nanoseconds */
    int32_t rate_ratio;         /* Rate ratio (parts per billion) */
    bool step_clock;            /* Step clock vs. adjust rate */
} ptp_correction_t;

/* Timestamp Capture Point */
typedef enum {
    TS_CAP_PTP_EVENT_RX = 0,    /* PTP Event Message RX */
    TS_CAP_PTP_EVENT_TX,        /* PTP Event Message TX */
    TS_CAP_ALL_RX,              /* All Frames RX */
    TS_CAP_ALL_TX,              /* All Frames TX */
    TS_CAP_PEER_DELAY_RX,       /* Peer Delay Request/Response RX */
    TS_CAP_PEER_DELAY_TX        /* Peer Delay Request/Response TX */
} ts_capture_point_t;

/*==============================================================================
 * Hardware Timestamp Register Definitions
 * Based on ENET_MAC_TSCR and related registers
 *============================================================================*/

/* Time Stamp Control Register Bits (already defined in enet_driver.h) */
/* Additional bits for gPTP */
#define MAC_TSCR_TSENMACADDR    (1UL << 18) /* Enable MAC address for PTP filtering */
#define MAC_TSCR_SNAPTYPSEL_SHIFT   16      /* Snapshot Type Select */

/* Time Stamp Status Register */
#define ENET_MAC_TSSR_OFFSET    0x0728
#define MAC_TSSR_TSSOVF         (1UL << 0)  /* Time Stamp Seconds Overflow */
#define MAC_TSSR_TSTARGT        (1UL << 1)  /* Time Stamp Target Time Reached */

/* Time Stamp PPS Control Register */
#define ENET_MAC_PPSCR_OFFSET   0x072C

/* Auxiliary Time Stamp Control Register */
#define ENET_MAC_ATSCR_OFFSET   0x0730

/* Time Stamp Addend Register (for fine correction) */
#define ENET_MAC_TAR_OFFSET     0x0710

/* Target Time Seconds/Nanoseconds Registers */
#define ENET_MAC_TTSR_OFFSET    0x0718
#define ENET_MAC_TTNR_OFFSET    0x071C

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Initialization */
int ptp_hw_init(uint8_t enet_instance);
int ptp_hw_deinit(uint8_t enet_instance);

/* Time Operations */
int ptp_hw_get_time(uint8_t enet_instance, ptp_time_t *time);
int ptp_hw_set_time(uint8_t enet_instance, const ptp_time_t *time);
int ptp_hw_adj_time(uint8_t enet_instance, int64_t delta_ns);
int ptp_hw_adj_freq(uint8_t enet_instance, int32_t ppb);

/* Fine Correction (using Addend Register) */
int ptp_hw_set_addend(uint8_t enet_instance, uint32_t addend);
int ptp_hw_update_addend(uint8_t enet_instance, int32_t ppb);

/* Timestamp Capture Configuration */
int ptp_hw_config_capture(uint8_t enet_instance, ts_capture_point_t point, bool enable);
int ptp_hw_get_tx_timestamp(uint8_t enet_instance, uint64_t *timestamp);
int ptp_hw_get_rx_timestamp(uint8_t enet_instance, uint64_t *timestamp);

/* One-Step vs Two-Step Clock */
int ptp_hw_set_one_step(uint8_t enet_instance, bool enable);
bool ptp_hw_is_one_step(uint8_t enet_instance);

/* Hardware Clock Control */
int ptp_hw_enable(uint8_t enet_instance);
int ptp_hw_disable(uint8_t enet_instance);

/* Periodic Output (PPS) */
int ptp_hw_config_pps(uint8_t enet_instance, uint32_t period_ns);
int ptp_hw_enable_pps(uint8_t enet_instance);
int ptp_hw_disable_pps(uint8_t enet_instance);

/* Timestamp Availability Check */
bool ptp_hw_tx_timestamp_ready(uint8_t enet_instance);
bool ptp_hw_rx_timestamp_ready(uint8_t enet_instance);

/* Interrupt Control */
int ptp_hw_enable_irq(uint8_t enet_instance);
int ptp_hw_disable_irq(uint8_t enet_instance);
void ptp_hw_irq_handler(uint8_t enet_instance);

/* Utility Functions */
uint64_t ptp_time_to_ns(const ptp_time_t *time);
void ptp_ns_to_time(uint64_t ns, ptp_time_t *time);
int64_t ptp_time_diff(const ptp_time_t *t1, const ptp_time_t *t2);

#endif /* GPTP_HW_H */
