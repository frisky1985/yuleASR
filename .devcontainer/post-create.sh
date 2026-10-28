#!/bin/bash
# post-create.sh — Codespaces 容器首次启动时执行
set -euo pipefail

echo "=== 安装系统依赖 ==="
sudo apt-get update -qq
sudo apt-get install -y -qq \
  build-essential \
  gcc g++ gcov \
  cmake make ninja-build \
  lcov \
  git \
  python3 python3-pip python3-venv \
  gdb \
  file \
  2>/dev/null

echo "=== 版本确认 ==="
gcc --version | head -1
cmake --version | head -1
lcov --version
python3 --version

echo "=== 配置 build-coverage ==="
# -DYULE_PLATFORM_S32K312=OFF: 在 Linux x86_64 容器上跑 host build，
#  强制 MCAL 驱动走非 S32K312 代码路径（寄存器 mock 模式）
cmake -S . -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DENABLE_COVERAGE=ON \
  -DYULE_PLATFORM_S32K312=OFF \
  -G Ninja

echo "=== 编译 ==="
cmake --build build-coverage --parallel "$(nproc)"

echo ""
echo "=== ✅ 就绪 ==="
echo "  ctest --test-dir build-coverage            跑全部测试"
echo "  bash build-coverage/coverage-run.sh        全量覆盖率 (需先生成)"
echo "  cmake --build build-coverage --target help 看所有 target"
