/*
 * mdio_bus.c
 * MDIO Bus Management Implementation
 */

#include "mdio_bus.h"
#include <string.h>

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static int mdio_wait_ready(mdio_bus_handle_t *bus)
{
    (void)bus;
    /* Hardware-specific wait - default implementation */
    volatile uint32_t timeout = 10000;
    while (timeout--);
    return 0;
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void mdio_bus_get_default_config(mdio_bus_handle_t *bus)
{
    if (!bus) return;
    
    memset(bus, 0, sizeof(mdio_bus_handle_t));
    
    bus->clock_freq = 2500000;      /* 2.5 MHz default MDIO clock */
    bus->use_c45 = false;            /* Default to Clause 22 */
    bus->phy_addr_mask = 0xFFFFFFFF; /* All addresses potentially valid */
}

int mdio_bus_init(mdio_bus_handle_t *bus)
{
    if (!bus) {
        return -1;
    }
    
    /* Reset statistics */
    bus->access_count = 0;
    bus->error_count = 0;
    
    return 0;
}

int mdio_bus_deinit(mdio_bus_handle_t *bus)
{
    if (!bus) {
        return -1;
    }
    
    /* Cleanup if needed */
    
    return 0;
}

int mdio_c22_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t reg_addr, uint16_t *value)
{
    if (!bus || !value) {
        return -1;
    }
    
    if (phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    if (!bus->read_c22) {
        return -1;
    }
    
    bus->access_count++;
    
    int ret = bus->read_c22(phy_addr, reg_addr, value);
    
    if (ret != 0) {
        bus->error_count++;
    }
    
    return ret;
}

int mdio_c22_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    if (!bus) {
        return -1;
    }
    
    if (phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    if (!bus->write_c22) {
        return -1;
    }
    
    bus->access_count++;
    
    int ret = bus->write_c22(phy_addr, reg_addr, value);
    
    if (ret != 0) {
        bus->error_count++;
    }
    
    return ret;
}

int mdio_c45_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t dev_addr, 
                   uint16_t reg_addr, uint16_t *value)
{
    if (!bus || !value) {
        return -1;
    }
    
    if (phy_addr > 31 || dev_addr > 31) {
        return -1;
    }
    
    if (!bus->read_c45) {
        return -1;
    }
    
    bus->access_count++;
    
    int ret = bus->read_c45(phy_addr, dev_addr, reg_addr, value);
    
    if (ret != 0) {
        bus->error_count++;
    }
    
    return ret;
}

int mdio_c45_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t dev_addr, 
                    uint16_t reg_addr, uint16_t value)
{
    if (!bus) {
        return -1;
    }
    
    if (phy_addr > 31 || dev_addr > 31) {
        return -1;
    }
    
    if (!bus->write_c45) {
        return -1;
    }
    
    bus->access_count++;
    
    int ret = bus->write_c45(phy_addr, dev_addr, reg_addr, value);
    
    if (ret != 0) {
        bus->error_count++;
    }
    
    return ret;
}

int mdio_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t reg_addr, uint16_t *value)
{
    if (!bus || !value) {
        return -1;
    }
    
    if (bus->use_c45 && reg_addr > 0xFFFF) {
        /* Clause 45 access with device address in upper bits */
        uint8_t dev_addr = (uint8_t)((reg_addr >> 16) & 0x1F);
        uint16_t reg = (uint16_t)(reg_addr & 0xFFFF);
        return mdio_c45_read(bus, phy_addr, dev_addr, reg, value);
    } else {
        /* Clause 22 access */
        return mdio_c22_read(bus, phy_addr, (uint8_t)reg_addr, value);
    }
}

int mdio_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t reg_addr, uint16_t value)
{
    if (!bus) {
        return -1;
    }
    
    if (bus->use_c45 && reg_addr > 0xFFFF) {
        /* Clause 45 access */
        uint8_t dev_addr = (uint8_t)((reg_addr >> 16) & 0x1F);
        uint16_t reg = (uint16_t)(reg_addr & 0xFFFF);
        return mdio_c45_write(bus, phy_addr, dev_addr, reg, value);
    } else {
        /* Clause 22 access */
        return mdio_c22_write(bus, phy_addr, (uint8_t)reg_addr, value);
    }
}

int mdio_scan_bus(mdio_bus_handle_t *bus, uint32_t *phy_mask)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus || !phy_mask) {
        return -1;
    }
    
    *phy_mask = 0;
    
    for (uint8_t addr = 0; addr < 32; addr++) {
        /* Try to read PHY ID register */
        ret = mdio_c22_read(bus, addr, MDIO_REG_PHY_ID1, &reg_val);
        
        if (ret == 0 && reg_val != 0xFFFF && reg_val != 0x0000) {
            /* Valid PHY found */
            *phy_mask |= (1UL << addr);
        }
    }
    
    return 0;
}

int mdio_get_phy_id(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t *phy_id)
{
    uint16_t id1, id2;
    int ret;
    
    if (!bus || !phy_id) {
        return -1;
    }
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_PHY_ID1, &id1);
    if (ret != 0) return ret;
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_PHY_ID2, &id2);
    if (ret != 0) return ret;
    
    *phy_id = ((uint32_t)id1 << 16) | id2;
    
    return 0;
}

int mdio_reset_phy(mdio_bus_handle_t *bus, uint8_t phy_addr)
{
    int ret;
    uint16_t reg_val;
    
    if (!bus) {
        return -1;
    }
    
    /* Set reset bit */
    ret = mdio_c22_write(bus, phy_addr, MDIO_REG_CTRL, MDIO_CTRL_RESET);
    if (ret != 0) return ret;
    
    /* Wait for reset complete (bit clears when done) */
    volatile uint32_t timeout = 10000;
    do {
        ret = mdio_c22_read(bus, phy_addr, MDIO_REG_CTRL, &reg_val);
        if (ret != 0) return ret;
    } while ((reg_val & MDIO_CTRL_RESET) && --timeout);
    
    if (timeout == 0) {
        return -1;  /* Timeout */
    }
    
    return 0;
}

int mdio_get_link_status(mdio_bus_handle_t *bus, uint8_t phy_addr, bool *link_up)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus || !link_up) {
        return -1;
    }
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_STAT, &reg_val);
    if (ret != 0) return ret;
    
    *link_up = (reg_val & MDIO_STAT_LINK_STAT) != 0;
    
    return 0;
}

int mdio_set_speed_duplex(mdio_bus_handle_t *bus, uint8_t phy_addr, uint16_t speed, bool full_duplex)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus) {
        return -1;
    }
    
    /* Read current control register */
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    /* Clear speed and duplex bits */
    reg_val &= ~(MDIO_CTRL_SPEED_SEL | MDIO_CTRL_SPEED1000 | MDIO_CTRL_DUPLEX);
    
    /* Set new speed */
    switch (speed) {
        case 1000:
            reg_val |= MDIO_CTRL_SPEED1000;
            break;
        case 100:
            reg_val |= MDIO_CTRL_SPEED_SEL;
            break;
        case 10:
            /* 10Mbps - no speed bits set */
            break;
        default:
            return -1;  /* Invalid speed */
    }
    
    /* Set duplex */
    if (full_duplex) {
        reg_val |= MDIO_CTRL_DUPLEX;
    }
    
    /* Disable auto-negotiation for manual configuration */
    reg_val &= ~MDIO_CTRL_AN_EN;
    
    ret = mdio_c22_write(bus, phy_addr, MDIO_REG_CTRL, reg_val);
    
    return ret;
}

int mdio_enable_autoneg(mdio_bus_handle_t *bus, uint8_t phy_addr)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus) {
        return -1;
    }
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= MDIO_CTRL_AN_EN;
    
    ret = mdio_c22_write(bus, phy_addr, MDIO_REG_CTRL, reg_val);
    
    return ret;
}

int mdio_restart_autoneg(mdio_bus_handle_t *bus, uint8_t phy_addr)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus) {
        return -1;
    }
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    reg_val |= MDIO_CTRL_RESTART_AN;
    
    ret = mdio_c22_write(bus, phy_addr, MDIO_REG_CTRL, reg_val);
    
    return ret;
}

int mdio_power_down(mdio_bus_handle_t *bus, uint8_t phy_addr, bool down)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus) {
        return -1;
    }
    
    ret = mdio_c22_read(bus, phy_addr, MDIO_REG_CTRL, &reg_val);
    if (ret != 0) return ret;
    
    if (down) {
        reg_val |= MDIO_CTRL_PWR_DOWN;
    } else {
        reg_val &= ~MDIO_CTRL_PWR_DOWN;
    }
    
    ret = mdio_c22_write(bus, phy_addr, MDIO_REG_CTRL, reg_val);
    
    return ret;
}

void mdio_dump_phy_regs(mdio_bus_handle_t *bus, uint8_t phy_addr)
{
    uint16_t reg_val;
    int ret;
    
    if (!bus) return;
    
    /* Dump standard MII registers */
    for (uint8_t reg = 0; reg < 16; reg++) {
        ret = mdio_c22_read(bus, phy_addr, reg, &reg_val);
        if (ret == 0) {
            (void)reg_val;  /* Would print register value */
        }
    }
}

const char *mdio_speed_to_str(uint16_t speed)
{
    switch (speed) {
        case 10:    return "10Mbps";
        case 100:   return "100Mbps";
        case 1000:  return "1000Mbps";
        case 2500:  return "2.5Gbps";
        case 5000:  return "5Gbps";
        case 10000: return "10Gbps";
        default:    return "Unknown";
    }
}
