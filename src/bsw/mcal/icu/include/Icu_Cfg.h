/**
 * @file Icu_Cfg.h
 * @brief ICU Driver configuration header file
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ICU_CFG_H
#define ICU_CFG_H

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ICU_CFG_VENDOR_ID                   (0x01U)
#define ICU_CFG_MODULE_ID                   (0x10U)
#define ICU_CFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define ICU_CFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define ICU_CFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define ICU_CFG_SW_MAJOR_VERSION            (0x01U)
#define ICU_CFG_SW_MINOR_VERSION            (0x00U)
#define ICU_CFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/* Development error detection */
#ifndef ICU_DEV_ERROR_DETECT
#define ICU_DEV_ERROR_DETECT                (STD_ON)
#endif

/* Version info API */
#ifndef ICU_VERSION_INFO_API
#define ICU_VERSION_INFO_API                (STD_ON)
#endif

/* DeInit API */
#ifndef ICU_DE_INIT_API
#define ICU_DE_INIT_API                     (STD_ON)
#endif

/* Set Mode API */
#ifndef ICU_SET_MODE_API
#define ICU_SET_MODE_API                    (STD_ON)
#endif

/* Wakeup functionality API */
#ifndef ICU_WAKEUP_FUNCTIONALITY_API
#define ICU_WAKEUP_FUNCTIONALITY_API        (STD_ON)
#endif

/* Disable Wakeup API */
#ifndef ICU_DISABLE_WAKEUP_API
#define ICU_DISABLE_WAKEUP_API              (STD_ON)
#endif

/* Timestamp API */
#ifndef ICU_TIMESTAMP_API
#define ICU_TIMESTAMP_API                   (STD_ON)
#endif

/* Edge Count API */
#ifndef ICU_EDGE_COUNT_API
#define ICU_EDGE_COUNT_API                  (STD_ON)
#endif

/* Signal Measurement API */
#ifndef ICU_SIGNAL_MEASUREMENT_API
#define ICU_SIGNAL_MEASUREMENT_API          (STD_ON)
#endif

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/* Number of ICU channels */
#define ICU_NUM_CHANNELS                    (8U)

/* Channel IDs */
#define ICU_CHANNEL_0                       (0U)
#define ICU_CHANNEL_1                       (1U)
#define ICU_CHANNEL_2                       (2U)
#define ICU_CHANNEL_3                       (3U)
#define ICU_CHANNEL_4                       (4U)
#define ICU_CHANNEL_5                       (5U)
#define ICU_CHANNEL_6                       (6U)
#define ICU_CHANNEL_7                       (7U)

/*==================================================================================================
*                                    HARDWARE CONFIGURATION (i.MX8M Mini)
==================================================================================================*/

/* TPM (Timer/PWM Module) Base Addresses */
#define ICU_TPM1_BASE_ADDR                  (0x30660000UL)
#define ICU_TPM2_BASE_ADDR                  (0x30670000UL)
#define ICU_TPM3_BASE_ADDR                  (0x30680000UL)
#define ICU_TPM4_BASE_ADDR                  (0x30690000UL)
#define ICU_TPM5_BASE_ADDR                  (0x306A0000UL)
#define ICU_TPM6_BASE_ADDR                  (0x306B0000UL)

/* TPM Register Offsets */
#define ICU_TPM_SC_OFFSET                   (0x00U)
#define ICU_TPM_CNT_OFFSET                  (0x04U)
#define ICU_TPM_MOD_OFFSET                  (0x08U)
#define ICU_TPM_C0SC_OFFSET                 (0x0CU)
#define ICU_TPM_C0V_OFFSET                  (0x10U)
#define ICU_TPM_STATUS_OFFSET               (0x50U)
#define ICU_TPM_CONF_OFFSET                 (0x84U)

/* TPM SC (Status and Control) Register Bits */
#define ICU_TPM_SC_PS_MASK                  (0x07U)
#define ICU_TPM_SC_CMOD_MASK                (0x18U)
#define ICU_TPM_SC_CPWMS                    (0x20U)
#define ICU_TPM_SC_TOIE                     (0x40U)
#define ICU_TPM_SC_TOF                      (0x80U)

/* TPM CnSC (Channel Status and Control) Register Bits */
#define ICU_TPM_CnSC_DMA                    (0x01U)
#define ICU_TPM_CnSC_ICRST                  (0x02U)
#define ICU_TPM_CnSC_ELSA                   (0x04U)
#define ICU_TPM_CnSC_ELSB                   (0x08U)
#define ICU_TPM_CnSC_MSA                    (0x10U)
#define ICU_TPM_CnSC_MSB                    (0x20U)
#define ICU_TPM_CnSC_CHIE                   (0x40U)
#define ICU_TPM_CnSC_CHF                    (0x80U)

/* TPM CONF (Configuration) Register Bits */
#define ICU_TPM_CONF_DOZEEN                 (0x00000020U)
#define ICU_TPM_CONF_DBGMODE_MASK           (0x000000C0U)
#define ICU_TPM_CONF_GTBEEN                 (0x00000200U)
#define ICU_TPM_CONF_CSOT                   (0x00010000U)
#define ICU_TPM_CONF_CSOO                   (0x00020000U)
#define ICU_TPM_CONF_CROT                   (0x00040000U)
#define ICU_TPM_CONF_CPOT                   (0x00080000U)
#define ICU_TPM_CONF_TRGPOL                 (0x01000000U)
#define ICU_TPM_CONF_TRGSRC                 (0x02000000U)

/* Edge Selection Values */
#define ICU_TPM_CnSC_RISING_EDGE            (ICU_TPM_CnSC_ELSB)
#define ICU_TPM_CnSC_FALLING_EDGE           (ICU_TPM_CnSC_ELSA)
#define ICU_TPM_CnSC_BOTH_EDGES             (ICU_TPM_CnSC_ELSA | ICU_TPM_CnSC_ELSB)

/*==================================================================================================
*                                    DEFAULT CONFIGURATION
==================================================================================================*/

/* Default mode */
#define ICU_DEFAULT_MODE                    (ICU_MODE_NORMAL)

/* Default timestamp buffer size */
#define ICU_DEFAULT_BUFFER_SIZE             (32U)

/* Max edge count value */
#define ICU_MAX_EDGE_COUNT                  (0xFFFFU)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/
typedef uint16 Icu_EdgeNumberType;

#endif /* ICU_CFG_H */
