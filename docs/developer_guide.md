# ETH-DDS Integration 开发者指南

## 目录

- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [构建系统](#构建系统)
- [测试](#测试)
- [编码规范](#编码规范)
- [贡献指南](#贡献指南)
- [调试技巧](#调试技巧)
- [问题排除](#问题排除)

## 快速开始

### 环境要求

- **操作系统**: Linux (Ubuntu 20.04+) / macOS / Windows (WSL)
- **编译器**: GCC 9+ 或 Clang 10+
- **构建工具**: CMake 3.10+
- **依赖库**: pthread (Linux)

### 获取代码

```bash
git clone https://github.com/your-org/eth-dds-integration.git
cd eth-dds-integration
```

### 快速构建

```bash
# 使用脚本构建
./build.sh

# 或手动构建
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行测试

```bash
cd build
ctest --output-on-failure

# 或运行单个测试
./tests/unit/test_eth_types
```

## 项目结构

```
eth-dds-integration/
├── src/                    # 源代码目录
│   ├── common/              # 通用模块
│   │   ├── types/           # 数据类型定义
│   │   ├── utils/           # 工具函数
│   │   └── config/          # 配置管理
│   ├── ethernet/            # 以太网模块
│   │   ├── driver/         # 驱动程序
│   │   ├── stack/          # 协议栈
│   │   └── api/            # API封装
│   ├── dds/                 # DDS模块
│   │   ├── core/           # 核心功能
│   │   ├── pubsub/         # 发布订阅
│   │   ├── qos/            # 服务质量
│   │   └── discovery/      # 服务发现
│   ├── transport/           # 传输层
│   └── platform/            # 平台抽象层
│       ├── linux/
│       ├── freertos/
│       └── baremetal/
├── tests/                  # 测试目录
│   ├── unity/              # Unity测试框架
│   └── unit/               # 单元测试用例
├── examples/               # 示例项目
│   ├── basic_pubsub.c
│   └── temperature_sensor/ # 温度传感器示例
├── docs/                   # 文档
├── dds-config-tool/        # DDS配置工具
├── CMakeLists.txt
└── Makefile
```

## 构建系统

### CMake选项

| 选项 | 默认 | 描述 |
|-------|-------|-------|
| `BUILD_EXAMPLES` | ON | 构建示例程序 |
| `BUILD_TESTS` | ON | 构建单元测试 |
| `ENABLE_ETHERNET` | ON | 启用以太网驱动 |
| `ENABLE_DDS` | ON | 启用DDS中间件 |
| `ENABLE_COVERAGE` | OFF | 启用代码覆盖率 |

### 构建示例

```bash
# Debug构建
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Release构建
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# 仅构建库（无测试、无示例）
mkdir build-min && cd build-min
cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..
make

# 启用覆盖率
mkdir build-coverage && cd build-coverage
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make
make coverage
```

### 交叉编译

```bash
# ARM Cortex-M4
mkdir build-arm && cd build-arm
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-cortex-m4.cmake ..
make

# 或使用预设脚本
./build.sh --target arm
```

## 测试

### 测试框架

项目使用[Unity](http://www.throwtheswitch.org/unity)作为单元测试框架。测试代码位于`tests/unit/`目录。

### 测试结构

```c
#include "unity.h"
#include "module_under_test.h"

void setUp(void) {
    // 每个测试前执行
}

void tearDown(void) {
    // 每个测试后执行
}

void test_feature_specific_case(void) {
    // 实际测试代码
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_feature_specific_case);
    return UNITY_END();
}
```

### 运行测试

```bash
# 运行所有测试
cd build
ctest -V

# 运行特定测试
./tests/unit/test_eth_types

# 运行带过滤器的测试
ctest -R eth_types

# 运行并生成覆盖报告
make coverage
gcovr -r .. --html -o coverage.html
```

### 添加新测试

1. 在`tests/unit/`创建测试源文件
2. 在`tests/CMakeLists.txt`添加测试到`TEST_SOURCES`
3. 重新配置并构建

## 编码规范

### 命名规范

- **文件名**: 小写下划线连接，如`eth_driver.c`, `dds_qos.h`
- **函数名**: 模块前缀+下划线+动词，如`eth_driver_init()`, `dds_topic_create()`
- **类型名**: 模块前缀+下划线+名称+后缀，如`eth_config_t`, `dds_qos_t`
- **常量/宏**: 大写下划线连接，如`ETH_OK`, `DDS_QOS_RELIABLE`
- **变量名**: 小写驼峰式，如`bufferSize`, `currentStatus`

### 代码风格

```c
/**
 * @file eth_driver.c
 * @brief 简短描述模块功能
 * @version 1.0
 * @date 2026-04-24
 * @author Your Name
 */

#include "eth_driver.h"

/* 全局变量 */
static eth_driver_state_t g_driver_state;

/* 函数实现 */
eth_status_t eth_driver_init(const eth_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 验证参数 */
    if (config->rx_buffer_size < ETH_MIN_BUFFER_SIZE) {
        return ETH_INVALID_PARAM;
    }
    
    /* 初始化逻辑 */
    memcpy(&g_driver_state.config, config, sizeof(eth_config_t));
    g_driver_state.initialized = true;
    
    return ETH_OK;
}
```

### 注释规范

使用Doxygen格式注释：

```c
/**
 * @brief 函数简短说明
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 * @note 额外注意事项
 * @warning 警告信息
 */
```

### 错误处理

```c
eth_status_t process_data(const uint8_t *data, size_t len)
{
    /* 验证输入 */
    if (data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查缓冲区大小 */
    if (len > MAX_PACKET_SIZE) {
        return ETH_ERROR;
    }
    
    /* 执行操作 */
    eth_status_t status = internal_process(data, len);
    if (status != ETH_OK) {
        /* 清理已分配资源 */
        cleanup_resources();
        return status;
    }
    
    return ETH_OK;
}
```

## 贡献指南

### 工作流程

1. **Fork仓库**
2. **创建功能分支**
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **提交更改**
   ```bash
   git commit -m "Add feature: description"
   ```
4. **推送到远程**
   ```bash
   git push origin feature/my-new-feature
   ```
5. **创建Pull Request**

### 提交规范

- 使用描述性提交消息
- 使用现在时态，如"Add" 而非 "Added"
- 应用类型前缀：`feat:`, `fix:`, `docs:`, `test:`, `refactor:`

### PR检查清单

- [ ] 代码符合项目风格规范
- [ ] 添加/更新单元测试
- [ ] 所有测试通过
- [ ] 更新文档
- [ ] 代码审查通过

## 调试技巧

### 使用GDB

```bash
# 编译Debug版本
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
make

# 运行带调试信息的测试
gdb ./tests/unit/test_eth_types
(gdb) break test_eth_buffer_init
(gdb) run
(gdb) print buffer
```

### 日志调试

```c
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[DEBUG] %s:%d: " fmt, \
                                        __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) ((void)0)
#endif
```

### 内存检查

```bash
# 使用Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
    ./tests/unit/test_eth_types

# 使用AddressSanitizer
cmake -DCMAKE_C_FLAGS="-fsanitize=address -g" \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make
./tests/unit/test_eth_types
```

## 问题排除

### 常见问题

#### 构建失败

**问题**: CMake配置失败
```
CMake Error: Could not find CMAKE_ROOT
```

**解决方案**:
```bash
# 确保CMake版本>=3.10
cmake --version
# 如果版本过低，升级CMake
sudo apt-get install cmake
```

**问题**: 缺头文件
```
fatal error: eth_types.h: No such file or directory
```

**解决方案**:
```bash
# 检查头文件搜索路径
cmake -B build -DCMAKE_VERBOSE_MAKEFILE=ON ..
# 确保包含路径正确
```

#### 测试失败

**问题**: 测试缺失库
```
error while loading shared libraries: libeth_dds.so
```

**解决方案**:
```bash
# 设置库路径
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build
# 或使用静态链接
cmake -DBUILD_SHARED_LIBS=OFF ..
```

#### 运行时问题

**问题**: 段错误
```
Segmentation fault (core dumped)
```

**解决方案**:
```bash
# 使用AddressSanitizer调试
cmake -DCMAKE_C_FLAGS="-fsanitize=address -g" ..
make
./your_program 2>&1 | head -50
```

### 获取帮助

- 查阅[API文档](api.md)
- 提交Issue到GitHub仓库
- 查看[常见问题](FAQ.md)

---

*最后更新: 2026-04-24*
