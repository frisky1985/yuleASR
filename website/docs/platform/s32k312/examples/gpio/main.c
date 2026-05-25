/**
 * S32K312 GPIO LED 闪烁示例
 * 
 * 功能: 控制PTB15引脚上的LED每500ms闪烁一次
 * 学习点: 时钟配置、GPIO配置、定时延时
 */

#include "Mcu.h"
#include "Port.h"
#include "Dio.h"
#include "McalLib.h"

/* 定时定义 - 500ms @ 80MHz */
#define DELAY_COUNT_500MS   40000000UL

/**
 * @brief 软件延时函数
 */
static void Delay(volatile uint32 count)
{
    while(count--);
}

/**
 * @brief 系统初始化
 */
static void SystemInit(void)
{
    /* 初始化Mcu模块 */
    Mcu_Init(&Mcu_Config);
    
    /* 初始化Port模块 */
    Port_Init(&Port_Config);
    
    /* 初始化Dio模块 */
    Dio_Init(&Dio_Config);
}

int main(void)
{
    /* 系统初始化 */
    SystemInit();
    
    /* 设置Mcu模式 - RUN */
    Mcu_SetMode(MCU_MODE_RUN);
    
    /* 主循环 */
    while(1)
    {
        /* 翻转LED状态 */
        Dio_FlipChannel(DIO_CHANNEL_LED_RED);
        
        /* 延时500ms */
        Delay(DELAY_COUNT_500MS);
    }
    
    return 0;
}
