#!/bin/bash
# DDS Web GUI Server Startup Script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting DDS Web GUI Server...${NC}"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check Python version
PYTHON_VERSION=$(python3 --version 2>&1 | awk '{print $2}')
echo -e "${YELLOW}Python version: $PYTHON_VERSION${NC}"

# Check if virtual environment exists, create if not
if [ ! -d "venv" ]; then
    echo -e "${YELLOW}Creating virtual environment...${NC}"
    python3 -m venv venv
fi

# Activate virtual environment
echo -e "${YELLOW}Activating virtual environment...${NC}"
source venv/bin/activate

# Install/update dependencies
echo -e "${YELLOW}Installing dependencies...${NC}"
pip install -q --upgrade pip
pip install -q -r server/requirements.txt

# Set environment variables
export FLASK_APP=server/app.py
export FLASK_ENV=development
export PYTHONPATH="${SCRIPT_DIR}:${PYTHONPATH}"

# Create necessary directories
mkdir -p logs
mkdir -p uploads

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  DDS Web GUI Server Started${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${YELLOW}Dashboard URL: http://localhost:5000${NC}"
echo -e "${YELLOW}API Base: http://localhost:5000/api/${NC}"
echo -e "${YELLOW}WebSocket: ws://localhost:5000${NC}"
echo -e ""
echo -e "${YELLOW}Default credentials:${NC}"
echo -e "  Admin:    admin / admin123"
echo -e "  Operator: operator / operator123"
echo -e "  Viewer:   viewer / viewer123"
echo -e ""
echo -e "${YELLOW}Press Ctrl+C to stop${NC}"
echo -e "${GREEN}========================================${NC}"

# Start server
cd server
python app.py

# Deactivate virtual environment on exit
deactivate