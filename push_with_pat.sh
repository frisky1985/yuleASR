#!/bin/bash
# 使用保存的PAT推送

# 从环境变量或文件读取PAT
if [ -f ~/.github_pat ]; then
    GITHUB_PAT=$(cat ~/.github_pat)
elif [ -n "$GITHUB_PAT" ]; then
    GITHUB_PAT="$GITHUB_PAT"
else
    echo "错误: 找不到PAT"
    echo "请设置环境变量 GITHUB_PAT 或创建 ~/.github_pat 文件"
    exit 1
fi

cd /home/admin/eth-dds-integration

# 设置带PAT的远程URL
git remote set-url origin https://frisky1985:${GITHUB_PAT}@github.com/frisky1985/yuleASR.git

# 推送
echo "推送到GitHub..."
git push -u origin main
PUSH_RESULT=$?

# 清理凭据
git remote set-url origin https://github.com/frisky1985/yuleASR.git

if [ $PUSH_RESULT -eq 0 ]; then
    echo "✅ 推送成功！"
    git remote -v
else
    echo "❌ 推送失败"
    exit 1
fi
