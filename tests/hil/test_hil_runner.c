/**
 * @file test_hil_runner.c
 * @brief HIL (Hardware-in-the-Loop) 测试入口
 * @details HIL 测试框架入口，聚合所有 HIL 测试套件。
 *          需要 S32K312 目标板连接在 CAN/LIN/Ethernet HIL 测试台上运行。
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: D
 * Target: SWR-002, SWR-003, SWR-004 (Full qualification)
 *
 * 硬件前提条件:
 *   1. S32K312 评估板通过 J-Link / PEmicro 连接主机
 *   2. CAN 总线已连接 CANcaseXL / PCAN-USB 适配器
 *   3. 12V 电源供电正常
 *   4. 目标固件已烧录 HIL 测试镜像
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>

/* 外部测试套件声明 */
extern int test_hil_can_run(void);
extern int test_hil_diag_run(void);

/*==================================================================================================
 *                                    Test Suite Registrations
 *================================================================================================*/

int main(void)
{
    int ret = 0;

    printf("\n");
    printf("===============================================================\n");
    printf("  yuleASR HIL (Hardware-in-the-Loop) Test Suite\n");
    printf("  Target: S32K312 | Framework: cmocka\n");
    printf("===============================================================\n");
    printf("\n");
    printf("  [INFO] 请确保以下硬件已就绪:\n");
    printf("  [INFO]   1. S32K312 评估板供电正常\n");
    printf("  [INFO]   2. CAN 总线适配器已连接\n");
    printf("  [INFO]   3. 目标固件已烧录 HIL 镜像\n");
    printf("  [INFO]   4. 诊断工具已连接 (CANoe / PCAN-View)\n");
    printf("\n");

    /* 运行 CAN 通信 HIL 测试 */
    printf("--- CAN Communication HIL Tests ---\n");
    ret |= test_hil_can_run();

    /* 运行诊断协议 HIL 测试 */
    printf("--- Diagnostic Protocol HIL Tests ---\n");
    ret |= test_hil_diag_run();

    printf("\n");
    printf("===============================================================\n");
    printf("  HIL Test Suite Complete: %s\n",
           (ret == 0) ? "ALL PASSED" : "SOME FAILED/SKIPPED");
    printf("===============================================================\n");

    return ret;
}
