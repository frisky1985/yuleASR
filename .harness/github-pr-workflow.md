---
name: github-pr-workflow
description: GitHub Pull Request 完整工作流 Skill。涵盖从功能开发、代码提交、分支推送、PR 创建到合并的端到端流程。适用于团队协作和代码审查管理。
---

# GitHub Pull Request 工作流

## 概述

本 Skill 定义了从功能开发到 PR 合并的完整 GitHub 协作流程。

## 完整工作流

### Phase 1: 本地开发

#### 1.1 创建功能分支
```bash
# 从主分支创建功能分支
git checkout -b feature/功能名称

# 示例
git checkout -b feature/add-dev-process-skill
```

#### 1.2 开发并提交代码
```bash
# 添加变更到暂存区
git add -A

# 创建提交（遵循提交规范）
git commit -m "type: 提交描述

详细说明

🤖 Generated with [Qoder](https://qoder.com)"
```

### Phase 2: 推送分支

#### 2.1 推送到远程仓库
```bash
# 首次推送（建立追踪关系）
git push -u origin feature/功能名称

# 后续推送
git push
```

### Phase 3: 创建 Pull Request

#### 3.1 使用 GitHub API 创建 PR
```bash
curl -X POST \
  -H "Authorization: token $GITHUB_PAT" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/用户名/仓库名/pulls \
  -d '{
    "title": "PR标题",
    "head": "feature/分支名",
    "base": "master",
    "body": "PR描述内容"
  }'
```

#### 3.2 PR 描述模板
```markdown
## 变更说明

描述本次变更的目的和内容。

### 新增内容

- 功能点 1
- 功能点 2

### 变更统计

- 新增文件：X 个
- 修改文件：X 个
- 新增行数：X 行

### 检查清单

- [ ] 代码规范检查通过
- [ ] 功能验证通过
- [ ] 文档更新完整

---
🤖 Generated with [Qoder](https://qoder.com)
```

### Phase 4: 代码审查

#### 4.1 审查清单
- [ ] 代码逻辑正确
- [ ] 命名规范符合要求
- [ ] 错误处理完善
- [ ] 注释清晰完整
- [ ] 无安全漏洞

#### 4.2 处理审查意见
```bash
# 根据审查意见修改代码
git add -A
git commit -m "fix: 修复审查意见

- 修复问题 1
- 修复问题 2

🤖 Generated with [Qoder](https://qoder.com)"

# 推送到远程（自动更新 PR）
git push
```

### Phase 5: 合并 PR

#### 5.1 使用 GitHub API 合并
```bash
# 创建合并请求体文件 merge_body.json
{
  "commit_title": "合并后的提交标题",
  "commit_message": "合并后的提交描述",
  "merge_method": "merge"
}

# 执行合并
curl -X PUT \
  -H "Authorization: token $GITHUB_PAT" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/用户名/仓库名/pulls/PR编号/merge \
  -d @merge_body.json
```

#### 5.2 合并方法
| 方法 | 说明 | 适用场景 |
|:-----|:-----|:---------|
| `merge` | 创建合并提交，保留完整历史 | 需要保留分支历史 |
| `squash` | 压缩所有提交为一个 | 清理琐碎提交 |
| `rebase` | 变基合并，线性历史 | 保持历史整洁 |

### Phase 6: 本地同步

#### 6.1 切换回主分支并拉取更新
```bash
# 切换回主分支
git checkout master

# 拉取最新代码
git pull origin master

# 删除已合并的功能分支（可选）
git branch -d feature/功能名称
```

## 环境变量配置

### 设置 GitHub PAT
```powershell
# Windows PowerShell
[Environment]::SetEnvironmentVariable('GITHUB_PAT', 'ghp_xxx', 'User')
[Environment]::SetEnvironmentVariable('GITHUB_USERNAME', '用户名', 'User')
```

## 提交规范

```
<type>: <subject>

<body>

🤖 Generated with [Qoder](https://qoder.com)
```

### Type 类型
| 类型 | 说明 |
|:-----|:-----|
| `feat` | 新功能 |
| `fix` | 修复 |
| `docs` | 文档 |
| `style` | 格式 |
| `refactor` | 重构 |
| `test` | 测试 |
| `chore` | 构建/工具 |

## 快速命令参考

| 阶段 | 命令 | 说明 |
|:-----|:-----|:-----|
| 创建分支 | `git checkout -b feature/xxx` | 从主分支创建 |
| 提交代码 | `git commit -m "type: desc"` | 遵循提交规范 |
| 推送分支 | `git push -u origin feature/xxx` | 首次推送 |
| 创建 PR | GitHub API | 使用 curl 创建 |
| 合并 PR | GitHub API | 使用 curl 合并 |
| 同步代码 | `git pull origin master` | 拉取最新代码 |

## 检查清单

### 提交前检查
- [ ] 代码编译通过
- [ ] 测试通过
- [ ] 代码规范检查通过
- [ ] 提交信息符合规范

### PR 创建前检查
- [ ] 分支已推送到远程
- [ ] 变更描述清晰完整
- [ ] 相关 Issue 已关联

### 合并前检查
- [ ] 代码审查通过
- [ ] 所有检查项通过
- [ ] 无冲突需要解决

## 最佳实践

1. **小步提交**：每次提交只做一件事
2. **清晰描述**：提交信息和 PR 描述要清晰
3. **及时推送**：开发过程中及时推送代码
4. **快速审查**：PR 创建后尽快进行审查
5. **及时合并**：审查通过后及时合并
6. **保持同步**：定期拉取主分支更新
