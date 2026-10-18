# yuleASR Configurator Web GUI

Vue 3 + TypeScript + Element Plus 前端应用，用于可视化配置 AUTOSAR BSW 模块。

## 功能特性

- **Schema-driven 表单**: 基于 JSON Schema 自动生成模块配置表单
- **实时验证**: 通过 WebSocket 实时验证配置
- **模块树形导航**: 按 AUTOSAR 层级浏览 92 个 BSW 模块
- **YAML 编辑器**: Monaco Editor 支持的配置编辑器
- **依赖图可视化**: D3.js 力导向图展示模块依赖关系
- **代码生成**: 一键生成 C 代码配置文件

## 技术栈

- Vue 3.4 + TypeScript 5.3
- Element Plus 2.6 (UI 组件库)
- Pinia 2.1 (状态管理)
- Vue Router 4.3 (路由)
- Monaco Editor (代码编辑器)
- D3.js 7.8 (依赖图可视化)
- Axios (HTTP 客户端)
- Vite 5.1 (构建工具)

## 开发

```bash
# 安装依赖
npm install

# 启动开发服务器 (端口 3000)
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview
```

## 项目结构

```
web/
├── src/
│   ├── api/
│   │   └── client.ts          # API 客户端
│   ├── components/
│   │   ├── SchemaForm.vue     # Schema-driven 表单
│   │   └── SchemaField.vue    # 递归字段渲染
│   ├── stores/
│   │   └── config.ts          # Pinia 配置 store
│   ├── types/
│   │   └── index.ts           # TypeScript 类型定义
│   ├── views/
│   │   ├── HomeView.vue       # 首页仪表盘
│   │   ├── ConfigView.vue     # 配置编辑器
│   │   ├── ModulesView.vue    # 模块列表
│   │   ├── ModuleDetailView.vue # 模块详情 (Schema 表单)
│   │   ├── EditorView.vue     # YAML 编辑器
│   │   └── GraphView.vue      # 依赖图
│   ├── App.vue                # 根组件
│   ├── main.ts                # 入口
│   └── router.ts              # 路由配置
├── index.html
├── package.json
├── tsconfig.json
└── vite.config.ts
```

## API 端点

前端通过 Vite proxy 连接后端 API (默认 `http://localhost:8000`):

- `GET /api/config` - 加载配置
- `POST /api/config` - 保存配置
- `POST /api/config/validate` - 验证配置
- `GET /api/schemas` - 获取所有 Schema
- `GET /api/schemas/:module` - 获取模块 Schema
- `GET /api/modules` - 获取模块列表
- `GET /api/init-order` - 获取初始化顺序
- `POST /api/generate` - 生成代码
- `WS /ws` - WebSocket 实时更新

## 使用流程

1. 启动后端 API:
   ```bash
   cd tools/yuleasr-configurator
   python -m yuleasr.web.api.main
   ```

2. 启动前端开发服务器:
   ```bash
   cd tools/yuleasr-configurator/web
   npm run dev
   ```

3. 访问 `http://localhost:3000`

## 页面说明

### Home (首页)
- 配置状态概览
- 模块统计 (按 AUTOSAR 层级)
- 快速操作入口
- 验证结果显示

### Config (配置编辑)
- 模块列表表格
- 启用/禁用模块
- 项目信息编辑
- 验证和保存

### Modules (模块浏览)
- 按层级分组显示
- 搜索过滤
- 点击进入模块详情

### Module Detail (模块详情)
- Schema-driven 表单
- 自动生成字段
- 依赖关系显示
- 重置默认值

### Editor (YAML 编辑器)
- Monaco Editor
- YAML 语法高亮
- 实时验证
- 格式化

### Graph (依赖图)
- D3 力导向图
- 缩放和拖拽
- 层级颜色编码
- 交互式节点

## 构建输出

生产构建输出到 `dist/` 目录，可直接部署到静态文件服务器。

## 下一步

- ARXML 导入/导出 (Phase 3 W4-W5)
- 集成测试 (Phase 3 W6)
- 用户文档
