---
sidebar_position: 2
---

# 安装

本指南将帮助您在本地环境中安装和配置 YuleTech AutoSAR BSW Platform。

## 环境要求

- C 编译器 (GCC 或 IAR)
- Python 3.9+
- CMake 3.20+
- Git

## 安装步骤

### 1. 克隆仓库

```bash
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
```

### 2. 安装依赖

```bash
pip install -r requirements.txt
```

### 3. 构建项目

```bash
mkdir build && cd build
cmake ..
make
```

## 验证安装

运行测试集：

```bash
make test
```
