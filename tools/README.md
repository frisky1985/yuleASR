# DLT Configuration Tool

AutoSAR DLT (Diagnostic and Logging Trace) 配置工具

## 安装依赖

```bash
pip install -r requirements.txt
```

## 用法

### 1. 生成默认配置

```bash
python dlt_config_tool.py generate --output dlt_config.json
```

生成的文件:
- `dlt_config.json` - JSON格式配置
- `dlt_config.h` - C头文件（用于编译）
- `dlt_viewer_config.json` - DLT Viewer配置（如果指定--viewer-config）

### 2. 修改配置

```bash
# 修改日志级别
python dlt_config_tool.py modify --input dlt_config.json --level DEBUG

# 修改ECU ID
python dlt_config_tool.py modify --input dlt_config.json --ecu MY_ECU

# 修改缓冲区大小
python dlt_config_tool.py modify --input dlt_config.json --buffer-size 131072
```

### 3. 验证配置

```bash
python dlt_config_tool.py validate --input dlt_config.json
```

### 4. 添加新上下文

```bash
python dlt_config_tool.py add-context \
    --input dlt_config.json \
    --app-id TEST \
    --context-id DEMO \
    --description "Test context" \
    --level DEBUG
```

## 配置参数说明

### 全局参数

| 参数 | 说明 | 可选值 |
|-----|-----|-------|
| version | 配置版本 | 字符串 |
| ecu_id | ECU标识 | 4字符串 |
| default_log_level | 默认日志级别 | OFF, FATAL, ERROR, WARN, INFO, DEBUG, VERBOSE |
| enable_timestamp | 启用时间戳 | true/false |
| enable_ecu_id | 启用ECU ID | true/false |
| enable_session_id | 启用Session ID | true/false |

### 输出配置

| 参数 | 说明 | 默认值 |
|-----|-----|-------|
| enable_udp | 启用UDP输出 | true |
| udp_port | UDP目标端口 | 3490 |
| udp_address | UDP组播地址 | 239.255.42.99 |
| enable_file | 启用文件输出 | true |
| file_path | 日志文件路径 | /tmp/dlt.log |
| file_max_size | 单文件最大大小 | 10MB |

### 缓冲区配置

| 参数 | 说明 | 默认值 |
|-----|-----|-------|
| buffer_size | 缓冲区大小 | 64KB |
| overflow_strategy | 溢出策略 | DROP_OLD (丢弃旧日志) |
| flush_interval_ms | 刷新间隔 | 100ms |

## 与DLT Viewer配合使用

1. 生成配置时指定 `--viewer-config` 参数
2. 在PC上启动DLT Viewer（可从Vector官网下载）
3. 加载生成的 `dlt_viewer_config.json`
4. 配置网络连接参数

## 示例工作流

```bash
# 1. 初始化项目配置
cd config
python ../tools/dlt_config_tool.py generate --output dlt_config.json --viewer-config

# 2. 为调试模式设置更详细的日志级别
python ../tools/dlt_config_tool.py modify --input dlt_config.json --level DEBUG

# 3. 添加特定模块的上下文
python ../tools/dlt_config_tool.py add-context \
    --input dlt_config.json \
    --app-id MYNC \
    --context-id FUNC \
    --description "My New Component" \
    --level VERBOSE

# 4. 验证配置
python ../tools/dlt_config_tool.py validate --input dlt_config.json

# 5. 生成的C头文件可用于编译
```
