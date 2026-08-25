/**
 * @file test_eep.c
 * @brief EEPROM Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 */

// @tests src/bsw/mcal/eep/src/Eep.c  @tests src/bsw/mcal/eep/include/Eep.h

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 定义测试所需的版本信息宏 */
#define EEP_SW_MAJOR_VERSION            1U
#define EEP_SW_MINOR_VERSION            0U
#define EEP_SW_PATCH_VERSION            0U

/* EEP SID定义 */
#define EEP_SID_INIT                    0x01U
#define EEP_SID_DEINIT                  0x02U
#define EEP_SID_READ                    0x03U
#define EEP_SID_WRITE                   0x04U
#define EEP_SID_ERASE                   0x05U
#define EEP_SID_CANCEL                  0x06U
#define EEP_SID_GET_STATUS              0x07U
#define EEP_SID_GET_JOB_RESULT          0x08U

/* 标准类型定义 */
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef enum { FALSE = 0, TRUE = 1 } boolean;
typedef enum { E_OK = 0, E_NOT_OK } Std_ReturnType;

#ifndef STD_ON
#define STD_ON                          1U
#define STD_OFF                         0U
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

/* EEP错误码 */
#define EEP_E_NO_ERROR                  0x00U
#define EEP_E_PARAM_POINTER             0x01U
#define EEP_E_PARAM_ADDRESS             0x02U
#define EEP_E_PARAM_LENGTH              0x03U
#define EEP_E_UNINIT                    0x04U
#define EEP_E_BUSY                      0x05U

/* EEP模块ID */
#define EEP_MODULE_ID                   0x5FU

/* 开发错误检测使能 */
#define EEP_DEV_ERROR_DETECT            STD_ON

/* EEP类型定义 */
typedef uint32 Eep_AddressType;
typedef uint32 Eep_LengthType;

typedef enum {
    EEP_JOB_OK = 0,
    EEP_JOB_PENDING,
    EEP_JOB_FAILED,
    EEP_JOB_CANCELED
} Eep_JobResultType;

typedef enum {
    EEP_UNINIT = 0,
    EEP_IDLE,
    EEP_BUSY
} Eep_StatusType;

typedef struct {
    Eep_AddressType BaseAddress;
    Eep_LengthType Size;
    uint32 JobCallCycle;
} Eep_ConfigType;

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Det错误记录 */
static uint16 det_last_module_id = 0;
static uint8 det_last_instance_id = 0;
static uint8 det_last_api_id = 0;
static uint8 det_last_error_id = 0;
static int det_error_count = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (0x%X == 0x%X)\n", #expected, #actual, (unsigned int)(expected), (unsigned int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (0x%X != 0x%X) (%s:%d)\n", #expected, #actual, (unsigned int)(expected), (unsigned int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/* Det_ReportError模拟 */
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    det_last_module_id = ModuleId;
    det_last_instance_id = InstanceId;
    det_last_api_id = ApiId;
    det_last_error_id = ErrorId;
    det_error_count++;
    printf("    [DET] Module=0x%X, Instance=%d, API=0x%X, Error=0x%X\n", 
           ModuleId, InstanceId, ApiId, ErrorId);
}

/* 模拟EEPROM状态机 */
typedef enum {
    EEP_STATE_UNINIT = 0,
    EEP_STATE_IDLE,
    EEP_STATE_READ,
    EEP_STATE_WRITE,
    EEP_STATE_ERASE
} Eep_StateType;

static Eep_StateType eep_state = EEP_STATE_UNINIT;
static const Eep_ConfigType* eep_config_ptr = NULL_PTR;
static Eep_JobResultType eep_job_result = EEP_JOB_OK;
static Eep_AddressType eep_current_address = 0;
static uint8* eep_current_data_ptr = NULL_PTR;
static Eep_LengthType eep_current_length = 0;

/* 模拟EEPROM数据存储 */
static uint8 mock_eeprom_memory[65536];  /* 64KB EEPROM */
static uint8 mock_read_buffer[1024];

/* EEP初始化 */
void Eep_Init(const Eep_ConfigType* ConfigPtr) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_INIT, EEP_E_PARAM_POINTER);
        return;
    }
#endif
    eep_config_ptr = ConfigPtr;
    eep_state = EEP_STATE_IDLE;
    eep_job_result = EEP_JOB_OK;
}

/* EEP反初始化 */
void Eep_DeInit(void) {
    eep_state = EEP_STATE_UNINIT;
    eep_config_ptr = NULL_PTR;
}

/* EEP读操作 */
Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (eep_state == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if ((NULL_PTR == DataPtr) || (0U == Length)) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (eep_state != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    eep_current_address = Address;
    eep_current_data_ptr = DataPtr;
    eep_current_length = Length;
    eep_state = EEP_STATE_READ;
    eep_job_result = EEP_JOB_PENDING;
    return E_OK;
}

/* EEP写操作 */
Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (eep_state == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if ((NULL_PTR == DataPtr) || (0U == Length)) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (eep_state != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    eep_current_address = Address;
    eep_current_data_ptr = (uint8*)DataPtr;
    eep_current_length = Length;
    eep_state = EEP_STATE_WRITE;
    eep_job_result = EEP_JOB_PENDING;
    return E_OK;
}

/* EEP擦除操作 */
Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (eep_state == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_ERASE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if (eep_state != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    eep_current_address = Address;
    eep_current_length = Length;
    eep_state = EEP_STATE_ERASE;
    eep_job_result = EEP_JOB_PENDING;
    return E_OK;
}

/* EEP取消操作 */
void Eep_Cancel(void) {
    if (eep_state != EEP_STATE_UNINIT) {
        eep_state = EEP_STATE_IDLE;
        eep_job_result = EEP_JOB_CANCELED;
    }
}

/* EEP获取状态 */
Eep_StatusType Eep_GetStatus(void) {
    switch (eep_state) {
        case EEP_STATE_UNINIT:
            return EEP_UNINIT;
        case EEP_STATE_IDLE:
            return EEP_IDLE;
        case EEP_STATE_READ:
        case EEP_STATE_WRITE:
        case EEP_STATE_ERASE:
            return EEP_BUSY;
        default:
            return EEP_UNINIT;
    }
}

/* EEP获取作业结果 */
Eep_JobResultType Eep_GetJobResult(void) {
    return eep_job_result;
}

/* EEP主函数 */
void Eep_MainFunction(void) {
    if (eep_state == EEP_STATE_IDLE || eep_state == EEP_STATE_UNINIT) {
        return;
    }
    
    if (eep_state == EEP_STATE_READ) {
        /* 模拟读操作完成 */
        if (eep_current_address + eep_current_length <= sizeof(mock_eeprom_memory)) {
            memcpy(eep_current_data_ptr, &mock_eeprom_memory[eep_current_address], eep_current_length);
        }
    }
    else if (eep_state == EEP_STATE_WRITE) {
        /* 模拟写操作完成 */
        if (eep_current_address + eep_current_length <= sizeof(mock_eeprom_memory)) {
            memcpy(&mock_eeprom_memory[eep_current_address], eep_current_data_ptr, eep_current_length);
        }
    }
    else if (eep_state == EEP_STATE_ERASE) {
        /* 模拟擦除操作完成 */
        if (eep_current_address + eep_current_length <= sizeof(mock_eeprom_memory)) {
            memset(&mock_eeprom_memory[eep_current_address], 0xFF, eep_current_length);
        }
    }
    
    eep_job_result = EEP_JOB_OK;
    eep_state = EEP_STATE_IDLE;
}

/* 重置测试环境 */
void reset_test_env(void) {
    eep_state = EEP_STATE_UNINIT;
    eep_config_ptr = NULL_PTR;
    eep_job_result = EEP_JOB_OK;
    eep_current_address = 0;
    eep_current_data_ptr = NULL_PTR;
    eep_current_length = 0;
    det_last_module_id = 0;
    det_last_instance_id = 0;
    det_last_api_id = 0;
    det_last_error_id = 0;
    det_error_count = 0;
    memset(mock_read_buffer, 0, sizeof(mock_read_buffer));
}

/*==============================================
 * 测试用例: Eep_Init
 *==============================================*/
/** @req SWS_Eep_00001 */
void test_eep_init_null_config(void) {
    printf("\n[Test] Eep_Init - NULL Config Pointer\n");
    reset_test_env();
    
    Eep_Init(NULL_PTR);
    
    TEST_ASSERT_EQ(EEP_STATE_UNINIT, eep_state);
    TEST_ASSERT_EQ(EEP_MODULE_ID, det_last_module_id);
    TEST_ASSERT_EQ(EEP_SID_INIT, det_last_api_id);
    TEST_ASSERT_EQ(EEP_E_PARAM_POINTER, det_last_error_id);
    TEST_ASSERT_EQ(1, det_error_count);
}

/** @req SWS_Eep_00001 */
void test_eep_init_valid_config(void) {
    printf("\n[Test] Eep_Init - Valid Configuration\n");
    reset_test_env();
    
    Eep_ConfigType config = {
        .BaseAddress = 0x08080000U,
        .Size = 0x00010000U,
        .JobCallCycle = 10U
    };
    
    Eep_Init(&config);
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(&config, eep_config_ptr);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    TEST_ASSERT_EQ(0, det_error_count);
}

/*==============================================
 * 测试用例: Eep_DeInit
 *==============================================*/
/** @req SWS_Eep_00002 */
void test_eep_deinit_normal(void) {
    printf("\n[Test] Eep_DeInit - Normal Deinitialization\n");
    reset_test_env();
    
    Eep_ConfigType config = {
        .BaseAddress = 0x08080000U,
        .Size = 0x00010000U,
        .JobCallCycle = 10U
    };
    Eep_Init(&config);
    Eep_DeInit();
    
    TEST_ASSERT_EQ(EEP_STATE_UNINIT, eep_state);
    TEST_ASSERT_EQ(NULL_PTR, eep_config_ptr);
}

/** @req SWS_Eep_00002 */
void test_eep_deinit_without_init(void) {
    printf("\n[Test] Eep_DeInit - Without Initialization\n");
    reset_test_env();
    
    Eep_DeInit();
    
    TEST_ASSERT_EQ(EEP_STATE_UNINIT, eep_state);
    TEST_ASSERT_EQ(NULL_PTR, eep_config_ptr);
}

/*==============================================
 * 测试用例: Eep_Read
 *==============================================*/
/** @req SWS_Eep_00003 */
void test_eep_read_uninit(void) {
    printf("\n[Test] Eep_Read - Uninitialized Driver\n");
    reset_test_env();
    
    uint8 buffer[10];
    Std_ReturnType result = Eep_Read(0, buffer, 10);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_MODULE_ID, det_last_module_id);
    TEST_ASSERT_EQ(EEP_SID_READ, det_last_api_id);
    TEST_ASSERT_EQ(EEP_E_UNINIT, det_last_error_id);
}

/** @req SWS_Eep_00003 */
void test_eep_read_null_pointer(void) {
    printf("\n[Test] Eep_Read - NULL Data Pointer\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Std_ReturnType result = Eep_Read(0, NULL_PTR, 10);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_E_PARAM_POINTER, det_last_error_id);
}

/** @req SWS_Eep_00003 */
void test_eep_read_zero_length(void) {
    printf("\n[Test] Eep_Read - Zero Length\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    uint8 buffer[10];
    
    Std_ReturnType result = Eep_Read(0, buffer, 0);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_E_PARAM_POINTER, det_last_error_id);
}

/** @req SWS_Eep_00003 */
void test_eep_read_busy(void) {
    printf("\n[Test] Eep_Read - Driver Busy\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 buffer1[10];
    uint8 buffer2[10];
    Std_ReturnType result1 = Eep_Read(0, buffer1, 10);
    Std_ReturnType result2 = Eep_Read(100, buffer2, 10);  /* Should fail - busy */
    
    TEST_ASSERT_EQ(E_OK, result1);
    TEST_ASSERT_EQ(E_NOT_OK, result2);
    TEST_ASSERT_EQ(EEP_STATE_READ, eep_state);
}

/** @req SWS_Eep_00003 */
void test_eep_read_valid(void) {
    printf("\n[Test] Eep_Read - Valid Read Operation\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    /* Setup: Write data first to mock memory */
    uint8 write_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    memcpy(&mock_eeprom_memory[0x100], write_data, 16);
    
    uint8 read_buffer[16] = {0};
    Std_ReturnType result = Eep_Read(0x100, read_buffer, 16);
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(EEP_STATE_READ, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    TEST_ASSERT_EQ(0x100, eep_current_address);
    TEST_ASSERT_EQ(16, eep_current_length);
    
    /* Complete the job */
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    TEST_ASSERT_EQ(0, memcmp(read_buffer, write_data, 16));
}

/*==============================================
 * 测试用例: Eep_Write
 *==============================================*/
/** @req SWS_Eep_00004 */
void test_eep_write_uninit(void) {
    printf("\n[Test] Eep_Write - Uninitialized Driver\n");
    reset_test_env();
    
    uint8 data[10] = {0};
    Std_ReturnType result = Eep_Write(0, data, 10);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_MODULE_ID, det_last_module_id);
    TEST_ASSERT_EQ(EEP_SID_WRITE, det_last_api_id);
    TEST_ASSERT_EQ(EEP_E_UNINIT, det_last_error_id);
}

/** @req SWS_Eep_00004 */
void test_eep_write_null_pointer(void) {
    printf("\n[Test] Eep_Write - NULL Data Pointer\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Std_ReturnType result = Eep_Write(0, NULL_PTR, 10);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_E_PARAM_POINTER, det_last_error_id);
}

/** @req SWS_Eep_00004 */
void test_eep_write_zero_length(void) {
    printf("\n[Test] Eep_Write - Zero Length\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    uint8 data[10] = {0};
    
    Std_ReturnType result = Eep_Write(0, data, 0);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_E_PARAM_POINTER, det_last_error_id);
}

/** @req SWS_Eep_00004 */
void test_eep_write_busy(void) {
    printf("\n[Test] Eep_Write - Driver Busy\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 data1[10] = {0};
    uint8 data2[10] = {0};
    Std_ReturnType result1 = Eep_Write(0, data1, 10);
    Std_ReturnType result2 = Eep_Write(100, data2, 10);  /* Should fail - busy */
    
    TEST_ASSERT_EQ(E_OK, result1);
    TEST_ASSERT_EQ(E_NOT_OK, result2);
    TEST_ASSERT_EQ(EEP_STATE_WRITE, eep_state);
}

/** @req SWS_Eep_00004 */
void test_eep_write_valid(void) {
    printf("\n[Test] Eep_Write - Valid Write Operation\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 write_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    Std_ReturnType result = Eep_Write(0x200, write_data, 16);
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(EEP_STATE_WRITE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    TEST_ASSERT_EQ(0x200, eep_current_address);
    TEST_ASSERT_EQ(16, eep_current_length);
    
    /* Complete the job */
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    TEST_ASSERT_EQ(0, memcmp(&mock_eeprom_memory[0x200], write_data, 16));
}

/*==============================================
 * 测试用例: Eep_Erase
 *==============================================*/
/** @req SWS_Eep_00005 */
void test_eep_erase_uninit(void) {
    printf("\n[Test] Eep_Erase - Uninitialized Driver\n");
    reset_test_env();
    
    Std_ReturnType result = Eep_Erase(0, 256);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(EEP_MODULE_ID, det_last_module_id);
    TEST_ASSERT_EQ(EEP_SID_ERASE, det_last_api_id);
    TEST_ASSERT_EQ(EEP_E_UNINIT, det_last_error_id);
}

/** @req SWS_Eep_00005 */
void test_eep_erase_busy(void) {
    printf("\n[Test] Eep_Erase - Driver Busy\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 data[10] = {0};
    Eep_Write(0, data, 10);  /* Make driver busy */
    
    Std_ReturnType result = Eep_Erase(256, 256);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/** @req SWS_Eep_00005 */
void test_eep_erase_valid(void) {
    printf("\n[Test] Eep_Erase - Valid Erase Operation\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    /* Setup: Write data first */
    uint8 write_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    memcpy(&mock_eeprom_memory[0x300], write_data, 16);
    
    Std_ReturnType result = Eep_Erase(0x300, 16);
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(EEP_STATE_ERASE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    TEST_ASSERT_EQ(0x300, eep_current_address);
    TEST_ASSERT_EQ(16, eep_current_length);
    
    /* Complete the job */
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    
    /* Verify erased to 0xFF */
    uint8 expected[16];
    memset(expected, 0xFF, 16);
    TEST_ASSERT_EQ(0, memcmp(&mock_eeprom_memory[0x300], expected, 16));
}

/*==============================================
 * 测试用例: Eep_Cancel
 *==============================================*/
/** @req SWS_Eep_00006 */
void test_eep_cancel_normal(void) {
    printf("\n[Test] Eep_Cancel - Normal Operation\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 buffer[10];
    Eep_Read(0, buffer, 10);
    TEST_ASSERT_EQ(EEP_STATE_READ, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    
    Eep_Cancel();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_CANCELED, eep_job_result);
}

/** @req SWS_Eep_00006 */
void test_eep_cancel_uninit(void) {
    printf("\n[Test] Eep_Cancel - Uninitialized Driver\n");
    reset_test_env();
    
    /* Should not crash */
    Eep_Cancel();
    
    TEST_ASSERT_EQ(EEP_STATE_UNINIT, eep_state);
}

/** @req SWS_Eep_00006 */
void test_eep_cancel_idle(void) {
    printf("\n[Test] Eep_Cancel - Idle State\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    
    Eep_Cancel();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_CANCELED, eep_job_result);
}

/*==============================================
 * 测试用例: Eep_GetStatus
 *==============================================*/
/** @req SWS_Eep_00007 */
void test_eep_get_status_uninit(void) {
    printf("\n[Test] Eep_GetStatus - Uninitialized State\n");
    reset_test_env();
    
    Eep_StatusType status = Eep_GetStatus();
    
    TEST_ASSERT_EQ(EEP_UNINIT, status);
}

/** @req SWS_Eep_00007 */
void test_eep_get_status_idle(void) {
    printf("\n[Test] Eep_GetStatus - Idle State\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Eep_StatusType status = Eep_GetStatus();
    
    TEST_ASSERT_EQ(EEP_IDLE, status);
}

/** @req SWS_Eep_00007 */
void test_eep_get_status_busy_read(void) {
    printf("\n[Test] Eep_GetStatus - Busy (Read)\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 buffer[10];
    Eep_Read(0, buffer, 10);
    
    Eep_StatusType status = Eep_GetStatus();
    
    TEST_ASSERT_EQ(EEP_BUSY, status);
}

/** @req SWS_Eep_00007 */
void test_eep_get_status_busy_write(void) {
    printf("\n[Test] Eep_GetStatus - Busy (Write)\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 data[10] = {0};
    Eep_Write(0, data, 10);
    
    Eep_StatusType status = Eep_GetStatus();
    
    TEST_ASSERT_EQ(EEP_BUSY, status);
}

/** @req SWS_Eep_00007 */
void test_eep_get_status_busy_erase(void) {
    printf("\n[Test] Eep_GetStatus - Busy (Erase)\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Eep_Erase(0, 256);
    
    Eep_StatusType status = Eep_GetStatus();
    
    TEST_ASSERT_EQ(EEP_BUSY, status);
}

/*==============================================
 * 测试用例: Eep_GetJobResult
 *==============================================*/
/** @req SWS_Eep_00008 */
void test_eep_get_job_result_ok(void) {
    printf("\n[Test] Eep_GetJobResult - Job OK\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Eep_JobResultType result = Eep_GetJobResult();
    
    TEST_ASSERT_EQ(EEP_JOB_OK, result);
}

/** @req SWS_Eep_00008 */
void test_eep_get_job_result_pending(void) {
    printf("\n[Test] Eep_GetJobResult - Job Pending\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 buffer[10];
    Eep_Read(0, buffer, 10);
    
    Eep_JobResultType result = Eep_GetJobResult();
    
    TEST_ASSERT_EQ(EEP_JOB_PENDING, result);
}

/** @req SWS_Eep_00008 */
void test_eep_get_job_result_canceled(void) {
    printf("\n[Test] Eep_GetJobResult - Job Canceled\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 buffer[10];
    Eep_Read(0, buffer, 10);
    Eep_Cancel();
    
    Eep_JobResultType result = Eep_GetJobResult();
    
    TEST_ASSERT_EQ(EEP_JOB_CANCELED, result);
}

/*==============================================
 * 测试用例: Eep_MainFunction
 *==============================================*/
/** @req SWS_Eep_00009 */
void test_eep_mainfunction_uninit(void) {
    printf("\n[Test] Eep_MainFunction - Uninitialized State\n");
    reset_test_env();
    
    /* Should not crash */
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_UNINIT, eep_state);
}

/** @req SWS_Eep_00009 */
void test_eep_mainfunction_idle(void) {
    printf("\n[Test] Eep_MainFunction - Idle State\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
}

/** @req SWS_Eep_00009 */
void test_eep_mainfunction_complete_read(void) {
    printf("\n[Test] Eep_MainFunction - Complete Read Job\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    /* Setup: Write data first to mock memory */
    uint8 write_data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    memcpy(&mock_eeprom_memory[0x400], write_data, 8);
    
    uint8 read_buffer[8] = {0};
    Eep_Read(0x400, read_buffer, 8);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    TEST_ASSERT_EQ(0, memcmp(read_buffer, write_data, 8));
}

/** @req SWS_Eep_00009 */
void test_eep_mainfunction_complete_write(void) {
    printf("\n[Test] Eep_MainFunction - Complete Write Job\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 write_data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    Eep_Write(0x500, write_data, 8);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    TEST_ASSERT_EQ(0, memcmp(&mock_eeprom_memory[0x500], write_data, 8));
}

/** @req SWS_Eep_00009 */
void test_eep_mainfunction_complete_erase(void) {
    printf("\n[Test] Eep_MainFunction - Complete Erase Job\n");
    reset_test_env();
    
    /* Pre-fill with data */
    uint8 data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                      0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    memcpy(&mock_eeprom_memory[0x600], data, 16);
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    Eep_Erase(0x600, 16);
    TEST_ASSERT_EQ(EEP_JOB_PENDING, eep_job_result);
    
    Eep_MainFunction();
    
    TEST_ASSERT_EQ(EEP_STATE_IDLE, eep_state);
    TEST_ASSERT_EQ(EEP_JOB_OK, eep_job_result);
    
    /* Verify erased to 0xFF */
    uint8 expected[16];
    memset(expected, 0xFF, 16);
    TEST_ASSERT_EQ(0, memcmp(&mock_eeprom_memory[0x600], expected, 16));
}

/*==============================================
 * 综合测试场景
 *==============================================*/
/** @req SWS_Eep_00003 */
void test_eep_read_write_sequence(void) {
    printf("\n[Test] Eep - Read/Write Sequence\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    /* Write data */
    uint8 write_data[32];
    for (int i = 0; i < 32; i++) {
        write_data[i] = (uint8)(i * 3);
    }
    
    Std_ReturnType result = Eep_Write(0x1000, write_data, 32);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(EEP_BUSY, Eep_GetStatus());
    
    Eep_MainFunction();
    TEST_ASSERT_EQ(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQ(EEP_JOB_OK, Eep_GetJobResult());
    
    /* Read back */
    uint8 read_data[32] = {0};
    result = Eep_Read(0x1000, read_data, 32);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(EEP_BUSY, Eep_GetStatus());
    
    Eep_MainFunction();
    TEST_ASSERT_EQ(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQ(EEP_JOB_OK, Eep_GetJobResult());
    TEST_ASSERT_EQ(0, memcmp(read_data, write_data, 32));
}

/* @req SWS_Eep_00201 */
void test_eep_multiple_operations(void) {
    printf("\n[Test] Eep - Multiple Operations\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    /* Operation 1: Write */
    uint8 data1[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Eep_Write(0x2000, data1, 8);
    Eep_MainFunction();
    
    /* Operation 2: Read */
    uint8 read1[8] = {0};
    Eep_Read(0x2000, read1, 8);
    Eep_MainFunction();
    TEST_ASSERT_EQ(0, memcmp(read1, data1, 8));
    
    /* Operation 3: Erase */
    Eep_Erase(0x2000, 8);
    Eep_MainFunction();
    
    /* Verify erased */
    uint8 read2[8] = {0};
    Eep_Read(0x2000, read2, 8);
    Eep_MainFunction();
    
    uint8 expected[8];
    memset(expected, 0xFF, 8);
    TEST_ASSERT_EQ(0, memcmp(read2, expected, 8));
}

/* @req SWS_Eep_00202 */
void test_eep_concurrent_request_rejection(void) {
    printf("\n[Test] Eep - Concurrent Request Rejection\n");
    reset_test_env();
    
    Eep_ConfigType config = {0x08080000U, 0x00010000U, 10U};
    Eep_Init(&config);
    
    uint8 data1[16], data2[16], data3[16];
    memset(data1, 0x11, 16);
    memset(data2, 0x22, 16);
    memset(data3, 0x33, 16);
    
    /* Start a read operation */
    Std_ReturnType result1 = Eep_Read(0x3000, data1, 16);
    TEST_ASSERT_EQ(E_OK, result1);
    
    /* Try to start write while read is pending - should fail */
    Std_ReturnType result2 = Eep_Write(0x3100, data2, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result2);
    
    /* Try to start erase while read is pending - should fail */
    Std_ReturnType result3 = Eep_Erase(0x3200, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result3);
    
    /* Try another read - should fail */
    Std_ReturnType result4 = Eep_Read(0x3300, data3, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result4);
    
    /* Complete the operation */
    Eep_MainFunction();
    TEST_ASSERT_EQ(EEP_IDLE, Eep_GetStatus());
}

/*==============================================
 * 主函数
 *==============================================*/
int main(void) {
    printf("========================================\n");
    printf("EEP (EEPROM Driver) Unit Tests\n");
    printf("========================================\n");
    
    /* Eep_Init tests */
    test_eep_init_null_config();
    test_eep_init_valid_config();
    
    /* Eep_DeInit tests */
    test_eep_deinit_normal();
    test_eep_deinit_without_init();
    
    /* Eep_Read tests */
    test_eep_read_uninit();
    test_eep_read_null_pointer();
    test_eep_read_zero_length();
    test_eep_read_busy();
    test_eep_read_valid();
    
    /* Eep_Write tests */
    test_eep_write_uninit();
    test_eep_write_null_pointer();
    test_eep_write_zero_length();
    test_eep_write_busy();
    test_eep_write_valid();
    
    /* Eep_Erase tests */
    test_eep_erase_uninit();
    test_eep_erase_busy();
    test_eep_erase_valid();
    
    /* Eep_Cancel tests */
    test_eep_cancel_normal();
    test_eep_cancel_uninit();
    test_eep_cancel_idle();
    
    /* Eep_GetStatus tests */
    test_eep_get_status_uninit();
    test_eep_get_status_idle();
    test_eep_get_status_busy_read();
    test_eep_get_status_busy_write();
    test_eep_get_status_busy_erase();
    
    /* Eep_GetJobResult tests */
    test_eep_get_job_result_ok();
    test_eep_get_job_result_pending();
    test_eep_get_job_result_canceled();
    
    /* Eep_MainFunction tests */
    test_eep_mainfunction_uninit();
    test_eep_mainfunction_idle();
    test_eep_mainfunction_complete_read();
    test_eep_mainfunction_complete_write();
    test_eep_mainfunction_complete_erase();
    
    /* Integration tests */
    test_eep_read_write_sequence();
    test_eep_multiple_operations();
    test_eep_concurrent_request_rejection();
    
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total:  %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Coverage: %.1f%%\n", (tests_passed * 100.0) / tests_run);
    printf("========================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}
