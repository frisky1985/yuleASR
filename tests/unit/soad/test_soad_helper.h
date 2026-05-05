/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : SoAd Unit Test Helper
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-01
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TEST_SOAD_HELPER_H
#define TEST_SOAD_HELPER_H

#include "test_framework.h"
#include "SoAd.h"
#include "SoAd_Cfg.h"
#include "TcpIp.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST CONFIGURATION
==================================================================================================*/
#define SOAD_TEST_NUM_SOCKETS           (4U)
#define SOAD_TEST_NUM_CONNECTIONS       (4U)
#define SOAD_TEST_NUM_PDU_ROUTES        (8U)

#define SOAD_TEST_CONN_ID_0             (0U)
#define SOAD_TEST_CONN_ID_1             (1U)
#define SOAD_TEST_CONN_ID_2             (2U)
#define SOAD_TEST_CONN_ID_3             (3U)

#define SOAD_TEST_PDU_ID_0              (0U)
#define SOAD_TEST_PDU_ID_1              (1U)
#define SOAD_TEST_SOCKET_ID_INVALID     (0xFFU)

/*==================================================================================================
*                                      TEST DATA STRUCTURES
==================================================================================================*/
typedef struct {
    uint16 SoConId;
    SoAd_ConnStateType State;
    TcpIp_SocketIdType SocketId;
    boolean IsValid;
} SoAd_TestConnectionType;

typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint16 SoConId;
    boolean IsValid;
} SoAd_TestPduRouteType;

/*==================================================================================================
*                                      MOCK FUNCTION DECLARATIONS
==================================================================================================*/
void mock_SoAd_Reset(void);
void mock_TcpIp_SetCreateResult(TcpIp_ReturnType result);
void mock_TcpIp_SetBindResult(TcpIp_ReturnType result);
void mock_TcpIp_SetSendResult(TcpIp_ReturnType result);
void mock_TcpIp_SetCloseResult(TcpIp_ReturnType result);
Std_ReturnType mock_SoAd_GetConnectionState(uint16 SoConId, SoAd_ConnStateType* State);

#endif /* TEST_SOAD_HELPER_H */
