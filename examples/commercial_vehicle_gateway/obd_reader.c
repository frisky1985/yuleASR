/*
 * @file obd_reader.c
 * @brief OBD-II数据读取示例
 */

#include "Dcm_OBD.h"
#include "Com.h"

void ReadOBDData(void) {
    /* 读取发动机转速 */
    uint8 rpmRequest[] = {0x01, 0x0C};
    uint8 rpmResponse[8];
    uint16 responseLen;
    
    Dcm_OBD_ProcessRequest(rpmRequest, sizeof(rpmRequest), 
                           rpmResponse, &responseLen);
    
    if (responseLen > 2) {
        uint16 rpm = (rpmResponse[2] << 8) | rpmResponse[3];
        rpm = rpm / 4;  /* OBD-II RPM计算公式 */
        
        /* 发送到COM */
        Com_SendSignal(0, &rpm);
    }
}
