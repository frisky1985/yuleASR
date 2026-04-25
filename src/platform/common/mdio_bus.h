/*
 * mdio_bus.h
 * MDIO (Management Data Input/Output) Bus Management
 * IEEE 802.3 Clause 22 and Clause 45 support
 */

#ifndef MDIO_BUS_H
#define MDIO_BUS_H

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * MDIO Definitions
 *============================================================================*/

/* Clause 22 (Standard) PHY Addresses */
#define MDIO_PHY_ADDR_MIN       0
#define MDIO_PHY_ADDR_MAX       31
#define MDIO_BROADCAST_ADDR     0x1F

/* Clause 22 Register Addresses */
#define MDIO_REG_CTRL           0x00        /* Control Register */
#define MDIO_REG_STAT           0x01        /* Status Register */
#define MDIO_REG_PHY_ID1        0x02        /* PHY Identifier 1 */
#define MDIO_REG_PHY_ID2        0x03        /* PHY Identifier 2 */
#define MDIO_REG_AN_ADV         0x04        /* Auto-Negotiation Advertisement */
#define MDIO_REG_AN_LPA         0x05        /* Auto-Negotiation Link Partner Ability */
#define MDIO_REG_AN_EXP         0x06        /* Auto-Negotiation Expansion */
#define MDIO_REG_AN_NP          0x07        /* Auto-Negotiation Next Page */
#define MDIO_REG_AN_LPNP        0x08        /* Auto-Negotiation Link Partner Next Page */
#define MDIO_REG_1000T_CTRL     0x09        /* 1000BASE-T Control */
#define MDIO_REG_1000T_STAT     0x0A        /* 1000BASE-T Status */
#define MDIO_REG_EXT_STAT       0x0F        /* Extended Status */

/* Clause 45 (Extended) Device Addresses */
#define MDIO_DEV_PMAPMD         0x01        /* PMA/PMD */
#define MDIO_DEV_WIS            0x02        /* WIS */
#define MDIO_DEV_PCS            0x03        /* PCS */
#define MDIO_DEV_PHYXS          0x04        /* PHY XS */
#define MDIO_DEV_DTEXS          0x05        /* DTE XS */
#define MDIO_DEV_TC             0x06        /* TC */
#define MDIO_DEV_AN             0x07        /* Auto-Negotiation */
#define MDIO_DEV_C22EXT         0x1D        /* Clause 22 Extension */
#define MDIO_DEV_VEND1          0x1E        /* Vendor Specific 1 */
#define MDIO_DEV_VEND2          0x1F        /* Vendor Specific 2 */

/* Clause 45 Register Addresses */
#define MDIO_C45_CTRL1          0x0000      /* Control 1 */
#define MDIO_C45_STAT1          0x0001      /* Status 1 */
#define MDIO_C45_DEV_ID1        0x0002      /* Device Identifier 1 */
#define MDIO_C45_DEV_ID2        0x0003      /* Device Identifier 2 */
#define MDIO_C45_SPEED_ABILITY  0x0004      /* Speed Ability */
#define MDIO_C45_DEV_PKG1       0x0005      /* Devices in Package 1 */
#define MDIO_C45_DEV_PKG2       0x0006      /* Devices in Package 2 */
#define MDIO_C45_CTRL2          0x0007      /* Control 2 */
#define MDIO_C45_STAT2          0x0008      /* Status 2 */
#define MDIO_C45_PKG_ID1        0x000E      /* Package Identifier 1 */
#define MDIO_C45_PKG_ID2        0x000F      /* Package Identifier 2 */

/* Control Register Bits */
#define MDIO_CTRL_RESET         (1 << 15)
#define MDIO_CTRL_LOOPBACK      (1 << 14)
#define MDIO_CTRL_SPEED_SEL     (1 << 13)
#define MDIO_CTRL_AN_EN         (1 << 12)
#define MDIO_CTRL_PWR_DOWN      (1 << 11)
#define MDIO_CTRL_ISOLATE       (1 << 10)
#define MDIO_CTRL_RESTART_AN    (1 << 9)
#define MDIO_CTRL_DUPLEX        (1 << 8)
#define MDIO_CTRL_COL_TEST      (1 << 7)
#define MDIO_CTRL_SPEED1000     (1 << 6)    /* Speed Select MSB */

/* Status Register Bits */
#define MDIO_STAT_100BT4        (1 << 15)
#define MDIO_STAT_100BTX_FD     (1 << 14)
#define MDIO_STAT_100BTX_HD     (1 << 13)
#define MDIO_STAT_10BT_FD       (1 << 12)
#define MDIO_STAT_10BT_HD       (1 << 11)
#define MDIO_STAT_100BT2_FD     (1 << 10)
#define MDIO_STAT_100BT2_HD     (1 << 9)
#define MDIO_STAT_EXT_STAT      (1 << 8)
#define MDIO_STAT_AN_COMPLETE   (1 << 5)
#define MDIO_STAT_REMOTE_FAULT  (1 << 4)
#define MDIO_STAT_AN_ABILITY    (1 << 3)
#define MDIO_STAT_LINK_STAT     (1 << 2)    /* Latch low */
#define MDIO_STAT_JABBER_DET    (1 << 1)
#define MDIO_STAT_EXT_CAP       (1 << 0)

/*==============================================================================
 * MDIO Bus Operations
 *============================================================================*/

typedef enum {
    MDIO_OP_C22_READ = 0,
    MDIO_OP_C22_WRITE = 1,
    MDIO_OP_C45_ADDRESS = 0,    /* Set register address */
    MDIO_OP_C45_WRITE = 1,      /* Write to set address */
    MDIO_OP_C45_READ_INC = 2,   /* Read and increment address */
    MDIO_OP_C45_READ = 3        /* Read from current address */
} mdio_opcode_t;

typedef enum {
    MDIO_BUS_OK = 0,
    MDIO_BUS_ERROR,
    MDIO_BUS_TIMEOUT,
    MDIO_BUS_BUSY
} mdio_bus_status_t;

/*==============================================================================
 * MDIO Bus Handle
 *============================================================================*/

typedef struct {
    /* Hardware interface functions */
    int (*read_c22)(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value);
    int (*write_c22)(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);
    int (*read_c45)(uint8_t phy_addr, uint8_t dev_addr, uint16_t reg_addr, uint16_t *value);
    int (*write_c45)(uint8_t phy_addr, uint8_t dev_addr, uint16_t reg_addr, uint16_t value);
    
    /* Bus configuration */
    uint32_t clock_freq;        /* MDIO clock frequency in Hz */
    bool use_c45;               /* Use Clause 45 access */
    uint8_t phy_addr_mask;      /* PHY addresses present on bus */
    
    /* Statistics */
    uint32_t access_count;
    uint32_t error_count;
} mdio_bus_handle_t;

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Bus Initialization */
int mdio_bus_init(mdio_bus_handle_t *bus);
int mdio_bus_deinit(mdio_bus_handle_t *bus);
void mdio_bus_get_default_config(mdio_bus_handle_t *bus);

/* Clause 22 Operations (Standard) */
int mdio_c22_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t reg_addr, uint16_t *value);
int mdio_c22_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t reg_addr, uint16_t value);

/* Clause 45 Operations (Extended) */
int mdio_c45_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t dev_addr, uint16_t reg_addr, uint16_t *value);
int mdio_c45_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint8_t dev_addr, uint16_t reg_addr, uint16_t value);

/* Convenience Functions */
int mdio_read(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t reg_addr, uint16_t *value);
int mdio_write(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t reg_addr, uint16_t value);

/* PHY Discovery and Management */
int mdio_scan_bus(mdio_bus_handle_t *bus, uint32_t *phy_mask);
int mdio_get_phy_id(mdio_bus_handle_t *bus, uint8_t phy_addr, uint32_t *phy_id);
int mdio_reset_phy(mdio_bus_handle_t *bus, uint8_t phy_addr);
int mdio_get_link_status(mdio_bus_handle_t *bus, uint8_t phy_addr, bool *link_up);

/* PHY Configuration */
int mdio_set_speed_duplex(mdio_bus_handle_t *bus, uint8_t phy_addr, uint16_t speed, bool full_duplex);
int mdio_enable_autoneg(mdio_bus_handle_t *bus, uint8_t phy_addr);
int mdio_restart_autoneg(mdio_bus_handle_t *bus, uint8_t phy_addr);
int mdio_power_down(mdio_bus_handle_t *bus, uint8_t phy_addr, bool down);

/* Debug */
void mdio_dump_phy_regs(mdio_bus_handle_t *bus, uint8_t phy_addr);
const char *mdio_speed_to_str(uint16_t speed);

#endif /* MDIO_BUS_H */
