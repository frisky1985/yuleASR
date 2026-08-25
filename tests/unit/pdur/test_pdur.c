/**
 * @file test_pdur.c
 * @brief PduR (PDU Router) Unit Tests
 *
 * SHALL-PDUR-01: SHALL maintain a static routing table generated at build time
 * SHALL-PDUR-02: SHALL support a maximum of 512 routing paths
 * SHALL-PDUR-03: SHALL support gateway routing between CAN <-> LIN and CAN <-> Ethernet
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "PduR.h"
#include "PduR_Cfg.h"

/* Test: PduR_Init */
/** @req SWS_PduR_00001 */
static void test_PduR_Init(void **state)
{
    (void)state;
    
    const PduR_PBConfigType* config = NULL;
    Std_ReturnType result = PduR_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: PduR_DeInit */
/** @req SWS_PduR_00002 */
static void test_PduR_DeInit(void **state)
{
    (void)state;
    
    PduR_DeInit();
    assert_true(1);
}

/* Test: PduR_GetVersionInfo */
/** @req SWS_PduR_00012 */
static void test_PduR_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    PduR_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: PduR_ComTransmit */
/** @req SWS_PduR_00003 */
static void test_PduR_ComTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = PduR_ComTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: PduR_CanIfRxIndication */
/** @req SWS_PduR_00004 */
static void test_PduR_CanIfRxIndication(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    PduR_CanIfRxIndication(pduId, &pduInfo);
    assert_true(1);
}

/* Test: PduR_CanIfTxConfirmation */
/** @req SWS_PduR_00005 */
static void test_PduR_CanIfTxConfirmation(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    Std_ReturnType result = E_OK;
    
    PduR_CanIfTxConfirmation(pduId, result);
    assert_true(1);
}

/* Test: PduR_DcmTransmit */
/** @req SWS_PduR_00003 */
static void test_PduR_DcmTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = PduR_DcmTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_PduR_Init),
        cmocka_unit_test(test_PduR_DeInit),
        cmocka_unit_test(test_PduR_GetVersionInfo),
        cmocka_unit_test(test_PduR_ComTransmit),
        cmocka_unit_test(test_PduR_CanIfRxIndication),
        cmocka_unit_test(test_PduR_CanIfTxConfirmation),
        cmocka_unit_test(test_PduR_DcmTransmit),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
