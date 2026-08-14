# AUTOSAR ARXML Configuration Tool

## 概述

这是一套用于解析、生成和验证AUTOSAR ARXML配置文件的工具集。它支持从ARXML文件生成C语言配置代码，并检查ARXML文件的完整性。

## 功能特点

- **ARXML解析**: 支持AUTOSAR R20-11标准的ARXML文件解析
- **配置生成**: 生成符合AUTOSAR规范的C语言配置代码
- **完整性检查**: 验证ARXML文件的语法和语义正确性
- **模块支持**: COM、CanIf、NvM、PduR等常见模块

## 目录结构

```
tools/arxml/
├── arxml_tool.py          # 主入口脚本
├── README.md              # 本文件
├── parser/                # ARXML解析器
│   ├── arxml_parser.py    # 核心解析器
│   └── __init__.py        # 模块入口
├── generator/             # 配置生成器
│   └── config_generator.py
├── checker/               # 完整性分析器
│   └── integrity_checker.py
└── utils/                 # 工具函数
```

## 安装要求

- Python 3.8+
- 无外部依赖（只使用Python标准库）

## 快速开始

### 1. 解析ARXML文件

```bash
cd /home/admin/yuleASR
python tools/arxml/arxml_tool.py parse config/input/arxml/example.arxml --output parsed.json
```

### 2. 生成配置代码

```bash
# 从ARXML直接生成
python tools/arxml/arxml_tool.py generate config/input/arxml/example.arxml --output-dir ./generated

# 或从解析后的JSON生成
python tools/arxml/arxml_tool.py generate parsed.json --output-dir ./generated
```

### 3. 检查完整性

```bash
python tools/arxml/arxml_tool.py check config/input/arxml/example.arxml --report report.txt
```

## 使用示例

### 解析ARXML

```python
from arxml.parser import ARXMLParser

parser = ARXMLParser()
parser.parse_file('example.arxml')

# 获取软件组件
components = parser.parse_software_components()
for comp in components:
    print(f"Component: {comp.name} (Type: {comp.component_type})")

# 获取数据类型
data_types = parser.parse_data_types()

# 获取端口接口
interfaces = parser.parse_port_interfaces()

# 解析所有内容
result = parser.parse_all()
```

### 生成配置

```python
from arxml.generator import ConfigGenerator

generator = ConfigGenerator(output_dir='./generated')
generated_files = generator.generate_from_json('parsed.json')

for name, path in generated_files.items():
    print(f"Generated: {name} -> {path}")
```

### 检查完整性

```bash
# 基本检查
python tools/arxml/checker/integrity_checker.py example.arxml

# 生成详细报告
python tools/arxml/checker/integrity_checker.py example.arxml -o report.txt

# 只显示摘要
python tools/arxml/checker/integrity_checker.py example.arxml -s

# 批量检查
python tools/arxml/checker/integrity_checker.py file1.arxml file2.arxml file3.arxml

# 递归检查目录
python tools/arxml/checker/integrity_checker.py -r ./arxml_files/
```

## 支持的ARXML元素

### 已实现

| 元素类型 | 说明 |
|---------|------|
| ECU-CONFIGURATION | ECU配置信息 |
| APPLICATION-SW-COMPONENT-TYPE | 应用软件组件 |
| SERVICE-SW-COMPONENT-TYPE | 服务软件组件 |
| COMPOSITION-SW-COMPONENT-TYPE | 组合软件组件 |
| SWC-INTERNAL-BEHAVIOR | 软件组件内部行为 |
| RUNNABLE-ENTITY | Runnable实体 |
| P-PORT-PROTOTYPE | 提供端口 |
| R-PORT-PROTOTYPE | 需求端口 |
| SENDER-RECEIVER-INTERFACE | 发送-接收接口 |
| CLIENT-SERVER-INTERFACE | 客户端-服务端接口 |
| MODE-SWITCH-INTERFACE | 模式切换接口 |
| APPLICATION-PRIMITIVE-DATA-TYPE | 应用原始数据类型 |
| IMPLEMENTATION-DATA-TYPE | 实现数据类型 |
| SW-DATA-DEF-PROPS | 软件数据定义属性 |

### 生成的配置模块

| 模块 | 生成文件 |
|------|---------|
| COM | Com_Cfg.h, Com_Cfg.c |
| CanIf | CanIf_Cfg.h |
| NvM | NvM_Cfg.h |
| PduR | PduR_Cfg.h |

## 完整性检查规则

工具会检查以下方面：

1. **XML语法正确性**: 确保XML格式正确
2. **必需元素**: 验证SHORT-NAME等必需字段存在
3. **UUID唯一性**: 检查UUID是否重复
4. **引用关系**: 验证引用是否指向有效实体
5. **数据类型**: 确保所有数据类型已定义
6. **参数范围**: 检查数值参数是否在有效范围内

## 命令行参考

### 主工具 (arxml_tool.py)

```
Usage: arxml_tool.py [command] [options]

Commands:
    parse       Parse ARXML file to JSON
    generate    Generate C configuration
    check       Check ARXML integrity

Options:
    -h, --help      Show help message
    -v, --version   Show version
```

### 解析器 (arxml_parser.py)

```
Usage: arxml_parser.py <arxml_file> [options]

Options:
    -o, --output    Output JSON file
    -v, --verbose   Verbose output
```

### 检查器 (integrity_checker.py)

```
Usage: integrity_checker.py <file.arxml> [options]

Options:
    -o, --output        Output report file
    -s, --summary       Summary only
    -r, --recursive     Recursive directory check
```

## 开发计划

### 已完成

- [x] ARXML解析器核心功能
- [x] 配置生成器基础框架
- [x] 完整性分析器
- [x] 主入口脚本
- [x] 示例ARXML文件

### 计划中

- [ ] 支持更多模块（DoIp、SoAd等）
- [ ] 完善的RTE配置生成
- [ ] ARXML Schema验证
- [ ] 可视化配置编辑器
- [ ] 与主流工具链的接口

## 贡献指南

欢迎提交Issue和Pull Request。请确保：

1. 代码符合Python PEP 8规范
2. 添加适当的单元测试
3. 更新文档

## 授权

本工具集是yuleASR项目的一部分，遵循项目的开源协议。

## 联系方式

如有问题或建议，请通过GitHub Issue与我们联系。
