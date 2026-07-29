#!/usr/bin/env bash
#=============================================================================
# debug.sh — Debug yuleASR firmware on S32K312
#
# Usage:
#   ./scripts/debug.sh                  # GDB + OpenOCD
#   ./scripts/debug.sh jlink            # GDB + JLinkGDBServer
#   ./scripts/debug.sh [file.elf]       # Custom ELF
#=============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default firmware
DEFAULT_ELF="$PROJECT_DIR/build/yuleasr.elf"
ELF="${2:-$DEFAULT_ELF}"

DEBUG_MODE="${1:-openocd}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ ! -f "$ELF" ]; then
    echo -e "${RED}Error: ELF not found: $ELF${NC}"
    echo "Build firmware first: cmake --build build"
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN} yuleASR Debug Session for S32K312${NC}"
echo -e "${GREEN}========================================${NC}"
echo "ELF: $ELF"
echo "Debug mode: $DEBUG_MODE"
echo ""

debug_with_openocd() {
    echo -e "${YELLOW}Starting OpenOCD in background...${NC}"
    
    local openocd_cfg="$PROJECT_DIR/scripts/openocd_s32k312.cfg"
    if [ ! -f "$openocd_cfg" ]; then
        echo -e "${RED}Error: OpenOCD config not found: $openocd_cfg${NC}"
        exit 1
    fi
    
    # Start OpenOCD in background
    openocd -f "$openocd_cfg" &
    OPENOCD_PID=$!
    echo "OpenOCD PID: $OPENOCD_PID"
    
    # Wait for OpenOCD to be ready
    sleep 2
    
    # Start GDB
    echo -e "${GREEN}Starting GDB...${NC}"
    arm-none-eabi-gdb "$ELF" \
        -ex "target extended-remote :3333" \
        -ex "monitor reset halt" \
        -ex "load" \
        -ex "monitor reset" \
        -ex "break main" \
        -ex "continue"
    
    # Cleanup
    kill $OPENOCD_PID 2>/dev/null || true
}

debug_with_jlink() {
    echo -e "${YELLOW}Starting JLinkGDBServer in background...${NC}"
    
    # Start JLink GDB server
    JLinkGDBServer -device S32K312 -if SWD -speed 4000 -port 2331 &
    JLINK_PID=$!
    echo "JLinkGDBServer PID: $JLINK_PID"
    
    # Wait for server
    sleep 2
    
    # Start GDB
    echo -e "${GREEN}Starting GDB...${NC}"
    arm-none-eabi-gdb "$ELF" \
        -ex "target remote :2331" \
        -ex "monitor reset" \
        -ex "load" \
        -ex "break main" \
        -ex "continue"
    
    # Cleanup
    kill $JLINK_PID 2>/dev/null || true
}

case "$DEBUG_MODE" in
    openocd)
        debug_with_openocd
        ;;
    jlink)
        debug_with_jlink
        ;;
    *)
        echo -e "${RED}Unknown debug mode: $DEBUG_MODE${NC}"
        echo "Usage: $0 [openocd|jlink] [elf_file]"
        exit 1
        ;;
esac
