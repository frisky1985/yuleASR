/*
 * enet_driver.h
 * NXP S32G3 ENET/QoS Ethernet Controller Driver
 * Register-level access for Gigabit Ethernet MAC
 */

#ifndef ENET_DRIVER_H
#define ENET_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * S32G3 ENET Memory Map
 * Base addresses for ENET instances
 *============================================================================*/
#define ENET0_BASE_ADDR         0x40120000UL
#define ENET1_BASE_ADDR         0x40122000UL
#define ENET2_BASE_ADDR         0x40124000UL

/*==============================================================================
 * ENET Register Definitions (offset from base)
 *============================================================================*/

/* MAC Configuration Register */
#define ENET_MAC_CFG_OFFSET     0x0000
#define ENET_MAC_CFG            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_CFG_OFFSET)))

#define MAC_CFG_RE              (1UL << 0)      /* Receiver Enable */
#define MAC_CFG_TE              (1UL << 1)      /* Transmitter Enable */
#define MAC_CFG_DM              (1UL << 13)     /* Duplex Mode (1=Full) */
#define MAC_CFG_LM              (1UL << 12)     /* Loopback Mode */
#define MAC_CFG_DO              (1UL << 10)     /* Disable Receive Own */
#define MAC_CFG_DR              (1UL << 9)      /* Disable Retry */
#define MAC_CFG_BL              (0x3 << 5)      /* Back-off Limit */
#define MAC_CFG_DC              (1UL << 4)      /* Deferral Check */

/* MAC Frame Filter Register */
#define ENET_MAC_FFR_OFFSET     0x0004
#define ENET_MAC_FFR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_FFR_OFFSET)))

#define MAC_FFR_PR              (1UL << 0)      /* Promiscuous Mode */
#define MAC_FFR_DAIF            (1UL << 3)      /* DA Inverse Filtering */
#define MAC_FFR_PM              (1UL << 4)      /* Pass All Multicast */
#define MAC_FFR_DBF             (1UL << 5)      /* Disable Broadcast Frames */

/* MAC MII Address Register (for MDIO operations) */
#define ENET_MAC_MAR_OFFSET     0x0010
#define ENET_MAC_MAR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_MAR_OFFSET)))

#define MAC_MAR_GB              (1UL << 0)      /* MII Busy */
#define MAC_MAR_MW              (1UL << 1)      /* MII Write */
#define MAC_MAR_CR_SHIFT        2               /* Clock Range shift */
#define MAC_MAR_GR_SHIFT        6               /* PHY Address shift */
#define MAC_MAR_PA_SHIFT        11              /* Register Address shift */

/* MAC MII Data Register */
#define ENET_MAC_MDR_OFFSET     0x0014
#define ENET_MAC_MDR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_MDR_OFFSET)))

/* MAC Flow Control Register */
#define ENET_MAC_FCR_OFFSET     0x0018
#define ENET_MAC_FCR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_FCR_OFFSET)))

#define MAC_FCR_FCB_BPA         (1UL << 0)      /* Flow Control Busy */
#define MAC_FCR_TFE             (1UL << 1)      /* Transmit Flow Control Enable */
#define MAC_FCR_RFE             (1UL << 2)      /* Receive Flow Control Enable */
#define MAC_FCR_UP              (1UL << 3)      /* Unicast Pause Frame Detect */
#define MAC_FCR_PLT_SHIFT       4               /* Pause Low Threshold */
#define MAC_FCR_DZPQ            (1UL << 7)      /* Disable Zero-Quanta Pause */

/* MAC VLAN Tag Register */
#define ENET_MAC_VTR_OFFSET     0x001C
#define ENET_MAC_VTR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_VTR_OFFSET)))

/* MAC PMT Control and Status Register */
#define ENET_MAC_PMT_OFFSET     0x002C
#define ENET_MAC_PMT            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_PMT_OFFSET)))

/* MAC Debug Register */
#define ENET_MAC_DBG_OFFSET     0x0034
#define ENET_MAC_DBG            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_DBG_OFFSET)))

/* MAC Interrupt Status/Mask Registers */
#define ENET_MAC_ISTS_OFFSET    0x0038
#define ENET_MAC_ISTS           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_ISTS_OFFSET)))

#define ENET_MAC_IMSK_OFFSET    0x003C
#define ENET_MAC_IMSK           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_IMSK_OFFSET)))

#define MAC_INT_PHYSTS          (1UL << 3)      /* PHY Status Change */
#define MAC_INT_PMT             (1UL << 4)      /* PMT Interrupt */
#define MAC_INT_MMC             (1UL << 27)     /* MMC Interrupt */

/* MAC PTP Time Stamp Control Register */
#define ENET_MAC_TSCR_OFFSET    0x0700
#define ENET_MAC_TSCR           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_TSCR_OFFSET)))

#define MAC_TSCR_TSENA          (1UL << 0)      /* Time Stamp Enable */
#define MAC_TSCR_TSCFUPDT       (1UL << 1)      /* Time Stamp Fine Update */
#define MAC_TSCR_TSINIT         (1UL << 2)      /* Time Stamp Initialize */
#define MAC_TSCR_TSUPDT         (1UL << 3)      /* Time Stamp Update */
#define MAC_TSCR_TSTRIG         (1UL << 4)      /* Time Stamp Interrupt Trigger Enable */
#define MAC_TSCR_TSADDREG       (1UL << 5)      /* Addend Register Update */
#define MAC_TSCR_TSENALL        (1UL << 8)      /* Enable Time Stamp for All Frames */
#define MAC_TSCR_TSCTRLSSR      (1UL << 9)      /* Digital or Binary rollover */

/* MAC Sub-Second Increment Register */
#define ENET_MAC_SSIR_OFFSET    0x0704
#define ENET_MAC_SSIR           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_SSIR_OFFSET)))

/* MAC System Time Seconds Register */
#define ENET_MAC_STSR_OFFSET    0x0708
#define ENET_MAC_STSR           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_STSR_OFFSET)))

/* MAC System Time Nanoseconds Register */
#define ENET_MAC_STNR_OFFSET    0x070C
#define ENET_MAC_STNR           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_MAC_STNR_OFFSET)))

/* DMA Bus Mode Register */
#define ENET_DMA_BMR_OFFSET     0x1000
#define ENET_DMA_BMR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_BMR_OFFSET)))

#define DMA_BMR_SWR             (1UL << 0)      /* Software Reset */
#define DMA_BMR_DA              (1UL << 1)      /* DMA Arbitration */
#define DMA_BMR_DSL_SHIFT       2               /* Descriptor Skip Length */
#define DMA_BMR_PBL_SHIFT       8               /* Programmable Burst Length */
#define DMA_BMR_AAL             (1UL << 25)     /* Address-Aligned Beats */

/* DMA Status Register */
#define ENET_DMA_STS_OFFSET     0x1014
#define ENET_DMA_STS            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_STS_OFFSET)))

#define DMA_STS_TS_SHIFT        20              /* Transmit Process State */
#define DMA_STS_RS_SHIFT        17              /* Receive Process State */
#define DMA_STS_NIS             (1UL << 16)     /* Normal Interrupt Summary */
#define DMA_STS_AIS             (1UL << 15)     /* Abnormal Interrupt Summary */
#define DMA_STS_ERI             (1UL << 14)     /* Early Receive Interrupt */
#define DMA_STS_FBI             (1UL << 13)     /* Fatal Bus Error Interrupt */
#define DMA_STS_ETI             (1UL << 10)     /* Early Transmit Interrupt */
#define DMA_STS_RWT             (1UL << 9)      /* Receive Watchdog Timeout */
#define DMA_STS_RPS             (1UL << 8)      /* Receive Process Stopped */
#define DMA_STS_RU              (1UL << 7)      /* Receive Buffer Unavailable */
#define DMA_STS_RI              (1UL << 6)      /* Receive Interrupt */
#define DMA_STS_UNF             (1UL << 5)      /* Transmit Underflow */
#define DMA_STS_OVF             (1UL << 4)      /* Receive Overflow */
#define DMA_STS_TJT             (1UL << 3)      /* Transmit Jabber Timeout */
#define DMA_STS_TU              (1UL << 2)      /* Transmit Buffer Unavailable */
#define DMA_STS_TPS             (1UL << 1)      /* Transmit Process Stopped */
#define DMA_STS_TI              (1UL << 0)      /* Transmit Interrupt */

/* DMA Interrupt Enable Register */
#define ENET_DMA_IER_OFFSET     0x101C
#define ENET_DMA_IER            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_IER_OFFSET)))

/* DMA Receive/Transmit Descriptor List Address */
#define ENET_DMA_RDLA_OFFSET    0x100C
#define ENET_DMA_RDLA           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_RDLA_OFFSET)))

#define ENET_DMA_TDLA_OFFSET    0x1010
#define ENET_DMA_TDLA           (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_TDLA_OFFSET)))

/* DMA Operation Mode Register */
#define ENET_DMA_OMR_OFFSET     0x1018
#define ENET_DMA_OMR            (*((volatile uint32_t *)(ENET0_BASE_ADDR + ENET_DMA_OMR_OFFSET)))

#define DMA_OMR_SR              (1UL << 1)      /* Start/Stop Receive */
#define DMA_OMR_OSF             (1UL << 2)      /* Operate on Second Frame */
#define DMA_OMR_RTC_SHIFT       3               /* Receive Threshold Control */
#define DMA_OMR_FUF             (1UL << 6)      /* Forward Undersized Frames */
#define DMA_OMR_FEF             (1UL << 7)      /* Forward Error Frames */
#define DMA_OMR_ST              (1UL << 13)     /* Start/Stop Transmission */
#define DMA_OMR_TTC_SHIFT       14              /* Transmit Threshold Control */
#define DMA_OMR_FTF             (1UL << 20)     /* Flush Transmit FIFO */

/*==============================================================================
 * DMA Descriptor Definitions (Enhanced Descriptors for IEEE 1588)
 *============================================================================*/

/* Transmit Descriptor */
typedef struct {
    volatile uint32_t tdes0;    /* Status */
    volatile uint32_t tdes1;    /* Control */
    volatile uint32_t tdes2;    /* Buffer1 Address */
    volatile uint32_t tdes3;    /* Buffer2 Address */
    volatile uint32_t tdes4;    /* Extended Status (IEEE 1588) */
    volatile uint32_t tdes5;    /* Reserved */
    volatile uint32_t tdes6;    /* Transmit Time Stamp Low */
    volatile uint32_t tdes7;    /* Transmit Time Stamp High */
} enet_tx_desc_t;

/* Transmit Descriptor Bits */
#define TDES0_OWN               (1UL << 31)     /* Own by DMA */
#define TDES0_IC                (1UL << 30)     /* Interrupt on Completion */
#define TDES0_LS                (1UL << 29)     /* Last Segment */
#define TDES0_FS                (1UL << 28)     /* First Segment */
#define TDES0_DC                (1UL << 27)     /* Disable CRC */
#define TDES0_DP                (1UL << 26)     /* Disable Pad */
#define TDES0_TTSE              (1UL << 25)     /* Transmit Time Stamp Enable */
#define TDES0_CIC_SHIFT         22              /* Checksum Insertion Control */
#define TDES0_TER               (1UL << 21)     /* Transmit End of Ring */
#define TDES0_TCH               (1UL << 20)     /* Second Address Chained */
#define TDES0_TTSS              (1UL << 17)     /* TX Time Stamp Status */
#define TDES0_ES                (1UL << 15)     /* Error Summary */
#define TDES0_JT                (1UL << 14)     /* Jabber Timeout */
#define TDES0_FF                (1UL << 13)     /* Frame Flushed */
#define TDES0_IPE               (1UL << 12)     /* IP Payload Error */
#define TDES0_LCA               (1UL << 11)     /* Loss of Carrier */
#define TDES0_NC                (1UL << 10)     /* No Carrier */
#define TDES0_LCO               (1UL << 9)      /* Late Collision */
#define TDES0_EC                (1UL << 8)      /* Excessive Collisions */
#define TDES0_VF                (1UL << 7)      /* VLAN Frame */
#define TDES0_CC_SHIFT          3               /* Collision Count */
#define TDES0_ED                (1UL << 2)      /* Excessive Deferral */
#define TDES0_UF                (1UL << 1)      /* Underflow Error */
#define TDES0_DB                (1UL << 0)      /* Deferred Bit */

/* Receive Descriptor */
typedef struct {
    volatile uint32_t rdes0;    /* Status */
    volatile uint32_t rdes1;    /* Control */
    volatile uint32_t rdes2;    /* Buffer1 Address */
    volatile uint32_t rdes3;    /* Buffer2 Address */
    volatile uint32_t rdes4;    /* Extended Status (IEEE 1588) */
    volatile uint32_t rdes5;    /* Reserved */
    volatile uint32_t rdes6;    /* Receive Time Stamp Low */
    volatile uint32_t rdes7;    /* Receive Time Stamp High */
} enet_rx_desc_t;

/* Receive Descriptor Bits */
#define RDES0_OWN               (1UL << 31)     /* Own by DMA */
#define RDES0_AFM               (1UL << 30)     /* Destination Address Filter Fail */
#define RDES0_FL_SHIFT          16              /* Frame Length */
#define RDES0_ES                (1UL << 15)     /* Error Summary */
#define RDES0_DE                (1UL << 14)     /* Descriptor Error */
#define RDES0_SAF               (1UL << 13)     /* Source Address Filter Fail */
#define RDES0_LE                (1UL << 12)     /* Length Error */
#define RDES0_OE                (1UL << 11)     /* Overflow Error */
#define RDES0_VLAN              (1UL << 10)     /* VLAN Tag */
#define RDES0_FS                (1UL << 9)      /* First Descriptor */
#define RDES0_LS                (1UL << 8)      /* Last Descriptor */
#define RDES0_TS                (1UL << 7)      /* Time Stamp Available */
#define RDES0_LCO               (1UL << 6)      /* Late Collision */
#define RDES0_FT                (1UL << 5)      /* Frame Type */
#define RDES0_RWT               (1UL << 4)      /* Receive Watchdog Timeout */
#define RDES0_RE                (1UL << 3)      /* Receive Error */
#define RDES0_DBE               (1UL << 2)      /* Dribble Bit Error */
#define RDES0_CE                (1UL << 1)      /* CRC Error */
#define RDES0_PCE               (1UL << 0)      /* Payload Checksum Error */

#define RDES1_RER               (1UL << 15)     /* Receive End of Ring */
#define RDES1_RCH               (1UL << 14)     /* Second Address Chained */
#define RDES1_RBS2_SHIFT        16              /* Buffer 2 Size */
#define RDES1_RBS1_SHIFT        0               /* Buffer 1 Size */

/*==============================================================================
 * Configuration Structures
 *============================================================================*/

typedef enum {
    ENET_SPEED_10M = 0,
    ENET_SPEED_100M,
    ENET_SPEED_1000M
} enet_speed_t;

typedef enum {
    ENET_DUPLEX_HALF = 0,
    ENET_DUPLEX_FULL
} enet_duplex_t;

typedef enum {
    ENET_MII = 0,
    ENET_RMII,
    ENET_RGMII
} enet_interface_t;

typedef struct {
    enet_speed_t speed;
    enet_duplex_t duplex;
    enet_interface_t interface;
    bool enable_1588;
    bool enable_vlan;
    bool enable_jumbo;
    uint8_t mac_addr[6];
    uint16_t rx_desc_count;
    uint16_t tx_desc_count;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
} enet_config_t;

typedef struct {
    uint32_t base_addr;
    enet_tx_desc_t *tx_desc;
    enet_rx_desc_t *rx_desc;
    uint8_t *tx_buffers;
    uint8_t *rx_buffers;
    uint16_t tx_prod_idx;
    uint16_t tx_cons_idx;
    uint16_t rx_prod_idx;
    uint16_t rx_cons_idx;
    uint16_t tx_desc_num;
    uint16_t rx_desc_num;
    void (*rx_callback)(uint8_t *data, uint32_t len, uint64_t timestamp);
    void (*tx_complete_callback)(void);
} enet_handle_t;

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Initialization */
int enet_init(uint8_t instance, const enet_config_t *config);
int enet_deinit(uint8_t instance);
void enet_get_default_config(enet_config_t *config);

/* MAC Control */
int enet_set_mac_address(uint8_t instance, const uint8_t mac[6]);
int enet_get_mac_address(uint8_t instance, uint8_t mac[6]);
int enet_set_speed(uint8_t instance, enet_speed_t speed, enet_duplex_t duplex);
int enet_enable(uint8_t instance);
int enet_disable(uint8_t instance);

/* Transmit/Receive */
int enet_send_frame(uint8_t instance, const uint8_t *data, uint32_t len, bool timestamp);
int enet_receive_frame(uint8_t instance, uint8_t *buffer, uint32_t max_len, uint64_t *timestamp);
int enet_get_tx_buffer(uint8_t instance, uint8_t **buffer, uint32_t *size);
int enet_send_buffer(uint8_t instance, uint32_t len, bool timestamp);

/* Interrupt Handling */
void enet_irq_handler(uint8_t instance);
int enet_enable_irq(uint8_t instance, uint32_t mask);
int enet_disable_irq(uint8_t instance, uint32_t mask);
int enet_clear_irq(uint8_t instance, uint32_t mask);
uint32_t enet_get_irq_status(uint8_t instance);

/* MDIO Interface */
int enet_mdio_read(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data);
int enet_mdio_write(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t data);

/* Statistics */
void enet_get_stats(uint8_t instance, uint32_t *rx_frames, uint32_t *tx_frames, 
                    uint32_t *rx_errors, uint32_t *tx_errors);

/* Debug */
void enet_dump_regs(uint8_t instance);

#endif /* ENET_DRIVER_H */
