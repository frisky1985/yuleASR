---
sidebar_position: 99
---

# 贡献指南

感谢您对 YuleTech 的关注！我们欢迎各种形式的贡献。

## 如何贡献

### 1. 报告问题

发现 bug 或有改进建议？请通过 GitHub Issues 告诉我们：

1. 检查是否已存在相关 Issue
2. 使用模板创建新 Issue
3. 提供完整的复现步骤和环境信息

### 2. 提交代码

#### 工作流程

```bash
# 1. Fork 仓库
# 2. 克隆您的 Fork
git clone https://github.com/YOUR_USERNAME/yuleASR.git
cd yuleASR

# 3. 创建分支
git checkout -b feature/my-feature

# 4. 进行更改
git add .
git commit -m "feat: 添加新功能"

# 5. 推送到您的 Fork
git push origin feature/my-feature

# 6. 创建 Pull Request
```

#### 提交信息规范

我们使用 [Conventional Commits](https://www.conventionalcommits.org/)：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型:**

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具相关

**Scope 范围:**

- `mcal`: MCAL 层驱动
- `ecual`: ECUAL 层
- `services`: 服务层
- `docs`: 文档
- `build`: 构建系统
- `tests`: 测试框架

**示例:**

```bash
feat(mcal): 添加 CAN 驱动的过滤功能

实现了 CAN 消息过滤，支持标准和扩展标识符过滤。

Closes #123
```

## 代码规范

### C 代码规范

#### 格式化

使用 `clang-format` 统一代码格式：

```bash
find src -name '*.c' -o -name '*.h' | xargs clang-format -i
```

配置文件: `.harness/lint-rules/.clang-format`

#### 命名规范

- **文件名**: `ModuleName.c`, `ModuleName.h`
- **函数名**: `Module_FunctionName` (PascalCase)
- **宏**: `MODULE_NAME` (UPPER_SNAKE_CASE)
- **变量**: `localVariable` (camelCase)
- **全局变量**: `g_GlobalVariable` (g_ 前缀)

#### 示例

```c
/**
 * @file Port.c
 * @brief Port 驱动实现
 * @version 1.0.0
 */

#include "Port.h"

/**
 * @brief 初始化 Port 模块
 * @param ConfigPtr 配置指针
 * @return Std_ReturnType E_OK: 成功, E_NOT_OK: 失败
 */
Std_ReturnType Port_Init(const Port_ConfigType* ConfigPtr)
{
    Std_ReturnType result = E_OK;
    
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(PORT_MODULE_ID, PORT_INSTANCE_ID, 
                        PORT_INIT_SID, PORT_E_PARAM_POINTER);
        result = E_NOT_OK;
    } else {
        /* 初始化逻辑 */
        for (uint8 i = 0; i < PORT_NUM_PINS; i++) {
            g_PortConfig[i] = ConfigPtr->PinConfig[i];
        }
    }
    
    return result;
}
```

### 文档规范

- 使用 Markdown 格式
- 代码块指定语言类型
- 图片存放在 `static/img/`

## 测试要求

### 单元测试

所有新功能必须包含单元测试：

```bash
# 创建测试文件
touch tests/unit/mcal/test_new_feature.c
```

测试文件结构：

```c
#include "unity.h"
#include "mock_mcal.h"

void setUp(void) {
    /* 每个测试前执行 */
}

void tearDown(void) {
    /* 每个测试后执行 */
}

void test_FunctionName_NormalCase(void) {
    // Arrange
    // Act
    // Assert
    TEST_ASSERT_EQUAL(expected, actual);
}
```

### 运行测试

```bash
python3 tools/build/build.py test
```

## 静态分析

提交前运行静态分析：

```bash
python3 tools/build/build.py lint
```

## PR 检查清单

创建 PR 前，请确保：

- [ ] 代码格式化通过 (`clang-format`)
- [ ] 静态分析无警告 (`cppcheck`)
- [ ] 单元测试通过
- [ ] 提交信息符合规范
- [ ] 文档已更新（如需）；

## 开发环境设置

### 推荐工具

- **IDE**: VS Code + C/C++ 扩展
- **调试器**: J-Link / ST-Link
- **代码分析**: Cppcheck, clang-tidy

### VS Code 配置

安装插件：

- C/C++ (Microsoft)
- CMake Tools
- clangd

### 调试配置

在 `.vscode/launch.json` 中配置调试器：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Tests",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/test_runner",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb"
        }
    ]
}
```

## 获得帮助

- 查看 [FAQ](/docs/faq)
- 参与 [Discussions](https://github.com/frisky1985/yuleASR/discussions)
- 加入社区交流

## 认证开发者

活跃贡献者可以申请成为认证开发者，获得更多存仓权限。

## 行为准则

我们遵循 [Contributor Covenant](https://www.contributor-covenant.org/)
行为准则。请保持尊重、专业的沟通。

---

再次感谢您的贡献！🚀
