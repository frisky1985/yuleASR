/*
 * phy_tja1102.h
 * NXP TJA1102 Dual-Port 100BASE-T1 Ethernet PHY Driver
 * Two independent 100BASE-T1 PHYs in single package
 */

#ifndef PHY_TJA1102_H
#define PHY_TJA1102_H

#include "phy_tja1101.h"    /* TJA1102 uses same register map as TJA1101 per port */
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * TJA1102 PHY IDs
 *============================================================================*/
#define TJA1102_PHY_ID_1        0x0180      /* Same OUI as TJA1101 */
#define TJA1102_PHY_ID_2        0xDD00      /* Different model number */
#define TJA1102_PHY_ID_2_MASK   0xFFF0

/*==============================================================================
 * TJA1102 Specific Register Definitions
 * Some registers are shared between ports
 *============================================================================*/

/* Port addresses - TJA1102 has two PHYs */
#define TJA1102_PORT_0          0
#define TJA1102_PORT_1          1

/* Shared Configuration Registers (accessible from either port) */
#define TJA1102_SHARED_CONFIG   0x1C        /* Shared configuration */
#define TJA1102_SHARED_CTRL     0x1D        /* Shared control */
#define TJA1102_SHARED_STAT     0x1E        /* Shared status */

/*==============================================================================
 * TJA1102 Shared Configuration Register (0x1C) Bits
 *============================================================================*/
#define TJA1102_SHARED_CFG_REF_CLOCK_SHIFT      0       /* Reference Clock Select */
#define TJA1102_SHARED_CFG_REF_CLOCK_MASK       0x0003
#define TJA1102_SHARED_CFG_CLK_OUT_EN           (1 << 2)    /* Clock Output Enable */
#define TJA1102_SHARED_CFG_CLK_OUT_SHIFT        3       /* Clock Output Select */
#define TJA1102_SHARED_CFG_CLK_OUT_MASK         0x0038
#define TJA1102_SHARED_CFG_FAILSAFE_EN          (1 << 6)    /* Failsafe Enable */
#define TJA1102_SHARED_CFG_FAILSAFE_MODE_SHIFT  7       /* Failsafe Mode */
#define TJA1102_SHARED_CFG_FAILSAFE_MODE_MASK   0x0180

/*==============================================================================
 * TJA1102 Shared Control Register (0x1D) Bits
 *============================================================================*/
#define TJA1102_SHARED_CTRL_RESET_PORT0         (1 << 0)    /* Reset Port 0 */
#define TJA1102_SHARED_CTRL_RESET_PORT1         (1 << 1)    /* Reset Port 1 */
#define TJA1102_SHARED_CTRL_WAKE_PORT0          (1 << 2)    /* Wake Port 0 */
#define TJA1102_SHARED_CTRL_WAKE_PORT1          (1 << 3)    /* Wake Port 1 */
#define TJA1102_SHARED_CTRL_SLEEP_REQ_PORT0     (1 << 4)    /* Sleep Request Port 0 */
#define TJA1102_SHARED_CTRL_SLEEP_REQ_PORT1     (1 << 5)    /* Sleep Request Port 1 */

/*==============================================================================
 * TJA1102 Shared Status Register (0x1E) Bits
 *============================================================================*/
#define TJA1102_SHARED_STAT_INT_PORT0           (1 << 0)    /* Interrupt Port 0 */
#define TJA1102_SHARED_STAT_INT_PORT1           (1 << 1)    /* Interrupt Port 1 */
#define TJA1102_SHARED_STAT_WAKE_PORT0          (1 << 2)    /* Wake-up Port 0 */
#define TJA1102_SHARED_STAT_WAKE_PORT1          (1 << 3)    /* Wake-up Port 1 */

/*==============================================================================
 * Configuration Structure
 *============================================================================*/

typedef struct {
    tja1101_config_t port_config[2];    /* Config for each port */
    bool shared_ref_clock;              /* Use shared reference clock */
    uint8_t ref_clock_sel;              /* Reference clock selection */
    bool clock_output_en;               /* Enable clock output */
    bool failsafe_en;                   /* Enable failsafe mode */
} tja1102_config_t;

typedef struct {
    uint8_t base_phy_addr;              /* Base PHY address (port 0) */
    uint32_t (*mdio_read)(uint8_t phy_addr, uint8_t reg_addr);
    int (*mdio_write)(uint8_t phy_addr, uint8_t reg_addr, uint16_t data);
} tja1102_handle_t;

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/* Initialization */
int tja1102_init(tja1102_handle_t *handle, const tja1102_config_t *config);
int tja1102_deinit(tja1102_handle_t *handle);
void tja1102_get_default_config(tja1102_config_t *config);

/* PHY Identification */
int tja1102_identify(tja1102_handle_t *handle, bool *is_tja1102);

/* Port Management */
int tja1102_get_port_phy_addr(tja1102_handle_t *handle, uint8_t port, uint8_t *phy_addr);

/* Individual Port Operations (use TJA1101 functions after getting phy_addr) */
int tja1102_init_port(tja1102_handle_t *handle, uint8_t port, const tja1101_config_t *config);
int tja1102_get_port_link_status(tja1102_handle_t *handle, uint8_t port, bool *link_up);
int tja1102_get_port_sqi(tja1102_handle_t *handle, uint8_t port, uint8_t *sqi);

/* Shared Control */
int tja1102_shared_reset(tja1102_handle_t *handle, uint8_t port_mask);
int tja1102_shared_wake(tja1102_handle_t *handle, uint8_t port_mask);
int tja1102_shared_sleep_request(tja1102_handle_t *handle, uint8_t port_mask);

/* Interrupt Management */
int tja1102_get_shared_interrupts(tja1102_handle_t *handle, uint8_t *int_mask);
int tja1102_clear_shared_interrupts(tja1102_handle_t *handle, uint8_t int_mask);

/* Diagnostics */
void tja1102_print_status(tja1102_handle_t *handle);

#endif /* PHY_TJA1102_H */
