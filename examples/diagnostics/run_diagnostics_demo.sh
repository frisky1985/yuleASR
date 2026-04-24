#!/bin/bash
###############################################################################
# Diagnostics System Demo Runner
###############################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/build/bin"

cleanup() {
    echo "Shutting down Diagnostics demo..."
    kill $DTC_PID $ROUTINE_PID 2>/dev/null || true
    wait 2>/dev/null || true
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo "========================================"
echo "  Diagnostics System Demo"
echo "  AUTOSAR DDS Integration"
echo "  ASIL-B Safety Level"
echo "========================================"
echo ""

mkdir -p "${SCRIPT_DIR}/logs"

${BIN_DIR}/dtc_manager > "${SCRIPT_DIR}/logs/dtc.log" 2>&1 &
DTC_PID=$!

${BIN_DIR}/routine_controller > "${SCRIPT_DIR}/logs/routine.log" 2>&1 &
ROUTINE_PID=$!

echo "Components started:"
echo "  - DTC Manager (PID: $DTC_PID) - 1 Hz"
echo "  - Routine Controller (PID: $ROUTINE_PID) - 2 Hz"
echo ""
echo "Press Ctrl+C to stop"

wait $DTC_PID $ROUTINE_PID
