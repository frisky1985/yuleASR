# YuleTech OS Configuration Tool

基于 AutoSAR OS 标准的配置工具核心引擎

## 功能特性

1. **配置文件读取** - 支持 YAML 和 JSON 格式
2. **配置验证** - 验证任务优先级、资源冲突等
3. **代码生成** - 自动生成 Os_Cfg.h 和 Os_Cfg.c
4. **命令行支持** - 提供便捷的 CLI 接口

## 安装依赖

```bash
pip install pyyaml jinja2
```

## 快速开始

### 命令行使用

```bash
# 基本用法
python yule_os_config.py -c config.yaml -o ./output

# 详细输出
python yule_os_config.py -c config.yaml -o ./output -v

# 使用自定义模板
python yule_os_config.py -c config.yaml -o ./output -t ./templates

# 显示帮助
python yule_os_config.py -h
```

### 配置文件格式

#### Tasks (任务)

```yaml
tasks:
  - name: "TaskName"          # 任务名称
    priority: 10              # 优先级 (0-255)
    activation: 1             # 激活次数 (1-255)
    autostart: true           # 是否自动启动
    schedule: "FULL"          # 调度类型: FULL/NON
    events: ["Event1"]        # 事件列表 (可选)
    resources: ["Res1"]       # 资源列表 (可选)
```

#### Alarms (报警)

```yaml
alarms:
  - name: "AlarmName"         # 报警名称
    counter: "CounterName"    # 计数器名称
    action: "ACTIVATETASK"    # 动作类型: ACTIVATETASK/SETEVENT/ALARMCALLBACK
    task: "TaskName"          # 目标任务 (可选)
    event: "EventName"        # 目标事件 (可选)
    callback: "CallbackFunc"  # 回调函数 (可选)
    autostart: true           # 是否自动启动
    period: 10                # 周期 (刻)
```

#### Resources (资源)

```yaml
resources:
  - name: "ResourceName"      # 资源名称
    priority_ceiling: 8       # 优先级天花板 (0-255)
```

#### Events (事件)

```yaml
events:
  - name: "EventName"         # 事件名称
    mask: 0x00000001          # 事件掩码 (0-0xFFFFFFFF)
```

#### ScheduleTables (调度表)

```yaml
schedule_tables:
  - name: "TableName"         # 调度表名称
    periodic: true            # 是否周期性
    expiry_points:            # 到期点列表
      - offset: 0             # 偏移量
        tasks: ["Task1"]      # 激活的任务列表
        events: []            # 触发的事件列表
```

## API 接口

```python
from yule_os_config import OSConfigTool

# 创建工具实例
tool = OSConfigTool()

# 处理配置文件
success = tool.process("config.yaml", "./output")

if success:
    print("代码生成成功!")
else:
    print("配置验证失败")
```

## 验证规则

- 任务名称唯一
- 优先级范围: 0-255
- 激活次数范围: 1-255
- 资源天花板范围: 0-255
- 事件掩码范围: 0-0xFFFFFFFF
- 报警动作与参数匹配
- 所有交叉引用必须有效

## 目录结构

```
yule-configurator/os/
├── yule_os_config.py        # 核心引擎
├── example_os_config.yaml  # YAML 配置示例
├── example_os_config.json  # JSON 配置示例
├── test_yule_os_config.py  # 测试脚本
├── README.md               # 使用文档
└── output/                 # 输出目录
    ├── Os_Cfg.h
    └── Os_Cfg.c
```

## 版本

- Version: 1.0.0
- Author: YuleTech
- License: MIT
