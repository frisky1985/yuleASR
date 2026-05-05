# DoIP Configurator Tool

Diagnostic over IP (ISO 13400) 配置管理工具

## 概述

本工具用于动态配置 AUTOSAR Diagnostic over IP (ISO 13400) 模块参数，支持：
- CSV/Excel 配置文件导入
- JSON 配置保存/加载
- C 代码生成 (基于 Jinja2 模板)

## 配置项

- Entities (实体配置)
- Routing Activation (路由激活)
- Timings (时序参数)

## 快速使用

```bash
# 启动GUI工具
./doip-tool.sh        # Linux/Mac
doip-tool.bat         # Windows
```

## 文件结构

```
doip_configurator/
├── gui/
│   ├── __init__.py
│   └── parser.py          # 配置解析器
├── templates/
│   ├── doip_config.csv   # CSV配置模板
│   └── doip_cfg.j2       # C代码生成模板
├── examples/
│   └── example_config.json    # 示例配置
├── output/                  # 生成的输出文件
├── doip-tool.sh     # Linux/Mac启动脚本
├── doip-tool.bat    # Windows启动脚本
└── requirements.txt       # Python依赖
```

## 依赖

```bash
pip install jinja2
```

## 版本

v1.0.0 - 初始版本
