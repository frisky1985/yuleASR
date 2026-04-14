---
name: yuletech-complete-workflow
description: 予乐科技 AutoSAR BSW 完整项目开发工作流 Skill。涵盖从代码开发、验证测试、文档生成、Git 版本控制到 GitHub 协作的端到端流程。适用于完整项目交付和发布管理。
---

# 予乐科技完整项目开发工作流

## 概述

本 Skill 定义了从代码开发到 GitHub 发布的完整端到端工作流。

## 完整工作流

### Phase 1: 代码开发

#### 1.1 开发 Service 层模块
```
任务：完成 Com, Dcm, Dem 模块
- 创建模块源文件 (Module.c)
- 实现 API 函数
- 添加 DET 错误检测
- 遵循命名规范
```

#### 1.2 开发 RTE 层
```
任务：完成 RTE 运行时环境
- Rte.c - 核心实现
- Rte_ComInterface.c - COM 接口
- Rte_NvMInterface.c - NVM 接口
- Rte_Scheduler.c - 调度器
```

### Phase 2: 验证测试

#### 2.1 生成验证报告
```bash
# 为每个模块创建验证报告
verification/
├── pdur_verification.md
├── nvm_verification.md
└── rte_verification.md
```

#### 2.2 验证清单
- [ ] 功能验证通过
- [ ] 代码规范检查
- [ ] 接口兼容性检查

### Phase 3: 文档生成

#### 3.1 项目文档
```
docs/
├── README.md              # 项目主文档
├── architecture.md        # 架构设计
├── api-reference.md       # API 参考
├── development-guide.md   # 开发指南
├── modules.md            # 模块清单
└── changelog.md          # 版本历史
```

#### 3.2 README 徽章
```markdown
GitHub 统计徽章:
- Stars, Forks, Issues, License

技术栈徽章:
- AutoSAR 4.x, C99, i.MX8M Mini

项目统计徽章:
- 模块数, 代码行数, 验证报告数
```

### Phase 4: Git 版本控制

#### 4.1 初始化仓库
```bash
git init
git config user.email "developer@company.com"
git config user.name "Developer"
```

#### 4.2 创建提交
```bash
git add -A
git commit -m "Initial commit: Complete AutoSAR BSW Platform v1.0.0"
```

### Phase 5: GitHub 协作

#### 5.1 创建仓库
```bash
# 使用 GitHub API 创建公开仓库
curl -X POST \
  -H "Authorization: token $GITHUB_PAT" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/user/repos \
  -d '{"name":"repo-name","private":false}'
```

#### 5.2 推送代码
```bash
git remote add origin https://github.com/username/repo.git
git push -u origin master
```

#### 5.3 修改仓库名
```bash
curl -X PATCH \
  -H "Authorization: token $GITHUB_PAT" \
  https://api.github.com/repos/username/old-name \
  -d '{"name":"new-name"}'
```

#### 5.4 创建 Pull Request
```bash
# 1. 创建功能分支
git checkout -b feature/xxx

# 2. 推送分支
git push -u origin feature/xxx

# 3. 创建 PR
curl -X POST \
  -H "Authorization: token $GITHUB_PAT" \
  https://api.github.com/repos/username/repo/pulls \
  -d '{"title":"PR标题","head":"feature/xxx","base":"master"}'
```

### Phase 6: Skill 保存

#### 6.1 创建 Skill
```
~/.qoderwork/skills/skill-name/
├── SKILL.md          # 主要说明文档
├── reference.md      # 详细参考
└── examples.md       # 使用示例
```

#### 6.2 更新项目引用
```bash
# 复制 skill 到项目目录
cp SKILL.md project/.harness/skill-name.md

# 更新 README 添加 skill 引用
```

## 环境变量配置

### GitHub 认证
```bash
# Windows PowerShell
[Environment]::SetEnvironmentVariable('GITHUB_USERNAME', 'username', 'User')
[Environment]::SetEnvironmentVariable('GITHUB_PAT', 'ghp_xxx', 'User')
```

## 提交规范

```
<type>: <subject>

<body>

🤖 Generated with [Qoder](https://qoder.com)
```

Type 类型：
- `feat`: 新功能
- `fix`: 修复
- `docs`: 文档
- `style`: 格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建/工具

## 检查清单

### 开发完成检查
- [ ] 所有模块实现完成
- [ ] 验证报告生成
- [ ] 代码规范符合要求

### 文档完成检查
- [ ] README 完整
- [ ] 架构文档
- [ ] API 参考
- [ ] 徽章添加

### GitHub 发布检查
- [ ] 仓库创建
- [ ] 代码推送
- [ ] PR 创建（如需要）
- [ ] Skill 保存

## 快速命令参考

| 阶段 | 命令 | 说明 |
|:-----|:-----|:-----|
| 开发 | `/triple-dev` | 执行开发 |
| 验证 | `/triple-verify` | 验证审查 |
| 提交 | `git commit` | 创建提交 |
| 推送 | `git push` | 推送到远程 |
| PR | GitHub API | 创建 Pull Request |
