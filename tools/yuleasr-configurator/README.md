# yuleASR-Configurator

AUTOSAR BSW 配置管理工具，支持 YAML/JSON 配置、JSON Schema 验证、代码生成、Web GUI 和 ARXML 导入导出。

## 功能特性

### Phase 1: 核心框架 ✓
- **配置加载器**: YAML/JSON 配置加载、合并、`$include` 指令支持
- **Schema 引擎**: JSON Schema Draft-7 验证，per-module 验证
- **模板引擎**: Jinja2 模板 + AUTOSAR 专用过滤器 (c_bool, c_uint, c_hex, pad*)
- **模板迁移**: 92 个 Jinja2 模板按 AUTOSAR 层级组织
- **引用解析器**: 跨模块依赖解析，拓扑排序初始化顺序
- **Golden 测试**: 100% 向后兼容验证

### Phase 2: Schema + YAML ✓
- **JSON Schema**: 92 个 per-module JSON Schema 文件
- **YAML-first**: YAML 配置格式，去重 111→92 模块
- **迁移工具**: JSON→YAML 自动转换脚本

### Phase 3: Web GUI + ARXML ✓
- **FastAPI 后端**: REST API + WebSocket 实时更新
- **Vue 3 前端**: Schema-driven 表单、Monaco 编辑器、D3 依赖图
- **ARXML 支持**: AUTOSAR 标准 ARXML 格式导入/导出，100% 往返测试通过

## 快速开始

### 安装

```bash
# 安装 Python 包
cd tools/yuleasr-configurator
pip install -e .

# 安装前端依赖
cd web
npm install
```

### 命令行使用

```bash
# 验证配置
yuleasr validate config/bsw_config.yaml

# 生成代码
yuleasr generate config/bsw_config.yaml --output config/generated

# 列出模块
yuleasr list-modules
```

### Web GUI

```bash
# 启动后端 API (端口 8000)
cd tools/yuleasr-configurator
python -m yuleasr.web.api.main

# 启动前端开发服务器 (端口 3000)
cd tools/yuleasr-configurator/web
npm run dev

# 访问 http://localhost:3000
```

### ARXML 导入导出

```bash
# 导出到 ARXML
curl -X POST http://localhost:8000/api/arxml/export-file \
  -H "Content-Type: application/json" \
  -d '{}' \
  --output config/bsw_config.arxml

# 从 ARXML 导入
curl -X POST http://localhost:8000/api/arxml/import-file \
  -H "Content-Type: application/json" \
  -d '{"file_path": "config/bsw_config.arxml"}'
```

## 项目结构

```
tools/yuleasr-configurator/
├── yuleasr/
│   ├── core/
│   │   ├── config_loader.py      # 配置加载器
│   │   ├── schema_engine.py      # Schema 验证引擎
│   │   ├── template_engine.py    # Jinja2 模板引擎
│   │   ├── resolver.py           # 跨模块引用解析
│   │   ├── code_generator.py     # 代码生成器
│   │   ├── golden_test.py        # Golden 测试框架
│   │   └── arxml_handler.py      # ARXML 解析/导出
│   ├── cli/
│   │   └── main.py               # Typer CLI
│   └── web/
│       └── api/
│           └── main.py           # FastAPI 后端
├── templates/                    # 92 个 Jinja2 模板
│   ├── mcal/
│   ├── ecual/
│   ├── services/
│   ├── os/
│   └── integration/
├── schemas/                      # 92 个 JSON Schema
│   ├── mcal/
│   ├── ecual/
│   ├── services/
│   ├── os/
│   └── integration/
├── web/                          # Vue 3 前端
│   ├── src/
│   │   ├── api/                  # API 客户端
│   │   ├── components/           # Schema 表单组件
│   │   ├── views/                # 页面视图
│   │   ├── stores/               # Pinia 状态管理
│   │   └── types/                # TypeScript 类型
│   └── package.json
├── scripts/                      # 工具脚本
│   ├── generate_schemas.py
│   ├── convert_to_yaml.py
│   ├── migrate_templates.py
│   ├── test_arxml.py
│   └── test_arxml_api.py
└── pyproject.toml
```

## API 端点

### 配置管理
- `GET /api/config` - 加载配置
- `POST /api/config` - 保存配置
- `POST /api/config/validate` - 验证配置

### Schema 管理
- `GET /api/schemas` - 获取所有 Schema
- `GET /api/schemas/{module}` - 获取模块 Schema

### 模块管理
- `GET /api/modules` - 获取模块列表
- `GET /api/init-order` - 获取初始化顺序

### 代码生成
- `POST /api/generate` - 生成代码

### ARXML
- `POST /api/arxml/import` - 从 ARXML 字符串导入
- `POST /api/arxml/import-file` - 从 ARXML 文件导入
- `POST /api/arxml/export` - 导出到 ARXML 字符串
- `POST /api/arxml/export-file` - 导出到 ARXML 文件

### WebSocket
- `WS /ws` - 实时更新通道

## 测试

```bash
# 运行 ARXML 单元测试
python tools/yuleasr-configurator/scripts/test_arxml.py

# 运行 ARXML API 测试 (需要先启动 API 服务器)
python tools/yuleasr-configurator/scripts/test_arxml_api.py

# 运行前端构建测试
cd tools/yuleasr-configurator/web
npm run build
```

## 技术栈

### 后端
- Python 3.10+
- PyYAML - YAML 解析
- Jinja2 - 模板引擎
- jsonschema - JSON Schema 验证
- Typer - CLI 框架
- FastAPI - Web API
- uvicorn - ASGI 服务器

### 前端
- Vue 3.4 + TypeScript 5.3
- Element Plus 2.6 - UI 组件
- Pinia 2.1 - 状态管理
- Monaco Editor - 代码编辑器
- D3.js 7.8 - 依赖图可视化
- Vite 5.1 - 构建工具

## 统计

- **模块总数**: 92 个 AUTOSAR BSW 模块
- **模板数量**: 92 个 Jinja2 模板
- **Schema 数量**: 92 个 JSON Schema
- **AUTOSAR 层级**: MCAL (20), ECUAL (32), Services (36), OS (1), Integration (3)
- **ARXML 往返测试**: 100% 模块匹配

## 下一步

- 集成测试自动化
- 性能优化 (大配置文件)
- 用户文档完善
- 部署脚本 (Docker)
- ARXML 3D 可视化增强

## 许可证

MIT License

## 联系

- 公司: 上海予乐电子科技有限公司
- 项目: YuleTech AutoSAR BSW Platform
- 版本: v1.0.0
