/*
 * @file main.c
 * @brief 商用车网关示例 - J1939到CAN转发
 * 
 * 功能:
 * - 接收J1939广播消息
 * - 转发到CAN网络
 * - OBD-II数据读取
 */

#include "Com.h"
#include "J1939Tp.h"
#include "Dcm.h"
#include "Dcm_OBD.h"
#include "Os.h"

#define GATEWAY_TASK_CYCLE  10  /* ms */

void Gateway_Task(void) {
    /* COM主函数 */
    Com_MainFunctionTx();
    Com_MainFunctionRx();
    
    /* J1939处理 */
    J1939Tp_MainFunction();
    
    /* 网关逻辑 */
    ProcessJ1939ToCanGateway();
    
    /* OBD读取 */
    ReadOBDData();
}

int main(void) {
    /* 初始化 */
    Os_Init();
    Com_Init(NULL_PTR);
    J1939Tp_Init(NULL_PTR);
    Dcm_Init(NULL_PTR);
    
    /* 启动OS */
    Os_Start();
    
    return 0;
}
