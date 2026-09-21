/*==================================================================================================
 * @file    ModuleId.h
 * @brief   Centralized AUTOSAR Module ID Registry - Single Source of Truth
 * @version 1.0.0
 * @date    2026-09-03
 *
 * All Det_ReportError() module IDs MUST come from this file.
 * Each module header includes this file and maps its legacy macro name
 * to the canonical MODULE_ID_xxx defined here.
 *
 * ID allocation ranges:
 *   0x00-0x0F  Core (OS, Det, Std)
 *   0x10-0x2F  MCAL Drivers
 *   0x30-0x4F  ECUAL
 *   0x50-0x5F  MCAL extended / Communication Services
 *   0x60-0x6F  Services
 *   0x70-0x7F  Middleware
 *   0x80-0x8F  CDD (Complex Device Drivers)
 *   0x90-0x9F  Safety & Custom
 *   0xA0-0xAF  Network Management extended
 *   0xB0-0xBF  Application SWC
 *   0xC0-0xCF  Diagnostic Transport
 *   0xD0-0xDF  Measurement & Calibration
 *   0xE0-0xEF  Security
 *   0xF0-0xFF  Protection (E2E, CRC)
 *
 * Reference: AUTOSAR SWS, R22-11
 *================================================================================================*/

#ifndef MODULE_ID_H
#define MODULE_ID_H

/*==============================================================================================
 * Core (0x00-0x0F)
 *==============================================================================================*/
#define MODULE_ID_STD_TYPES         0x00U
#define MODULE_ID_OS                0x01U
#define MODULE_ID_DET               0x02U
#define MODULE_ID_CANSM             0x08U   /* AUTOSAR: 8 */
#define MODULE_ID_ECUM              0x0AU   /* AUTOSAR: 10 */
#define MODULE_ID_LDCOM             0x0BU   /* AUTOSAR: 11 */
#define MODULE_ID_TM                0x0CU   /* AUTOSAR: 12 */
#define MODULE_ID_ETHTSYN           0x0DU   /* CHANGED: was 0x0A, conflict with EcuM */

/*==============================================================================================
 * MCAL Drivers (0x10-0x2F)
 *==============================================================================================*/
#define MODULE_ID_WDG               0x10U   /* AUTOSAR: 16 */
#define MODULE_ID_UART              0x11U   /* Custom */
#define MODULE_ID_COMM              0x12U   /* AUTOSAR: 18 (ComM) */
#define MODULE_ID_ECUC              0x13U   /* AUTOSAR: 19 */
#define MODULE_ID_NVM               0x14U   /* AUTOSAR: 20 */
#define MODULE_ID_ICU               0x16U   /* AUTOSAR: 22 */
#define MODULE_ID_NM                0x1DU   /* AUTOSAR: 29 */
#define MODULE_ID_COM               0x1EU   /* AUTOSAR: 30 */
#define MODULE_ID_CANNM             0x1FU   /* AUTOSAR: 31 */
#define MODULE_ID_DIO               0x29U   /* AUTOSAR: 41 */
#define MODULE_ID_PORT              0x2AU   /* AUTOSAR: 42 */
#define MODULE_ID_MCU               0x2BU   /* AUTOSAR: 43 */
#define MODULE_ID_ADC               0x2CU   /* AUTOSAR: 44 */
#define MODULE_ID_DOIP_ECUAL        0x25U   /* Custom */
#define MODULE_ID_LINIF             0x27U   /* AUTOSAR: 39 */

/*==============================================================================================
 * ECUAL (0x30-0x4F)
 *==============================================================================================*/
#define MODULE_ID_EA                0x31U   /* AUTOSAR: 49 */
#define MODULE_ID_FEE               0x32U   /* AUTOSAR: 50 */
#define MODULE_ID_UDPNM             0x33U   /* AUTOSAR: 51 */
#define MODULE_ID_CANIF             0x3CU   /* AUTOSAR: 60 */
#define MODULE_ID_CANTP             0x3DU   /* AUTOSAR: 61 */
#define MODULE_ID_CANTRCV           0x3EU   /* AUTOSAR: 62 */
#define MODULE_ID_FRIF              0x3FU   /* AUTOSAR: 63 */
#define MODULE_ID_ETHIF             0x40U   /* CHANGED: was 0x70 */
#define MODULE_ID_ETHSM_ECUAL       0x41U   /* CHANGED: was 0x43 */
#define MODULE_ID_ETHTRCV           0x42U   /* CHANGED: was unassigned */
#define MODULE_ID_J1939TP           0x44U   /* AUTOSAR: 68 */
#define MODULE_ID_LINNM             0x45U   /* AUTOSAR: 69 */
#define MODULE_ID_LINTRCV           0x46U   /* CHANGED: was unassigned */
#define MODULE_ID_LINM_ECUAL        0x47U   /* CHANGED: was unassigned */
#define MODULE_ID_SOMEIPIF          0x48U   /* CHANGED: was 0x82 */
#define MODULE_ID_SOMEIPSD_ECUAL    0x49U   /* CHANGED: was 0x81 */
#define MODULE_ID_MEMIF_ECUAL       0x4AU   /* CHANGED: was 0x16 */
#define MODULE_ID_FIM_ECUAL         0x4BU   /* CHANGED: was 0x71 */
#define MODULE_ID_IOHWAB            0x78U   /* AUTOSAR: 120 - CHANGED: was 0x7A */
#define MODULE_ID_IPDUM_ECUAL       0x4DU   /* AUTOSAR: 77 */
#define MODULE_ID_DOCAN_ECUAL       0x4EU   /* CHANGED: was 0x4D */
#define MODULE_ID_SOAD              0x43U   /* AUTOSAR: 67 */

/*==============================================================================================
 * Services - Communication & Scheduler (0x35-0x6F)
 *==============================================================================================*/
#define MODULE_ID_DCM               0x35U   /* AUTOSAR: 53 - CHANGED: was 0x29 */
#define MODULE_ID_BSWM              0x36U   /* CHANGED: was 0x12 */
#define MODULE_ID_SCHM              0x3AU   /* AUTOSAR: 58 */
#define MODULE_ID_MEMIF_SERVICES    0x4CU   /* CHANGED: was 0x16, conflict with ICU */
#define MODULE_ID_PDUR              0x69U   /* AUTOSAR: 105 */
#define MODULE_ID_MEM               0x6AU   /* CHANGED: was unassigned */
#define MODULE_ID_SWC               0x6BU   /* CHANGED: was 0x50 */
#define MODULE_ID_LINTP_ECUAL       0x62U   /* AUTOSAR: 98 */
#define MODULE_ID_RAMTST_MCAL       0x64U   /* AUTOSAR: 100 */
#define MODULE_ID_FIM_SERVICES      0x56U   /* CHANGED: was 0x55 */

/*==============================================================================================
 * MCAL extended (0x50-0x5F)
 *==============================================================================================*/
#define MODULE_ID_CAN               0x50U   /* AUTOSAR: 80 */
#define MODULE_ID_LIN               0x52U   /* AUTOSAR: 82 */
#define MODULE_ID_ETH               0x53U   /* AUTOSAR: 83 */
#define MODULE_ID_DEM               0x54U   /* AUTOSAR: 84 */
#define MODULE_ID_TCPIP             0x55U   /* AUTOSAR: 85 */
#define MODULE_ID_I2C               0x57U   /* Custom */
#define MODULE_ID_FLS               0x5CU   /* AUTOSAR: 92 */
#define MODULE_ID_EEP               0x5FU   /* AUTOSAR: 95 */

/*==============================================================================================
 * Middleware (0x70-0x7F)
 *==============================================================================================*/
#define MODULE_ID_RTE               0x70U
#define MODULE_ID_SOMEIP            0x73U   /* CHANGED: was 0x70 */
#define MODULE_ID_SOMEIPSD_SERVICES 0x74U   /* CHANGED: was 0x71 */
#define MODULE_ID_SOMEIPTP          0x75U   /* CHANGED: was 0x7C */
#define MODULE_ID_SOMEIPXF          0x76U   /* CHANGED: was 0x7B */
#define MODULE_ID_SD                0x71U
#define MODULE_ID_SPI               0x7AU   /* AUTOSAR: 122 */
#define MODULE_ID_GPT               0x79U   /* AUTOSAR: 121 */
#define MODULE_ID_PWM               0x7BU   /* AUTOSAR: 123 */
#define MODULE_ID_OCU               0x7DU   /* CHANGED: was 0x7A */
#define MODULE_ID_CRYPTO            0x7CU   /* AUTOSAR: 124 */
#define MODULE_ID_DDS               0x7EU   /* Custom */

/*==============================================================================================
 * CDD - Complex Device Drivers (0x80-0x8F)
 *==============================================================================================*/
#define MODULE_ID_CDD_HSM           0x80U
#define MODULE_ID_CDD_RAMECC        0x81U
#define MODULE_ID_CDD_LOCKSTEP      0x82U
#define MODULE_ID_CDD_SAFETY        0x83U
#define MODULE_ID_CDD_BOOT          0x84U
#define MODULE_ID_CDD_FVM           0x85U
#define MODULE_ID_S32K312_HSM       0x86U   /* CHANGED: was 0x81 */
#define MODULE_ID_ETHSWT            0x88U   /* AUTOSAR: 136 */
#define MODULE_ID_FLSTST            0x89U
#define MODULE_ID_RAMTST_SERVICES   0x8AU
#define MODULE_ID_ETHSM_SERVICES    0xA0U   /* CHANGED: was 0x8A */
#define MODULE_ID_LINM_SERVICES     0x8EU   /* AUTOSAR: 142 */
#define MODULE_ID_LINSM             0x8FU   /* AUTOSAR: 143 */
#define MODULE_ID_J1939NM           0x8DU   /* AUTOSAR: 141 */

/*==============================================================================================
 * Safety & Custom (0x90-0x9F)
 *==============================================================================================*/
#define MODULE_ID_SRP               0x90U
#define MODULE_ID_LINTP_SERVICES    0x92U   /* CHANGED: was 0x90 */
#define MODULE_ID_RAMSAFETY         0x91U

/*==============================================================================================
 * Network Management extended (0xA0-0xAF)
 *==============================================================================================*/
#define MODULE_ID_STBM              0xA2U   /* AUTOSAR: 162 */
#define MODULE_ID_CANTPSYN          0xA4U   /* AUTOSAR: 164 */

/*==============================================================================================
 * Application SWC (0xB0-0xBF)
 *==============================================================================================*/
#define MODULE_ID_MQTT              0xB0U   /* CHANGED: was 0xB0 */
#define MODULE_ID_SWC_ENGINE        0xB1U   /* CHANGED: was 0x80 */
#define MODULE_ID_SWC_VEHICLE       0xB2U   /* CHANGED: was 0x81 */
#define MODULE_ID_SWC_DIAG          0xB3U   /* CHANGED: was 0x82 */
#define MODULE_ID_SWC_COMM          0xB4U   /* CHANGED: was 0x83 */
#define MODULE_ID_SWC_STORAGE       0xB5U   /* CHANGED: was 0x84 */
#define MODULE_ID_SWC_IO            0xB6U   /* CHANGED: was 0x85 */
#define MODULE_ID_SWC_MODE          0xB7U   /* CHANGED: was 0x86 */
#define MODULE_ID_SWC_WDG_MGR      0xB8U   /* CHANGED: was 0x87 */

/*==============================================================================================
 * Diagnostic Transport (0xC0-0xCF)
 *==============================================================================================*/
#define MODULE_ID_DOIP_SERVICES     0xC0U   /* CHANGED: was 0x4C */
#define MODULE_ID_DOCAN_SERVICES    0xC1U   /* CHANGED: was 0x4D */

/*==============================================================================================
 * Measurement (0xD0-0xDF)
 *==============================================================================================*/
#define MODULE_ID_XCP               0xD0U   /* AUTOSAR: 208 */

/*==============================================================================================
 * Security (0xE0-0xEF)
 *==============================================================================================*/
#define MODULE_ID_SECOC             0xE0U
#define MODULE_ID_CSM               0xE1U   /* CHANGED: was 0x70 */
#define MODULE_ID_CRYIF             0xE2U   /* CHANGED: was 0x7C */
#define MODULE_ID_KEYM              0xE3U

/*==============================================================================================
 * Protection (0xF0-0xFF)
 *==============================================================================================*/
#define MODULE_ID_E2E               0xF0U   /* AUTOSAR: 240 */
#define MODULE_ID_CRC               0xF1U

#endif /* MODULE_ID_H */
