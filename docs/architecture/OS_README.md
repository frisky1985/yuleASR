# YuleTech AutoSAR OS - FreeRTOS 适配层

## 概述

YuleTech AutoSAR OS 是一个基于 FreeRTOS 的 AutoSAR OS 标准实现，提供完整的 AutoSAR OS 接口封装和可视化配置工具。

## 项目结构

```
yuleASR/
├── src/bsw/os/               # OS 核心代码
│   ├── include/
│   │   ├── Os.h              # AutoSAR OS 标准接口定义
│   │   ├── Os_Cfg.h          # 配置头文件
│   │   └── Std_Types.h       # 标准类型定义
│   └── src/
│       └── Os.c              # FreeRTOS 适配层实现
├── tools/yule-configurator/os/  # 配置工具
│   ├── yule_os_config.py     # 核心引擎
│   ├── web/
│   │   └── index.html       # Web GUI 界面
│   ├── templates/            # 代码生成模板
│   └── examples/             # 配置示例
└── docs/                    # 文档
    ├── OS_INTERFACE.md       # 接口文档
    └── CONFIGURATOR.md       # 配置工具文档
```

## 功能特点

### 1. 标准接口实现
- 完整的 AutoSAR OS 标准接口 (AUTOSAR Release 4.4)
- 支持 Task、Alarm、Event、Resource 等核心功能
- 支持 Schedule Table (调度表)
- 支持多核 (可选)

### 2. FreeRTOS 适配层
- 透明的 FreeRTOS 映射
- 保持 AutoSAR 语义和行为
- 支持错误检查和 Hook 函数

### 3. 配置工具
- 命令行工具 (CLI)
- Web GUI 界面 (响应式设计)
- 代码自动生成
- YAML/JSON 配置格式

## 快速开始

### 使用配置工具

```bash
# 进入配置工具目录
cd tools/yule-configurator/os

# 使用命令行工具
python3 yule_os_config.py -c examples/example_os_config.yaml -o output/

# 或使用 Web GUI
# 用浏览器打开 web/index.html
```

### 编译项目

```bash
# 包含头文件路径
gcc -I src/bsw/os/include -I src/bsw/os/cfg -c src/bsw/os/src/Os.c
```

## 示例代码

```c
#include "Os.h"

TASK(Task_Main)
{
    /* 任务初始化 */
    EventMaskType events;
    
    while (1) {
        /* 等待事件 */
        Os_WaitEvent(EVENT_TIMER | EVENT_MESSAGE);
        Os_GetEvent(Task_Main, &events);
        Os_ClearEvent(events);
        
        /* 处理事件 */
        if (events & EVENT_TIMER) {
            /* 处理定时事件 */
        }
    }
}

int main(void)
{
    /* 启动 OS */
    Os_StartOS(OSDEFAULTAPPMODE);
    return 0;
}
```

## 配置示例

```yaml
os:
  tasks:
    - name: Task_Init
      priority: 5
      schedule: FULL
      autostart: true
      activation: 1
      
    - name: Task_Main
      priority: 3
      schedule: FULL
      autostart: false
      activation: 1
      events:
        - EVENT_TIMER
        - EVENT_MESSAGE
        
  alarms:
    - name: Alarm_Periodic
      counter: SystemCounter
      action: TASK
      task: Task_Main
      
  resources:
    - name: Res_UART
      priority_ceiling: 10
```

## 支持的功能

| 功能 | 状态 | 说明 |
|:-----|:-----|:-----|
| Task 管理 | ✅ 完成 | Activate/Terminate/Chain/Schedule |
| Event 管理 | ✅ 完成 | Wait/Set/Clear/Get |
| Alarm 管理 | ✅ 完成 | SetRel/SetAbs/Cancel/Get |
| Resource 管理 | ✅ 完成 | Get/Release, LIFO 检查 |
| Schedule Table | ✅ 完成 | Start/Stop/Next/Chain |
| 中断管理 | ✅ 完成 | 可嵌套中断禁用/6066复 |
| Hook 函数 | ✅ 完成 | Error/Pre/Post/Startup/Shutdown |
| 多核支持 | ☑️ 基础 | 基本接口定义 |

## 许可

MIT License

## 联系

YuleTech - 致力于嵌入式汽车软件开发
