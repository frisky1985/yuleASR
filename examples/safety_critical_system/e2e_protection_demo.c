/*
 * @file e2e_protection_demo.c
 * @brief E2E保护演示 - ASIL-D级安全通信
 */

#include "E2E.h"
#include "Com.h"
#include "Os.h"

/* E2E配置 */
static E2E_P05ConfigType e2eConfig;
static E2E_P05ProtectStateType protectState;
static E2E_P05CheckStateType checkState;

void InitE2EProtection(void) {
    /* Profile 5配置 - 最高安全级别 */
    e2eConfig.CRCOffset = 0;
    e2eConfig.CounterOffset = 32;
    e2eConfig.DataID = 0x12345678;
    e2eConfig.DataLength = 64;
    e2eConfig.MaxDeltaCounterInit = 1;
    
    memset(&protectState, 0, sizeof(protectState));
    memset(&checkState, 0, sizeof(checkState));
}

void SendSafetyCriticalData(void) {
    uint8 data[64] = {0};
    
    /* 填充关键数据 */
    data[8] = GetBrakePressure();
    data[9] = GetSteeringAngle();
    data[10] = GetVehicleSpeed();
    
    /* E2E保护 */
    E2E_P05Protect(&e2eConfig, &protectState, data);
    
    /* 通过COM发送 */
    Com_SendSignal(SAFETY_SIGNAL_ID, data);
}

void ReceiveSafetyCriticalData(void) {
    uint8 data[64];
    E2E_CheckResultType checkResult;
    
    /* 从COM接收 */
    Com_ReceiveSignal(SAFETY_SIGNAL_ID, data);
    
    /* E2E检查 */
    E2E_P05Check(&e2eConfig, &checkState, data, &checkResult);
    
    if (checkResult == E2E_P05STATUS_OK) {
        /* 数据有效，处理 */
        ProcessSafetyData(data);
    } else {
        /* 数据无效，触发安全响应 */
        TriggerSafetyFault(checkResult);
    }
}
