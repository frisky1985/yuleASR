# DDS配置工具链 (dds-config-tool)

支持可视化配置QoS参数并生成符合microdds API的C代码的工具链。

## 功能特性

- **多格式支持**: XML和JSON配置文件
- **完整验证**: 验证DDS配置合法性
- **代码生成**: 生成C头文件和源文件
- **模板支持**: 支持Jinja2自定义模板
- **QoS支持**: Reliability, Durability, Deadline, LatencyBudget, Liveliness, History, ResourceLimits, Lifespan

## 安装

### 从源码安装

```bash
cd tools/dds-config-tool
pip install -e .
```

### 安装依赖

```bash
pip install -r requirements.txt
```

## 快速开始

### 1. 创建配置文件

**XML格式** (`vehicle_dds.xml`):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="VehicleDDS" version="1.0.0">
    <domain_participant name="VehicleDomain" domain_id="0">
        <topics>
            <topic name="VehicleSpeed" type_name="Vehicle::SpeedData">
                <qos>
                    <reliability kind="BEST_EFFORT"/>
                    <history kind="KEEP_LAST" depth="1"/>
                </qos>
            </topic>
        </topics>
    </domain_participant>
</dds_config>
```

**JSON格式** (`vehicle_dds.json`):

```json
{
    "name": "VehicleDDS",
    "version": "1.0.0",
    "domain_participants": [
        {
            "name": "VehicleDomain",
            "domain_id": 0,
            "topics": [
                {
                    "name": "VehicleSpeed",
                    "type_name": "Vehicle::SpeedData",
                    "qos": {
                        "reliability": { "kind": "BEST_EFFORT" },
                        "history": { "kind": "KEEP_LAST", "depth": 1 }
                    }
                }
            ]
        }
    ]
}
```

### 2. 生成代码

```bash
# 生成代码
dds-config-tool vehicle_dds.xml -o ./generated

# 详细输出
dds-config-tool vehicle_dds.xml -o ./generated -v

# 仅验证配置
dds-config-tool vehicle_dds.xml --validate-only

# 使用自定义前缀
dds-config-tool vehicle_dds.xml -o ./generated -p mydds
```

### 3. 生成的文件

工具将生成以下文件：

- `dds_config.h` - 配置头文件
- `dds_domain_config.c` - 域配置
- `dds_topic_config.c` - 主题配置
- `dds_qos_config.c` - QoS配置

### 4. 使用生成的代码

```c
#include "dds_config.h"

int main(void) {
    /* 使用生成的配置 */
    DDS_DomainParticipant participant = DDS_DomainParticipant_create(
        0,  /* domain_id */
        &dds_default_qos.domain_participant_qos
    );
    
    /* 创建主题时使用生成的QoS */
    DDS_Topic topic = DDS_Topic_create(
        participant,
        "VehicleSpeed",
        "Vehicle::SpeedData",
        &dds_VehicleDomain_VehicleSpeed.qos
    );
    
    return 0;
}
```

## 命令行参数

```
dds-config-tool [-h] [-o OUTPUT] [-p PREFIX] [-t TEMPLATE_DIR] [-v]
                [--validate-only] [--version]
                input

位置参数:
  input                 输入配置文件路径 (.xml 或 .json)

可选参数:
  -h, --help            显示帮助信息并退出
  -o OUTPUT, --output OUTPUT
                        输出目录 (默认: ./generated)
  -p PREFIX, --prefix PREFIX
                        生成文件名前缀 (默认: dds)
  -t TEMPLATE_DIR, --template-dir TEMPLATE_DIR
                        自定义模板目录
  -v, --verbose         详细输出
  --validate-only       仅验证配置文件，不生成代码
  --version             显示版本信息并退出
```

## QoS策略配置

### Reliability (可靠性)

```xml
<reliability kind="RELIABLE" max_blocking_time_sec="0" max_blocking_time_nsec="100000000"/>
```

支持的值: `BEST_EFFORT`, `RELIABLE`

### Durability (持久性)

```xml
<durability kind="TRANSIENT_LOCAL"/>
```

支持的值: `VOLATILE`, `TRANSIENT_LOCAL`, `TRANSIENT`, `PERSISTENT`

### Deadline (截止日期)

```xml
<deadline period_sec="0" period_nsec="500000000"/>
```

### History (历史)

```xml
<history kind="KEEP_LAST" depth="10"/>
```

支持的值: `KEEP_LAST`, `KEEP_ALL`

## 目录结构

```
dds-config-tool/
├── dds_config_tool/          # 主包
│   ├── __init__.py
│   ├── parser.py              # 配置解析器
│   ├── validator.py          # 配置验证器
│   ├── generator.py          # 代码生成器
│   └── templates/            # Jinja2模板
│       ├── header.h.j2
│       ├── domain_config.c.j2
│       ├── topic_config.c.j2
│       └── qos_config.c.j2
├── examples/               # 示例配置
├── tests/                  # 测试套件
├── cli.py                  # 命令行入口
├── setup.py               # 包安装
├── requirements.txt       # 依赖
└── README.md              # 本文件
```

## 测试

```bash
# 运行测试
pytest

# 详细输出
pytest -v

# 生成覆盖率报告
pytest --cov=dds_config_tool --cov-report=html
```

## 自定义模板

可以通过自定义Jinja2模板来控制生成的代码格式：

```bash
dds-config-tool config.xml -t ./my_templates
```

模板中可用的变量：

- `config` - DDS配置对象
- `prefix` - 前缀名称
- `prefix_upper` - 大写前缀
- `prefix_lower` - 小写前缀

## 版本历史

### v1.0.0 (2024-05)
- 初始版本
- 支持XML和JSON配置
- 支持基本QoS策略
- 代码生成功能

## 许可证

MIT License

## 联系方式

- 公司: 上海予乐电子科技有限公司
- 项目: YuleTech DDS Config Tool
