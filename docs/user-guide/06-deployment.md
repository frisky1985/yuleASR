# 第6章: 部署指南

**目标**: 详细介绍交叉编译和目标设备烧录流程。

---

## 6.1 交叉编译

### 6.1.1 支持的平台

| 平台 | 架构 | 工具链 | 状态 |
|------|------|---------|------|
| Linux x86_64 | x86_64 | GCC/Clang | 完全支持 |
| Linux ARM64 | aarch64 | GCC | 完全支持 |
| FreeRTOS | ARM Cortex-M | GCC | 完全支持 |
| Bare-metal | ARM Cortex-R | GCC | 完全支持 |
| QNX | x86_64/ARM | QCC | 部分支持 |

### 6.1.2 工具链安装

#### ARM Linux 工具链

```bash
# Ubuntu/Debian
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 或使用ARM官方工具链
wget https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xf arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
export PATH=$PATH:$(pwd)/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu/bin
```

#### ARM Embedded 工具链 (FreeRTOS/Bare-metal)

```bash
# 安装arm-none-eabi-gcc
sudo apt-get install gcc-arm-none-eabi

# 或使用官方发布
wget https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz
tar -xf arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz
```

### 6.1.3 配置文件

#### ARM Linux 工具链配置

创建 `cmake/toolchain-arm64.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定编译器
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_AR aarch64-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-linux-gnu-ranlib)
set(CMAKE_STRIP aarch64-linux-gnu-strip)

# 查找路径
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 编译选项
set(CMAKE_C_FLAGS "-march=armv8-a -mtune=cortex-a72" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-march=armv8-a -mtune=cortex-a72" CACHE STRING "" FORCE)
```

#### FreeRTOS 工具链配置

创建 `cmake/toolchain-freertos.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 指定编译器
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

# 编译选项
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard" CACHE STRING "" FORCE)

# 链接选项
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/platform/freertos/linker.ld -nostartfiles" CACHE STRING "" FORCE)
```

### 6.1.4 编译步骤

#### 交叉编译到ARM Linux

```bash
# 创建构建目录
mkdir build_arm64 && cd build_arm64

# 配置
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-arm64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SECURITY=ON \
    -DPLATFORM=linux

# 编译
make -j$(nproc)

# 安装到交叉环境
make DESTDIR=$PWD/install install
```

#### 交叉编译到FreeRTOS

```bash
# 创建构建目录
mkdir build_freertos && cd build_freertos

# 配置
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-freertos.cmake \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DPLATFORM=freertos \
    -DFREERTOS_CONFIG_FILE=/path/to/FreeRTOSConfig.h \
    -DENABLE_SECURITY=OFF \
    -DENABLE_TSN=OFF

# 编译
make -j$(nproc)

# 生成二进制文件
arm-none-eabi-objcopy -O binary eth_dds.elf eth_dds.bin
```

### 6.1.5 交叉编译选项

| 选项 | ARM Linux | FreeRTOS | Bare-metal |
|-----|-----------|----------|------------|
| 构建类型 | Release | MinSizeRel | MinSizeRel |
| ENABLE_SECURITY | ON | OFF | OFF |
| ENABLE_TSN | ON | OFF | OFF |
| ENABLE_DIAGNOSTICS | ON | ON | ON |
| ENABLE_OTA | ON | OFF | OFF |
| USE_STATIC_LIBS | OFF | ON | ON |

## 6.2 目标设备部署

### 6.2.1 部署流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    Deployment Workflow                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. Cross Compile ─────────┾                                         │
│       │                                                  │              │
│       v                                                  v              │
│  2. Package ───────────────┾                                      │
│       │                                                  │              │
│       v                                                  v              │
│  3. Transfer to Device ───┾                                      │
│       │                                                  │              │
│       v                                                  v              │
│  4. Install/Flash ───────┾                                      │
│       │                                                  │              │
│       v                                                  v              │
│  5. Configure ───────────┾                                      │
│       │                                                  │              │
│       v                                                  v              │
│  6. Verify ──────────────┾                                      │
│                                                                         │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2.2 打包

```bash
# 创建部署包
mkdir -p deploy_package/{bin,lib,config,scripts}

# 复制二进制文件
cp build_arm64/bin/* deploy_package/bin/
cp build_arm64/lib/* deploy_package/lib/

# 复制配置文件
cp -r config/* deploy_package/config/

# 复制启动脚本
cp scripts/start.sh deploy_package/scripts/
cp scripts/stop.sh deploy_package/scripts/

# 打包
tar -czf eth_dds_v2.0.0_arm64.tar.gz deploy_package/
```

### 6.2.3 传输到设备

```bash
# 使用SCP传输
scp eth_dds_v2.0.0_arm64.tar.gz root@target_device:/tmp/

# 使用NFS挂载
mount -t nfs server:/export/nfs /mnt/nfs
cp eth_dds_v2.0.0_arm64.tar.gz /mnt/nfs/

# 使用SD卡/
mkdir -p /media/sdcard
cp eth_dds_v2.0.0_arm64.tar.gz /media/sdcard/
```

### 6.2.4 安装

#### Linux目标

```bash
# 在目标设备上执行
cd /tmp
tar -xzf eth_dds_v2.0.0_arm64.tar.gz
cd deploy_package

# 安装到系统
sudo cp -r bin/* /usr/local/bin/
sudo cp -r lib/* /usr/local/lib/
sudo cp -r config/* /etc/dds/
sudo cp -r scripts/* /usr/local/bin/

# 更新动态链接库缓存
sudo ldconfig
```

#### FreeRTOS/Bare-metal目标

```bash
# 使用调试器烧录
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "program eth_dds.bin 0x08000000 verify reset exit"

# 使用串口烧录
stm32flash -w eth_dds.bin -v -g 0x0 /dev/ttyUSB0
```

### 6.2.5 配置

```bash
# 创建DDS配置目录
sudo mkdir -p /etc/dds

# 复制配置文件
sudo cp config/dds_config.xml /etc/dds/
sudo cp config/security.xml /etc/dds/

# 设置环境变量
echo 'export DDS_CONFIG_FILE=/etc/dds/dds_config.xml' | sudo tee /etc/profile.d/dds.sh
echo 'export DDS_DOMAIN_ID=0' | sudo tee -a /etc/profile.d/dds.sh

# 创建启动服务
sudo cp scripts/eth-dds.service /etc/systemd/system/
sudo systemctl enable eth-dds
```

### 6.2.6 验证

```bash
# 检查服务状态
sudo systemctl status eth-dds

# 查看日志
sudo journalctl -u eth-dds -f

# 测试基本功能
/usr/local/bin/dds_ping

# 验证网络通信
/usr/local/bin/dds_test_pub
/usr/local/bin/dds_test_sub
```

## 6.3 更新管理

### 6.3.1 OTA更新流程

```
┌─────────────────────────────────────────────────────────────────┐
│                      OTA Update Flow                                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                        Update Server                                 │   │
│  │  1. Prepare Update Package                                          │   │
│  │  2. Sign Package                                                    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│       │                                                    │              │
│       │ 3. Download                                        │              │
│       │─────────────────────────────────────────────────────┾  │              │
│       │                                                    │              │
│  ┌────┴────────────────────────────────────────────────────────────┘   │
│  │                        Target Device                                │   │
│  │  4. Verify Signature                                               │   │
│  │  5. Install to Staging Area                                        │   │
│  │  6. Activate New Version                                           │   │
│  │  7. Test                                                           │   │
│  │  8. Commit or Rollback                                             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────┘
```

### 6.3.2 更新脚本示例

```bash
#!/bin/bash
# update_eth_dds.sh - OTA更新脚本

UPDATE_PACKAGE="$1"
STAGING_DIR="/tmp/eth_dds_update"
BACKUP_DIR="/opt/eth_dds_backup"
INSTALL_DIR="/opt/eth_dds"

echo "Starting ETH-DDS update..."

# 1. 验证签名
if ! verify_signature "$UPDATE_PACKAGE"; then
    echo "ERROR: Signature verification failed"
    exit 1
fi

# 2. 创建临时目录
rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR"

# 3. 解压更新包
tar -xzf "$UPDATE_PACKAGE" -C "$STAGING_DIR"

# 4. 备份当前版本
cp -r "$INSTALL_DIR" "$BACKUP_DIR.$(date +%Y%m%d_%H%M%S)"

# 5. 安装新版本
cp -r "$STAGING_DIR"/* "$INSTALL_DIR/"

# 6. 重启服务
systemctl restart eth-dds

# 7. 验证更新
if /opt/eth_dds/bin/dds_ping; then
    echo "Update successful!"
    rm -rf "$STAGING_DIR"
else
    echo "Update failed, rolling back..."
    # 执行回滚
    ./rollback.sh
    exit 1
fi
```

## 6.4 生产环境配置

### 6.4.1 systemd服务

创建 `/etc/systemd/system/eth-dds.service`:

```ini
[Unit]
Description=ETH-DDS Integration Framework
After=network.target

[Service]
Type=simple
User=dds
Group=dds
Environment="DDS_CONFIG_FILE=/etc/dds/dds_config.xml"
Environment="DDS_DOMAIN_ID=0"
Environment="DDS_LOG_LEVEL=INFO"
Environment="DDS_LOG_FILE=/var/log/dds/dds.log"
ExecStart=/usr/local/bin/eth_dds_daemon
ExecReload=/bin/kill -HUP $MAINPID
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### 6.4.2 日志旋转

创建 `/etc/logrotate.d/eth-dds`:

```
/var/log/dds/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 dds dds
    sharedscripts
    postrotate
        /bin/kill -HUP $(cat /var/run/syslogd.pid 2> /dev/null) 2> /dev/null || true
    endscript
}
```

### 6.4.3 安全硬化

```bash
# 创建专用用户
sudo useradd -r -s /bin/false dds

# 设置文件权限
sudo chown -R dds:dds /opt/eth_dds
sudo chown -R dds:dds /etc/dds
sudo chown -R dds:dds /var/log/dds
sudo chmod 750 /opt/eth_dds
sudo chmod 750 /etc/dds

# 设置容器限制
sudo systemctl set-property eth-dds.service \
    MemoryMax=512M \
    CPUQuota=50% \
    TasksMax=100
```

---

**版本**: 2.0.0  
**上一章**: @ref debugging  
**下一章**: @ref security_guide
