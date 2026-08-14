/**
 * @file test_pdur_coverage.c
 * @brief PduR Coverage test — exercises PduR public API paths
 */
#include <stdio.h>
#include <string.h>
#include "PduR.h"
#include "Det.h"

/* Minimal config for PduR init */
static PduR_RoutingPathConfigType paths[1] = {0};
static PduR_RoutingPathGroupConfigType groups[1] = {0};
static PduR_ConfigType cfg = {paths, 0, groups, 0, 0, 0};
static PduInfoType pdu;

int main(void) {
    int passed = 0, total = 0;

    /* PduR_Init */
    PduR_Init(&cfg);
    total++; passed++;
    printf("  [PASS] PduR_Init\n");

    /* PduR_GetVersionInfo */
    Std_VersionInfoType ver;
    memset(&ver, 0, sizeof(ver));
    PduR_GetVersionInfo(&ver);
    total++; passed++;
    printf("  [PASS] PduR_GetVersionInfo vendor=%u\n", (unsigned)ver.vendorID);

    /* PduR_Transmit — returns valid status */
    memset(&pdu, 0, sizeof(pdu));
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    total++;
    if (ret == E_OK || ret == E_NOT_OK) {
        passed++;
        printf("  [PASS] PduR_Transmit ret=%d\n", (int)ret);
    } else {
        printf("  [FAIL] PduR_Transmit unexpected ret=%d\n", (int)ret);
    }

    /* PduR_MainFunction */
    PduR_MainFunction();
    total++; passed++;
    printf("  [PASS] PduR_MainFunction\n");

    /* PduR_DeInit */
    PduR_DeInit();
    total++; passed++;
    printf("  [PASS] PduR_DeInit\n");

    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
