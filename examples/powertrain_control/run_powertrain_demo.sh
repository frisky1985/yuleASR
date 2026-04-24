#!/bin/bash
###############################################################################
# Powertrain Control System Demo Runner
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
    print_info "Shutting down Powertrain demo..."
    
    if [ -n "$MOTOR_PID" ]; then
        kill $MOTOR_PID 2>/dev/null || true
        wait $MOTOR_PID 2>/dev/null || true
    fi
    
    if [ -n "$BMS_PID" ]; then
        kill $BMS_PID 2>/dev/null || true
        wait $BMS_PID 2>/dev/null || true
    fi
    
    if [ -n "$VCU_PID" ]; then
        kill $VCU_PID 2>/dev/null || true
        wait $VCU_PID 2>/dev/null || true
    fi
    
    if [ -n "$REGEN_PID" ]; then
        kill $REGEN_PID 2>/dev/null || true
        wait $REGEN_PID 2>/dev/null || true
    fi
    
    print_success "Powertrain demo stopped"
    exit 0
}

# Trap signals
trap cleanup SIGINT SIGTERM EXIT

# Print banner
print_banner() {
    echo ""
    echo "========================================"
    echo "  Powertrain Control System Demo"
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
    for binary in motor_publisher bms_publisher vcu_publisher regen_manager; do
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
    
    print_info "Starting Powertrain control system..."
    print_info "Press Ctrl+C to stop\n"
    
    # Start components
    MOTOR_PID=$(run_component "Motor Publisher" "motor_publisher" "${SCRIPT_DIR}/logs/motor.log")
    sleep 1
    
    BMS_PID=$(run_component "BMS Publisher" "bms_publisher" "${SCRIPT_DIR}/logs/bms.log")
    sleep 1
    
    VCU_PID=$(run_component "VCU Publisher" "vcu_publisher" "${SCRIPT_DIR}/logs/vcu.log")
    sleep 1
    
    REGEN_PID=$(run_component "Regen Manager" "regen_manager" "${SCRIPT_DIR}/logs/regen.log")
    
    print_success "All components started successfully!"
    print_info ""
    print_info "Components:"
    print_info "  - Motor Publisher (PID: $MOTOR_PID) - 100 Hz"
    print_info "  - BMS Publisher (PID: $BMS_PID) - 10 Hz"
    print_info "  - VCU Publisher (PID: $VCU_PID) - 50 Hz"
    print_info "  - Regen Manager (PID: $REGEN_PID) - 50 Hz"
    print_info ""
    print_info "Logs directory: ${SCRIPT_DIR}/logs"
    print_info "Press Ctrl+C to stop the demo"
    print_info ""
    
    # Wait for all processes
    wait $MOTOR_PID $BMS_PID $VCU_PID $REGEN_PID
}

# Run main
main "$@"
