#!/bin/bash
# DDS Web GUI Installation Script

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  DDS Web GUI Installation${NC}"
echo -e "${BLUE}========================================${NC}"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check Python version
echo -e "\n${YELLOW}Checking Python version...${NC}"
PYTHON_VERSION=$(python3 --version 2>&1 | awk '{print $2}')
REQUIRED_VERSION="3.8"

if [ "$(printf '%s\n' "$REQUIRED_VERSION" "$PYTHON_VERSION" | sort -V | head -n1)" != "$REQUIRED_VERSION" ]; then 
    echo -e "${RED}Error: Python 3.8 or higher is required (found $PYTHON_VERSION)${NC}"
    exit 1
fi

echo -e "${GREEN}Python version $PYTHON_VERSION OK${NC}"

# Create virtual environment
echo -e "\n${YELLOW}Creating virtual environment...${NC}"
if [ -d "venv" ]; then
    echo -e "${YELLOW}Virtual environment exists, removing old one...${NC}"
    rm -rf venv
fi

python3 -m venv venv
source venv/bin/activate

# Upgrade pip
echo -e "\n${YELLOW}Upgrading pip...${NC}"
pip install --upgrade pip

# Install dependencies
echo -e "\n${YELLOW}Installing Python dependencies...${NC}"
pip install -r server/requirements.txt

# Create necessary directories
echo -e "\n${YELLOW}Creating directories...${NC}"
mkdir -p logs
mkdir -p uploads
mkdir -p data

# Set permissions
echo -e "\n${YELLOW}Setting permissions...${NC}"
chmod +x start_server.sh

# Create systemd service file (optional)
read -p "Install systemd service? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo -e "\n${YELLOW}Installing systemd service...${NC}"
    
    # Update service file with correct paths (P2-10: 占位符替换, 不再依赖硬编码路径)
    sed -i "s|__WEBGUI_DIR__|$SCRIPT_DIR|g" dds-web-gui.service
    sed -i "s|__RUN_USER__|$USER|g" dds-web-gui.service
    
    sudo mkdir -p /etc/eth-dds
    # 生产密钥: 首次安装生成随机 SECRET_KEY / JWT_SECRET_KEY (缺失时 app 拒绝启动)
    if [ ! -f /etc/eth-dds/web-gui.env ]; then
        umask 077
        echo "SECRET_KEY=$(openssl rand -hex 32)" | sudo tee /etc/eth-dds/web-gui.env > /dev/null
        echo "JWT_SECRET_KEY=$(openssl rand -hex 32)" | sudo tee -a /etc/eth-dds/web-gui.env > /dev/null
        sudo chown root:root /etc/eth-dds/web-gui.env
        sudo chmod 600 /etc/eth-dds/web-gui.env
    fi
    
    sudo cp dds-web-gui.service /etc/systemd/system/
    sudo systemctl daemon-reload
    sudo systemctl enable dds-web-gui
    
    echo -e "${GREEN}Systemd service installed${NC}"
    echo -e "${YELLOW}Start with: sudo systemctl start dds-web-gui${NC}"
fi

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}  Installation Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "\n${YELLOW}Next steps:${NC}"
echo -e "  1. Start the server: ./start_server.sh"
echo -e "  2. Open browser: http://localhost:5000"
echo -e "  3. Login with default credentials:"
echo -e "     - admin / admin123"
echo -e "     - operator / operator123"
echo -e "     - viewer / viewer123"
echo -e "\n${YELLOW}For production:${NC}"
echo -e "  - 修改默认凭据 (server/app.py 或接入认证后端)"
echo -e "  - SECRET_KEY / JWT_SECRET_KEY 已由 install.sh 写入 /etc/eth-dds/web-gui.env"
echo -e "    (权限 600, 从环境变量读取; 缺失时 app 拒绝启动, 无模板默认值)"
echo -e "  - DDS_CONFIG_PATH 指向 dds-config-tool 生成的 config.yaml (见 service 注释)"
echo -e "  - 配置 HTTPS 反向代理 (nginx/apache)"
echo -e "${GREEN}========================================${NC}"
