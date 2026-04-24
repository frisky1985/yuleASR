/******************************************************************************
 * @file    autosar_errors.h
 * @brief   AUTOSAR Error Codes Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef AUTOSAR_ERRORS_H
#define AUTOSAR_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Standard Error Codes (from AUTOSAR_SWS_StandardTypes)
 ******************************************************************************/

/* Standard return codes */
#ifndef E_OK
#define E_OK            0x00U
#endif

#ifndef E_NOT_OK
#define E_NOT_OK        0x01U
#endif

/* Extended error codes */
#define E_NO_DTC_AVAILABLE      0x02U
#define E_SESSION_NOT_ALLOWED   0x03U
#define E_PROTOCOL_NOT_ALLOWED  0x04U
#define E_REQUEST_NOT_ACCEPTED  0x05U
#define E_RESPONSE_NOT_OK       0x06U
#define E_RESET_NOT_ACCEPTED    0x07U
#define E_PAGE_BUFFER_OVERFLOW  0x08U
#define E_PENDING               0x10U
#define E_COMPARE_KEY_FAILED    0x11U
#define E_IDLE                  0x12U

/******************************************************************************
 * Communication Error Codes
 ******************************************************************************/

#define E_COMMS_INIT_FAILED         0x20U
#define E_COMMS_NOT_INITIALIZED     0x21U
#define E_COMMS_BUS_OFF             0x22U
#define E_COMMS_TIMEOUT             0x23U
#define E_COMMS_CRC_ERROR           0x24U
#define E_COMMS_SEQUENCE_ERROR      0x25U
#define E_COMMS_DATA_INVALID        0x26U
#define E_COMMS_BUFFER_OVERFLOW     0x27U
#define E_COMMS_FRAME_ERROR         0x28U
#define E_COMMS_PARITY_ERROR        0x29U
#define E_COMMS_OVERRUN             0x2AU

/******************************************************************************
 * E2E Protection Error Codes
 ******************************************************************************/

#define E2E_E_OK                    0x00U
#define E2E_E_INPUTERR_WRONG        0x01U
#define E2E_E_INPUTERR_NULL         0x02U
#define E2E_E_INTERR                0x03U
#define E2E_E_WRONGSTATE            0x04U
#define E2E_E_OK_SOMELOST           0x05U

/* E2E check status flags */
#define E2E_STATUS_NONEWDATA        0x01U
#define E2E_STATUS_WRONGCRC         0x02U
#define E2E_STATUS_WRONGSEQUENCE    0x04U
#define E2E_STATUS_REPEATED         0x08U
#define E2E_STATUS_OK               0x00U

/******************************************************************************
 * RTE Error Codes
 ******************************************************************************/

#define RTE_E_OK                    0x00U
#define RTE_E_NOK                   0x01U
#define RTE_E_INVALID               0x02U
#define RTE_E_COMMS_ERROR           0x03U
#define RTE_E_TIMEOUT               0x04U
#define RTE_E_LIMIT                 0x05U
#define RTE_E_NO_DATA               0x06U
#define RTE_E_UNCONNECTED           0x07U
#define RTE_E_E2E_ERROR             0x08U
#define RTE_E_HARD_TRANSFORMER_ERROR    0x09U
#define RTE_E_SOFT_TRANSFORMER_ERROR    0x0AU
#define RTE_E_COM_STOPPED           0x0BU
#define RTE_E_LOST_DATA             0x0CU
#define RTE_E_MAX_AGE_EXCEEDED      0x0DU

/******************************************************************************
 * ara::com Error Codes
 ******************************************************************************/

#define ARA_COM_E_OK                0x00U
#define ARA_COM_E_NOT_OK            0x01U
#define ARA_COM_E_INVALID           0x02U
#define ARA_COM_E_COMMUNICATION_ERROR   0x03U
#define ARA_COM_E_TIMEOUT           0x04U
#define ARA_COM_E_SERVICE_NOT_AVAILABLE 0x05U
#define ARA_COM_E_SUBSCRIPTION_REFUSED  0x06U
#define ARA_COM_E_E2E_ERROR         0x07U
#define ARA_COM_E_BUFFER_OVERFLOW   0x08U

/******************************************************************************
 * Memory Error Codes
 ******************************************************************************/

#define E_MEM_OK                    0x00U
#define E_MEM_ERROR                 0x30U
#define E_MEM_OUT_OF_MEMORY         0x31U
#define E_MEM_ACCESS_VIOLATION      0x32U
#define E_MEM_ALIGNMENT_ERROR       0x33U
#define E_MEM_DOUBLE_FREE           0x34U
#define E_MEM_BUFFER_OVERFLOW       0x35U

/******************************************************************************
 * OS Error Codes
 ******************************************************************************/

#define E_OS_OK                     0x00U
#define E_OS_ACCESS                 0x40U
#define E_OS_CALLEVEL               0x41U
#define E_OS_ID                     0x42U
#define E_OS_LIMIT                  0x43U
#define E_OS_NOFUNC                 0x44U
#define E_OS_RESOURCE               0x45U
#define E_OS_STATE                  0x46U
#define E_OS_VALUE                  0x47U
#define E_OS_SERVICE_NOT_FOUND      0x48U
#define E_OS_STACK_OVERFLOW         0x49U

/******************************************************************************
 * BSW Error Codes
 ******************************************************************************/

#define E_BSWM_OK                   0x00U
#define E_BSWM_ERROR                0x50U
#define E_BSWM_INVALID_MODE         0x51U
#define E_BSWM_INVALID_REQUEST      0x52U

/******************************************************************************
 * Safety Error Codes
 ******************************************************************************/

#define E_SAFETY_OK                 0x00U
#define E_SAFETY_ERROR              0x60U
#define E_SAFETY_WATCHDOG_ERROR     0x61U
#define E_SAFETY_CLOCK_ERROR        0x62U
#define E_SAFETY_ALU_ERROR          0x63U
#define E_SAFETY_MEMORY_ERROR       0x64U
#define E_SAFETY_REGISTER_ERROR     0x65U
#define E_SAFETY_INTERRUPTS_DISABLED    0x66U
#define E_SAFETY_TIMING_ERROR       0x67U

/******************************************************************************
 * Diagnostic Error Codes
 ******************************************************************************/

#define E_DIAG_OK                   0x00U
#define E_DIAG_NOT_OK               0x70U
#define E_DIAG_DTC_NOT_FOUND        0x71U
#define E_DIAG_SESSION_NOT_SUPPORTED    0x72U
#define E_DIAG_SECURITY_ACCESS_DENIED   0x73U
#define E_DIAG_REQUEST_OUT_OF_RANGE 0x74U
#define E_DIAG_BUSY_REPEAT_REQUEST  0x75U
#define E_DIAG_CONDITIONS_NOT_CORRECT   0x76U

/******************************************************************************
 * Debug and Trace Error Codes
 ******************************************************************************/

#define E_DBG_OK                    0x00U
#define E_DBG_ERROR                 0x80U
#define E_DBG_BUFFER_FULL           0x81U
#define E_DBG_NOT_INITIALIZED       0x82U
#define E_DBG_TIMESTAMP_ERROR       0x83U

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAR_ERRORS_H */
