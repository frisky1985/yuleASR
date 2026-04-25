/*
 * enet_driver.c
 * NXP S32G3 ENET/QoS Ethernet Controller Driver Implementation
 */

#include "enet_driver.h"
#include <string.h>

/*==============================================================================
 * Static Variables
 *============================================================================*/
static enet_handle_t enet_handles[3];   /* Support up to 3 ENET instances */

/* Clock ranges for MDIO */
#define MAC_MII_ADDR_CR_60_100     (0)     /* 60-100 MHz */
#define MAC_MII_ADDR_CR_100_150    (1)     /* 100-150 MHz */
#define MAC_MII_ADDR_CR_20_35      (2)     /* 20-35 MHz */
#define MAC_MII_ADDR_CR_35_60      (3)     /* 35-60 MHz */
#define MAC_MII_ADDR_CR_150_250    (4)     /* 150-250 MHz */
#define MAC_MII_ADDR_CR_250_300    (5)     /* 250-300 MHz */
#define MAC_MII_ADDR_CR_300_500    (6)     /* 300-500 MHz */
#define MAC_MII_ADDR_CR_500_800    (7)     /* 500-800 MHz */

/*==============================================================================
 * Static Helper Functions
 *============================================================================*/

/* Get base address for ENET instance */
static uint32_t enet_get_base_addr(uint8_t instance)
{
    switch(instance) {
        case 0: return ENET0_BASE_ADDR;
        case 1: return ENET1_BASE_ADDR;
        case 2: return ENET2_BASE_ADDR;
        default: return 0;
    }
}

/* Read 32-bit register */
static inline uint32_t enet_read_reg(uint32_t base, uint32_t offset)
{
    return *((volatile uint32_t *)(base + offset));
}

/* Write 32-bit register */
static inline void enet_write_reg(uint32_t base, uint32_t offset, uint32_t value)
{
    *((volatile uint32_t *)(base + offset)) = value;
}

/* Wait for MDIO operation to complete */
static int enet_wait_mdio_ready(uint32_t base)
{
    volatile uint32_t timeout = 10000;
    
    while (timeout--) {
        if (!(enet_read_reg(base, ENET_MAC_MAR_OFFSET) & MAC_MAR_GB)) {
            return 0;
        }
    }
    return -1;  /* Timeout */
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void enet_get_default_config(enet_config_t *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(enet_config_t));
    
    config->speed = ENET_SPEED_1000M;
    config->duplex = ENET_DUPLEX_FULL;
    config->interface = ENET_RGMII;
    config->enable_1588 = true;
    config->enable_vlan = false;
    config->enable_jumbo = false;
    
    /* Default MAC address - should be overridden */
    config->mac_addr[0] = 0x00;
    config->mac_addr[1] = 0x04;
    config->mac_addr[2] = 0x9F;
    config->mac_addr[3] = 0x00;
    config->mac_addr[4] = 0x00;
    config->mac_addr[5] = 0x01;
    
    config->rx_desc_count = 16;
    config->tx_desc_count = 16;
    config->rx_buffer_size = 1536;  /* Standard MTU + alignment */
    config->tx_buffer_size = 1536;
}

int enet_init(uint8_t instance, const enet_config_t *config)
{
    uint32_t base;
    uint32_t reg_val;
    enet_handle_t *handle;
    
    if (!config || instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    if (!base) {
        return -1;
    }
    
    handle = &enet_handles[instance];
    memset(handle, 0, sizeof(enet_handle_t));
    handle->base_addr = base;
    handle->tx_desc_num = config->tx_desc_count;
    handle->rx_desc_num = config->rx_desc_count;
    
    /* 1. Software reset DMA */
    enet_write_reg(base, ENET_DMA_BMR_OFFSET, DMA_BMR_SWR);
    
    /* Wait for reset to complete */
    volatile uint32_t timeout = 10000;
    while (enet_read_reg(base, ENET_DMA_BMR_OFFSET) & DMA_BMR_SWR) {
        if (--timeout == 0) {
            return -1;  /* Reset failed */
        }
    }
    
    /* 2. Configure MAC Frame Filter - accept broadcast and unicast */
    reg_val = MAC_FFR_DBF;  /* Allow broadcast frames */
    enet_write_reg(base, ENET_MAC_FFR_OFFSET, reg_val);
    
    /* 3. Set MAC Address */
    enet_set_mac_address(instance, config->mac_addr);
    
    /* 4. Configure MAC based on speed and duplex */
    reg_val = 0;
    
    /* Receiver Enable */
    reg_val |= MAC_CFG_RE;
    
    /* Transmitter Enable */
    reg_val |= MAC_CFG_TE;
    
    /* Duplex Mode */
    if (config->duplex == ENET_DUPLEX_FULL) {
        reg_val |= MAC_CFG_DM;
    }
    
    /* Interface mode - RGMII/RMII/MII specific config */
    if (config->interface == ENET_RGMII) {
        /* RGMII specific settings in MAC Configuration Register extension */
        reg_val |= (1UL << 24);  /* RGMII mode */
    }
    
    /* Port Select for 1000Mbps */
    if (config->speed == ENET_SPEED_1000M) {
        reg_val |= (1UL << 15);  /* PS = 0 for 1000Mbps (bit cleared) */
    } else if (config->speed == ENET_SPEED_100M) {
        reg_val |= (1UL << 14);  /* FES = 1 for 100Mbps */
    } else {
        /* 10Mbps - default */
    }
    
    /* Inter-frame gap */
    reg_val |= (0x7 << 17);  /* IFG = 96 bit times */
    
    /* Disable Jabber Timer for jumbo frames */
    if (config->enable_jumbo) {
        reg_val |= (1UL << 22);  /* JD = 1 */
    }
    
    enet_write_reg(base, ENET_MAC_CFG_OFFSET, reg_val);
    
    /* 5. Initialize DMA descriptors (simplified - assumes descriptors already allocated) */
    /* In a real implementation, descriptors would be allocated here */
    
    /* Set descriptor list addresses */
    if (handle->tx_desc) {
        enet_write_reg(base, ENET_DMA_TDLA_OFFSET, (uint32_t)handle->tx_desc);
    }
    if (handle->rx_desc) {
        enet_write_reg(base, ENET_DMA_RDLA_OFFSET, (uint32_t)handle->rx_desc);
    }
    
    /* 6. Configure DMA Bus Mode */
    reg_val = (32 << DMA_BMR_PBL_SHIFT) |   /* Programmable Burst Length */
              (0 << DMA_BMR_DSL_SHIFT)  |   /* Descriptor Skip Length */
              DMA_BMR_AAL;                   /* Address-Aligned Beats */
    enet_write_reg(base, ENET_DMA_BMR_OFFSET, reg_val);
    
    /* 7. Configure DMA Operation Mode */
    reg_val = DMA_OMR_SR |                   /* Start Receive */
              DMA_OMR_ST |                   /* Start Transmit */
              DMA_OMR_OSF |                  /* Operate on Second Frame */
              (0 << DMA_OMR_TTC_SHIFT) |     /* Transmit Threshold */
              (0 << DMA_OMR_RTC_SHIFT);      /* Receive Threshold */
    enet_write_reg(base, ENET_DMA_OMR_OFFSET, reg_val);
    
    /* 8. Initialize IEEE 1588 if enabled */
    if (config->enable_1588) {
        /* Enable time stamping */
        reg_val = MAC_TSCR_TSENA |          /* Time Stamp Enable */
                  MAC_TSCR_TSCFUPDT |       /* Fine Update */
                  MAC_TSCR_TSCTRLSSR;       /* Digital rollover */
        enet_write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
        
        /* Set sub-second increment (for 20ns accuracy at 50MHz) */
        enet_write_reg(base, ENET_MAC_SSIR_OFFSET, 20);
        
        /* Initialize timestamp to 0 */
        enet_write_reg(base, ENET_MAC_STSR_OFFSET, 0);
        enet_write_reg(base, ENET_MAC_STNR_OFFSET, 0);
        reg_val = enet_read_reg(base, ENET_MAC_TSCR_OFFSET);
        reg_val |= MAC_TSCR_TSINIT;
        enet_write_reg(base, ENET_MAC_TSCR_OFFSET, reg_val);
    }
    
    /* 9. Enable interrupts (RX and TX complete) */
    reg_val = DMA_OMR_SR | DMA_OMR_ST;
    enet_write_reg(base, ENET_DMA_IER_OFFSET, 
                   DMA_STS_NIS | DMA_STS_RI | DMA_STS_TI);
    
    return 0;
}

int enet_deinit(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    if (!base) {
        return -1;
    }
    
    /* Disable MAC */
    enet_write_reg(base, ENET_MAC_CFG_OFFSET, 0);
    
    /* Stop DMA */
    enet_write_reg(base, ENET_DMA_OMR_OFFSET, 0);
    
    /* Reset DMA */
    enet_write_reg(base, ENET_DMA_BMR_OFFSET, DMA_BMR_SWR);
    
    /* Clear handle */
    memset(&enet_handles[instance], 0, sizeof(enet_handle_t));
    
    return 0;
}

int enet_set_mac_address(uint8_t instance, const uint8_t mac[6])
{
    uint32_t base;
    uint32_t high, low;
    
    if (!mac || instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    /* MAC Address High Register (offset 0x40) */
    high = ((uint32_t)mac[5] << 8) | (uint32_t)mac[4] | (1UL << 31);  /* Address Enable */
    
    /* MAC Address Low Register (offset 0x44) */
    low = ((uint32_t)mac[3] << 24) | ((uint32_t)mac[2] << 16) |
          ((uint32_t)mac[1] << 8)  | (uint32_t)mac[0];
    
    enet_write_reg(base, 0x40, high);
    enet_write_reg(base, 0x44, low);
    
    return 0;
}

int enet_get_mac_address(uint8_t instance, uint8_t mac[6])
{
    uint32_t base;
    uint32_t high, low;
    
    if (!mac || instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    high = enet_read_reg(base, 0x40);
    low = enet_read_reg(base, 0x44);
    
    mac[0] = (uint8_t)(low & 0xFF);
    mac[1] = (uint8_t)((low >> 8) & 0xFF);
    mac[2] = (uint8_t)((low >> 16) & 0xFF);
    mac[3] = (uint8_t)((low >> 24) & 0xFF);
    mac[4] = (uint8_t)(high & 0xFF);
    mac[5] = (uint8_t)((high >> 8) & 0xFF);
    
    return 0;
}

int enet_enable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Enable receiver and transmitter */
    reg_val = enet_read_reg(base, ENET_MAC_CFG_OFFSET);
    reg_val |= (MAC_CFG_RE | MAC_CFG_TE);
    enet_write_reg(base, ENET_MAC_CFG_OFFSET, reg_val);
    
    /* Start DMA */
    reg_val = enet_read_reg(base, ENET_DMA_OMR_OFFSET);
    reg_val |= (DMA_OMR_SR | DMA_OMR_ST);
    enet_write_reg(base, ENET_DMA_OMR_OFFSET, reg_val);
    
    return 0;
}

int enet_disable(uint8_t instance)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 2) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Stop DMA */
    reg_val = enet_read_reg(base, ENET_DMA_OMR_OFFSET);
    reg_val &= ~(DMA_OMR_SR | DMA_OMR_ST);
    enet_write_reg(base, ENET_DMA_OMR_OFFSET, reg_val);
    
    /* Disable receiver and transmitter */
    reg_val = enet_read_reg(base, ENET_MAC_CFG_OFFSET);
    reg_val &= ~(MAC_CFG_RE | MAC_CFG_TE);
    enet_write_reg(base, ENET_MAC_CFG_OFFSET, reg_val);
    
    return 0;
}

int enet_mdio_read(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (!data || instance > 2 || phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Wait for MDIO ready */
    if (enet_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    /* Setup MII address register for read */
    reg_val = MAC_MAR_GB |                           /* MII Busy */
              (MAC_MII_ADDR_CR_35_60 << MAC_MAR_CR_SHIFT) |  /* Clock Range */
              ((uint32_t)phy_addr << MAC_MAR_GR_SHIFT) |
              ((uint32_t)reg_addr << MAC_MAR_PA_SHIFT);
    
    enet_write_reg(base, ENET_MAC_MAR_OFFSET, reg_val);
    
    /* Wait for operation complete */
    if (enet_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    /* Read data */
    *data = (uint16_t)enet_read_reg(base, ENET_MAC_MDR_OFFSET);
    
    return 0;
}

int enet_mdio_write(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    uint32_t base;
    uint32_t reg_val;
    
    if (instance > 2 || phy_addr > 31 || reg_addr > 31) {
        return -1;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Wait for MDIO ready */
    if (enet_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    /* Write data first */
    enet_write_reg(base, ENET_MAC_MDR_OFFSET, (uint32_t)data);
    
    /* Setup MII address register for write */
    reg_val = MAC_MAR_GB | MAC_MAR_MW |              /* MII Busy + Write */
              (MAC_MII_ADDR_CR_35_60 << MAC_MAR_CR_SHIFT) |
              ((uint32_t)phy_addr << MAC_MAR_GR_SHIFT) |
              ((uint32_t)reg_addr << MAC_MAR_PA_SHIFT);
    
    enet_write_reg(base, ENET_MAC_MAR_OFFSET, reg_val);
    
    /* Wait for operation complete */
    if (enet_wait_mdio_ready(base) != 0) {
        return -1;
    }
    
    return 0;
}

void enet_irq_handler(uint8_t instance)
{
    uint32_t base;
    uint32_t status;
    enet_handle_t *handle;
    
    if (instance > 2) {
        return;
    }
    
    base = enet_get_base_addr(instance);
    handle = &enet_handles[instance];
    
    /* Read DMA status */
    status = enet_read_reg(base, ENET_DMA_STS_OFFSET);
    
    /* Handle normal interrupts */
    if (status & DMA_STS_NIS) {
        /* Receive interrupt */
        if (status & DMA_STS_RI) {
            /* Process received frames */
            /* In a full implementation, this would walk the descriptor ring */
            if (handle->rx_callback) {
                /* Call registered callback with frame data */
                /* handle->rx_callback(data, len, timestamp); */
            }
        }
        
        /* Transmit interrupt */
        if (status & DMA_STS_TI) {
            if (handle->tx_complete_callback) {
                handle->tx_complete_callback();
            }
        }
    }
    
    /* Clear interrupts */
    enet_write_reg(base, ENET_DMA_STS_OFFSET, status);
}

int enet_send_frame(uint8_t instance, const uint8_t *data, uint32_t len, bool timestamp)
{
    enet_handle_t *handle;
    enet_tx_desc_t *desc;
    
    if (!data || instance > 2 || len == 0) {
        return -1;
    }
    
    handle = &enet_handles[instance];
    if (!handle->tx_desc) {
        return -1;
    }
    
    /* Get current descriptor */
    desc = &handle->tx_desc[handle->tx_prod_idx];
    
    /* Check if descriptor is owned by CPU */
    if (desc->tdes0 & TDES0_OWN) {
        return -1;  /* Buffer not available */
    }
    
    /* Copy data to transmit buffer */
    if (handle->tx_buffers) {
        memcpy(&handle->tx_buffers[handle->tx_prod_idx * handle->tx_desc_num], 
               data, len);
        desc->tdes2 = (uint32_t)&handle->tx_buffers[handle->tx_prod_idx * handle->tx_desc_num];
    }
    
    /* Setup descriptor */
    desc->tdes1 = (len & 0x1FFF);           /* Buffer 1 size */
    desc->tdes0 = TDES0_FS | TDES0_LS;      /* First and Last segment */
    
    if (timestamp) {
        desc->tdes0 |= TDES0_TTSE;          /* Enable timestamp */
    }
    
    desc->tdes0 |= TDES0_OWN;               /* Give to DMA */
    
    /* Advance producer index */
    handle->tx_prod_idx = (handle->tx_prod_idx + 1) % handle->tx_desc_num;
    
    /* Poll DMA to start transmission */
    enet_write_reg(handle->base_addr, ENET_DMA_OMR_OFFSET,
                   enet_read_reg(handle->base_addr, ENET_DMA_OMR_OFFSET));
    
    return 0;
}

void enet_get_stats(uint8_t instance, uint32_t *rx_frames, uint32_t *tx_frames,
                    uint32_t *rx_errors, uint32_t *tx_errors)
{
    uint32_t base;
    
    if (instance > 2) {
        return;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Read MMC counters (offset 0x100-0x1B0) */
    if (rx_frames) {
        *rx_frames = enet_read_reg(base, 0x108);  /* MMC Received Good Frames */
    }
    if (tx_frames) {
        *tx_frames = enet_read_reg(base, 0x118);  /* MMC Transmitted Good Frames */
    }
    if (rx_errors) {
        *rx_errors = enet_read_reg(base, 0x114);  /* MMC Receive Frame Errors */
    }
    if (tx_errors) {
        *tx_errors = enet_read_reg(base, 0x11C);  /* MMC Transmit Errors */
    }
}

void enet_dump_regs(uint8_t instance)
{
    uint32_t base;
    
    if (instance > 2) {
        return;
    }
    
    base = enet_get_base_addr(instance);
    
    /* Print key registers - would be replaced with proper logging */
    (void)enet_read_reg(base, ENET_MAC_CFG_OFFSET);
    (void)enet_read_reg(base, ENET_DMA_BMR_OFFSET);
    (void)enet_read_reg(base, ENET_DMA_OMR_OFFSET);
    (void)enet_read_reg(base, ENET_DMA_STS_OFFSET);
}
