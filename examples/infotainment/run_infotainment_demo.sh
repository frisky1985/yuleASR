#!/bin/bash
###############################################################################
# Infotainment System Demo Runner
###############################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/build/bin"

cleanup() {
    echo "Shutting down Infotainment demo..."
    kill $HUD_PID $NAV_PID $MEDIA_PID 2>/dev/null || true
    wait 2>/dev/null || true
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo "========================================"
echo "  Infotainment System Demo"
echo "  AUTOSAR DDS Integration"
echo "  QM (Non-Safety)"
echo "========================================"
echo ""

mkdir -p "${SCRIPT_DIR}/logs"

${BIN_DIR}/hud_publisher > "${SCRIPT_DIR}/logs/hud.log" 2>&1 &
HUD_PID=$!

${BIN_DIR}/navigation_publisher > "${SCRIPT_DIR}/logs/navigation.log" 2>&1 &
NAV_PID=$!

${BIN_DIR}/media_controller > "${SCRIPT_DIR}/logs/media.log" 2>&1 &
MEDIA_PID=$!

echo "Components started:"
echo "  - HUD Publisher (PID: $HUD_PID) - 30 Hz"
echo "  - Navigation (PID: $NAV_PID) - 5 Hz"
echo "  - Media Controller (PID: $MEDIA_PID) - 10 Hz"
echo ""
echo "Press Ctrl+C to stop"

wait $HUD_PID $NAV_PID $MEDIA_PID
