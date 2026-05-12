/**
 * @file Icu_Lcfg.h
 * @brief ICU Driver link-time configuration header file
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ICU_LCFG_H
#define ICU_LCFG_H

/*==================================================================================================
*                                    INCLUDE FILES
==================================================================================================*/
#include "Icu.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ICU_LCFG_VENDOR_ID                  (0x01U)
#define ICU_LCFG_MODULE_ID                  (0x10U)
#define ICU_LCFG_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define ICU_LCFG_AR_RELEASE_MINOR_VERSION   (0x04U)
#define ICU_LCFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define ICU_LCFG_SW_MAJOR_VERSION           (0x01U)
#define ICU_LCFG_SW_MINOR_VERSION           (0x00U)
#define ICU_LCFG_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    LINK-TIME CONFIGURATION
==================================================================================================*/

/* External declaration of link-time configuration structure */
#define ICU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Icu_ChannelConfigType Icu_ChannelConfig[ICU_NUM_CHANNELS];

#define ICU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/* Channel 0 - Edge Detection Mode */
#define ICU_CH0_MODE                        (ICU_MODE_SIGNAL_EDGE_DETECT)
#define ICU_CH0_EDGE                        (ICU_RISING_EDGE)
#define ICU_CH0_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH0_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH0_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH0_BUFFER_SIZE                 (0U)
#define ICU_CH0_PRESCALER                   (0U)
#define ICU_CH0_BASE_ADDR                   (ICU_TPM1_BASE_ADDR)

/* Channel 1 - Signal Measurement Mode */
#define ICU_CH1_MODE                        (ICU_MODE_SIGNAL_MEASUREMENT)
#define ICU_CH1_EDGE                        (ICU_RISING_EDGE)
#define ICU_CH1_PROPERTY                    (ICU_PERIOD_TIME)
#define ICU_CH1_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH1_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH1_BUFFER_SIZE                 (0U)
#define ICU_CH1_PRESCALER                   (0U)
#define ICU_CH1_BASE_ADDR                   (ICU_TPM1_BASE_ADDR)

/* Channel 2 - Timestamp Mode */
#define ICU_CH2_MODE                        (ICU_MODE_TIMESTAMP)
#define ICU_CH2_EDGE                        (ICU_BOTH_EDGES)
#define ICU_CH2_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH2_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH2_TIMESTAMP_ENABLED           (STD_ON)
#define ICU_CH2_BUFFER_SIZE                 (ICU_DEFAULT_BUFFER_SIZE)
#define ICU_CH2_PRESCALER                   (0U)
#define ICU_CH2_BASE_ADDR                   (ICU_TPM2_BASE_ADDR)

/* Channel 3 - Edge Counter Mode */
#define ICU_CH3_MODE                        (ICU_MODE_EDGE_COUNTER)
#define ICU_CH3_EDGE                        (ICU_RISING_EDGE)
#define ICU_CH3_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH3_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH3_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH3_BUFFER_SIZE                 (0U)
#define ICU_CH3_PRESCALER                   (0U)
#define ICU_CH3_BASE_ADDR                   (ICU_TPM2_BASE_ADDR)

/* Channel 4 - Edge Detection Mode */
#define ICU_CH4_MODE                        (ICU_MODE_SIGNAL_EDGE_DETECT)
#define ICU_CH4_EDGE                        (ICU_FALLING_EDGE)
#define ICU_CH4_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH4_WAKEUP_SUPPORT              (STD_ON)
#define ICU_CH4_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH4_BUFFER_SIZE                 (0U)
#define ICU_CH4_PRESCALER                   (0U)
#define ICU_CH4_BASE_ADDR                   (ICU_TPM3_BASE_ADDR)

/* Channel 5 - Signal Measurement Mode */
#define ICU_CH5_MODE                        (ICU_MODE_SIGNAL_MEASUREMENT)
#define ICU_CH5_EDGE                        (ICU_BOTH_EDGES)
#define ICU_CH5_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH5_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH5_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH5_BUFFER_SIZE                 (0U)
#define ICU_CH5_PRESCALER                   (0U)
#define ICU_CH5_BASE_ADDR                   (ICU_TPM3_BASE_ADDR)

/* Channel 6 - Timestamp Mode */
#define ICU_CH6_MODE                        (ICU_MODE_TIMESTAMP)
#define ICU_CH6_EDGE                        (ICU_RISING_EDGE)
#define ICU_CH6_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH6_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH6_TIMESTAMP_ENABLED           (STD_ON)
#define ICU_CH6_BUFFER_SIZE                 (ICU_DEFAULT_BUFFER_SIZE)
#define ICU_CH6_PRESCALER                   (0U)
#define ICU_CH6_BASE_ADDR                   (ICU_TPM4_BASE_ADDR)

/* Channel 7 - Edge Counter Mode */
#define ICU_CH7_MODE                        (ICU_MODE_EDGE_COUNTER)
#define ICU_CH7_EDGE                        (ICU_FALLING_EDGE)
#define ICU_CH7_PROPERTY                    (ICU_DUTY_CYCLE)
#define ICU_CH7_WAKEUP_SUPPORT              (STD_OFF)
#define ICU_CH7_TIMESTAMP_ENABLED           (STD_OFF)
#define ICU_CH7_BUFFER_SIZE                 (0U)
#define ICU_CH7_PRESCALER                   (0U)
#define ICU_CH7_BASE_ADDR                   (ICU_TPM4_BASE_ADDR)

#endif /* ICU_LCFG_H */
