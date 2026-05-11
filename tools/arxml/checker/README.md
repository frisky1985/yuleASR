# ARXML Integrity Checker

ARXML文件完整性分析器 - 用于验证AUTOSAR XML文件的完整性和一致性

## 功能特性

1. **XML语法检查**
   - 验证XML结构正确性
   - 检测未闭合标签
   - 定位语法错误位置

2. **必需元素检查**
   - 验证根元素必须是`AUTOSAR`
   - 检查SHORT-NAME不为空
   - 验证必需结构元素存在

3. **引用关系验证**
   - 检查UUID引用是否存在
   - 验证数据类型引用
   - 检测空引用

4. **数据类型定义检查**
   - 验证Application Data Types
   - 检查Implementation Data Types
   - 确认Compu Methods定义

5. **配置参数范围检查**
   - LENGTH (0-65535)
   - SIZE (0-4294967295)
   - PRIORITY (0-255)
   - PERIOD/TIMEOUT范围验证

6. **UUID唯一性检查**
   - 检测重复UUID
   - 确保全局唯一性

## 安装

```bash
# 不需要额外依赖，使用Python标准库
python3 integrity_checker.py --help
```

## 使用方法

### 检查单个文件
```bash
python3 integrity_checker.py file.arxml
```

### 检查多个文件
```bash
python3 integrity_checker.py file1.arxml file2.arxml
```

### 递归检查目录
```bash
python3 integrity_checker.py -r ./arxml_directory/
```

### 输出报告到文件
```bash
python3 integrity_checker.py file.arxml -o report.txt
```

### 仅显示摘要
```bash
python3 integrity_checker.py -s file.arxml
```

## 报告格式

```
============================================================
ARXML Integrity Check Report
============================================================
File: example.arxml
Errors: X
Warnings: Y
Infos: Z
------------------------------------------------------------

[ERRORS]
[ERROR] Line 45: Missing required element [Element: SHORT-NAME]
    Suggestion: Provide a valid short name for the element

[WARNINGS]
[WARNING] Line 23: Potentially undefined data type reference [Element: TYPE-TREF]
    Suggestion: Ensure the data type is defined in this or imported ARXML

[INFOS]
[INFO] Line 1: XML syntax validation passed
------------------------------------------------------------
Check completed: PASSED
```

## 错误级别

- **ERROR**: 严重问题，必须修复
- **WARNING**: 潜在问题，建议修复
- **INFO**: 信息性消息

## 示例

### 测试文件

目录中包含以下测试文件:
- `test_sample.arxml` - 含有重复UUID的测试样本
- `test_invalid.arxml` - 含有空SHORT-NAME的无效样本

### 运行测试
```bash
python3 integrity_checker.py test_sample.arxml test_invalid.arxml
```

## 返回码

- `0`: 检查通过(无错误)
- `1`: 检查失败(有错误)

## 技术详情

- 基于Python 3标准库
- 支持AUTOSAR 4.0标准
- 兼容多种命名空间格式
