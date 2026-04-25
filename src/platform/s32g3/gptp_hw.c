/*
 * gptp_hw.c
 * NXP S32G3 Hardware PTP/IEEE 1588 Implementation
 */

#include "gptp_hw.h"
#include "enet_driver.h"
#include <string.h>

/*==============================================================================
 * Static Variables
 *============================================================================*/

/* Addend register base value for 50MHz PTP clock */
#define PTP_ADDEND_BASE         0x80000000UL

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static uint32_t get_enet_base(uint8_t instance)
{
    switch(instance) {
        case 0: return ENET0_BASE_ADDR;
        case 1: return ENET1_BASE_ADDR;
        case 2: return ENET2_BASE_ADDR;
        default: return 0;
    }
}

static inline uint32_t read_reg(uint32_t base, uint32_t offset)
{
    return *((volatile uint32_t *)(base + offset));
}

static inline void write_reg(uint32_t base, uint32_t offset, uint32_t value)
{
    *((volatile uint32_t *)(base + offset)) = value;
}

/*==============================================================================
 * Time Conversion Utilities
 *============================================================================*/

uint64_t ptp_time_to_ns(const ptp_time_t *time)
{
    if (!time) return 0;
    return (time->seconds * 1000000000ULL) + time->nanoseconds;
}

void ptp_ns_to_time(uint64_t ns, ptp_time_t *time)
{
    if (!time) return;
    time->seconds = ns / 1000000000ULL;
    time->nanoseconds = (uint32_t)(ns % 1000000000ULL);
}

int64_t ptp_time_diff(const ptp_time_t *t1, const ptp_time_t *t2)
{
    int64_t diff_ns;
    
    if (!t1 || !t2) return 0;
    
    diff_ns = (int64_t)(t1->seconds - t2->seconds) * 1000000000LL;
    diff_ns += (int64_t)(t1->nanoseconds - t2->nanoseconds);
    
    return diff_ns;
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

int ptp_hw_init(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    if (!base) {
        return -1;
    }
    
    /* 1. Disable timestamping during configuration */
    write_reg(base, ENET_MAC_TSCR_OFFSET, 0);
    
    /* 2. Configure sub-second increment for 20ns resolution */
    /* For 50MHz PTP clock: 1/50MHz = 20ns */
    write_reg(base, ENET_MAC_SSIR_OFFSET, PTP_NS_PER_TICK);
    
    /* 3. Initialize addend register for fine correction */
    write_reg(base, ENET_MAC_TAR_OFFSET, PTP_ADDEND_BASE);
    
    /* 4. Configure timestamp control register */
    reg_val = MAC_TSCR_TSENA |          /* Enable timestamp */
              MAC_TSCR_TSCFUPDT |       /* Fine update mode */
              MAC_TSCR_TSCTRLSSR;       /* Digital (binary) rollover */
    
    /* Enable timestamp for all frames (for gPTP operation) */
    reg_val |= MAC_TSCR_TSENALL;
    
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    /* 5. Initialize time to zero */
    write_reg(base, ENET_MAC_STSR_OFFSET, 0);
    write_reg(base, ENET_MAC_STNR_OFFSET, 0);
    
    /* Trigger initialization */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val |= MAC_TSCR_TSINIT;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    /* Wait for initialization complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
        if (!(reg_val & MAC_TSCR_TSINIT)) {
            break;
        }
    }
    
    return 0;
}

int ptp_hw_deinit(uint8_t enet_instance)
{
    uint32_t base;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    if (!base) {
        return -1;
    }
    
    /* Disable timestamping */
    write_reg(base, ENET_MAC_TSCR_OFFSET, 0);
    
    return 0;
}

int ptp_hw_get_time(uint8_t enet_instance, ptp_time_t *time)
{
    uint32_t base;
    uint32_t seconds, nanoseconds;
    
    if (!time || enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Read time - need to read twice to handle overflow */
    do {
        seconds = read_reg(base, ENET_MAC_STSR_OFFSET);
        nanoseconds = read_reg(base, ENET_MAC_STNR_OFFSET);
    } while (seconds != read_reg(base, ENET_MAC_STSR_OFFSET));
    
    /* Check for negative nanoseconds (two's complement) */
    if (nanoseconds & 0x80000000UL) {
        /* Negative value - convert to positive representation */
        nanoseconds = 1000000000UL - (0x80000000UL - (nanoseconds & 0x7FFFFFFFUL));
        if (seconds > 0) {
            seconds--;
        }
    }
    
    time->seconds = seconds;
    time->nanoseconds = nanoseconds;
    
    return 0;
}

int ptp_hw_set_time(uint8_t enet_instance, const ptp_time_t *time)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (!time || enet_instance > 2) {
        return -1;
    }
    
    if (time->nanoseconds >= 1000000000UL) {
        return -1;  /* Invalid nanoseconds value */
    }
    
    base = get_enet_base(enet_instance);
    
    /* Write new time values */
    write_reg(base, ENET_MAC_STSR_OFFSET, (uint32_t)time->seconds);
    write_reg(base, ENET_MAC_STNR_OFFSET, time->nanoseconds);
    
    /* Trigger update */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val |= MAC_TSCR_TSUPDT;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    /* Wait for update complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
        if (!(reg_val & MAC_TSCR_TSUPDT)) {
            break;
        }
    }
    
    return 0;
}

int ptp_hw_adj_time(uint8_t enet_instance, int64_t delta_ns)
{
    ptp_time_t current_time;
    ptp_time_t new_time;
    uint64_t current_ns;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    /* Get current time */
    if (ptp_hw_get_time(enet_instance, &current_time) != 0) {
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
    
    /* Set new time */
    return ptp_hw_set_time(enet_instance, &new_time);
}

int ptp_hw_set_addend(uint8_t enet_instance, uint32_t addend)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Write addend value */
    write_reg(base, ENET_MAC_TAR_OFFSET, addend);
    
    /* Trigger addend register update */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val |= MAC_TSCR_TSADDREG;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    /* Wait for update complete */
    volatile uint32_t timeout = 1000;
    while (timeout--) {
        reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
        if (!(reg_val & MAC_TSCR_TSADDREG)) {
            return 0;
        }
    }
    
    return -1;  /* Timeout */
}

int ptp_hw_adj_freq(uint8_t enet_instance, int32_t ppb)
{
    int64_t addend;
    int64_t diff;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    /* Clamp ppb to reasonable range */
    if (ppb > 10000000) ppb = 10000000;   /* +10 ppm */
    if (ppb < -10000000) ppb = -10000000; /* -10 ppm */
    
    /* Calculate new addend value */
    /* addend = base_addend * (1 + ppb/1e9) */
    addend = (int64_t)PTP_ADDEND_BASE;
    diff = (addend * ppb) / 1000000000LL;
    addend += diff;
    
    /* Clamp to valid range */
    if (addend < 0) addend = 0;
    if (addend > 0xFFFFFFFFLL) addend = 0xFFFFFFFFLL;
    
    return ptp_hw_set_addend(enet_instance, (uint32_t)addend);
}

int ptp_hw_update_addend(uint8_t enet_instance, int32_t ppb)
{
    /* Same as adj_freq - provides interface consistency */
    return ptp_hw_adj_freq(enet_instance, ppb);
}

int ptp_hw_config_capture(uint8_t enet_instance, ts_capture_point_t point, bool enable)
{
    uint32_t base;
    uint32_t reg_val;
    uint32_t mask = 0;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    
    /* Configure based on capture point */
    switch (point) {
        case TS_CAP_PTP_EVENT_RX:
        case TS_CAP_ALL_RX:
            /* Enable timestamp for all RX frames */
            if (enable) {
                reg_val |= MAC_TSCR_TSENALL;
            } else {
                reg_val &= ~MAC_TSCR_TSENALL;
            }
            break;
            
        case TS_CAP_PTP_EVENT_TX:
        case TS_CAP_ALL_TX:
            /* TX timestamping enabled in descriptor */
            break;
            
        default:
            break;
    }
    
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    return 0;
}

bool ptp_hw_tx_timestamp_ready(uint8_t enet_instance)
{
    /* TX timestamp availability is indicated in descriptor status */
    /* This is a simplified check - real implementation would check descriptor */
    (void)enet_instance;
    return true;  /* Placeholder */
}

bool ptp_hw_rx_timestamp_ready(uint8_t enet_instance)
{
    /* RX timestamp availability is indicated in descriptor status */
    (void)enet_instance;
    return true;  /* Placeholder */
}

int ptp_hw_get_tx_timestamp(uint8_t enet_instance, uint64_t *timestamp)
{
    enet_handle_t *handle;
    enet_tx_desc_t *desc;
    uint64_t seconds;
    uint32_t nanoseconds;
    
    if (!timestamp || enet_instance > 2) {
        return -1;
    }
    
    /* Get handle from enet driver */
    /* Note: In real implementation, this would need proper access to handle */
    handle = NULL;  /* Would be retrieved from enet driver */
    
    if (!handle || !handle->tx_desc) {
        return -1;
    }
    
    /* Get descriptor with timestamp */
    desc = &handle->tx_desc[handle->tx_cons_idx];
    
    /* Check if timestamp is valid (TDES0_TTSS bit) */
    if (!(desc->tdes0 & TDES0_TTSS)) {
        return -1;  /* Timestamp not ready */
    }
    
    /* Extract timestamp from descriptor */
    nanoseconds = desc->tdes6;
    seconds = desc->tdes7;
    
    /* Convert to nanoseconds */
    *timestamp = (seconds * 1000000000ULL) + nanoseconds;
    
    return 0;
}

int ptp_hw_get_rx_timestamp(uint8_t enet_instance, uint64_t *timestamp)
{
    enet_handle_t *handle;
    enet_rx_desc_t *desc;
    uint64_t seconds;
    uint32_t nanoseconds;
    
    if (!timestamp || enet_instance > 2) {
        return -1;
    }
    
    /* Get handle from enet driver */
    handle = NULL;  /* Would be retrieved from enet driver */
    
    if (!handle || !handle->rx_desc) {
        return -1;
    }
    
    /* Get descriptor with timestamp */
    desc = &handle->rx_desc[handle->rx_cons_idx];
    
    /* Check if timestamp is available (RDES0_TS bit) */
    if (!(desc->rdes0 & RDES0_TS)) {
        return -1;  /* Timestamp not available */
    }
    
    /* Extract timestamp from descriptor */
    nanoseconds = desc->rdes6;
    seconds = desc->rdes7;
    
    /* Convert to nanoseconds */
    *timestamp = (seconds * 1000000000ULL) + nanoseconds;
    
    return 0;
}

int ptp_hw_set_one_step(uint8_t enet_instance, bool enable)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* One-step clock is controlled via additional configuration */
    /* This is platform-specific for S32G3 */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    
    if (enable) {
        /* Enable one-step operation */
        reg_val |= (1UL << 10);  /* Enable one-step timestamp */
    } else {
        /* Two-step operation (default) */
        reg_val &= ~(1UL << 10);
    }
    
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    return 0;
}

bool ptp_hw_is_one_step(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return false;
    }
    
    base = get_enet_base(enet_instance);
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    
    return (reg_val & (1UL << 10)) != 0;
}

int ptp_hw_enable(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val |= MAC_TSCR_TSENA;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    return 0;
}

int ptp_hw_disable(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val &= ~MAC_TSCR_TSENA;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    return 0;
}

int ptp_hw_config_pps(uint8_t enet_instance, uint32_t period_ns)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Configure PPS output frequency */
    /* period_ns defines the PPS period in nanoseconds */
    /* For 1 PPS: period_ns = 1000000000 */
    
    reg_val = read_reg(base, ENET_MAC_PPSCR_OFFSET);
    
    /* Clear PPS control bits */
    reg_val &= ~0x0F;
    
    if (period_ns == 1000000000UL) {
        /* 1 PPS output */
        reg_val |= 0x01;
    } else if (period_ns == 500000000UL) {
        /* 2 Hz output */
        reg_val |= 0x02;
    } else if (period_ns == 250000000UL) {
        /* 4 Hz output */
        reg_val |= 0x03;
    } else {
        /* Flexible mode - would require target time programming */
        reg_val |= 0x0F;
    }
    
    write_reg(base, ENET_MAC_PPSCR_OFFSET, reg_val);
    return 0;
}

int ptp_hw_enable_pps(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Enable PPS output */
    reg_val = read_reg(base, ENET_MAC_PPSCR_OFFSET);
    reg_val |= (1UL << 4);
    write_reg(base, ENET_MAC_PPSCR_OFFSET, reg_val);
    
    return 0;
}

int ptp_hw_disable_pps(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Disable PPS output */
    reg_val = read_reg(base, ENET_MAC_PPSCR_OFFSET);
    reg_val &= ~(1UL << 4);
    write_reg(base, ENET_MAC_PPSCR_OFFSET, reg_val);
    
    return 0;
}

int ptp_hw_enable_irq(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Enable timestamp trigger interrupt */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val |= MAC_TSCR_TSTRIG;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    return 0;
}

int ptp_hw_disable_irq(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (enet_instance > 2) {
        return -1;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Disable timestamp trigger interrupt */
    reg_val = read_reg(base, ENET_MAC_TSCR_OFFSET);
    reg_val &= ~MAC_TSCR_TSTRIG;
    write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    
    return 0;
}

void ptp_hw_irq_handler(uint8_t enet_instance)
{
    uint32_t base;
    uint32_t status;
    
    if (enet_instance > 2) {
        return;
    }
    
    base = get_enet_base(enet_instance);
    
    /* Read timestamp status */
    status = read_reg(base, ENET_MAC_TSSR_OFFSET);
    
    /* Handle overflow */
    if (status & MAC_TSSR_TSSOVF) {
        /* Seconds counter overflow - may need to handle */
    }
    
    /* Handle target time reached */
    if (status & MAC_TSSR_TSTARGT) {
        /* Target time reached - signal to application */
    }
    
    /* Clear status bits by writing back */
    write_reg(base, ENET_MAC_TSSR_OFFSET, status);
}
