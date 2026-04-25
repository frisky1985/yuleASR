/*
 * gptp_hw_tc3xx.c
 * Infineon AURIX TC3xx Hardware PTP/IEEE 1588 Implementation
 * Uses GETH internal timestamping capabilities
 */

#include "geth_driver.h"
#include <string.h>

/*==============================================================================
 * TC3xx GETH Timestamp Register Definitions
 *============================================================================*/

/* Timestamp Control Register */
#define GETH_MAC_TIMESTAMP_CTRL_OFFSET  0x0700
#define GETH_MAC_TIMESTAMP_CTRL         (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_TIMESTAMP_CTRL_OFFSET)))

#define TS_CTRL_TSENA           (1UL << 0)      /* Timestamp Enable */
#define TS_CTRL_TSCFUPDT        (1UL << 1)      /* Timestamp Fine/Coarse Update */
#define TS_CTRL_TSINIT          (1UL << 2)      /* Timestamp Initialize */
#define TS_CTRL_TSUPDT          (1UL << 3)      /* Timestamp Update */
#define TS_CTRL_TSTRIG          (1UL << 4)      /* Timestamp Interrupt Trigger Enable */
#define TS_CTRL_TSADDREG        (1UL << 5)      /* Addend Reg Update */
#define TS_CTRL_TSENALL         (1UL << 8)      /* Enable Timestamp for All Frames */
#define TS_CTRL_TSCTRLSSR       (1UL << 9)      /* Digital/Binary Rollover */
#define TS_CTRL_TSVER2ENA       (1UL << 10)     /* Enable PTP Packet Processing for Version 2 */
#define TS_CTRL_TSIPENA         (1UL << 11)     /* Enable Processing of PTP over IPv4 */
#define TS_CTRL_TSIPV6ENA       (1UL << 12)     /* Enable Processing of PTP over IPv6 */
#define TS_CTRL_TSEVNTENA       (1UL << 14)     /* Enable Timestamp for Event Messages */
#define TS_CTRL_TSMSTRENA       (1UL << 15)     /* Enable Timestamp for Master Node */

/* Sub-Second Increment Register */
#define GETH_MAC_SUB_SEC_INCR_OFFSET    0x0704
#define GETH_MAC_SUB_SEC_INCR           (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_SUB_SEC_INCR_OFFSET)))

#define SSINCR_SSINC_SHIFT      16              /* Sub-second Increment Value */
#define SSINCR_SNSINC_MASK      0xFF            /* Sub-nanosecond Increment */

/* System Time Seconds/Nanoseconds Registers */
#define GETH_MAC_SYS_TIME_SEC_OFFSET    0x0708
#define GETH_MAC_SYS_TIME_NANOSEC_OFFSET 0x070C
#define GETH_MAC_SYS_TIME_SEC_UPD_OFFSET 0x0710
#define GETH_MAC_SYS_TIME_NANOSEC_UPD_OFFSET 0x0714

/* Timestamp Addend Register */
#define GETH_MAC_TIMESTAMP_ADDEND_OFFSET 0x0718
#define GETH_MAC_TIMESTAMP_ADDEND       (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_TIMESTAMP_ADDEND_OFFSET)))

/* Timestamp Status Register */
#define GETH_MAC_TIMESTAMP_STATUS_OFFSET 0x0728
#define GETH_MAC_TIMESTAMP_STATUS       (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_TIMESTAMP_STATUS_OFFSET)))

#define TS_STATUS_TSSOVF        (1UL << 0)      /* Timestamp Seconds Overflow */
#define TS_STATUS_TSTARGT       (1UL << 1)      /* Timestamp Target Time Reached */

/*==============================================================================
 * TC3xx PTP Hardware Types
 *============================================================================*/

typedef struct {
    uint64_t seconds;
    uint32_t nanoseconds;
} tc3xx_ptp_time_t;

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static uint32_t get_geth_base(uint8_t instance)
{
    if (instance == 0) return GETH0_BASE_ADDR;
    if (instance == 1) return GETH1_BASE_ADDR;
    return 0;
}

static inline uint32_t read_reg(uint32_t base, uint32_t offset)
{
    return *((volatile uint32_t *)(base + offset));
}

static inline void write_reg(uint32_t base, uint32_t offset, uint32_t value)
{
    *((volatile uint32_t *)(base + offset)) = value;
}

static inline void dmb(void)
{
    __asm__ volatile ("dmb" ::: "memory");
}

/*==============================================================================
 * PTP Time Conversion
 *============================================================================*/

static uint64_t ptp_time_to_ns(const tc3xx_ptp_time_t *time)
{
    if (!time) return 0;
    return (time->seconds * 1000000000ULL) + time->nanoseconds;
}

static void ptp_ns_to_time(uint64_t ns, tc3xx_ptp_time_t *time)
{
    if (!time) return;
    time->seconds = ns / 1000000000ULL;
    time->nanoseconds = (uint32_t)(ns % 1000000000ULL);
}

/*==============================================================================
 * PTP Hardware API Implementation
 *============================================================================*/

/*
 * gptp_tc3xx_init - Initialize PTP hardware timestamping
 */
int gptp_tc3xx_init(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    if (!base) {
        return -1;
    }
    
    /* Disable timestamping during configuration */
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, 0);
    dmb();
    
    /* Configure sub-second increment for 20ns resolution */
    /* TC3xx typically runs at 50MHz: 1/50MHz = 20ns */
    write_reg(base, GETH_MAC_SUB_SEC_INCR_OFFSET, 20 << SSINCR_SSINC_SHIFT);
    
    /* Initialize addend register for fine correction */
    /* For 50MHz: addend = 2^31 / (50MHz / 2^31) = 0x80000000 */
    write_reg(base, GETH_MAC_TIMESTAMP_ADDEND_OFFSET, 0x80000000UL);
    
    /* Configure timestamp control */
    reg_val = TS_CTRL_TSENA |           /* Enable timestamp */
              TS_CTRL_TSCFUPDT |        /* Fine update mode */
              TS_CTRL_TSCTRLSSR |       /* Digital rollover */
              TS_CTRL_TSVER2ENA |       /* PTP v2 support */
              TS_CTRL_TSEVNTENA |       /* Event messages only */
              TS_CTRL_TSENALL;          /* Enable for all frames */
    
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    /* Initialize system time to zero */
    write_reg(base, GETH_MAC_SYS_TIME_SEC_OFFSET, 0);
    write_reg(base, GETH_MAC_SYS_TIME_NANOSEC_OFFSET, 0);
    
    /* Trigger initialization */
    reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
    reg_val |= TS_CTRL_TSINIT;
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    /* Wait for initialization complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
        if (!(reg_val & TS_CTRL_TSINIT)) {
            break;
        }
    }
    
    return 0;
}

/*
 * gptp_tc3xx_deinit - Disable PTP timestamping
 */
int gptp_tc3xx_deinit(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    /* Disable timestamping */
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, 0);
    dmb();
    
    return 0;
}

/*
 * gptp_tc3xx_get_time - Read current PTP hardware time
 */
int gptp_tc3xx_get_time(uint8_t instance, tc3xx_ptp_time_t *time)
{
    uint32_t base;
    uint32_t seconds, nanoseconds;
    
    if (!time || instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    /* Read time - must read twice to handle overflow */
    do {
        seconds = read_reg(base, GETH_MAC_SYS_TIME_SEC_OFFSET);
        nanoseconds = read_reg(base, GETH_MAC_SYS_TIME_NANOSEC_OFFSET);
    } while (seconds != read_reg(base, GETH_MAC_SYS_TIME_SEC_OFFSET));
    
    /* Handle negative nanoseconds (two's complement in TC3xx) */
    if (nanoseconds & 0x80000000UL) {
        nanoseconds = 1000000000UL - (0x80000000UL - (nanoseconds & 0x7FFFFFFFUL));
        if (seconds > 0) {
            seconds--;
        }
    }
    
    time->seconds = seconds;
    time->nanoseconds = nanoseconds;
    
    return 0;
}

/*
 * gptp_tc3xx_set_time - Set PTP hardware time
 */
int gptp_tc3xx_set_time(uint8_t instance, const tc3xx_ptp_time_t *time)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (!time || instance > 1) {
        return -1;
    }
    
    if (time->nanoseconds >= 1000000000UL) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    /* Write update registers */
    write_reg(base, GETH_MAC_SYS_TIME_SEC_UPD_OFFSET, (uint32_t)time->seconds);
    write_reg(base, GETH_MAC_SYS_TIME_NANOSEC_UPD_OFFSET, time->nanoseconds);
    dmb();
    
    /* Trigger update */
    reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
    reg_val |= TS_CTRL_TSUPDT;
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    /* Wait for update complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
        if (!(reg_val & TS_CTRL_TSUPDT)) {
            return 0;
        }
    }
    
    return -1;
}

/*
 * gptp_tc3xx_adj_time - Step-adjust PTP time (coarse correction)
 */
int gptp_tc3xx_adj_time(uint8_t instance, int64_t delta_ns)
{
    tc3xx_ptp_time_t current_time;
    tc3xx_ptp_time_t new_time;
    uint64_t current_ns;
    
    if (instance > 1) {
        return -1;
    }
    
    /* Get current time */
    if (gptp_tc3xx_get_time(instance, &current_time) != 0) {
        return -1;
    }
    
    /* Calculate new time */
    current_ns = ptp_time_to_ns(&current_time);
    current_ns += delta_ns;
    
    /* Handle negative adjustment */
    if (delta_ns < 0 && (uint64_t)(-delta_ns) > current_ns) {
        current_ns = 0;
    }
    
    ptp_ns_to_time(current_ns, &new_time);
    
    return gptp_tc3xx_set_time(instance, &new_time);
}

/*
 * gptp_tc3xx_set_addend - Set frequency adjustment addend
 */
int gptp_tc3xx_set_addend(uint8_t instance, uint32_t addend)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    /* Write addend value */
    write_reg(base, GETH_MAC_TIMESTAMP_ADDEND_OFFSET, addend);
    dmb();
    
    /* Trigger addend register update */
    reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
    reg_val |= TS_CTRL_TSADDREG;
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    /* Wait for update complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
        if (!(reg_val & TS_CTRL_TSADDREG)) {
            return 0;
        }
    }
    
    return -1;
}

/*
 * gptp_tc3xx_adj_freq - Adjust frequency (fine correction)
 * ppb: parts per billion (+/- 10000000 for +/- 10ppm)
 */
int gptp_tc3xx_adj_freq(uint8_t instance, int32_t ppb)
{
    int64_t addend;
    int64_t diff;
    
    if (instance > 1) {
        return -1;
    }
    
    /* Clamp to reasonable range */
    if (ppb > 10000000) ppb = 10000000;
    if (ppb < -10000000) ppb = -10000000;
    
    /* Calculate new addend: addend = base * (1 + ppb/1e9) */
    addend = 0x80000000LL;  /* Base addend for 50MHz */
    diff = (addend * ppb) / 1000000000LL;
    addend += diff;
    
    /* Clamp to valid range */
    if (addend < 0) addend = 0;
    if (addend > 0xFFFFFFFFLL) addend = 0xFFFFFFFFLL;
    
    return gptp_tc3xx_set_addend(instance, (uint32_t)addend);
}

/*
 * gptp_tc3xx_get_tx_timestamp - Get transmit timestamp from descriptor
 */
int gptp_tc3xx_get_tx_timestamp(uint8_t instance, uint64_t *timestamp)
{
    geth_handle_t *handle;
    geth_tx_desc_t *desc;
    
    if (!timestamp || instance > 1) {
        return -1;
    }
    
    handle = &geth_handles[instance];
    if (!handle || !handle->tx_desc) {
        return -1;
    }
    
    desc = &handle->tx_desc[handle->tx_cons_idx];
    
    /* Check if timestamp is valid */
    if (!(desc->tdes0 & TDES0_TTSS)) {
        return -1;
    }
    
    /* Extract timestamp (64-bit nanoseconds from descriptors) */
    *timestamp = ((uint64_t)desc->tdes7 << 32) | desc->tdes6;
    
    return 0;
}

/*
 * gptp_tc3xx_get_rx_timestamp - Get receive timestamp from descriptor
 */
int gptp_tc3xx_get_rx_timestamp(uint8_t instance, uint64_t *timestamp)
{
    geth_handle_t *handle;
    geth_rx_desc_t *desc;
    
    if (!timestamp || instance > 1) {
        return -1;
    }
    
    handle = &geth_handles[instance];
    if (!handle || !handle->rx_desc) {
        return -1;
    }
    
    desc = &handle->rx_desc[handle->rx_cons_idx];
    
    /* Check if timestamp available */
    if (!(desc->rdes0 & RDES0_TS)) {
        return -1;
    }
    
    /* Extract timestamp */
    *timestamp = ((uint64_t)desc->rdes7 << 32) | desc->rdes6;
    
    return 0;
}

/*
 * gptp_tc3xx_enable - Enable timestamping
 */
int gptp_tc3xx_enable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
    reg_val |= TS_CTRL_TSENA;
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    return 0;
}

/*
 * gptp_tc3xx_disable - Disable timestamping
 */
int gptp_tc3xx_disable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = get_geth_base(instance);
    
    reg_val = read_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET);
    reg_val &= ~TS_CTRL_TSENA;
    write_reg(base, GETH_MAC_TIMESTAMP_CTRL_OFFSET, reg_val);
    dmb();
    
    return 0;
}

/*
 * gptp_tc3xx_get_status - Get timestamp status (overflow, target reached)
 */
uint32_t gptp_tc3xx_get_status(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 1) {
        return 0;
    }
    
    base = get_geth_base(instance);
    return read_reg(base, GETH_MAC_TIMESTAMP_STATUS_OFFSET);
}

/*
 * gptp_tc3xx_clear_status - Clear timestamp status flags
 */
void gptp_tc3xx_clear_status(uint8_t instance, uint32_t flags)
{
    uint32_t base;
    
    if (instance > 1) {
        return;
    }
    
    base = get_geth_base(instance);
    /* Write back status to clear (W1C) */
    write_reg(base, GETH_MAC_TIMESTAMP_STATUS_OFFSET, flags);
}
