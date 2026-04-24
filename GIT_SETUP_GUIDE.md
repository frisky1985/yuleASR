# Git仓库设置指南

## 仓库信息

- **本地路径**: `/home/admin/eth-dds-integration`
- **远程地址**: `https://github.com/yourusername/eth-dds-automotive.git`
- **默认分支**: `main`
- **总文件数**: 289个
- **总行数**: ~120,000行

## 快速开始

### 1. 创建远程仓库

在GitHub/GitLab上创建新的空仓库，然后更新远程URL：

```bash
# 更改为您的实际仓库URL
git remote set-url origin https://github.com/YOUR_USERNAME/YOUR_REPO.git
```

### 2. 推送到远程

```bash
cd /home/admin/eth-dds-integration

# 推送到远程仓库
git push -u origin main
```

如果需要认证，使用以下方式之一：

#### 方式A: 使用HTTPS + Personal Access Token
```bash
# 在GitHub Settings -> Developer settings -> Personal access tokens 创建Token
git push -u origin main
# 输入用户名: 您的GitHub用户名
# 输入密码: 创建的Personal Access Token
```

#### 方式B: 使用SSH密钥
```bash
# 配置SSH密钥后，更改为SSH URL
git remote set-url origin git@github.com:YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

### 3. 验证推送

```bash
# 查看远程分支
git branch -r

# 查看提交历史
git log --oneline --graph --all
```

## 常用Git命令

### 每日工作流

```bash
# 获取最新代码
git pull origin main

# 创建新分支
git checkout -b feature/new-feature

# 添加更改
git add .
git commit -m "feat: add new feature"

# 推送分支
git push -u origin feature/new-feature
```

### 查看状态

```bash
# 查看工作区状态
git status

# 查看文件修改
git diff

# 查看提交历史
git log --oneline -10

# 查看远程信息
git remote -v
```

## 项目结构

```
eth-dds-automotive/
├── src/              # 源代码
│   ├── dds/            # DDS核心
│   ├── transport/      # 传输层
│   ├── ethernet/       # 以太网驱动
│   ├── tsn/            # TSN协议
│   ├── autosar/        # AUTOSAR集成
│   ├── nvm/            # NvM存储管理
│   └── safety/         # 功能安全
├── dds-config-tool/  # 配置工具
├── tools/            # 监控工具
├── examples/         # 应用示例
├── tests/            # 测试套件
├── docs/             # 文档
└── scripts/          # 脚本
```

## 版本标签

```bash
# 创建版本标签
git tag -a v2.0.0 -m "Release version 2.0.0"

# 推送标签到远程
git push origin v2.0.0

# 推送所有标签
git push origin --tags
```

## CI/CD配置

项目已包含GitHub Actions CI配置在 `.github/workflows/ci.yml`

推送后会自动触发：
- 代码构建
- 单元测试
- 代码覆盖率检查

## 帮助

如遇问题，请检查：
1. 远程仓库URL是否正确
2. 网络连接是否正常
3. 认证信息是否有效
