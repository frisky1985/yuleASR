/*
 * @file runner_single.c
 * @brief 单套件集成测试入口
 *
 * 通过编译宏 RUN_COMM / RUN_DIAG / RUN_SAFETY / RUN_MEM 选择要运行的测试套件，
 * 使每个套件独立成可执行文件。这样每个测试文件的测试替身（mock）互不干扰，
 * 避免多个套件链接进同一可执行文件时的重复符号冲突（如 PduR_Transmit）。
 */
#include <unity.h>
#include <stdio.h>

#if defined(RUN_COMM)
extern int test_communication_stack_main(void);
#define SUITE_MAIN test_communication_stack_main
#define SUITE_NAME "Communication Stack"
#elif defined(RUN_DIAG)
extern int test_diagnostic_stack_main(void);
#define SUITE_MAIN test_diagnostic_stack_main
#define SUITE_NAME "Diagnostic Stack"
#elif defined(RUN_SAFETY)
extern int test_safety_stack_main(void);
#define SUITE_MAIN test_safety_stack_main
#define SUITE_NAME "Safety Stack"
#elif defined(RUN_MEM)
extern int test_memory_stack_main(void);
#define SUITE_MAIN test_memory_stack_main
#define SUITE_NAME "Memory Stack"
#else
#error "Define one of RUN_COMM / RUN_DIAG / RUN_SAFETY / RUN_MEM"
#endif

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    printf("========================================\n");
    printf("   AUTOSAR Integration Test - %s\n", SUITE_NAME);
    printf("========================================\n\n");

    int result = SUITE_MAIN();

    printf("\n========================================\n");
    if (result == 0)
    {
        printf("   %s TESTS PASSED!\n", SUITE_NAME);
    }
    else
    {
        printf("   %s TESTS FAILED!\n", SUITE_NAME);
    }
    printf("========================================\n");
    return result;
}
