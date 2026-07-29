# yuleASR ARXML Tool

yuleASR ARXML工具 - 统一的ARXML解析、配置生成和完整性分析命令行工具。

## 功能特性

- **ARXML解析**: 解析AUTOSAR R4.0 ARXML文件，提取软件组件、接口、数据类型、ECU配置等
- **配置生成**: 从ARXML或JSON生成AUTOSAR BSW配置文件（C头文件、源文件、ARXML）
- **完整性分析**: 检查ARXML的结构和语义完整性，支持严格/宽松模式
- **Schema验证**: 验证ARXML文件符合AUTOSAR XSD Schema

## 安装

```bash
# 克隆工具库
cd /home/admin/yuleASR/tools/arxml-tool

# 安装依赖
pip install -r requirements.txt

# 设置可执行权限
chmod +x arxml-tool.py

# 可选：添加到PATH
ln -s $(pwd)/arxml-tool.py ~/.local/bin/arxml-tool
```

## 快速开始

```bash
# 解析ARXML文件
./arxml-tool.py parse examples/example.arxml

# 生成JSON格式报告
./arxml-tool.py parse examples/example.arxml --format json -o output.json

# 分析完整性
./arxml-tool.py analyze examples/example.arxml --strict

# 生成配置文件
./arxml-tool.py generate examples/example.arxml -o ./generated --module Can

# 验证Schema
./arxml-tool.py validate examples/example.arxml --schema schema/autosar.xsd
```

## 命令参考

### parse - ARXML解析

解析ARXML文件并输出组件、接口、数据类型等信息。

```bash
arxml-tool.py parse <file> [options]

选项:
  -o, --output          输出文件路径
  -f, --format          输出格式: json, md, markdown, console
  -s, --schema          XSD schema文件路径（用于验证）
  -q, --query           执行查询 (格式: type:value)

查询示例:
  # 查找特定组件
  arxml-tool.py parse system.arxml -q swc:EngineControl
  
  # 查找使用某接口的组件
  arxml-tool.py parse system.arxml -q interface:VehicleSpeed
  
  # 查找连接的组件
  arxml-tool.py parse system.arxml -q connected:EngineControl
```

### generate - 配置生成

从ARXML或JSON生成AUTOSAR BSW配置文件。

```bash
arxml-tool.py generate <file> [options]

选项:
  -o, --output          输出目录路径 (必需)
  -f, --format          输入文件格式: arxml, json, auto
  -m, --module          指定模块名 (仅用于ARXML)
  -t, --templates       自定义模板目录

示例:
  # 从ARXML生成
  arxml-tool.py generate system.arxml -o ./generated -m Can
  
  # 从JSON生成
  arxml-tool.py generate config.json -o ./generated --format json
  
  # 使用自定义模板
  arxml-tool.py generate system.arxml -o ./generated -t ./templates
```

生成的文件包括:
- `{Module}_Cfg.h` - 配置头文件
- `{Module}_Cfg.c` - 配置源文件
- `{Module}_Lcfg.c` - 链接时配置
- `{Module}_Config.arxml` - ECUC ARXML配置

### analyze - 完整性分析

检查ARXML文件的结构和语义完整性。

```bash
arxml-tool.py analyze <file> [options]

选项:
  -o, --output          输出报告文件路径
  -f, --format          报告格式: json, md, markdown, console
  --strict              启用严格模式
  --disable-rules       禁用的规则ID列表，逗号分隔
  --fail-on-error       发现错误时返回非零退出码

示例:
  # 基本分析
  arxml-tool.py analyze system.arxml
  
  # 严格模式分析
  arxml-tool.py analyze system.arxml --strict
  
  # 输出Markdown报告
  arxml-tool.py analyze system.arxml --format md -o report.md
  
  # 禁用特定规则
  arxml-tool.py analyze system.arxml --disable-rules RULE-005,RULE-006
```

**默认检查规则:**

| 规则ID | 名称 | 类型 | 严重级别 |
|--------|------|------|---------|
| RULE-001 | 必需元素检查 | 结构性 | ERROR |
| RULE-002 | UUID唯一性检查 | 结构性 | ERROR |
| RULE-003 | 引用有效性检查 | 结构性 | ERROR |
| RULE-004 | 数据类型匹配检查 | 结构性 | WARNING |
| RULE-005 | ECU映射完整性 | 语义性 | ERROR |
| RULE-006 | 组件连接完整性 | 语义性 | WARNING |
| RULE-007 | 接口兼容性 | 语义性 | ERROR |
| RULE-008 | UUID格式检查* | 结构性 | WARNING |
| RULE-009 | 命名规范检查* | 结构性 | INFO |
| RULE-010 | 未使用元素检查* | 语义性 | INFO |

*仅在严格模式下启用

### validate - Schema验证

验证ARXML文件符合XSD Schema。

```bash
arxml-tool.py validate <file> [options]

选项:
  -s, --schema          XSD schema文件路径
  -o, --output          输出文件路径
  -f, --format          输出格式: json, md, markdown, console
  --check-wellformed    仅检查XML良构性

示例:
  # 完整Schema验证
  arxml-tool.py validate system.arxml --schema autosar.xsd
  
  # 仅检查良构性
  arxml-tool.py validate system.arxml --check-wellformed
  
  # 输出JSON格式结果
  arxml-tool.py validate system.arxml -s autosar.xsd -f json -o result.json
```

## 环境变量

| 变量名 | 说明 | 示例 |
|--------|------|------|
| `ARXML_TOOL_CONFIG` | 默认配置文件路径 | `/path/to/config.json` |
| `ARXML_TOOL_SCHEMA` | 默认Schema文件路径 | `/path/to/autosar.xsd` |

## 配置文件

可以通过JSON配置文件设置默认选项:

```json
{
  "default_output_format": "json",
  "default_schema": "/path/to/autosar.xsd",
  "analyze": {
    "strict_mode": false,
    "disabled_rules": []
  },
  "generate": {
    "templates_dir": "/path/to/templates"
  }
}
```

使用配置文件:
```bash
arxml-tool.py -c config.json parse system.arxml
```

## 项目结构

```
arxml-tool/
├── arxml-tool.py          # 主入口脚本
├── requirements.txt       # Python依赖
├── README.md              # 这个文件
├── src/
│   ├── arxml_parser.py      # ARXML解析器
│   ├── config_generator.py  # 配置生成器
│   ├── integrity_analyzer.py # 完整性分析器
│   ├── usage_example.py     # 使用示例
│   └── requirements.txt     # 原始依赖文件
├── tests/
│   └── test_arxml_parser.py # 单元测试
└── examples/
    └── example_usage.py     # 使用示例脚本
```

## 开发

```bash
# 运行测试
python -m pytest tests/

# 代码格式化
black arxml-tool.py src/

# 类型检查
mypy arxml-tool.py src/
```

## 许可证

MIT License

## 作者

YuleTech AutoSAR Team
