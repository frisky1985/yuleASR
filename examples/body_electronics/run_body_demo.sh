#!/bin/bash
###############################################################################
# Body Electronics Demo Runner
###############################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/build/bin"

cleanup() {
    echo "Shutting down Body Electronics demo..."
    kill $SEAT_PID $HVAC_PID $DOOR_PID $LIGHT_PID 2>/dev/null || true
    wait 2>/dev/null || true
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo "========================================"
echo "  Body Electronics System Demo"
echo "  AUTOSAR DDS Integration"
echo "  ASIL-B Safety Level"
echo "========================================"
echo ""

mkdir -p "${SCRIPT_DIR}/logs"

${BIN_DIR}/seat_controller > "${SCRIPT_DIR}/logs/seat.log" 2>&1 &
SEAT_PID=$!

${BIN_DIR}/hvac_controller > "${SCRIPT_DIR}/logs/hvac.log" 2>&1 &
HVAC_PID=$!

${BIN_DIR}/door_controller > "${SCRIPT_DIR}/logs/door.log" 2>&1 &
DOOR_PID=$!

${BIN_DIR}/lighting_controller > "${SCRIPT_DIR}/logs/lighting.log" 2>&1 &
LIGHT_PID=$!

echo "Components started:"
echo "  - Seat Controller (PID: $SEAT_PID)"
echo "  - HVAC Controller (PID: $HVAC_PID)"
echo "  - Door Controller (PID: $DOOR_PID)"
echo "  - Lighting Controller (PID: $LIGHT_PID)"
echo ""
echo "Press Ctrl+C to stop"

wait $SEAT_PID $HVAC_PID $DOOR_PID $LIGHT_PID
