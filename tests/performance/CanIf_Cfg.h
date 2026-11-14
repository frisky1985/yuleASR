/**
 * @file CanIf_Cfg.h
 * @brief TEST-PRIVATE shadow of the production CanIf pre-compile configuration
 *        (benchmark dimension), used ONLY by the P1 Phase 8 communication-stack
 *        benchmark (tests/performance/test_comstack_benchmark.c).
 *
 * WHY THIS FILE EXISTS (test-side only; src/ is NOT modified):
 *   CanIf.c bounds every Rx-side static array and loop with the compile-time
 *   macro CANIF_NUM_RX_PDUS (production value 4U): the Rx notification-status
 *   table, the Hoh-bucketed Rx lookup table (CanIf_RxLookup) and the Init /
 *   dispatch loops all use i < CANIF_NUM_RX_PDUS and index
 *   ConfigPtr->RxPdus[i]. Benchmarking CanIf_RxIndication dispatch latency
 *   "at different PDU counts" therefore requires a pre-compile configuration
 *   with a larger Rx table. This header is force-included (-include) into every
 *   translation unit of comstack_benchmark BEFORE any production header, so its
 *   include guard (CANIF_CFG_H) neuters the production
 *   src/bsw/ecual/canif/include/CanIf_Cfg.h pulled in via CanIf.h.
 *
 *   The benchmark runtime configuration then provides the full 64-entry Rx
 *   table (CANIF_NUM_RX_PDUS == 64U) and dispatch cost is measured as a
 *   function of the match position within the Hoh bucket (entry 0 vs entry 31
 *   vs entry 63 vs miss), i.e. the number of table entries scanned.
 *
 *   All other values are identical to production.
 */

#ifndef CANIF_CFG_H
#define CANIF_CFG_H

#include "Std_Types.h"   /* 非宏段依赖 Std_Types 基础类型 (uint32/uint8/uint16/boolean 等)，生成宏段不含此 include */

#include "Std_Types.h"

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
*================================================================================================*/

/*==================================================================================================
*                                    General Configuration
*================================================================================================*/
#define CANIF_DEV_ERROR_DETECT    STD_ON
#define CANIF_VERSION_INFO_API    STD_ON

/*==================================================================================================
*                                    Module Configuration Counts
*================================================================================================*/
#define CANIF_NUM_CONTROLLERS    (1U)
#define CANIF_NUM_TRANSCEIVERS    (1U)
#define CANIF_NUM_TX_PDUS    (4U)
/* TEST SHADOW VALUE: production uses (4U); the Rx dispatch benchmark needs a
 * 64-entry Rx table to expose the O(n) scan cost inside the Hoh bucket. */
#define CANIF_NUM_RX_PDUS    (64U)

/*==================================================================================================
*                                    Baudrate Configurations
*================================================================================================*/
#define CANIF_DEFAULT_BAUDRATE    (500U)

/*==================================================================================================
*                                    Controller Definitions
*================================================================================================*/
#define CANIF_CONTROLLER_CNT    (1U)
#define CANIF_CONTROLLER_0    (0U)

/*==================================================================================================
*                                    Hardware Object Handles
*================================================================================================*/
#define CANIF_HOH_CNT    (4U)
#define CANIF_TX_LPDU_CNT    (4U)
/* TEST SHADOW VALUE: kept equal to CANIF_NUM_RX_PDUS, same reason as above. */
#define CANIF_RX_LPDU_CNT    (64U)
#define CANIF_RX_INDICATION    STD_ON
#define CANIF_TX_CONFIRMATION    STD_ON
#define CANIF_TX_LPDU_0    (0U)
#define CANIF_TX_LPDU_1    (1U)
#define CANIF_TX_LPDU_2    (2U)
#define CANIF_TX_LPDU_3    (3U)
#define CANIF_RX_LPDU_0    (4U)
#define CANIF_RX_LPDU_1    (5U)
#define CANIF_RX_LPDU_2    (6U)
#define CANIF_RX_LPDU_3    (7U)

/*==================================================================================================
*                                    Other Configuration
*================================================================================================*/
#define CANIF_HTH_CNT    (2U)
/* TEST SHADOW VALUE: 4 Tx + 64 Rx L-PDUs. */
#define CANIF_LPDU_CNT    (68U)
#define CANIF_TRANSMIT_CANCELLATION    STD_OFF
#define CANIF_WAKEUP_SUPPORT    STD_ON
#define CANIF_HTH_0    (0U)
#define CANIF_HTH_1    (1U)
#define CANIF_HRH_0    (2U)
#define CANIF_HRH_1    (3U)
#define CANIF_E_PARAM_CANID    (1U)
#define CANIF_E_PARAM_DLC    (2U)


/*==================================================================================================
*  NON-MACRO SEGMENT (preserved from handwritten header, merged by codegen splice)
*  typedef(7) + struct(4) + extern config tables(5) — 依赖宏段计数宏, 故置于宏段之后
*================================================================================================*/
typedef uint32 CanIf_CanIdType;
typedef uint32 CanIf_CanIdTypeType;

/* Hardware Object Handle type */
typedef uint8 CanIf_HohType;

/* Hardware Transmit Handle type */
typedef uint8 CanIf_HthType;

/* L-PDU ID type */
typedef uint16 CanIf_PduIdType;

/* HOH configuration type */
typedef struct
{
    uint8 controllerId;      /* Associated CAN controller */
    boolean isTx;            /* TRUE = Tx HOH, FALSE = Rx HOH */
    uint8 driverObjId;       /* Driver-specific object ID */
} CanIf_HohCfgType;

/* Tx L-PDU configuration type */
typedef struct
{
    CanIf_PduIdType pduId;           /* L-PDU ID */
    CanIf_CanIdType canId;           /* CAN Identifier */
    CanIf_HthType hthId;             /* Associated HTH */
    uint8 controllerId;              /* Associated controller */
    uint8 dlc;                       /* Data Length Code (0-8) */
} CanIf_TxPduCfgType;

/* Rx L-PDU configuration type */
typedef struct
{
    CanIf_PduIdType pduId;           /* L-PDU ID */
    CanIf_CanIdType canId;           /* CAN Identifier */
    CanIf_CanIdType canIdMask;       /* CAN ID mask for filtering */
    CanIf_HohType hohId;             /* Associated HRH */
    uint8 controllerId;              /* Associated controller */
    uint8 dlc;                       /* Data Length Code (0-8) */
} CanIf_RxPduCfgType;

/* Controller Mode type (redefined as enum in CanIf.h) */
typedef uint8 CanIf_ControllerModeType;
typedef uint8 CanIf_PduModeType;

/* Controller configuration type */
typedef struct
{
    uint8 controllerId;              /* Controller ID */
    CanIf_ControllerModeType initMode; /* Initial mode */
} CanIf_ControllerCfgType;

/*=============================================================================
 * External Configuration References (defined in CanIf_Lcfg.c)
 *=============================================================================*/

extern const CanIf_HohCfgType CanIf_HohCfg[CANIF_HOH_CNT];
extern const CanIf_TxPduCfgType CanIf_TxPduCfg[CANIF_TX_LPDU_CNT];
extern const CanIf_RxPduCfgType CanIf_RxPduCfg[CANIF_RX_LPDU_CNT];
extern const CanIf_ControllerCfgType CanIf_ControllerCfg[CANIF_CONTROLLER_CNT];

/* Rx L-PDU to HOH mapping table (for fast lookup) */
extern const CanIf_PduIdType CanIf_RxPduHohMap[CANIF_HOH_CNT][CANIF_RX_LPDU_CNT];
#endif /* CANIF_CFG_H */

/*==================[end of file]===========================================*/
