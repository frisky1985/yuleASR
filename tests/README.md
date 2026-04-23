# YuleTech AutoSAR BSW Test Framework

## 概览

这是 YuleTech AutoSAR BSW 平台的完整测试框架，支持单元测试、集成测试和模拟对象。框架基于 C 语言内建测试工具，兼容 CMocka 和 Unity 测试框架。

## 目录结构

```
tests/
├── CMakeLists.txt           # CMake 构建配置
├── README.md                # 本文件
├── unit/                    # 单元测试
│   ├── test_framework.h       # 测试框架头文件
│   ├── mcal/                  # MCAL 层测试
│   │   ├── test_mcu.c
│   │   ├── test_port.c
│   │   ├── test_dio.c
│   │   ├── test_can.c
│   │   ├── test_spi.c
│   │   ├── test_gpt.c
│   │   ├── test_pwm.c
│   │   ├── test_adc.c
│   │   └── test_wdg.c
│   ├── ecual/                 # ECUAL 层测试
│   ├── services/              # Service 层测试
│   └── os/                    # OS 层测试
├── integration/             # 集成测试
│   ├── mcal/
│   └── bsw_stack/
├── mocks/                   # 模拟对象
│   ├── mock_det.h/.c          # DET 模拟
│   ├── mock_mcal.h/.c         # MCAL 模拟
│   ├── mock_ecual.h/.c        # ECUAL 模拟
│   ├── mock_services.h/.c     # Services 模拟
│   └── mock_os.h/.c           # OS 模拟
└── tools/                   # 测试工具
    └── run_tests.py           # 测试运行脚本
```

## 快速开始

### 1. 构建测试

```bash
cd tests
mkdir -p build && cd build
cmake ..
make
```

### 2. 运行测试

```bash
# 运行所有测试
make run_tests

# 或使用脚本
python3 tools/run_tests.py

# 运行特定模块测试
python3 tools/run_tests.py -m mcu

# 运行特定层测试
python3 tools/run_tests.py -l mcal
```

### 3. 生成测试报告

```bash
# 生成 HTML 报告
python3 tools/run_tests.py -r html

# 生成 JSON 报告
python3 tools/run_tests.py -r text

# 启用代码覆盖率
python3 tools/run_tests.py -c
```

## 测试框架 API

### 基本断言

```c
#include "test_framework.h"

// 基本断言
ASSERT_TRUE(condition);
ASSERT_FALSE(condition);
ASSERT_EQ(expected, actual);
ASSERT_NE(expected, actual);
ASSERT_LT(left, right);
ASSERT_LE(left, right);
ASSERT_GT(left, right);
ASSERT_GE(left, right);

// 指针断言
ASSERT_NULL(ptr);
ASSERT_NOT_NULL(ptr);

// 字符串断言
ASSERT_STR_EQ(expected, actual);
ASSERT_STR_NE(expected, actual);

// 内存断言
ASSERT_MEM_EQ(expected, actual, size);

// 范围断言
ASSERT_WITHIN_RANGE(lower, upper, actual);
```

### 测试用例定义

```c
// 定义测试用例
TEST_CASE(test_name)
{
    // 测试代码
    ASSERT_EQ(expected, actual);
    TEST_PASS();
}

// 定义测试套件
TEST_SUITE(my_module)
{
    RUN_TEST(test_name);
    RUN_TEST(another_test);
}

// 测试套件初始化/清理
TEST_SUITE_SETUP(my_module)
{
    // 初始化代码
}

TEST_SUITE_TEARDOWN(my_module)
{
    // 清理代码
}

// 主函数
TEST_MAIN_BEGIN()
{
    RUN_TEST_SUITE(my_module);
}
TEST_MAIN_END()
```

### Mock 使用

```c
#include "mock_mcal.h"
#include "mock_det.h"

// 重置 mock
Mcu_Mock_Reset();
Det_Mock_Reset();

// 设置 mock 状态
Mcu_Mock_SetClockFrequency(16000000);
Dio_Mock_SetChannelLevel(0, STD_HIGH);

// 验证 DET 错误
ASSERT_EQ(1, Det_MockData.CallCount);
ASSERT_EQ(PORT_E_PARAM_PIN, Det_MockData.ErrorId);
```

## MCAL 驱动测试覆盖

| 驱动 | 测试文件 | 测试用例数 | 状态 |
|------|----------|----------|------|
| Mcu  | test_mcu.c | 12+ | ✅ 完成 |
| Port | test_port.c | 15+ | ✅ 完成 |
| Dio  | test_dio.c | 16+ | ✅ 完成 |
| Can  | test_can.c | 18+ | ✅ 完成 |
| Spi  | test_spi.c | 15+ | ✅ 完成 |
| Gpt  | test_gpt.c | 14+ | ✅ 完成 |
| Pwm  | test_pwm.c | 11+ | ✅ 完成 |
| Adc  | test_adc.c | 15+ | ✅ 完成 |
| Wdg  | test_wdg.c | 13+ | ✅ 完成 |

## 连续集成

框架支持 CI/CD 集成，返回码：
- `0` - 所有测试通过
- `1` - 有测试失败

示例 GitHub Actions 配置：

```yaml
- name: Run Unit Tests
  run: |
    cd tests
    mkdir build && cd build
    cmake ..
    make
    ctest --output-on-failure
```

## 扩展框架

### 添加新的测试模块

1. 创建测试文件 `tests/unit/<layer>/test_<module>.c`
2. 在 `CMakeLists.txt` 中添加目标
3. 运行测试验证

### 添加新的 Mock

1. 在相应的 `mock_*.h` 中声明 mock 数据结构
2. 在相应的 `mock_*.c` 中实现 mock 函数
3. 在测试中包含并使用 mock

## 版本信息

- **版本**: 1.0.0
- **日期**: 2026-04-23
- **作者**: Shanghai Yule Electronics Technology Co., Ltd.

## 授权

Copyright (c) 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
All Rights Reserved.
