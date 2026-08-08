# yuleASR Dashboard & Web UI 工具

yuleASR 提供多个基于 Web 的可视化管理工具，用于配置生成、分析报告检视与 CI 监控。

## 工具清单

### 1. ARXML 配置生成器 GUI

- **路径**: `tools/code_generators/arxml/gui/`
- **后端**: Flask API (`gui/api/server.py`)
- **前端**: HTML + CSS + JS (`templates/`, `static/`)
- **启动**: `python3 tools/code_generators/arxml/gui_launcher.py` 或 `gui_qt.py`
- **功能**: 可视化 ARXML ECU 配置生成，支持 MCAL/BSW 模块配置

**架构**:
```
gui/
├── __init__.py          # 模块入口
└── api/
    └── server.py        # Flask REST API — MCAL/BSW 配置端点
templates/
└── index.html           # 主页面
static/
├── css/style.css        # 样式
└── js/app.js            # 前端交互逻辑
```

### 2. CAN 配置工具 Web GUI

- **路径**: `tools/can_config/gui/`
- **启动**: `python3 tools/can_config/launch_web.py`
- **功能**: DBC 文件导入、CAN 矩阵编辑、Com 配置生成
- 另有桌面版 (`launch_desktop.py`)

### 3. 其他配置工具

| 工具 | 路径 | 功能 |
|:-----|:-----|:-----|
| DDS Config | `tools/dds_config/` (Python CLI) + `dds-config-tool/` (C 工具链) | DDS 通信配置生成 |
| DoIP Configurator | `tools/doip_configurator/gui/` | DoIP 诊断配置 |
| UDS Configurator | `tools/uds_configurator/gui/` | UDS 诊断服务配置 |
| DTC Configurator | `tools/dtc_config/gui/` | DTC 故障码配置 |
| OS Configurator | `tools/yule-configurator/os/web/` | FreeRTOS/Optiga OS 参数配置 |

### 4. CI Dashboard

CI 流水线状态通过 `.yuleosh/store.db` 持久化，包含流水线、CI 运行、
审查记录等，可通过 `/api/` 端点查询。

## 通用访问

本地 Flask 服务默认绑定 `0.0.0.0:5000`（可通过环境变量 `PORT` 覆盖）。
API 端点提供 JSON 响应，适合集成到 Prometheus/Grafana 或自有 Dashboard。

## 开发

所有 GUI 工具使用 **Flask** + **Jinja2** 模板引擎，前端组件使用
原生 HTML/CSS/JS，无额外构建依赖。
