/*
 * pinmux_config.h
 * Pin Multiplexing Configuration Header
 * Defines pin mappings for different target platforms
 */

#ifndef PINMUX_CONFIG_H
#define PINMUX_CONFIG_H

#include <stdint.h>
#include "board_config.h"

/*==============================================================================
 * Pin Configuration Structure
 *============================================================================*/

typedef enum {
    PIN_FUNC_GPIO = 0,
    PIN_FUNC_UART,
    PIN_FUNC_SPI,
    PIN_FUNC_I2C,
    PIN_FUNC_CAN,
    PIN_FUNC_ETH_MII,
    PIN_FUNC_ETH_RMII,
    PIN_FUNC_ETH_RGMII,
    PIN_FUNC_ETH_MDIO,
    PIN_FUNC_ETH_MDC,
    PIN_FUNC_QSPI,
    PIN_FUNC_SDHC,
    PIN_FUNC_USB,
    PIN_FUNC_FLEXRAY,
    PIN_FUNC_DISABLED
} pin_function_t;

typedef enum {
    PIN_DIR_INPUT = 0,
    PIN_DIR_OUTPUT,
    PIN_DIR_BIDIR
} pin_direction_t;

typedef enum {
    PIN_PULL_NONE = 0,
    PIN_PULL_UP,
    PIN_PULL_DOWN
} pin_pull_t;

typedef enum {
    PIN_DRIVE_LOW = 0,
    PIN_DRIVE_MEDIUM,
    PIN_DRIVE_HIGH
} pin_drive_t;

typedef struct {
    uint32_t port;              /* Port number */
    uint32_t pin;               /* Pin number within port */
    pin_function_t function;    /* Pin function */
    pin_direction_t direction;  /* Pin direction */
    pin_pull_t pull;            /* Pull-up/down configuration */
    pin_drive_t drive;          /* Drive strength */
    uint32_t flags;             /* Additional flags */
} pinmux_cfg_t;

/* Pin configuration flags */
#define PIN_FLAG_OPEN_DRAIN     (1 << 0)
#define PIN_FLAG_SLEW_SLOW      (1 << 1)
#define PIN_FLAG_SLEW_FAST      (1 << 2)
#define PIN_FLAG_HYSTERESIS     (1 << 3)
#define PIN_FLAG_WAKEUP         (1 << 4)

/*==============================================================================
 * NXP S32G3 Pin Mux Configuration
 *============================================================================*/
#ifdef BOARD_S32G3

/* ENET0 - RGMII Interface */
#define ENET0_RGMII_RXC_PORT        4
#define ENET0_RGMII_RXC_PIN         0
#define ENET0_RGMII_RXD0_PORT       4
#define ENET0_RGMII_RXD0_PIN        1
#define ENET0_RGMII_RXD1_PORT       4
#define ENET0_RGMII_RXD1_PIN        2
#define ENET0_RGMII_RXD2_PORT       4
#define ENET0_RGMII_RXD2_PIN        3
#define ENET0_RGMII_RXD3_PORT       4
#define ENET0_RGMII_RXD3_PIN        4
#define ENET0_RGMII_RX_CTL_PORT     4
#define ENET0_RGMII_RX_CTL_PIN      5

#define ENET0_RGMII_TXC_PORT        4
#define ENET0_RGMII_TXC_PIN         6
#define ENET0_RGMII_TXD0_PORT       4
#define ENET0_RGMII_TXD0_PIN        7
#define ENET0_RGMII_TXD1_PORT       4
#define ENET0_RGMII_TXD1_PIN        8
#define ENET0_RGMII_TXD2_PORT       4
#define ENET0_RGMII_TXD2_PIN        9
#define ENET0_RGMII_TXD3_PORT       4
#define ENET0_RGMII_TXD3_PIN        10
#define ENET0_RGMII_TX_CTL_PORT     4
#define ENET0_RGMII_TX_CTL_PIN      11

#define ENET0_MDIO_PORT             4
#define ENET0_MDIO_PIN              12
#define ENET0_MDC_PORT              4
#define ENET0_MDC_PIN               13

/* ENET1 - RGMII Interface */
#define ENET1_RGMII_RXC_PORT        3
#define ENET1_RGMII_RXC_PIN         0
#define ENET1_RGMII_RXD0_PORT       3
#define ENET1_RGMII_RXD0_PIN        1
#define ENET1_RGMII_RXD1_PORT       3
#define ENET1_RGMII_RXD1_PIN        2
#define ENET1_RGMII_RXD2_PORT       3
#define ENET1_RGMII_RXD2_PIN        3
#define ENET1_RGMII_RXD3_PORT       3
#define ENET1_RGMII_RXD3_PIN        4
#define ENET1_RGMII_RX_CTL_PORT     3
#define ENET1_RGMII_RX_CTL_PIN      5

#define ENET1_RGMII_TXC_PORT        3
#define ENET1_RGMII_TXC_PIN         6
#define ENET1_RGMII_TXD0_PORT       3
#define ENET1_RGMII_TXD0_PIN        7
#define ENET1_RGMII_TXD1_PORT       3
#define ENET1_RGMII_TXD1_PIN        8
#define ENET1_RGMII_TXD2_PORT       3
#define ENET1_RGMII_TXD2_PIN        9
#define ENET1_RGMII_TXD3_PORT       3
#define ENET1_RGMII_TXD3_PIN        10
#define ENET1_RGMII_TX_CTL_PORT     3
#define ENET1_RGMII_TX_CTL_PIN      11

#define ENET1_MDIO_PORT             3
#define ENET1_MDIO_PIN              12
#define ENET1_MDC_PORT              3
#define ENET1_MDC_PIN               13

/* UART Configuration */
#define UART0_TX_PORT               0
#define UART0_TX_PIN                0
#define UART0_RX_PORT               0
#define UART0_RX_PIN                1

/* CAN Configuration (CAN_FD) */
#define CAN0_TX_PORT                1
#define CAN0_TX_PIN                 0
#define CAN0_RX_PORT                1
#define CAN0_RX_PIN                 1

/* SPI Configuration */
#define SPI0_SCK_PORT               2
#define SPI0_SCK_PIN                0
#define SPI0_MOSI_PORT              2
#define SPI0_MOSI_PIN               1
#define SPI0_MISO_PORT              2
#define SPI0_MISO_PIN               2
#define SPI0_CS0_PORT               2
#define SPI0_CS0_PIN                3

/* I2C Configuration */
#define I2C0_SCL_PORT               5
#define I2C0_SCL_PIN                0
#define I2C0_SDA_PORT               5
#define I2C0_SDA_PIN                1

/* LED GPIO */
#define LED0_PORT                   6
#define LED0_PIN                    0
#define LED1_PORT                   6
#define LED1_PIN                    1
#define LED2_PORT                   6
#define LED2_PIN                    2

/* PTP Timestamp GPIO */
#define PTP_PPS_PORT                6
#define PTP_PPS_PIN                 10
#define PTP_EXT_TRIG_PORT           6
#define PTP_EXT_TRIG_PIN            11

#endif /* BOARD_S32G3 */

/*==============================================================================
 * Infineon AURIX TC3xx Pin Mux Configuration
 *============================================================================*/
#ifdef BOARD_AURIX_TC3XX

/* GETH0 - RMII Interface */
#define GETH0_RMII_REF_CLK_PORT     14
#define GETH0_RMII_REF_CLK_PIN      0
#define GETH0_RMII_RX_DV_PORT       14
#define GETH0_RMII_RX_DV_PIN        1
#define GETH0_RMII_TX_EN_PORT       14
#define GETH0_RMII_TX_EN_PIN        2
#define GETH0_RMII_TXD0_PORT        14
#define GETH0_RMII_TXD0_PIN         3
#define GETH0_RMII_TXD1_PORT        14
#define GETH0_RMII_TXD1_PIN         4
#define GETH0_RMII_RXD0_PORT        14
#define GETH0_RMII_RXD0_PIN         5
#define GETH0_RMII_RXD1_PORT        14
#define GETH0_RMII_RXD1_PIN         6
#define GETH0_RMII_RX_ER_PORT       14
#define GETH0_RMII_RX_ER_PIN        7

#define GETH0_MDIO_PORT             15
#define GETH0_MDIO_PIN              0
#define GETH0_MDC_PORT              15
#define GETH0_MDC_PIN               1

/* GETH1 - RMII Interface */
#define GETH1_RMII_REF_CLK_PORT     20
#define GETH1_RMII_REF_CLK_PIN      3
#define GETH1_RMII_RX_DV_PORT       20
#define GETH1_RMII_RX_DV_PIN        0
#define GETH1_RMII_TX_EN_PORT       20
#define GETH1_RMII_TX_EN_PIN        2
#define GETH1_RMII_TXD0_PORT        20
#define GETH1_RMII_TXD0_PIN         1
#define GETH1_RMII_TXD1_PORT        11
#define GETH1_RMII_TXD1_PIN         12
#define GETH1_RMII_RXD0_PORT        11
#define GETH1_RMII_RXD0_PIN         10
#define GETH1_RMII_RXD1_PORT        11
#define GETH1_RMII_RXD1_PIN         11

#define GETH1_MDIO_PORT             11
#define GETH1_MDIO_PIN              14
#define GETH1_MDC_PORT              11
#define GETH1_MDC_PIN               15

/* ASCLIN (UART) Configuration */
#define ASCLIN0_TX_PORT             14
#define ASCLIN0_TX_PIN              0
#define ASCLIN0_RX_PORT             14
#define ASCLIN0_RX_PIN              1

/* MultiCAN+ Configuration */
#define CAN0_TX_PORT                20
#define CAN0_TX_PIN                 8
#define CAN0_RX_PORT                20
#define CAN0_RX_PIN                 7

/* QSPI Configuration */
#define QSPI0_SCLK_PORT             20
#define QSPI0_SCLK_PIN              11
#define QSPI0_MOSI_PORT             20
#define QSPI0_MOSI_PIN              12
#define QSPI0_MISO_PORT             20
#define QSPI0_MISO_PIN              13
#define QSPI0_SLSO0_PORT            20
#define QSPI0_SLSO0_PIN             14

/* LED GPIO (TriBoard LEDs on Port 23) */
#define LED0_PORT                   23
#define LED0_PIN                    0
#define LED1_PORT                   23
#define LED1_PIN                    1
#define LED2_PORT                   23
#define LED2_PIN                    2
#define LED3_PORT                   23
#define LED3_PIN                    3
#define LED4_PORT                   23
#define LED4_PIN                    4
#define LED5_PORT                   23
#define LED5_PIN                    5
#define LED6_PORT                   23
#define LED6_PIN                    6
#define LED7_PORT                   23
#define LED7_PIN                    7

#endif /* BOARD_AURIX_TC3XX */

/*==============================================================================
 * POSIX/Linux Configuration (Virtual/Placeholder)
 *============================================================================*/
#ifdef BOARD_POSIX

/* Virtual pins for simulation */
#define VIRT_ETH_IF_NAME            "eth0"
#define VIRT_ETH_IF_INDEX           0

/* Virtual GPIO */
#define VIRT_GPIO_BASE              0

#endif /* BOARD_POSIX */

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* Pin mux initialization */
int pinmux_init(void);
int pinmux_deinit(void);

/* Pin configuration */
int pinmux_configure(const pinmux_cfg_t *cfg);
int pinmux_configure_array(const pinmux_cfg_t *cfg_array, uint32_t count);

/* GPIO functions */
int pinmux_gpio_set(uint32_t port, uint32_t pin, uint32_t value);
int pinmux_gpio_get(uint32_t port, uint32_t pin, uint32_t *value);
int pinmux_gpio_toggle(uint32_t port, uint32_t pin);

/* Platform-specific helpers */
#ifdef BOARD_S32G3
    int pinmux_configure_enet_rgmii(uint8_t enet_instance);
    int pinmux_configure_enet_mdio(uint8_t enet_instance);
#endif

#ifdef BOARD_AURIX_TC3XX
    int pinmux_configure_geth_rmii(uint8_t geth_instance);
    int pinmux_configure_geth_mdio(uint8_t geth_instance);
    int pinmux_configure_leds(void);
#endif

/* Pre-defined pin configurations */
extern const pinmux_cfg_t enet_pinmux_cfg[];
extern const uint32_t enet_pinmux_count;

extern const pinmux_cfg_t uart_pinmux_cfg[];
extern const uint32_t uart_pinmux_count;

extern const pinmux_cfg_t can_pinmux_cfg[];
extern const uint32_t can_pinmux_count;

extern const pinmux_cfg_t spi_pinmux_cfg[];
extern const uint32_t spi_pinmux_count;

extern const pinmux_cfg_t i2c_pinmux_cfg[];
extern const uint32_t i2c_pinmux_count;

#ifdef __cplusplus
}
#endif

#endif /* PINMUX_CONFIG_H */
