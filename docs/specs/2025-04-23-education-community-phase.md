# YuleTech Phase 1-2: 教育内容 + 社区平台 实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 构建完整的学习资源和社区基础设施，降低用户使用门槛，建立技术社区

**Architecture:** 使用 Docusaurus 静态网站托管教程文档，Discourse 论坛作为社区交流平台，两者都通过 GitHub Pages 免费托管

**Tech Stack:** Node.js, Docusaurus, Docker, GitHub Actions, Discourse (self-hosted)

---

## 阶段一：教育内容基础设施 (Week 1-2)

### Task 1: 初始化 Docusaurus 文档站

**Files:**
- Create: `website/package.json`
- Create: `website/docusaurus.config.js`
- Create: `website/sidebars.js`
- Create: `website/src/pages/index.js`

**Step 1: 创建项目目录结构**

```bash
cd ~/yuleASR
mkdir -p website/docs website/blog website/src/pages website/static/img
```

**Step 2: 初始化 package.json**

```json
{
  "name": "yuleasr-docs",
  "version": "1.0.0",
  "private": true,
  "scripts": {
    "start": "docusaurus start",
    "build": "docusaurus build",
    "swizzle": "docusaurus swizzle",
    "deploy": "docusaurus deploy",
    "clear": "docusaurus clear",
    "serve": "docusaurus serve"
  },
  "dependencies": {
    "@docusaurus/core": "3.7.0",
    "@docusaurus/preset-classic": "3.7.0",
    "@mdx-js/react": "^3.0.0",
    "clsx": "^2.0.0",
    "prism-react-renderer": "^2.3.0",
    "react": "^19.0.0",
    "react-dom": "^19.0.0"
  },
  "devDependencies": {
    "@docusaurus/module-type-aliases": "3.7.0",
    "@docusaurus/types": "3.7.0"
  },
  "browserslist": {
    "production": [">0.5%", "not dead", "not op_mini all"],
    "development": ["last 1 chrome version", "last 1 firefox version", "last 1 safari version"]
  }
}
```

**Step 3: 配置 Docusaurus**

```javascript
// docusaurus.config.js
module.exports = {
  title: 'YuleTech AutoSAR BSW',
  tagline: '开源 AutoSAR 基础软件平台',
  favicon: 'img/favicon.ico',
  url: 'https://yuletech.github.io',
  baseUrl: '/yuleASR/',
  organizationName: 'frisky1985',
  projectName: 'yuleASR',
  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',
  i18n: {
    defaultLocale: 'zh-Hans',
    locales: ['zh-Hans', 'en'],
  },
  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.js',
          editUrl: 'https://github.com/frisky1985/yuleASR/tree/main/website/',
        },
        blog: {
          showReadingTime: true,
          editUrl: 'https://github.com/frisky1985/yuleASR/tree/main/website/blog/',
        },
        theme: {
          customCss: './src/css/custom.css',
        },
      },
    ],
  ],
  themeConfig: {
    navbar: {
      title: 'YuleTech',
      logo: {
        alt: 'YuleTech Logo',
        src: 'img/logo.svg',
      },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'tutorialSidebar',
          position: 'left',
          label: '文档',
        },
        {to: '/blog', label: '博客', position: 'left'},
        {to: '/community', label: '社区', position: 'left'},
        {
          href: 'https://github.com/frisky1985/yuleASR',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: '文档',
          items: [
            { label: '快速开始', to: '/docs/intro' },
            { label: '概念指南', to: '/docs/concepts' },
          ],
        },
        {
          title: '社区',
          items: [
            { label: '论坛', href: 'https://forum.yuletech.com' },
            { label: 'GitHub Discussions', href: 'https://github.com/frisky1985/yuleASR/discussions' },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} YuleTech.`,
    },
  },
};
```

**Step 4: 配置侧边栏**

```javascript
// sidebars.js
module.exports = {
  tutorialSidebar: [
    'intro',
    {
      type: 'category',
      label: '快速开始',
      items: [
        'getting-started/installation',
        'getting-started/first-project',
        'getting-started/configuration',
      ],
    },
    {
      type: 'category',
      label: '核心概念',
      items: [
        'concepts/architecture',
        'concepts/mcal',
        'concepts/ecual',
        'concepts/services',
      ],
    },
    {
      type: 'category',
      label: '驱动开发',
      items: [
        'drivers/port-driver',
        'drivers/dio-driver',
        'drivers/can-driver',
        'drivers/pwm-driver',
      ],
    },
    {
      type: 'category',
      label: '高级主题',
      items: [
        'advanced/testing',
        'advanced/debugging',
        'advanced/optimization',
      ],
    },
  ],
};
```

**Step 5: 创建主页**

```jsx
// src/pages/index.js
import React from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

export default function Home() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <Layout title={siteConfig.title} description={siteConfig.tagline}>
      <header className="hero hero--primary">
        <div className="container">
          <Heading as="h1" className="hero__title">{siteConfig.title}</Heading>
          <p className="hero__subtitle">{siteConfig.tagline}</p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/docs/intro">
              开始学习 →
            </Link>
          </div>
        </div>
      </header>
      <main>
        <section className="features">
          <div className="container">
            <div className="row">
              <Feature title="标准兼容" description="完全兼容 AutoSAR 4.x 标准" />
              <Feature title="轻量高效" description="专为资源受限场景优化" />
              <Feature title="开源免费" description="Apache 2.0 授权，商业友好" />
            </div>
          </div>
        </section>
      </main>
    </Layout>
  );
}

function Feature({title, description}) {
  return (
    <div className="col col--4">
      <h3>{title}</h3>
      <p>{description}</p>
    </div>
  );
}
```

**Step 6: 测试构建**

```bash
cd ~/yuleASR/website
npm install
npm run build
```

Expected: Build succeeds with no errors

**Step 7: 提交**

```bash
cd ~/yuleASR
git add website/
git commit -m "feat(website): 初始化 Docusaurus 文档站" -m "- 添加 Docusaurus 3.7 配置" -m "- 创建基础项目结构" -m "- 配置中英双语支持"
```

---

### Task 2: 编写核心文档

**Files:**
- Create: `website/docs/intro.md`
- Create: `website/docs/getting-started/installation.md`
- Create: `website/docs/getting-started/first-project.md`
- Create: `website/docs/concepts/architecture.md`

**Step 1: 编写介绍页**

```markdown
---
sidebar_position: 1
---

# 介绍

## YuleTech AutoSAR BSW 是什么？

YuleTech AutoSAR BSW 是一个开源的 AutoSAR 基础软件 (BSW) 实现，专为嵌入式汽车电子控制单元 (ECU) 设计。

## 特性

- 📋 **标准兼容**: 完全兼容 AutoSAR Classic Platform 4.x
- ⚡ **轻量高效**: 最小的内存占用和 CPU 开销
- 🔧 **模块化设计**: 灵活的配置和扩展
- 📚 **完善文档**: 详细的技术文档和示例
- ✅ **全面测试**: 单元测试 + 集成测试覆盖

## 快速开始

```bash
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR
python3 tools/build/build.py configure --tests
python3 tools/build/build.py build
python3 tools/build/build.py test
```

## 支持的平台

| 芯片 | 状态 | 版本 |
|-----|------|------|
| STM32F4 | 支持 | v1.0.0 |
| STM32H7 | 开发中 | v1.1.0 |
| TC3xx | 计划 | v2.0.0 |
```

**Step 2: 编写安装指南**

```markdown
---
sidebar_position: 1
---

# 安装

## 系统要求

- **操作系统**: Linux (Ubuntu 20.04+) / macOS / Windows (WSL2)
- **编译器**: GCC ARM 10.3+ 或 IAR/Keil
- **构建工具**: CMake 3.20+, Python 3.8+
- **依赖**: git, make, ninja

## 一键安装

### Ubuntu/Debian

```bash
# 安装工具链
sudo apt-get update
sudo apt-get install -y cmake ninja-build python3 python3-pip git

# 安装 ARM 工具链
sudo apt-get install -y gcc-arm-none-eabi

# 安装 Python 依赖
pip3 install -r tools/requirements.txt
```

### 验证安装

```bash
cmake --version  # 应显示 >= 3.20
arm-none-eabi-gcc --version  # 应显示 ARM 版本
python3 tools/build/build.py --help  # 应显示帮助信息
```

## 克隆代码库

```bash
git clone --recursive https://github.com/frisky1985/yuleASR.git
cd yuleASR
```
```

**Step 3: 编写第一个项目教程**

```markdown
---
sidebar_position: 2
---

# 第一个项目

## 创建项目

```bash
mkdir my_ecu_project
cd my_ecu_project
cp -r ../yuleASR/template/* .
```

## 配置驱动

编辑 `config/Port_Cfg.h`:

```c
#ifndef PORT_CFG_H
#define PORT_CFG_H

#define PORT_NUM_PINS 32

#endif
```

## 构建项目

```bash
python3 tools/build/build.py configure
python3 tools/build/build.py build
```

## 验证结果

```bash
ls build/my_ecu.hex  # 确认生成固件
```

## 下一步

- [学习配置驱动](configuration.md)
- [阅读架构文档](../concepts/architecture.md)
```

**Step 4: 编写架构概述**

```markdown
---
sidebar_position: 1
---

# 架构概述

## 层次结构

```
┌─────────────────────────────────────────────────┐
│              Application Layer (ASW)               │
├─────────────────────────────────────────────────┤
│              Runtime Environment (RTE)              │
├─────────────────────────────────────────────────┤
│           Basic Software (BSW) - 本项目重点          │
│  ┌────────────┬────────────┬────────────┬────────────┐  │
│  │ Services │  ECUAL   │  MCAL   │  DET    │  │
│  │   (COM)  │ (DEM)    │ (PORT) │ (Error)│  │
├─────────────────────────────────────────────────┤
│              Microcontroller Abstraction           │
└─────────────────────────────────────────────────┘
```

## 核心模块

| 层级 | 模块 | 功能 |
|------|------|------|
| MCAL | Port, Dio, Can, Pwm, Spi, Wdg | 微控制器驱动 |
| ECUAL | Dem, Dcm, Com | 通信中间件 |
| Services | Com Services | 通信服务 |
| DET | Default Error Tracer | 错误追踪 |

## 数据流

```
ASW → RTE → BSW → MCAL → Hardware
  ←──────────────────────←
```
```

**Step 5: 测试文档站**

```bash
cd ~/yuleASR/website
npm run build
```

Expected: Build succeeds, check `build/` folder

**Step 6: 提交**

```bash
cd ~/yuleASR
git add website/docs/
git commit -m "docs: 添加核心文档内容" -m "- 介绍页面" -m "- 安装指南" -m "- 第一个项目教程" -m "- 架构概述"
```

---

### Task 3: 配置 GitHub Pages 自动部署

**Files:**
- Modify: `.github/workflows/ci.yml`

**Step 1: 添加文档部署 job**

Append to `.github/workflows/ci.yml`:

```yaml
  deploy-docs:
    needs: test
    if: github.ref == 'refs/heads/master'
    runs-on: ubuntu-latest
    permissions:
      contents: read
      pages: write
      id-token: write
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20'
          cache: 'npm'
          cache-dependency-path: website/package-lock.json
      - name: Install dependencies
        run: cd website && npm ci
      - name: Build docs
        run: cd website && npm run build
      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: website/build
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

**Step 2: 测试 workflow**

```bash
git push origin feature/enhance-testing-framework
# Check Actions tab in GitHub
```

**Step 3: 提交**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: 添加 GitHub Pages 文档部署" -m "- 自动部署 Docusaurus 站点"
```

---

## 阶段二：社区平台 (Week 2-4)

### Task 4: 创建社区页面

**Files:**
- Create: `website/src/pages/community.js`

**Step 1: 编写社区页面**

```jsx
import React from 'react';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';

export default function Community() {
  return (
    <Layout title="社区" description="加入 YuleTech 社区">
      <main className="container margin-vert--lg">
        <h1>加入 YuleTech 社区</h1>
        <div className="row">
          <div className="col col--6">
            <h3>💬 论坛</h3>
            <p>加入我们的 Discourse 论坛，与其他开发者交流。</p>
            <Link className="button button--primary" href="https://forum.yuletech.com">
              访问论坛
            </Link>
          </div>
          <div className="col col--6">
            <h3>🐛 GitHub Issues</h3>
            <p>报告 bug 或提出功能请求。</p>
            <Link className="button button--primary" href="https://github.com/frisky1985/yuleASR/issues">
              创建 Issue
            </Link>
          </div>
        </div>
        <div className="row margin-top--lg">
          <div className="col col--6">
            <h3>📈 贡献指南</h3>
            <p>了解如何为项目做出贡献。</p>
            <Link className="button button--primary" to="/docs/contributing">
              查看指南
            </Link>
          </div>
        </div>
      </main>
    </Layout>
  );
}
```

**Step 2: 测试页面**

```bash
cd ~/yuleASR/website
npm run start &
curl http://localhost:3000/yuleASR/community | grep "加入 YuleTech"
```

Expected: Page content found

**Step 3: 提交**

```bash
git add website/src/pages/community.js
git commit -m "feat(website): 添加社区页面"
```

---

### Task 5: 创建贡献指南

**Files:**
- Create: `website/docs/contributing.md`
- Modify: `website/sidebars.js`

**Step 1: 编写贡献文档**

```markdown
---
sidebar_position: 99
---

# 贡献指南

## 如何贡献

感谢您对 YuleTech 的关注！我们欢迎各种形式的贡献。

## 流程

1. **Fork** 仓库
2. **创建分支**: `git checkout -b feature/my-feature`
3. **提交变更**: `git commit -m "feat: add feature"`
4. **推送分支**: `git push origin feature/my-feature`
5. **创建 Pull Request**

## 代码规范

- 使用 4 空格缩进
- 函数名使用 `Module_FunctionName` 格式
- 添加注释说明
- 测试覆盖新功能

## 测试

```bash
# 运行测试
python3 tools/build/build.py test

# 运行静态分析
python3 tools/build/build.py lint
```

## 申请开发者认证

如果您想更深入地参与，可以申请成为认证开发者。
```

**Step 2: 更新侧边栏**

Add to `website/sidebars.js`:
```javascript
'contributing',
```

**Step 3: 提交**

```bash
git add website/docs/contributing.md website/sidebars.js
git commit -m "docs: 添加贡献指南"
```

---

### Task 6: 创建常见问题页面

**Files:**
- Create: `website/docs/faq.md`
- Create: `website/docs/troubleshooting.md`

**Step 1: 编写 FAQ**

```markdown
---
sidebar_position: 100
---

# 常见问题

## YuleTech 和 AutoSAR 商业版本有什么区别？

YuleTech 是开源实现，专注于：
- 轻量化设计
- 主要 MCU 平台支持
- 社区驱动开发

商业版本 (EB, Vector) 提供更完整的工具链和技术支持。

## 支持哪些芯片？

当前支持 STM32F4 系列。更多平台支持中...

## 如何报告 Bug？

1. 检查是否已存在相关 Issue
2. 使用模板创建新 Issue
3. 提供完整的复现步骤

## 是否可以用于商业项目？

是的，Apache 2.0 授权允许商业使用。
```

**Step 2: 编写故障排除指南**

```markdown
---
sidebar_position: 101
---

# 故障排除

## 构建失败

### 错误: `cmake not found`

**解决:** 安装 CMake

```bash
sudo apt-get install cmake
```

### 错误: `arm-none-eabi-gcc: command not found`

**解决:** 安装 ARM 工具链

```bash
sudo apt-get install gcc-arm-none-eabi
```

## 测试失败

### 错误: `mock function not found`

**解决:** 确保 mock 文件已包含在测试中

```c
#include "mock_det.h"
```
```

**Step 3: 更新侧边栏**

```javascript
'faq',
'troubleshooting',
```

**Step 4: 提交**

```bash
git add website/docs/faq.md website/docs/troubleshooting.md website/sidebars.js
git commit -m "docs: 添加 FAQ 和故障排除指南"
```

---

## 阶段三：论坛部署准备 (Optional)

### Task 7: 准备 Discourse Docker 部署

**Files:**
- Create: `infra/discourse/docker-compose.yml`
- Create: `infra/discourse/README.md`

**Step 1: 创建 Docker Compose**

```yaml
version: '3'

services:
  discourse:
    image: discourse/discourse:latest
    ports:
      - "80:80"
    volumes:
      - discourse-data:/var/discourse/shared
    environment:
      DISCOURSE_HOSTNAME: forum.yuletech.com
      DISCOURSE_DEVELOPER_EMAILS: admin@yuletech.com
      DISCOURSE_SMTP_ADDRESS: smtp.example.com
      DISCOURSE_SMTP_PORT: 587
    depends_on:
      - postgres
      - redis

  postgres:
    image: postgres:15
    volumes:
      - postgres-data:/var/lib/postgresql/data
    environment:
      POSTGRES_USER: discourse
      POSTGRES_PASSWORD: ${DB_PASSWORD}

  redis:
    image: redis:7
    volumes:
      - redis-data:/data

volumes:
  discourse-data:
  postgres-data:
  redis-data:
```

**Step 2: 创建部署文档**

```markdown
# Discourse 论坛部署指南

## 要求

- Docker + Docker Compose
- 至少 2GB 内存
- 域名和 SSL 证书

## 部署

```bash
cd infra/discourse
docker-compose up -d
```

## 配置

1. 访问 `http://your-server`
2. 创建管理员账户
3. 配置邮件发送
4. 配置 OAuth (可选)
```

**Step 3: 提交**

```bash
git add infra/
git commit -m "feat(infra): 添加 Discourse 论坛部署配置"
```

---

## 里程碑时间表

| 周 | 任务 | 贡献者 |
|----|------|---------|
| 1  | Task 1-2: 文档站 + 核心文档 | Docs Agent |
| 1  | Task 3: GitHub Pages 部署 | CI/CD Agent |
| 2  | Task 4: 社区页面 | Frontend Agent |
| 2  | Task 5: 贡献指南 | Community Agent |
| 2  | Task 6: FAQ/故障排除 | Support Agent |
| 3-4| Task 7: 论坛部署 (可选) | DevOps Agent |

---

## 验收标准

- [ ] 文档站可访问: `https://yuletech.github.io/yuleASR/`
- [ ] 所有核心文档完成
- [ ] 贡献指南已发布
- [ ] 社区页面可用
- [ ] GitHub Pages 自动部署流程测试通过

---

## 验证命令

```bash
# 验证文档站构建
cd website && npm run build

# 验证贡献者工作流
git log --oneline -10

# 验证页面内容
curl -s https://yuletech.github.io/yuleASR/docs/intro | grep "YuleTech"
```
