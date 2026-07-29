/*=============================================================================
 * S32K312.h -- S32K312 Register Map (Single Source of Truth)
 *
 * This header defines the complete memory map and register offsets for the
 * NXP S32K312 microcontroller (ARM Cortex-M7).
 *
 * All MCAL modules and QEMU simulation must include this header and use
 * these definitions as the single source of truth for hardware addresses.
 *
 * Reference: NXP S32K312 Reference Manual (Rev 8+)
 *============================================================================*/

#ifndef S32K312_H
#define S32K312_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Memory Map Overview
 *
 * Flash (alias):   0x00000000 - 0x003FFFFF  (4 MB)
 * Flash (primary): 0x00400000 - 0x007FFFFF  (4 MB)
 * SRAM_L:          0x1FFF0000 - 0x1FFFFFFF  (64 KB)
 * SRAM_U:          0x20000000 - 0x2007FFFF  (512 KB)
 * AIPS0:           0x40000000 - 0x4007FFFF  (512 KB)
 * AIPS1:           0x40080000 - 0x400FFFFF  (512 KB)
 * AIPS2:           0x40100000 - 0x4017FFFF  (512 KB)
 * AIPS3:           0x40180000 - 0x401FFFFF  (512 KB)
 * AIPS4:           0x40200000 - 0x4027FFFF  (512 KB)
 * AIPS5:           0x40280000 - 0x402FFFFF  (512 KB)
 * GPIO:            0x40810000 - 0x4081FFFF  (64 KB)
 * HSM:             0x40460000 - 0x4046FFFF  (64 KB)
 *============================================================================*/

/*-----------------------------------------------------------------------------
 * Memory Regions
 *----------------------------------------------------------------------------*/
#define S32K312_FLASH_BASE_ALIAS        0x00000000UL
#define S32K312_FLASH_BASE              0x00400000UL
#define S32K312_FLASH_SIZE              0x00400000UL   /* 4 MB */

#define S32K312_SRAM_L_BASE             0x1FFF0000UL
#define S32K312_SRAM_L_SIZE             0x00010000UL   /* 64 KB */

#define S32K312_SRAM_U_BASE             0x20000000UL
#define S32K312_SRAM_U_SIZE             0x00080000UL   /* 512 KB */

#define S32K312_SRAM_BASE               S32K312_SRAM_U_BASE
#define S32K312_SRAM_SIZE               S32K312_SRAM_U_SIZE

/*-----------------------------------------------------------------------------
 * AIPS Bridge Addresses (IP Bus Bridge)
 *----------------------------------------------------------------------------*/
#define S32K312_AIPS0_BASE              0x40000000UL
#define S32K312_AIPS1_BASE              0x40080000UL
#define S32K312_AIPS2_BASE              0x40100000UL
#define S32K312_AIPS3_BASE              0x40180000UL
#define S32K312_AIPS4_BASE              0x40200000UL
#define S32K312_AIPS5_BASE              0x40280000UL

/*-----------------------------------------------------------------------------
 * Peripheral Base Addresses
 *----------------------------------------------------------------------------*/

/* System / Core */
#define S32K312_MSCM_BASE               0x4001F000UL   /* System Control Module */
#define S32K312_STM0_BASE               0x40120000UL   /* System Timer 0 */
#define S32K312_SWT_BASE                0x40130000UL   /* Software Watchdog Timer */
#define S32K312_CSE_BASE                0x40140000UL   /* Crypto Services Engine */
#define S32K312_DMA_BASE                0x40058000UL   /* eDMA Controller */
#define S32K312_DMA_MUX_BASE            0x40059000UL   /* eDMA Channel Mux */

/* MCU / Clock / Reset */
#define S32K312_MCU_BASE                0x402A0000UL   /* MCU (SCG + SIM + ME) */
#define S32K312_CMU_BASE                0x402B0000UL   /* Clock Monitor Unit */
#define S32K312_FCCU_BASE               0x402C0000UL   /* Fault Collection Unit */

/* Serial Communication */
#define S32K312_LPUART0_BASE            0x40180000UL   /* LPUART 0 */
#define S32K312_LPUART1_BASE            0x40182000UL   /* LPUART 1 */
#define S32K312_LPUART2_BASE            0x40184000UL   /* LPUART 2 */
#define S32K312_LPUART3_BASE            0x40186000UL   /* LPUART 3 */
#define S32K312_LPUART4_BASE            0x40188000UL   /* LPUART 4 */
#define S32K312_LPUART5_BASE            0x4018A000UL   /* LPUART 5 */

#define S32K312_LPSPI0_BASE             0x40038000UL   /* LPSPI 0 */
#define S32K312_LPSPI1_BASE             0x4003A000UL   /* LPSPI 1 */
#define S32K312_LPSPI2_BASE             0x4003C000UL   /* LPSPI 2 */

#define S32K312_LPI2C0_BASE             0x40040000UL   /* LPI2C 0 */
#define S32K312_LPI2C1_BASE             0x40042000UL   /* LPI2C 1 */

/* CAN */
#define S32K312_CAN0_BASE               0x40050000UL   /* FlexCAN 0 */
#define S32K312_CAN1_BASE               0x40052000UL   /* FlexCAN 1 */
#define S32K312_CAN2_BASE               0x40054000UL   /* FlexCAN 2 */

/* Watchdog */
#define S32K312_WDOG_BASE               0x40053000UL   /* Watchdog Timer */

/* ADC */
#define S32K312_ADC0_BASE               0x400C0000UL   /* ADC 0 */
#define S32K312_ADC1_BASE               0x400C1000UL   /* ADC 1 */

/* Timer / PWM */
#define S32K312_FTM0_BASE               0x400D0000UL   /* FlexTimer 0 */
#define S32K312_FTM1_BASE               0x400D2000UL   /* FlexTimer 1 */
#define S32K312_FTM2_BASE               0x400D4000UL   /* FlexTimer 2 */
#define S32K312_FTM3_BASE               0x400D6000UL   /* FlexTimer 3 */

#define S32K312_PIT_BASE                0x400E0000UL   /* Periodic Interrupt Timer */

/* I2S / SAI */
#define S32K312_SAI0_BASE               0x400F0000UL   /* SAI 0 */
#define S32K312_SAI1_BASE               0x400F2000UL   /* SAI 1 */

/* Port / GPIO */
#define S32K312_SIUL2_BASE              0x40290000UL   /* SIUL2 (Port control) */
#define S32K312_SIUL2_GPIO_BASE         0x40810000UL   /* SIUL2 GPIO */
#define S32K312_SIUL2_MID_BASE          0x40280000UL   /* SIUL2 MID (Mux) */

/* Flash Controller */
#define S32K312_FLASH_CTRL_BASE         0x40020000UL   /* Flash Controller (PFlash) */
#define S32K312_FLASH_DCTRL_BASE        0x40022000UL   /* Flash Data Controller (DFlash) */
#define S32K312_FLEXSPI_BASE            0x400A0000UL   /* FlexSPI (external flash) */

/* ECC / Safety */
#define S32K312_ECC_SRAM_BASE           0x40024000UL   /* SRAM ECC Controller */
#define S32K312_ECC_FLASH_BASE          0x40026000UL   /* Flash ECC Controller */

/* HSM (Hardware Security Module) */
#define S32K312_HSM_BASE                0x40460000UL
#define S32K312_HSM_AES_BASE            0x40461000UL
#define S32K312_HSM_ECC_BASE            0x40462000UL
#define S32K312_HSM_SHA_BASE            0x40463000UL
#define S32K312_HSM_TRNG_BASE           0x40464000UL
#define S32K312_HSM_KEYSTORE_BASE       0x40465000UL

/* SPI Compatibility (DSPI emulation via LPSPI) */
#define S32K312_SPI0_BASE               S32K312_LPSPI0_BASE
#define S32K312_SPI1_BASE               S32K312_LPSPI1_BASE

/*=============================================================================
 * Peripheral Register Offsets
 *============================================================================*/

/*-----------------------------------------------------------------------------
 * LPUART (0x4018_0000 base, 0x20 stride)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x00   | VERID           | Version ID
 * 0x04   | PARAM           | Parameter
 * 0x08   | GLOBAL          | Global
 * 0x0C   | PINCFG          | Pin Configuration
 * 0x10   | BAUD            | Baud Rate
 * 0x14   | STAT            | Status
 * 0x18   | CTRL            | Control
 * 0x1C   | DATA            | Data
 * 0x20   | MATCH           | Match
 * 0x24   | MODIR           | Modem IrDA
 * 0x28   | FIFO            | FIFO
 * 0x2C   | WATER           | Watermark
 *----------------------------------------------------------------------------*/
#define LPUART_VERID_OFF                0x00U
#define LPUART_PARAM_OFF                0x04U
#define LPUART_GLOBAL_OFF               0x08U
#define LPUART_PINCFG_OFF               0x0CU
#define LPUART_BAUD_OFF                 0x10U
#define LPUART_STAT_OFF                 0x14U
#define LPUART_CTRL_OFF                 0x18U
#define LPUART_DATA_OFF                 0x1CU
#define LPUART_MATCH_OFF                0x20U
#define LPUART_MODIR_OFF                0x24U
#define LPUART_FIFO_OFF                 0x28U
#define LPUART_WATER_OFF                0x2CU

/* LPUART STAT register bits */
#define LPUART_STAT_MSB_MASK            (0x0001U)   /* MSB first */
#define LPUART_STAT_RXEDGIF             (0x0002U)   /* RX edge */
#define LPUART_STAT_IDLE                (0x0010U)   /* Idle line */
#define LPUART_STAT_OR                  (0x0020U)   /* Overrun */
#define LPUART_STAT_NF                  (0x0040U)   /* Noise */
#define LPUART_STAT_FE                  (0x0080U)   /* Framing error */
#define LPUART_STAT_PF                  (0x0100U)   /* Parity error */
#define LPUART_STAT_MA1F                (0x0200U)   /* Match 1 flag */
#define LPUART_STAT_MA2F                (0x0400U)   /* Match 2 flag */
#define LPUART_STAT_RXINV               (0x1000U)   /* RX invert */
#define LPUART_STAT_RXINV_SHIFT         (12U)
#define LPUART_STAT_RXINV_WIDTH         (1U)
#define LPUART_STAT_TDRE                (0x00800000UL)  /* TX data register empty */
#define LPUART_STAT_TC                  (0x01000000UL)  /* TX complete */
#define LPUART_STAT_RDRF                (0x02000000UL)  /* RX data register full */
#define LPUART_STAT_RAF                 (0x04000000UL)  /* Receiver active flag */

/* LPUART CTRL register bits */
#define LPUART_CTRL_SBK                 (0x0001U)   /* Send break */
#define LPUART_CTRL_RWU                 (0x0002U)   /* Receiver wakeup */
#define LPUART_CTRL_RE                  (0x0004U)   /* Receiver enable */
#define LPUART_CTRL_TE                  (0x0008U)   /* Transmitter enable */
#define LPUART_CTRL_ILIE                (0x0010U)   /* Idle line interrupt */
#define LPUART_CTRL_RIE                 (0x0020U)   /* RX interrupt */
#define LPUART_CTRL_TCIE                (0x0040U)   /* TX complete interrupt */
#define LPUART_CTRL_TIE                 (0x0080U)   /* TX interrupt */
#define LPUART_CTRL_PE                  (0x0100U)   /* Parity enable */
#define LPUART_CTRL_PM                  (0x0200U)   /* Parity mode */
#define LPUART_CTRL_ESCF                (0x0400U)   /* Escape character */
#define LPUART_CTRL_M                   (0x0800U)   /* 9-bit mode */
#define LPUART_CTRL_WAKE                (0x1000U)   /* Wakeup method */
#define LPUART_CTRL_SRC                 (0x2000U)   /* 16-bit baud vs 13-bit */
#define LPUART_CTRL_MA1IE               (0x4000U)   /* Match 1 IE */
#define LPUART_CTRL_MA2IE               (0x8000U)   /* Match 2 IE */
#define LPUART_CTRL_FRZ                 (0x00010000UL)  /* Freeze */
#define LPUART_CTRL_ORIE                (0x04000000UL)  /* Overrun IE */
#define LPUART_CTRL_NEIE                (0x08000000UL)  /* Noise error IE */
#define LPUART_CTRL_FEIE                (0x10000000UL)  /* Framing error IE */
#define LPUART_CTRL_PEIE                (0x20000000UL)  /* Parity error IE */
#define LPUART_CTRL_R8T9                (0x40000000UL)  /* 9th data bit */

/* LPUART BAUD register */
#define LPUART_BAUD_SBR_MASK            (0x00001FFFU)   /* Baud rate divisor */
#define LPUART_BAUD_SBNS                (0x00002000U)   /* Stop bit number select */
#define LPUART_BAUD_RXEDGIE             (0x00004000U)   /* RX edge IE */
#define LPUART_BAUD_LBKDIE              (0x00008000U)   /* LIN break detect IE */
#define LPUART_BAUD_RESYNCDIS           (0x00010000U)   /* Resync disable */
#define LPUART_BAUD_BOTHEDGE            (0x00020000U)   /* Both edge sampling */
#define LPUART_BAUD_MATCFG_SHIFT        (18U)
#define LPUART_BAUD_MATCFG_MASK         (0x000C0000U)
#define LPUART_BAUD_TDMAE               (0x00800000UL)  /* TX DMA enable */
#define LPUART_BAUD_RDMAE               (0x01000000UL)  /* RX DMA enable */
#define LPUART_BAUD_OSR_SHIFT           (24U)
#define LPUART_BAUD_OSR_MASK            (0x1F000000UL)  /* Oversampling ratio */
#define LPUART_BAUD_BOTHEDGE_OSR16      (0x02020000U)   /* Both edge + OSR=16 */
#define LPUART_BAUD_DEFAULT_OSR         (16U)

/* LPUART DATA register */
#define LPUART_DATA_MASK                (0x000001FFU)   /* Data bits (0-8) */
#define LPUART_DATA_NOISY               (0x00000400U)   /* Noisy bit */
#define LPUART_DATA_PARITYE             (0x00000800U)   /* Parity error */
#define LPUART_DATA_FRETSC              (0x00001000U)   /* Frame error / TX complete */
#define LPUART_DATA_RXEMPT              (0x00002000U)   /* RX buffer empty */

/* LPUART GLOBAL register */
#define LPUART_GLOBAL_RST               (0x00000001U)   /* Software reset */

/* LPUART FIFO register */
#define LPUART_FIFO_TXEMPT              (0x00000001U)   /* TX FIFO empty */
#define LPUART_FIFO_TXOF                (0x00000002U)   /* TX FIFO overflow */
#define LPUART_FIFO_RXUF                (0x00000004U)   /* RX FIFO underflow */
#define LPUART_FIFO_TXFLUSH             (0x00000040U)   /* TX FIFO flush */
#define LPUART_FIFO_RXFLUSH             (0x00000080U)   /* RX FIFO flush */
#define LPUART_FIFO_TXFIFOSIZE_SHIFT    (4U)
#define LPUART_FIFO_TXFIFOSIZE_MASK     (0x000000F0U)
#define LPUART_FIFO_RXFIFOSIZE_SHIFT    (8U)
#define LPUART_FIFO_RXFIFOSIZE_MASK     (0x00000F00U)
#define LPUART_FIFO_TXFE_SHIFT          (12U)
#define LPUART_FIFO_TXFE_MASK           (0x00001000U)
#define LPUART_FIFO_DMAEN               (0x00010000UL)  /* DMA Enable */

/*-----------------------------------------------------------------------------
 * SIUL2 (Port / Pad Control) -- 0x4029_0000
 *
 * Offset | Register      | Description
 * -------+---------------+--------------------------------
 * 0x20   | MSCR[0-271]   | Pad Mux Control (272 × 4 bytes)
 * 0x460  | IMCR[0-511]   | Input Mux Control
 * 0x4000 | PGPDOn        | Pad GPIO Data Out n
 * 0x5000 | PGPDIn         | Pad GPIO Data In
 * 0x5020 | PGPDOEn        | Pad GPIO Data Out Enable
 * 0x6C00 | PCR[0-271]    | Pad Configuration (272 × 4 bytes) — alias of MSCR
 * 0x6000 | GPDOnnnnn     | GPIO Pad Data Out registers
 *
 * SIUL2 GPIO Registers at 0x4081_0000 (regular GPIO):
 * Offset | Register      | Description
 * -------+---------------+--------------------------------
 * 0x000  | PDOR           | Port Data Out
 * 0x004  | PSOR           | Port Set Out
 * 0x008  | PCOR           | Port Clear Out
 * 0x00C  | PTOR           | Port Toggle Out
 * 0x010  | PDIR           | Port Data In
 * 0x014  | PDDR           | Port Data Direction
 * 0x018  | PIDR           | Port Input Disable
 *
 * Each of the 32 GPIO groups has its own block (0x1000 stride).
 *----------------------------------------------------------------------------*/
#define SIUL2_MSCR0_OFF                 (0x0020U)
#define SIUL2_MSCR_SIZE                 (4U)            /* 4 bytes per MSCR */
#define SIUL2_NUM_MSCR                  (272U)

#define SIUL2_IMCR0_OFF                 (0x0460U)
#define SIUL2_IMCR_SIZE                 (4U)            /* 4 bytes per IMCR */
#define SIUL2_NUM_IMCR                  (512U)

#define SIUL2_PGPDO0_OFF                (0x4000U)       /* Pad GPIO Data Out 0 */
#define SIUL2_PGPDI0_OFF                (0x5000U)       /* Pad GPIO Data In 0 */
#define SIUL2_PGPDOE0_OFF               (0x5020U)       /* Pad Data Out Enable 0 */

/* SIUL2 MSCR fields */
#define SIUL2_MSCR_SMC_SHIFT            (0U)
#define SIUL2_MSCR_SMC_MASK             (0x00000007U)
#define SIUL2_MSCR_APC_SHIFT            (3U)
#define SIUL2_MSCR_APC_MASK             (0x00000018U)
#define SIUL2_MSCR_IBE                 (0x00000020U)    /* Input buffer enable */
#define SIUL2_MSCR_OBE                 (0x00000040U)    /* Output buffer enable */
#define SIUL2_MSCR_ODE                 (0x00000080U)    /* Open drain enable */
#define SIUL2_MSCR_SRC_SHIFT            (8U)
#define SIUL2_MSCR_SRC_MASK             (0x00000300U)    /* Slew rate control */
#define SIUL2_MSCR_PU                  (0x00000400U)    /* Pull up */
#define SIUL2_MSCR_PD                  (0x00000800U)    /* Pull down */
#define SIUL2_MSCR_PKE                 (0x00001000U)    /* Pull keep enable */
#define SIUL2_MSCR_PFE                 (0x00002000U)    /* Passive filter */
#define SIUL2_MSCR_SSS_SHIFT            (24U)
#define SIUL2_MSCR_SSS_MASK             (0x0F000000U)    /* Pad mux select */
#define SIUL2_MSCR_IBE_EN              (0x00000020U)
#define SIUL2_MSCR_OBE_EN              (0x00000040U)
#define SIUL2_MSCR_PU_EN               (0x00000400U)
#define SIUL2_MSCR_ALT0                (0x00000000U)    /* GPIO */
#define SIUL2_MSCR_ALT1                (0x01000000U)    /* ALT1 */
#define SIUL2_MSCR_ALT2                (0x02000000U)    /* ALT2 */
#define SIUL2_MSCR_ALT3                (0x03000000U)    /* ALT3 */
#define SIUL2_MSCR_ALT4                (0x04000000U)    /* ALT4 */
#define SIUL2_MSCR_ALT5                (0x05000000U)    /* ALT5 */
#define SIUL2_MSCR_ALT6                (0x06000000U)    /* ALT6 */
#define SIUL2_MSCR_ALT7                (0x07000000U)    /* ALT7 */

/* GPIO register offsets (per port group, 0x1000 stride) */
#define SIUL2_GPIO_PDOR_OFF             (0x0000U)       /* Port Data Output */
#define SIUL2_GPIO_PSOR_OFF             (0x0004U)       /* Port Set Output */
#define SIUL2_GPIO_PCOR_OFF             (0x0008U)       /* Port Clear Output */
#define SIUL2_GPIO_PTOR_OFF             (0x000CU)       /* Port Toggle Output */
#define SIUL2_GPIO_PDIR_OFF             (0x0010U)       /* Port Data Input */
#define SIUL2_GPIO_PDDR_OFF             (0x0014U)       /* Port Data Direction */
#define SIUL2_GPIO_PIDR_OFF             (0x0018U)       /* Port Input Disable */

/* GPIO Port Group stride (0x1000 = 4 KB per port group) */
#define SIUL2_GPIO_GROUP_STRIDE         (0x1000U)

/* GPIO port index macros */
#define SIUL2_GPIO_PORT(port)           (S32K312_SIUL2_GPIO_BASE + (port) * SIUL2_GPIO_GROUP_STRIDE)
#define SIUL2_GPIO_PDOR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PDOR_OFF)
#define SIUL2_GPIO_PSOR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PSOR_OFF)
#define SIUL2_GPIO_PCOR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PCOR_OFF)
#define SIUL2_GPIO_PTOR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PTOR_OFF)
#define SIUL2_GPIO_PDIR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PDIR_OFF)
#define SIUL2_GPIO_PDDR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PDDR_OFF)
#define SIUL2_GPIO_PIDR(port)           (SIUL2_GPIO_PORT(port) + SIUL2_GPIO_PIDR_OFF)

/*-----------------------------------------------------------------------------
 * FlexCAN (0x4005_0000 base, 0x2000 stride)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x000  | MCR             | Module Configuration
 * 0x004  | CTRL1           | Control 1
 * 0x008  | TIMER           | Free Running Timer
 * 0x00C  | RXMGMASK        | RX Mailboxes Global Mask
 * 0x010  | RX14MASK        | RX 14 Mask
 * 0x014  | RX15MASK        | RX 15 Mask
 * 0x018  | ECR             | Error Counter
 * 0x01C  | ESR1            | Error Status 1
 * 0x020  | IMASK1          | Interrupt Mask 1
 * 0x024  | IFREG1          | Interrupt Flag 1
 * 0x028  | CTRL2           | Control 2
 * 0x02C  | ESR2            | Error Status 2
 * 0x030-0x07F              | Reserved
 * 0x080  | CTRL2_EXT       | Control 2 Extended (S32K specific)
 * 0x084  | RXMGMASK_EXT    | RX Global Mask Extended
 * 0x088-0x0BF              | Reserved
 * 0x0C0  | MB0-15          | Message Buffers (16 × 16 bytes)
 * 0x1C0  | RXIMR0-15       | RX Individual Mask (16 × 4 bytes)
 *----------------------------------------------------------------------------*/
#define FLEXCAN_MCR_OFF                 (0x000U)
#define FLEXCAN_CTRL1_OFF               (0x004U)
#define FLEXCAN_TIMER_OFF               (0x008U)
#define FLEXCAN_RXMGMASK_OFF            (0x00CU)
#define FLEXCAN_RX14MASK_OFF            (0x010U)
#define FLEXCAN_RX15MASK_OFF            (0x014U)
#define FLEXCAN_ECR_OFF                 (0x018U)
#define FLEXCAN_ESR1_OFF                (0x01CU)
#define FLEXCAN_IMASK1_OFF              (0x020U)
#define FLEXCAN_IFREG1_OFF              (0x024U)
#define FLEXCAN_CTRL2_OFF               (0x028U)
#define FLEXCAN_ESR2_OFF                (0x02CU)
#define FLEXCAN_CTRL2_EXT_OFF           (0x080U)
#define FLEXCAN_RXMGMASK_EXT_OFF        (0x084U)
#define FLEXCAN_MB0_OFF                 (0x0C0U)
#define FLEXCAN_MB_SIZE                 (16U)         /* 16 bytes per MB */
#define FLEXCAN_NUM_MB                  (16U)
#define FLEXCAN_RXIMR0_OFF              (0x1C0U)
#define FLEXCAN_RXIMR_SIZE              (4U)

/* FlexCAN MCR bits */
#define FLEXCAN_MCR_MDIS               (0x00000001U)   /* Module disable */
#define FLEXCAN_MCR_FRZ                (0x00000002U)   /* Freeze enable */
#define FLEXCAN_MCR_FEN                (0x00000004U)   /* Filter enable */
#define FLEXCAN_MCR_HALT               (0x00000008U)   /* Halt */
#define FLEXCAN_MCR_NOTRDY             (0x00000010U)   /* Not ready */
#define FLEXCAN_MCR_WAKMSK             (0x00000020U)   /* Wakeup mask */
#define FLEXCAN_MCR_SOFTRST            (0x00000040U)   /* Soft reset */
#define FLEXCAN_MCR_FRZACK             (0x00000080U)   /* Freeze ack */
#define FLEXCAN_MCR_SUPV               (0x00000100U)   /* Supervisor mode */
#define FLEXCAN_MCR_SLF_WAK            (0x00000200U)   /* Self wake */
#define FLEXCAN_MCR_WRNEN              (0x00000400U)   /* Warning interrupt */
#define FLEXCAN_MCR_LPMACK             (0x00000800U)   /* Low-power ack */
#define FLEXCAN_MCR_WAKSRC             (0x00001000U)   /* Wakeup source */
#define FLEXCAN_MCR_DMA                (0x00002000U)   /* DMA enable */
#define FLEXCAN_MCR_LPRIO_EN           (0x00008000U)   /* Local priority */
#define FLEXCAN_MCR_PNET_EN            (0x00010000U)   /* Pretended networking */
#define FLEXCAN_MCR_ACK_ERR            (0x00020000U)   /* Halt ack error */

/* FlexCAN CTRL1 bits */
#define FLEXCAN_CTRL1_PROPSEG_SHIFT    (0U)
#define FLEXCAN_CTRL1_LOM              (0x00000400U)   /* Listen-only mode */
#define FLEXCAN_CTRL1_LBUF             (0x00000800U)   /* Lowest buffer */
#define FLEXCAN_CTRL1_TSYN             (0x00001000U)   /* Timer sync */
#define FLEXCAN_CTRL1_BOFFREC          (0x00002000U)   /* Bus-off recovery */
#define FLEXCAN_CTRL1_SMP              (0x00004000U)   /* Sampling */
#define FLEXCAN_CTRL1_RWRNMSK          (0x00008000U)   /* RX warning mask */
#define FLEXCAN_CTRL1_TWRNMSK          (0x00010000U)   /* TX warning mask */
#define FLEXCAN_CTRL1_LPB              (0x00020000U)   /* Loopback */

/*-----------------------------------------------------------------------------
 * WDOG (0x4005_3000)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x00   | CS              | Control and Status
 * 0x04   | CNT             | Counter
 * 0x08   | TOVAL           | Timeout Value
 * 0x0C   | WIN             | Window
 *----------------------------------------------------------------------------*/
#define WDOG_CS_OFF                     (0x00U)
#define WDOG_CNT_OFF                    (0x04U)
#define WDOG_TOVAL_OFF                  (0x08U)
#define WDOG_WIN_OFF                    (0x0CU)

#define WDOG_CS_STOP                   (0x00000001U)
#define WDOG_CS_WAIT                   (0x00000002U)
#define WDOG_CS_DBG                    (0x00000004U)
#define WDOG_CS_TST_MASK               (0x00000018U)
#define WDOG_CS_UPDATE                 (0x00000020U)
#define WDOG_CS_INT                    (0x00000040U)
#define WDOG_CS_EN                     (0x00000080U)
#define WDOG_CS_CLK_MASK               (0x00000300U)
#define WDOG_CS_CLK_SHIFT              (8U)
#define WDOG_CS_PRES                   (0x00000400U)
#define WDOG_CS_ULK                    (0x00000800U)
#define WDOG_CS_RCS                    (0x00001000U)

#define WDOG_UNLOCK_SEQ1                (0xA602U)
#define WDOG_UNLOCK_SEQ2                (0xB480U)
#define WDOG_REFRESH_SEQ1               (0xA602U)
#define WDOG_REFRESH_SEQ2               (0xB480U)

/*-----------------------------------------------------------------------------
 * ADC (0x400C_0000 base, 0x1000 stride)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x00   | SC1_B           | Status & Control 1 (B-side)
 * 0x04   | SC1_A           | Status & Control 1 (A-side)
 * 0x08   | CFG1            | Configuration 1
 * 0x0C   | CFG2            | Configuration 2
 * 0x10   | R_B             | Data Result (B)
 * 0x14   | R_A             | Data Result (A)
 * 0x18   | CV1             | Compare Value 1
 * 0x1C   | CV2             | Compare Value 2
 * 0x20   | SC2             | Status & Control 2
 * 0x24   | SC3             | Status & Control 3
 * 0x28   | OFS_A           | Offset Correction (A)
 * 0x2C   | PG_A            | Plus-Side Gain (A)
 * 0x30   | MG_A            | Minus-Side Gain (A)
 * 0x34   | OFS_B           | Offset Correction (B)
 * 0x38   | PG_B            | Plus-Side Gain (B)
 * 0x3C   | MG_B            | Minus-Side Gain (B)
 * 0x40   | CAL             | Calibration
 * 0x44   | DST             | DMA Status
 * 0x48   | DP               | DMA Pending
 *----------------------------------------------------------------------------*/
#define ADC_SC1_B_OFF                   (0x00U)
#define ADC_SC1_A_OFF                   (0x04U)
#define ADC_CFG1_OFF                    (0x08U)
#define ADC_CFG2_OFF                    (0x0CU)
#define ADC_R_B_OFF                     (0x10U)
#define ADC_R_A_OFF                     (0x14U)
#define ADC_CV1_OFF                     (0x18U)
#define ADC_CV2_OFF                     (0x1CU)
#define ADC_SC2_OFF                     (0x20U)
#define ADC_SC3_OFF                     (0x24U)
#define ADC_OFS_A_OFF                   (0x28U)
#define ADC_PG_A_OFF                    (0x2CU)
#define ADC_MG_A_OFF                    (0x30U)
#define ADC_OFS_B_OFF                   (0x34U)
#define ADC_PG_B_OFF                    (0x38U)
#define ADC_MG_B_OFF                    (0x3CU)
#define ADC_CAL_OFF                     (0x40U)
#define ADC_DST_OFF                     (0x44U)
#define ADC_DP_OFF                      (0x48U)

/*-----------------------------------------------------------------------------
 * PIT (0x400E_0000)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x000  | PITMCR          | Module Control
 * 0x004  | Reserved        |
 * 0x008  | Reserved        |
 * 0x00C  | Reserved        |
 * 0x100  | LDVAL0          | Timer Load Value 0
 * 0x104  | CVAL0           | Current Value 0
 * 0x108  | TCTRL0          | Timer Control 0
 * 0x10C  | TFLG0           | Timer Flag 0
 * 0x110  | LDVAL1          | Timer Load Value 1
 * ...    | (0x10 stride)   |
 *----------------------------------------------------------------------------*/
#define PIT_MCR_OFF                     (0x000U)
#define PIT_LDVAL_OFF(ch)              (0x100U + (ch) * 0x10U)
#define PIT_CVAL_OFF(ch)               (0x104U + (ch) * 0x10U)
#define PIT_TCTRL_OFF(ch)              (0x108U + (ch) * 0x10U)
#define PIT_TFLG_OFF(ch)               (0x10CU + (ch) * 0x10U)

#define PIT_MCR_MDIS                   (0x00000001U)   /* Module disable */
#define PIT_MCR_FRZ                    (0x00000002U)   /* Freeze */
#define PIT_TCTRL_TEN                  (0x00000001U)   /* Timer enable */
#define PIT_TCTRL_TIE                  (0x00000002U)   /* Timer interrupt */
#define PIT_TCTRL_CHN                  (0x00000004U)   /* Chain mode */
#define PIT_TFLG_TIF                   (0x00000001U)   /* Timer interrupt flag */

/*-----------------------------------------------------------------------------
 * FTM (0x400D_0000 base, 0x2000 stride)
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x000  | SC              | Status and Control
 * 0x004  | CNT             | Counter
 * 0x008  | MOD             | Modulus
 * 0x00C  | CNTIN           | Counter Initial Value
 * 0x010  | STATUS          | Capture/Compare Status
 * 0x014  | STATUS_EXT      | Status Extension
 * 0x018  | MODE            | Features Mode Selection
 * 0x01C  | SYNC            | Synchronization
 * 0x020  | OUTINIT         | Initial Output Value
 * 0x024  | OUTMASK         | Output Mask
 * 0x028  | COMBINE         | Function for Combined PWM
 * 0x02C  | COMBINE_EXT     | Combine Extended
 * 0x030  | DEADTIME        | Dead-Time Insertion
 * 0x034  | EXTTRIG         | External Trigger
 * 0x038  | POL             | Polarity
 * 0x03C  | FMS             | Fault Mode Status
 * 0x040  | FILTER          | Input Filter
 * 0x044  | FLTCTRL         | Fault Control
 * 0x048  | QDCTRL          | Quad Decoder Control
 * 0x04C  | CONF            | Configuration
 * 0x050  | FLTPOL          | Fault Input Polarity
 * 0x054  | SYNCONF         | Synchronization Configuration
 * 0x058  | INVCTRL         | Inverting Control
 * 0x05C  | SWOCTRL         | Software Output Control
 * 0x060  | PWMLOAD         | PWM Reload
 * 0x070-0x07F              | Reserved
 * 0x080-0x09F C0V-C7V     | Channel Value (8 × 4 bytes)
 * 0x100  | PAIRMASK        | Pair Mask
 * 0x1E0  | MCTRL           | Module Control
 *----------------------------------------------------------------------------*/
#define FTM_SC_OFF                      (0x000U)
#define FTM_CNT_OFF                     (0x004U)
#define FTM_MOD_OFF                     (0x008U)
#define FTM_CNTIN_OFF                   (0x00CU)
#define FTM_STATUS_OFF                  (0x010U)
#define FTM_STATUS_EXT_OFF              (0x014U)
#define FTM_MODE_OFF                    (0x018U)
#define FTM_SYNC_OFF                    (0x01CU)
#define FTM_OUTINIT_OFF                 (0x020U)
#define FTM_OUTMASK_OFF                 (0x024U)
#define FTM_COMBINE_OFF                 (0x028U)
#define FTM_COMBINE_EXT_OFF             (0x02CU)
#define FTM_DEADTIME_OFF                (0x030U)
#define FTM_EXTTRIG_OFF                 (0x034U)
#define FTM_POL_OFF                     (0x038U)
#define FTM_FMS_OFF                     (0x03CU)
#define FTM_FILTER_OFF                  (0x040U)
#define FTM_FLTCTRL_OFF                 (0x044U)
#define FTM_QDCTRL_OFF                  (0x048U)
#define FTM_CONF_OFF                    (0x04CU)
#define FTM_FLTPOL_OFF                  (0x050U)
#define FTM_SYNCONF_OFF                 (0x054U)
#define FTM_INVCTRL_OFF                 (0x058U)
#define FTM_SWOCTRL_OFF                 (0x05CU)
#define FTM_PWMLOAD_OFF                 (0x060U)
#define FTM_CV_OFF(ch)                 (0x080U + (ch) * 4U)
#define FTM_PAIRMASK_OFF                (0x100U)
#define FTM_MCTRL_OFF                   (0x1E0U)

/* FTM SC bits */
#define FTM_SC_PS_SHIFT                 (0U)
#define FTM_SC_PS_MASK                  (0x00000007U)   /* Prescaler */
#define FTM_SC_CLKS_SHIFT               (3U)
#define FTM_SC_CLKS_MASK                (0x00000018U)   /* Clock source */
#define FTM_SC_CLKS_DISABLED            (0x00000000U)
#define FTM_SC_CLKS_SYSTEM              (0x00000008U)
#define FTM_SC_CLKS_FIXED               (0x00000010U)
#define FTM_SC_CLKS_EXT                 (0x00000018U)
#define FTM_SC_CPWMS                   (0x00000020U)   /* Center-aligned PWM */
#define FTM_SC_TOIE                    (0x00000040U)   /* Timeout interrupt */
#define FTM_SC_TOF                     (0x00000080U)   /* Timeout flag */
#define FTM_SC_DMA                     (0x00000100U)   /* DMA enable */

/* FTM MODE bits */
#define FTM_MODE_FTMEN                 (0x00000001U)   /* FTM enable */
#define FTM_MODE_INIT                  (0x00000002U)   /* Initialize channels */
#define FTM_MODE_WPDIS                 (0x00000004U)   /* Write protection disable */
#define FTM_MODE_PWMSYNC               (0x00000008U)   /* PWM sync */
#define FTM_MODE_CAPTEST               (0x00000010U)   /* Capture test */
#define FTM_MODE_FAULTM_SHIFT          (6U)
#define FTM_MODE_FAULTM_MASK           (0x000000C0U)   /* Fault mode */

/* FTM SC specific CLKS values (common) */
#define FTM_SC_CLKS_SYSTEM_CLK         (0x00000008U)

/*-----------------------------------------------------------------------------
 * STM0 (System Timer Module 0) -- 0x4012_0000
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x00   | CR              | Control Register
 * 0x04   | CNT             | Count Register
 * 0x08   | CMP0            | Compare 0
 * 0x0C   | CMP1            | Compare 1
 * 0x10   | CMP2            | Compare 2
 * 0x14   | CMP3            | Compare 3
 * 0x18   | SR              | Status Register
 *----------------------------------------------------------------------------*/
#define STM0_CR_OFF                     (0x00U)
#define STM0_CNT_OFF                    (0x04U)
#define STM0_CMP_OFF(ch)               (0x08U + (ch) * 4U)
#define STM0_SR_OFF                     (0x18U)

#define STM0_CR_EN                      (0x00000001U)   /* Counter enable */
#define STM0_CR_FRZ                     (0x00000002U)   /* Freeze */
#define STM0_CR_SYNCEN                  (0x00000004U)   /* Sync enable */
#define STM0_SR_CMP_IF(ch)             (1UL << (ch))   /* Compare channel flag */

/*-----------------------------------------------------------------------------
 * SWT (Software Watchdog Timer) -- 0x4013_0000
 *
 * Offset | Register        | Description
 * -------+-----------------+--------------------------------
 * 0x00   | CR              | Control Register
 * 0x04   | TO              | Timeout Register
 * 0x08   | WIN             | Window Register
 * 0x0C   | SR              | Status Register
 * 0x10   | SK              | Service Key Register
 * 0x14   | IR              | Interrupt Register
 *----------------------------------------------------------------------------*/
#define SWT_CR_OFF                      (0x00U)
#define SWT_TO_OFF                      (0x04U)
#define SWT_WIN_OFF                     (0x08U)
#define SWT_SR_OFF                      (0x0CU)
#define SWT_SK_OFF                      (0x10U)
#define SWT_IR_OFF                      (0x14U)

#define SWT_CR_WEN                      (0x00000001U)   /* Watchdog enable */
#define SWT_CR_FRZ                      (0x00000002U)   /* Freeze */
#define SWT_CR_STP                      (0x00000004U)   /* Stop */
#define SWT_CR_WDT                      (0x00000008U)   /* Window mode */
#define SWT_CR_SST                      (0x00000010U)   /* Self-supervised test */
#define SWT_CR_IT_EN                    (0x00000100U)   /* Interrupt enable */
#define SWT_CR_RL                       (0x00000200U)   /* Reset lock */
#define SWT_CR_CS                       (0x00000400U)   /* Clock select */
#define SWT_CR_SL                       (0x00000800U)   /* Service lock */
#define SWT_SR_WTO                      (0x00000001U)   /* Watchdog timeout */
#define SWT_SR_WOV                      (0x00000002U)   /* Window overflow */

#define SWT_SERVICE_KEY                 (0xA5A5A5A5UL)

/*-----------------------------------------------------------------------------
 * MCU (SCG + SIM + ME) Base -- 0x402A_0000
 *
 * NOTE: MCU encompasses the System Clock Generator (SCG),
 *       System Integration Module (SIM), and Mode Entry (ME).
 *       Sub-blocks are at fixed offsets:
 *       ME:  0x0000 (default)
 *       SCG: 0x1000
 *       SIM: 0x2000
 *----------------------------------------------------------------------------*/
#define MCU_ME_BASE                     (S32K312_MCU_BASE + 0x0000U)
#define MCU_SCG_BASE                    (S32K312_MCU_BASE + 0x1000U)
#define MCU_SIM_BASE                    (S32K312_MCU_BASE + 0x2000U)

/*===================================================================
 * Access macros for quick register access
 *===================================================================*/

/* Register access (volatile pointer to uint32) */
#define S32K312_REG(base, offset)       (*(volatile uint32*)((uint32)(base) + (uint32)(offset)))
#define S32K312_REG16(base, offset)     (*(volatile uint16*)((uint32)(base) + (uint32)(offset)))
#define S32K312_REG8(base, offset)      (*(volatile uint8*)((uint32)(base) + (uint32)(offset)))

/* Convenience macros for LPUART, CAN, WDOG, etc. */
#define LPUART_REG(instance, offset)    S32K312_REG(S32K312_LPUART##instance##_BASE, offset)
#define FLEXCAN_REG(instance, offset)   S32K312_REG(S32K312_CAN##instance##_BASE, offset)
#define WDOG_REG(offset)                S32K312_REG(S32K312_WDOG_BASE, offset)
#define ADC_REG(instance, offset)       S32K312_REG(S32K312_ADC##instance##_BASE, offset)
#define FTM_REG(instance, offset)       S32K312_REG(S32K312_FTM##instance##_BASE, offset)
#define PIT_REG(ch, regoff)             S32K312_REG(S32K312_PIT_BASE, regoff)
#define STM0_REG(offset)                S32K312_REG(S32K312_STM0_BASE, offset)
#define SWT_REG(offset)                 S32K312_REG(S32K312_SWT_BASE, offset)
#define SIUL2_REG(offset)               S32K312_REG(S32K312_SIUL2_BASE, offset)

#ifdef __cplusplus
}
#endif

#endif /* S32K312_H */
