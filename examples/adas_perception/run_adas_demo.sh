#!/bin/bash
###############################################################################
# ADAS Perception System Demo Runner
# AUTOSAR DDS Integration
###############################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"

# Function to print colored messages
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to cleanup processes on exit
cleanup() {
    print_info "Shutting down ADAS demo..."
    
    if [ -n "$LIDAR_PID" ]; then
        kill $LIDAR_PID 2>/dev/null || true
        wait $LIDAR_PID 2>/dev/null || true
    fi
    
    if [ -n "$CAMERA_PID" ]; then
        kill $CAMERA_PID 2>/dev/null || true
        wait $CAMERA_PID 2>/dev/null || true
    fi
    
    if [ -n "$FUSION_PID" ]; then
        kill $FUSION_PID 2>/dev/null || true
        wait $FUSION_PID 2>/dev/null || true
    fi
    
    if [ -n "$DETECTOR_PID" ]; then
        kill $DETECTOR_PID 2>/dev/null || true
        wait $DETECTOR_PID 2>/dev/null || true
    fi
    
    print_success "ADAS demo stopped"
    exit 0
}

# Trap signals
trap cleanup SIGINT SIGTERM EXIT

# Print banner
print_banner() {
    echo ""
    echo "========================================"
    echo "  ADAS Perception System Demo"
    echo "  AUTOSAR DDS Integration"
    echo "  ASIL-D Safety Level"
    echo "========================================"
    echo ""
}

# Check if binaries exist
check_binaries() {
    if [ ! -d "$BIN_DIR" ]; then
        print_error "Build directory not found. Please build first:"
        print_info "  mkdir build && cd build && cmake .. && make"
        exit 1
    fi
    
    local missing=0
    for binary in lidar_publisher camera_publisher sensor_fusion object_detector; do
        if [ ! -f "${BIN_DIR}/${binary}" ]; then
            print_error "Binary not found: ${binary}"
            missing=1
        fi
    done
    
    if [ $missing -eq 1 ]; then
        print_error "Please build the project first"
        exit 1
    fi
    
    print_success "All binaries found"
}

# Function to run component in background
run_component() {
    local name=$1
    local binary=$2
    local log_file=$3
    
    print_info "Starting ${name}..."
    "${BIN_DIR}/${binary}" > "${log_file}" 2>&1 &
    local pid=$!
    
    # Wait a moment to check if process started
    sleep 1
    if ! kill -0 $pid 2>/dev/null; then
        print_error "Failed to start ${name}"
        cat "${log_file}" || true
        return 1
    fi
    
    print_success "${name} started (PID: $pid)"
    echo $pid
}

# Main execution
main() {
    print_banner
    
    # Check binaries
    check_binaries
    
    # Create log directory
    mkdir -p "${SCRIPT_DIR}/logs"
    
    print_info "Starting ADAS perception pipeline..."
    print_info "Press Ctrl+C to stop\n"
    
    # Start components
    LIDAR_PID=$(run_component "LiDAR Publisher" "lidar_publisher" "${SCRIPT_DIR}/logs/lidar.log")
    sleep 2
    
    CAMERA_PID=$(run_component "Camera Publisher" "camera_publisher" "${SCRIPT_DIR}/logs/camera.log")
    sleep 2
    
    FUSION_PID=$(run_component "Sensor Fusion" "sensor_fusion" "${SCRIPT_DIR}/logs/fusion.log")
    sleep 2
    
    DETECTOR_PID=$(run_component "Object Detector" "object_detector" "${SCRIPT_DIR}/logs/detector.log")
    
    print_success "All components started successfully!"
    print_info ""
    print_info "Components:"
    print_info "  - LiDAR Publisher (PID: $LIDAR_PID)"
    print_info "  - Camera Publisher (PID: $CAMERA_PID)"
    print_info "  - Sensor Fusion (PID: $FUSION_PID)"
    print_info "  - Object Detector (PID: $DETECTOR_PID)"
    print_info ""
    print_info "Logs directory: ${SCRIPT_DIR}/logs"
    print_info "Press Ctrl+C to stop the demo"
    print_info ""
    
    # Wait for all processes
    wait $LIDAR_PID $CAMERA_PID $FUSION_PID $DETECTOR_PID
}

# Run main
main "$@"
