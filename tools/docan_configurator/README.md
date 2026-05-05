# DoCan Configurator Tool

Diagnostic over CAN 配置管理工具

## 概述

本工具用于动态配置 AUTOSAR Diagnostic over CAN 模块参数，支持：
- CSV/Excel 配置文件导入
- JSON 配置保存/加载
- C 代码生成 (基于 Jinja2 模板)

## 配置项

- Connections (连接配置)
- Timings (时序参数)
- Buffer (缓冲区配置)

## 快速使用

```bash
# 启动GUI工具
./docan-tool.sh        # Linux/Mac
docan-tool.bat         # Windows
```

## 文件结构

```
docan_configurator/
├── gui/
│   ├── __init__.py
│   └── parser.py          # 配置解析器
├── templates/
│   ├── docan_config.csv   # CSV配置模板
│   └── docan_cfg.j2       # C代码生成模板
├── examples/
│   └── example_config.json    # 示例配置
├── output/                  # 生成的输出文件
├── docan-tool.sh     # Linux/Mac启动脚本
├── docan-tool.bat    # Windows启动脚本
└── requirements.txt       # Python依赖
```

## 依赖

```bash
pip install jinja2
```

## 版本

v1.0.0 - 初始版本
