# YuleTech OS Configurator 使用手册

## 简介

YuleTech OS Configurator 是专为 AutoSAR OS 配置设计的可视化工具，支持从配置到代码生成的完整工作流。

## 安装

### 依赖

```bash
pip3 install pyyaml jinja2
```

### 安装工具

```bash
cd tools/yule-configurator/os
python3 -m pip install -e .
```

## 使用方法

### 方法一: 命令行工具

```bash
# 基本用法
python3 yule_os_config.py -c <config_file> -o <output_dir>

# 完整示例
python3 yule_os_config.py \
    -c examples/example_os_config.yaml \
    -o output/ \
    -v

# 帮助
python3 yule_os_config.py -h
```

### 方法二: Web GUI

```bash
# 方法 1: 直接打开文件
open web/index.html

# 方法 2: 使用简单 HTTP 服务器
cd web
python3 -m http.server 8080
# 然后访问 http://localhost:8080
```

## 配置文件格式

### YAML 格式 (推荐)

```yaml
os:
  # 系统配置
  system:
    status_level: EXTENDED      # 错误检查级别: STANDARD/EXTENDED
    use_get_service_id: true    # 启用服务ID获取
    use_parameter_access: true  # 启用参数访问
    
  # 任务配置
  tasks:
    - name: Task_Init           # 任务名称
      priority: 5               # 优先级 (0-255)
      schedule: FULL            # 调度策略: FULL/NON
      autostart: true           # 自动启动
      activation: 1             # 激活数量
      stack_size: 1024          # 堆栈大小
      events:                   # 事件列表 (可选)
        - EVENT_INIT
        - EVENT_READY
        
    - name: Task_Cyclic
      priority: 3
      schedule: FULL
      autostart: false
      activation: 1
      stack_size: 512
      
  # Alarm 配置
  alarms:
    - name: Alarm_10ms
      counter: SystemCounter    # 关联计数器
      action: TASK              # 动作类型: TASK/EVENT/CALLBACK
      task: Task_Cyclic         # 目标任务 (如果 action 是 TASK)
      event: 0x0001            # 事件掩码 (如果 action 是 EVENT)
      
  # 资源配置
  resources:
    - name: Res_UART
      priority_ceiling: 10      # 优先级天花板
      
  # 事件配置
  events:
    - name: EVENT_INIT
      mask: "0x0001"           # 事件掩码
    - name: EVENT_READY
      mask: "0x0002"
      
  # 计数器配置
  counters:
    - name: SystemCounter
      maxallowedvalue: 65535
      ticksperbase: 1
      mincycle: 1
```

### JSON 格式

```json
{
  "os": {
    "tasks": [
      {
        "name": "Task_Init",
        "priority": 5,
        "schedule": "FULL",
        "autostart": true
      }
    ]
  }
}
```

## 生成的文件

### Os_Cfg.h

```c
#ifndef OS_CFG_H
#define OS_CFG_H

/* 版本信息 */
#define OS_VENDOR_ID                    0x0001
#define OS_MODULE_ID                    0x0001
#define OS_AR_RELEASE_MAJOR_VERSION     4
#define OS_AR_RELEASE_MINOR_VERSION     4

/* 任务定义 */
#define TASK_TASK_INIT                  0
#define TASK_TASK_CYCLIC                1
#define OS_TASK_COUNT                   2

/* Alarm 定义 */
#define ALARM_ALARM_10MS                0
#define OS_ALARM_COUNT                  1

/* 资源定义 */
#define RES_RES_UART                    0
#define OS_RESOURCE_COUNT               1

/* 事件定义 */
#define EVENT_EVENT_INIT                0x0001
#define EVENT_EVENT_READY               0x0002

#endif /* OS_CFG_H */
```

### Os_Cfg.c

```c
#include "Os.h"
#include "Os_Cfg.h"

/* 任务控制块 */
static Os_TaskControlBlockType TaskControlBlocks[OS_TASK_COUNT];

/* 任务配置表 */
const Os_TaskConfigType Os_TaskConfig[OS_TASK_COUNT] = {
    {
        .name = "Task_Init",
        .priority = 5,
        .schedule = SCHEDULE_FULL,
        .autostart = TRUE,
        .activation = 1,
        .stack_size = 1024
    },
    /* ... */
};

/* FreeRTOS 任务句柄 */
TaskHandle_t Os_FreeRTOSTaskHandles[OS_TASK_COUNT];
```

## 验证规则

配置工具会自动检查以下规则：

1. **任务名称唯一性** - 不允许重复的任务名
2. **优先级范围** - 0-255 之间
3. **交叉引用检查** - Alarm 引用的任务必须存在
4. **资源天花板合法性** - 必须高于使用该资源的所有任务优先级

## 常见问题

### Q: 如何导入现有配置？
A: 在 Web GUI 中点击"导入"按钮，选择 YAML 或 JSON 配置文件。

### Q: 如何与构建系统集成？
A: 生成的文件可直接用于 CMake/Make 项目。

### Q: 支持哪些编译器？
A: 生成的代码符合 C99 标准，支持 GCC、IAR、Keil 等主流编译器。
