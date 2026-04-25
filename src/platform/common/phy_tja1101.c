/*
 * phy_tja1101.c
 * NXP TJA1101 100BASE-T1 PHY Driver Implementation
 */

#include "phy_tja1101.h"
#include <string.h>

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static int tja1101_read_reg(tja1101_handle_t *handle, uint8_t reg, uint16_t *value)
{
    if (!handle || !value || !handle->mdio_read) {
        return -1;
    }
    
    *value = handle->mdio_read(handle->phy_addr, reg);
    return 0;
}

static int tja1101_write_reg(tja1101_handle_t *handle, uint8_t reg, uint16_t value)
{
    if (!handle || !handle->mdio_write) {
        return -1;
    }
    
    return handle->mdio_write(handle->phy_addr, reg, value);
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void tja1101_get_default_config(tja1101_config_t *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(tja1101_config_t));
    
    config->mode = TJA1101_MODE_SLAVE;      /* Default to slave mode */
    config->auto_negotiation = true;        /* Enable auto-negotiation */
    config->tx_amplitude = 8;               /* Default amplitude */
    config->led_enable = false;             /* LED disabled by default */
    config->fusa_enable = false;            /* FuSa disabled by default */
}

int tja1101_init(tja1101_handle_t *handle, const tja1101_config_t *config)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !config) {
        return -1;
    }
    
    /* Verify PHY presence */
    bool is_tja1101;
    ret = tja1101_identify(handle, &is_tja1101);
    if (ret != 0 || !is_tja1101) {
        return -1;
    }
    
    /* Software reset */
    ret = tja1101_write_reg(handle, MII_CTRL_REG, MII_CTRL_RESET);
    if (ret != 0) return ret;
    
    /* Wait for reset complete */
    volatile uint32_t timeout = 10000;
    do {
        ret = tja1101_read_reg(handle, MII_CTRL_REG, &reg_val);
        if (ret != 0) return ret;
    } while ((reg_val & MII_CTRL_RESET) && --timeout);
    
    if (timeout == 0) {
        return -1;  /* Reset timeout */
    }
    
    /* Configure extended control register */
    reg_val = TJA1101_EXT_CTRL_CONFIG_EN;   /* Enable configuration */
    
    /* Set master/slave mode */
    if (config->mode == TJA1101_MODE_MASTER) {
        reg_val |= TJA1101_EXT_CTRL_PHY_MODE;
    }
    
    ret = tja1101_write_reg(handle, TJA1101_EXT_CTRL, reg_val);
    if (ret != 0) return ret;
    
    /* Configure configuration register 1 */
    reg_val = 0;
    
    if (config->mode == TJA1101_MODE_MASTER) {
        reg_val |= TJA1101_CFG1_MASTER;
    }
    
    reg_val |= ((config->tx_amplitude << 5) & TJA1101_CFG1_TX_AMP_MASK);
    
    if (config->led_enable) {
        reg_val |= TJA1101_CFG1_LED_EN;
    }
    
    ret = tja1101_write_reg(handle, TJA1101_CONFIG1, reg_val);
    if (ret != 0) return ret;
    
    /* Configure MII control register */
    reg_val = MII_CTRL_SPEED_SEL | MII_CTRL_DUPLEX;  /* 100Mbps Full Duplex */
    
    if (config->auto_negotiation) {
        reg_val |= MII_CTRL_AN_EN;
    }
    
    ret = tja1101_write_reg(handle, MII_CTRL_REG, reg_val);
    if (ret != 0) return ret;
    
    /* Start auto-negotiation if enabled */
    if (config->auto_negotiation) {
        ret = tja1101_write_reg(handle, MII_CTRL_REG, reg_val | MII_CTRL_RESTART_AN);
        if (ret != 0) return ret;
    }
    
    /* Enable interrupts */
    uint16_t int_mask = TJA1101_INT_LINK_FAIL | TJA1101_INT_LINK_UP;
    ret = tja1101_write_reg(handle, TJA1101_INT_EN, int_mask);
    if (ret != 0) return ret;
    
    /* Exit configuration mode and enable link */
    ret = tja1101_read_reg(handle, TJA1101_EXT_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    reg_val &= ~TJA1101_EXT_CTRL_CONFIG_EN;     /* Clear config enable */
    reg_val |= TJA1101_EXT_CTRL_LINK_CTRL;      /* Enable link control */
    reg_val |= TJA1101_PM_NORMAL;               /* Normal power mode */
    
    ret = tja1101_write_reg(handle, TJA1101_EXT_CTRL, reg_val);
    if (ret != 0) return ret;
    
    return 0;
}

int tja1101_deinit(tja1101_handle_t *handle)
{
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    /* Power down the PHY */
    uint16_t reg_val;
    ret = tja1101_read_reg(handle, MII_CTRL_REG, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= MII_CTRL_PWR_DOWN;
    ret = tja1101_write_reg(handle, MII_CTRL_REG, reg_val);
    
    return ret;
}

int tja1101_identify(tja1101_handle_t *handle, bool *is_tja1101)
{
    uint16_t id1, id2;
    int ret;
    
    if (!handle || !is_tja1101) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, MII_PHY_ID1_REG, &id1);
    if (ret != 0) return ret;
    
    ret = tja1101_read_reg(handle, MII_PHY_ID2_REG, &id2);
    if (ret != 0) return ret;
    
    /* Check PHY ID */
    *is_tja1101 = (id1 == TJA1101_PHY_ID_1) && 
                  ((id2 & TJA1101_PHY_ID_2_MASK) == TJA1101_PHY_ID_2);
    
    return 0;
}

int tja1101_get_link_status(tja1101_handle_t *handle, bool *link_up)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !link_up) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, MII_STAT_REG, &reg_val);
    if (ret != 0) return ret;
    
    *link_up = (reg_val & MII_STAT_LINK_STAT) != 0;
    
    return 0;
}

int tja1101_get_link_speed(tja1101_handle_t *handle, uint32_t *speed_mbps)
{
    (void)handle;
    
    if (!speed_mbps) {
        return -1;
    }
    
    /* TJA1101 is always 100Mbps */
    *speed_mbps = 100;
    
    return 0;
}

int tja1101_restart_autoneg(tja1101_handle_t *handle)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, MII_CTRL_REG, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= MII_CTRL_RESTART_AN;
    ret = tja1101_write_reg(handle, MII_CTRL_REG, reg_val);
    
    return ret;
}

int tja1101_set_power_mode(tja1101_handle_t *handle, uint8_t power_mode)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || power_mode > TJA1101_PM_SILENT) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_EXT_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    /* Clear power mode bits and set new mode */
    reg_val &= ~TJA1101_EXT_CTRL_POWER_MODE_MASK;
    reg_val |= (power_mode & TJA1101_EXT_CTRL_POWER_MODE_MASK);
    
    ret = tja1101_write_reg(handle, TJA1101_EXT_CTRL, reg_val);
    
    return ret;
}

int tja1101_get_power_mode(tja1101_handle_t *handle, uint8_t *power_mode)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !power_mode) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_EXT_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    *power_mode = reg_val & TJA1101_EXT_CTRL_POWER_MODE_MASK;
    
    return 0;
}

int tja1101_enter_sleep(tja1101_handle_t *handle)
{
    return tja1101_set_power_mode(handle, TJA1101_PM_SLEEP);
}

int tja1101_wake_up(tja1101_handle_t *handle)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    /* Send wake request */
    ret = tja1101_read_reg(handle, TJA1101_EXT_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= TJA1101_EXT_CTRL_WAKE_REQ;
    ret = tja1101_write_reg(handle, TJA1101_EXT_CTRL, reg_val);
    
    return ret;
}

int tja1101_get_sqi(tja1101_handle_t *handle, uint8_t *sqi)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !sqi) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_COMM_STAT, &reg_val);
    if (ret != 0) return ret;
    
    *sqi = (reg_val & TJA1101_COMM_SQI_MASK) >> 0;
    
    return 0;
}

int tja1101_get_symbol_errors(tja1101_handle_t *handle, uint16_t *errors)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !errors) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_SYM_ERR_COUNTER, &reg_val);
    if (ret != 0) return ret;
    
    *errors = reg_val;
    
    return 0;
}

int tja1101_enable_interrupts(tja1101_handle_t *handle, uint16_t mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_INT_EN, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= mask;
    ret = tja1101_write_reg(handle, TJA1101_INT_EN, reg_val);
    
    return ret;
}

int tja1101_disable_interrupts(tja1101_handle_t *handle, uint16_t mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_INT_EN, &reg_val);
    if (ret != 0) return ret;
    
    reg_val &= ~mask;
    ret = tja1101_write_reg(handle, TJA1101_INT_EN, reg_val);
    
    return ret;
}

int tja1101_get_interrupt_status(tja1101_handle_t *handle, uint16_t *status)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !status) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_INT_SRC, &reg_val);
    if (ret != 0) return ret;
    
    *status = reg_val;
    
    return 0;
}

int tja1101_clear_interrupts(tja1101_handle_t *handle, uint16_t mask)
{
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    /* Write back status to clear (W1C - Write 1 to Clear) */
    ret = tja1101_write_reg(handle, TJA1101_INT_SRC, mask);
    
    return ret;
}

int tja1101_set_master_slave(tja1101_handle_t *handle, tja1101_mode_t mode)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_CONFIG1, &reg_val);
    if (ret != 0) return ret;
    
    if (mode == TJA1101_MODE_MASTER) {
        reg_val |= TJA1101_CFG1_MASTER;
    } else {
        reg_val &= ~TJA1101_CFG1_MASTER;
    }
    
    ret = tja1101_write_reg(handle, TJA1101_CONFIG1, reg_val);
    
    return ret;
}

int tja1101_get_phy_state(tja1101_handle_t *handle, uint8_t *state)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !state) {
        return -1;
    }
    
    ret = tja1101_read_reg(handle, TJA1101_COMM_STAT, &reg_val);
    if (ret != 0) return ret;
    
    *state = (reg_val & TJA1101_COMM_PHY_STATE_MASK) >> 10;
    
    return 0;
}

int tja1101_run_cable_test(tja1101_handle_t *handle)
{
    (void)handle;
    
    /* TJA1101 does not support cable diagnostic test */
    /* This is a placeholder for future implementations */
    
    return -1;
}

void tja1101_print_status(tja1101_handle_t *handle)
{
    uint16_t reg_val;
    bool link_up;
    uint8_t sqi;
    uint8_t state;
    
    if (!handle) return;
    
    /* Link status */
    tja1101_get_link_status(handle, &link_up);
    
    /* SQI */
    tja1101_get_sqi(handle, &sqi);
    
    /* PHY state */
    tja1101_get_phy_state(handle, &state);
    
    /* Symbol errors */
    uint16_t sym_errors;
    tja1101_get_symbol_errors(handle, &sym_errors);
    
    /* Communication status for detailed info */
    tja1101_read_reg(handle, TJA1101_COMM_STAT, &reg_val);
    
    (void)reg_val;  /* Would be used for logging */
    (void)link_up;
    (void)sqi;
    (void)state;
    (void)sym_errors;
}
