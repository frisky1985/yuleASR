/*
 * mock_PduR.c
 * Mock implementation for PduR module - used by unit tests
 */

#define MOCK_PDUR_C
#include "mock_PduR.h"

/* Mock state variables */
PduIdType mock_PduR_IfTransmit_lastPduId = 0;
int mock_PduR_IfTransmit_callCount = 0;
Std_ReturnType mock_PduR_IfTransmit_nextResult = E_OK;

/* Mock implementation */
Std_ReturnType PduR_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    mock_PduR_IfTransmit_lastPduId = TxPduId;
    mock_PduR_IfTransmit_callCount++;
    (void)PduInfoPtr;
    return mock_PduR_IfTransmit_nextResult;
}
