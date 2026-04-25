/*
 * triboard_init.c
 * Infineon AURIX TC3xx TriBoard Initialization
 * Board-specific setup for TC375/TC397 TriBoards
 */

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * TC3xx SCU (System Control Unit) Registers
 *============================================================================*/

#define SCU_BASE_ADDR           0xF0036000UL

/* Clock Control Registers */
#define SCU_CCUCON0             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x30)))
#define SCU_CCUCON1             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x34)))
#define SCU_CCUCON2             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x38)))
#define SCU_CCUCON5             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x44)))
#define SCU_CCUCON6             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x48)))
#define SCU_CCUCON7             (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x4C)))

/* Oscillator Control */
#define SCU_OSCCON              (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x10)))
#define SCU_SYSPLLCON0          (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x14)))
#define SCU_SYSPLLCON1          (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x18)))
#define SCU_SYSPLLCON2          (*((volatile uint32_t *)(SCU_BASE_ADDR + 0x1C)))

/*==============================================================================
 * TC3xx Port (GPIO) Registers
 *============================================================================*/

#define PORT_BASE_ADDR          0xF003A000UL

#define PORT_IOCR_OFFSET        0x04
#define PORT_OMR_OFFSET         0x04
#define PORT_OUT_OFFSET         0x00

/* Port 14 - Used for Ethernet on TriBoard */
#define PORT14_BASE             (PORT_BASE_ADDR + 0x1400)
#define PORT14_IOCR0            (*((volatile uint32_t *)(PORT14_BASE + PORT_IOCR_OFFSET)))
#define PORT14_IOCR4            (*((volatile uint32_t *)(PORT14_BASE + PORT_IOCR_OFFSET + 4)))
#define PORT14_IOCR8            (*((volatile uint32_t *)(PORT14_BASE + PORT_IOCR_OFFSET + 8)))
#define PORT14_IOCR12           (*((volatile uint32_t *)(PORT14_BASE + PORT_IOCR_OFFSET + 12)))

/* Port 15 - Used for Ethernet MDIO/MDC on TriBoard */
#define PORT15_BASE             (PORT_BASE_ADDR + 0x1500)
#define PORT15_IOCR0            (*((volatile uint32_t *)(PORT15_BASE + PORT_IOCR_OFFSET)))

/* Port 23 - Used for LED/debug on TriBoard */
#define PORT23_BASE             (PORT_BASE_ADDR + 0x2300)
#define PORT23_OUT              (*((volatile uint32_t *)(PORT23_BASE + PORT_OUT_OFFSET)))
#define PORT23_OMR              (*((volatile uint32_t *)(PORT23_BASE + PORT_OMR_OFFSET)))

/*==============================================================================
 * TC3xx GETH Module Registers (External from geth_driver.h)
 *============================================================================*/

#define GETH0_CLC               (*((volatile uint32_t *)(GETH0_BASE_ADDR + 0x000)))
#define GETH0_ID                (*((volatile uint32_t *)(GETH0_BASE_ADDR + 0x008)))

#define GETH_CLC_DISR           (1UL << 0)      /* Module Disable Request */
#define GETH_CLC_DISS           (1UL << 1)      /* Module Disable Status */
#define GETH_CLC_EDIS           (1UL << 3)      /* Sleep Mode Enable */
#define GETH_CLC_SBWE           (1UL << 4)      /* Module Suspend Bit Write Enable */
#define GETH_CLC_FSOE           (1UL << 5)      /* Fast Switch Off Enable */

/*==============================================================================
 * Clock Configuration
 *============================================================================*/

/* Target Clock Frequencies */
#define F_OSC                   20000000UL      /* 20MHz external crystal */
#define F_PLL                   300000000UL     /* 300MHz system PLL */
#define F_SPB                   100000000UL     /* 100MHz system peripheral bus */
#define F_GETH                  100000000UL     /* 100MHz GETH clock */

/*
 * system_clock_init - Initialize system clocks
 * Configure PLL and clock dividers
 */
static void system_clock_init(void)
{
    /* Wait for oscillator stable */
    volatile uint32_t timeout = 10000;
    while (!(SCU_OSCCON & 0x10)) {
        if (--timeout == 0) break;
    }

    /* Configure System PLL for 300MHz from 20MHz crystal */
    /* PLL formula: fPLL = fOSC * (N + 1) / ((P + 1) * (K2 + 1)) */
    /* For 300MHz: N=29, P=0, K2=1 -> 20 * 30 / (1 * 2) = 300MHz */

    /* PLL Configuration:
     * NDIV = 30 (N=29), PDIV = 1 (P=0), K2DIV = 2 (K2=1)
     */
    SCU_SYSPLLCON0 = (29 << 9) |    /* NDIV */
                     (0 << 24);     /* PDIV */

    SCU_SYSPLLCON1 = (1 << 0);      /* K2DIV = 2 */

    /* Wait for PLL lock */
    timeout = 10000;
    while (!(SCU_SYSPLLCON0 & (1UL << 28))) {
        if (--timeout == 0) break;
    }

    /* Configure CCU to use PLL */
    /* SPB clock = fPLL / 3 = 100MHz */
    SCU_CCUCON0 = (2 << 16) |       /* SPBDIV = 3 */
                  (0 << 0);         /* Clock selection = PLL */

    /* Wait for clock switch */
    timeout = 1000;
    while (SCU_CCUCON0 & (1UL << 28)) {
        if (--timeout == 0) break;
    }
}

/*==============================================================================
 * GETH Module Initialization
 *============================================================================*/

/*
 * geth_module_init - Enable GETH module clock
 */
static void geth_module_init(void)
{
    /* Enable GETH0 module clock */
    GETH0_CLC &= ~GETH_CLC_DISR;    /* Clear disable request */

    /* Wait for module enabled */
    volatile uint32_t timeout = 1000;
    while (GETH0_CLC & GETH_CLC_DISS) {
        if (--timeout == 0) break;
    }
}

/*==============================================================================
 * Pin Mux Configuration
 *============================================================================*/

/* Pin Configuration Values */
#define PORT_IOCR_PC_SHIFT      3
#define PORT_IOCR_PC_INPUT      (0x0 << PORT_IOCR_PC_SHIFT)      /* Input */
#define PORT_IOCR_PC_OUTPUT_PP  (0x8 << PORT_IOCR_PC_SHIFT)      /* Output Push-Pull */
#define PORT_IOCR_PC_ALT1       (0x1 << PORT_IOCR_PC_SHIFT)      /* Alternate 1 */
#define PORT_IOCR_PC_ALT2       (0x2 << PORT_IOCR_PC_SHIFT)      /* Alternate 2 */
#define PORT_IOCR_PC_ALT3       (0x3 << PORT_IOCR_PC_SHIFT)      /* Alternate 3 */
#define PORT_IOCR_PC_ALT4       (0x4 << PORT_IOCR_PC_SHIFT)      /* Alternate 4 */
#define PORT_IOCR_PC_ALT5       (0x5 << PORT_IOCR_PC_SHIFT)      /* Alternate 5 */
#define PORT_IOCR_PC_ALT6       (0x6 << PORT_IOCR_PC_SHIFT)      /* Alternate 6 */
#define PORT_IOCR_PC_ALT7       (0x7 << PORT_IOCR_PC_SHIFT)      /* Alternate 7 */

/*
 * pinmux_ethernet_init - Configure Ethernet pin multiplexing
 * TriBoard TC375/TC397 GETH pin mapping:
 * - P14.0: RMII_REF_CLK (Input)
 * - P14.1: RMII_CRS_DV  (Input)
 * - P14.2: RMII_TX_EN   (Output)
 * - P14.3: RMII_TXD0    (Output)
 * - P14.4: RMII_TXD1    (Output)
 * - P14.5: RMII_RXD0    (Input)
 * - P14.6: RMII_RXD1    (Input)
 * - P14.7: RMII_RX_ER   (Input)
 * - P15.0: MDIO         (In/Out)
 * - P15.1: MDC          (Output)
 */
static void pinmux_ethernet_init(void)
{
    /* Port 14 - RMII Interface */
    /* P14.0: RMII_REF_CLK - Input, Alternate function */
    PORT14_IOCR0 &= ~(0xF8 << 0);
    PORT14_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_REF_CLK */

    /* P14.1: RMII_CRS_DV - Input */
    PORT14_IOCR0 &= ~(0xF8 << 8);
    PORT14_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_CRS_DV */

    /* P14.2: RMII_TX_EN - Output */
    PORT14_IOCR0 &= ~(0xF8 << 16);
    PORT14_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_TX_EN */

    /* P14.3: RMII_TXD0 - Output */
    PORT14_IOCR0 &= ~(0xF8 << 24);
    PORT14_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_TXD0 */

    /* P14.4: RMII_TXD1 - Output */
    PORT14_IOCR4 &= ~(0xF8 << 0);
    PORT14_IOCR4 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_TXD1 */

    /* P14.5: RMII_RXD0 - Input */
    PORT14_IOCR4 &= ~(0xF8 << 8);
    PORT14_IOCR4 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_RXD0 */

    /* P14.6: RMII_RXD1 - Input */
    PORT14_IOCR4 &= ~(0xF8 << 16);
    PORT14_IOCR4 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_RXD1 */

    /* P14.7: RMII_RX_ER - Input */
    PORT14_IOCR4 &= ~(0xF8 << 24);
    PORT14_IOCR4 |= PORT_IOCR_PC_ALT6;  /* GETH0 RMII_RX_ER */

    /* Port 15 - MDIO Interface */
    /* P15.0: MDIO - Bidirectional */
    PORT15_IOCR0 &= ~(0xF8 << 0);
    PORT15_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 MDIO */

    /* P15.1: MDC - Output */
    PORT15_IOCR0 &= ~(0xF8 << 8);
    PORT15_IOCR0 |= PORT_IOCR_PC_ALT6;  /* GETH0 MDC */
}

/*
 * pinmux_led_init - Configure LED pins (TriBoard has LEDs on Port 23)
 */
static void pinmux_led_init(void)
{
    /* P23.0-P23.7: LEDs - Output Push-Pull */
    volatile uint32_t *port23_iocr = (volatile uint32_t *)(PORT23_BASE + PORT_IOCR_OFFSET);

    for (int i = 0; i < 8; i++) {
        *port23_iocr &= ~(0xF8 << (i * 8));
        *port23_iocr |= PORT_IOCR_PC_OUTPUT_PP;
    }
}

/*==============================================================================
 * LED Control
 *============================================================================*/

#define LED_SHIFT_ALL           0x00FF0000UL

/*
 * triboard_led_set - Set LED state
 * led: LED number (0-7)
 * on: true to turn on, false to turn off
 */
void triboard_led_set(uint8_t led, bool on)
{
    if (led > 7) return;

    uint32_t mask = (1UL << led);

    if (on) {
        PORT23_OMR = (mask << 16);  /* Set output */
    } else {
        PORT23_OMR = (mask << 0);   /* Reset output */
    }
}

/*
 * triboard_led_toggle - Toggle LED state
 */
void triboard_led_toggle(uint8_t led)
{
    if (led > 7) return;

    uint32_t mask = (1UL << led);
    PORT23_OMR = (mask << 16) | (mask << 0);  /* Toggle output */
}

/*
 * triboard_led_all_off - Turn off all LEDs
 */
void triboard_led_all_off(void)
{
    PORT23_OMR = ((LED_SHIFT_ALL >> 16) << 0);  /* Reset all LEDs */
}

/*
 * triboard_led_all_on - Turn on all LEDs
 */
void triboard_led_all_on(void)
{
    PORT23_OMR = ((LED_SHIFT_ALL >> 16) << 16); /* Set all LEDs */
}

/*==============================================================================
 * Board Initialization
 *============================================================================*/

/*
 * triboard_init_early - Early initialization (before main)
 * Called from startup code
 */
void triboard_init_early(void)
{
    /* Initialize system clocks */
    system_clock_init();

    /* Enable GETH module */
    geth_module_init();

    /* Configure pin multiplexing */
    pinmux_ethernet_init();
    pinmux_led_init();

    /* Turn on status LED to indicate init started */
    triboard_led_set(0, true);
}

/*
 * triboard_init - Full board initialization
 * Called from main() or application init
 */
void triboard_init(void)
{
    /* Additional initialization can be added here */
    /* - External memory
     * - Peripherals
     * - Interrupts
     * - etc.
     */

    /* Signal init complete */
    triboard_led_all_off();
    triboard_led_set(0, true);
    triboard_led_set(7, true);
}

/*
 * triboard_get_system_clock - Get system clock frequency in Hz
 */
uint32_t triboard_get_system_clock(void)
{
    return F_PLL;
}

/*
 * triboard_get_spb_clock - Get SPB bus clock frequency in Hz
 */
uint32_t triboard_get_spb_clock(void)
{
    return F_SPB;
}

/*==============================================================================
 * Error Handling
 *============================================================================*/

/*
 * triboard_error_handler - Called on fatal errors
 * Shows error pattern on LEDs
 */
void triboard_error_handler(uint32_t error_code)
{
    /* Turn on all LEDs to indicate error */
    triboard_led_all_on();

    /* Flash error code on LEDs */
    while (1) {
        for (int i = 0; i < 8; i++) {
            if (error_code & (1 << i)) {
                triboard_led_toggle(i);
            }
        }

        /* Simple delay */
        for (volatile uint32_t d = 0; d < 1000000; d++);
    }
}

/*==============================================================================
 * Safety Watchdog (for ASIL applications)
 *============================================================================*/

#define SCU_WDT_BASE            (SCU_BASE_ADDR + 0xF0)
#define WDT_CON0                (*((volatile uint32_t *)(SCU_WDT_BASE + 0x00)))
#define WDT_CON1                (*((volatile uint32_t *)(SCU_WDT_BASE + 0x04)))

#define WDT_PASSWORD            0xF0F0F0F0UL

/*
 * triboard_wdt_disable - Disable safety watchdog (debug only)
 */
void triboard_wdt_disable(void)
{
    /* Write password to access */
    WDT_CON0 = WDT_PASSWORD;

    /* Disable watchdog */
    WDT_CON1 &= ~(1UL << 0);
}

/*
 * triboard_wdt_service - Service watchdog
 */
void triboard_wdt_service(void)
{
    /* Service sequence required by TC3xx */
    WDT_CON0 = WDT_PASSWORD | 0x01;
    WDT_CON0 = WDT_PASSWORD | 0x00;
}
