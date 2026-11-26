/*==================================================================================================
 * S32K312_LegacyCompat.h - Legacy S32K312.h macro compatibility for RTD builds
 *
 * This header is included ONLY when YULE_USE_NXP_RTD is defined (see S32K312.h).
 * It maps the legacy hand-written S32K312_* base macros used by the MCAL drivers
 * onto the NXP RTD official register definitions from S32K312_RTD.h.
 *
 * Two classes of macros are provided:
 *
 *   1. RTD-backed: the peripheral has a genuine memory-mapped RTD equivalent,
 *      so the legacy name is mapped to the official IP_* base macro. This
 *      replaces the (inaccurate) hand-written addresses with NXP-correct ones.
 *
 *   2. Synthesized: the peripheral (or driver register model) has no direct
 *      RTD memory-mapped equivalent on S32K3 (e.g. FTM which does not exist on
 *      S32K3, HSM which is reached via MU/HSE, the legacy SIUL2 "GPIO window"
 *      model). These keep the historical values so the driver compiles, and are
 *      documented as NOT RTD-backed.
 *
 * Reference: NXP S32K3xx RM Rev.4 / RTD R21-11.
 *================================================================================================*/
#ifndef S32K312_LEGACY_COMPAT_H
#define S32K312_LEGACY_COMPAT_H

#include <stddef.h>   /* offsetof */

/*==================================================================================================
 * 1. RTD-backed peripherals (official IP_* base addresses)
 *================================================================================================*/
/* FlexCAN: S32K312 has 6 instances; legacy map exposed CAN0/CAN1. */
#define S32K312_CAN0_BASE               (IP_CAN_0_BASE)
#define S32K312_CAN1_BASE               (IP_CAN_1_BASE)

/* LPUART */
#define S32K312_LPUART0_BASE            (IP_LPUART_0_BASE)
#define S32K312_LPUART1_BASE            (IP_LPUART_1_BASE)
#define S32K312_LPUART2_BASE            (IP_LPUART_2_BASE)
#define S32K312_LPUART3_BASE            (IP_LPUART_3_BASE)

/* LPI2C */
#define S32K312_LPI2C0_BASE             (IP_LPI2C_0_BASE)
#define S32K312_LPI2C1_BASE             (IP_LPI2C_1_BASE)

/* ADC */
#define S32K312_ADC0_BASE               (IP_ADC_0_BASE)
#define S32K312_ADC1_BASE               (IP_ADC_1_BASE)

/* PIT (used as GPT time base) */
#define S32K312_PIT_BASE                (IP_PIT_0_BASE)

/* SIUL2 (Port mux / GPIO control) */
#define S32K312_SIUL2_BASE              (IP_SIUL2_BASE)

/*==================================================================================================
 * 2. Synthesized peripherals (no direct RTD memory-mapped equivalent)
 *
 *    Retained with historical values so the existing driver models compile.
 *    These are NOT backed by RTD register definitions and must be revisited
 *    when the corresponding driver is migrated to the real S32K3 IP
 *    (FTM -> eMIOS, WDOG -> SWT, HSM -> MU/HSE, MCU -> MC_ME/MC_CGM/PLL, ...).
 *================================================================================================*/
/* Legacy SIUL2 "GPIO window" model (PDOR/PDIR/PDDR at +0x0/+0x10/+0x14).
 * NOTE: real S32K3 GPIO uses SIUL2 GPDO/GPDI/PGPDO registers, not this window. */
#define S32K312_SIUL2_GPIO_BASE         0x40810000UL

/* FTM: not present on S32K3 (eMIOS is the timer IP). PWM driver still models FTM. */
#define S32K312_FTM0_BASE               0x400D0000UL
#define S32K312_FTM1_BASE               0x400D2000UL
#define S32K312_FTM2_BASE               0x400D4000UL
#define S32K312_FTM3_BASE               0x400D6000UL

/* Watchdog: legacy WDOG model (S32K3 uses SWT hardware). */
#define S32K312_WDOG_BASE               0x40053000UL

/* MCU aggregate (clock/reset); real IP split as MC_ME/MC_CGM/PLL/PMC. */
#define S32K312_MCU_BASE                0x402A0000UL

/* Flash controller base used by the PFlash driver model. */
#define S32K312_FLASH_CTRL_BASE         0x40020000UL
/* Flash memory alias (boot alias window). */
#define S32K312_FLASH_BASE_ALIAS        0x00000000UL

/* HSM: reached via MU/HSE on S32K3, not a linear register block. Crypto (stub) only. */
#define S32K312_HSM_BASE                0x40460000UL
#define S32K312_HSM_AES_BASE            0x40461000UL
#define S32K312_HSM_ECC_BASE            0x40462000UL
#define S32K312_HSM_SHA_BASE            0x40463000UL
#define S32K312_HSM_TRNG_BASE           0x40464000UL
#define S32K312_HSM_KEYSTORE_BASE       0x40465000UL

/*==================================================================================================
 * 3. Register offset / bit constants shared by the drivers
 *
 *    SIUL2 MSCR offset is taken from the official RTD struct layout so the
 *    Port driver indexes the real MSCR array (0x240) instead of the stale
 *    hand-written value (0x20).
 *================================================================================================*/
#define SIUL2_MSCR0_OFF                 ((uint32_t)offsetof(SIUL2_Type, MSCR))
#define SIUL2_MSCR_SSS_SHIFT            (24U)

/* Legacy SIUL2 GPIO window offsets (see synthesized section above). */
#define SIUL2_GPIO_PDOR_OFF             (0x0000U)
#define SIUL2_GPIO_PDIR_OFF             (0x0010U)
#define SIUL2_GPIO_PDDR_OFF             (0x0014U)

/* Watchdog (legacy WDOG model) register and bit definitions. */
#define WDOG_CS_OFF                     (0x00U)
#define WDOG_CS_STOP                    (0x00000001U)
#define WDOG_CS_UPDATE                  (0x00000020U)
#define WDOG_CS_INT                     (0x00000040U)
#define WDOG_CS_EN                      (0x00000080U)

/* PIT control bits.
 * RTD's S32K312_PIT.h defines PIT_MCR_FRZ(x) as a function-like bit-field
 * helper; the legacy GPT driver uses PIT_MCR_FRZ as a plain mask value, so
 * the vendor function-like form is dropped and the legacy value restored. */
#undef PIT_MCR_FRZ
#define PIT_MCR_FRZ                     (0x00000002U)

#endif /* S32K312_LEGACY_COMPAT_H */
