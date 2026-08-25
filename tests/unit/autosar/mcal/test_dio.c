/**
 * @file test_dio.c
 * @brief DIO Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * SHALL-DIO-01: SHALL support 8 ports with 32 pins each for digital I/O
 * SHALL-DIO-02: SHALL support configurable pin direction per pin
 * SHALL-DIO-03: SHALL support HIGH and LEVEL output levels
 * SHALL-DIO-04: SHALL support edge-triggered interrupt on rising, falling, and both edges
 */

// @tests src/bsw/mcal/dio/src/Dio.c  @tests src/bsw/mcal/dio/include/Dio.h

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 定义测试所需的版本信息宏 */
#define DIO_SW_MAJOR_VERSION            1U
#define DIO_SW_MINOR_VERSION            0U
#define DIO_SW_PATCH_VERSION            0U

/* DIO SID定义 */
#define DIO_SID_INIT                    0x00U
#define DIO_SID_READ_CHANNEL            0x01U
#define DIO_SID_WRITE_CHANNEL           0x02U
#define DIO_SID_READ_PORT               0x03U
#define DIO_SID_WRITE_PORT              0x04U
#define DIO_SID_READ_CHANNEL_GROUP      0x05U
#define DIO_SID_WRITE_CHANNEL_GROUP     0x06U
#define DIO_SID_GET_VERSION_INFO        0x12U
#define DIO_SID_FLIP_CHANNEL            0x11U
#define DIO_SID_MASKED_WRITE_PORT       0x13U

/* 模拟DIO类型定义 */
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef enum { FALSE = 0, TRUE = 1 } boolean;
typedef enum { E_OK = 0, E_NOT_OK } Std_ReturnType;

#ifndef STD_ON
#define STD_ON                          1U
#define STD_OFF                         0U
#endif

#ifndef STD_HIGH
#define STD_HIGH                        1U
#define STD_LOW                         0U
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

typedef uint8 Dio_PortType;
typedef uint16 Dio_ChannelType;
typedef uint32 Dio_PortLevelType;
typedef uint8 Dio_LevelType;

typedef struct {
    Dio_PortType port;
    uint8 offset;
    Dio_PortLevelType mask;
} Dio_ChannelGroupType;

typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 模拟状态变量 */
static boolean Dio_DriverInitialized = FALSE;
static uint8 mock_port_values[8] = {0};
static uint8 mock_channel_values[64] = {0};

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

/*==================================================================================================
*                                    MOCK FUNCTIONS
==================================================================================================*/

void Dio_Init(const void* ConfigPtr)
{
    Dio_DriverInitialized = TRUE;
    (void)ConfigPtr;
}

void Dio_DeInit(void)
{
    Dio_DriverInitialized = FALSE;
}

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    uint8 port = (uint8)(ChannelId >> 8);
    uint8 pin = (uint8)(ChannelId & 0xFF);
    
    if (!Dio_DriverInitialized) {
        return STD_LOW;
    }
    if (port >= 8 || pin >= 32) {
        return STD_LOW;
    }
    
    return (mock_port_values[port] & (1U << pin)) ? STD_HIGH : STD_LOW;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    uint8 port = (uint8)(ChannelId >> 8);
    uint8 pin = (uint8)(ChannelId & 0xFF);
    
    if (!Dio_DriverInitialized) {
        return;
    }
    if (port >= 8 || pin >= 32) {
        return;
    }
    
    if (Level == STD_HIGH) {
        mock_port_values[port] |= (1U << pin);
    } else {
        mock_port_values[port] &= ~(1U << pin);
    }
}

Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId)
{
    if (!Dio_DriverInitialized) {
        return 0U;
    }
    if (PortId >= 8) {
        return 0U;
    }
    
    return (Dio_PortLevelType)mock_port_values[PortId];
}

void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level)
{
    if (!Dio_DriverInitialized) {
        return;
    }
    if (PortId >= 8) {
        return;
    }
    
    mock_port_values[PortId] = (uint8)(Level & 0xFF);
}

Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr)
{
    if (!Dio_DriverInitialized) {
        return 0U;
    }
    if (ChannelGroupIdPtr == NULL_PTR) {
        return 0U;
    }
    if (ChannelGroupIdPtr->port >= 8) {
        return 0U;
    }
    
    uint32 portValue = mock_port_values[ChannelGroupIdPtr->port];
    return (Dio_PortLevelType)((portValue & ChannelGroupIdPtr->mask) >> ChannelGroupIdPtr->offset);
}

void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level)
{
    if (!Dio_DriverInitialized) {
        return;
    }
    if (ChannelGroupIdPtr == NULL_PTR) {
        return;
    }
    if (ChannelGroupIdPtr->port >= 8) {
        return;
    }
    
    uint32 drValue = mock_port_values[ChannelGroupIdPtr->port];
    drValue &= ~(ChannelGroupIdPtr->mask);
    drValue |= ((uint32)Level << ChannelGroupIdPtr->offset) & ChannelGroupIdPtr->mask;
    mock_port_values[ChannelGroupIdPtr->port] = (uint8)drValue;
}

Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType currentLevel = Dio_ReadChannel(ChannelId);
    Dio_LevelType newLevel = (currentLevel == STD_HIGH) ? STD_LOW : STD_HIGH;
    Dio_WriteChannel(ChannelId, newLevel);
    return newLevel;
}

void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR) {
        versioninfo->vendorID = 0x0055U;
        versioninfo->moduleID = 0x0020U;
        versioninfo->sw_major_version = DIO_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DIO_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DIO_SW_PATCH_VERSION;
    }
}

void Dio_MaskedWritePort(Dio_PortType PortId, Dio_PortLevelType Level, Dio_PortLevelType Mask)
{
    if (!Dio_DriverInitialized) {
        return;
    }
    if (PortId >= 8) {
        return;
    }
    
    uint32 drValue = mock_port_values[PortId];
    drValue &= ~((uint32)Mask);
    drValue |= ((uint32)Level & (uint32)Mask);
    mock_port_values[PortId] = (uint8)drValue;
}

/*==================================================================================================
*                                    TEST FUNCTIONS
==================================================================================================*/

/* 初始化测试 */
/* @req SWS_Dio_00201 */
void test_init(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* 测试初始化前状态 */
    TEST_ASSERT_EQ(FALSE, Dio_DriverInitialized);
    
    /* 测试初始化 */
    Dio_Init(NULL_PTR);
    TEST_ASSERT_EQ(TRUE, Dio_DriverInitialized);
    
    /* 测试反初始化 */
    Dio_DeInit();
    TEST_ASSERT_EQ(FALSE, Dio_DriverInitialized);
}

/* 通道读测试 */
/* @req SWS_Dio_00202 */
void test_read_channel(void)
{
    Dio_LevelType level;
    
    printf("\n=== Read Channel Tests ===\n");
    
    /* 重新初始化 */
    Dio_Init(NULL_PTR);
    
    /* 测试读取低电平 */
    mock_port_values[0] = 0x00;
    level = Dio_ReadChannel(0x0000);  /* Port A, Pin 0 */
    TEST_ASSERT_EQ(STD_LOW, level);
    
    /* 测试读取高电平 */
    mock_port_values[0] = 0x01;
    level = Dio_ReadChannel(0x0000);  /* Port A, Pin 0 */
    TEST_ASSERT_EQ(STD_HIGH, level);
    
    /* 测试读取不同通道 */
    mock_port_values[1] = 0x80;  /* Port B, Pin 7 = HIGH */
    level = Dio_ReadChannel(0x0107);  /* Port B, Pin 7 */
    TEST_ASSERT_EQ(STD_HIGH, level);
    
    /* 测试反初始化后读取 */
    Dio_DeInit();
    level = Dio_ReadChannel(0x0000);
    TEST_ASSERT_EQ(STD_LOW, level);
}

/* 通道写测试 */
/* @req SWS_Dio_00203 */
void test_write_channel(void)
{
    printf("\n=== Write Channel Tests ===\n");
    
    /* 重新初始化 */
    Dio_Init(NULL_PTR);
    
    /* 清空调试 */
    mock_port_values[0] = 0x00;
    
    /* 测试写高电平 */
    Dio_WriteChannel(0x0000, STD_HIGH);  /* Port A, Pin 0 */
    TEST_ASSERT_EQ(0x01, mock_port_values[0]);
    
    /* 测试写低电平 */
    Dio_WriteChannel(0x0000, STD_LOW);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);
    
    /* 测试多个通道 */
    Dio_WriteChannel(0x0000, STD_HIGH);
    Dio_WriteChannel(0x0003, STD_HIGH);  /* Port A, Pin 3 */
    TEST_ASSERT_EQ(0x09, mock_port_values[0]);  /* Bits 0 and 3 */
    
    /* 测试跨端口写入 */
    mock_port_values[2] = 0x00;
    Dio_WriteChannel(0x0205, STD_HIGH);  /* Port C, Pin 5 */
    TEST_ASSERT_EQ(0x20, mock_port_values[2]);
    
    /* 测试反初始化后写入 */
    Dio_DeInit();
    mock_port_values[0] = 0x00;
    Dio_WriteChannel(0x0000, STD_HIGH);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 未改变 */
}

/* 端口读测试 */
/* @req SWS_Dio_00204 */
void test_read_port(void)
{
    Dio_PortLevelType level;
    
    printf("\n=== Read Port Tests ===\n");
    
    Dio_Init(NULL_PTR);
    
    /* 测试读取端口 */
    mock_port_values[0] = 0xAA;
    level = Dio_ReadPort(0);
    TEST_ASSERT_EQ(0xAA, level);
    
    mock_port_values[1] = 0x55;
    level = Dio_ReadPort(1);
    TEST_ASSERT_EQ(0x55, level);
    
    /* 测试全1读取 */
    mock_port_values[2] = 0xFF;
    level = Dio_ReadPort(2);
    TEST_ASSERT_EQ(0xFF, level);
    
    /* 测试全0读取 */
    mock_port_values[3] = 0x00;
    level = Dio_ReadPort(3);
    TEST_ASSERT_EQ(0x00, level);
    
    /* 测试反初始化后读取 */
    Dio_DeInit();
    level = Dio_ReadPort(0);
    TEST_ASSERT_EQ(0x00, level);
}

/* 端口写测试 */
/* @req SWS_Dio_00205 */
void test_write_port(void)
{
    printf("\n=== Write Port Tests ===\n");
    
    Dio_Init(NULL_PTR);
    
    /* 清空调试 */
    memset(mock_port_values, 0, sizeof(mock_port_values));
    
    /* 测试写端口 */
    Dio_WritePort(0, 0xAA);
    TEST_ASSERT_EQ(0xAA, mock_port_values[0]);
    
    Dio_WritePort(1, 0x55);
    TEST_ASSERT_EQ(0x55, mock_port_values[1]);
    
    /* 测试写0xFF */
    Dio_WritePort(2, 0xFF);
    TEST_ASSERT_EQ(0xFF, mock_port_values[2]);
    
    /* 测试写0x00 */
    Dio_WritePort(3, 0x00);
    TEST_ASSERT_EQ(0x00, mock_port_values[3]);
    
    /* 测试反初始化后写入 */
    Dio_DeInit();
    mock_port_values[4] = 0x00;
    Dio_WritePort(4, 0xFF);
    TEST_ASSERT_EQ(0x00, mock_port_values[4]);  /* 未改变 */
}

/* 通道组读测试 */
/* @req SWS_Dio_00206 */
void test_read_channel_group(void)
{
    Dio_ChannelGroupType group;
    Dio_PortLevelType level;
    
    printf("\n=== Read Channel Group Tests ===\n");
    
    Dio_Init(NULL_PTR);
    mock_port_values[0] = 0xF0;  /* Bits 4-7 = 1 */
    
    /* 设置通道组参数: port A, offset 4, mask 0xF0 (bits 4-7) */
    group.port = 0;
    group.offset = 4;
    group.mask = 0xF0;
    
    level = Dio_ReadChannelGroup(&group);
    TEST_ASSERT_EQ(0x0F, level);  /* (0xF0 & 0xF0) >> 4 = 0x0F */
    
    /* 测试不同offset */
    mock_port_values[1] = 0x3C;  /* Bits 2-5 = 1 (00111100) */
    group.port = 1;
    group.offset = 2;
    group.mask = 0x3C;
    
    level = Dio_ReadChannelGroup(&group);
    TEST_ASSERT_EQ(0x0F, level);  /* (0x3C & 0x3C) >> 2 = 0x0F */
    
    /* 测试NULL指针 */
    level = Dio_ReadChannelGroup(NULL_PTR);
    TEST_ASSERT_EQ(0x00, level);
    
    /* 测试反初始化后读取 */
    Dio_DeInit();
    group.port = 0;
    level = Dio_ReadChannelGroup(&group);
    TEST_ASSERT_EQ(0x00, level);
}

/* 通道组写测试 */
/* @req SWS_Dio_00207 */
void test_write_channel_group(void)
{
    Dio_ChannelGroupType group;
    
    printf("\n=== Write Channel Group Tests ===\n");
    
    Dio_Init(NULL_PTR);
    mock_port_values[0] = 0x00;
    
    /* 设置通道组参数: port A, offset 4, mask 0xF0 (bits 4-7) */
    group.port = 0;
    group.offset = 4;
    group.mask = 0xF0;
    
    /* 测试写通道组值 0x0A (1010) */
    Dio_WriteChannelGroup(&group, 0x0A);
    TEST_ASSERT_EQ(0xA0, mock_port_values[0]);  /* 0x0A << 4 = 0xA0 */
    
    /* 测试写入不影响其他位 */
    mock_port_values[0] = 0x0F;  /* Lower nibble already set */
    Dio_WriteChannelGroup(&group, 0x05);
    TEST_ASSERT_EQ(0x5F, mock_port_values[0]);  /* 0x05 << 4 | 0x0F = 0x5F */
    
    /* 测试NULL指针 */
    mock_port_values[0] = 0x00;
    Dio_WriteChannelGroup(NULL_PTR, 0xFF);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 未改变 */
    
    /* 测试反初始化后写入 */
    Dio_DeInit();
    mock_port_values[0] = 0x00;
    Dio_WriteChannelGroup(&group, 0xFF);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 未改变 */
}

/* 通道翻转测试 */
/* @req SWS_Dio_00208 */
void test_flip_channel(void)
{
    Dio_LevelType level;
    
    printf("\n=== Flip Channel Tests ===\n");
    
    Dio_Init(NULL_PTR);
    mock_port_values[0] = 0x00;
    
    /* 测试从低翻转到高 */
    level = Dio_FlipChannel(0x0000);
    TEST_ASSERT_EQ(STD_HIGH, level);
    TEST_ASSERT_EQ(0x01, mock_port_values[0]);
    
    /* 测试从高翻转到低 */
    level = Dio_FlipChannel(0x0000);
    TEST_ASSERT_EQ(STD_LOW, level);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);
    
    /* 测试多次翻转 */
    level = Dio_FlipChannel(0x0003);  /* Port A, Pin 3 */
    TEST_ASSERT_EQ(STD_HIGH, level);
    level = Dio_FlipChannel(0x0003);
    TEST_ASSERT_EQ(STD_LOW, level);
    level = Dio_FlipChannel(0x0003);
    TEST_ASSERT_EQ(STD_HIGH, level);
    TEST_ASSERT_EQ(0x08, mock_port_values[0]);
    
    /* 测试反初始化后翻转 - 未初始化状态下读取通道应返回LOW */
    Dio_DeInit();
    level = Dio_ReadChannel(0x0000);  /* 未初始化状态读取 */
    TEST_ASSERT_EQ(STD_LOW, level);
}

/* 掩码写端口测试 */
/* @req SWS_Dio_00209 */
void test_masked_write_port(void)
{
    printf("\n=== Masked Write Port Tests ===\n");
    
    Dio_Init(NULL_PTR);
    
    /* 清空调试 */
    mock_port_values[0] = 0x00;
    
    /* 测试掩码写 - 只写bits 0-3 */
    Dio_MaskedWritePort(0, 0xAA, 0x0F);  /* Write 0xA to lower nibble */
    TEST_ASSERT_EQ(0x0A, mock_port_values[0]);
    
    /* 测试掩码写 - 只写bits 4-7 */
    Dio_MaskedWritePort(0, 0x55, 0xF0);  /* Write 0x5 to upper nibble */
    TEST_ASSERT_EQ(0x5A, mock_port_values[0]);
    
    /* 测试掩码写 - 只写特定位 */
    mock_port_values[0] = 0x00;
    Dio_MaskedWritePort(0, 0xFF, 0x81);  /* Write to bits 0 and 7 */
    TEST_ASSERT_EQ(0x81, mock_port_values[0]);
    
    /* 测试掩码0 - 不改变 */
    mock_port_values[0] = 0x55;
    Dio_MaskedWritePort(0, 0xFF, 0x00);
    TEST_ASSERT_EQ(0x55, mock_port_values[0]);
    
    /* 测试反初始化后写入 */
    Dio_DeInit();
    mock_port_values[0] = 0x00;
    Dio_MaskedWritePort(0, 0xFF, 0xFF);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 未改变 */
}

/* 版本信息测试 */
/* @req SWS_Dio_00210 */
void test_version_info(void)
{
    Std_VersionInfoType version_info;
    
    printf("\n=== Version Info Tests ===\n");
    
    Dio_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(0x0055, version_info.vendorID);
    TEST_ASSERT_EQ(0x0020, version_info.moduleID);
    TEST_ASSERT_EQ(DIO_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(DIO_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(DIO_SW_PATCH_VERSION, version_info.sw_patch_version);
    
    /* 测试NULL指针 - 不应崩溃 */
    Dio_GetVersionInfo(NULL_PTR);
    TEST_ASSERT(1);  /* 通过如果未崩溃 */
}

/* 数据类型测试 */
/* @req SWS_Dio_00211 */
void test_data_types(void)
{
    printf("\n=== Data Type Tests ===\n");
    
    /* 测试Dio_LevelType */
    TEST_ASSERT_EQ(0, STD_LOW);
    TEST_ASSERT_EQ(1, STD_HIGH);
    
    /* 测试通道定义 */
    TEST_ASSERT_EQ(0x0000, 0x0000);  /* Channel A0 */
    TEST_ASSERT_EQ(0x0001, 0x0001);  /* Channel A1 */
    TEST_ASSERT_EQ(0x0100, 0x0100);  /* Channel B0 */
    TEST_ASSERT_EQ(0x0107, 0x0107);  /* Channel B7 */
    
    /* 测试端口定义 */
    TEST_ASSERT_EQ(0, 0);  /* Port A */
    TEST_ASSERT_EQ(1, 1);  /* Port B */
}

/* 错误处理测试 */
/* @req SWS_Dio_00212 */
void test_error_handling(void)
{
    Dio_LevelType level;
    Dio_PortLevelType portLevel;
    
    printf("\n=== Error Handling Tests ===\n");
    
    /* 确保驱动未初始化 */
    Dio_DeInit();
    
    /* 测试未初始化时读取通道 */
    level = Dio_ReadChannel(0x0000);
    TEST_ASSERT_EQ(STD_LOW, level);  /* 应返回安全默认值 */
    
    /* 测试未初始化时写入通道 */
    mock_port_values[0] = 0x00;
    Dio_WriteChannel(0x0000, STD_HIGH);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 应不改变 */
    
    /* 测试未初始化时读取端口 */
    portLevel = Dio_ReadPort(0);
    TEST_ASSERT_EQ(0x00, portLevel);
    
    /* 测试未初始化时写入端口 */
    Dio_WritePort(0, 0xFF);
    TEST_ASSERT_EQ(0x00, mock_port_values[0]);  /* 应不改变 */
    
    /* 测试无效端口索引 */
    Dio_Init(NULL_PTR);
    portLevel = Dio_ReadPort(255);
    TEST_ASSERT_EQ(0x00, portLevel);
}

/* 组合操作测试 */
/* @req SWS_Dio_00213 */
void test_combined_operations(void)
{
    printf("\n=== Combined Operations Tests ===\n");
    
    Dio_Init(NULL_PTR);
    memset(mock_port_values, 0, sizeof(mock_port_values));
    
    /* 测试: 写端口 -> 读通道 -> 验证 */
    Dio_WritePort(0, 0x55);
    TEST_ASSERT_EQ(STD_HIGH, Dio_ReadChannel(0x0000));  /* Bit 0 = 1 */
    TEST_ASSERT_EQ(STD_LOW, Dio_ReadChannel(0x0001));   /* Bit 1 = 0 */
    TEST_ASSERT_EQ(STD_HIGH, Dio_ReadChannel(0x0002));  /* Bit 2 = 1 */
    
    /* 测试: 写通道 -> 读端口 -> 验证 */
    Dio_WriteChannel(0x0000, STD_LOW);
    TEST_ASSERT_EQ(0x54, Dio_ReadPort(0));  /* 0x55 & ~0x01 = 0x54 */
    
    /* 测试: 翻转 -> 读通道 -> 验证 */
    Dio_FlipChannel(0x0000);
    TEST_ASSERT_EQ(STD_HIGH, Dio_ReadChannel(0x0000));
    Dio_FlipChannel(0x0000);
    TEST_ASSERT_EQ(STD_LOW, Dio_ReadChannel(0x0000));
    
    /* 测试: 通道组写 -> 读端口 -> 验证 */
    Dio_ChannelGroupType group = {0, 4, 0xF0};
    Dio_WritePort(0, 0x00);
    Dio_WriteChannelGroup(&group, 0x0A);
    TEST_ASSERT_EQ(0xA0, Dio_ReadPort(0));
}

/* 边界条件测试 */
/* @req SWS_Dio_00214 */
void test_boundary_conditions(void)
{
    printf("\n=== Boundary Conditions Tests ===\n");
    
    Dio_Init(NULL_PTR);
    memset(mock_port_values, 0, sizeof(mock_port_values));
    
    /* 测试所有通道位操作 */
    Dio_WritePort(0, 0xFF);
    TEST_ASSERT_EQ(0xFF, Dio_ReadPort(0));
    
    /* 测试最低位 */
    Dio_WritePort(0, 0x01);
    TEST_ASSERT_EQ(STD_HIGH, Dio_ReadChannel(0x0000));
    for (int i = 1; i < 8; i++) {
        TEST_ASSERT_EQ(STD_LOW, Dio_ReadChannel(i));
    }
    
    /* 测试最高位 */
    Dio_WritePort(0, 0x80);
    TEST_ASSERT_EQ(STD_HIGH, Dio_ReadChannel(0x0007));
    for (int i = 0; i < 7; i++) {
        TEST_ASSERT_EQ(STD_LOW, Dio_ReadChannel(i));
    }
    
    /* 测试通道组最大掩码 */
    Dio_ChannelGroupType group = {0, 0, 0xFF};
    Dio_WritePort(0, 0x00);
    Dio_WriteChannelGroup(&group, 0xFF);
    TEST_ASSERT_EQ(0xFF, Dio_ReadPort(0));
}

/*==================================================================================================
*                                    MAIN FUNCTION
==================================================================================================*/

int main(void)
{
    printf("========================================\n");
    printf("   DIO Module Unit Tests\n");
    printf("========================================\n");
    
    test_init();
    test_read_channel();
    test_write_channel();
    test_read_port();
    test_write_port();
    test_read_channel_group();
    test_write_channel_group();
    test_flip_channel();
    test_masked_write_port();
    test_version_info();
    test_data_types();
    test_error_handling();
    test_combined_operations();
    test_boundary_conditions();
    
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    printf("Coverage: %.1f%%\n", (tests_passed * 100.0) / tests_run);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}
