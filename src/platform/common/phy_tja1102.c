/*
 * phy_tja1102.c
 * NXP TJA1102 Dual-Port PHY Driver Implementation
 */

#include "phy_tja1102.h"
#include <string.h>

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static int tja1102_read_reg(tja1102_handle_t *handle, uint8_t phy_addr, uint8_t reg, uint16_t *value)
{
    if (!handle || !value || !handle->mdio_read) {
        return -1;
    }
    
    *value = handle->mdio_read(phy_addr, reg);
    return 0;
}

static int tja1102_write_reg(tja1102_handle_t *handle, uint8_t phy_addr, uint8_t reg, uint16_t value)
{
    if (!handle || !handle->mdio_write) {
        return -1;
    }
    
    return handle->mdio_write(phy_addr, reg, value);
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void tja1102_get_default_config(tja1102_config_t *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(tja1102_config_t));
    
    /* Default configuration for both ports */
    tja1101_get_default_config(&config->port_config[0]);
    tja1101_get_default_config(&config->port_config[1]);
    
    /* Default port 0 as master, port 1 as slave (for switch/bridge config) */
    config->port_config[0].mode = TJA1101_MODE_MASTER;
    config->port_config[1].mode = TJA1101_MODE_SLAVE;
    
    /* Shared configuration */
    config->shared_ref_clock = true;
    config->ref_clock_sel = 0;      /* Default ref clock */
    config->clock_output_en = false;
    config->failsafe_en = false;
}

int tja1102_init(tja1102_handle_t *handle, const tja1102_config_t *config)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !config) {
        return -1;
    }
    
    /* Verify PHY presence */
    bool is_tja1102;
    ret = tja1102_identify(handle, &is_tja1102);
    if (ret != 0 || !is_tja1102) {
        return -1;
    }
    
    /* Configure shared registers first (via port 0) */
    reg_val = 0;
    reg_val |= (config->ref_clock_sel & 0x03);
    
    if (config->clock_output_en) {
        reg_val |= TJA1102_SHARED_CFG_CLK_OUT_EN;
    }
    
    if (config->failsafe_en) {
        reg_val |= TJA1102_SHARED_CFG_FAILSAFE_EN;
    }
    
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CONFIG, reg_val);
    if (ret != 0) return ret;
    
    /* Initialize both ports */
    for (int port = 0; port < 2; port++) {
        ret = tja1102_init_port(handle, port, &config->port_config[port]);
        if (ret != 0) {
            return ret;
        }
    }
    
    return 0;
}

int tja1102_deinit(tja1102_handle_t *handle)
{
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    /* Power down both ports */
    for (int port = 0; port < 2; port++) {
        uint8_t phy_addr = handle->base_phy_addr + port;
        uint16_t reg_val;
        
        ret = tja1102_read_reg(handle, phy_addr, MII_CTRL_REG, &reg_val);
        if (ret == 0) {
            reg_val |= MII_CTRL_PWR_DOWN;
            tja1102_write_reg(handle, phy_addr, MII_CTRL_REG, reg_val);
        }
    }
    
    return 0;
}

int tja1102_identify(tja1102_handle_t *handle, bool *is_tja1102)
{
    uint16_t id1, id2;
    int ret;
    
    if (!handle || !is_tja1102) {
        return -1;
    }
    
    /* Read from port 0 */
    ret = tja1102_read_reg(handle, handle->base_phy_addr, MII_PHY_ID1_REG, &id1);
    if (ret != 0) return ret;
    
    ret = tja1102_read_reg(handle, handle->base_phy_addr, MII_PHY_ID2_REG, &id2);
    if (ret != 0) return ret;
    
    /* Check PHY ID */
    *is_tja1102 = (id1 == TJA1102_PHY_ID_1) && 
                  ((id2 & TJA1102_PHY_ID_2_MASK) == TJA1102_PHY_ID_2);
    
    return 0;
}

int tja1102_get_port_phy_addr(tja1102_handle_t *handle, uint8_t port, uint8_t *phy_addr)
{
    if (!handle || !phy_addr || port > 1) {
        return -1;
    }
    
    *phy_addr = handle->base_phy_addr + port;
    return 0;
}

int tja1102_init_port(tja1102_handle_t *handle, uint8_t port, const tja1101_config_t *config)
{
    tja1101_handle_t port_handle;
    
    if (!handle || !config || port > 1) {
        return -1;
    }
    
    /* Create handle for this port */
    port_handle.phy_addr = handle->base_phy_addr + port;
    port_handle.mdio_read = handle->mdio_read;
    port_handle.mdio_write = handle->mdio_write;
    
    /* Use TJA1101 init for the port */
    return tja1101_init(&port_handle, config);
}

int tja1102_get_port_link_status(tja1102_handle_t *handle, uint8_t port, bool *link_up)
{
    tja1101_handle_t port_handle;
    
    if (!handle || !link_up || port > 1) {
        return -1;
    }
    
    port_handle.phy_addr = handle->base_phy_addr + port;
    port_handle.mdio_read = handle->mdio_read;
    port_handle.mdio_write = handle->mdio_write;
    
    return tja1101_get_link_status(&port_handle, link_up);
}

int tja1102_get_port_sqi(tja1102_handle_t *handle, uint8_t port, uint8_t *sqi)
{
    tja1101_handle_t port_handle;
    
    if (!handle || !sqi || port > 1) {
        return -1;
    }
    
    port_handle.phy_addr = handle->base_phy_addr + port;
    port_handle.mdio_read = handle->mdio_read;
    port_handle.mdio_write = handle->mdio_write;
    
    return tja1101_get_sqi(&port_handle, sqi);
}

int tja1102_shared_reset(tja1102_handle_t *handle, uint8_t port_mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1102_read_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    /* Set reset bits for selected ports */
    if (port_mask & (1 << 0)) {
        reg_val |= TJA1102_SHARED_CTRL_RESET_PORT0;
    }
    if (port_mask & (1 << 1)) {
        reg_val |= TJA1102_SHARED_CTRL_RESET_PORT1;
    }
    
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, reg_val);
    if (ret != 0) return ret;
    
    /* Clear reset bits after short delay */
    for (volatile int d = 0; d < 1000; d++);
    
    reg_val &= ~(TJA1102_SHARED_CTRL_RESET_PORT0 | TJA1102_SHARED_CTRL_RESET_PORT1);
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, reg_val);
    
    return ret;
}

int tja1102_shared_wake(tja1102_handle_t *handle, uint8_t port_mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1102_read_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    /* Set wake bits */
    if (port_mask & (1 << 0)) {
        reg_val |= TJA1102_SHARED_CTRL_WAKE_PORT0;
    }
    if (port_mask & (1 << 1)) {
        reg_val |= TJA1102_SHARED_CTRL_WAKE_PORT1;
    }
    
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, reg_val);
    
    return ret;
}

int tja1102_shared_sleep_request(tja1102_handle_t *handle, uint8_t port_mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    ret = tja1102_read_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    /* Set sleep request bits */
    if (port_mask & (1 << 0)) {
        reg_val |= TJA1102_SHARED_CTRL_SLEEP_REQ_PORT0;
    }
    if (port_mask & (1 << 1)) {
        reg_val |= TJA1102_SHARED_CTRL_SLEEP_REQ_PORT1;
    }
    
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_CTRL, reg_val);
    
    return ret;
}

int tja1102_get_shared_interrupts(tja1102_handle_t *handle, uint8_t *int_mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!handle || !int_mask) {
        return -1;
    }
    
    ret = tja1102_read_reg(handle, handle->base_phy_addr, TJA1102_SHARED_STAT, &reg_val);
    if (ret != 0) return ret;
    
    *int_mask = (uint8_t)(reg_val & 0x0F);
    
    return 0;
}

int tja1102_clear_shared_interrupts(tja1102_handle_t *handle, uint8_t int_mask)
{
    int ret;
    
    if (!handle) {
        return -1;
    }
    
    /* Write to clear (W1C) bits */
    ret = tja1102_write_reg(handle, handle->base_phy_addr, TJA1102_SHARED_STAT, int_mask);
    
    return ret;
}

void tja1102_print_status(tja1102_handle_t *handle)
{
    uint8_t int_mask;
    
    if (!handle) return;
    
    /* Print status for both ports */
    for (int port = 0; port < 2; port++) {
        tja1101_handle_t port_handle;
        port_handle.phy_addr = handle->base_phy_addr + port;
        port_handle.mdio_read = handle->mdio_read;
        port_handle.mdio_write = handle->mdio_write;
        
        (void)port;  /* Use in actual logging */
        tja1101_print_status(&port_handle);
    }
    
    /* Print shared interrupt status */
    tja1102_get_shared_interrupts(handle, &int_mask);
    (void)int_mask;
}
