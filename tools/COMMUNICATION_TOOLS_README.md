# AUTOSAR 通信协议配置工具集

本目录包含4个AUTOSAR诊断通信协议的配置管理工具：

| 工具 | 协议 | 标准 |
|------|------|------|
| CanTp | CAN Transport Protocol | ISO 15765-2 |
| UDS | Unified Diagnostic Services | ISO 14229 |
| DoCan | Diagnostic over CAN | - |
| DoIP | Diagnostic over IP | ISO 13400 |

## 快速开始

```bash
# 进入任意工具目录
cd cantp_configurator  # 或 uds_configurator, docan_configurator, doip_configurator

# 安装依赖
pip install -r requirements.txt

# 启动工具
./cantp-tool.sh   # Linux/Mac
cantp-tool.bat    # Windows
```

## 功能特点

- **CSV/Excel 配置**: 使用Excel编辑CSV配置文件
- **JSON 导出**: 保存为JSON格式便于版本管理
- **C代码生成**: 基于Jinja2模板生成AUTOSAR标准C头文件
- **GUI界面**: 基于tkinter的跨平台图形界面

## 工具架构

所有工具共享相同的架构模式：

```
templates/
  ├── xxx_config.csv    # CSV配置模板
  └── xxx_cfg.j2        # Jinja2 C代码模板
gui/
  ├── parser.py         # 配置解析器
  └── configurator.py   # GUI工具
examples/
  └── example_config.json
```

## 版本

v1.0.0 - 初始版本 (使用OSH Autonomous Execution V2并行开发)
