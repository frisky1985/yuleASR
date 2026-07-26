/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Mock HAL — Preconfigured Register Defaults
*
* SW Version           : 1.0.0
* Build Date           : 2026-07-26
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file mock_hal_config.h
* @brief Default register values per MCAL module for S32K312 memory map
* @details Each module needs:
*   1. Clock enable bit (PCC) — 1
*   2. Version/ID register — non-zero
*   3. Status bits that poll loops wait for
*   4. Configuration shadow registers
*
* MCAL modules & S32K312 base addresses:
*   PORT:  0x4004_9000  (SIUL2)
*   ADC:   0x4003_B000
*   PWM:   0x4003_8000  (FTM0)
*   CAN:   0x4002_C000  (CAN0) — actual: 0x308C0000 (FlexCAN on i.MX8M Mini)
*   ICU:   0x4003_7000  (PIT)
*   SPI:   0x4002_C000  (LPSPI0)
*   GPT:   0x4003_7000  (PIT)
*   DIO:   0x4004_9000  (SIUL2 GPIO)
*   WDG:   0x4005_2000  (WDOG)
==================================================================================================*/

#ifndef MOCK_HAL_CONFIG_H
#define MOCK_HAL_CONFIG_H

#include <stdint.h>

/*==================================================================================================
*                                      PORT / DIO (SIUL2) — 0x4004_9000
==================================================================================================*/
#define SIUL2_BASE                      0x40049000UL
#define SIUL2_MIDR1                     (SIUL2_BASE + 0x0000UL)   /* Module ID Register 1 */
#define SIUL2_MIDR2                     (SIUL2_BASE + 0x0004UL)   /* Module ID Register 2 */
#define SIUL2_GPIO_BASE                 0x4004_A000UL             /* GPIO block (DIO) */

/*==================================================================================================
*                                      ADC — 0x4003_B000
==================================================================================================*/
#define ADC0_BASE                       0x4003B000UL
#define ADC_VERID                       0x0000UL                  /* Version ID */
#define ADC_PARAM                       0x0004UL                  /* Parameter */
#define ADC_GC                          0x0020UL                  /* General Control */
#define ADC_GS                          0x0024UL                  /* General Status */
#define ADC_CFG                         0x0028UL                  /* Configuration */

#define ADC_VERID_VAL                   0x01010000UL              /* Version 1.1 */
#define ADC_PARAM_VAL                   0x00010010UL              /* 1 group, 16 channels */
#define ADC_GS_IDLE                     0x00000000UL              /* Idle status */

/*==================================================================================================
*                                      PWM / FTM — 0x4003_8000
==================================================================================================*/
#define FTM0_BASE                       0x40038000UL
#define FTM1_BASE                       0x40039000UL
#define FTM2_BASE                       0x4003A000UL

#define FTM_SC                          0x0000UL                  /* Status & Control */
#define FTM_CNT                         0x0004UL                  /* Counter */
#define FTM_MOD                         0x0008UL                  /* Modulo */
#define FTM_C0SC                        0x000CUL                  /* Channel 0 Status & Control */
#define FTM_C0V                         0x0010UL                  /* Channel 0 Value */
#define FTM_C1SC                        0x0014UL
#define FTM_C1V                         0x0018UL
#define FTM_C2SC                        0x001CUL
#define FTM_C2V                         0x0020UL
#define FTM_C3SC                        0x0024UL
#define FTM_C3V                         0x0028UL
#define FTM_CNTIN                       0x004CUL                  /* Counter Initial Value */
#define FTM_STATUS                      0x0050UL                  /* Capture Status */
#define FTM_MODE                        0x0054UL                  /* Features Mode Selection */
#define FTM_SYNC                        0x0058UL
#define FTM_OUTINIT                     0x005CUL
#define FTM_OUTMASK                     0x0060UL
#define FTM_COMBINE                     0x0064UL
#define FTM_DEADTIME                    0x0068UL
#define FTM_EXTTRIG                     0x006CUL
#define FTM_POL                         0x0070UL
#define FTM_FMS                         0x0074UL
#define FTM_FILTER                      0x0078UL
#define FTM_FLTCTRL                     0x007CUL
#define FTM_QDCTRL                      0x0080UL
#define FTM_CONF                        0x0084UL
#define FTM_FLTPOL                      0x0088UL
#define FTM_SYNCONF                     0x008CUL
#define FTM_SWOCTRL                     0x0090UL
#define FTM_PWMLOAD                     0x0094UL

#define FTM_MODE_VAL                    0x00000004UL              /* FTEN=1, WPDIS=0 */
#define FTM_SC_VAL                      0x00000000UL              /* Clock=disabled */

/*==================================================================================================
*                                      CAN / FlexCAN — 0x308C_0000 (i.MX8M Mini)
==================================================================================================*/
#define FLEXCAN0_BASE                   0x308C0000UL
#define FLEXCAN1_BASE                   0x308D0000UL

#define CAN_MCR                         0x0000UL
#define CAN_CTRL1                       0x0004UL
#define CAN_TIMER                       0x0008UL
#define CAN_RXMGMASK                    0x0010UL
#define CAN_RX14MASK                    0x0014UL
#define CAN_RX15MASK                    0x0018UL
#define CAN_ECR                         0x001CUL
#define CAN_ESR1                        0x0020UL
#define CAN_IMASK2                      0x0024UL
#define CAN_IMASK1                      0x0028UL
#define CAN_IFLAG2                      0x002CUL
#define CAN_IFLAG1                      0x0030UL
#define CAN_CTRL2                       0x0034UL
#define CAN_ESR2                        0x0038UL

#define CAN_MCR_FRZ                     0x00000001UL              /* Freeze Enable */
#define CAN_MCR_NOT_RDY                 0x00000200UL              /* Not Ready */
#define CAN_MCR_HALT                    0x00010000UL              /* Halt */
#define CAN_ESR1_BOFFINT                0x04000000UL              /* Bus Off Int */
#define CAN_ESR1_ACK_ERR                0x00020000UL              /* ACK Error */

/* Default CAN values needed for init to succeed */
#define CAN_MCR_INIT_OK                 0x00001000UL              /* SOFT_RST=0, FRZ=0, HALT=0 */
#define CAN_ESR1_OK                     0x00000000UL              /* No errors */
#define CAN_CTRL1_VAL                   0x00000000UL

/*==================================================================================================
*                                      ICU / PIT — 0x4003_7000
==================================================================================================*/
#define PIT_BASE                        0x40037000UL
#define PIT_MCR                         0x0000UL                  /* Module Control */
#define PIT_LTMR64H                     0x00E0UL                  /* 64-bit timer high */
#define PIT_LTMR64L                     0x00E4UL                  /* 64-bit timer low */

/* PIT channel registers (each 0x10 apart) */
#define PIT_CHANNEL_OFFSET              0x0010UL
#define PIT_LDVAL(ch)                   ((ch) * PIT_CHANNEL_OFFSET + 0x0000UL)
#define PIT_CVAL(ch)                    ((ch) * PIT_CHANNEL_OFFSET + 0x0004UL)
#define PIT_TCTRL(ch)                   ((ch) * PIT_CHANNEL_OFFSET + 0x0008UL)
#define PIT_TFLG(ch)                    ((ch) * PIT_CHANNEL_OFFSET + 0x000CUL)

#define PIT_MCR_FRZ                     0x00000001UL              /* Freeze in Debug */
#define PIT_TCTRL_EN                    0x00000001UL              /* Timer Enable */
#define PIT_TCTRL_IE                    0x00000002UL              /* Interrupt Enable */
#define PIT_TFLG_IF                     0x00000001UL              /* Interrupt Flag */

/* Default values */
#define PIT_MCR_VAL                     0x00000001UL              /* MDIS=0, FRZ=1 */

/*==================================================================================================
*                                      GPT — 0x4003_8000 area (uses same FTM block as PWM)
*                                      Or separate GPT block at 0x302C_0000 on i.MX8M
==================================================================================================*/
#define GPT1_BASE                       0x302C0000UL
#define GPT_CR                          0x0000UL                  /* Control */
#define GPT_PR                          0x0004UL                  /* Prescaler */
#define GPT_SR                          0x0008UL                  /* Status */
#define GPT_IR                          0x000CUL                  /* Interrupt */
#define GPT_OCR1                        0x0010UL                  /* Output Compare 1 */
#define GPT_OCR2                        0x0014UL
#define GPT_OCR3                        0x0018UL
#define GPT_ICR1                        0x001CUL                  /* Input Capture 1 */
#define GPT_ICR2                        0x0020UL
#define GPT_CNT                         0x0024UL                  /* Counter */

#define GPT_CR_SWR                      0x00001000UL              /* Software Reset */
#define GPT_CR_EN                       0x00000001UL              /* GPT Enable */
#define GPT_CR_ENMOD                    0x00000002UL              /* Enable Mode */
#define GPT_SR_OF1                      0x00000001UL              /* Output Flag 1 */

/* Default — SWR bit clear, GPT enabled */
#define GPT_CR_VAL                      0x00000000UL              /* SWR=0, EN=0 after reset */

/*==================================================================================================
*                                      SPI / LPSPI — 0x4002_C000
==================================================================================================*/
#define LPSPI0_BASE                     0x4002C000UL
#define LPSPI1_BASE                     0x4002D000UL
#define LPSPI_VERID                     0x0000UL
#define LPSPI_PARAM                     0x0004UL
#define LPSPI_CR                        0x0008UL
#define LPSPI_SR                        0x000CL
#define LPSPI_IER                       0x0010UL
#define LPSPI_DER                       0x0014UL
#define LPSPI_CFGR0                     0x0018UL
#define LPSPI_CFGR1                     0x001CL
#define LPSPI_DMR0                      0x0020UL
#define LPSPI_DMR1                      0x0024UL
#define LPSPI_CCR                       0x0028UL
#define LPSPI_FCR                       0x002CL
#define LPSPI_FSR                       0x0030UL
#define LPSPI_TCR                       0x0034UL
#define LPSPI_TDR                       0x0038UL
#define LPSPI_RSR                       0x003CL
#define LPSPI_RDR                       0x0040UL

#define LPSPI_VERID_VAL                 0x01040000UL              /* Version 1.4 */
#define LPSPI_PARAM_VAL                 0x00000004UL              /* 4 FIFOs */
#define LPSPI_SR_IDLE                   0x00000100UL              /* Idle flag set */

/*==================================================================================================
*                                      WDG / WDOG — 0x4005_2000
==================================================================================================*/
#define WDOG_BASE                       0x40052000UL
#define WDOG_CS                         0x0000UL                  /* Control & Status */
#define WDOG_CNT                        0x0004UL                  /* Counter */
#define WDOG_TOVAL                      0x0008UL                  /* Timeout Value */
#define WDOG_WIN                        0x000CUL                  /* Window */

#define WDOG_CS_EN                      0x00000080UL              /* Enable */
#define WDOG_CS_UPDATE                  0x00000020UL              /* Update Enable */
#define WDOG_CS_INT                     0x00000040UL              /* Interrupt */
#define WDOG_CS_STOP                    0x00000001UL              /* Stop in debug */

/* Default — watchdog disabled, update enabled */
#define WDOG_CS_VAL                     0x00000020UL              /* UPDATE=1, EN=0 */

/*==================================================================================================
*                                      PCC (Clock Control) — 0x4006_5000
==================================================================================================*/
#define PCC_BASE                        0x40065000UL

#define PCC_PORT_OFFSET                 0x0090UL                  /* SIUL2 clock gate */
#define PCC_ADC0_OFFSET                 0x006CUL                  /* ADC0 clock gate */
#define PCC_FTM0_OFFSET                 0x008CUL                  /* FTM0 clock gate */
#define PCC_FTM1_OFFSET                 0x0090UL                  /* FTM1 clock gate */
#define PCC_FTM2_OFFSET                 0x0094UL                  /* FTM2 clock gate */
#define PCC_CAN0_OFFSET                 0x0068UL                  /* CAN0 clock gate */
#define PCC_LPSPI0_OFFSET               0x007CUL                  /* LPSPI0 clock gate */
#define PCC_PIT_OFFSET                  0x0080UL                  /* PIT clock gate */
#define PCC_CGC_MASK                    0xC0000000UL              /* Clock Gate Control bits */
#define PCC_CGC_ENABLED                 0x40000000UL              /* Clock enabled (bus) */

/*==================================================================================================
*                                      DEFAULT REGISTER TABLE
* Array of {address, value} pairs preloaded by mock_hal_set_defaults()
==================================================================================================*/

/* Defines for the table entries — NULL-terminated */
#define MOCK_HAL_CONFIG_END             {0x00000000UL, 0x00000000UL}

/* Module ID registers (return version info) */
#define MOCK_HAL_CONFIG_MODULE_IDS \
    {SIUL2_MIDR1,          0x00A50001UL}, \
    {SIUL2_MIDR2,          0x00000000UL}

/* ADC default registers */
#define MOCK_HAL_CONFIG_ADC \
    {ADC0_BASE + ADC_VERID, ADC_VERID_VAL}, \
    {ADC0_BASE + ADC_PARAM, ADC_PARAM_VAL}, \
    {ADC0_BASE + ADC_GS,    ADC_GS_IDLE}

/* FTM/PWM default registers */
#define MOCK_HAL_CONFIG_FTM(base) \
    {base + FTM_MODE,       FTM_MODE_VAL}, \
    {base + FTM_SC,         FTM_SC_VAL}

/* CAN default registers */
#define MOCK_HAL_CONFIG_CAN(base) \
    {base + CAN_MCR,        CAN_MCR_INIT_OK}, \
    {base + CAN_ESR1,       CAN_ESR1_OK}, \
    {base + CAN_CTRL1,      CAN_CTRL1_VAL}

/* PIT/ICU default registers */
#define MOCK_HAL_CONFIG_PIT \
    {PIT_BASE + PIT_MCR,    PIT_MCR_VAL}, \
    {PIT_BASE + PIT_LTMR64H, 0x00000000UL}, \
    {PIT_BASE + PIT_LTMR64L, 0x00000000UL}

/* GPT default registers */
#define MOCK_HAL_CONFIG_GPT(base) \
    {base + GPT_CR,         GPT_CR_VAL}, \
    {base + GPT_SR,         0x00000000UL}, \
    {base + GPT_IR,         0x00000000UL}

/* LPSPI default registers */
#define MOCK_HAL_CONFIG_LPSPI(base) \
    {base + LPSPI_VERID,    LPSPI_VERID_VAL}, \
    {base + LPSPI_PARAM,    LPSPI_PARAM_VAL}, \
    {base + LPSPI_SR,       LPSPI_SR_IDLE}

/* WDOG default registers */
#define MOCK_HAL_CONFIG_WDOG \
    {WDOG_BASE + WDOG_CS,   WDOG_CS_VAL}

/* PCC clock gates (enable all MCAL module clocks) */
#define MOCK_HAL_CONFIG_PCC_CLOCKS \
    {PCC_BASE + PCC_PORT_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_ADC0_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_FTM0_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_FTM1_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_FTM2_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_CAN0_OFFSET,    PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_LPSPI0_OFFSET,  PCC_CGC_ENABLED}, \
    {PCC_BASE + PCC_PIT_OFFSET,     PCC_CGC_ENABLED}

/*==================================================================================================
*                                      CONTEXT-SPECIFIC DEFAULT TABLES
* Each function returns the defaults for a specific MCAL module.
* Used by test setup to load module-specific register defaults.
==================================================================================================*/

/**
 * @brief Get default registers for PORT/DIO module
 */
static inline const uint32_t (*mock_hal_config_port(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_MODULE_IDS,
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for ADC module
 */
static inline const uint32_t (*mock_hal_config_adc(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_MODULE_IDS,
        MOCK_HAL_CONFIG_ADC,
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for PWM module (FTM0)
 */
static inline const uint32_t (*mock_hal_config_pwm(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_MODULE_IDS,
        MOCK_HAL_CONFIG_FTM(FTM0_BASE),
        MOCK_HAL_CONFIG_FTM(FTM1_BASE),
        MOCK_HAL_CONFIG_FTM(FTM2_BASE),
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for CAN module
 */
static inline const uint32_t (*mock_hal_config_can(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_CAN(FLEXCAN0_BASE),
        MOCK_HAL_CONFIG_CAN(FLEXCAN1_BASE),
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for ICU (PIT) module
 */
static inline const uint32_t (*mock_hal_config_icu(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_MODULE_IDS,
        MOCK_HAL_CONFIG_PIT,
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for GPT module
 */
static inline const uint32_t (*mock_hal_config_gpt(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_GPT(GPT1_BASE),
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for SPI (LPSPI) module
 */
static inline const uint32_t (*mock_hal_config_spi(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_LPSPI(LPSPI0_BASE),
        MOCK_HAL_CONFIG_LPSPI(LPSPI1_BASE),
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get default registers for WDG (WDOG) module
 */
static inline const uint32_t (*mock_hal_config_wdg(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_WDOG,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

/**
 * @brief Get all module defaults (for comprehensive test setup)
 */
static inline const uint32_t (*mock_hal_config_all(void))[2]
{
    static const uint32_t table[][2] = {
        MOCK_HAL_CONFIG_MODULE_IDS,
        MOCK_HAL_CONFIG_ADC,
        MOCK_HAL_CONFIG_FTM(FTM0_BASE),
        MOCK_HAL_CONFIG_FTM(FTM1_BASE),
        MOCK_HAL_CONFIG_FTM(FTM2_BASE),
        MOCK_HAL_CONFIG_CAN(FLEXCAN0_BASE),
        MOCK_HAL_CONFIG_CAN(FLEXCAN1_BASE),
        MOCK_HAL_CONFIG_PIT,
        MOCK_HAL_CONFIG_GPT(GPT1_BASE),
        MOCK_HAL_CONFIG_LPSPI(LPSPI0_BASE),
        MOCK_HAL_CONFIG_LPSPI(LPSPI1_BASE),
        MOCK_HAL_CONFIG_WDOG,
        MOCK_HAL_CONFIG_PCC_CLOCKS,
        MOCK_HAL_CONFIG_END
    };
    return table;
}

#endif /* MOCK_HAL_CONFIG_H */
