# 快速开始

## 环境要求

- **操作系统**: Linux (Ubuntu 20.04+), Windows 10/11 (WSL2), macOS (12+)
- **编译器**: GCC ARM 10.3+, Clang 12+
- **Python**: 3.8+ (pip, venv)
- **CMake**: 3.20+
- **Git**: 2.30+

## 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 python3-pip python3-venv

# 安装 Python 依赖
pip3 install -r tools/arxml/requirements.txt
```

## 获取代码

```bash
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
git submodule update --init --recursive
```

## 构建

```bash
# 使用构建脚本（推荐）
./build.sh --platform S32K312

# 或手动构建
mkdir -p build && cd build
cmake .. -DTARGET_PLATFORM=S32K312
make -j$(nproc)

# 运行测试
make test
ctest --output-on-failure
```

## 使用工具链

```bash
# ARXML 工具
python3 tools/arxml/arxml_tool.py --help
python3 tools/arxml/arxml_tool.py parse examples/system.arxml

# CAN 配置
python3 tools/can_config/can-config-tool.py --dbc examples/example.dbc

# DTC 配置
bash tools/dtc_config/dtc-tool.sh --input dtc_config.csv

# RTE 代码生成
python3 tools/rte-generator/rte_generator.py \
  --config config/rte_cfg.json --output src/rte/
```

## 目录结构

```
yuleASR/
├── src/                  # 源代码
│   ├── bsw/             # AutoSAR BSW (MCAL + ECUAL + Services)
│   ├── asw/             # 应用层组件
│   ├── rte/             # 运行时环境
│   └── micro-dds/       # DDS 中间件
├── config/              # 配置文件 (117 个)
├── tests/               # 测试 (260+)
├── tools/               # 工具链 (12+ 工具)
├── docs/                # 文档 (150+)
└── docs-site/           # 文档站 (Vite + React)
```

## 下一步

- 查阅 [BSW 模块清单](modules.md) 了解所有可用模块
- 阅读 [架构概览](https://frisky1985.github.io/yuleASR/)
- 查看 [迁移指南](migration-guide.md)（从 EasyXMen 迁移）
