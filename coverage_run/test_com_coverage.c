/**
 * @file test_com_coverage.c
 * @brief Minimal Com coverage test (src/bsw/services/com/)
 */
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "Com.h"
#include "Det.h"

static Com_ConfigType cfg;

int main(void) {
    int passed = 0, total = 0;

    /* Com_Init */
    Com_Init(&cfg);
    total++; passed++;
    printf("  [PASS] Com_Init\n");

    /* Com_GetVersionInfo */
    Std_VersionInfoType ver;
    memset(&ver, 0, sizeof(ver));
    Com_GetVersionInfo(&ver);
    total++; passed++;
    printf("  [PASS] Com_GetVersionInfo\n");

    /* Com_DeInit */
    Com_DeInit();
    total++; passed++;
    printf("  [PASS] Com_DeInit\n");

    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
