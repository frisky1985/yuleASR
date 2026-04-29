/*
 * mock_PduR.h
 * Mock header for PduR module - used by unit tests
 */

#ifndef MOCK_PDUR_H
#define MOCK_PDUR_H

#include "PduR.h"
#include "unity.h"

/* CMock-style mock functions for PduR_IfTransmit */
#define PduR_IfTransmit_ExpectAndReturn(TxPduId, PduInfoPtr, result) \
    do { \
        /* Mock implementation - just record the call */ \
        extern PduIdType mock_PduR_IfTransmit_lastPduId; \
        extern int mock_PduR_IfTransmit_callCount; \
        extern Std_ReturnType mock_PduR_IfTransmit_nextResult; \
        mock_PduR_IfTransmit_lastPduId = TxPduId; \
        mock_PduR_IfTransmit_callCount++; \
        mock_PduR_IfTransmit_nextResult = result; \
    } while(0)

#define PduR_IfTransmit_IgnoreArg_PduInfoPtr() \
    do { /* No-op for ignoring argument */ } while(0)

/* Declaration of mock state variables */
extern PduIdType mock_PduR_IfTransmit_lastPduId;
extern int mock_PduR_IfTransmit_callCount;
extern Std_ReturnType mock_PduR_IfTransmit_nextResult;

/* Mock implementation of PduR_IfTransmit */
static inline Std_ReturnType PduR_IfTransmit_Mock(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return mock_PduR_IfTransmit_nextResult;
}

/* Reset mock state */
static inline void mock_PduR_Init(void)
{
    mock_PduR_IfTransmit_lastPduId = 0;
    mock_PduR_IfTransmit_callCount = 0;
    mock_PduR_IfTransmit_nextResult = E_OK;
}

/* Variables defined in mock_PduR.c */
#ifdef MOCK_PDUR_C
PduIdType mock_PduR_IfTransmit_lastPduId = 0;
int mock_PduR_IfTransmit_callCount = 0;
Std_ReturnType mock_PduR_IfTransmit_nextResult = E_OK;
#endif

#endif /* MOCK_PDUR_H */
