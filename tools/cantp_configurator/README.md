# CanTp Configurator Tool

CAN Transport Protocol (ISO 15765-2) 配置管理工具

## 概述

本工具用于动态配置 AUTOSAR CAN Transport Protocol (ISO 15765-2) 模块参数，支持：
- CSV/Excel 配置文件导入
- JSON 配置保存/加载
- C 代码生成 (基于 Jinja2 模板)

## 配置项

- Channels (CAN FD/2.0)
- TxNSdu (发送NSDU)
- RxNSdu (接收NSDU)
- General (通用参数)

## 快速使用

```bash
# 启动GUI工具
./cantp-tool.sh        # Linux/Mac
cantp-tool.bat         # Windows
```

## 文件结构

```
cantp_configurator/
├── gui/
│   ├── __init__.py
│   └── parser.py          # 配置解析器
├── templates/
│   ├── cantp_config.csv   # CSV配置模板
│   └── cantp_cfg.j2       # C代码生成模板
├── examples/
│   └── example_config.json    # 示例配置
├── output/                  # 生成的输出文件
├── cantp-tool.sh     # Linux/Mac启动脚本
├── cantp-tool.bat    # Windows启动脚本
└── requirements.txt       # Python依赖
```

## 依赖

```bash
pip install jinja2
```

## 版本

v1.0.0 - 初始版本
