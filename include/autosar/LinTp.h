/*
 * LinTp.h - AUTOSAR LinTp types stub
 *
 * Provides the type definitions needed by LinTp service modules.
 * When the real LinTp.h from the BSW module's include directory is
 * found first (via -I ordering), this stub is not used.
 */
#ifndef LINTP_H
#define LINTP_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "MemMap.h"

/* Module IDs */
#define LINTP_VENDOR_ID                     (0x01U)
#define LINTP_MODULE_ID                     (0x90U)
#define LINTP_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define LINTP_AR_RELEASE_MINOR_VERSION      (0x04U)
#define LINTP_AR_RELEASE_REVISION_VERSION   (0x00U)
#define LINTP_SW_MAJOR_VERSION              (0x01U)
#define LINTP_SW_MINOR_VERSION              (0x00U)
#define LINTP_SW_PATCH_VERSION              (0x00U)

/* Service IDs */
#define LINTP_SID_INIT                      (0x01U)
#define LINTP_SID_DEINIT                    (0x02U)
#define LINTP_SID_GET_VERSION_INFO          (0x03U)
#define LINTP_SID_TRANSMIT                  (0x49U)
#define LINTP_SID_CANCEL_RECEIVE            (0x4CU)
#define LINTP_SID_CANCEL_TRANSMIT           (0x4DU)
#define LINTP_SID_CHANGE_PARAMETER          (0x4BU)
#define LINTP_SID_MAIN_FUNCTION             (0x06U)
#define LINTP_SID_RX_INDICATION             (0x42U)
#define LINTP_SID_TX_CONFIRMATION           (0x40U)

/* Error codes */
#define LINTP_E_NOT_INITIALIZED             (0x01U)
#define LINTP_E_INVALID_PARAMETER           (0x02U)
#define LINTP_E_INVALID_POINTER             (0x03U)
#define LINTP_E_INVALID_PDU_SDU_ID          (0x04U)
#define LINTP_E_PARAM_CONFIG                (0x05U)

/* PCI types */
#define LINTP_PCI_TYPE_SF                   (0x00U)
#define LINTP_PCI_TYPE_FF                   (0x01U)
#define LINTP_PCI_TYPE_CF                   (0x02U)

/* Basic types */
typedef uint8  LinTp_ChannelType;
typedef uint8  LinTp_ConnectionType;
typedef uint8  LinTp_NADType;

/* State type */
typedef enum {
    LINTP_STATE_UNINIT = 0,
    LINTP_STATE_IDLE,
    LINTP_STATE_SF_TRANSMIT,
    LINTP_STATE_FF_RECEIVE,
    LINTP_STATE_CF_RECEIVE,
    LINTP_STATE_CF_TRANSMIT,
    LINTP_STATE_WAIT_FIRST_FRAME,
    LINTP_STATE_WAIT_CONSECUTIVE_FRAME,
    LINTP_STATE_TRANSMIT_COMPLETED,
    LINTP_STATE_RECEIVE_COMPLETED,
    LINTP_STATE_ERROR,
    LINTP_STATE_TX_READY,
    LINTP_STATE_TX_BUSY,
    LINTP_STATE_WAIT_FC,
    LINTP_STATE_WAIT_STMIN,
    LINTP_STATE_RX_READY,
    LINTP_STATE_RX_BUSY
} LinTp_StateType;

/* Config and channel types - must match the real implementation */
typedef struct LinTp_ConnectionConfigType LinTp_ConnectionConfigType;

struct LinTp_ConnectionConfigType {
    uint8  NAD;
    uint8  STmin;
    uint16 N_As;
    uint16 N_Cr;
};

typedef struct {
    const LinTp_ConnectionConfigType* Connections;
    uint16 N_As;
    uint16 N_Cr;
    uint8  STmin;
    uint8  NumConnections;
} LinTp_ChannelConfigType;

typedef struct {
    const LinTp_ChannelConfigType* ChannelConfig;
    uint8 NumChannels;
    uint8 NumConnections;
} LinTp_ConfigType;

/* Default configuration values (from LinTp_Cfg.h) */
#ifndef LINTP_NUMBER_OF_CHANNELS
#define LINTP_NUMBER_OF_CHANNELS            (1U)
#endif
#ifndef LINTP_NUMBER_OF_CONNECTIONS
#define LINTP_NUMBER_OF_CONNECTIONS         (2U)
#endif
#ifndef LINTP_NUMBER_OF_PDUS
#define LINTP_NUMBER_OF_PDUS                (4U)
#endif
#ifndef LINTP_FRAME_SIZE
#define LINTP_FRAME_SIZE                    (8U)
#endif
#ifndef LINTP_SF_MAX_DATA_LENGTH
#define LINTP_SF_MAX_DATA_LENGTH            (6U)
#endif
#ifndef LINTP_FF_DATA_LENGTH
#define LINTP_FF_DATA_LENGTH                (5U)
#endif
#ifndef LINTP_CF_DATA_LENGTH
#define LINTP_CF_DATA_LENGTH                (6U)
#endif
#ifndef LINTP_PCI_SIZE
#define LINTP_PCI_SIZE                      (1U)
#endif
#ifndef LINTP_PCI_MASK
#define LINTP_PCI_MASK                      (0xF0U)
#endif
#ifndef LINTP_PCI_SF
#define LINTP_PCI_SF                        (0x00U)
#endif
#ifndef LINTP_PCI_FF
#define LINTP_PCI_FF                        (0x10U)
#endif
#ifndef LINTP_PCI_CF
#define LINTP_PCI_CF                        (0x20U)
#endif
#ifndef LINTP_PCI_TYPE_MASK
#define LINTP_PCI_TYPE_MASK                 (0xF0U)
#endif
#ifndef LINTP_PCI_DL_MASK
#define LINTP_PCI_DL_MASK                   (0x0FU)
#endif
#ifndef LINTP_PCI_SN_MASK
#define LINTP_PCI_SN_MASK                   (0x0FU)
#endif
#ifndef LINTP_SN_MAX
#define LINTP_SN_MAX                        (0x0FU)
#endif
#ifndef LINTP_SN_FIRST_CF
#define LINTP_SN_FIRST_CF                   (0x01U)
#endif
#ifndef LINTP_MAX_MESSAGE_LENGTH
#define LINTP_MAX_MESSAGE_LENGTH            (4095U)
#endif

#endif /* LINTP_H */
