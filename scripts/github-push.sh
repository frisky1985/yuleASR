#!/bin/bash
# GitHub Push Script with PAT
# 自动使用环境变量中的PAT进行推送

set -e

# 检查环境变量
if [ -z "$GITHUB_PAT" ]; then
    echo "❌ 错误: 未设置 GITHUB_PAT 环境变量"
    echo "请检查 ~/.bashrc 是否配置了 PAT"
    exit 1
fi

if [ -z "$GITHUB_USER" ]; then
    GITHUB_USER="frisky1985"
fi

if [ -z "$GITHUB_REPO" ]; then
    GITHUB_REPO="yuleASR"
fi

# 获取当前分支
CURRENT_BRANCH=$(git branch --show-current)

# 如果指定了分支参数，则使用该分支
if [ -n "$1" ]; then
    TARGET_BRANCH="$1"
else
    TARGET_BRANCH="$CURRENT_BRANCH"
fi

echo "🚀 推送到 GitHub..."
echo "   仓库: $GITHUB_USER/$GITHUB_REPO"
echo "   分支: $TARGET_BRANCH"

# 设置带有PAT的远程URL
git remote set-url origin "https://$GITHUB_USER:$GITHUB_PAT@github.com/$GITHUB_USER/$GITHUB_REPO.git"

# 执行推送
git push origin "$TARGET_BRANCH"

# 清理凭据
git remote set-url origin "https://github.com/$GITHUB_USER/$GITHUB_REPO.git"

echo "✅ 推送成功: $TARGET_BRANCH"
