/* 
 * @file test_someipsd.c
 * @brief SOMEIPSD 模块单元测试
 */

// @tests src/bsw/ecual/someipsd/src/SomeIpSd.c  @tests src/bsw/ecual/someipsd/include/SomeIpSd.h

#include <unity.h>
#include <string.h>
#include "someipsd.h"

/* Det mock - may not be called if SOMEIPSD_DEV_ERROR_DETECT is not defined
 * in the SUT compilation, but provided for linker safety */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId; (void)ApiId; (void)ErrorId;
    return E_OK;
}

/* Test config */
static const SomeIpSd_ConfigType testSdConfig =
{
    .Services = NULL_PTR,
    .NumServices = 0U,
    .LocalIpAddress = 0xC0A80101U,  /* 192.168.1.1 */
    .LocalPort = 30490U,
    .MulticastIpAddress = 0xE0000001U,
    .MulticastPort = 30490U,
    .DevErrorDetect = TRUE
};

void setUp(void)
{
    /* Init with valid config for each test */
    SomeIpSd_Init(&testSdConfig);
}

void tearDown(void)
{
    SomeIpSd_DeInit();
}

/** @req SWS_SomeIp_00001 */
void test_someipsd_Init_should_initialize(void) {
    /* setUp already called Init; deinit and reinit with NULL to test NULL path */
    SomeIpSd_DeInit();
    SomeIpSd_Init(NULL_PTR);
    /* If we reach here without crash, init handled NULL gracefully */
    TEST_PASS();
}

void test_someipsd_GetVersionInfo_should_return_version(void) {
    Std_VersionInfoType versionInfo;
    memset(&versionInfo, 0, sizeof(Std_VersionInfoType));
    SomeIpSd_GetVersionInfo(&versionInfo);
    TEST_ASSERT_NOT_EQUAL(0u, versionInfo.vendorID);
    TEST_ASSERT_NOT_EQUAL(0u, versionInfo.moduleID);
    TEST_ASSERT_NOT_EQUAL(0u, versionInfo.sw_major_version);
}

/*==============================================================================
*                                      BRANCH COVERAGE SUPPLEMENT
* Lines: 111 (ServiceId match in OfferService duplicate check),
*        121 (free entry found), 127 (registry full),
*        164 (ServiceId match in StopOfferService)
*==============================================================================*/

/* Line 121: Successful offer - free entry found and used */
void test_SomeIpSd_OfferService_FreeEntry_ShouldSucceed(void)
{
    Std_ReturnType ret = SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3600U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* Line 111: Duplicate offer - same ServiceId+InstanceId already available */
void test_SomeIpSd_OfferService_Duplicate_ShouldFail(void)
{
    /* First offer succeeds */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x2000U, 0x0002U, 1U, 0U, 3600U));
    /* Duplicate offer with same ServiceId+InstanceId should fail */
    Std_ReturnType ret = SomeIpSd_OfferService(0x2000U, 0x0002U, 1U, 0U, 3600U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* Line 164: StopOfferService finds the service by ServiceId+InstanceId */
void test_SomeIpSd_StopOfferService_Found_ShouldSucceed(void)
{
    /* Offer a service first */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x3000U, 0x0003U, 2U, 100U, 7200U));
    /* Stop offer should find and remove it */
    Std_ReturnType ret = SomeIpSd_StopOfferService(0x3000U, 0x0003U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Second stop should fail (no longer offered) */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_StopOfferService(0x3000U, 0x0003U));
}

/* Line 127: Registry full - fill all 32 slots */
void test_SomeIpSd_OfferService_RegistryFull_ShouldFail(void)
{
    Std_ReturnType ret;
    uint16 i;
    /* Fill all 32 slots (SOMEIPSD_MAX_SERVICES = 32) */
    for (i = 0U; i < 32U; i++)
    {
        ret = SomeIpSd_OfferService(0x4000U + i, 0x0010U, 1U, 0U, 3600U);
        TEST_ASSERT_EQUAL(E_OK, ret);
    }
    /* 33rd offer should fail - registry full */
    ret = SomeIpSd_OfferService(0x4020U, 0x0010U, 1U, 0U, 3600U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_someipsd_Init_should_initialize);
    RUN_TEST(test_someipsd_GetVersionInfo_should_return_version);
    RUN_TEST(test_SomeIpSd_OfferService_FreeEntry_ShouldSucceed);
    RUN_TEST(test_SomeIpSd_OfferService_Duplicate_ShouldFail);
    RUN_TEST(test_SomeIpSd_StopOfferService_Found_ShouldSucceed);
    RUN_TEST(test_SomeIpSd_OfferService_RegistryFull_ShouldFail);
    return UNITY_END();
}
