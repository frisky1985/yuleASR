/**
 * @file RamTst.c
 * @brief RAM Test Implementation
 */

#include "RamTst.h"
#include "RamTst_Cfg.h"
#include "Det.h"

typedef enum {
    RAMTST_STATE_UNINIT = 0,
    RAMTST_STATE_IDLE,
    RAMTST_STATE_RUNNING
} RamTst_StateType;

static RamTst_StateType RamTst_State = RAMTST_STATE_UNINIT;
static const RamTst_ConfigType* RamTst_ConfigPtr = NULL_PTR;
static RamTst_TestResultType RamTst_Result = RAMTST_RESULT_NOT_TESTED;

void RamTst_Init(const RamTst_ConfigType* ConfigPtr) {
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(RAMTST_MODULE_ID, 0U, RAMTST_SID_INIT, RAMTST_E_PARAM_POINTER);
        return;
    }
#endif
    RamTst_ConfigPtr = ConfigPtr;
    RamTst_State = RAMTST_STATE_IDLE;
    RamTst_Result = RAMTST_RESULT_NOT_TESTED;
}

void RamTst_DeInit(void) {
    RamTst_State = RAMTST_STATE_UNINIT;
    RamTst_ConfigPtr = NULL_PTR;
}

Std_ReturnType RamTst_Run(void) {
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (RamTst_State == RAMTST_STATE_UNINIT) {
        Det_ReportError(RAMTST_MODULE_ID, 0U, RAMTST_SID_RUN, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if (RamTst_State == RAMTST_STATE_RUNNING) {
        return E_NOT_OK;
    }
    RamTst_State = RAMTST_STATE_RUNNING;
    RamTst_Result = RAMTST_RESULT_NOT_TESTED;
    return E_OK;
}

void RamTst_Stop(void) {
    if (RamTst_State == RAMTST_STATE_RUNNING) {
        RamTst_State = RAMTST_STATE_IDLE;
    }
}

RamTst_TestResultType RamTst_GetTestResult(void) {
    return RamTst_Result;
}

RamTst_StatusType RamTst_GetTestStatus(void) {
    switch (RamTst_State) {
        case RAMTST_STATE_UNINIT:
            return RAMTST_STATUS_UNINIT;
        case RAMTST_STATE_IDLE:
            return RAMTST_STATUS_IDLE;
        case RAMTST_STATE_RUNNING:
            return RAMTST_STATUS_RUNNING;
        default:
            return RAMTST_STATUS_UNINIT;
    }
}

void RamTst_MainFunction(void) {
    if (RamTst_State != RAMTST_STATE_RUNNING) {
        return;
    }
    /* Simulate test completion - always pass for now */
    RamTst_Result = RAMTST_RESULT_OK;
    RamTst_State = RAMTST_STATE_IDLE;
}
