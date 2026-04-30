/*==================================================================================================
 *                            YULETECH AUTOSAR BSW INTEGRATION TEST FRAMEWORK
 *==================================================================================================
 * FILENAME: integration_test_framework.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Integration test framework implementation
 *==================================================================================================
 */

#include "integration_test_framework.h"

/*==================================================================================================
 *                                      GLOBAL DEFINITIONS
 *==================================================================================================*/
IntTestStatsType g_intTestStats = {0};
IntTestContextType g_intTestContext = {0};
jmp_buf g_intTestJumpBuffer;
int g_intTestJumpReady = 0;

/*==================================================================================================
 *                                      STATIC DATA
 *==================================================================================================*/
static struct {
    char srcModule[32];
    char dstModule[32];
    uint8_t data[INT_TEST_BUFFER_SIZE];
    uint32_t size;
    bool valid;
} s_dataFlowTrace[20];

static uint32_t s_dataFlowCount = 0;

/*==================================================================================================
 *                                      HELPER FUNCTIONS
 *==================================================================================================*/
const char* IntTest_GetResultString(IntTestResultType result) {
    switch (result) {
        case INT_TEST_RESULT_NOT_RUN: return "NOT RUN";
        case INT_TEST_RESULT_PASS: return "PASS";
        case INT_TEST_RESULT_FAIL: return "FAIL";
        case INT_TEST_RESULT_SKIP: return "SKIP";
        case INT_TEST_RESULT_TIMEOUT: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

void IntTest_ResetStats(void) {
    memset(&g_intTestStats, 0, sizeof(IntTestStatsType));
    memset(&g_intTestContext, 0, sizeof(IntTestContextType));
    s_dataFlowCount = 0;
    g_intTestStats.startTime = clock();
}

void IntTest_PrintSummary(void) {
    clock_t totalTime = clock() - g_intTestStats.startTime;
    
    printf("\n");
    printf("===============================================================\n");
    printf("  " TEST_COLOR_CYAN TEST_COLOR_BOLD "Integration Test Summary" TEST_COLOR_RESET "\n");
    printf("===============================================================\n");
    printf("  Total Tests:    %d\n", g_intTestStats.totalTests);
    printf("  " TEST_COLOR_GREEN "Passed:         %d" TEST_COLOR_RESET "\n", g_intTestStats.passed);
    printf("  " TEST_COLOR_RED "Failed:         %d" TEST_COLOR_RESET "\n", g_intTestStats.failed);
    printf("  " TEST_COLOR_YELLOW "Skipped:        %d" TEST_COLOR_RESET "\n", g_intTestStats.skipped);
    printf("  Timeouts:       %d\n", g_intTestStats.timeouts);
    printf("---------------------------------------------------------------\n");
    printf("  Total Time:     %ld ms\n", (long)(totalTime * 1000 / CLOCKS_PER_SEC));
    printf("  Avg Test Time:  %ld ms\n", 
           g_intTestStats.totalTests > 0 ? (long)(g_intTestStats.totalExecutionTimeMs / g_intTestStats.totalTests) : 0);
    printf("===============================================================\n");
    
    if (g_intTestStats.failed == 0 && g_intTestStats.passed > 0) {
        printf("  " TEST_COLOR_GREEN TEST_COLOR_BOLD "ALL INTEGRATION TESTS PASSED!" TEST_COLOR_RESET "\n\n");
    } else if (g_intTestStats.failed > 0) {
        printf("  " TEST_COLOR_RED TEST_COLOR_BOLD "SOME INTEGRATION TESTS FAILED!" TEST_COLOR_RESET "\n\n");
    } else {
        printf("  " TEST_COLOR_YELLOW "NO TESTS RUN!" TEST_COLOR_RESET "\n\n");
    }
}

void IntTest_TraceDataFlow(const char* srcModule, const char* dstModule, const void* data, uint32_t size) {
    if (s_dataFlowCount < 20) {
        strncpy(s_dataFlowTrace[s_dataFlowCount].srcModule, srcModule, 31);
        strncpy(s_dataFlowTrace[s_dataFlowCount].dstModule, dstModule, 31);
        s_dataFlowTrace[s_dataFlowCount].srcModule[31] = '\0';
        s_dataFlowTrace[s_dataFlowCount].dstModule[31] = '\0';
        
        uint32_t copySize = size < INT_TEST_BUFFER_SIZE ? size : INT_TEST_BUFFER_SIZE;
        memcpy(s_dataFlowTrace[s_dataFlowCount].data, data, copySize);
        s_dataFlowTrace[s_dataFlowCount].size = copySize;
        s_dataFlowTrace[s_dataFlowCount].valid = true;
        
        s_dataFlowCount++;
    }
}

bool IntTest_VerifyDataFlow(const char* srcModule, const char* dstModule, const void* data, uint32_t size) {
    for (uint32_t i = 0; i < s_dataFlowCount; i++) {
        if (s_dataFlowTrace[i].valid &&
            strcmp(s_dataFlowTrace[i].srcModule, srcModule) == 0 &&
            strcmp(s_dataFlowTrace[i].dstModule, dstModule) == 0) {
            
            uint32_t compareSize = size < s_dataFlowTrace[i].size ? size : s_dataFlowTrace[i].size;
            if (memcmp(s_dataFlowTrace[i].data, data, compareSize) == 0) {
                return true;
            }
        }
    }
    return false;
}

const char* _get_stack_name_for_suite(const char* suiteName) {
    if (strstr(suiteName, "Communication") != NULL) return "COM-PDUR-CANIF-CAN";
    if (strstr(suiteName, "Diagnostic") != NULL) return "DCM-DoIP-CanTSyn";
    if (strstr(suiteName, "Safety") != NULL) return "E2E-CRC-OS";
    if (strstr(suiteName, "Memory") != NULL) return "NVM-FEE-FLS";
    return "Unknown";
}

/*==================================================================================================
 *                                      MOCK UTILITIES
 *==================================================================================================*/
void IntTest_DelayMs(uint32_t ms) {
    clock_t start = clock();
    while ((clock() - start) * 1000 / CLOCKS_PER_SEC < ms) {
        /* Busy wait */
    }
}

uint32_t IntTest_GetTimestamp(void) {
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

void IntTest_FillTestData(uint8_t* buffer, uint32_t size, uint8_t pattern) {
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = pattern + (uint8_t)(i & 0xFF);
    }
}

bool IntTest_VerifyTestData(const uint8_t* buffer, uint32_t size, uint8_t pattern) {
    for (uint32_t i = 0; i < size; i++) {
        if (buffer[i] != (pattern + (uint8_t)(i & 0xFF))) {
            return false;
        }
    }
    return true;
}
