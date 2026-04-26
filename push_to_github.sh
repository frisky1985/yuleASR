#!/bin/bash
# GitHub 推送脚本 - 需要填写PAT

# 配置
USERNAME="frisky1985"
REPO="yuleASR"
BRANCH="main"

# 提示输入PAT
echo "请输入GitHub Personal Access Token (PAT):"
read -s PAT
echo ""

if [ -z "$PAT" ]; then
    echo "错误: PAT不能为空"
    exit 1
fi

# 设置带PAT的远程URL
echo "设置远程URL..."
git remote set-url origin https://${USERNAME}:${PAT}@github.com/${USERNAME}/${REPO}.git

# 推送
echo "推送到GitHub..."
git push -u origin ${BRANCH}

# 清理凭据 (安全必需)
echo "清理凭据..."
git remote set-url origin https://github.com/${USERNAME}/${REPO}.git

# 验证
echo "验证远程URL (应不包含PAT):"
git remote -v

echo ""
echo "推送完成！"
