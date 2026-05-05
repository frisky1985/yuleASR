/*******************************************************************************
 * File Name          : LinTrcv_Cfg.h
 * Description        : AUTOSAR LIN Transceiver Driver Configuration header
 *                      Pre-compile and link-time configuration parameters
 ******************************************************************************/

#ifndef LINTRCV_CFG_H
#define LINTRCV_CFG_H

/*=============================================================================
 * Includes
 ============================================================================*/
#include "Std_Types.h"

/*=============================================================================
 * Module Configuration Switches
 ============================================================================*/

/* Development error detection */
#ifndef LINTRCV_DEV_ERROR_DETECT
#define LINTRCV_DEV_ERROR_DETECT             STD_ON
#endif

/* Version info API */
#ifndef LINTRCV_VERSION_INFO_API
#define LINTRCV_VERSION_INFO_API             STD_ON
#endif

/* Wake-up functionality support */
#ifndef LINTRCV_WAKEUP_SUPPORTED
#define LINTRCV_WAKEUP_SUPPORTED             STD_ON
#endif

/* Wake-up by bus support */
#ifndef LINTRCV_WAKEUP_BY_BUS_USED
#define LINTRCV_WAKEUP_BY_BUS_USED           STD_ON
#endif

/* Wake-up by pin support */
#ifndef LINTRCV_WAKEUP_BY_PIN_USED
#define LINTRCV_WAKEUP_BY_PIN_USED           STD_ON
#endif

/* SPI control interface support */
#ifndef LINTRCV_SPI_SUPPORT
#define LINTRCV_SPI_SUPPORT                  STD_OFF
#endif

/* I2C control interface support */
#ifndef LINTRCV_I2C_SUPPORT
#define LINTRCV_I2C_SUPPORT                  STD_OFF
#endif

/* TJA1021 specific support */
#ifndef LINTRCV_TJA1021_SUPPORT
#define LINTRCV_TJA1021_SUPPORT              STD_ON
#endif

/* TJA1022 specific support */
#ifndef LINTRCV_TJA1022_SUPPORT
#define LINTRCV_TJA1022_SUPPORT              STD_ON
#endif

/*=============================================================================
 * Channel Configuration
 ============================================================================*/

/* Number of configured LIN transceiver channels */
#ifndef LINTRCV_NUM_CHANNELS
#define LINTRCV_NUM_CHANNELS                 (2U)
#endif

/* Maximum number of channels supported */
#define LINTRCV_MAX_CHANNELS                 (4U)

/*=============================================================================
 * Channel IDs
 ============================================================================*/
#define LINTRCV_CHANNEL_0                    (0U)
#define LINTRCV_CHANNEL_1                    (1U)
#define LINTRCV_CHANNEL_2                    (2U)
#define LINTRCV_CHANNEL_3                    (3U)

/*=============================================================================
 * TJA1021 Pin Configuration
 * TJA1021 Pinout:
 * - Pin 1: INH   - Inhibit output (to power management)
 * - Pin 2: EN    - Enable input (control pin)
 * - Pin 3: GND   - Ground
 * - Pin 4: TXD   - Transmit data input (from LIN controller)
 * - Pin 5: RXD   - Receive data output (to LIN controller)
 * - Pin 6: NWake - Local wake-up input (active low)
 * - Pin 7: VIO   - Voltage input for I/O adaptation
 * - Pin 8: VBat  - Battery supply voltage
 * - Pin 9: NERR  - Error output (active low, open drain)
 * - Pin 10: LIN  - LIN bus line
 * - Pin 11: GND  - Ground
 * - Pin 12: SLP_N - Sleep control (alternative to EN for mode control)
 * 
 * Mode Control via EN pin:
 * - EN = 1: Normal Mode
 * - EN = 0: Standby Mode (if NWake = 1) or Sleep Mode (if NWake = 0)
 ============================================================================*/

/* DIO Channel IDs for TJA1021 control pins */
#ifndef DIO_CHANNEL_TJA1021_0_EN
#define DIO_CHANNEL_TJA1021_0_EN             (0U)  /* Enable pin - TJA1021 Pin 2 */
#endif

#ifndef DIO_CHANNEL_TJA1021_0_NWAKE
#define DIO_CHANNEL_TJA1021_0_NWAKE          (1U)  /* Wake-up pin - TJA1021 Pin 6 */
#endif

#ifndef DIO_CHANNEL_TJA1021_0_NERR
#define DIO_CHANNEL_TJA1021_0_NERR           (2U)  /* Error pin - TJA1021 Pin 9 */
#endif

#ifndef DIO_CHANNEL_TJA1021_0_TXD
#define DIO_CHANNEL_TJA1021_0_TXD            (3U)  /* TXD pin - TJA1021 Pin 4 */
#endif

#ifndef DIO_CHANNEL_TJA1021_0_RXD
#define DIO_CHANNEL_TJA1021_0_RXD            (4U)  /* RXD pin - TJA1021 Pin 5 */
#endif

/* Second channel (for TJA1022 dual transceiver) */
#ifndef DIO_CHANNEL_TJA1021_1_EN
#define DIO_CHANNEL_TJA1021_1_EN             (10U)
#endif

#ifndef DIO_CHANNEL_TJA1021_1_NWAKE
#define DIO_CHANNEL_TJA1021_1_NWAKE          (11U)
#endif

#ifndef DIO_CHANNEL_TJA1021_1_NERR
#define DIO_CHANNEL_TJA1021_1_NERR           (12U)
#endif

/*=============================================================================
 * EcuM Wake-up Sources
 ============================================================================*/

/* EcuM Wake-up source references */
#ifndef LINTRCV_WAKEUP_SOURCE_0
#define LINTRCV_WAKEUP_SOURCE_0              (0x00000010UL)  /* LIN Channel 0 */
#endif

#ifndef LINTRCV_WAKEUP_SOURCE_1
#define LINTRCV_WAKEUP_SOURCE_1              (0x00000020UL)  /* LIN Channel 1 */
#endif

#ifndef LINTRCV_WAKEUP_SOURCE_2
#define LINTRCV_WAKEUP_SOURCE_2              (0x00000040UL)  /* LIN Channel 2 */
#endif

#ifndef LINTRCV_WAKEUP_SOURCE_3
#define LINTRCV_WAKEUP_SOURCE_3              (0x00000080UL)  /* LIN Channel 3 */
#endif

/*=============================================================================
 * Mode Transition Delays (in microseconds)
 * Per TJA1021 datasheet specifications
 ============================================================================*/

/* TJA1021 Mode Transition Delays */
#define LINTRCV_TJA1021_SLEEP_TO_NORMAL_US   (1000U)   /* 1ms max per datasheet */
#define LINTRCV_TJA1021_STANDBY_TO_NORMAL_US (100U)    /* 100us typical */
#define LINTRCV_TJA1021_NORMAL_TO_STANDBY_US (50U)     /* 50us typical */
#define LINTRCV_TJA1021_NORMAL_TO_SLEEP_US   (50U)     /* 50us typical */

/* Wake-up detection debounce time */
#define LINTRCV_WAKEUP_DEBOUNCE_US           (50U)

/*=============================================================================
 * Hardware-Specific Configuration
 ============================================================================*/

/* TJA1021 Mode Control */
#define LINTRCV_TJA1021_EN_HIGH              STD_HIGH  /* Normal mode */
#define LINTRCV_TJA1021_EN_LOW               STD_LOW   /* Standby/Sleep mode */

/* TJA1021 NWake Pin (active low) */
#define LINTRCV_TJA1021_NWAKE_ACTIVE         STD_LOW
#define LINTRCV_TJA1021_NWAKE_INACTIVE       STD_HIGH

/* TJA1021 NERR Pin (active low, open drain) */
#define LINTRCV_TJA1021_NERR_ERROR           STD_LOW
#define LINTRCV_TJA1021_NERR_NO_ERROR        STD_HIGH

/*=============================================================================
 * Default Operation Mode Configuration
 ============================================================================*/

/* Default initial mode after initialization */
#define LINTRCV_DEFAULT_INITIAL_MODE         LINTRCV_OPMODE_NORMAL

/* Default wake-up settings */
#define LINTRCV_DEFAULT_WAKEUP_BY_BUS        STD_ON
#define LINTRCV_DEFAULT_WAKEUP_BY_PIN        STD_ON

/*=============================================================================
 * SPI Configuration (if SPI control is used)
 ============================================================================*/

/* SPI Channel for transceiver control */
#define LINTRCV_SPI_CHANNEL                  (0U)

/* SPI Device ID */
#define LINTRCV_SPI_DEVICE_ID                (0U)

/*=============================================================================
 * Callback Function Configuration
 ============================================================================*/

/* EcuM callback for wake-up notification */
extern void EcuM_SetWakeupEvent(uint32 wakeupSource);

/* Dio callback configuration */
#define LINTRCV_DIO_NOTIFICATION_API         STD_ON

/*=============================================================================
 * Memory Mapping
 ============================================================================*/
#define LINTRCV_START_SEC_CODE
#include "MemMap.h"

#define LINTRCV_STOP_SEC_CODE
#include "MemMap.h"

#define LINTRCV_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

#define LINTRCV_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

#define LINTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define LINTRCV_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define LINTRCV_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define LINTRCV_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#endif /* LINTRCV_CFG_H */