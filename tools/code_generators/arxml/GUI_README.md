# yuleASR ARXML Generator - GUI 使用说明

本工具提供两种可视化界面：**Web版** 和 **桌面版**

---

## 方式一: Web版 GUI (推荐)

基于 Flask 的浏览器界面，功能最完善。

### 安装依赖

```bash
cd tools/arxml-generator
pip3 install flask flask-cors
```

### 启动Web服务器

```bash
# 方式1: 使用启动脚本
python3 gui_launcher.py

# 方式2: 直接运行
python3 gui/api/server.py
```

### 访问界面

打开浏览器访问: **http://localhost:5000**

### Web界面特点

- ✅ 三栏布局 (模块选择 | 配置编辑 | ARXML预览)
- ✅ 支持所有MCAL和BSW模块的可视化配置
- ✅ 列表型配置支持 (引脚、信号、NVRAM块等)
- ✅ 实时XML语法高亮
- ✅ 一键复制/下载

---

## 方式二: 桌面版 GUI

基于 PyQt6 的本地应用，无需浏览器。

### 安装依赖

```bash
pip3 install PyQt6
```

### 启动桌面应用

```bash
python3 gui_qt.py
```

### 桌面版特点

- ✅ 原生桌面应用体验
- ✅ 响应更快
- ✅ 无需浏览器
- ⚠️ 部分模块简化实现

---

## 界面截图

```
├────────────────────────────────────────────────────────────────────────┐
│  📤 模块选择    │  ⚙️ 配置编辑              │  📄 ARXML预览        │
│                 │                          │                      │
│  MCAL           │  ECU名称: [ECU0    ]     │  ╔══════════════════════════════╗  │
│  🔫 Mcu        │                          │  ║ <?xml version... ║  │
│  🔗 Port       │  ┌───────────────────┐            │  ║ <AUTOSAR>      ║  │
│  📡 CAN        │  │ 通用配置    │            │  ║   <ECUC>       ║  │
│  ...            │  │                  │            │  ║     ...        ║  │
│                 │  │ ☑ DevError检测  │            │  ╚═══════════════════════════════╝  │
│  BSW            │  │ ☑ 初始化时钟   │            │                      │
│  💬 COM        │  └───────────────────┘            │  [  复制  ] [ 下载 ]  │
│  ...            │                          │                      │
│                 │  ┌───────────────────┐            │                      │
│                 │  │ 时钟配置    │            │                      │
│                 │  │                  │            │                      │
│                 │  │ CPU: [80000000]Hz│            │                      │
│                 │  │ PER: [40000000]Hz│            │                      │
│                 │  └───────────────────┘            │                      │
│                 │                          │                      │
│                 │  [   生成ARXML    ]         │                      │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 生成MCU配置

1. 点击左侧 "🔫 Mcu" 按钮
2. 设置ECU名称 (如: ECU0)
3. 配置时钟参数 (默认80MHz)
4. 点击 "生成ARXML"
5. 在右侧预览区查看结果
6. 点击 "下载" 保存文件

### 生成Port配置

1. 选择 "🔗 Port" 模块
2. 配置引脚名称、方向、模式
3. 可添加多个引脚配置
4. 生成ARXML

---

## 故障排除

### 问题1: 无法启动Web服务器

**解决方案:**
```bash
# 检查端口是否被占用
sudo lsof -i :5000

# 使用其他端口
python3 -c "from gui.api.server import app; app.run(port=8080)"
```

### 问题2: 模块加载失败

**解决方案:**
```bash
# 确保从正确目录运行
cd tools/arxml-generator
python3 gui_launcher.py
```

### 问题3: PyQt6安装失败

**解决方案:**
```bash
# Ubuntu/Debian
sudo apt-get install python3-pyqt6

# 或使用conda
conda install pyqt
```

---

## 技术架构

### Web版

```
Frontend (HTML/JS/Tailwind)
    ↓ AJAX
Backend (Flask/Python)
    ↓ 调用
ARXML Generator Core
    ↓ 生成
ECUC配置模型 → ARXML文件
```

### 桌面版

```
PyQt6 GUI
    ↓ 直接调用
ARXML Generator Core
    ↓ 生成
ECUC配置模型 → ARXML文件
```

---

## 开发计划

### 已完成 ✅

- [x] Web版三栏布局界面
- [x] 模块选择面板
- [x] 参数配置表单
- [x] ARXML实时预览
- [x] 一键复制/下载
- [x] 桌面版PyQt6实现

### 计划中 📝

- [ ] 支持导入现有ARXML
- [ ] 支持配置模板保存/加载
- [ ] 多语言支持
- [ ] 黑色/白色主题切换

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-05-09 | 初始版本, 支持Web和桌面双界面 |

---

*开发团队: yuleASR Team*
