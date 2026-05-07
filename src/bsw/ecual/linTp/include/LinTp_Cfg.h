/*
 * LinTp_Cfg.h
 *
 *  Created on: May 5, 2026
 *      Author: YuleTech
 *
 *  AUTOSAR LinTp (LIN Transport Protocol) Configuration Header
 *  Based on ISO 17987-2 Transport Protocol
 *  Following AUTOSAR_SWS_LINTransportProtocol
 */

#ifndef LINTP_CFG_H
#define LINTP_CFG_H

/*==================================================================================================
 *                                         VERSION INFORMATION
 *================================================================================================*/
#define LINTP_CFG_VENDOR_ID                     0x0099U /* YuleTech Vendor ID */
#define LINTP_CFG_MODULE_ID                     0x0062U /* LinTp Module ID */

#define LINTP_CFG_SW_MAJOR_VERSION              1U
#define LINTP_CFG_SW_MINOR_VERSION              0U
#define LINTP_CFG_SW_PATCH_VERSION              0U

#define LINTP_CFG_AR_RELEASE_MAJOR_VERSION      4U
#define LINTP_CFG_AR_RELEASE_MINOR_VERSION      4U
#define LINTP_CFG_AR_RELEASE_REVISION_VERSION   0U

/*==================================================================================================
 *                                         CONFIGURATION SWITCHES
 *================================================================================================*/
/* Development Error Detection */
#define LINTP_DEV_ERROR_DETECT                  STD_ON

/* Version Info API */
#define LINTP_VERSION_INFO_API                  STD_ON

/* Support for cancel transmit/receive operations */
#define LINTP_CANCEL_TRANSMIT_API               STD_ON
#define LINTP_CANCEL_RECEIVE_API                STD_ON

/* Support for changing protocol parameters */
#define LINTP_CHANGE_PARAMETER_API              STD_ON

/* Support for FC with wait frames */
#define LINTP_SUPPORT_WAIT_FRAMES               STD_ON

/* Extended addressing support */
#define LINTP_EXTENDED_ADDRESSING_SUPPORT       STD_OFF

/*==================================================================================================
 *                                         CHANNEL CONFIGURATION
 *================================================================================================*/
/* Maximum number of LinTp channels */
#define LINTP_MAX_CHANNEL_COUNT                 2U

/* Number of NSC (Network Service Connection) configurations */
#define LINTP_NSDU_COUNT                        4U

/* Number of Rx NSC connections */
#define LINTP_RX_NSDU_COUNT                     2U

/* Number of Tx NSC connections */
#define LINTP_TX_NSDU_COUNT                     2U

/*==================================================================================================
 *                                         TIMING PARAMETERS (in ms)
 *  Following ISO 17987-2 specification
 *================================================================================================*/
/* N_As: Sender response timeout - Maximum time for transmission of a frame */
#define LINTP_NAS_DEFAULT                       1000U

/* N_Cs: Sender confirmation timeout - Time until next CF transmission */
#define LINTP_NCS_DEFAULT                       1000U

/* N_Cr: Receiver confirmation timeout - Time until reception of next CF */
#define LINTP_NCR_DEFAULT                       1000U

/* Minimum separation time between consecutive frames (STmin) - in ms */
#define LINTP_STMIN_DEFAULT                     20U

/* Block Size - Number of CF frames before FC is expected */
#define LINTP_BS_DEFAULT                        8U

/* Maximum Wait Frame transmissions before abort */
#define LINTP_MAX_WFT                           10U

/*==================================================================================================
 *                                         BUFFER CONFIGURATION
 *================================================================================================*/
/* Maximum buffer size for TP messages */
#define LINTP_BUFFER_SIZE                       4095U

/* Maximum single frame data length (6 bytes for LIN with PCI) */
#define LINTP_SF_MAX_DATA_LENGTH                6U

/* Maximum first frame data length (5 bytes for LIN with PCI) */
#define LINTP_FF_MAX_DATA_LENGTH                5U

/* Maximum consecutive frame data length (6 bytes for LIN with PCI) */
#define LINTP_CF_MAX_DATA_LENGTH                6U

/*==================================================================================================
 *                                         STATE TIMEOUTS (in MainFunction cycles)
 *================================================================================================*/
/* Timeout for SF transmission/reception */
#define LINTP_SF_TIMEOUT_CYCLES                 100U

/* Timeout for FF reception confirmation */
#define LINTP_FF_TIMEOUT_CYCLES                 100U

/* Timeout for CF transmission/reception */
#define LINTP_CF_TIMEOUT_CYCLES                 100U

/* Timeout for FC reception/transmission */
#define LINTP_FC_TIMEOUT_CYCLES                 100U

/*==================================================================================================
 *                                         ERROR CODES
 *================================================================================================*/
/* AUTOSAR defined error codes */
#define LINTP_E_UNINIT                          0x00U /* API service called before initialization */
#define LINTP_E_INVALID_PDU_SDU_ID              0x01U /* Invalid PDU/SDU ID */
#define LINTP_E_PARAM_POINTER                   0x02U /* API called with NULL pointer */
#define LINTP_E_INVALID_PARAMETER               0x03U /* Invalid parameter value */
#define LINTP_E_INIT_FAILED                     0x04U /* Initialization failed */
#define LINTP_E_INVALID_TX_ID                   0x05U /* Invalid Tx N-SDU ID */
#define LINTP_E_INVALID_RX_ID                   0x06U /* Invalid Rx N-SDU ID */
#define LINTP_E_INVALD_NSDU_ID                  0x20U /* Invalid N-SDU ID */
#define LINTP_E_INVALD_NSA                      0x30U /* Invalid Network Source Address */

/* Runtime error codes */
#define LINTP_E_TIMEOUT                         0x70U /* Timeout occurred */
#define LINTP_E_INVALID_FRAME                   0x71U /* Invalid frame received */
#define LINTP_E_BUFFER_OVERFLOW                 0x72U /* Buffer overflow */
#define LINTP_E_SEQUENCE_ERROR                  0x73U /* Sequence number error */
#define LINTP_E_INVALID_FC                      0x74U /* Invalid Flow Control frame */

/*==================================================================================================
 *                                         SERVICE IDs
 *================================================================================================*/
#define LINTP_SID_INIT                          0x01U
#define LINTP_SID_DEINIT                        0x02U
#define LINTP_SID_TRANSMIT                      0x03U
#define LINTP_SID_CANCEL_TRANSMIT               0x04U
#define LINTP_SID_CANCEL_RECEIVE                0x05U
#define LINTP_SID_CHANGE_PARAMETER              0x06U
#define LINTP_SID_GET_VERSION_INFO              0x07U
#define LINTP_SID_MAIN_FUNCTION                 0x08U
#define LINTP_SID_RX_INDICATION                 0x42U
#define LINTP_SID_TX_CONFIRMATION               0x43U
#define LINTP_SID_TRIGGER_TRANSMIT              0x44U

#endif /* LINTP_CFG_H */
