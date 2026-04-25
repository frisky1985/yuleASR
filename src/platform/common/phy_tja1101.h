/*
 * phy_tja1101.h
 * NXP TJA1101 100BASE-T1 Single-Port Ethernet PHY Driver
 * Automotive Ethernet PHY for 100Mbps over single twisted pair
 */

#ifndef PHY_TJA1101_H
#define PHY_TJA1101_H

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * TJA1101 PHY ID
 *============================================================================*/
#define TJA1101_PHY_ID_1        0x0180      /* PHY ID 1 register value */
#define TJA1101_PHY_ID_2        0xDC00      /* PHY ID 2 register value (mask) */
#define TJA1101_PHY_ID_2_MASK   0xFFF0

/*==============================================================================
 * Standard MII Registers (Address 0-15)
 *============================================================================*/
#define MII_CTRL_REG            0x00        /* Control Register */
#define MII_STAT_REG            0x01        /* Status Register */
#define MII_PHY_ID1_REG         0x02        /* PHY Identifier 1 */
#define MII_PHY_ID2_REG         0x03        /* PHY Identifier 2 */
#define MII_AN_ADV_REG          0x04        /* Auto-Negotiation Advertisement */
#define MII_AN_LPA_REG          0x05        /* Auto-Negotiation Link Partner Ability */
#define MII_AN_EXP_REG          0x06        /* Auto-Negotiation Expansion */
#define MII_1000BT_CTRL         0x09        /* 1000BASE-T Control (not used for 100BT1) */
#define MII_1000BT_STAT         0x0A        /* 1000BASE-T Status (not used for 100BT1) */

/* Extended Registers (Address 16-31) */
#define MII_EXT_STAT_REG        0x0F        /* Extended Status */

/*==============================================================================
 * TJA1101 Extended Registers (SMI Address > 15)
 *============================================================================*/
#define TJA1101_PHY_ID          0x10        /* Extended PHY Identifier */
#define TJA1101_EXT_CTRL        0x11        /* Extended Control */
#define TJA1101_CONFIG1         0x12        /* Configuration 1 */
#define TJA1101_CONFIG2         0x13        /* Configuration 2 */
#define TJA1101_SYM_ERR_COUNTER 0x14        /* Symbol Error Counter */
#define TJA1101_INT_SRC         0x15        /* Interrupt Source */
#define TJA1101_INT_EN          0x16        /* Interrupt Enable */
#define TJA1101_COMM_STAT       0x17        /* Communication Status */
#define TJA1101_GENERAL_STAT    0x18        /* General Status */
#define TJA1101_EXTERNAL_STAT   0x19        /* External Status */
#define TJA1101_LINK_FAIL_CNT   0x1A        /* Link Fail Counter */
#define TJA1101_PHY_CTRL        0x1B        /* PHY Control */

/*==============================================================================
 * MII Control Register (0x00) Bits
 *============================================================================*/
#define MII_CTRL_RESET          (1 << 15)   /* PHY Reset */
#define MII_CTRL_LOOPBACK       (1 << 14)   /* Loopback Mode */
#define MII_CTRL_SPEED_SEL      (1 << 13)   /* Speed Select (1=100Mbps, 0=10Mbps) */
#define MII_CTRL_AN_EN          (1 << 12)   /* Auto-Negotiation Enable */
#define MII_CTRL_PWR_DOWN       (1 << 11)   /* Power Down */
#define MII_CTRL_ISOLATE        (1 << 10)   /* Isolate */
#define MII_CTRL_RESTART_AN     (1 << 9)    /* Restart Auto-Negotiation */
#define MII_CTRL_DUPLEX         (1 << 8)    /* Duplex Mode (1=Full) */
#define MII_CTRL_COL_TEST       (1 << 7)    /* Collision Test */

/*==============================================================================
 * MII Status Register (0x01) Bits
 *============================================================================*/
#define MII_STAT_100BT4         (1 << 15)   /* 100BASE-T4 Support */
#define MII_STAT_100BTX_FD      (1 << 14)   /* 100BASE-TX Full Duplex */
#define MII_STAT_100BTX_HD      (1 << 13)   /* 100BASE-TX Half Duplex */
#define MII_STAT_10BT_FD        (1 << 12)   /* 10BASE-T Full Duplex */
#define MII_STAT_10BT_HD        (1 << 11)   /* 10BASE-T Half Duplex */
#define MII_STAT_100BT2_FD      (1 << 10)   /* 100BASE-T2 Full Duplex */
#define MII_STAT_100BT2_HD      (1 << 9)    /* 100BASE-T2 Half Duplex */
#define MII_STAT_EXT_STAT       (1 << 8)    /* Extended Status */
#define MII_STAT_AN_COMPLETE    (1 << 5)    /* Auto-Negotiation Complete */
#define MII_STAT_REMOTE_FAULT   (1 << 4)    /* Remote Fault */
#define MII_STAT_AN_ABILITY     (1 << 3)    /* Auto-Negotiation Ability */
#define MII_STAT_LINK_STAT      (1 << 2)    /* Link Status (1=up, latch low) */
#define MII_STAT_JABBER_DET     (1 << 1)    /* Jabber Detect */
#define MII_STAT_EXT_CAP        (1 << 0)    /* Extended Capability */

/*==============================================================================
 * TJA1101 Extended Control Register (0x11) Bits
 *============================================================================*/
#define TJA1101_EXT_CTRL_WAKE_REQ       (1 << 14)   /* Wake-up Request */
#define TJA1101_EXT_CTRL_CONFIG_EN      (1 << 11)   /* Configuration Enable */
#define TJA1101_EXT_CTRL_CONFIG_INH     (1 << 10)   /* Configuration Inhibit */
#define TJA1101_EXT_CTRL_WAKE_INH       (1 << 9)    /* Wake-up Inhibit */
#define TJA1101_EXT_CTRL_LPS_ACTIVE     (1 << 8)    /* LPS Active */
#define TJA1101_EXT_CTRL_PHY_MODE       (1 << 7)    /* PHY Mode (1=Slave, 0=Master) */
#define TJA1101_EXT_CTRL_TRAFFIC_DIS    (1 << 6)    /* Traffic Disable */
#define TJA1101_EXT_CTRL_LINK_CTRL      (1 << 4)    /* Link Control */
#define TJA1101_EXT_CTRL_POWER_MODE_SHIFT   0       /* Power Mode */
#define TJA1101_EXT_CTRL_POWER_MODE_MASK    0x0F

/* Power Modes */
#define TJA1101_PM_NORMAL               0x00
#define TJA1101_PM_STANDBY              0x01
#define TJA1101_PM_SLEEP_REQUEST        0x02
#define TJA1101_PM_SLEEP                0x04
#define TJA1101_PM_SILENT               0x08

/*==============================================================================
 * TJA1101 Configuration 1 Register (0x12) Bits
 *============================================================================*/
#define TJA1101_CFG1_MASTER             (1 << 15)   /* Master/Slave Configuration */
#define TJA1101_CFG1_AUTO_MASTER        (1 << 14)   /* Auto Master/Slave */
#define TJA1101_CFG1_AUTO_MASTER_DIS    (1 << 13)   /* Auto Master Disable */
#define TJA1101_CFG1_LED_EN             (1 << 12)   /* LED Enable */
#define TJA1101_CFG1_LED_MODE_SHIFT     10          /* LED Mode */
#define TJA1101_CFG1_TX_AMP_SHIFT       5           /* Transmit Amplitude */
#define TJA1101_CFG1_TX_AMP_MASK        0x03E0
#define TJA1101_CFG1_FUSA_PASS          (1 << 4)    /* FuSa Pass Through */
#define TJA1101_CFG1_FUSA_RECOVER       (1 << 3)    /* FuSa Recover */
#define TJA1101_CFG1_FUSA_INIT          (1 << 2)    /* FuSa Initialization */
#define TJA1101_CFG1_FUSA_BC            (1 << 1)    /* FuSa Broadcast Counter */
#define TJA1101_CFG1_FUSA_ERR           (1 << 0)    /* FuSa Error */

/*==============================================================================
 * TJA1101 Interrupt Source Register (0x15) Bits
 *============================================================================*/
#define TJA1101_INT_PWON                (1 << 15)   /* Power-on Interrupt */
#define TJA1101_INT_WAKE                (1 << 14)   /* Wake-up Interrupt */
#define TJA1101_INT_LINK_FAIL           (1 << 10)   /* Link Failure */
#define TJA1101_INT_LINK_UP             (1 << 9)    /* Link Up */
#define TJA1101_INT_SYM_ERR             (1 << 8)    /* Symbol Error */
#define TJA1101_INT_SYNTAX_ERR          (1 << 7)    /* Syntax Error */
#define TJA1101_INT_FUSA_ERR            (1 << 3)    /* FuSa Error */

/*==============================================================================
 * TJA1101 Communication Status Register (0x17) Bits
 *============================================================================*/
#define TJA1101_COMM_RX_ERR             (1 << 15)   /* Receive Error */
#define TJA1101_COMM_TX_ERR             (1 << 14)   /* Transmit Error */
#define TJA1101_COMM_PHY_STATE_SHIFT    10          /* PHY State */
#define TJA1101_COMM_PHY_STATE_MASK     0x1C00
#define TJA1101_COMM_LOC_RCVR_STAT      (1 << 9)    /* Local Receiver Status */
#define TJA1101_COMM_REM_RCVR_STAT      (1 << 8)    /* Remote Receiver Status */
#define TJA1101_COMM_SCR_LOCKED         (1 << 7)    /* Scrambler Locked */
#define TJA1101_COMM_SSD_ERR            (1 << 6)    /* SSD Error */
#define TJA1101_COMM_ESD_ERR            (1 << 5)    /* ESD Error */
#define TJA1101_COMM_ALIGN_ERR          (1 << 4)    /* Alignment Error */
#define TJA1101_COMM_SQI_SHIFT          0           /* Signal Quality Indicator */
#define TJA1101_COMM_SQI_MASK           0x0007

/* PHY States */
#define TJA1101_STATE_IDLE              0x00
#define TJA1101_STATE_INITIALIZING      0x01
#define TJA1101_STATE_CONFIGURED        0x02
#define TJA1101_STATE_OFFLINE           0x03
#define TJA1101_STATE_ACTIVE            0x04
#define TJA1101_STATE_ISOLATE           0x05

/* SQI Values (Signal Quality Indicator) */
#define TJA1101_SQI_NO_SIGNAL           0
#define TJA1101_SQI_VERY_POOR           1
#define TJA1101_SQI_POOR                2
#define TJA1101_SQI_MODERATE            3
#define TJA1101_SQI_GOOD                4
#define TJA1101_SQI_EXCELLENT           5
#define TJA1101_SQI_RESERVED_1          6
#define TJA1101_SQI_RESERVED_2          7

/*==============================================================================
 * Configuration Structure
 *============================================================================*/

typedef enum {
    TJA1101_MODE_SLAVE = 0,
    TJA1101_MODE_MASTER
} tja1101_mode_t;

typedef struct {
    tja1101_mode_t mode;            /* Master or Slave */
    bool auto_negotiation;          /* Enable auto-negotiation */
    uint8_t tx_amplitude;           /* Transmit amplitude (0-15) */
    bool led_enable;                /* Enable LED output */
    bool fusa_enable;               /* Enable Functional Safety features */
} tja1101_config_t;

typedef struct {
    uint8_t phy_addr;
    uint32_t (*mdio_read)(uint8_t phy_addr, uint8_t reg_addr);
    int (*mdio_write)(uint8_t phy_addr, uint8_t reg_addr, uint16_t data);
} tja1101_handle_t;

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Initialization */
int tja1101_init(tja1101_handle_t *handle, const tja1101_config_t *config);
int tja1101_deinit(tja1101_handle_t *handle);
void tja1101_get_default_config(tja1101_config_t *config);

/* PHY Identification */
int tja1101_identify(tja1101_handle_t *handle, bool *is_tja1101);

/* Link Management */
int tja1101_get_link_status(tja1101_handle_t *handle, bool *link_up);
int tja1101_get_link_speed(tja1101_handle_t *handle, uint32_t *speed_mbps);
int tja1101_restart_autoneg(tja1101_handle_t *handle);

/* Power Management */
int tja1101_set_power_mode(tja1101_handle_t *handle, uint8_t power_mode);
int tja1101_get_power_mode(tja1101_handle_t *handle, uint8_t *power_mode);
int tja1101_enter_sleep(tja1101_handle_t *handle);
int tja1101_wake_up(tja1101_handle_t *handle);

/* Signal Quality */
int tja1101_get_sqi(tja1101_handle_t *handle, uint8_t *sqi);
int tja1101_get_symbol_errors(tja1101_handle_t *handle, uint16_t *errors);

/* Interrupts */
int tja1101_enable_interrupts(tja1101_handle_t *handle, uint16_t mask);
int tja1101_disable_interrupts(tja1101_handle_t *handle, uint16_t mask);
int tja1101_get_interrupt_status(tja1101_handle_t *handle, uint16_t *status);
int tja1101_clear_interrupts(tja1101_handle_t *handle, uint16_t mask);

/* Configuration */
int tja1101_set_master_slave(tja1101_handle_t *handle, tja1101_mode_t mode);
int tja1101_get_phy_state(tja1101_handle_t *handle, uint8_t *state);

/* Diagnostics */
int tja1101_run_cable_test(tja1101_handle_t *handle);
void tja1101_print_status(tja1101_handle_t *handle);

#endif /* PHY_TJA1101_H */
