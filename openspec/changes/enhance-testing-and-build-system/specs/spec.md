# 变更规范: 增强测试框架和构建系统

## 概述

使用 superpowers 技能对 YuleTech AutoSAR BSW Platform 进行全面增强，重点解决测试覆盖率不足和构建系统不完善的问题。

## 变更范围

### 1. 测试框架

创建完整的测试框架，包括：
- 单元测试目录结构
- Mock 系统 (DET, MCAL, ECUAL, Services, OS)
- MCAL 层单元测试 (130+ 测试用例)
- 集成测试
- 测试工具脚本

### 2. 构建系统

完善 CMake 配置：
- 项目顶层 CMakeLists.txt
- 测试目标支持
- 覆盖率配置
- 地址检查器支持

### 3. 代码质量

添加静态分析配置：
- cppcheck 配置
- clang-format 配置
- CI/CD 集成

### 4. 验证报告

创建验证报告：
- Com_verification.md
- Dcm_verification.md
- Dem_verification.md

## 质量门禁

- [x] 测试框架完整，支持所有 MCAL 驱动
- [x] CMake 配置正确，可构建和测试
- [x] 静态分析配置完善
- [x] 验证报告完整
- [x] CI/CD 工作流更新

## 文件列表

### 创建的文件
- tests/TEST_FRAMEWORK_SUMMARY.md\n- tests/README.md\n- tests/FRAMEWORK_GUIDE.md\n- tests/CMakeLists.txt\n- tests/mocks/mock_ecual.c\n- tests/mocks/mock_services.c\n- tests/mocks/mock_os.c\n- tests/mocks/mock_os.h\n- tests/mocks/mock_det.h\n- tests/mocks/mock_mcal.c\n- ... (共 27 个测试文件)\n\n### 构建相关\n- CMakeLists.txt\n- tools/build/CMakeLists.txt\n- tools/build/build.py\n- .harness/lint-rules/cppcheck.xml\n- .harness/lint-rules/.clang-format\n- .github/workflows/ci.yml\n\n### 验证报告\n- verification/Com_verification.md\n- verification/Dcm_verification.md\n- verification/Dem_verification.md\n
## 使用方法

### 构建项目
```bash
# 配置并构建
python3 tools/build/build.py configure --tests
python3 tools/build/build.py build

# 或使用 CMake 直接
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

### 运行测试
```bash
# 运行所有测试
python3 tools/build/build.py test

# 或使用 Python 测试运行器
cd tests
python3 tools/run_tests.py -l mcal -r html
```

### 静态分析
```bash
# 运行 cppcheck
python3 tools/build/build.py lint

# 代码格式化
python3 tools/build/build.py format
```

## 验证结果

| 项目 | 改进前 | 改进后 |
|------|---------|---------|
| 测试文件数 | 1 | 22+ |
| 测试用例数 | - | 130+ |
| 验证报告 | 6 | 9 |
| CI/CD 测试 | 无 | 有 |
| 静态分析 | 无 | 有 |
