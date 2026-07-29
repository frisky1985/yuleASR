/*
 * Com_Types.h
 * AUTOSAR COM Module - Type Definitions
 * According to AUTOSAR SWS COM 4.4.0
 */

#ifndef COM_TYPES_H
#define COM_TYPES_H

#include "Std_Types.h"
#include "PduR.h"  /* For PduIdType and PduInfoType */

/*==================[Type Definitions]======================================*/

/* Signal ID type */
typedef uint16 Com_SignalIdType;

/* Signal Group ID type */
typedef uint16 Com_SignalGroupIdType;

/* I-PDU ID type */
typedef uint16 Com_IPduIdType;

/* I-PDU Group ID type */
typedef uint16 Com_IpduGroupIdType;

/* Signal Type enumeration */
typedef enum {
    COM_BOOLEAN,
    COM_UINT8,
    COM_UINT16,
    COM_UINT32,
    COM_UINT64,
    COM_SINT8,
    COM_SINT16,
    COM_SINT32,
    COM_SINT64,
    COM_FLOAT32,
    COM_FLOAT64,
    COM_UINT8_N,
    COM_UINT16_N,
    COM_UINT32_N,
    COM_UINT64_N
} Com_SignalTypeType;

/* Signal Endianness */
typedef enum {
    COM_LITTLE_ENDIAN,
    COM_BIG_ENDIAN,
    COM_OPAQUE
} Com_SignalEndiannessType;

/* Transfer Property */
typedef enum {
    COM_PENDING,
    COM_TRIGGERED,
    COM_TRIGGERED_ON_CHANGE,
    COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION,
    COM_TRIGGERED_WITHOUT_REPETITION
} Com_TransferPropertyType;

/* IPdu Direction */
typedef enum {
    COM_SEND,
    COM_RECEIVE
} Com_IPduDirectionType;

/* IPdu Type */
typedef enum {
    COM_NORMAL,
    COM_TP
} Com_IPduType;

/* IPdu Signal Processing */
typedef enum {
    COM_IMMEDIATE,
    COM_DEFERRED
} Com_IPduSignalProcessingType;

/* Transmission Mode */
typedef enum {
    COM_DIRECT,
    COM_MIXED,
    COM_NONE,
    COM_PERIODIC
} Com_TransferModeType;

/* IPdu Group Status */
typedef enum {
    COM_IPDU_GROUP_STOPPED = 0,
    COM_IPDU_GROUP_STARTED = 1
} Com_IpduGroupStatusType;

/* Module Status */
typedef enum {
    COM_UNINIT = 0,
    COM_READY = 1
} Com_StatusType;

/*==================[Configuration Types]===================================*/

/* Signal Configuration */
typedef struct {
    Com_SignalIdType SignalId;
    uint8* DataPtr;
    uint16 BitPosition;
    uint8 BitSize;
    Com_SignalEndiannessType Endianness;
    Com_SignalTypeType SignalType;
    Com_TransferPropertyType TransferProperty;
    void (*ComNotification)(void);
    uint32 Timeout;
    const void* InitValue;
} Com_SignalConfigType;

/* Signal Group Configuration */
typedef struct {
    Com_SignalGroupIdType SignalGroupId;
    Com_SignalIdType* SignalRefs;
    uint8 NumSignals;
    uint8* ShadowBuffer;
    void (*ComNotification)(void);
} Com_SignalGroupConfigType;

/* Transmission Mode Mode Enumeration */
typedef enum {
    COM_MODE_DIRECT = 0,
    COM_MODE_PERIODIC,
    COM_MODE_MIXED,
    COM_MODE_NONE
} ComTxModeModeType;

/* Transmission Mode Configuration (ComTxMode) */
typedef struct {
    ComTxModeModeType Mode;             /*!< DIRECT, PERIODIC, MIXED, NONE */
    uint32 CycleTime;                   /*!< Period between transmissions in ms */
    uint32 RepetitionPeriod;            /*!< Time between repetitions in ms */
    uint8 NumRepetitions;               /*!< Number of repetitions for direct transmission */
    uint32 TimeOffset;                  /*!< Initial time offset before first transmission */
    boolean RepeatingEnabled;           /*!< TRUE if repetitions are enabled */
} Com_TxModeType;

/* Legacy Tx Mode Configuration (for backward compatibility) */
typedef struct {
    Com_TransferModeType Mode;
    uint32 Period;
    uint32 RepetitionPeriod;
    uint8 NumRepetitions;
    uint32 TimeOffset;
} Com_TxModeConfigType;

/* Transmission Mode Condition (TMC) Configuration */
typedef struct {
    Com_SignalIdType SignalId;          /*!< Signal ID for TMC evaluation */
    uint32 ThresholdValue;              /*!< Threshold for comparison */
    boolean UseGreaterThan;             /*!< TRUE: signal > threshold, FALSE: signal < threshold */
    boolean IsConfigured;               /*!< TRUE if TMC is configured */
} Com_TmcConfigType;

/* Complete I-PDU Transmission Mode Configuration (ComTxModeTrue/ComTxModeFalse) */
typedef struct {
    Com_TxModeType TxModeTrue;          /*!< Configuration when TMC is TRUE */
    Com_TxModeType TxModeFalse;         /*!< Configuration when TMC is FALSE */
    Com_TmcConfigType TmcConfig;        /*!< TMC evaluation configuration */
    boolean UseTmc;                     /*!< TRUE if TMC-based switching is enabled */
} Com_IPduTxModeConfigType;

/* Transmission Confirmation Configuration */
typedef struct {
    boolean EnableConfirmation;         /*!< Enable transmission confirmation */
    uint32 TxTimeout;                   /*!< Transmission timeout in ms (ComTxTimeout) */
    uint8 MaxRetries;                   /*!< Maximum retry attempts (ComTxRetries) */
    void (*ComTxConfirmation)(void);    /*!< Success notification callback */
    void (*ComTxErrorNotification)(void); /*!< Error notification callback */
    void (*ComTxTimeoutNotification)(void); /*!< Timeout notification callback */
} Com_TxConfirmationConfigType;

/* IPdu Configuration */
typedef struct {
    Com_IPduIdType IPduId;
    uint8* DataPtr;
    uint8 Length;
    Com_IPduDirectionType Direction;
    Com_IPduType Type;
    Com_IPduSignalProcessingType SignalProcessing;
    Com_SignalIdType* SignalRefs;
    uint8 NumSignals;
    Com_SignalGroupIdType* SignalGroupRefs;
    uint8 NumSignalGroups;
    Com_IPduTxModeConfigType TxMode;    /*!< Full transmission mode configuration (ComTxModeTrue/False) */
    Com_IpduGroupIdType* IpduGroupRefs;
    uint8 NumIpduGroups;
    uint32 Timeout;
    void (*ComIPduCallout)(PduIdType PduId, PduInfoType* PduInfoPtr);
    Com_TxConfirmationConfigType TxConfirmation; /*!< Transmission confirmation config */
} Com_IPduConfigType;

/* IPdu Group Configuration */
typedef struct {
    Com_IpduGroupIdType IpduGroupId;
    Com_IPduIdType* IPduRefs;
    uint8 NumIPdus;
} Com_IPduGroupConfigType;

/* Global Configuration */
typedef struct {
    const Com_SignalConfigType* Signals;
    uint16 NumSignals;
    const Com_SignalGroupConfigType* SignalGroups;
    uint16 NumSignalGroups;
    const Com_IPduConfigType* IPdus;
    uint16 NumIPdus;
    const Com_IPduGroupConfigType* IPduGroups;
    uint16 NumIPduGroups;
} Com_ConfigType;

/*==================[Return Types]=========================================*/

#ifndef COM_SERVICE_NOT_AVAILABLE
#define COM_SERVICE_NOT_AVAILABLE 0x80
#endif

#ifndef COM_BUSY
#define COM_BUSY 0x81
#endif

/*==================[Deadline Monitoring Types]============================*/
/* T012: Deadline Monitoring Type Definitions */

/**
 * @brief Deadline Monitoring State Enumeration
 * @req SWS_Com_00500
 */
typedef enum {
    COM_DM_STATE_STOPPED = 0,       /*!< Monitoring stopped */
    COM_DM_STATE_RUNNING,           /*!< Timer running */
    COM_DM_STATE_EXPIRED,           /*!< Timeout occurred */
    COM_DM_STATE_ERROR              /*!< Error condition */
} Com_DmStateType;

/**
 * @brief Deadline Monitoring Action Type
 * Defines action on timeout
 */
typedef enum {
    COM_DM_ACTION_NONE = 0,         /*!< No action */
    COM_DM_ACTION_ERROR_HOOK,       /*!< Call ComErrorHook only */
    COM_DM_ACTION_DEFAULT_VALUE,    /*!< Substitute default value */
    COM_DM_ACTION_BOTH              /*!< Both hook and default value */
} Com_DmActionType;

/**
 * @brief Rx Deadline Monitoring Configuration
 * @req SWS_Com_00500: ComIPduRxTimeout configuration
 */
typedef struct {
    uint32 ComIPduRxTimeout;                    /*!< Rx timeout in ms */
    const uint8* ComIPduRxDefaultValue;         /*!< Default value buffer */
    uint8 DefaultValueLength;                   /*!< Length of default value */
    Com_DmActionType TimeoutAction;             /*!< Action on timeout */
    void (*ComErrorHook)(Com_IPduIdType PduId); /*!< Error notification callback */
    boolean EnableDeadlineMonitoring;           /*!< Enable/disable flag */
} Com_DmRxConfigType;

#endif /* COM_TYPES_H */
