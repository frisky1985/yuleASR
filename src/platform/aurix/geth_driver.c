/*
 * geth_driver.c
 * Infineon AURIX TC3xx GETH Driver Implementation
 */

#include "geth_driver.h"
#include <string.h>

/*==============================================================================
 * Static Variables
 *============================================================================*/
static geth_handle_t geth_handles[2];   /* Support 2 GETH instances */

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

static uint32_t geth_get_base_addr(uint8_t instance)
{
    switch(instance) {
        case 0: return GETH0_BASE_ADDR;
        case 1: return GETH1_BASE_ADDR;
        default: return 0;
    }
}

static inline uint32_t geth_read_reg(uint32_t base, uint32_t offset)
{
    return *((volatile uint32_t *)(base + offset));
}

static inline void geth_write_reg(uint32_t base, uint32_t offset, uint32_t value)
{
    *((volatile uint32_t *)(base + offset)) = value;
}

static int geth_wait_mdio_ready(uint32_t base)
{
    volatile uint32_t timeout = 10000;
    
    while (timeout--) {
        if (!(geth_read_reg(base, GETH_MAC_MII_ADDR_OFFSET) & MII_ADDR_GB)) {
            return 0;
        }
    }
    return -1;
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void geth_get_default_config(geth_config_t *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(geth_config_t));
    
    config->speed = GETH_SPEED_100M;    /* TC3xx typically 100M with TJA1101 */
    config->duplex = GETH_DUPLEX_FULL;
    config->interface = GETH_RMII;
    config->enable_1588 = true;
    config->enable_vlan = false;
    config->enable_jumbo = false;
    
    config->mac_addr[0] = 0x00;
    config->mac_addr[1] = 0x04;
    config->mac_addr[2] = 0x9F;
    config->mac_addr[3] = 0x00;
    config->mac_addr[4] = 0x00;
    config->mac_addr[5] = 0x01;
    
    config->rx_desc_count = 16;
    config->tx_desc_count = 16;
    config->rx_buffer_size = 1536;
    config->tx_buffer_size = 1536;
}

int geth_init(uint8_t instance, const geth_config_t *config)
{
    uint32_t base;
    uint32_t reg_val;
    geth_handle_t *handle;
    
    if (!config || instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    if (!base) {
        return -1;
    }
    
    handle = &geth_handles[instance];
    memset(handle, 0, sizeof(geth_handle_t));
    handle->base_addr = base;
    handle->tx_desc_num = config->tx_desc_count;
    handle->rx_desc_num = config->rx_desc_count;
    
    /* 1. Software reset DMA */
    geth_write_reg(base, GETH_DMA_BUS_MODE_OFFSET, DMA_BM_SWR);
    
    volatile uint32_t timeout = 10000;
    while (geth_read_reg(base, GETH_DMA_BUS_MODE_OFFSET) & DMA_BM_SWR) {
        if (--timeout == 0) {
            return -1;
        }
    }
    
    /* 2. Configure MAC Frame Filter */
    reg_val = 0;  /* Allow all valid frames, filter based on address */
    geth_write_reg(base, GETH_MAC_FRAME_FILTER_OFFSET, reg_val);
    
    /* 3. Set MAC Address */
    geth_set_mac_address(instance, config->mac_addr);
    
    /* 4. Configure MAC Configuration */
    reg_val = 0;
    
    /* Receiver and Transmitter Enable */
    reg_val |= MAC_CFG_RE | MAC_CFG_TE;
    
    /* Duplex Mode */
    if (config->duplex == GETH_DUPLEX_FULL) {
        reg_val |= MAC_CFG_DM;
    }
    
    /* Speed configuration (for MII/RMII) */
    if (config->speed == GETH_SPEED_100M) {
        reg_val |= (1UL << 14);  /* FES bit for 100Mbps */
    }
    
    /* Inter-frame gap - 96 bit times */
    reg_val |= (0x7 << 17);
    
    /* Carrier sense disable in half duplex */
    if (config->duplex == GETH_DUPLEX_HALF) {
        reg_val |= (1UL << 16);
    }
    
    geth_write_reg(base, GETH_MAC_CONFIGURATION_OFFSET, reg_val);
    
    /* 5. Configure DMA Bus Mode */
    reg_val = (32 << DMA_BM_PBL_SHIFT) |
              (0 << DMA_BM_DSL_SHIFT) |
              DMA_BM_AAL |
              DMA_BM_FB;           /* Fixed burst for AURIX SPB */
    geth_write_reg(base, GETH_DMA_BUS_MODE_OFFSET, reg_val);
    
    /* 6. Set descriptor list addresses */
    if (handle->tx_desc) {
        geth_write_reg(base, GETH_DMA_TX_DESC_LIST_ADDR_OFFSET, (uint32_t)handle->tx_desc);
    }
    if (handle->rx_desc) {
        geth_write_reg(base, GETH_DMA_RX_DESC_LIST_ADDR_OFFSET, (uint32_t)handle->rx_desc);
    }
    
    /* 7. Configure DMA Operation Mode */
    reg_val = DMA_OM_SR |           /* Start Receive */
              DMA_OM_ST |           /* Start Transmit */
              DMA_OM_OSF |          /* Operate on Second Frame */
              (0 << DMA_OM_TTC_SHIFT) |
              (0 << DMA_OM_RTC_SHIFT);
    geth_write_reg(base, GETH_DMA_OP_MODE_OFFSET, reg_val);
    
    /* 8. Enable interrupts */
    reg_val = DMA_INTR_NIE |
              DMA_INTR_RIE |
              DMA_INTR_TIE;
    geth_write_reg(base, GETH_DMA_INTR_ENA_OFFSET, reg_val);
    
    return 0;
}

int geth_deinit(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    if (!base) {
        return -1;
    }
    
    /* Disable MAC */
    geth_write_reg(base, GETH_MAC_CONFIGURATION_OFFSET, 0);
    
    /* Stop DMA */
    geth_write_reg(base, GETH_DMA_OP_MODE_OFFSET, 0);
    
    /* Reset DMA */
    geth_write_reg(base, GETH_DMA_BUS_MODE_OFFSET, DMA_BM_SWR);
    
    memset(&geth_handles[instance], 0, sizeof(geth_handle_t));
    
    return 0;
}

int geth_set_mac_address(uint8_t instance, const uint8_t mac[6])
{
    uint32_t base;
    uint32_t high, low;
    
    if (!mac || instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    high = ((uint32_t)mac[5] << 8) | (uint32_t)mac[4] | MAC_ADDR_HIGH_AE;
    low = ((uint32_t)mac[3] << 24) | ((uint32_t)mac[2] << 16) |
          ((uint32_t)mac[1] << 8)  | (uint32_t)mac[0];
    
    geth_write_reg(base, GETH_MAC_ADDR0_HIGH_OFFSET, high);
    geth_write_reg(base, GETH_MAC_ADDR0_LOW_OFFSET, low);
    
    return 0;
}

int geth_get_mac_address(uint8_t instance, uint8_t mac[6])
{
    uint32_t base;
    uint32_t high, low;
    
    if (!mac || instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    high = geth_read_reg(base, GETH_MAC_ADDR0_HIGH_OFFSET);
    low = geth_read_reg(base, GETH_MAC_ADDR0_LOW_OFFSET);
    
    mac[0] = (uint8_t)(low & 0xFF);
    mac[1] = (uint8_t)((low >> 8) & 0xFF);
    mac[2] = (uint8_t)((low >> 16) & 0xFF);
    mac[3] = (uint8_t)((low >> 24) & 0xFF);
    mac[4] = (uint8_t)(high & 0xFF);
    mac[5] = (uint8_t)((high >> 8) & 0xFF);
    
    return 0;
}

int geth_enable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    /* Enable MAC receiver and transmitter */
    reg_val = geth_read_reg(base, GETH_MAC_CONFIGURATION_OFFSET);
    reg_val |= (MAC_CFG_RE | MAC_CFG_TE);
    geth_write_reg(base, GETH_MAC_CONFIGURATION_OFFSET, reg_val);
    
    /* Start DMA */
    reg_val = geth_read_reg(base, GETH_DMA_OP_MODE_OFFSET);
    reg_val |= (DMA_OM_SR | DMA_OM_ST);
    geth_write_reg(base, GETH_DMA_OP_MODE_OFFSET, reg_val);
    
    return 0;
}

int geth_disable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    /* Stop DMA */
    reg_val = geth_read_reg(base, GETH_DMA_OP_MODE_OFFSET);
    reg_val &= ~(DMA_OM_SR | DMA_OM_ST);
    geth_write_reg(base, GETH_DMA_OP_MODE_OFFSET, reg_val);
    
    /* Disable MAC */
    reg_val = geth_read_reg(base, GETH_MAC_CONFIGURATION_OFFSET);
    reg_val &= ~(MAC_CFG_RE | MAC_CFG_TE);
    geth_write_reg(base, GETH_MAC_CONFIGURATION_OFFSET, reg_val);
    
    return 0;
}

int geth_mdio_read(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (!data || instance > 1 || phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    if (geth_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    reg_val = MII_ADDR_GB |
              (MII_CR_DIV_10 << MII_ADDR_CR_SHIFT) |  /* 10MHz for TJA1101 */
              ((uint32_t)phy_addr << MII_ADDR_GR_SHIFT) |
              ((uint32_t)reg_addr << MII_ADDR_PA_SHIFT);
    
    geth_write_reg(base, GETH_MAC_MII_ADDR_OFFSET, reg_val);
    
    if (geth_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    *data = (uint16_t)geth_read_reg(base, GETH_MAC_MII_DATA_OFFSET);
    
    return 0;
}

int geth_mdio_write(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 1 || phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    base = geth_get_base_addr(instance);
    
    if (geth_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    geth_write_reg(base, GETH_MAC_MII_DATA_OFFSET, (uint32_t)data);
    
    reg_val = MII_ADDR_GB | MII_ADDR_MW |
              (MII_CR_DIV_10 << MII_ADDR_CR_SHIFT) |
              ((uint32_t)phy_addr << MII_ADDR_GR_SHIFT) |
              ((uint32_t)reg_addr << MII_ADDR_PA_SHIFT);
    
    geth_write_reg(base, GETH_MAC_MII_ADDR_OFFSET, reg_val);
    
    if (geth_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    return 0;
}

void geth_irq_handler(uint8_t instance)
{
    uint32_t base;
    uint32_t status;
    geth_handle_t *handle;
    
    if (instance > 1) {
        return;
    }
    
    base = geth_get_base_addr(instance);
    handle = &geth_handles[instance];
    
    status = geth_read_reg(base, GETH_DMA_STATUS_OFFSET);
    
    if (status & DMA_STATUS_NIS) {
        if (status & DMA_STATUS_RI) {
            if (handle->rx_callback) {
                /* Process received frames */
            }
        }
        
        if (status & DMA_STATUS_TI) {
            if (handle->tx_complete_callback) {
                handle->tx_complete_callback();
            }
        }
    }
    
    /* Clear all interrupts */
    geth_write_reg(base, GETH_DMA_STATUS_OFFSET, status);
}

int geth_send_frame(uint8_t instance, const uint8_t *data, uint32_t len, bool timestamp)
{
    geth_handle_t *handle;
    geth_tx_desc_t *desc;
    
    if (!data || instance > 1 || len == 0) {
        return -1;
    }
    
    handle = &geth_handles[instance];
    if (!handle->tx_desc) {
        return -1;
    }
    
    desc = &handle->tx_desc[handle->tx_prod_idx];
    
    if (desc->tdes0 & TDES0_OWN) {
        return -1;
    }
    
    if (handle->tx_buffers) {
        memcpy(&handle->tx_buffers[handle->tx_prod_idx * handle->tx_desc_num],
               data, len);
        desc->tdes2 = (uint32_t)&handle->tx_buffers[handle->tx_prod_idx * handle->tx_desc_num];
    }
    
    desc->tdes1 = (len & 0x1FFF);
    desc->tdes0 = TDES0_FS | TDES0_LS;
    
    if (timestamp) {
        desc->tdes0 |= TDES0_TTSE;
    }
    
    desc->tdes0 |= TDES0_OWN;
    
    handle->tx_prod_idx = (handle->tx_prod_idx + 1) % handle->tx_desc_num;
    
    /* Poll DMA */
    geth_write_reg(handle->base_addr, GETH_DMA_OP_MODE_OFFSET,
                   geth_read_reg(handle->base_addr, GETH_DMA_OP_MODE_OFFSET));
    
    return 0;
}

int geth_receive_frame(uint8_t instance, uint8_t *buffer, uint32_t max_len, uint64_t *timestamp)
{
    geth_handle_t *handle;
    geth_rx_desc_t *desc;
    uint32_t len;
    
    if (!buffer || instance > 1) {
        return -1;
    }
    
    handle = &geth_handles[instance];
    if (!handle->rx_desc) {
        return -1;
    }
    
    desc = &handle->rx_desc[handle->rx_cons_idx];
    
    if (desc->rdes0 & RDES0_OWN) {
        return -1;  /* No frame available */
    }
    
    len = (desc->rdes0 >> RDES0_FL_SHIFT) & 0x3FFF;
    if (len > max_len) {
        len = max_len;
    }
    
    if (handle->rx_buffers) {
        memcpy(buffer, &handle->rx_buffers[handle->rx_cons_idx * handle->rx_desc_num], len);
    }
    
    if (timestamp && (desc->rdes0 & RDES0_TS)) {
        *timestamp = ((uint64_t)desc->rdes7 << 32) | desc->rdes6;
    }
    
    /* Return descriptor to DMA */
    desc->rdes0 = RDES0_OWN;
    handle->rx_cons_idx = (handle->rx_cons_idx + 1) % handle->rx_desc_num;
    
    return (int)len;
}

void geth_get_stats(uint8_t instance, uint32_t *rx_frames, uint32_t *tx_frames,
                    uint32_t *rx_errors, uint32_t *tx_errors)
{
    uint32_t base;
    
    if (instance > 1) {
        return;
    }
    
    base = geth_get_base_addr(instance);
    
    if (rx_frames) {
        *rx_frames = geth_read_reg(base, 0x108);
    }
    if (tx_frames) {
        *tx_frames = geth_read_reg(base, 0x118);
    }
    if (rx_errors) {
        *rx_errors = geth_read_reg(base, 0x114);
    }
    if (tx_errors) {
        *tx_errors = geth_read_reg(base, 0x11C);
    }
}

void geth_dump_regs(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 1) {
        return;
    }
    
    base = geth_get_base_addr(instance);
    
    (void)geth_read_reg(base, GETH_MAC_CONFIGURATION_OFFSET);
    (void)geth_read_reg(base, GETH_DMA_BUS_MODE_OFFSET);
    (void)geth_read_reg(base, GETH_DMA_OP_MODE_OFFSET);
    (void)geth_read_reg(base, GETH_DMA_STATUS_OFFSET);
}
