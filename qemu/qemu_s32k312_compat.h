/*=============================================================================
 * qemu_s32k312_compat.h -- QEMU MPS2 AN500 to S32K312 compatibility shim
 *
 * When building for QEMU MPS2 AN500 (CMSDK APB UART at 0x40004000 instead of
 * LPUART0 at 0x40180000), this header redirects LPUART0 register accesses to
 * the CMSDK UART hardware.
 *
 * The LPUART register API (LPUART0_DATA, LPUART0_STAT, etc.) is preserved.
 * At compile time, these macros redirect to the CMSDK UART registers.
 *
 * For real S32K312 hardware: exclude this shim; LPUART0 is at 0x40180000.
 *============================================================================*/

#ifndef QEMU_S32K312_COMPAT_H
#define QEMU_S32K312_COMPAT_H

/*=============================================================================
 * CMSDK APB UART (QEMU MPS2 AN500 UART0 at 0x40004000)
 *
 * Registers (32-bit aligned, 4-byte stride):
 *   0x000 DATA     - Transmit/Receive data
 *   0x004 STATE    - Status (bit0=txfull, bit1=rxfull, bit2=txbusy)
 *   0x008 CTRL     - Control (bit0=txen, bit1=rxen)
 *   0x00C INTSTATUS- Interrupt status
 *   0x010 BAUDDIV  - Baud rate divider
 *============================================================================*/
#define CMSDK_UART_BASE     0x40004000UL

#define CMSDK_DATA          (*(volatile unsigned long *)(CMSDK_UART_BASE + 0x000))
#define CMSDK_STATE         (*(volatile unsigned long *)(CMSDK_UART_BASE + 0x004))
#define CMSDK_CTRL          (*(volatile unsigned long *)(CMSDK_UART_BASE + 0x008))
#define CMSDK_BAUDDIV       (*(volatile unsigned long *)(CMSDK_UART_BASE + 0x010))

#define CMSDK_STATE_TXFULL  (1U << 0)
#define CMSDK_STATE_RXFULL  (1U << 1)
#define CMSDK_CTRL_TXEN     (1U << 0)
#define CMSDK_CTRL_RXEN     (1U << 1)

/*=============================================================================
 * LPUART0 registers -> CMSDK remap
 *
 * On MPS2 AN500, there is no LPUART0 at 0x40180000.
 * We redirect all LPUART0 register accesses to the CMSDK UART.
 *
 * LPUART0_DATA  = CMSDK_DATA      (write char to DATA register)
 * LPUART0_STAT  = CMSDK_STATE     (check TX/RX status flags)
 * LPUART0_CTRL  = CMSDK_CTRL      (enable TX/RX)
 * LPUART0_BAUD  = CMSDK_BAUDDIV   (baud rate divider)
 * LPUART0_GLOBAL= reserved address (no-op, reads back 0)
 * LPUART0_FIFO  = reserved address (no-op FIFO control)
 *============================================================================*/

/* These define/redefine must happen AFTER S32K312.h includes (done in main_qemu.c)
 * but BEFORE any LPUART0 register usage. */
#define LPUART0_DATA    CMSDK_DATA
#define LPUART0_STAT    CMSDK_STATE
#define LPUART0_CTRL    CMSDK_CTRL
#define LPUART0_BAUD    CMSDK_BAUDDIV
#define LPUART0_GLOBAL  CMSDK_BAUDDIV  /* No-op, maps to dummy reg */
#define LPUART0_FIFO    CMSDK_BAUDDIV  /* No-op, maps to dummy reg */

/*=============================================================================
 * Helper macros for UART status checking
 *============================================================================*/
/* LPUART TDRE equivalent: CMSDK TX is ready when TXFULL is clear */
#define lpuart_tx_ready()   ((CMSDK_STATE & CMSDK_STATE_TXFULL) == 0)
#define lpuart_rx_ready()   ((CMSDK_STATE & CMSDK_STATE_RXFULL) != 0)

#endif /* QEMU_S32K312_COMPAT_H */
