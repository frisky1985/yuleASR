# DTC Configurator Tool

自动车辆诊断故障码 (DTC) 配置管理工具

## 概述

DTC Configurator Tool 是一个用于动态配置AUTOSAR标准诊断故障码 (DTC) 的图形化工具。该工具允许用户：

- 创建和管理 DTC 定义
- 配置事件 (Events) 和去滑策略
- 管理指示器 (Indicators) 和恢复策略
- 配置操作周期 (Operation Cycles)
- 导入/CSV/Excel 配置文件
- 导出 C 代码以集成到 AUTOSAR 项目

## 功能特性

### 1. DTC 定义管理
- DTC 代码 (十六进制)
- 严重级别 (LOW/MEDIUM/HIGH/CRITICAL)
- DTC 分组 (ALL_DTCS, EMISSION_DTCS, POWERTRAIN_DTCS, CHASSIS_DTCS, BODY_DTCS, NETWORK_DTCS)
- DTC 优先级
- 立即存储设置
- Aging (老化) 配置

### 2. 事件配置
- 事件 ID 管理
- DTC 关联
- 去滑策略 (COUNTER/TIME/MONITOR/NO_DEBOUNCE)
- 错误/通过阈值配置
- Freeze Frame (冻结帧) DID 列表配置
- Extended Data (扩展数据) 配置

### 3. 指示器配置
- 指示器行为 (OFF/CONTINUOUS/BLINKING/SLOW_BLINK/FAST_BLINK)
- 故障/恢复周期阈值

### 4. 操作周期配置
- 周期类型 (IGNITION, OBD_DCY, WARMUP, POWER, DRIVING_CYCLE, TIME, OTHER)
- 自动启动设置

## 安装

### 系统要求
- Python 3.7+
- tkinter (通常包含在Python标准库中)
- jinja2 (用于代码生成)

### 安装依赖
```bash
pip install -r requirements.txt
```

## 使用方法

### 启动GUI工具
```bash
# Windows
python -m gui.dtc_configurator

# Linux/Mac
python3 -m gui.dtc_configurator

# 或者使用启动脚本
./dtc-tool.sh  # Linux/Mac
dtc-tool.bat   # Windows
```

### 命令行解析CSV
```bash
python -m gui.config_parser templates/dtc_config.csv
```

## 文件结构

```
tools/dtc_configurator/
├── gui/
│   ├── __init__.py
│   ├── dtc_configurator.py    # 主GUI应用
│   └── config_parser.py       # 配置解析器
├── templates/
│   ├── dtc_config.csv         # CSV配置模板
│   ├── c_code_template.j2     # C代码生成模板 (Jinja2)
│   └── example_dtc_config.json # 示例配置
├── output/                  # 生成的输出文件
├── excel/                   # Excel配置文件
├── requirements.txt         # Python依赖
├── dtc-tool.sh             # Linux/Mac启动脚本
├── dtc-tool.bat            # Windows启动脚本
└── README.md               # 本文件
```

## 配置文件格式

### CSV格式
CSV配置文件支持多个配置段：

```csv
# DTC Basic Information
DTC_CODE,SEVERITY,FUNCTIONAL_UNIT,DTC_GROUP,PRIORITY,KIND,DESCRIPTION
0x010101,HIGH,0x01,EMISSION,1,EMISSION_REL_DTCS,Engine Misfire Detected

# Event Configuration  
EVENT_ID,DTC_CODE,EVENT_NAME,DEBOUNCE_TYPE,DEBOUNCE_FAILED_THR,DEBOUNCE_PASSED_THR
0,0x010101,Misfire_Cyl_1,COUNTER,127,-128

# Freeze Frame Configuration
EVENT_ID,RECORD_NUMBER,DID_LIST
0,1,"0x0100;0x0101;0x0105"

# Indicator Configuration
EVENT_ID,INDICATOR_ID,BEHAVIOR,FAILURE_CYCLES,HEALING_CYCLES
0,0,BLINKING,3,3
```

### JSON格式
完整的JSON配置结构包含：

```json
{
  "project_name": "My_AUTOSAR_Project",
  "version": {"major": 1, "minor": 0, "patch": 0},
  "generation_date": "2024-01-15",
  "dtcs": [...],
  "events": [...],
  "indicators": [...],
  "operation_cycles": [...]
}
```

## 代码生成

工具支持生成AUTOSAR标准C代码，包括：

- `Dem_DtcConfig.h` - 完整的DTC配置头文件
- DTC定义表
- 事件配置表
- Freeze Frame配置
- Extended Data配置
- 指示器配置
- 操作周期配置

## 快捷键

| 快捷键 | 功能 |
|---------|------|
| Ctrl+N | 新建配置 |
| Ctrl+O | 打开配置 |
| Ctrl+S | 保存配置 |
| 双击表格行 | 编辑选中项 |

## 版本历史

### v1.0.0 (2024-01-15)
- 初始版本发布
- 支持DTC、事件、指示器、操作周期管理
- CSV导入/导出
- C代码生成

## 技术支持

问题报告和功能请求，请联系项目维护者。

## 许可证

本项目作为yuleASR的一部分。
