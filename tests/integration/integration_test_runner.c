/*
 * @file integration_test_runner.c
 * @brief 集成测试主程序
 * 
 * 运行所有集成测试套件
 */

#include <unity.h>
#include <stdio.h>

/* 外部测试套件 */
extern int test_communication_stack_main(void);
extern int test_diagnostic_stack_main(void);
extern int test_safety_stack_main(void);
extern int test_memory_stack_main(void);

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    int result = 0;
    
    printf("========================================\n");
    printf("   AUTOSAR Integration Test Suite\n");
    printf("========================================\n\n");
    
    printf("[1/4] Running Communication Stack Tests...\n");
    result |= test_communication_stack_main();
    
    printf("\n[2/4] Running Diagnostic Stack Tests...\n");
    result |= test_diagnostic_stack_main();
    
    printf("\n[3/4] Running Safety Stack Tests...\n");
    result |= test_safety_stack_main();
    
    printf("\n[4/4] Running Memory Stack Tests...\n");
    result |= test_memory_stack_main();
    
    printf("\n========================================\n");
    if (result == 0) {
        printf("   ALL INTEGRATION TESTS PASSED!\n");
    } else {
        printf("   SOME TESTS FAILED!\n");
    }
    printf("========================================\n");
    
    return result;
}
