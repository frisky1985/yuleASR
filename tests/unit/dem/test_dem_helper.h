/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DEM Unit Test Helper
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-01
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TEST_DEM_HELPER_H
#define TEST_DEM_HELPER_H

#include "test_framework.h"
#include "Dem.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST CONFIGURATION
==================================================================================================*/
#define DEM_TEST_EVENT_ID_1             ((Dem_EventIdType)0x0001)
#define DEM_TEST_EVENT_ID_2             ((Dem_EventIdType)0x0002)
#define DEM_TEST_DTC_ID_1               ((uint32)0x123456)
#define DEM_TEST_DEBOUNCE_TIME          (100U)

/*==================================================================================================
*                                      TEST DATA TYPES
==================================================================================================*/
typedef uint16 Dem_EventIdType;

typedef enum {
    DEM_EVENT_STATUS_PASSED = 0,
    DEM_EVENT_STATUS_FAILED,
    DEM_EVENT_STATUS_PREPASSED,
    DEM_EVENT_STATUS_PREFAILED
} Dem_EventStatusType;

typedef enum {
    DEM_UDS_STATUS_TF = 0x01,
    DEM_UDS_STATUS_TFTOC = 0x02,
    DEM_UDS_STATUS_PDTC = 0x04,
    DEM_UDS_STATUS_CDTC = 0x08,
    DEM_UDS_STATUS_TNCSLC = 0x10,
    DEM_UDS_STATUS_TFSLC = 0x20,
    DEM_UDS_STATUS_TNCTOC = 0x40,
    DEM_UDS_STATUS_WIR = 0x80
} Dem_UdsStatusByteType;

typedef struct {
    Dem_EventIdType EventId;
    Dem_EventStatusType Status;
    uint8 DebounceCounter;
    boolean IsEnabled;
    boolean IsAvailable;
} Dem_TestEventInfoType;

/*==================================================================================================
*                                      MOCK FUNCTION DECLARATIONS
==================================================================================================*/
void mock_Dem_Reset(void);
void mock_Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType Status);
Dem_EventStatusType mock_Dem_GetEventStatus(Dem_EventIdType EventId);
uint8 mock_Dem_GetNumberOfStoredDTCs(void);
Std_ReturnType mock_Dem_ClearDTC(uint16 DTCGroup, uint8 DTCFormat);

#endif /* TEST_DEM_HELPER_H */
