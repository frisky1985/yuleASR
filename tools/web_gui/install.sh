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
    
    # Update service file with correct paths
    sed -i "s|/home/admin/eth-dds-integration/tools/web_gui|$SCRIPT_DIR|g" dds-web-gui.service
    sed -i "s|User=admin|User=$USER|g" dds-web-gui.service
    
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
echo -e "  - Change default credentials in server/app.py"
echo -e "  - Set SECRET_KEY and JWT_SECRET_KEY environment variables"
echo -e "  - Configure HTTPS with reverse proxy (nginx/apache)"
echo -e "${GREEN}========================================${NC}"
