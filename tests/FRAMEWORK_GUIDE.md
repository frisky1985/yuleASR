# YuleTech BSW 测试框架使用指南

## 目录

1. [框架介绍](#框架介绍)
2. [写你的第一个测试](#写你的第一个测试)
3. [断言完整参考](#断言完整参考)
4. [Mock 使用进阶](#mock-使用进阶)
5. [测试套件模式](#测试套件模式)
6. [最佳实践](#最佳实践)
7. [故障排除](#故障排除)

---

## 框架介绍

YuleTech BSW 测试框架是专为 AutoSAR BSW 开发的单元测试框架，具有以下特点：

- **轻量**: 无需外部依赖，纯 C 实现
- **易用**: 简洁的宏 API
- **完整**: 丰富的断言集合
- **Mock 支持**: 完整的模拟对象体系
- **可扩展**: 支持扩展和定制

---

## 写你的第一个测试

### 步骤 1: 创建测试文件

创建 `tests/unit/my_module/test_feature.c`

```c
/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Feature Unit Tests
==================================================================================================*/

#include "test_framework.h"
#include "MyFeature.h"  // 被测模块的头文件
#include "mock_det.h"   // DET 模拟

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(feature_init_valid)
{
    // Arrange - 准备测试数据
    MyFeature_ConfigType config = {
        .Param1 = 100,
        .Param2 = TRUE
    };
    
    // Act - 执行测试
    MyFeature_Init(&config);
    
    // Assert - 验证结果
    ASSERT_EQ(MYFEATURE_STATE_INIT, MyFeature_GetState());
    TEST_PASS();
}

TEST_CASE(feature_init_null_config)
{
    // Arrange
    Det_Mock_Reset();
    
    // Act
    MyFeature_Init(NULL);
    
    // Assert - 验证 DET 错误
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MYFEATURE_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/

TEST_SUITE_SETUP(feature)
{
    // 每个测试套件前执行
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(feature)
{
    // 每个测试套件后执行
}

TEST_SUITE(feature)
{
    RUN_TEST(feature_init_valid);
    RUN_TEST(feature_init_null_config);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- Feature Unit Tests ---\n");
    RUN_TEST_SUITE(feature);
}
TEST_MAIN_END()
```

### 步骤 2: 更新 CMakeLists.txt

在 `tests/CMakeLists.txt` 中添加：

```cmake
add_executable(test_feature unit/my_module/test_feature.c ${MOCK_SOURCES})
add_test(NAME Feature_Test COMMAND test_feature)
```

### 步骤 3: 构建和运行

```bash
cd tests/build
make test_feature
./test_feature
```

---

## 断言完整参考

### 基本断言

| 宏 | 说明 | 示例 |
|-----|------|------|
| `ASSERT_TRUE(x)` | 断言为真 | `ASSERT_TRUE(ptr != NULL)` |
| `ASSERT_FALSE(x)` | 断言为假 | `ASSERT_FALSE(isError)` |
| `ASSERT_EQ(a, b)` | 相等 | `ASSERT_EQ(100, value)` |
| `ASSERT_NE(a, b)` | 不等 | `ASSERT_NE(NULL, ptr)` |
| `ASSERT_LT(a, b)` | 小于 | `ASSERT_LT(0, count)` |
| `ASSERT_LE(a, b)` | 小于等于 | `ASSERT_LE(retries, 3)` |
| `ASSERT_GT(a, b)` | 大于 | `ASSERT_GT(100, temp)` |
| `ASSERT_GE(a, b)` | 大于等于 | `ASSERT_GE(voltage, 0)` |

### 指针断言

| 宏 | 说明 | 示例 |
|-----|------|------|
| `ASSERT_NULL(p)` | 为空 | `ASSERT_NULL(null_ptr)` |
| `ASSERT_NOT_NULL(p)` | 非空 | `ASSERT_NOT_NULL(buffer)` |

### 字符串断言

| 宏 | 说明 | 示例 |
|-----|------|------|
| `ASSERT_STR_EQ(a, b)` | 字符串相等 | `ASSERT_STR_EQ("OK", status)` |
| `ASSERT_STR_NE(a, b)` | 字符串不等 | `ASSERT_STR_NE("", name)` |

### 内存断言

| 宏 | 说明 | 示例 |
|-----|------|------|
| `ASSERT_MEM_EQ(a, b, n)` | 内容相同 | `ASSERT_MEM_EQ(exp, buf, 8)` |

### 范围断言

| 宏 | 说明 | 示例 |
|-----|------|------|
| `ASSERT_WITHIN_RANGE(l, u, v)` | 在范围内 | `ASSERT_WITHIN_RANGE(0, 100, val)` |

---

## Mock 使用进阶

### DET Mock

```c
// 重置 DET 调用记录
Det_Mock_Reset();

// 执行测试
MyFunction(NULL);  // 应该报错

// 验证报错
ASSERT_EQ(1, Det_Mock_GetCallCount());
ASSERT_EQ(MY_MODULE_ID, Det_MockData.ModuleId);
ASSERT_EQ(0, Det_MockData.InstanceId);  // 实例 ID
ASSERT_EQ(MY_API_ID_INIT, Det_MockData.ApiId);
ASSERT_EQ(MY_E_PARAM_NULL, Det_MockData.ErrorId);

// 获取最后错误详情
uint16 modId;
uint8 instId, apiId, errId;
Det_Mock_GetLastError(&modId, &instId, &apiId, &errId);
```

### MCAL Driver Mock

```c
// MCU Mock
Mcu_Mock_Reset();
Mcu_Mock_SetClockFrequency(16000000);
ASSERT_EQ(16000000, Mcu_Mock_GetClockFrequency());

// Port Mock
Port_Mock_Reset();
Port_Mock_SetPinState(0, PORT_PIN_OUT, PORT_PIN_LEVEL_HIGH);
ASSERT_EQ(PORT_PIN_OUT, Port_MockPinStates[0].Direction);

// DIO Mock
Dio_Mock_Reset();
Dio_Mock_SetChannelLevel(0, STD_HIGH);
ASSERT_EQ(STD_HIGH, Dio_Mock_GetChannelLevel(0));

// CAN Mock
Can_Mock_Reset();
Can_Mock_SetControllerState(0, CAN_CS_STARTED);
ASSERT_EQ(CAN_CS_STARTED, Can_Mock_GetControllerState(0));

// SPI Mock
Spi_Mock_Reset();
Spi_Mock_SetJobResult(0, SPI_JOB_OK);
ASSERT_EQ(SPI_JOB_OK, Spi_GetJobResult(0));

// GPT Mock
Gpt_Mock_Reset();
Gpt_Mock_SetChannelTime(0, 500, 1500);

// PWM Mock
Pwm_Mock_Reset();
Pwm_Mock_SetChannelDutyCycle(0, 5000);
ASSERT_EQ(5000, Pwm_Mock_GetChannelDutyCycle(0));

// ADC Mock
Adc_Mock_Reset();
uint16 values[2] = {100, 200};
Adc_Mock_SetGroupResult(0, values, 2);

// WDG Mock
Wdg_Mock_Reset();
Wdg_Mock_SetMode(WDGIF_FAST_MODE);
ASSERT_EQ(WDGIF_FAST_MODE, Wdg_Mock_GetMode());
```

### ECUAL Mock

```c
// CANIF Mock
CanIf_Mock_Reset();

// PDUR Mock
PduR_Mock_Reset();

// NVM Mock
NvM_Mock_Reset();
uint8 data[4] = {1, 2, 3, 4};
NvM_Mock_SetBlockData(1, data, 4);

// COM Mock
Com_Mock_Reset();

// DCM Mock
Dcm_Mock_Reset();

// DEM Mock
Dem_Mock_Reset();
Dem_Mock_SetEventStatus(0, DEM_MOCK_EVENT_STATUS_FAILED);
ASSERT_EQ(DEM_MOCK_EVENT_STATUS_FAILED, Dem_Mock_GetEventStatus(0));
```

### OS Mock

```c
// Task Mock
Os_Mock_Reset();
Os_Mock_SetTaskState(0, OS_MOCK_TASK_RUNNING);
ASSERT_EQ(OS_MOCK_TASK_RUNNING, Os_Mock_GetTaskState(0));

// Event Mock
Os_Mock_SetEvent(0, 0x01);
ASSERT_EQ(0x01, Os_Mock_GetEvent(0));
Os_Mock_ClearEvent(0);

// Alarm Mock
Os_Mock_ResetAlarms();
Os_Mock_SetAlarm(0, 100);

// Resource Mock
Os_Mock_ResetResources();
Os_Mock_LockResource(0, 1);
ASSERT_TRUE(Os_Mock_IsResourceLocked(0));

// ISR Mock
Os_Mock_ResetIsrs();
Os_Mock_EnableIsr(0);
Os_Mock_TriggerIsr(0);
```

---

## 测试套件模式

### 完整测试结构

```c
/*==================================================================================================
*                                      INCLUDES
==================================================================================================*/
#include "test_framework.h"
#include "Module.h"
#include "mock_dep.h"

/*==================================================================================================
*                                      TEST DATA
==================================================================================================*/
static Module_ConfigType g_test_config;

/*==================================================================================================
*                                      HELPERS
==================================================================================================*/
static void setup_test_data(void)
{
    // 初始化测试数据
}

/*==================================================================================================
*                                      POSITIVE TESTS
*                                      正向测试
==================================================================================================*/

TEST_CASE(module_init_valid)
{
    setup_test_data();
    
    Module_Init(&g_test_config);
    
    ASSERT_EQ(MODULE_STATE_INIT, Module_GetState());
    TEST_PASS();
}

TEST_CASE(module_normal_operation)
{
    Module_ResultType result;
    
    setup_test_data();
    Module_Init(&g_test_config);
    
    result = Module_DoSomething();
    
    ASSERT_EQ(MODULE_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      NEGATIVE TESTS
*                                      负向测试
==================================================================================================*/

TEST_CASE(module_init_null)
{
    Det_Mock_Reset();
    
    Module_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MODULE_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(module_uninit_call)
{
    Det_Mock_Reset();
    
    // 尝试在未初始化时调用
    Module_DoSomething();
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MODULE_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(module_invalid_param)
{
    Module_ResultType result;
    
    setup_test_data();
    Module_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Module_ProcessData(999); // 无效参数
    
    ASSERT_EQ(MODULE_NOT_OK, result);
    ASSERT_EQ(MODULE_E_PARAM_DATA, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      BOUNDARY TESTS
*                                      边界测试
==================================================================================================*/

TEST_CASE(module_boundary_min)
{
    Module_ResultType result;
    
    setup_test_data();
    Module_Init(&g_test_config);
    
    result = Module_ProcessData(0); // 最小值
    
    ASSERT_EQ(MODULE_OK, result);
    TEST_PASS();
}

TEST_CASE(module_boundary_max)
{
    Module_ResultType result;
    
    setup_test_data();
    Module_Init(&g_test_config);
    
    result = Module_ProcessData(255); // 最大值
    
    ASSERT_EQ(MODULE_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      STATE TESTS
*                                      状态测试
==================================================================================================*/

TEST_CASE(module_state_transition)
{
    setup_test_data();
    
    ASSERT_EQ(MODULE_STATE_UNINIT, Module_GetState());
    
    Module_Init(&g_test_config);
    ASSERT_EQ(MODULE_STATE_INIT, Module_GetState());
    
    Module_Start();
    ASSERT_EQ(MODULE_STATE_RUNNING, Module_GetState());
    
    Module_Stop();
    ASSERT_EQ(MODULE_STATE_STOPPED, Module_GetState());
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/

TEST_SUITE_SETUP(module)
{
    // 重置所有 mock
    Det_Mock_Reset();
    Other_Mock_Reset();
    
    // 重置测试数据
    memset(&g_test_config, 0, sizeof(g_test_config));
}

TEST_SUITE_TEARDOWN(module)
{
    // 清理
}

TEST_SUITE(module)
{
    // 正向测试
    RUN_TEST(module_init_valid);
    RUN_TEST(module_normal_operation);
    
    // 负向测试
    RUN_TEST(module_init_null);
    RUN_TEST(module_uninit_call);
    RUN_TEST(module_invalid_param);
    
    // 边界测试
    RUN_TEST(module_boundary_min);
    RUN_TEST(module_boundary_max);
    
    // 状态测试
    RUN_TEST(module_state_transition);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- Module Unit Tests ---\n");
    RUN_TEST_SUITE(module);
}
TEST_MAIN_END()
```

---

## 最佳实践

### 1. 测试命名约定

- 测试文件: `test_<module>.c`
- 测试函数: `<module>_<action>_<condition>`
- 例子: `wdg_trigger_not_initialized`

### 2. 测试结构

每个测试应遵循 AAA 模式：
- **Arrange**: 准备测试数据和状态
- **Act**: 执行测试操作
- **Assert**: 验证结果

### 3. Mock 使用原则

- 每个测试套件前重置 mock
- 测试结束后验证 mock 状态
- DET 错误必须验证所有字段

### 4. 覆盖率目标

- 代码覆盖率: >= 80%
- 函数覆盖率: 100%
- 分支覆盖率: >= 70%

---

## 故障排除

### 测试编译失败

1. 检查头文件包含路径
2. 确认所有依赖文件存在
3. 检查 CMakeLists.txt 配置

### 测试运行失败

1. 检查 mock 是否正确重置
2. 确认测试顺序正确
3. 使用 `-v` 参数查看详细输出

### 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|--------|
| 段错误 | 数组越界 | 检查数组大小 |
| 挂起 | 死循环 | 添加超时检查 |
| 失败 | 逻辑错误 | 修复实现代码 |
