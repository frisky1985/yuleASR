/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Integration Test Runner
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include "test_framework.h"

/*==================================================================================================
*                                  EXTERNAL TEST CASES
==================================================================================================*/
extern void test_can_signal_end_to_end(void);
extern void test_nvm_startup_recovery(void);
extern void test_diagnostic_request_end_to_end(void);
extern void test_mode_switch_bswm_ecum(void);
extern void test_os_alarm_cyclic_scheduling(void);

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    printf("\n  Cross-Layer Integration Test Suite\n");
    printf("  MCAL -> ECUAL -> Service -> RTE\n");
    printf("  -----------------------------------\n");

    RUN_TEST(test_can_signal_end_to_end);
    RUN_TEST(test_nvm_startup_recovery);
    RUN_TEST(test_diagnostic_request_end_to_end);
    RUN_TEST(test_mode_switch_bswm_ecum);
    RUN_TEST(test_os_alarm_cyclic_scheduling);

TEST_MAIN_END()
