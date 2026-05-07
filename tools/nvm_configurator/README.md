# NvM Configuration Tool

## 概述

专为 AUTOSAR NvM (NVRAM Manager) 模块设计的配置工具，支持 JSON/Excel 配置文件导入，自动生成 AUTOSAR 标准的 C 代码文件。

## 功能特性

### 支持的配置参数

#### 通用配置 (General)
- 错误检测 (DevErrorDetect)
- 版本信息 API
- RAM 块状态设置
- 写保护/读保护 API
- 重试次数配置
- 主函数周期
- 任务队列大小

#### 块描述符 (Block Descriptors)
- 块 ID 和名称
- 管理类型 (Native/Redundant/Dataset)
- 数据长度和数量
- CRC 类型 (CRC8/16/32/None)
- 写保护/写一次标志
- 镜像/压缩支持
- ROM 块数据
- 回调函数

## 安装

```bash
# 安装依赖
pip install openpyxl

# 可选: Tkinter GUI 支持
pip install tkinter
```

## 使用方法

### 1. 使用示例配置生成

```bash
# 生成示例 JSON 配置文件
python nvm_config_generator.py --sample --format json -o ./output

# 生成示例 Excel 配置文件
python nvm_config_generator.py --sample --format excel -o ./output

# 直接生成 C 代码
python nvm_config_generator.py --sample -o ./output
```

### 2. 使用 JSON 配置

```bash
python nvm_config_generator.py -i nvm_config.json -o ./output
```

### 3. 使用 Excel 配置

Excel 文件结构:
- **General** 工作表: 通用配置参数
- **Blocks** 工作表: 块描述符配置

```bash
python nvm_config_generator.py -i nvm_config.xlsx -o ./output
```

## 输出文件

工具生成以下文件:

### NvM_Cfg.h
- 预编译配置宏
- 块 ID 定义
- 块大小定义
- 重试和 CRC 配置
- 任务队列大小

### NvM_Lcfg.c
- 块描述符表
- ROM 块数据
- 全局配置结构

## 配置示例

### 示例 1: 基本配置
```json
{
  "blocks": [
    {
      "BlockId": 1,
      "BlockName": "Config",
      "ManagementType": "NVM_BLOCK_NATIVE",
      "NvBlockLength": 64,
      "CrcType": "NVM_CRC_16"
    }
  ]
}
```

### 示例 2: 写一次块
```json
{
  "blocks": [
    {
      "BlockId": 4,
      "BlockName": "VIN",
      "BlockWriteOnce": true,
      "BlockWriteProt": true,
      "CrcType": "NVM_CRC_8"
    }
  ]
}
```

### 示例 3: 多数据集块
```json
{
  "blocks": [
    {
      "BlockId": 2,
      "BlockName": "Calibration",
      "ManagementType": "NVM_BLOCK_DATASET",
      "NumberOfNvBlocks": 4,
      "NumberOfDataSets": 4,
      "NvBlockLength": 256
    }
  ]
}
```

### 示例 4: 冗余块
```json
{
  "blocks": [
    {
      "BlockId": 3,
      "BlockName": "FaultMemory",
      "ManagementType": "NVM_BLOCK_REDUNDANT",
      "NumberOfNvBlocks": 2,
      "CrcType": "NVM_CRC_32"
    }
  ]
}
```

## GUI 设计规划

### 界面布局

```
+----------------------------------------------------------+
|  NvM Configuration Tool                        [File][Help]|
+----------------------------------------------------------+
|  📁 Project: MyProject                    [导入][导出][生成]|
+----------------------------------------------------------+
|  +------------------+  +--------------------------------+|
|  | 📋 Block List   |  | ⚙️ Block Configuration         ||
|  |                  |  |                                 ||
|  | ▶ Config        |  | Block ID:        [1        ]    ||
|  |   Calibration    |  | Block Name:      [Config   ]    ||
|  |   FaultMemory    |  |                                 ||
|  |   VIN            |  | Management:      [Native ▼]    ||
|  |   Odometer       |  | Block Length:    [64       ]    ||
|  |                  |  | Nv Blocks:       [1        ]    ||
|  | [+ Add Block]    |  | Datasets:        [1        ]    ||
|  |                  |  |                                 ||
|  +------------------+  | 🔐 CRC & Protection              ||
|                        |                                 ||
|  +------------------+  | [✓] Use CRC                     ||
|  | ⚙️ General      |  | CRC Type:        [CRC16 ▼]     ||
|  |                  |  |                                 ||
|  | API Settings     |  | [✓] Write Protection            ||
|  | Queue Settings   |  | [✓] Write Once                  ||
|  | CRC Settings     |  | [✓] Auto Validation             ||
|  |                  |  | [✓] Use Mirror                  ||
|  +------------------+  |                                 ||
|                        +--------------------------------+|
+----------------------------------------------------------+
|  ✅ Ready | 5 blocks | Generated: NvM_Cfg.h, NvM_Lcfg.c   |
+----------------------------------------------------------+
```

### 功能模块

#### 1. 块列表 (Block List)
- 显示所有配置的块
- 支持拖拽排序
- 双击编辑
- 右键菜单（复制/粘贴/删除）
- 颜色标记类型（绿色=Native，蓝色=Redundant，黄色=Dataset）

#### 2. 块配置面板 (Block Configuration)
- 基本参数: ID、名称、管理类型
- 大小参数: 长度、数量、数据集
- CRC配置: 启用、类型
- 保护配置: 写保护、写一次、自动验证
- 高级选项: 镜像、压缩、回调

#### 3. 通用配置面板 (General Settings)
- API 使能/禁用
- 重试次数设置
- 主函数周期
- 任务队列大小

#### 4. 实时预览 (Live Preview)
- 实时显示生成的代码片段
- 内存使用估计
- 配置验证状态

## Excel 配置模板

### General 工作表
| Parameter | Value | Description |
|-----------|-------|-------------|
| DevErrorDetect | STD_ON | 开发错误检测 |
| MaxWriteRetries | 3 | 最大写重试次数 |
| MainFunctionPeriod | 10 | 主函数周期(ms) |
| ... | ... | ... |

### Blocks 工作表
| BlockId | BlockName | ManagementType | NvBlockLength | CrcType | BlockUseCrc | BlockWriteOnce | Description |
|---------|-----------|----------------|---------------|---------|-------------|----------------|-------------|
| 1 | Config | NVM_BLOCK_NATIVE | 64 | NVM_CRC_16 | TRUE | FALSE | 系统配置 |
| 2 | Calibration | NVM_BLOCK_DATASET | 256 | NVM_CRC_16 | TRUE | FALSE | 标定数据 |
| ... | ... | ... | ... | ... | ... | ... | ... |

## 工具完数检查

工具自动检查以下配置问题:
- 块 ID 重复
- 名称重复
- 块大小超出限制
- 数据集数量不一致
- ROM 块配置错误

## 常见问题

### Q: 为什么生成的文件缺少 Std_Types.h?
A: Std_Types.h 是 AUTOSAR 基础文件，需要从 AUTOSAR 软件栈获取。

### Q: 如何添加自定义回调函数?
A: 在 JSON/Excel 中设置 InitCallback 和 JobEndCallback 字段。

### Q: 支持哪些管理类型?
A: Native（单块）、Redundant（冗余）、Dataset（多数据集）。

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-28 | 初始版本 - 支持 JSON/Excel 输入 |
| 1.1.0 | 2026-04-28 | 添加完数检查和错误提示 |

## 许可证

MIT License - yuleASR Team
