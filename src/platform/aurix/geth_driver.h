/*
 * geth_driver.h
 * Infineon AURIX TC3xx GETH (Gigabit Ethernet) Driver
 * Register-level access for GETH module
 */

#ifndef GETH_DRIVER_H
#define GETH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * AURIX TC3xx GETH Memory Map
 *============================================================================*/

#define GETH0_BASE_ADDR         0xF001D000UL
#define GETH1_BASE_ADDR         0xF0021000UL

/*==============================================================================
 * GETH Register Definitions
 *============================================================================*/

/* MAC Configuration Register 0 */
#define GETH_MAC_CONFIGURATION_OFFSET   0x0000
#define GETH_MAC_CONFIGURATION          (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_CONFIGURATION_OFFSET)))

#define MAC_CFG_RE              (1UL << 0)      /* Receiver Enable */
#define MAC_CFG_TE              (1UL << 1)      /* Transmitter Enable */
#define MAC_CFG_DM              (1UL << 13)     /* Duplex Mode */
#define MAC_CFG_LM              (1UL << 12)     /* Loopback Mode */
#define MAC_CFG_DO              (1UL << 10)     /* Disable Receive Own */
#define MAC_CFG_DR              (1UL << 9)      /* Disable Retry */
#define MAC_CFG_BL_SHIFT        5               /* Back-off Limit shift */
#define MAC_CFG_DC              (1UL << 4)      /* Deferral Check */
#define MAC_CFG_PRELEN_SHIFT    0               /* Preamble Length shift */

/* MAC Frame Filter Register */
#define GETH_MAC_FRAME_FILTER_OFFSET    0x0004
#define GETH_MAC_FRAME_FILTER           (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_FRAME_FILTER_OFFSET)))

#define MAC_FILTER_PR           (1UL << 0)      /* Promiscuous Mode */
#define MAC_FILTER_HUC          (1UL << 1)      /* Hash Unicast */
#define MAC_FILTER_HMC          (1UL << 2)      /* Hash Multicast */
#define MAC_FILTER_DAIF         (1UL << 3)      /* DA Inverse Filtering */
#define MAC_FILTER_PM           (1UL << 4)      /* Pass All Multicast */
#define MAC_FILTER_DBF          (1UL << 5)      /* Disable Broadcast Frames */
#define MAC_FILTER_RA           (1UL << 31)     /* Receive All */

/* MAC MII Address Register */
#define GETH_MAC_MII_ADDR_OFFSET        0x0010
#define GETH_MAC_MII_ADDR               (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_MII_ADDR_OFFSET)))

#define MII_ADDR_GB             (1UL << 0)      /* MII Busy */
#define MII_ADDR_MW             (1UL << 1)      /* MII Write */
#define MII_ADDR_CR_SHIFT       2               /* Clock Range shift */
#define MII_ADDR_GR_SHIFT       6               /* PHY Address shift */
#define MII_ADDR_PA_SHIFT       11              /* Register Address shift */

/* MII Clock Range values for TC3xx (fAHB = 100MHz typical) */
#define MII_CR_DIV_4            0       /* fAHB/4 = 25MHz */
#define MII_CR_DIV_6            1       /* fAHB/6 = 16.7MHz */
#define MII_CR_DIV_8            2       /* fAHB/8 = 12.5MHz */
#define MII_CR_DIV_10           3       /* fAHB/10 = 10MHz */
#define MII_CR_DIV_14           4       /* fAHB/14 = 7.14MHz */
#define MII_CR_DIV_20           5       /* fAHB/20 = 5MHz */
#define MII_CR_DIV_28           6       /* fAHB/28 = 3.57MHz */
#define MII_CR_DIV_36           7       /* fAHB/36 = 2.78MHz */

/* MAC MII Data Register */
#define GETH_MAC_MII_DATA_OFFSET        0x0014
#define GETH_MAC_MII_DATA               (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_MII_DATA_OFFSET)))

/* MAC Flow Control Register */
#define GETH_MAC_FLOW_CTRL_OFFSET       0x0018
#define GETH_MAC_FLOW_CTRL              (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_FLOW_CTRL_OFFSET)))

#define MAC_FC_FCB_BPA          (1UL << 0)      /* Flow Control Busy */
#define MAC_FC_TFE              (1UL << 1)      /* Transmit Flow Control Enable */
#define MAC_FC_RFE              (1UL << 2)      /* Receive Flow Control Enable */
#define MAC_FC_UP               (1UL << 3)      /* Unicast Pause Frame Detect */
#define MAC_FC_PLT_SHIFT        4               /* Pause Low Threshold */
#define MAC_FC_DZPQ             (1UL << 7)      /* Disable Zero-Quanta Pause */
#define MAC_FC_PT_SHIFT         16              /* Pause Time */

/* MAC Address High/Low Registers */
#define GETH_MAC_ADDR0_HIGH_OFFSET      0x0040
#define GETH_MAC_ADDR0_LOW_OFFSET       0x0044
#define GETH_MAC_ADDR0_HIGH             (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_ADDR0_HIGH_OFFSET)))
#define GETH_MAC_ADDR0_LOW              (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_ADDR0_LOW_OFFSET)))

#define MAC_ADDR_HIGH_AE        (1UL << 31)     /* Address Enable */
#define MAC_ADDR_HIGH_SA        (1UL << 30)     /* Source Address */
#define MAC_ADDR_HIGH_MBC_SHIFT 24              /* Mask Byte Control */

/* Interrupt Status/Mask Registers */
#define GETH_MAC_INTR_STATUS_OFFSET     0x0038
#define GETH_MAC_INTR_MASK_OFFSET       0x003C
#define GETH_MAC_INTR_STATUS            (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_INTR_STATUS_OFFSET)))
#define GETH_MAC_INTR_MASK              (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_MAC_INTR_MASK_OFFSET)))

#define MAC_INTR_PMT            (1UL << 4)      /* PMT Interrupt */
#define MAC_INTR_MMC            (1UL << 27)     /* MMC Interrupt */

/*==============================================================================
 * DMA Register Definitions
 *============================================================================*/

/* DMA Bus Mode Register */
#define GETH_DMA_BUS_MODE_OFFSET        0x1000
#define GETH_DMA_BUS_MODE               (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_BUS_MODE_OFFSET)))

#define DMA_BM_SWR              (1UL << 0)      /* Software Reset */
#define DMA_BM_DA               (1UL << 1)      /* DMA Arbitration */
#define DMA_BM_DSL_SHIFT        2               /* Descriptor Skip Length */
#define DMA_BM_ATDS             (1UL << 7)      /* Alternate Descriptor Size */
#define DMA_BM_PBL_SHIFT        8               /* Programmable Burst Length */
#define DMA_BM_FB               (1UL << 16)     /* Fixed Burst */
#define DMA_BM_RPBL_SHIFT       17              /* Rx DMA PBL */
#define DMA_BM_AAL              (1UL << 25)     /* Address-Aligned Beats */

/* DMA Status Register */
#define GETH_DMA_STATUS_OFFSET          0x1014
#define GETH_DMA_STATUS                 (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_STATUS_OFFSET)))

#define DMA_STATUS_TI           (1UL << 0)      /* Transmit Interrupt */
#define DMA_STATUS_TPS          (1UL << 1)      /* Transmit Process Stopped */
#define DMA_STATUS_TU           (1UL << 2)      /* Transmit Buffer Unavailable */
#define DMA_STATUS_TJT          (1UL << 3)      /* Transmit Jabber Timeout */
#define DMA_STATUS_OVF          (1UL << 4)      /* Receive Overflow */
#define DMA_STATUS_UNF          (1UL << 5)      /* Transmit Underflow */
#define DMA_STATUS_RI           (1UL << 6)      /* Receive Interrupt */
#define DMA_STATUS_RU           (1UL << 7)      /* Receive Buffer Unavailable */
#define DMA_STATUS_RPS          (1UL << 8)      /* Receive Process Stopped */
#define DMA_STATUS_RWT          (1UL << 9)      /* Receive Watchdog Timeout */
#define DMA_STATUS_ETI          (1UL << 10)     /* Early Transmit Interrupt */
#define DMA_STATUS_FBI          (1UL << 13)     /* Fatal Bus Error */
#define DMA_STATUS_ERI          (1UL << 14)     /* Early Receive Interrupt */
#define DMA_STATUS_AIS          (1UL << 15)     /* Abnormal Int Summary */
#define DMA_STATUS_NIS          (1UL << 16)     /* Normal Int Summary */
#define DMA_STATUS_RS_SHIFT     17              /* Receive Process State */
#define DMA_STATUS_TS_SHIFT     20              /* Transmit Process State */
#define DMA_STATUS_EB_SHIFT     23              /* Error Bits */
#define DMA_STATUS_GLI          (1UL << 26)     /* GMAC Line Interface Interrupt */
#define DMA_STATUS_GMI          (1UL << 27)     /* GMAC MMC Interrupt */

/* DMA Operation Mode Register */
#define GETH_DMA_OP_MODE_OFFSET         0x1018
#define GETH_DMA_OP_MODE                (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_OP_MODE_OFFSET)))

#define DMA_OM_SR               (1UL << 1)      /* Start/Stop Receive */
#define DMA_OM_OSF              (1UL << 2)      /* Operate on Second Frame */
#define DMA_OM_RTC_SHIFT        3               /* Receive Threshold Control */
#define DMA_OM_FUF              (1UL << 6)      /* Forward Undersized Frames */
#define DMA_OM_FEF              (1UL << 7)      /* Forward Error Frames */
#define DMA_OM_ST               (1UL << 13)     /* Start/Stop Transmission */
#define DMA_OM_TTC_SHIFT        14              /* Transmit Threshold Control */
#define DMA_OM_FTF              (1UL << 20)     /* Flush Transmit FIFO */
#define DMA_OM_DFF              (1UL << 24)     /* Disable Flushing of Received Frames */

/* DMA Interrupt Enable Register */
#define GETH_DMA_INTR_ENA_OFFSET        0x101C
#define GETH_DMA_INTR_ENA               (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_INTR_ENA_OFFSET)))

#define DMA_INTR_TIE            (1UL << 0)      /* Transmit Interrupt Enable */
#define DMA_INTR_TSE            (1UL << 1)      /* Transmit Stopped Enable */
#define DMA_INTR_TUE            (1UL << 2)      /* Transmit Unavailable Enable */
#define DMA_INTR_TJE            (1UL << 3)      /* Transmit Jabber Enable */
#define DMA_INTR_OVE            (1UL << 4)      /* Overflow Interrupt Enable */
#define DMA_INTR_UNE            (1UL << 5)      /* Underflow Interrupt Enable */
#define DMA_INTR_RIE            (1UL << 6)      /* Receive Interrupt Enable */
#define DMA_INTR_RUE            (1UL << 7)      /* Receive Unavailable Enable */
#define DMA_INTR_RSE            (1UL << 8)      /* Receive Stopped Enable */
#define DMA_INTR_RWE            (1UL << 9)      /* Receive Watchdog Enable */
#define DMA_INTR_ETE            (1UL << 10)     /* Early Transmit Interrupt Enable */
#define DMA_INTR_FBE            (1UL << 13)     /* Fatal Bus Error Enable */
#define DMA_INTR_ERE            (1UL << 14)     /* Early Receive Interrupt Enable */
#define DMA_INTR_AIE            (1UL << 15)     /* Abnormal Interrupt Summary Enable */
#define DMA_INTR_NIE            (1UL << 16)     /* Normal Interrupt Summary Enable */

/* DMA Descriptor List Address Registers */
#define GETH_DMA_RX_DESC_LIST_ADDR_OFFSET   0x100C
#define GETH_DMA_TX_DESC_LIST_ADDR_OFFSET   0x1010
#define GETH_DMA_RX_DESC_LIST_ADDR          (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_RX_DESC_LIST_ADDR_OFFSET)))
#define GETH_DMA_TX_DESC_LIST_ADDR          (*((volatile uint32_t *)(GETH0_BASE_ADDR + GETH_DMA_TX_DESC_LIST_ADDR_OFFSET)))

/*==============================================================================
 * DMA Descriptor Definitions (Enhanced Descriptors with IEEE 1588)
 *============================================================================*/

/* Transmit Descriptor */
typedef struct {
    volatile uint32_t tdes0;    /* Status */
    volatile uint32_t tdes1;    /* Control */
    volatile uint32_t tdes2;    /* Buffer1 Address / Header Length */
    volatile uint32_t tdes3;    /* Buffer2 Address / Control */
    /* Extended descriptors for IEEE 1588 */
    volatile uint32_t tdes4;    /* Extended Status */
    volatile uint32_t tdes5;    /* Reserved */
    volatile uint32_t tdes6;    /* Transmit Time Stamp Low */
    volatile uint32_t tdes7;    /* Transmit Time Stamp High */
} geth_tx_desc_t;

/* Transmit Descriptor TDES0 Bits */
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
#define TDES0_IHE               (1UL << 16)     /* IP Header Error */
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
    volatile uint32_t rdes3;    /* Buffer2 Address / Control */
    /* Extended descriptors for IEEE 1588 */
    volatile uint32_t rdes4;    /* Extended Status */
    volatile uint32_t rdes5;    /* Reserved */
    volatile uint32_t rdes6;    /* Receive Time Stamp Low */
    volatile uint32_t rdes7;    /* Receive Time Stamp High */
} geth_rx_desc_t;

/* Receive Descriptor RDES0 Bits */
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

/* Receive Descriptor RDES1 Bits */
#define RDES1_DIC               (1UL << 31)     /* Disable Interrupt on Completion */
#define RDES1_RBS2_SHIFT        16              /* Buffer 2 Size */
#define RDES1_RER               (1UL << 15)     /* Receive End of Ring */
#define RDES1_RCH               (1UL << 14)     /* Second Address Chained */
#define RDES1_RBS1_SHIFT        0               /* Buffer 1 Size */

/*==============================================================================
 * Configuration Structures
 *============================================================================*/

typedef enum {
    GETH_SPEED_10M = 0,
    GETH_SPEED_100M,
    GETH_SPEED_1000M
} geth_speed_t;

typedef enum {
    GETH_DUPLEX_HALF = 0,
    GETH_DUPLEX_FULL
} geth_duplex_t;

typedef enum {
    GETH_MII = 0,
    GETH_RMII,
    GETH_RGMII
} geth_interface_t;

typedef struct {
    geth_speed_t speed;
    geth_duplex_t duplex;
    geth_interface_t interface;
    bool enable_1588;
    bool enable_vlan;
    bool enable_jumbo;
    uint8_t mac_addr[6];
    uint16_t rx_desc_count;
    uint16_t tx_desc_count;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
} geth_config_t;

typedef struct {
    uint32_t base_addr;
    geth_tx_desc_t *tx_desc;
    geth_rx_desc_t *rx_desc;
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
} geth_handle_t;

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Initialization */
int geth_init(uint8_t instance, const geth_config_t *config);
int geth_deinit(uint8_t instance);
void geth_get_default_config(geth_config_t *config);

/* MAC Control */
int geth_set_mac_address(uint8_t instance, const uint8_t mac[6]);
int geth_get_mac_address(uint8_t instance, uint8_t mac[6]);
int geth_set_speed(uint8_t instance, geth_speed_t speed, geth_duplex_t duplex);
int geth_enable(uint8_t instance);
int geth_disable(uint8_t instance);

/* Transmit/Receive */
int geth_send_frame(uint8_t instance, const uint8_t *data, uint32_t len, bool timestamp);
int geth_receive_frame(uint8_t instance, uint8_t *buffer, uint32_t max_len, uint64_t *timestamp);

/* Interrupt Handling */
void geth_irq_handler(uint8_t instance);
int geth_enable_irq(uint8_t instance, uint32_t mask);
int geth_disable_irq(uint8_t instance, uint32_t mask);

/* MDIO Interface */
int geth_mdio_read(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t *data);
int geth_mdio_write(uint8_t instance, uint8_t phy_addr, uint8_t reg_addr, uint16_t data);

/* Statistics */
void geth_get_stats(uint8_t instance, uint32_t *rx_frames, uint32_t *tx_frames,
                    uint32_t *rx_errors, uint32_t *tx_errors);

/* Debug */
void geth_dump_regs(uint8_t instance);

#endif /* GETH_DRIVER_H */
