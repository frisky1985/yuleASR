# CAN配置工具 - GUI版

一个功能完整的GUI工具，用于从DBC、CSV、Excel文件生成AUTOSAR Com模块配置。

## 功能特性

### 核心功能
- 📁 **多格式支持** - 支持DBC、CSV、Excel(.xlsx)文件导入
- 📊 **可视化展示** - IPDU和信号列表可视化
- ✏️ **在线编辑** - 支持编辑配置参数
- 🚀 **自动生成** - 一键生成Com_Cfg.h和Com_Cfg.c
- 💾 **文件下载** - 支持下载生成的配置文件

### 双模式支持
1. **Web版** - 基于Flask的浏览器应用
2. **桌面版** - 基于PyQt6的本地应用

## 安装

### 安装依赖
```bash
cd tools/can-config-tool
pip install -r requirements.txt
```

### 安装PyQt6 (仅桌面版需要)
```bash
pip install PyQt6
```

## 使用方法

### Web版 (推荐)

1. **启动服务**
```bash
python launch_web.py
```

2. **打开浏览器**
访问 http://localhost:5000

3. **使用步骤**
   - 点击"文件导入"区域或拖拽文件
   - 选择DBC、CSV或Excel文件
   - 查看解析结果
   - 在选项卡中编辑IPDU和信号
   - 点击"生成配置文件"
   - 下载Com_Cfg.h和Com_Cfg.c

### 桌面版

1. **启动应用**
```bash
python launch_desktop.py
```

2. **使用步骤**
   - 拖拽文件到左侧区域，或点击"浏览文件"
   - 在IPDU和信号选项卡中查看和编辑
   - 点击"生成配置文件"
   - 在预览选项卡中查看代码
   - 点击"下载"保存文件

## 支持的文件格式

### DBC文件
- Vector DBC 标准格式
- 支持消息、信号、位定义

### CSV文件格式
需要包含以下列：
```csv
MessageName,MessageID,DLC,Direction,CycleTime,SignalName,StartBit,BitLength,Factor,Offset,MinValue,MaxValue,Unit,InitValue
EngineData,0x123,8,Tx,100,RPM,0,16,1,0,0,8000,rpm,0
EngineData,0x123,8,Tx,100,Temperature,16,8,0.5,-40,-40,215,C,0
```

### Excel文件格式
与CSV相同的列结构，支持.xlsx格式

## 项目结构

```
can-config-tool/
├── src/                      # 核心库
│   ├── dbc_parser.py          # DBC解析器
│   ├── can_matrix_parser.py   # CSV/Excel解析器
│   └── com_config_generator.py # 配置生成器
├── gui/                      # Web版GUI
│   ├── app.py                 # Flask应用
│   ├── templates/
│   │   └── index.html         # 主页面
│   └── static/
│       └── js/
│           └── app.js         # 前端逻辑
├── gui_desktop/              # 桌面版GUI
│   └── main.py               # PyQt6应用
├── tests/                    # 测试用例
├── examples/                 # 示例文件
├── launch_web.py           # Web版启动脚本
├── launch_desktop.py       # 桌面版启动脚本
├── requirements.txt        # 依赖列表
└── README.md               # 说明文档
```

## 界面预览

### Web版
```
+--------------------------------------------------+
|  🚀 CAN配置工具                                    |
+-------------+------------------------------------+
| 📁 文件导入  |  📊 统计: IPDU: 12 | 信号: 45       |
| [拖拽区域]  +------------------------------------+
|             |  [IPDU列表] [信号列表] [代码预览]    |
| ⚙️ 配置     |                                    |
| ECU: ECU0   |  名称    | CAN ID | 方向 | 操作    |
| 文件: *.dbc |  ------ | ------ | ---- | ------ |
|             |  EngData| 0x0123 | Tx   | [编辑] |
| 🔧 操作     |  VehData| 0x0456 | Rx   | [编辑] |
| [生成]      |                                    |
| [重置]      |                                    |
+-------------+------------------------------------+
```

### 桌面版
- 模拔窗体设计
- 可拖拽文件
- 双击编辑
- 快捷键支持

## 开发

### 运行测试
```bash
cd tools/can-config-tool
pytest tests/ -v
```

### 格式化代码
```bash
black src/ gui/ gui_desktop/
```

## 版本历史

- **v1.0.0** - 初始版本
  - Web版GUI支持
  - 桌面版GUI支持
  - DBC/CSV/Excel解析
  - 配置文件生成

## 许可

MIT License - 归属 yuleASR 项目
