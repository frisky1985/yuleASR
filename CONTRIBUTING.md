# 贡献指南

> **YuleTech AutoSAR BSW Platform** — 让每个工程师都能构建可靠的汽车软件

感谢您对 **yuleASR**（YuleTech AutoSAR BSW Platform）的关注与支持！我们欢迎任何形式的贡献，包括报告 Bug、提交功能建议、改进文档、提交代码等。

请花一点时间阅读本指南，以确保您的贡献过程顺畅高效。

---

## 目录

- [行为准则](#行为准则)
- [如何报告 Bug](#如何报告-bug)
- [如何提交功能建议](#如何提交功能建议)
- [如何提交 Pull Request](#如何提交-pull-request)
- [编码规范](#编码规范)
- [测试要求](#测试要求)
- [提交信息格式](#提交信息格式)
- [开发环境搭建](#开发环境搭建)
- [分支管理策略](#分支管理策略)
- [联系方式](#联系方式)

---

## 行为准则

本项目采用 [Contributor Covenant v2.1](CODE_OF_CONDUCT.md) 作为行为准则。所有参与者、维护者和贡献者都应遵守该准则。请阅读 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) 了解详情。

---

## 如何报告 Bug

### 在报告之前

1. **搜索已有 Issue** — 请先在 [GitHub Issues](https://github.com/frisky1985/yuleASR/issues) 中搜索，确认该 Bug 是否已被报告。
2. **确认版本** — 确认您使用的是最新版本，并检查 [CHANGELOG.md](CHANGELOG.md) 中是否已修复。
3. **确认环境** — 确保您的开发环境符合项目要求（见[开发环境搭建](#开发环境搭建)）。

### 使用模板

请使用 Bug 报告模板创建 Issue，并尽可能提供以下信息：

```markdown
**描述**
清晰简洁地描述问题是什么。

**复现步骤**
1. 执行命令 `...`
2. 配置参数 `...`
3. 观察结果 `...`

**期望行为**
描述您期望发生的行为。

**实际行为**
描述实际发生的行为，请包含错误日志。

**环境信息**
- 操作系统: [e.g. Ubuntu 22.04 ARM64]
- 编译器版本: [e.g. arm-none-eabi-gcc 10.3.1]
- CMake 版本: [e.g. 3.25]
- 目标平台: [e.g. S32K312]
- yuleASR 版本: [e.g. v2.0.0]

**日志/截图**
附上相关日志、栈回溯或截图。

**补充信息**
其他有助于定位问题的上下文。
```

> **📎 报告链接:** [创建 Bug Issue](https://github.com/frisky1985/yuleASR/issues/new?template=bug_report.md)

---

## 如何提交功能建议

1. 在 [GitHub Issues](https://github.com/frisky1985/yuleASR/issues) 中搜索是否已有类似提议。
2. 使用功能建议模板创建 Issue，描述：
   - **使用场景** — 该功能解决了什么问题？
   - **期望行为** — 该功能应如何工作？
   - **可选方案** — 您是否考虑过其他实现方式？
   - **补充信息** — 任何有助于理解的上下文或参考资料。

---

## 如何提交 Pull Request

### 流程概览

```
Fork 仓库 → 创建分支 → 编写代码 → 编写测试 → 运行检查 → 提交 PR → Review → 合并
```

### 详细步骤

#### 1. Fork 仓库

点击 GitHub 仓库右上角的 **Fork** 按钮，将项目 fork 到您的个人账号。

#### 2. 克隆并设置 upstream

```bash
# 克隆您的 fork
git clone https://github.com/您的用户名/yuleASR.git
cd yuleASR

# 添加上游仓库
git remote add upstream https://github.com/frisky1985/yuleASR.git

# 确保与上游同步
git fetch upstream
```

#### 3. 创建功能分支

从最新的 `main` 分支创建分支：

```bash
git checkout upstream/main -b feat/your-feature-name
```

分支命名规则：

| 分支类型 | 前缀 | 示例 |
|:---------|:-----|:-----|
| 新功能 | `feat/` | `feat/add-can-fd-support` |
| Bug 修复 | `fix/` | `fix/dcm-null-pointer` |
| 文档改进 | `docs/` | `docs/update-api-reference` |
| 重构 | `refactor/` | `refactor/rte-scheduler` |
| 测试 | `test/` | `test/add-nvm-cases` |
| 性能优化 | `perf/` | `perf/optimize-com-polling` |

#### 4. 编写代码

- 遵循 [编码规范](#编码规范)
- 确保所有现有测试通过
- 为新代码添加相应测试
- 确保测试覆盖率不低于 80%

#### 5. 提交变更

使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式提交：

```bash
git add .
git commit -m "feat(mcal): 添加 CAN FD 支持"
```

详见[提交信息格式](#提交信息格式)。

#### 6. 同步上游变更

在推送前，确保您的分支与上游 `main` 保持同步：

```bash
git fetch upstream
git rebase upstream/main
```

#### 7. 推送并创建 PR

```bash
git push origin feat/your-feature-name
```

然后在 GitHub 上创建 Pull Request，目标分支为 `main`。PR 标题应遵循 Conventional Commits 格式。

#### 8. PR 描述模板

```markdown
**关联 Issue**
Fixes #123

**变更说明**
清晰描述本次 PR 做了什么，解决了什么问题。

**测试说明**
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] MISRA 检查通过
- [ ] 测试覆盖率 >= 80%

**兼容性检查**
- [ ] 不破坏现有 API
- [ ] 不改变现有行为（Bug 修复除外）
- [ ] 文档已更新

**补充信息**
任何审查者需要知道的信息。
```

#### 9. 代码审查

- 至少需要 **1 位维护者** 批准
- 审查者可能要求修改，请及时响应
- 保持 PR 范围聚焦，避免混杂多个不相关的变更

#### 10. 合并

PR 通过审查后，将由维护者执行合并（Squash and Merge）。

---

## 编码规范

本项目严格遵循汽车行业编码标准，所有代码必须通过规范检查。

### 语言标准

- **C 代码**: C99 (ISO/IEC 9899:1999)
- **Python 代码**: Python 3.8+ (PEP 8)
- **Shell 脚本**: POSIX-compliant / Bash

### MISRA C:2012

所有 C 代码必须严格遵循 **MISRA C:2012** 规则：

| 类别 | 要求 |
|:-----|:------|
| **强制规则** | 必须遵守，无一例外 |
| **必要规则** | 必须遵守，如有偏差需评审并记录 |
| **建议规则** | 强烈建议遵守 |

关键规则示例：

- **Rule 1.1**: 不包含任何未定义或关键未确定的行为
- **Rule 8.5**: 每个标识符在翻译单元内只能有一个声明
- **Rule 10.1**: 不允许在表达式中使用隐式整数类型转换
- **Rule 11.1**: 指针类型转换受限
- **Rule 14.1**: 循环计数器应为基本整数类型
- **Rule 16.3**: switch 语句中每个 case 应以 break 结束
- **Rule 21.12**: 不应使用动态内存分配

> MISRA 偏差需在代码注释中使用 `/* MISRA Deviation: <理由> */` 明确标注。

### 命名规范

| 元素 | 规范 | 示例 |
|:-----|:-----|:------|
| 函数名 | 帕斯卡命名（模块前缀） | `Can_Write`, `Dcm_GetStatus` |
| 全局变量 | 匈牙利命名 + 模块前缀 | `Com_GlobalTxPdu` |
| 局部变量 | 下划线命名 | `pdu_handle`, `frame_count` |
| 宏定义 | 全大写 + 下划线 | `CAN_MAX_DLC`, `DCM_BUFFER_SIZE` |
| 类型定义 | 帕斯卡命名 + `_Type` 后缀 | `Can_PduType`, `Dcm_ConfigType` |
| 枚举值 | 全大写 + 下划线 | `CAN_OK`, `CAN_BUSY` |
| 文件名 | 下划线命名 | `can_driver.c`, `dcm_main.c` |

### 代码风格

- **缩进**: 4 个空格（不使用 Tab）
- **括号**: Kernighan & Ritchie 风格（左括号不换行）
- **行宽**: 不超过 120 字符
- **注释**: 使用 `/* */` 多行注释；单行可使用 `//`（C99）
- **头文件保护**: `#ifndef MODULE_NAME_H` / `#define MODULE_NAME_H` / `#endif`
- **函数长度**: 建议不超过 60 行
- **圈复杂度**: 每个函数不超过 10

### DET 集成

所有 BSW 模块必须集成默认错误追踪（Default Error Tracer, DET）：

```c
/* 所有 API 入口检查参数有效性 */
if (pdu_ptr == NULL_PTR) {
    Det_ReportError(MODULE_ID, INSTANCE_ID, API_ID, DET_PARAM_POINTER);
    return E_NOT_OK;
}

/* 所有 API 出口检查执行结果 */
result = SomeOperation();
if (result != E_OK) {
    Det_ReportError(MODULE_ID, INSTANCE_ID, API_ID, DET_INTERNAL_ERROR);
}
```

### 静态分析

提交前需通过静态分析检查：

```bash
# MISRA 检查
./scripts/misra-check.sh

# 使用 cppcheck（项目已配置）
cppcheck --enable=all --suppress=missingIncludeSystem src/

# 或使用 QAC / Coverity（如有许可证）
```

---

## 测试要求

### 覆盖率目标

| 测试类型 | 覆盖率要求 | 工具 |
|:---------|:----------|:-----|
| **语句覆盖** | ≥ 80% | gcov / lcov |
| **分支覆盖** | ≥ 80% | gcov |
| **MC/DC 覆盖** | ≥ 80%（安全关键模块） | VectorCAST / LDRA |
| **函数覆盖** | 100%（公开 API） | gcov |

### 测试层次

#### 1. 单元测试

- 位置: `tests/unit/`
- 框架: Unity / CMock（C 语言）、pytest（Python）
- 要求: 每个公开函数至少一个正向用例、一个边界用例

运行方式：

```bash
# 运行所有单元测试
cd build && ctest --output-on-failure

# 运行特定模块测试
cd build && ctest -R test_can_driver -V
```

#### 2. 集成测试

- 位置: `tests/integration/`
- 验证模块间交互（如 Com → PduR → CanIf → Can）
- 使用模拟硬件或硬件在环（HIL）

```bash
# 运行集成测试
cd build && ctest -R integration -V
```

#### 3. 回归测试

- 每次 PR 合并前自动执行
- 由 GitHub Actions CI 工作流保障

### 编写测试指南

- 测试代码同样遵循项目编码规范
- 使用项目提供的 Mock 层（位于 `tests/mock/`）
- 测试应独立、可重复
- 避免测试间共享状态
- 优先使用 Given-When-Then 风格

```c
/* test_can_write.c - 单元测试示例 */
void test_Can_Write_Success(void)
{
    /* Given */
    Can_PduType pdu = {
        .id = 0x123,
        .length = 8,
        .sdu = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };
    Can_ConfigType config = { /* 有效配置 */ };

    /* When */
    Std_ReturnType result = Can_Write(CAN_CONTROLLER_0, &pdu);

    /* Then */
    TEST_ASSERT_EQUAL(E_OK, result);
}
```

---

## 提交信息格式

本项目使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范，格式如下：

```
<类型>(<作用域>): <描述>

[可选正文]

[可选脚注]
```

### 类型

| 类型 | 说明 |
|:-----|:------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档变更 |
| `style` | 代码格式（不影响功能） |
| `refactor` | 重构（既不修复 Bug 也不添加功能） |
| `perf` | 性能优化 |
| `test` | 添加或修改测试 |
| `chore` | 构建过程或辅助工具变更 |
| `ci` | CI/CD 配置变更 |

### 作用域

| 作用域 | 说明 |
|:-------|:------|
| `mcal` | 微控制器抽象层 |
| `ecual` | ECU 抽象层 |
| `services` | 服务层 |
| `rte` | 运行时环境 |
| `asw` | 应用层 |
| `os` | 操作系统 |
| `config` | 配置模块 |
| `tools` | 工具链 |
| `docs` | 文档 |
| `tests` | 测试 |
| `ci` | CI/CD |

### 示例

```bash
# 新功能
feat(mcal): 添加 CAN FD 支持

# Bug 修复
fix(dcm): 修复 DCM 诊断会话超时处理

# 文档
docs(api): 更新 DCM API 参考文档

# 重构
refactor(rte): 重构 RTE 事件调度逻辑

# 性能
perf(com): 优化 COM 信号路由性能

# 破坏性变更
refactor(core)!: 重构模块初始化接口
```

### Git 提交最佳实践

- **原子提交**: 每个提交只包含一个逻辑变更
- **频率**: 小而频繁的提交优于大而稀疏的提交
- **完整性**: 每个提交应通过所有测试
- **消息**: 第一行不超过 72 字符，正文每行不超过 80 字符

---

## 开发环境搭建

### 推荐配置

| 组件 | 最低版本 | 推荐版本 |
|:-----|:---------|:---------|
| **操作系统** | Ubuntu 20.04 | Ubuntu 22.04 LTS |
| **编译器** | arm-none-eabi-gcc 10.3 | arm-none-eabi-gcc 12.2 |
| **CMake** | 3.20 | 3.27+ |
| **Python** | 3.8 | 3.11 |
| **Git** | 2.30 | 2.40+ |

### 快速开始

```bash
# 1. 安装系统依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 python3-pip python3-venv \
                        ninja-build doxygen cppcheck clang-format-14

# 2. 安装 ARM GCC 工具链
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/
export PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH"

# 3. 安装 Python 依赖
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/arxml/requirements.txt
pip install -r tools/arxml-tool/requirements.txt

# 4. 克隆并初始化项目
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
git submodule update --init --recursive

# 5. 构建
mkdir -p build && cd build
cmake .. -DTARGET_PLATFORM=S32K312 -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 6. 运行测试
ctest --output-on-failure
```

### IDE 推荐

- **VS Code**: 项目包含 `.vscode/` 配置，安装 C/C++ 扩展和 CMake Tools 扩展
- **CLion**: 直接打开 CMakeLists.txt，自动识别项目结构
- **IAR Embedded Workbench**: 用于硬件调试（需许可证）

### Git Hooks

项目提供 Git hooks 用于提交前检查：

```bash
# 安装 hooks
./scripts/install-hooks.sh
```

自动检查包括：
- 提交信息格式验证
- 代码风格检查（clang-format）
- 空白字符检查

---

## 分支管理策略

本项目采用 **Trunk-Based Development** 策略：

```
main — 稳定版本，始终可部署
  ├── feat/* — 功能开发分支
  ├── fix/*  — Bug 修复分支
  ├── docs/* — 文档分支
  └── release/* — 发布准备分支（临时）
```

- 功能分支从 `main` 创建，合并回 `main`
- 分支生命周期短（建议不超过 3 天）
- 合并前必须通过 CI 和 Code Review

---

## 联系方式

如有任何疑问，可通过以下方式联系我们：

| 渠道 | 地址 |
|:-----|:------|
| **GitHub Issues** | <https://github.com/frisky1985/yuleASR/issues> |
| **项目文档站** | <https://frisky1985.github.io/yuleASR/> |
| **公司** | 上海予乐电子科技有限公司 |

---

<p align="center">
  <strong>上海予乐电子科技有限公司</strong><br>
  让每个工程师都能构建可靠的汽车软件
</p>
