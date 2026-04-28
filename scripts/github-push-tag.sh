#!/bin/bash
# GitHub Tag Push Script with PAT

set -e

if [ -z "$GITHUB_PAT" ]; then
    echo "❌ 错误: 未设置 GITHUB_PAT 环境变量"
    exit 1
fi

GITHUB_USER="${GITHUB_USER:-frisky1985}"
GITHUB_REPO="${GITHUB_REPO:-yuleASR}"

if [ -z "$1" ]; then
    echo "使用方法: $0 <tag名>"
    echo "示例: $0 v0.6.0"
    exit 1
fi

TAG_NAME="$1"

echo "🏷️  推送标签到 GitHub..."
echo "   标签: $TAG_NAME"

# 设置带有PAT的远程URL
git remote set-url origin "https://$GITHUB_USER:$GITHUB_PAT@github.com/$GITHUB_USER/$GITHUB_REPO.git"

# 推送标签
git push origin "$TAG_NAME"

# 清理凭据
git remote set-url origin "https://github.com/$GITHUB_USER/$GITHUB_REPO.git"

echo "✅ 标签推送成功: $TAG_NAME"
echo "🔗 https://github.com/$GITHUB_USER/$GITHUB_REPO/releases/tag/$TAG_NAME"
