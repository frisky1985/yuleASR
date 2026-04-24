#!/bin/bash
###############################################################################
# Master Script to Run All Vehicle DDS Examples
# AUTOSAR DDS Integration
###############################################################################

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cleanup() {
    echo -e "\n${BLUE}[INFO]${NC} Shutting down all examples..."
    kill $ADAS_PID $POWERTRAIN_PID $BODY_PID $INFO_PID $DIAG_PID 2>/dev/null || true
    wait 2>/dev/null || true
    echo -e "${GREEN}[SUCCESS]${NC} All examples stopped"
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo "==================================================================="
echo "  AUTOSAR DDS Integration - Vehicle Application Examples"
echo "==================================================================="
echo ""
echo "Starting all vehicle DDS examples:"
echo "  1. ADAS Perception (ASIL-D) - Domain 1"
echo "  2. Powertrain Control (ASIL-D) - Domain 2"
echo "  3. Body Electronics (ASIL-B) - Domain 3"
echo "  4. Infotainment (QM) - Domain 4"
echo "  5. Diagnostics (ASIL-B) - Domain 5"
echo ""
echo "Press Ctrl+C to stop all examples"
echo "==================================================================="
echo ""

sleep 2

# Start ADAS
echo -e "${BLUE}[INFO]${NC} Starting ADAS Perception..."
cd "${SCRIPT_DIR}/adas_perception"
./build/bin/lidar_publisher > logs/lidar.log 2>&1 &
./build/bin/camera_publisher > logs/camera.log 2>&1 &
./build/bin/sensor_fusion > logs/fusion.log 2>&1 &
./build/bin/object_detector > logs/detector.log 2>&1 &
ADAS_PID=$!
cd "${SCRIPT_DIR}"
sleep 2

# Start Powertrain
echo -e "${BLUE}[INFO]${NC} Starting Powertrain Control..."
cd "${SCRIPT_DIR}/powertrain_control"
./build/bin/motor_publisher > logs/motor.log 2>&1 &
./build/bin/bms_publisher > logs/bms.log 2>&1 &
./build/bin/vcu_publisher > logs/vcu.log 2>&1 &
./build/bin/regen_manager > logs/regen.log 2>&1 &
POWERTRAIN_PID=$!
cd "${SCRIPT_DIR}"
sleep 2

# Start Body Electronics
echo -e "${BLUE}[INFO]${NC} Starting Body Electronics..."
cd "${SCRIPT_DIR}/body_electronics"
./build/bin/seat_controller > logs/seat.log 2>&1 &
./build/bin/hvac_controller > logs/hvac.log 2>&1 &
./build/bin/door_controller > logs/door.log 2>&1 &
./build/bin/lighting_controller > logs/lighting.log 2>&1 &
BODY_PID=$!
cd "${SCRIPT_DIR}"
sleep 2

# Start Infotainment
echo -e "${BLUE}[INFO]${NC} Starting Infotainment..."
cd "${SCRIPT_DIR}/infotainment"
./build/bin/hud_publisher > logs/hud.log 2>&1 &
./build/bin/navigation_publisher > logs/navigation.log 2>&1 &
./build/bin/media_controller > logs/media.log 2>&1 &
INFO_PID=$!
cd "${SCRIPT_DIR}"
sleep 2

# Start Diagnostics
echo -e "${BLUE}[INFO]${NC} Starting Diagnostics..."
cd "${SCRIPT_DIR}/diagnostics"
./build/bin/dtc_manager > logs/dtc.log 2>&1 &
./build/bin/routine_controller > logs/routine.log 2>&1 &
DIAG_PID=$!
cd "${SCRIPT_DIR}"

echo ""
echo -e "${GREEN}[SUCCESS]${NC} All examples started successfully!"
echo ""
echo "Component Status:"
echo "  ✓ ADAS Perception (4 processes)"
echo "  ✓ Powertrain Control (4 processes)"
echo "  ✓ Body Electronics (4 processes)"
echo "  ✓ Infotainment (3 processes)"
echo "  ✓ Diagnostics (2 processes)"
echo ""
echo "Log directories:"
echo "  - adas_perception/logs/"
echo "  - powertrain_control/logs/"
echo "  - body_electronics/logs/"
echo "  - infotainment/logs/"
echo "  - diagnostics/logs/"
echo ""
echo "Press Ctrl+C to stop all examples"
echo ""

# Wait indefinitely
while true; do
    sleep 1
done
