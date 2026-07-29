#!/usr/bin/env bash
#=============================================================================
# flash.sh — Flash yuleASR firmware to S32K312
#
# Usage:
#   ./scripts/flash.sh [file.elf]
#   ./scripts/flash.sh [file.hex]
#   ./scripts/flash.sh              # default: build/yuleasr.hex
#
# Supported debuggers: OpenOCD or JLink
#=============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default firmware path
DEFAULT_FW="$PROJECT_DIR/build/yuleasr.hex"
FW="${1:-$DEFAULT_FW}"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Validate firmware file
if [ ! -f "$FW" ]; then
    echo -e "${RED}Error: firmware file not found: $FW${NC}"
    echo "Usage: $0 [path/to/firmware.elf|.hex]"
    echo ""
    echo "Build firmware first:"
    echo "  cmake --build build"
    exit 1
fi

# Detect firmware format and compute offset
FW_EXT="${FW##*.}"
if [ "$FW_EXT" = "elf" ]; then
    FLASH_ADDR="0x00400000"
    echo -e "${YELLOW}ELF format detected. Using load_image (addr: $FLASH_ADDR).${NC}"
elif [ "$FW_EXT" = "hex" ]; then
    FLASH_ADDR=""
    echo -e "${YELLOW}HEX format detected. Addresses embedded in file.${NC}"
elif [ "$FW_EXT" = "bin" ]; then
    FLASH_ADDR="0x00400000"
    echo -e "${YELLOW}BIN format detected. Using load_image (addr: $FLASH_ADDR).${NC}"
else
    FLASH_ADDR="0x00400000"
    echo -e "${YELLOW}Unknown format. Treating as raw binary (addr: $FLASH_ADDR).${NC}"
fi

# Choose flashing method
flash_with_openocd() {
    echo -e "${GREEN}Flashing with OpenOCD...${NC}"
    
    local openocd_cfg="$SCRIPT_DIR/../scripts/openocd_s32k312.cfg"
    if [ ! -f "$openocd_cfg" ]; then
        echo -e "${RED}Error: OpenOCD config not found: $openocd_cfg${NC}"
        return 1
    fi
    
    if [ "$FW_EXT" = "hex" ]; then
        openocd -f "$openocd_cfg" \
            -c "program \"$FW\"" \
            -c "reset" \
            -c "exit"
    elif [ "$FW_EXT" = "elf" ]; then
        openocd -f "$openocd_cfg" \
            -c "program \"$FW\" $FLASH_ADDR" \
            -c "reset" \
            -c "exit"
    else
        openocd -f "$openocd_cfg" \
            -c "program \"$FW\" $FLASH_ADDR" \
            -c "reset" \
            -c "exit"
    fi
    
    local ret=$?
    if [ $ret -eq 0 ]; then
        echo -e "${GREEN}✓ Flash complete via OpenOCD${NC}"
    else
        echo -e "${RED}✗ OpenOCD flash failed (exit code: $ret)${NC}"
    fi
    return $ret
}

flash_with_jlink() {
    echo -e "${GREEN}Flashing with JLink...${NC}"
    
    local jlink_script="$PROJECT_DIR/scripts/flash.jlink"
    cat > "$jlink_script" << EOF
h
loadfile "$FW"
r
g
exit
EOF
    
    JLinkExe -device S32K312 -if SWD -speed 4000 -autoconnect 1 \
        -CommanderScript "$jlink_script"
    
    local ret=$?
    rm -f "$jlink_script"
    
    if [ $ret -eq 0 ]; then
        echo -e "${GREEN}✓ Flash complete via JLink${NC}"
    else
        echo -e "${RED}✗ JLink flash failed (exit code: $ret)${NC}"
    fi
    return $ret
}

# Try OpenOCD first, fallback to JLink
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN} yuleASR Firmware Flasher for S32K312${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Firmware: $FW"
echo ""

if command -v openocd &>/dev/null; then
    flash_with_openocd || flash_with_jlink
elif command -v JLinkExe &>/dev/null; then
    flash_with_jlink
else
    echo -e "${RED}Error: Neither OpenOCD nor JLink found.${NC}"
    echo "Install one of:"
    echo "  macOS: brew install openocd"
    echo "  Ubuntu: sudo apt install openocd"
    echo "  Or download SEGGER JLink from https://www.segger.com"
    exit 1
fi
