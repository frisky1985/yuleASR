/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Dio Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Dio.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Dio_ReadChannel with valid channel */
TEST_CASE(dio_read_channel_valid_high)
{
    Dio_LevelType level;
    
    /* Setup mock - channel 0 is high */
    Dio_Mock_SetChannelLevel(0, STD_HIGH);
    
    level = Dio_ReadChannel(0);
    
    ASSERT_EQ(STD_HIGH, level);
    TEST_PASS();
}

TEST_CASE(dio_read_channel_valid_low)
{
    Dio_LevelType level;
    
    /* Setup mock - channel 1 is low */
    Dio_Mock_SetChannelLevel(1, STD_LOW);
    
    level = Dio_ReadChannel(1);
    
    ASSERT_EQ(STD_LOW, level);
    TEST_PASS();
}

/* Test: Dio_ReadChannel with invalid channel */
TEST_CASE(dio_read_channel_invalid)
{
    Dio_LevelType level;
    
    Det_Mock_Reset();
    
    /* Use invalid channel ID */
    level = Dio_ReadChannel(999);
    
    ASSERT_EQ(STD_LOW, level);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_WriteChannel with valid channel */
TEST_CASE(dio_write_channel_valid_high)
{
    Dio_WriteChannel(0, STD_HIGH);
    
    ASSERT_EQ(STD_HIGH, Dio_Mock_GetChannelLevel(0));
    TEST_PASS();
}

TEST_CASE(dio_write_channel_valid_low)
{
    Dio_WriteChannel(0, STD_LOW);
    
    ASSERT_EQ(STD_LOW, Dio_Mock_GetChannelLevel(0));
    TEST_PASS();
}

/* Test: Dio_WriteChannel with invalid channel */
TEST_CASE(dio_write_channel_invalid)
{
    Det_Mock_Reset();
    
    Dio_WriteChannel(999, STD_HIGH);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_ReadPort with valid port */
TEST_CASE(dio_read_port_valid)
{
    Dio_PortLevelType level;
    
    /* Setup mock - port 0 has value 0xABCD */
    Dio_Mock_SetPortLevel(0, 0xABCD);
    
    level = Dio_ReadPort(0);
    
    ASSERT_EQ(0xABCD, level);
    TEST_PASS();
}

/* Test: Dio_ReadPort with invalid port */
TEST_CASE(dio_read_port_invalid)
{
    Dio_PortLevelType level;
    
    Det_Mock_Reset();
    
    level = Dio_ReadPort(99);
    
    ASSERT_EQ(0, level);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_PORT_ID, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_WritePort with valid port */
TEST_CASE(dio_write_port_valid)
{
    Dio_WritePort(0, 0x1234);
    
    ASSERT_EQ(0x1234, Dio_Mock_GetPortLevel(0));
    TEST_PASS();
}

/* Test: Dio_WritePort with invalid port */
TEST_CASE(dio_write_port_invalid)
{
    Det_Mock_Reset();
    
    Dio_WritePort(99, 0x1234);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_PORT_ID, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_ReadChannelGroup */
TEST_CASE(dio_read_channel_group_valid)
{
    Dio_ChannelGroupType group;
    Dio_PortLevelType level;
    
    /* Setup channel group: mask 0x0F on port 0, offset 0 */
    group.port = 0;
    group.offset = 0;
    group.mask = 0x0F;
    
    /* Port value is 0xAB */
    Dio_Mock_SetPortLevel(0, 0xAB);
    
    level = Dio_ReadChannelGroup(&group);
    
    ASSERT_EQ(0x0B, level);
    TEST_PASS();
}

/* Test: Dio_ReadChannelGroup with NULL pointer */
TEST_CASE(dio_read_channel_group_null)
{
    Dio_PortLevelType level;
    
    Det_Mock_Reset();
    
    level = Dio_ReadChannelGroup(NULL);
    
    ASSERT_EQ(0, level);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_ReadChannelGroup with invalid group */
TEST_CASE(dio_read_channel_group_invalid)
{
    Dio_ChannelGroupType group;
    Dio_PortLevelType level;
    
    Det_Mock_Reset();
    
    group.port = 99; /* Invalid port */
    group.offset = 0;
    group.mask = 0x0F;
    
    level = Dio_ReadChannelGroup(&group);
    
    ASSERT_EQ(0, level);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_GROUP, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Dio_WriteChannelGroup */
TEST_CASE(dio_write_channel_group_valid)
{
    Dio_ChannelGroupType group;
    
    /* Setup channel group: mask 0x0F on port 0, offset 0 */
    group.port = 0;
    group.offset = 0;
    group.mask = 0x0F;
    
    /* Initial port value */
    Dio_Mock_SetPortLevel(0, 0xF0);
    
    Dio_WriteChannelGroup(&group, 0x05);
    
    ASSERT_EQ(0xF5, Dio_Mock_GetPortLevel(0));
    TEST_PASS();
}

/* Test: Dio_WriteChannelGroup with NULL pointer */
TEST_CASE(dio_write_channel_group_null)
{
    Det_Mock_Reset();
    
    Dio_WriteChannelGroup(NULL, 0x05);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

#if (DIO_VERSION_INFO_API == STD_ON)
/* Test: Dio_GetVersionInfo with valid pointer */
TEST_CASE(dio_get_version_info_valid)
{
    Std_VersionInfoType version_info;
    
    Dio_GetVersionInfo(&version_info);
    
    ASSERT_EQ(DIO_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(DIO_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

/* Test: Dio_GetVersionInfo with NULL pointer */
TEST_CASE(dio_get_version_info_null)
{
    Det_Mock_Reset();
    
    Dio_GetVersionInfo(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}
#endif /* DIO_VERSION_INFO_API */

#if (DIO_FLIP_CHANNEL_API == STD_ON)
/* Test: Dio_FlipChannel */
TEST_CASE(dio_flip_channel_valid)
{
    Dio_LevelType level;
    
    /* Start with LOW */
    Dio_Mock_SetChannelLevel(0, STD_LOW);
    
    level = Dio_FlipChannel(0);
    
    ASSERT_EQ(STD_HIGH, level);
    ASSERT_EQ(STD_HIGH, Dio_Mock_GetChannelLevel(0));
    
    /* Flip again */
    level = Dio_FlipChannel(0);
    
    ASSERT_EQ(STD_LOW, level);
    ASSERT_EQ(STD_LOW, Dio_Mock_GetChannelLevel(0));
    TEST_PASS();
}

/* Test: Dio_FlipChannel with invalid channel */
TEST_CASE(dio_flip_channel_invalid)
{
    Det_Mock_Reset();
    
    Dio_FlipChannel(999);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
    TEST_PASS();
}
#endif /* DIO_FLIP_CHANNEL_API */

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(dio)
{
    Dio_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(dio)
{
    /* Cleanup */
}

TEST_SUITE(dio)
{
    RUN_TEST(dio_read_channel_valid_high);
    RUN_TEST(dio_read_channel_valid_low);
    RUN_TEST(dio_read_channel_invalid);
    RUN_TEST(dio_write_channel_valid_high);
    RUN_TEST(dio_write_channel_valid_low);
    RUN_TEST(dio_write_channel_invalid);
    RUN_TEST(dio_read_port_valid);
    RUN_TEST(dio_read_port_invalid);
    RUN_TEST(dio_write_port_valid);
    RUN_TEST(dio_write_port_invalid);
    RUN_TEST(dio_read_channel_group_valid);
    RUN_TEST(dio_read_channel_group_null);
    RUN_TEST(dio_read_channel_group_invalid);
    RUN_TEST(dio_write_channel_group_valid);
    RUN_TEST(dio_write_channel_group_null);
#if (DIO_VERSION_INFO_API == STD_ON)
    RUN_TEST(dio_get_version_info_valid);
    RUN_TEST(dio_get_version_info_null);
#endif
#if (DIO_FLIP_CHANNEL_API == STD_ON)
    RUN_TEST(dio_flip_channel_valid);
    RUN_TEST(dio_flip_channel_invalid);
#endif
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- DIO Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(dio);
}
TEST_MAIN_END()
