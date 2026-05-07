#!/bin/bash
# MQTT TLS 测试构建脚本

set -e

echo "=== MQTT TLS 测试构建 ==="

# 创建构建目录
mkdir -p build
cd build

# 配置
echo "配置 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 构建
echo "构建项目..."
make -j4

# 运行测试
echo "运行测试..."
./mqtt_tls_tests

echo "=== 构建完成 ==="
