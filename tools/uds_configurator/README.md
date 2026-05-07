# UDS Configurator Tool

Unified Diagnostic Services (ISO 14229) 配置管理工具

## 概述

本工具用于动态配置 AUTOSAR Unified Diagnostic Services (ISO 14229) 模块参数，支持：
- CSV/Excel 配置文件导入
- JSON 配置保存/加载
- C 代码生成 (基于 Jinja2 模板)

## 配置项

- Sessions (诊断会话)
- Security Access (安全访问)
- Services (诊断服务)
- DIDs (数据标识符)
- RIDs (例行程序)

## 快速使用

```bash
# 启动GUI工具
./uds-tool.sh        # Linux/Mac
uds-tool.bat         # Windows
```

## 文件结构

```
uds_configurator/
├── gui/
│   ├── __init__.py
│   └── parser.py          # 配置解析器
├── templates/
│   ├── uds_config.csv   # CSV配置模板
│   └── uds_cfg.j2       # C代码生成模板
├── examples/
│   └── example_config.json    # 示例配置
├── output/                  # 生成的输出文件
├── uds-tool.sh     # Linux/Mac启动脚本
├── uds-tool.bat    # Windows启动脚本
└── requirements.txt       # Python依赖
```

## 依赖

```bash
pip install jinja2
```

## 版本

v1.0.0 - 初始版本
