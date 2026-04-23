---
sidebar_position: 101
---

# 故障排除指南

本指南帮助您诊断和解决 YuleTech 项目中的常见问题。

## 构建故障排除

### CMake 配置失败

#### 症状
```
CMake Error: The source directory does not appear to contain CMakeLists.txt
```

#### 诊断步骤
1. 确认当前目录
   ```bash
   pwd
   # 应输出: .../yuleASR
   ```

2. 检查 CMakeLists.txt 存在
   ```bash
   ls -la CMakeLists.txt
   ```

3. 验证 CMake 版本
   ```bash
   cmake --version
   # 需要 >= 3.20
   ```

#### 解决方案
```bash
# 正确的配置流程
cd ~/yuleASR
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=ON
```

### 编译失败

#### 症状
```
error: ‘Port_Init’ defined but not used [-Werror=unused-function]
```

#### 诊断步骤
1. 检查是否正确包含头文件
2. 验证函数声明与定义一致
3. 检查是否需要宏保护

#### 解决方案
```c
// Port.h
#ifndef PORT_H
#define PORT_H

Std_ReturnType Port_Init(const Port_ConfigType* ConfigPtr);

#endif
```

```c
// Port.c
#include "Port.h"

Std_ReturnType Port_Init(const Port_ConfigType* ConfigPtr)
{
    // 实现
}
```

### 链接错误

#### 症状
```
undefined reference to `Det_ReportError'
```

#### 诊断步骤
1. 检查 DET 模块是否已编译
2. 验证链接库配置
3. 检查依赖项

#### 解决方案
```cmake
# 在 CMakeLists.txt 中
add_executable(my_app 
    src/main.c
    src/bsw/det/Det.c  # 确保包含 DET 源文件
)
```

## 测试故障排除

### 测试运行失败

#### 症状
```
Test suite exited with code 1
```

#### 诊断步骤
1. 详细查看测试输出
   ```bash
   python3 tools/build/build.py test --verbose
   ```

2. 检查具体失败的测试
   ```bash
   cd build && ./test_runner -v
   ```

3. 使用 GDB 调试
   ```bash
   gdb ./test_runner
   (gdb) run
   (gdb) bt  # 查看调用栈
   ```

#### 解决方案
```c
// 测试中添加调试输出
void test_SpecificCase(void) {
    printf("Debug: entering test\n");
    // 测试逻辑
    printf("Debug: result = %d\n", result);
}
```

### Mock 问题

#### 症状
```
Function called without expectation
```

#### 诊断步骤
1. 检查 mock 是否正确设置
2. 验证调用次数
3. 检查参数匹配

#### 解决方案
```c
void test_WithMock(void) {
    // 设置期望
    mock_Det_ReportError_ExpectAndReturn(
        MODULE_ID, INSTANCE_ID, API_ID, ERROR_CODE, E_OK);
    
    // 执行测试
    function_under_test();
    
    // 验证所有期望已满足
    mock_verify_all();
}
```

### 内存问题

#### 症状
```
Segmentation fault
```

#### 诊断步骤
1. 运行 Valgrind
   ```bash
   valgrind --leak-check=full ./test_runner
   ```

2. 使用 AddressSanitizer
   ```cmake
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
   ```

3. 检查数组越界

#### 解决方案
```c
// 错误: 数组越界
uint8 buffer[10];
buffer[10] = 0;  // 越界！

// 正确: 检查边界
if (index < ARRAY_SIZE(buffer)) {
    buffer[index] = value;
}
```

## 静态分析故障

### cppcheck 警告

#### 信任过度
```
[trustedCertProvider] Variable is assigned a value that is never read
```

**解决:**
```c
// 如果确实无需使用
(void)unused_variable;  // 显示表示无意使用

// 或移除未使用的变量
```

#### 内存泄漏
```
[memleak] Memory leak: ptr
```

**解决:**
```c
// 错误
void* ptr = malloc(size);
// 使用 ptr
// 缺少 free

// 正确
void* ptr = malloc(size);
if (ptr != NULL) {
    // 使用 ptr
    free(ptr);
}
```

#### 空指针解引用
```
[nullPointer] Null pointer dereference
```

**解决:**
```c
// 错误
void function(char* str) {
    char c = str[0];  // 可能空指针
}

// 正确
void function(char* str) {
    if (str != NULL) {
        char c = str[0];
    }
}
```

### clang-format 问题

#### 风格不一致
```bash
# 检查格式
clang-format -i src/Port/Port.c

# 查看差异
git diff src/Port/Port.c
```

#### 配置问题
```bash
# 确保配置文件存在
ls .harness/lint-rules/.clang-format

# 验证配置
clang-format --dump-config
```

## 硬件故障排除

### 调试器连接失败

#### 症状
```
Error: unable to open CMSIS-DAP device
```

#### 诊断步骤
1. 检查 USB 连接
2. 验证权限
   ```bash
   ls -la /dev/bus/usb/
   sudo usermod -a -G dialout $USER
   ```
3. 更新调试器固件

#### 解决方案
```bash
# 检查设备
lsusb | grep ST-Link

# 更新 udev 规则
sudo cp tools/udev/99-stlink.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

### 程序下载失败

#### 症状
```
Error: flash download failed
```

#### 诊断步骤
1. 检查调试器连接
2. 验证目标芯片选择
3. 检查 Flash 写保护

#### 解决方案
```bash
# 使用 OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# 或 STM32CubeProgrammer
STM32_Programmer_CLI -c port=SWD -e all -w build/my_app.hex
```

### 串口输出问题

#### 症状
没有串口输出或乱码

#### 诊断步骤
1. 检查波特率设置
2. 验证引脚配置
3. 使用逻辑分析仪

#### 解决方案
```c
// 正确的 UART 初始化
void Uart_Init(void) {
    // 115200, 8N1
    UART->BRR = SystemCoreClock / 115200;
    UART->CR1 = UART_CR1_TE | UART_CR1_RE | UART_CR1_UE;
}
```

## 运行时故障

### Hard Fault 处理

#### 诊断
```c
// HardFault 处理函数
void HardFault_Handler(void) {
    // 获取引发 HardFault 的地址
    volatile uint32_t* stack_ptr = (uint32_t*)__get_MSP();
    volatile uint32_t pc = stack_ptr[6];  // 程序计数器
    
    // 记录错误信息
    g_HardFaultInfo.pc = pc;
    g_HardFaultInfo.lr = stack_ptr[5];
    
    // 进入无限循环以便调试
    while (1) {
        __BKPT(0);
    }
}
```

#### 常见原因
1. **空指针解引用**
   - 检查所有指针使用前的 NULL 检查
   - 确保指针正确初始化

2. **数组越界**
   - 检查数组索引范围
   - 使用安全的字符串操作函数

3. **堆栈溢出**
   - 减少局部变量使用
   - 增加堆栈大小

### Bus Fault

#### 原因
- 访问未映射内存
- 非对齐访问

#### 解决
```c
// 确保内存对齐
struct __attribute__((packed)) MyStruct {
    uint8_t a;
    uint32_t b;  // 可能导致非对齐访问
};

// 更好的做法
struct MyStruct {
    uint32_t b;  // 先放置需要对齐的成员
    uint8_t a;
} __attribute__((aligned(4)));
```

## 工具链问题

### Python 脚本错误

#### ModuleNotFoundError
```bash
# 安装所有依赖
pip3 install -r tools/requirements.txt

# 或逐个安装
pip3 install pyyaml jinja2 colorama
```

#### 权限问题
```bash
# 添加执行权限
chmod +x tools/build/build.py

# 或使用 Python 直接运行
python3 tools/build/build.py
```

### 版本不匹配

```bash
# 检查各工具版本
cmake --version      # >= 3.20
python3 --version    # >= 3.8
arm-none-eabi-gcc --version  # >= 10.0
```

## 获取帮助

如果问题仍未解决：

1. **查看日志**
   ```bash
   python3 tools/build/build.py test --verbose 2>&1 | tee debug.log
   ```

2. **简化测试**
   - 创建最小可复现例子
   - 逐步添加功能确定问题位置

3. **社区支持**
   - [GitHub Discussions](https://github.com/frisky1985/yuleASR/discussions)
   - 创建 Issue 时附上 debug.log

4. **调试技巧**
   - 使用 GDB 调试
   - 添加日志输出
   - 检查内存使用
