/******************************************************************************
 * @file        E2E_Cfg.h
 * @brief       E2E End-to-End Protection Configuration
 * @module      Services (E2E)
 * @version     1.0.0
 * @implements  E2E AUTOSAR Configuration
 * @note        This file contains all configurable parameters for E2E module
 ******************************************************************************/

#ifndef E2E_CFG_H
#define E2E_CFG_H

/******************************************************************************
 * GENERAL CONFIGURATION
 ******************************************************************************/

#define E2E_DEV_ERROR_DETECT            STD_ON
#define E2E_VERSION_INFO_API            STD_ON
#define E2E_SAFE_BSW_CHECKS             STD_ON

#define E2E_MAX_CHANNELS                16U
#define E2E_MAX_DATA_LENGTH             4096U
#define E2E_MAX_WINDOW_SIZE             8U

#define E2E_CRC8_TABLE_SIZE             256U
#define E2E_CRC16_TABLE_SIZE            256U
#define E2E_CRC32_TABLE_SIZE            256U

/******************************************************************************
 * PROFILE ENABLEMENT
 ******************************************************************************/

#define E2E_P01_ENABLED                 STD_ON
#define E2E_P02_ENABLED                 STD_ON
#define E2E_P04_ENABLED                 STD_ON
#define E2E_P05_ENABLED                 STD_ON
#define E2E_P06_ENABLED                 STD_ON
#define E2E_P07_ENABLED                 STD_ON

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 0 (E2E Profile 01)
 ******************************************************************************/
#define E2E_CH0_ENABLED                 STD_ON
#define E2E_CH0_PROFILE                 E2E_P01
#define E2E_CH0_DATAID                  0x01U
#define E2E_CH0_DATALENGTH              8U
#define E2E_CH0_COUNTER_OFFSET          0U
#define E2E_CH0_CRC_OFFSET              1U
#define E2E_CH0_DATAID_OFFSET           2U
#define E2E_CH0_COUNTER_SIZE            4U
#define E2E_CH0_CRC_SIZE                8U
#define E2E_CH0_DATAID_SIZE             8U
#define E2E_CH0_DATAID_NIBBLE           STD_OFF
#define E2E_CH0_INVERT_BITORDER         STD_OFF
#define E2E_CH0_MAX_DELTA_COUNTER       1U

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 1 (E2E Profile 02)
 ******************************************************************************/
#define E2E_CH1_ENABLED                 STD_ON
#define E2E_CH1_PROFILE                 E2E_P02
#define E2E_CH1_DATAID_LIST             {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, \
                                         0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU, 0x10U}
#define E2E_CH1_DATALENGTH              16U
#define E2E_CH1_COUNTER_OFFSET          0U
#define E2E_CH1_CRC_OFFSET              1U
#define E2E_CH1_COUNTER_SIZE            4U
#define E2E_CH1_CRC_SIZE                8U
#define E2E_CH1_INCLUDE_DATAID          STD_ON
#define E2E_CH1_MAX_DELTA_COUNTER       1U

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 2 (E2E Profile 04)
 ******************************************************************************/
#define E2E_CH2_ENABLED                 STD_ON
#define E2E_CH2_PROFILE                 E2E_P04
#define E2E_CH2_DATAID                  0x0001U
#define E2E_CH2_DATALENGTH              32U
#define E2E_CH2_OFFSET                  0U
#define E2E_CH2_MAX_DELTA_COUNTER       1U
#define E2E_CH2_MIN_DELTA_COUNTER_INIT  0U
#define E2E_CH2_MAX_NO_NEW_OR_REPEATED  15U
#define E2E_CH2_SYNC_COUNTER_INIT       0U

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 3 (E2E Profile 05)
 ******************************************************************************/
#define E2E_CH3_ENABLED                 STD_ON
#define E2E_CH3_PROFILE                 E2E_P05
#define E2E_CH3_DATAID                  0x01U
#define E2E_CH3_DATALENGTH              8U
#define E2E_CH3_COUNTER_OFFSET          0U
#define E2E_CH3_CRC_OFFSET              1U
#define E2E_CH3_E2E_LENGTH              2U
#define E2E_CH3_OFFSET                  0U

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 4 (E2E Profile 06)
 ******************************************************************************/
#define E2E_CH4_ENABLED                 STD_ON
#define E2E_CH4_PROFILE                 E2E_P06
#define E2E_CH4_DATAID                  0x0001U
#define E2E_CH4_DATALENGTH              64U
#define E2E_CH4_OFFSET                  0U
#define E2E_CH4_MAX_DELTA_COUNTER       1U
#define E2E_CH4_MIN_DELTA_COUNTER_INIT  0U
#define E2E_CH4_MAX_NO_NEW_OR_REPEATED  15U
#define E2E_CH4_SYNC_COUNTER_INIT       0U

/******************************************************************************
 * CHANNEL CONFIGURATION - Channel 5 (E2E Profile 07)
 ******************************************************************************/
#define E2E_CH5_ENABLED                 STD_ON
#define E2E_CH5_PROFILE                 E2E_P07
#define E2E_CH5_DATAID                  0x00000001UL
#define E2E_CH5_DATALENGTH              128U
#define E2E_CH5_OFFSET                  0U
#define E2E_CH5_MAX_DELTA_COUNTER       1U
#define E2E_CH5_MIN_DELTA_COUNTER_INIT  0U
#define E2E_CH5_MAX_NO_NEW_OR_REPEATED  15U
#define E2E_CH5_SYNC_COUNTER_INIT       0U

/******************************************************************************
 * CHANNEL 6-15 (Reserved for application)
 ******************************************************************************/
#define E2E_CH6_ENABLED                 STD_OFF
#define E2E_CH7_ENABLED                 STD_OFF
#define E2E_CH8_ENABLED                 STD_OFF
#define E2E_CH9_ENABLED                 STD_OFF
#define E2E_CH10_ENABLED                STD_OFF
#define E2E_CH11_ENABLED                STD_OFF
#define E2E_CH12_ENABLED                STD_OFF
#define E2E_CH13_ENABLED                STD_OFF
#define E2E_CH14_ENABLED                STD_OFF
#define E2E_CH15_ENABLED                STD_OFF

/******************************************************************************
 * CRC CONFIGURATION
 ******************************************************************************/
#define E2E_CRC8_POLYNOMIAL             E2E_CRC8_AUTOSAR
#define E2E_CRC16_POLYNOMIAL            E2E_CRC16_CCITT
#define E2E_CRC32_POLYNOMIAL            E2E_CRC32_AUTOSAR
#define E2E_CRC8_INITIAL_VALUE          0xFFU
#define E2E_CRC16_INITIAL_VALUE         0xFFFFU
#define E2E_CRC32_INITIAL_VALUE         0xFFFFFFFFUL
#define E2E_CRC8_XOR_VALUE              0xFFU
#define E2E_CRC16_XOR_VALUE             0x0000U
#define E2E_CRC32_XOR_VALUE             0xFFFFFFFFUL

/******************************************************************************
 * STATE MACHINE CONFIGURATION
 ******************************************************************************/
#define E2E_STATE_MACHINE_ENABLED       STD_ON
#define E2E_MAX_ERROR_COUNT             3U
#define E2E_ERROR_WINDOW_SIZE           10U
#define E2E_RECOVERY_TIMEOUT_MS         1000U

/******************************************************************************
 * MEMORY SECTIONS
 ******************************************************************************/
#define E2E_VAR                         VAR
#define E2E_CONST                       CONST
#define E2E_CODE                        CODE
#define E2E_CALLOUT_CODE                CALLOUT_CODE
#define E2E_APPL_DATA                   APPL_DATA
#define E2E_APPL_CONST                  APPL_CONST

/******************************************************************************
 * CALLBACK CONFIGURATION
 ******************************************************************************/
#define E2E_PROTECTION_CALLBACK         STD_ON
#define E2E_CHECK_CALLBACK              STD_ON

/******************************************************************************
 * DEBUG AND TRACING
 ******************************************************************************/
#define E2E_DEBUG_TRACE                 STD_OFF
#define E2E_PERFORMANCE_COUNTERS        STD_OFF

/******************************************************************************
 * E2E CHECK RESULT TYPE
 ******************************************************************************/
/**
 * @brief E2E check result type for callbacks
 * Contains the channel ID and status after an E2E check
 */
typedef struct {
    uint16 ChannelId;
    uint8  Status;
    uint8  Counter;
} E2E_CheckResultType;

/******************************************************************************
 * POST BUILD CONFIGURATION TYPE
 ******************************************************************************/
typedef struct
{
    uint8 Dummy;
} E2E_PBConfigType;

/******************************************************************************
 * CALLBACK FUNCTION DECLARATIONS
 ******************************************************************************/
extern FUNC(void, E2E_CALLOUT_CODE) E2E_ProtectionCallback(
    VAR(uint16, AUTOMATIC) ChannelId,
    VAR(uint8, AUTOMATIC) Status
);

extern FUNC(void, E2E_CALLOUT_CODE) E2E_CheckCallback(
    VAR(uint16, AUTOMATIC) ChannelId,
    E2E_CheckResultType* ResultPtr
);

#endif /* E2E_CFG_H */
