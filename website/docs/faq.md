---
sidebar_position: 100
---

# 常见问题

## 一般问题

### YuleTech 和商业 AutoSAR 工具有什么区别？

| 特性 | YuleTech | EB tresos / Vector |
|------|----------|-------------------|
| 授权 | Apache 2.0 (免费) | 商业授权 |
| 代码 | 完全开源 | 闭源 |
| 配置工具 | 基于文件 | GUI 工具 |
| 技术支持 | 社区 | 商业支持 |
| 定制能力 | 高 (可修改源码) | 有限 |

YuleTech 适合：
- 学习 AutoSAR 标准
- 资源受限的项目
- 需要深度定制的应用
- 开源项目

### 支持哪些硬件平台？

当前支持：

| 芯片系列 | 状态 | 测试平台 |
|---------|------|----------|
| STM32F4xx | ✓ 稳定 | Nucleo-F446RE |
| STM32H7xx | 🔄 开发中 | Nucleo-H743ZI |
| TC3xx (Aurix) | 📍 计划 | - |

### 如何报告 Bug？

1. 检查 [GitHub Issues](https://github.com/frisky1985/yuleASR/issues) 确保问题尚未被报告
2. 使用 Bug 模板创建新 Issue
3. 提供：
   - 问题描述
   - 复现步骤
   - 预期行为 vs 实际行为
   - 环境信息 (芯片型号、编译器版本等)

## 安装问题

### 缺少 `cmake`

**错误:**
```
cmake: command not found
```

**解决:**
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake

# macOS
brew install cmake

# 验证
cmake --version  # 应 >= 3.20
```

### 缺少 ARM 工具链

**错误:**
```
arm-none-eabi-gcc: command not found
```

**解决:**
```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc

# 验证
arm-none-eabi-gcc --version
```

### Python 依赖缺少

**错误:**
```
ModuleNotFoundError: No module named 'yaml'
```

**解决:**
```bash
cd yuleASR
pip3 install -r tools/requirements.txt
```

## 构建问题

### 配置失败

**错误:**
```
CMake Error: Could not find CMakeLists.txt
```

**解决:**
```bash
# 确保在项目根目录
pwd  # 应输出 .../yuleASR

# 正确配置
python3 tools/build/build.py configure --tests
```

### 构建失败

**错误:**
```
undefined reference to `Port_Init'
```

**解决:**
- 检查是否包含正确的源文件
- 确保头文件路径正确
- 检查 CMakeLists.txt 配置

### 头文件找不到

**错误:**
```
fatal error: Port.h: No such file or directory
```

**解决:**
```c
// 正确的包含方式
#include "Port.h"  // 而不是 #include "port.h"
```

## 测试问题

### 测试编译失败

**错误:**
```
error: unknown type name 'TEST_ASSERT_EQUAL'
```

**解决:**
```bash
# 确保启用了测试
python3 tools/build/build.py configure --tests

# 重新构建
python3 tools/build/build.py build
```

### Mock 函数未找到

**错误:**
```
undefined reference to `mock_Det_ReportError'
```

**解决:**
```c
// 在测试文件中包含 mock 头文件
#include "mock_det.h"
```

### 测试超时

**现象:** 测试运行时间过长

**解决:**
```bash
# 检查是否有无限循环
# 确保 mock 定时器正确配置
python3 tools/build/build.py test --verbose
```

## 静态分析问题

### cppcheck 警告

**警告:**
```
[unusedFunction]: Function is never used
```

**解决:**
```bash
# 检查配置
cat .harness/lint-rules/cppcheck.xml

# 或忽略特定警告
cppcheck --suppress=unusedFunction src/
```

### clang-format 不一致

**错误:**
```
Code style issues found
```

**解决:**
```bash
# 自动格式化
python3 tools/build/build.py format

# 或手动运行
find src -name '*.c' -o -name '*.h' | xargs clang-format -i
```

## 配置问题

### 配置文件错误

**现象:** 代码行为与预期不符

**解决:**
```c
// 检查配置文件
#ifndef PORT_CFG_H
#define PORT_CFG_H

// 确保宏定义保护
#define PORT_NUM_PINS 32

#endif
```

### 版本不匹配

**错误:**
```
Version mismatch between header and implementation
```

**解决:**
```c
// 确保头文件和源文件版本一致
// Port.h
#define PORT_SW_MAJOR_VERSION 1
#define PORT_SW_MINOR_VERSION 0

// Port.c
#define PORT_SW_MAJOR_VERSION 1
#define PORT_SW_MINOR_VERSION 0
```

## 运行时问题

### Hard Fault

**现象:** 程序崩溃

**原因:**
- 空指针解引用
- 数组越界
- 栈溢出

**解决:**
```c
// 检查指针
if (ptr == NULL_PTR) {
    Det_ReportError(...);
    return E_NOT_OK;
}

// 检查数组边界
if (index >= ARRAY_SIZE) {
    // 错误处理
}
```

### 时序问题

**现象:** 定时器或中断异常

**解决:**
```c
// 确保正确的时序配置
// 检查时钟源
// 验证分频设置
```

## 性能问题

### 代码体积过大

**优化:**
```c
// 使用 const 数据存储在 Flash
const uint8 PortConfigData[] = {...};

// 消除未使用的代码
// 优化编译器选项 -Os
```

### 堆栈使用过高

**优化:**
```c
// 减少局部变量
// 使用静态分配而非动态分配
// 优化函数调用层次
```

## 获得更多帮助

如果以上方案无法解决您的问题：

1. 查看 [故障排除指南](/docs/troubleshooting)
2. 搜索 [GitHub Discussions](https://github.com/frisky1985/yuleASR/discussions)
3. 创建 [GitHub Issue](https://github.com/frisky1985/yuleASR/issues)
4. 加入社区交流
