/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : PduR Unit Test Helper
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-01
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TEST_PDUR_HELPER_H
#define TEST_PDUR_HELPER_H

#include "test_framework.h"
#include "PduR.h"
#include "PduR_Cfg.h"
#include "mock_det.h"
#include "mock_ecual.h"

/*==================================================================================================
*                                      TEST CONFIGURATION
==================================================================================================*/
#define PDUR_TEST_NUM_ROUTING_PATHS     (4U)
#define PDUR_TEST_NUM_DEST_PDU          (2U)
#define PDUR_TEST_PDU_ID_COM_TX         ((PduIdType)0U)
#define PDUR_TEST_PDU_ID_COM_RX         ((PduIdType)1U)
#define PDUR_TEST_PDU_ID_DCM_TX         ((PduIdType)2U)
#define PDUR_TEST_PDU_ID_DCM_RX         ((PduIdType)3U)

/*==================================================================================================
*                                      TEST DATA STRUCTURES
==================================================================================================*/
typedef struct {
    uint8 SrcModule;
    uint8 DestModule;
    PduIdType SrcPduId;
    PduIdType DestPduId;
    boolean IsEnabled;
} PduR_TestRoutingPathType;

typedef struct {
    uint8 GroupId;
    boolean IsEnabled;
    uint8 NumPaths;
    uint8 PathIds[4];
} PduR_TestRoutingGroupType;

/*==================================================================================================
*                                      MOCK FUNCTION DECLARATIONS
==================================================================================================*/
void mock_PduR_Reset(void);
void mock_CanIf_SetTransmitResult(Std_ReturnType result);
void mock_Com_SetRxIndicationResult(Std_ReturnType result);
void mock_Dcm_SetRxIndicationResult(Std_ReturnType result);

/*==================================================================================================
*                                      MOCK IMPLEMENTATION
==================================================================================================*/
static inline void mock_PduR_Reset(void) { }
static inline void mock_CanIf_SetTransmitResult(Std_ReturnType result) { (void)result; }
static inline void mock_Com_SetRxIndicationResult(Std_ReturnType result) { (void)result; }
static inline void mock_Dcm_SetRxIndicationResult(Std_ReturnType result) { (void)result; }

#endif /* TEST_PDUR_HELPER_H */
