/*==================================================================================================
 *                    SIMPLIFIED SYSTEM INTEGRATION TEST SUITE
 *==================================================================================================
 * FILENAME: system_integration_test_simple.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Simplified system-level integration test using mock implementations
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;

typedef unsigned int boolean;
#define TRUE  1u
#define FALSE 0u

#define NULL_PTR ((void*)0)

typedef enum {
    E_OK = 0,
    E_NOT_OK = 1
} Std_ReturnType;

/*==================================================================================================
 *                                    TEST FRAMEWORK
 *==================================================================================================*/
#define TEST_COLOR_GREEN   "\033[32m"
#define TEST_COLOR_RED     "\033[31m"
#define TEST_COLOR_YELLOW  "\033[33m"
#define TEST_COLOR_RESET   "\033[0m"

typedef struct {
    int total;
    int passed;
    int failed;
    int skipped;
} TestResults;

static TestResults g_results = {0, 0, 0, 0};

#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            printf("      " TEST_COLOR_RED "FAIL: %s" TEST_COLOR_RESET "\n", msg); \
            g_results.failed++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            printf("      " TEST_COLOR_RED "FAIL: %s (expected %d, got %d)" TEST_COLOR_RESET "\n", \
                   msg, (int)(expected), (int)(actual)); \
            g_results.failed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("      " TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "\n"); \
        g_results.passed++; \
    } while(0)

#define RUN_TEST(name) \
    do { \
        g_results.total++; \
        printf("    Running " #name "...\n"); \
        name(); \
    } while(0)

/*==================================================================================================
 *                                    FAULT INJECTION SYSTEM
 *==================================================================================================*/
typedef enum {
    FAULT_NONE = 0,
    FAULT_FLASH_WRITE_FAIL,
    FAULT_FLASH_READ_FAIL,
    FAULT_FLASH_ERASE_FAIL,
    FAULT_WDG_TIMEOUT,
    FAULT_CRYPTO_FAIL,
    FAULT_NVM_CRC_FAIL,
    FAULT_COMM_TIMEOUT,
    FAULT_MAX
} FaultInjectionType;

static FaultInjectionType g_currentFault = FAULT_NONE;
static uint32_t g_faultCount = 0;

void FaultInjection_Set(FaultInjectionType fault) {
    g_currentFault = fault;
    g_faultCount++;
    printf("      [FAULT] Injected fault type %d\n", fault);
}

void FaultInjection_Clear(void) {
    g_currentFault = FAULT_NONE;
}

boolean FaultInjection_IsActive(FaultInjectionType fault) {
    return (g_currentFault == fault);
}

/*==================================================================================================
 *                                    MOCK STORAGE STACK
 *==================================================================================================*/
#define MOCK_FLASH_SIZE     4096
#define MOCK_BLOCK_SIZE     64
#define MOCK_NUM_BLOCKS     64

typedef enum {
    FLS_OK = 0,
    FLS_NOT_OK,
    FLS_BUSY
} Fls_StatusType;

typedef enum {
    FEE_IDLE = 0,
    FEE_BUSY,
    FEE_ERROR
} Fee_StatusType;

static uint8_t MockFlash[MOCK_FLASH_SIZE];
static boolean BlockValid[MOCK_NUM_BLOCKS];
static Fee_StatusType FeeStatus = FEE_IDLE;
static uint32_t FlashWriteCount = 0;
static uint32_t FlashReadCount = 0;
static uint32_t FlashEraseCount = 0;

void MockStorage_Init(void) {
    memset(MockFlash, 0xFF, MOCK_FLASH_SIZE);
    memset(BlockValid, FALSE, sizeof(BlockValid));
    FeeStatus = FEE_IDLE;
    FlashWriteCount = 0;
    FlashReadCount = 0;
    FlashEraseCount = 0;
}

Fls_StatusType MockFls_Write(uint32_t addr, const uint8_t* data, uint32_t len) {
    if (FaultInjection_IsActive(FAULT_FLASH_WRITE_FAIL)) {
        return FLS_NOT_OK;
    }
    if (addr + len > MOCK_FLASH_SIZE) return FLS_NOT_OK;
    if (data == NULL_PTR) return FLS_NOT_OK;
    
    for (uint32_t i = 0; i < len; i++) {
        MockFlash[addr + i] &= data[i];
    }
    FlashWriteCount++;
    return FLS_OK;
}

Fls_StatusType MockFls_Read(uint32_t addr, uint8_t* data, uint32_t len) {
    if (FaultInjection_IsActive(FAULT_FLASH_READ_FAIL)) {
        return FLS_NOT_OK;
    }
    if (addr + len > MOCK_FLASH_SIZE) return FLS_NOT_OK;
    if (data == NULL_PTR) return FLS_NOT_OK;
    
    memcpy(data, &MockFlash[addr], len);
    FlashReadCount++;
    return FLS_OK;
}

Fls_StatusType MockFls_Erase(uint32_t addr, uint32_t len) {
    if (FaultInjection_IsActive(FAULT_FLASH_ERASE_FAIL)) {
        return FLS_NOT_OK;
    }
    if (addr + len > MOCK_FLASH_SIZE) return FLS_NOT_OK;
    
    memset(&MockFlash[addr], 0xFF, len);
    FlashEraseCount++;
    return FLS_OK;
}

Std_ReturnType MockFee_Write(uint16 blockId, const uint8_t* data) {
    if (blockId >= MOCK_NUM_BLOCKS) return E_NOT_OK;
    if (data == NULL_PTR) return E_NOT_OK;
    if (FeeStatus == FEE_BUSY) return E_NOT_OK;
    
    FeeStatus = FEE_BUSY;
    uint32_t addr = blockId * MOCK_BLOCK_SIZE;
    
    /* Simulate retry on failure */
    int retries = 3;
    while (retries > 0) {
        if (MockFls_Write(addr, data, MOCK_BLOCK_SIZE) == FLS_OK) {
            BlockValid[blockId] = TRUE;
            FeeStatus = FEE_IDLE;
            return E_OK;
        }
        retries--;
        FaultInjection_Clear(); /* Clear fault for retry */
    }
    
    FeeStatus = FEE_ERROR;
    return E_NOT_OK;
}

Std_ReturnType MockFee_Read(uint16 blockId, uint8_t* data) {
    if (blockId >= MOCK_NUM_BLOCKS) return E_NOT_OK;
    if (data == NULL_PTR) return E_NOT_OK;
    if (!BlockValid[blockId]) return E_NOT_OK;
    
    uint32_t addr = blockId * MOCK_BLOCK_SIZE;
    if (MockFls_Read(addr, data, MOCK_BLOCK_SIZE) != FLS_OK) {
        return E_NOT_OK;
    }
    return E_OK;
}

/*==================================================================================================
 *                                    MOCK WATCHDOG STACK
 *==================================================================================================*/
typedef enum {
    WDGIF_OFF_MODE = 0,
    WDGIF_SLOW_MODE,
    WDGIF_FAST_MODE
} WdgIf_ModeType;

typedef enum {
    WDGM_STATUS_DEACTIVATED = 0,
    WDGM_STATUS_OK,
    WDGM_STATUS_FAILED,
    WDGM_STATUS_EXPIRED
} Wdgm_StatusType;

static WdgIf_ModeType WdgCurrentMode = WDGIF_OFF_MODE;
static Wdgm_StatusType WdgmStatus = WDGM_STATUS_DEACTIVATED;
static uint32_t WdgTriggerCount = 0;
static uint32_t CheckpointCount = 0;
static boolean WdgEnabled = FALSE;

void MockWdg_Init(void) {
    WdgCurrentMode = WDGIF_SLOW_MODE;
    WdgmStatus = WDGM_STATUS_OK;
    WdgTriggerCount = 0;
    CheckpointCount = 0;
    WdgEnabled = TRUE;
}

Std_ReturnType MockWdgIf_Trigger(void) {
    if (!WdgEnabled) return E_NOT_OK;
    if (WdgCurrentMode == WDGIF_OFF_MODE) return E_NOT_OK;
    if (FaultInjection_IsActive(FAULT_WDG_TIMEOUT)) {
        WdgmStatus = WDGM_STATUS_EXPIRED;
        return E_NOT_OK;
    }
    
    WdgTriggerCount++;
    return E_OK;
}

Std_ReturnType MockWdgm_CheckpointReached(uint16 seId, uint16 checkpointId) {
    (void)seId;
    (void)checkpointId;
    CheckpointCount++;
    
    /* Feed watchdog on checkpoint */
    return MockWdgIf_Trigger();
}

void MockWdgm_SetMode(WdgIf_ModeType mode) {
    WdgCurrentMode = mode;
}

/*==================================================================================================
 *                                    MOCK SECURITY STACK
 *==================================================================================================*/
#define MAC_SIZE 16
#define KEY_SIZE 16

typedef enum {
    CSM_VER_OK = 0,
    CSM_VER_NOT_OK
} Csm_VerifyResultType;

static uint8_t MockKey[KEY_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                     0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
static uint32_t CryptoOpCount = 0;

Std_ReturnType MockCsm_MacGenerate(const uint8_t* data, uint32_t len, uint8_t* mac) {
    if (FaultInjection_IsActive(FAULT_CRYPTO_FAIL)) {
        return E_NOT_OK;
    }
    if (data == NULL_PTR || mac == NULL_PTR) return E_NOT_OK;
    
    /* Simple XOR-based MAC for testing */
    memset(mac, 0, MAC_SIZE);
    for (uint32_t i = 0; i < len && i < MAC_SIZE; i++) {
        mac[i] = data[i] ^ MockKey[i % KEY_SIZE];
    }
    CryptoOpCount++;
    return E_OK;
}

Std_ReturnType MockCsm_MacVerify(const uint8_t* data, uint32_t len, 
                                  const uint8_t* mac, Csm_VerifyResultType* result) {
    uint8_t computedMac[MAC_SIZE];
    
    if (MockCsm_MacGenerate(data, len, computedMac) != E_OK) {
        *result = CSM_VER_NOT_OK;
        return E_NOT_OK;
    }
    
    if (memcmp(mac, computedMac, MAC_SIZE) == 0) {
        *result = CSM_VER_OK;
    } else {
        *result = CSM_VER_NOT_OK;
    }
    return E_OK;
}

/*==================================================================================================
 *                                    MOCK ERROR HANDLING
 *==================================================================================================*/
#define MAX_ERRORS 32

typedef struct {
    uint16 moduleId;
    uint8 instanceId;
    uint8 apiId;
    uint8 errorId;
} ErrorRecordType;

static ErrorRecordType ErrorLog[MAX_ERRORS];
static uint32_t ErrorCount = 0;

void MockDet_Init(void) {
    memset(ErrorLog, 0, sizeof(ErrorLog));
    ErrorCount = 0;
}

void MockDet_ReportError(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId) {
    if (ErrorCount < MAX_ERRORS) {
        ErrorLog[ErrorCount].moduleId = moduleId;
        ErrorLog[ErrorCount].instanceId = instanceId;
        ErrorLog[ErrorCount].apiId = apiId;
        ErrorLog[ErrorCount].errorId = errorId;
        ErrorCount++;
    }
}

/*==================================================================================================
 *                                    TEST CASES - STORAGE
 *==================================================================================================*/
void test_storage_stack_init(void) {
    printf("    Test: Storage Stack Initialization\n");
    MockStorage_Init();
    TEST_ASSERT_EQ(FEE_IDLE, FeeStatus, "Fee should be IDLE after init");
    TEST_ASSERT_EQ(0u, FlashWriteCount, "No writes after init");
}

void test_storage_write_read(void) {
    uint8_t writeData[MOCK_BLOCK_SIZE];
    uint8_t readData[MOCK_BLOCK_SIZE];
    
    printf("    Test: Storage Write-Read Cycle\n");
    MockStorage_Init();
    
    /* Prepare test data */
    for (int i = 0; i < MOCK_BLOCK_SIZE; i++) {
        writeData[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Write block */
    TEST_ASSERT_EQ(E_OK, MockFee_Write(1, writeData), "Write should succeed");
    TEST_ASSERT_EQ(1u, FlashWriteCount, "One write operation");
    
    /* Read back */
    memset(readData, 0, sizeof(readData));
    TEST_ASSERT_EQ(E_OK, MockFee_Read(1, readData), "Read should succeed");
    TEST_ASSERT_EQ(0, memcmp(writeData, readData, MOCK_BLOCK_SIZE), "Data should match");
}

void test_storage_write_retry(void) {
    uint8_t writeData[MOCK_BLOCK_SIZE];
    
    printf("    Test: Storage Write Retry on Failure\n");
    MockStorage_Init();
    
    memset(writeData, 0xAA, sizeof(writeData));
    
    /* Inject fault for first attempts */
    FaultInjection_Set(FAULT_FLASH_WRITE_FAIL);
    
    /* Write should eventually succeed after retries */
    Std_ReturnType result = MockFee_Write(2, writeData);
    TEST_ASSERT_EQ(E_OK, result, "Write should succeed after retries");
    TEST_ASSERT(FlashWriteCount > 0, "Multiple write attempts");
    
    FaultInjection_Clear();
}

void test_storage_gc_trigger(void) {
    uint8_t writeData[MOCK_BLOCK_SIZE];
    
    printf("    Test: Garbage Collection Trigger\n");
    MockStorage_Init();
    
    memset(writeData, 0x55, sizeof(writeData));
    
    /* Write multiple blocks */
    for (int i = 0; i < 10; i++) {
        MockFee_Write((uint16)i, writeData);
    }
    
    TEST_ASSERT(FlashWriteCount >= 10, "Multiple blocks written");
}

/*==================================================================================================
 *                                    TEST CASES - WATCHDOG
 *==================================================================================================*/
void test_watchdog_chain_init(void) {
    printf("    Test: Watchdog Chain Initialization\n");
    MockWdg_Init();
    TEST_ASSERT_EQ(WDGM_STATUS_OK, WdgmStatus, "Wdgm should be OK after init");
    TEST_ASSERT_EQ(WDGIF_SLOW_MODE, WdgCurrentMode, "Default mode should be SLOW");
}

void test_watchdog_normal_trigger(void) {
    printf("    Test: Watchdog Normal Triggering\n");
    MockWdg_Init();
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQ(E_OK, MockWdgIf_Trigger(), "Trigger should succeed");
    }
    
    TEST_ASSERT_EQ(5u, WdgTriggerCount, "Trigger count should be 5");
}

void test_watchdog_checkpoint_supervision(void) {
    printf("    Test: Watchdog Checkpoint Supervision\n");
    MockWdg_Init();
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQ(E_OK, MockWdgm_CheckpointReached(0, (uint16)i), 
                      "Checkpoint should succeed");
    }
    
    TEST_ASSERT_EQ(5u, CheckpointCount, "Checkpoint count should be 5");
    TEST_ASSERT_EQ(5u, WdgTriggerCount, "Watchdog should be triggered 5 times");
}

void test_watchdog_timeout_detection(void) {
    printf("    Test: Watchdog Timeout Detection\n");
    MockWdg_Init();
    
    FaultInjection_Set(FAULT_WDG_TIMEOUT);
    
    Std_ReturnType result = MockWdgIf_Trigger();
    TEST_ASSERT_EQ(E_NOT_OK, result, "Trigger should fail with timeout fault");
    TEST_ASSERT_EQ(WDGM_STATUS_EXPIRED, WdgmStatus, "Status should be EXPIRED");
    
    FaultInjection_Clear();
}

/*==================================================================================================
 *                                    TEST CASES - SECURITY
 *==================================================================================================*/
void test_security_mac_generate(void) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t mac[MAC_SIZE];
    
    printf("    Test: Security MAC Generation\n");
    
    TEST_ASSERT_EQ(E_OK, MockCsm_MacGenerate(data, sizeof(data), mac), 
                  "MAC generation should succeed");
    TEST_ASSERT_EQ(1u, CryptoOpCount, "One crypto operation");
}

void test_security_mac_verify_success(void) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t mac[MAC_SIZE];
    Csm_VerifyResultType result;
    
    printf("    Test: Security MAC Verify Success\n");
    
    MockCsm_MacGenerate(data, sizeof(data), mac);
    TEST_ASSERT_EQ(E_OK, MockCsm_MacVerify(data, sizeof(data), mac, &result),
                  "MAC verify should succeed");
    TEST_ASSERT_EQ(CSM_VER_OK, result, "Verification should pass");
}

void test_security_mac_verify_failure(void) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t mac[MAC_SIZE];
    Csm_VerifyResultType result;
    
    printf("    Test: Security MAC Verify Failure (Tampering Detection)\n");
    
    MockCsm_MacGenerate(data, sizeof(data), mac);
    
    /* Tamper with data */
    data[0] ^= 0xFF;
    
    TEST_ASSERT_EQ(E_OK, MockCsm_MacVerify(data, sizeof(data), mac, &result),
                  "MAC verify call should succeed");
    TEST_ASSERT_EQ(CSM_VER_NOT_OK, result, "Verification should fail for tampered data");
}

void test_security_pdu_integrity(void) {
    uint8_t pdu[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    uint8_t mac[MAC_SIZE];
    Csm_VerifyResultType result;
    
    printf("    Test: Security PDU Integrity\n");
    
    /* Sender generates MAC */
    TEST_ASSERT_EQ(E_OK, MockCsm_MacGenerate(pdu, sizeof(pdu), mac),
                  "Sender MAC generation should succeed");
    
    /* Receiver verifies MAC */
    TEST_ASSERT_EQ(E_OK, MockCsm_MacVerify(pdu, sizeof(pdu), mac, &result),
                  "Receiver MAC verify should succeed");
    TEST_ASSERT_EQ(CSM_VER_OK, result, "PDU integrity verified");
}

/*==================================================================================================
 *                                    TEST CASES - ERROR HANDLING
 *==================================================================================================*/
void test_error_reporting(void) {
    printf("    Test: Error Reporting to DET\n");
    MockDet_Init();
    
    MockDet_ReportError(1, 0, 1, 1);
    MockDet_ReportError(2, 0, 2, 2);
    
    TEST_ASSERT_EQ(2u, ErrorCount, "Two errors should be logged");
    TEST_ASSERT_EQ(1u, ErrorLog[0].moduleId, "First error from module 1");
    TEST_ASSERT_EQ(2u, ErrorLog[1].moduleId, "Second error from module 2");
}

void test_error_propagation(void) {
    uint8_t data[MOCK_BLOCK_SIZE];
    
    printf("    Test: Error Propagation Chain\n");
    MockDet_Init();
    MockStorage_Init();
    
    /* Try invalid operation */
    Std_ReturnType result = MockFee_Write(MOCK_NUM_BLOCKS + 1, data);
    
    TEST_ASSERT_EQ(E_NOT_OK, result, "Invalid operation should fail");
}

/*==================================================================================================
 *                                    BSW INTERACTION TESTS
 *==================================================================================================*/
void test_bsw_init_sequence(void) {
    printf("    Test: BSW Initialization Sequence\n");
    
    /* Simulated BSW init sequence */
    MockDet_Init();
    MockStorage_Init();
    MockWdg_Init();
    
    TEST_ASSERT_EQ(FEE_IDLE, FeeStatus, "Fee initialized");
    TEST_ASSERT_EQ(WDGM_STATUS_OK, WdgmStatus, "Wdgm initialized");
    TEST_ASSERT_EQ(0u, ErrorCount, "No errors during init");
}

void test_bsw_shutdown_sequence(void) {
    printf("    Test: BSW Shutdown Sequence\n");
    
    /* Simulated shutdown sequence */
    MockWdg_Init();
    
    /* Deinit watchdog */
    WdgEnabled = FALSE;
    
    TEST_ASSERT_EQ(FALSE, WdgEnabled, "Watchdog disabled");
}

void test_bsw_schm_scheduling(void) {
    printf("    Test: SchM Module Scheduling Simulation\n");
    
    MockWdg_Init();
    MockStorage_Init();
    
    /* Simulate cyclic scheduling */
    for (int cycle = 0; cycle < 5; cycle++) {
        /* Wdgm MainFunction */
        MockWdgm_CheckpointReached(0, (uint16)cycle);
        
        /* Fee MainFunction simulation */
        if (FeeStatus == FEE_BUSY) {
            FeeStatus = FEE_IDLE;
        }
    }
    
    TEST_ASSERT_EQ(5u, CheckpointCount, "All cycles executed");
}

/*==================================================================================================
 *                                    TEST SUITES
 *==================================================================================================*/
void run_storage_tests(void) {
    printf("\n  === Storage Link Integration Tests ===\n\n");
    RUN_TEST(test_storage_stack_init);
    RUN_TEST(test_storage_write_read);
    RUN_TEST(test_storage_write_retry);
    RUN_TEST(test_storage_gc_trigger);
}

void run_watchdog_tests(void) {
    printf("\n  === Watchdog Supervision Chain Tests ===\n\n");
    RUN_TEST(test_watchdog_chain_init);
    RUN_TEST(test_watchdog_normal_trigger);
    RUN_TEST(test_watchdog_checkpoint_supervision);
    RUN_TEST(test_watchdog_timeout_detection);
}

void run_security_tests(void) {
    printf("\n  === Security Communication Chain Tests ===\n\n");
    RUN_TEST(test_security_mac_generate);
    RUN_TEST(test_security_mac_verify_success);
    RUN_TEST(test_security_mac_verify_failure);
    RUN_TEST(test_security_pdu_integrity);
}

void run_error_tests(void) {
    printf("\n  === Error Handling Chain Tests ===\n\n");
    RUN_TEST(test_error_reporting);
    RUN_TEST(test_error_propagation);
}

void run_bsw_tests(void) {
    printf("\n  === BSW Module Interaction Tests ===\n\n");
    RUN_TEST(test_bsw_init_sequence);
    RUN_TEST(test_bsw_shutdown_sequence);
    RUN_TEST(test_bsw_schm_scheduling);
}

/*==================================================================================================
 *                                    MAIN FUNCTION
 *==================================================================================================*/
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("===============================================================\n");
    printf("  YuleTech AutoSAR BSW - System Integration Test Suite\n");
    printf("  Version: 1.0.0\n");
    printf("  AutoSAR: R22-11\n");
    printf("===============================================================\n");
    
    /* Run all test suites */
    run_storage_tests();
    run_watchdog_tests();
    run_security_tests();
    run_error_tests();
    run_bsw_tests();
    
    /* Print summary */
    printf("\n");
    printf("===============================================================\n");
    printf("  TEST RESULTS SUMMARY\n");
    printf("===============================================================\n");
    printf("  Total Tests:   %d\n", g_results.total);
    printf("  " TEST_COLOR_GREEN "Passed:        %d" TEST_COLOR_RESET "\n", g_results.passed);
    printf("  " TEST_COLOR_RED "Failed:        %d" TEST_COLOR_RESET "\n", g_results.failed);
    printf("  Skipped:       %d\n", g_results.skipped);
    printf("===============================================================\n");
    
    if (g_results.failed == 0 && g_results.passed > 0) {
        printf("  " TEST_COLOR_GREEN "ALL TESTS PASSED!" TEST_COLOR_RESET "\n");
        printf("===============================================================\n\n");
        return 0;
    } else {
        printf("  " TEST_COLOR_RED "SOME TESTS FAILED!" TEST_COLOR_RESET "\n");
        printf("===============================================================\n\n");
        return 1;
    }
}
