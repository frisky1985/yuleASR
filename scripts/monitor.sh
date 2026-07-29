#!/usr/bin/env bash
#=============================================================================
# monitor.sh — Serial monitor for S32K312 UART output
#
# Usage:
#   ./scripts/monitor.sh
#   ./scripts/monitor.sh /dev/tty.usbserial-XXXX 115200
#=============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BAUD="${2:-115200}"

# Auto-detect serial device
detect_serial() {
    local candidates=()
    
    # macOS
    if [[ "$OSTYPE" == "darwin"* ]]; then
        candidates+=(/dev/cu.usbmodem*)
        candidates+=(/dev/cu.usbserial-*)
        candidates+=(/dev/cu.wchusbserial*)
        candidates+=(/dev/cu.SLAB_USBtoUART*)
    # Linux
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        candidates+=(/dev/ttyUSB*)
        candidates+=(/dev/ttyACM*)
    fi
    
    for dev in "${candidates[@]}"; do
        if [ -e "$dev" ]; then
            echo "$dev"
            return 0
        fi
    done
    
    return 1
}

# Select serial device
if [ -n "${1:-}" ]; then
    SERIAL_DEV="$1"
else
    SERIAL_DEV=$(detect_serial) || true
    if [ -z "$SERIAL_DEV" ]; then
        echo "Error: No serial device found."
        echo "Usage: $0 [device] [baud]"
        echo ""
        echo "Available devices:"
        if [[ "$OSTYPE" == "darwin"* ]]; then
            ls -1 /dev/cu.* 2>/dev/null || echo "  (none found)"
        else
            ls -1 /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  (none found)"
        fi
        exit 1
    fi
fi

# Validate device
if [ ! -e "$SERIAL_DEV" ]; then
    echo "Error: Device $SERIAL_DEV not found."
    exit 1
fi

echo "========================================"
echo " yuleASR Serial Monitor"
echo "========================================"
echo "Device: $SERIAL_DEV"
echo "Baud:   $BAUD"
echo ""
echo "Press Ctrl+A then Ctrl+Q to exit (screen)"
echo "Press Ctrl+C to exit (picocom)"
echo "========================================"
echo ""

# Choose monitor tool
if command -v picocom &>/dev/null; then
    exec picocom -b "$BAUD" "$SERIAL_DEV"
elif command -v screen &>/dev/null; then
    exec screen "$SERIAL_DEV" "$BAUD"
elif command -v minicom &>/dev/null; then
    exec minicom -D "$SERIAL_DEV" -b "$BAUD"
else
    echo "No serial terminal found. Install one of:"
    echo "  brew install picocom    (macOS)"
    echo "  sudo apt install picocom (Linux)"
    echo ""
    echo "Falling back to cat + stty..."
    stty -F "$SERIAL_DEV" "$BAUD" cs8 -cstopb -parenb raw
    cat "$SERIAL_DEV"
fi
